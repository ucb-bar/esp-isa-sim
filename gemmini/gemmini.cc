#include "gemmini.h"
#include "mmu.h"
#include "trap.h"
#include <stdexcept>
#include <iostream>
#include <assert.h>

REGISTER_EXTENSION(gemmini, []() { return new gemmini_t; })

void gemmini_state_t::reset()
{
  enable = true;
  // mode = OS;
  // act = NONE;
  // sys_shift = 0;
  // relu6_shift = 0;
  // output_sp_addr = 0;
  // load_stride = DIM * sizeof(elem_t);
  // store_stride = DIM * sizeof(elem_t);
  // pool_stride = 0;
  // load_shrunk = false;

  spad.clear();
  spad.resize(sp_matrices*DIM, std::vector<elem_t>(DIM, 0));

  pe_state.clear();
  pe_state.resize(DIM, std::vector<acc_t>(DIM));

  accumulator.clear();
  accumulator.resize(accum_rows, std::vector<acc_t>(DIM, 0));

  // cisc reset
  a_addr = b_addr = c_addr = d_addr = 0;
  m = n = k = 0;
  repeating_bias = false;

  printf("Gemmini extension configured with:\n");
  printf("    dim = %u\n", DIM);
}

void gemmini_t::reset() {
  gemmini_state.reset();
}

template <class T>
T gemmini_t::read_from_dram(reg_t addr) {
  T value = 0;
  for (size_t byte_idx = 0; byte_idx < sizeof(T); ++byte_idx) {
    value |= p->get_mmu()->load_uint8(addr + byte_idx) << (byte_idx*8);
  }
  return value;
}

template <class T>
std::vector<std::vector<T>> *
matrix_zeroes(reg_t rows, reg_t cols) {
  return new std::vector<std::vector<T>>(rows, std::vector<T>(cols, 0));
}

template <class T>
std::vector<std::vector<T>> *
gemmini_t::read_matrix_from_dram(reg_t addr, reg_t rows, reg_t cols,
                                  bool zeroable, bool repeating_bias) {
  // Read and return Matrix of size `rows*cols` from address `addr` in main
  // memory

  // Initialize to all zeroes
  auto result = matrix_zeroes<T>(rows, cols);

  // if an input matrix is at addr 0, it is NULL, so don't do anything with
  // it only the D matrix is zeroable; the A, B matrices must be valid
  if(addr == 0) {
    if(zeroable) {
      return result;
    }
    printf("ERROR: non-zeroable matrix given address zero!\n");
    exit(1);
  }

  // Load from memory
  for (size_t i = 0; i < rows; i++) {
    auto ii = repeating_bias ? 0 : i;
    auto const dram_row_addr = addr + ii*sizeof(T)*cols;
    for (size_t j = 0; j < cols; j++) {
      auto const dram_byte_addr = dram_row_addr + j*sizeof(T);
#ifdef ELEM_T_IS_FLOAT
      result->at(i).at(j) = elem_t_bits_to_elem_t(gemmini_t::read_from_dram<elem_t_bits>(dram_byte_addr));
#else
      result->at(i).at(j) = gemmini_t::read_from_dram<elem_t>(dram_byte_addr);
#endif
    }
  }
  return result;
}

template <class T>
void gemmini_t::write_to_dram(reg_t addr, T data) {
  for (size_t byte_idx = 0; byte_idx < sizeof(T); ++byte_idx) {
    p->get_mmu()->store_uint8(addr + byte_idx, (data >> (byte_idx*8)) & 0xFF);
  }
}

// Move a gemmini block from DRAM at dram_addr (byte addr) to
// the scratchpad/accumulator at sp_addr (gemmini-row addressed)
void gemmini_t::mvin(reg_t dram_addr, reg_t sp_addr, int state_id) {
  bool const accumulator = (sp_addr >> 31) & 0x1;
  bool const accumulate = (sp_addr >> 30) & 0x1;
  auto const base_row_addr = (sp_addr & 0x1FFFFFFF); // Strip accumulator addressing bits [31:29]
  auto const cols = (sp_addr >> addr_len) & 0xFFFF;
  auto const rows = (sp_addr >> (addr_len + 16)) & 0xFFFF;

  auto const load_stride = gemmini_state.load_strides[state_id];
  auto const load_shrunk = gemmini_state.load_shrunks[state_id];
  auto const load_scale = gemmini_state.load_scales[state_id];

  dprintf("GEMMINI: mvin - 0x%02lx cols and 0x%02lx rows from 0x%08lx to addr 0x%08lx\n", cols, rows, dram_addr, sp_addr & 0xFFFFFFFF);

  for (size_t row = 0; row < rows; ++row) {
    auto const dram_row_addr = dram_addr + row*load_stride;

    for (size_t col = 0; col < cols; ++col) {
      const size_t block = col / DIM;
      const size_t spad_col = col % DIM;

      if (accumulator) {
          auto const dram_byte_addr = dram_row_addr + col *
            (load_shrunk ? sizeof(elem_t) : sizeof(acc_t));

          acc_t value;
          if (!load_shrunk) {
#ifdef ELEM_T_IS_FLOAT
            value = acc_t_bits_to_acc_t(read_from_dram<acc_t_bits>(dram_byte_addr));
#else
            value = read_from_dram<acc_t>(dram_byte_addr);
#endif

#ifdef HAS_MVIN_ACC_SCALE
            value = mvin_scale_acc(value, load_scale);
#endif
          } else {
#ifdef ELEM_T_IS_FLOAT
            value = elem_t_bits_to_elem_t(read_from_dram<elem_t_bits>(dram_byte_addr));
#else
            value = read_from_dram<elem_t>(dram_byte_addr);
#endif

#ifdef HAS_MVIN_SCALE
            value = mvin_scale(value, load_scale);
#endif
          }

          if (accumulate) {
            gemmini_state.accumulator.at(base_row_addr + row + block*DIM).at(spad_col) += value;
          } else {
            gemmini_state.accumulator.at(base_row_addr + row + block*DIM).at(spad_col) = value;
          }

#ifdef ELEM_T_IS_FLOAT
          dprintf("%f ", gemmini_state.accumulator.at(base_row_addr + row + block*DIM).at(spad_col));
#else
          dprintf("%d ", gemmini_state.accumulator.at(base_row_addr + row + block*DIM).at(spad_col));
#endif
      } else {
          auto const dram_byte_addr = dram_row_addr + col*sizeof(elem_t);
#ifdef ELEM_T_IS_FLOAT
          auto value = elem_t_bits_to_elem_t(read_from_dram<elem_t_bits>(dram_byte_addr));
#else
          auto value = read_from_dram<elem_t>(dram_byte_addr);
#endif

#ifdef HAS_MVIN_SCALE
          value = mvin_scale(value, load_scale);
#endif

          gemmini_state.spad.at(base_row_addr + row + block*DIM).at(spad_col) = value;

#ifdef ELEM_T_IS_FLOAT
          dprintf("%f ", gemmini_state.spad.at(base_row_addr + row + block*DIM).at(spad_col));
#else
          dprintf("%d ", gemmini_state.spad.at(base_row_addr + row + block*DIM).at(spad_col));
#endif
      }
    }
    dprintf("\n");
  }
}

void gemmini_t::mvout(reg_t dram_addr, reg_t sp_addr) {
  bool const accumulator = (sp_addr >> 31) & 0x1;
  bool const full = (sp_addr >> 29) & 0x1;
  auto const base_row_addr = (sp_addr & 0x1FFFFFFF); // Strip accumulator addressing bits [31:30]
  auto const cols = (sp_addr >> addr_len) & 0xFFFF;
  auto const rows = (sp_addr >> (addr_len + 16)) & 0xFFFF;

  dprintf("GEMMINI: mvout - 0x%02lx cols and 0x%02lx rows from 0x%08lx to addr 0x%08lx\n", cols, rows, base_row_addr, dram_addr);

  if (gemmini_state.pool_stride == 0) {
    for (size_t i = 0; i < rows; ++i) {
      auto const dram_row_addr = dram_addr + i*gemmini_state.store_stride;

      for (size_t j = 0; j < cols; ++j) {
        if (accumulator) { // Apply shift and activation when moving out of accumulator
          acc_t acc_value = gemmini_state.accumulator.at(base_row_addr + i).at(j);
          auto shifted = acc_scale(acc_value, gemmini_state.acc_shift);
          elem_t activated = apply_activation(shifted); // Activation is always applied in either WS/OS mode

          auto const sizeof_output = full ? sizeof(acc_t) : sizeof(elem_t);

          auto const dram_byte_addr = dram_row_addr + j*sizeof_output;
#ifdef ELEM_T_IS_FLOAT
          if (full) {
            write_to_dram<acc_t_bits>(dram_byte_addr, acc_t_to_acc_t_bits(acc_value));
            dprintf("%f ", acc_value);
          } else {
            write_to_dram<elem_t_bits>(dram_byte_addr, elem_t_to_elem_t_bits(activated));
            dprintf("%f ", activated);
          }
#else
          if (full) {
            write_to_dram<acc_t>(dram_byte_addr, acc_value);
            dprintf("%d ", acc_value);
          } else {
            write_to_dram<elem_t>(dram_byte_addr, activated);
            dprintf("%d ", activated);
          }
#endif
        } else { // Scratchpad, write to DRAM directly
          auto const dram_byte_addr = dram_row_addr + j*sizeof(elem_t);
          elem_t value = gemmini_state.spad.at(base_row_addr + i).at(j);

#ifdef ELEM_T_IS_FLOAT
          write_to_dram<elem_t_bits>(dram_byte_addr, elem_t_to_elem_t_bits(value));
          dprintf("%f ", value);
#else
          write_to_dram<elem_t>(dram_byte_addr, value);
          dprintf("%d ", value);
#endif
        }
      }
      dprintf("\n");
    }
  } else {
    // Perform pooling
    auto const pool_stride = gemmini_state.pool_stride;
    auto const pool_size = gemmini_state.pool_size;
    auto const pool_out_dim = gemmini_state.pool_out_dim;
    auto const porows = gemmini_state.pool_porows;
    auto const pocols = gemmini_state.pool_pocols;
    auto const orows = gemmini_state.pool_orows;
    auto const ocols = gemmini_state.pool_ocols;
    auto const plpad = gemmini_state.pool_lpad;
    auto const pupad = gemmini_state.pool_upad;
    auto const channels = cols;

    for (int porow = 0; porow < porows; porow++) {
      for (int pocol = 0; pocol < pocols; pocol++) {
        for (int poch = 0; poch < (int)channels; poch++) {
          elem_t value = elem_t_min;

          for (int wrow = 0; wrow < pool_size; wrow++) {
            for (int wcol = 0; wcol < pool_size; wcol++) {

              const int orow = porow * pool_stride + wrow - pupad;
              const int ocol = pocol * pool_stride + wcol - plpad;

              const int row_addr = base_row_addr + orow*ocols + ocol;

              elem_t elem;

              if (orow < 0 || ocol < 0 || orow >= orows || ocol >= ocols) {
                elem = 0;
              } else if (accumulator) {
                acc_t acc_value = gemmini_state.accumulator.at(row_addr).at(poch);
                auto shifted = acc_scale(acc_value, gemmini_state.acc_shift);
                elem = apply_activation(shifted); // Activation is always applied in either WS/OS mode
              } else {
                elem = gemmini_state.spad.at(row_addr).at(poch);
              }

              if (elem > value) {
                value = elem;
              }
            }
          }

          auto const dram_byte_addr = dram_addr + (porow * pool_out_dim + pocol) * gemmini_state.store_stride + poch * sizeof(elem_t);

#ifdef ELEM_T_IS_FLOAT
          write_to_dram<elem_t_bits>(dram_byte_addr, elem_t_to_elem_t_bits(value));
#else
          write_to_dram<elem_t>(dram_byte_addr, value);
#endif
        }
      }
    }
  }
}

void gemmini_t::preload(reg_t bd_addr, reg_t c_addr) {
  // TODO: rename these state variables
  gemmini_state.preload_sp_addr = static_cast<uint32_t>(bd_addr & 0xFFFFFFFF);
  gemmini_state.output_sp_addr = static_cast<uint32_t>(c_addr & 0xFFFFFFFF);

  gemmini_state.preload_cols = (bd_addr >> addr_len) & 0xFFFF;
  gemmini_state.preload_rows = (bd_addr >> (addr_len + 16)) & 0xFFFF;
  gemmini_state.output_cols = (c_addr >> addr_len) & 0xFFFF;
  gemmini_state.output_rows = (c_addr >> (addr_len + 16)) & 0xFFFF;

  dprintf("GEMMINI: preload - scratchpad output addr = 0x%08x, scratchpad preload addr = 0x%08x\n",
            gemmini_state.output_sp_addr, gemmini_state.preload_sp_addr);
}

void gemmini_t::config(reg_t rs1, reg_t rs2) {
  if ((rs1 & 0b11) == 0) { // rs1[1:0] == 2'b00, config_ex, configure execute pipeline
    gemmini_state_t::Dataflow new_mode;
    gemmini_state_t::Activation new_act;
    reg_t new_acc_shift, new_sys_shift, new_relu6_shift, new_a_stride, new_a_transpose, new_b_transpose;

    auto rs1_2 = (rs1 >> 2) & 0b1; // extract rs1[2], 0 = output stationary, 1 = weight stationary
    if (rs1_2 == 0) {
      new_mode = gemmini_state_t::OS;
    } else {
      new_mode = gemmini_state_t::WS;
    }

    auto rs1_4_3 = (rs1 >> 3) & 0b11; // extract rs1[4:3], 0 = no activation, 1 = ReLU, 2 = ReLU6
    if (rs1_4_3 == 0) {
      new_act = gemmini_state_t::NONE;
    } else if (rs1_4_3 == 1) {
      new_act = gemmini_state_t::RELU;
    } else if (rs1_4_3 == 2) {
      new_act = gemmini_state_t::RELU6;
    } else {
      assert(false);
    }

    new_acc_shift = (rs1 >> 32) & 0xFFFFFFFF;
    new_sys_shift = (rs2) & 0xFFFFFFFF;
    new_relu6_shift = (rs2 >> 32) & 0xFFFFFFFF;
    new_a_stride = (rs1 >> 16) & 0xFFFF;
    new_a_transpose = (rs1 >> 8) & 0x1;
    new_b_transpose = (rs1 >> 9) & 0x1;

    dprintf("GEMMINI: config_ex - set dataflow mode from %d to %d\n", gemmini_state.mode, new_mode);
    dprintf("GEMMINI: config_ex - set activation function from %d to %d\n", gemmini_state.act, new_act);
    dprintf("GEMMINI: config_ex - set acc_shift from %lu to %lu\n", gemmini_state.acc_shift, new_acc_shift);
    dprintf("GEMMINI: config_ex - set sys_shift from %lu to %lu\n", gemmini_state.sys_shift, new_sys_shift);
    dprintf("GEMMINI: config_ex - set relu6_shift from %lu to %lu\n", gemmini_state.relu6_shift, new_relu6_shift);

    gemmini_state.mode = new_mode;
    gemmini_state.act = new_act;

    // assert(new_acc_shift >= 0 && new_acc_shift < sizeof(acc_t)*8);
    assert(new_sys_shift >= 0 && new_sys_shift < sizeof(output_t)*8);
    assert(new_relu6_shift >= 0);

    gemmini_state.acc_shift = acc_scale_t_bits_to_acc_scale_t(new_acc_shift);
    gemmini_state.sys_shift = new_sys_shift;
    gemmini_state.relu6_shift = new_relu6_shift;
    gemmini_state.a_stride = new_a_stride;
    gemmini_state.a_transpose = new_a_transpose;
    gemmini_state.b_transpose = new_b_transpose;

    assert(!(new_mode == gemmini_state_t::OS && !new_a_transpose && new_b_transpose) && !(new_mode == gemmini_state_t::WS && new_a_transpose && new_b_transpose));

  } else if ((rs1 & 0b11) == 1) { // rs1[1:0] == 2'b01, config_mvin, configure load pipeline
    const int state_id = (rs1 >> 3) & 0x3;
    dprintf("GEMMINI: config_mvin - set load stride from %lu to %lu\n", gemmini_state.load_strides[state_id], rs2);
    gemmini_state.load_strides[state_id] = rs2;
#if defined(HAS_MVIN_SCALE) || defined(HAS_MVIN_ACC_SCALE)
    dprintf("GEMMINI: config_mvin - set load scale from %lu to %lu\n", gemmini_state.load_scales[state_id], scale_t_bits_to_scale_t(rs1 >> 32));
    gemmini_state.load_scales[state_id] = scale_t_bits_to_scale_t(rs1 >> 32);
    gemmini_state.load_shrunks[state_id] = (rs1 >> 2) & 1;
#endif
  } else if ((rs1 & 0b11) == 2) { // rs1[1:0] == 2'b10, config_mvout, configure store pipeline
    dprintf("GEMMINI: config_mvout - set store stride from %lu to %lu\n", gemmini_state.store_stride, rs2);
    gemmini_state.store_stride = rs2;
    gemmini_state.pool_stride = (rs1 >> 4) & 0x3;
    gemmini_state.pool_size = (rs1 >> 6) & 0x3;
    gemmini_state.pool_upad = (rs1 >> 8) & 0x3;
    gemmini_state.pool_lpad = (rs1 >> 10) & 0x3;
    gemmini_state.pool_out_dim = (rs1 >> 24) & 0xFF;
    gemmini_state.pool_porows = (rs1 >> 32) & 0xFF;
    gemmini_state.pool_pocols = (rs1 >> 40) & 0xFF;
    gemmini_state.pool_orows = (rs1 >> 48) & 0xFF;
    gemmini_state.pool_ocols = (rs1 >> 56) & 0xFF;

    if (gemmini_state.pool_stride == 0) {
        dprintf("GEMMINI: config_mvout - no pooling\n");
    } else {
        dprintf("GEMMINI: config_mvout - set pool_stride to %u\n", gemmini_state.pool_stride);
        dprintf("GEMMINI: config_mvout - set pool_size to %u\n", gemmini_state.pool_size);
        dprintf("GEMMINI: config_mvout - set pool_upad to %u\n", gemmini_state.pool_upad);
        dprintf("GEMMINI: config_mvout - set pool_lpad to %u\n", gemmini_state.pool_lpad);
        dprintf("GEMMINI: config_mvout - set pool_out_dim to %u\n", gemmini_state.pool_out_dim);
        dprintf("GEMMINI: config_mvout - set pool_porows to %u\n", gemmini_state.pool_porows);
        dprintf("GEMMINI: config_mvout - set pool_pocols to %u\n", gemmini_state.pool_pocols);
        dprintf("GEMMINI: config_mvout - set pool_orows to %u\n", gemmini_state.pool_orows);
        dprintf("GEMMINI: config_mvout - set pool_ocols to %u\n", gemmini_state.pool_ocols);
        dprintf("GEMMINI: config_mvout - rs1 is %llx\n", rs1);
    }
  }
}

void gemmini_t::compute(reg_t a_addr, reg_t bd_addr, bool preload) {
  auto a_addr_real = static_cast<uint32_t>(a_addr & 0xFFFFFFFF);
  auto bd_addr_real = static_cast<uint32_t>(bd_addr & 0xFFFFFFFF);

  const uint16_t a_cols = (a_addr >> addr_len) & 0xFFFF;
  const uint16_t a_rows = (a_addr >> (addr_len + 16)) & 0xFFFF;

  const uint16_t bd_cols = (bd_addr >> addr_len) & 0xFFFF;
  const uint16_t bd_rows = (bd_addr >> (addr_len + 16)) & 0xFFFF;

  dprintf("GEMMINI: compute - preload = %d, scratchpad A addr = 0x%08x,"
           "scratchpad B addr 0x%08x\n", preload, a_addr_real, bd_addr_real);

  // Preload
  if (preload) {
    dprintf("GEMMINI: compute - PEs after preloading:\n");
    for (size_t i = 0; i < DIM; i++) {
      for (size_t j = 0; j < DIM; j++) {
        // TODO: Handle preloads from accumulator, values are shifted and activated before preload
        if (~gemmini_state.preload_sp_addr != 0) {
          assert(((gemmini_state.preload_sp_addr >> 30) & 0b11) == 0); // Preloads from accumulator not supported
        }

        // In OS mode, pe_state stores the accumulator values
        // In WS mode, pe_state stores the persistent weight matrix
        if (i < gemmini_state.preload_rows && j < gemmini_state.preload_cols) {
          auto preload_value = (~gemmini_state.preload_sp_addr == 0) ? 0 :
                  gemmini_state.spad.at(gemmini_state.preload_sp_addr + i).at(j);
          gemmini_state.pe_state.at(i).at(j) = preload_value;
        } else {
          gemmini_state.pe_state.at(i).at(j) = 0;
        }

#ifdef ELEM_T_IS_FLOAT
        dprintf("%f ", gemmini_state.pe_state.at(i).at(j));
#else
        dprintf("%d ", gemmini_state.pe_state.at(i).at(j));
#endif
      }
      dprintf("\n");
    }
  }

  // Compute
  // For OS, accumulate the PE results internally in pe_state
  // For WS, allocate a new results array which won't affect pe_state, seed the results array with the bias (D) matrix
  auto results = std::vector<std::vector<acc_t>>(DIM, std::vector<acc_t>(DIM));
  for (size_t i = 0; i < DIM; ++i) {
    for (size_t j = 0; j < DIM; ++j) {
      if (i < bd_rows && j < bd_cols) {
        results.at(i).at(j) = (~bd_addr_real == 0) ? 0 : gemmini_state.spad.at(bd_addr_real + i).at(j);
      } else {
        results.at(i).at(j) = 0;
      }
    }
  }

  for (size_t i = 0; i < DIM; ++i) {
    for (size_t j = 0; j < DIM; ++j) {
      for (size_t k = 0; k < DIM; ++k) {
        elem_t a;
        if (~a_addr_real != 0) {
            const size_t r = gemmini_state.a_transpose ? k : gemmini_state.a_stride * i;
            const size_t c = gemmini_state.a_transpose ? gemmini_state.a_stride * i : k;

            a = i < a_rows && k < a_cols ? gemmini_state.spad.at(a_addr_real + r).at(c) : 0;
        }

        if (gemmini_state.mode == gemmini_state_t::WS) {
          const size_t r = gemmini_state.b_transpose ? j : k;
          const size_t c = gemmini_state.b_transpose ? k : j;

          results.at(i).at(j) += a * gemmini_state.pe_state.at(r).at(c);
        } else {
          elem_t b = 0;
          if (~bd_addr_real != 0) {
            const size_t r = gemmini_state.b_transpose ? j : k;
            const size_t c = gemmini_state.b_transpose ? k : j;

            b = k < bd_rows && j < bd_cols ? gemmini_state.spad.at(bd_addr_real + r).at(c) : 0;
          }

          gemmini_state.pe_state.at(i).at(j) += a * b;
        }
      }
    }
  }

  dprintf("GEMMINI: compute - PEs after matmul:\n");
  for (size_t i = 0; i < DIM; ++i) {
    for (size_t j = 0; j < DIM; ++j) {
#ifdef ELEM_T_IS_FLOAT
      dprintf("%f ", gemmini_state.pe_state.at(i).at(j));
#else
      dprintf("%d ", gemmini_state.pe_state.at(i).at(j));
#endif
    }
    dprintf("\n");
  }

  // Write results
  if (~gemmini_state.output_sp_addr != 0) {
    bool const acc = (gemmini_state.output_sp_addr >> 31) & 0x1;
    bool const acc_accum = (gemmini_state.output_sp_addr >> 30) & 0x1;
    auto const base_sp_addr = gemmini_state.output_sp_addr & 0x3FFFFFFF;
    dprintf("GEMMINI: compute - writing results to addr 0x%08x, :\n", gemmini_state.output_sp_addr);

    for (size_t i = 0; i < gemmini_state.output_rows; ++i) {
      for (size_t j = 0; j < gemmini_state.output_cols; ++j) {
        acc_t value = gemmini_state.mode == gemmini_state_t::OS ? gemmini_state.pe_state.at(i).at(j) : results.at(i).at(j);
        if (acc) {
          output_t shifted = gemmini_state.mode == gemmini_state_t::OS ?
                  sys_shift(value, gemmini_state.sys_shift) :
                  sys_shift(value, 0);
          if (acc_accum) {
            gemmini_state.accumulator.at(base_sp_addr + i).at(j) += value;
          } else { // Overwrite
            gemmini_state.accumulator.at(base_sp_addr + i).at(j) = value;
          }
#ifdef ELEM_T_IS_FLOAT
          dprintf("%f ", gemmini_state.accumulator.at(base_sp_addr + i).at(j));
#else
          dprintf("%d ", gemmini_state.accumulator.at(base_sp_addr + i).at(j));
#endif
        } else { // Move to scratchpad, apply activation along the way
          elem_t shifted = gemmini_state.mode == gemmini_state_t::OS ?
                             sys_shift(value, gemmini_state.sys_shift) :
                             sys_shift(value, 0);
          elem_t activated = apply_activation(shifted);
          gemmini_state.spad.at(base_sp_addr + i).at(j) = activated;
#ifdef ELEM_T_IS_FLOAT
          dprintf("%f ", gemmini_state.spad.at(base_sp_addr + i).at(j));
#else
          dprintf("%d ", gemmini_state.spad.at(base_sp_addr + i).at(j));
#endif
        }
      }
      dprintf("\n");
    }
  }
}

void gemmini_t::loop_ws(reg_t rs1, reg_t rs2) {
  const bool ex_accumulate = rs1 & 1;

  const uint16_t I = gemmini_state.loop_ws_I;
  const uint16_t J = gemmini_state.loop_ws_J;
  const uint16_t K = gemmini_state.loop_ws_K;

  const uint16_t pad_I = gemmini_state.loop_ws_pad_I;
  const uint16_t pad_J = gemmini_state.loop_ws_pad_J;
  const uint16_t pad_K = gemmini_state.loop_ws_pad_K;

  const uint32_t GARBAGE_ADDR = ~0;

  const int total_spad_rows = (I * K + K * J) * DIM;
  const int total_acc_rows = (I * J) * DIM;

  if (total_spad_rows > BANK_NUM * BANK_ROWS / 2 || total_acc_rows > ACC_ROWS / 2) {
    printf("LOOP_WS bounds were too large for double-buffering\n");
    exit(1);
  }

  const uint32_t A_sp_addr_start = 0;
  const uint32_t B_sp_addr_start = (BANK_NUM * BANK_ROWS / 2) - K * J * DIM;
  const uint32_t D_sp_addr_start = 1 << (ADDR_LEN-1);
  const uint32_t C_sp_addr_start = 3 << (ADDR_LEN-2);

  if (gemmini_state.loop_ws_D != 0) {
    for (uint16_t i = 0; i < I; i++) {
      for (uint16_t j = 0; j < J; j++) {
        const uint64_t dram_addr = gemmini_state.loop_ws_D +
          (i * gemmini_state.loop_ws_D_stride + j) * DIM * sizeof(acc_t);

        const uint64_t sp_addr = D_sp_addr_start + (i*J + j)*DIM;

        const uint64_t cols = DIM - (j == J-1 ? pad_J : 0);
        const uint64_t rows = DIM - (i == I-1 ? pad_I : 0);

        mvin(dram_addr, (rows << 48) | (cols << 32) | sp_addr, 2);
      }
    }
  }

  for (uint16_t k = 0; k < K; k++) {
    for (uint16_t j = 0; j < J; j++) {
      for (uint16_t i = 0; i < I; i++) {
        const uint32_t A_sp_addr = A_sp_addr_start + (i*K + k)*DIM;
        const uint32_t B_sp_addr = B_sp_addr_start + (k*J + j)*DIM;
        const uint32_t C_sp_addr = C_sp_addr_start + (i*J + j)*DIM;

        // Mvin A
        if (j == 0) {
          const uint64_t A_dram_addr = gemmini_state.loop_ws_A +
              (i*gemmini_state.loop_ws_A_stride + k) * DIM * sizeof(elem_t);
          const uint64_t cols = DIM - (k == K-1 ? pad_K : 0);
          const uint64_t rows = DIM - (k == I-1 ? pad_I : 0);
          mvin(A_dram_addr, (rows << 48) | (cols << 32) | A_sp_addr, 0);
        }

        // Mvin B
        if (i == 0) {
          const uint64_t B_dram_addr = gemmini_state.loop_ws_B +
              (k*gemmini_state.loop_ws_B_stride + j) * DIM * sizeof(elem_t);
          const uint64_t cols = DIM - (j == J-1 ? pad_J : 0);
          const uint64_t rows = DIM - (k == K-1 ? pad_K : 0);
          mvin(B_dram_addr, (rows << 48) | (cols << 32) | B_sp_addr, 1);
        }

        // Compute
        {
          uint32_t pre_sp_addr = i == 0 ? B_sp_addr : GARBAGE_ADDR;
          uint32_t out_sp_addr = C_sp_addr;

          if (!ex_accumulate && k == 0) {
            out_sp_addr &= ~(1 << (ADDR_LEN-2));
          }

          const uint64_t A_cols = DIM - (k == K - 1 ? pad_K : 0);
          const uint64_t A_rows = DIM - (i == I - 1 ? pad_I : 0);
          const uint64_t B_cols = DIM - (j == J - 1 ? pad_J : 0);
          const uint64_t B_rows = DIM - (k == K - 1 ? pad_K : 0);
          const uint64_t C_cols = DIM - (j == J - 1 ? pad_J : 0);
          const uint64_t C_rows = DIM - (i == I - 1 ? pad_I : 0);

          preload((B_rows << 48) | (B_cols << 32) | pre_sp_addr,
              (C_rows << 48) | (C_cols << 32) | out_sp_addr);

          compute((A_rows << 48) | (A_cols << 32) | A_sp_addr,
              ((uint64_t)DIM << 48) | ((uint64_t)DIM << 32) | GARBAGE_ADDR, i == 0);
        }

        // Move-out C
        if (gemmini_state.loop_ws_C != 0 && k == K-1) {
          const uint64_t C_dram_addr = gemmini_state.loop_ws_C +
              (i*gemmini_state.loop_ws_C_stride + j) * DIM * sizeof(elem_t);

          const uint64_t C_cols = DIM - (j == J - 1 ? pad_J : 0);
          const uint64_t C_rows = DIM - (i == I - 1 ? pad_I : 0);

          mvout(C_dram_addr, (C_rows << 48) | (C_cols << 32) | C_sp_addr);
        }
      }
    }
  }
}

void gemmini_t::compute_cisc() {
  // `compute` performs Gemmini's core function - matrix multiply-add -
  //  without referencing any underlying hardware detail.
  //
  // * Operands A, B, and D are loaded from memory
  // * Multiply, add, activation, and any requested shifts are performed
  // * Result D is written back to memory
  //
  // These computations are made independent of systolic array sizes,
  // scratchpad-memory sizes,
  // and any other microarchitectural detail (other than datatypes).

  // Load operands from memory
  auto A = read_matrix_from_dram<elem_t>(gemmini_state.a_addr,
                                          gemmini_state.m,
                                          gemmini_state.k,
                                          false, false);
  auto B = read_matrix_from_dram<elem_t>(gemmini_state.b_addr,
                                          gemmini_state.k,
                                          gemmini_state.n,
                                          false, false);
  auto D = read_matrix_from_dram<acc_t>(gemmini_state.d_addr,
                                          gemmini_state.m,
                                          gemmini_state.n,
                                          true,
                                          gemmini_state.repeating_bias);
  // Initialize an accumulator/ result
  auto C = matrix_zeroes<elem_t>(gemmini_state.m, gemmini_state.n);

  // Multiply & apply activation
  for (size_t i=0; i<gemmini_state.m; i++) {
    for (size_t j=0; j<gemmini_state.n; j++) {
      acc_t value = D->at(i).at(j);
      for (size_t k=0; k<gemmini_state.k; k++) {
        value += ((acc_t)A->at(i).at(k)) * ((acc_t)B->at(k).at(j));
      }
      elem_t shifted = acc_scale(value,
                          gemmini_state.acc_shift);
      elem_t activated = apply_activation(shifted);
      C->at(i).at(j) = activated;
    }
  }

  // Write back to memory
  for (size_t i = 0; i < gemmini_state.m; i++) {
    auto const dram_row_addr = gemmini_state.c_addr +
                               i*sizeof(elem_t)*gemmini_state.n;
    for (size_t j = 0; j < gemmini_state.n; j++) {
      auto const dram_byte_addr = dram_row_addr + j*sizeof(elem_t);
#ifdef ELEM_T_IS_FLOAT
      write_to_dram<elem_t_bits>(dram_byte_addr, C->at(i).at(j));
#else
      write_to_dram<elem_t>(dram_byte_addr, C->at(i).at(j));
#endif
    }
  }
}

void gemmini_t::loop_ws_config_bounds(reg_t rs1, reg_t rs2) {
  gemmini_state.loop_ws_I = rs2 & 0xFFFF;
  gemmini_state.loop_ws_J = (rs2 >> 16) & 0xFFFF;
  gemmini_state.loop_ws_K = (rs2 >> 32) & 0xFFFF;

  gemmini_state.loop_ws_pad_I = rs1 & 0xFFFF;
  gemmini_state.loop_ws_pad_J = (rs1 >> 16) & 0xFFFF;
  gemmini_state.loop_ws_pad_K = (rs1 >> 32) & 0xFFFF;
}

void gemmini_t::loop_ws_config_addrs_AB(reg_t rs1, reg_t rs2) {
  gemmini_state.loop_ws_A = rs1;
  gemmini_state.loop_ws_B = rs2;
}

void gemmini_t::loop_ws_config_addrs_DC(reg_t rs1, reg_t rs2) {
  gemmini_state.loop_ws_D = rs1;
  gemmini_state.loop_ws_C = rs2;
}

void gemmini_t::loop_ws_config_strides_AB(reg_t rs1, reg_t rs2) {
  gemmini_state.loop_ws_A_stride = rs1;
  gemmini_state.loop_ws_B_stride = rs2;
}

void gemmini_t::loop_ws_config_strides_DC(reg_t rs1, reg_t rs2) {
  gemmini_state.loop_ws_D_stride = rs1;
  gemmini_state.loop_ws_C_stride = rs2;
}

reg_t gemmini_t::custom3(rocc_insn_t insn, reg_t xs1, reg_t xs2) {
  if (insn.funct == mvin_funct) {
    mvin(xs1, xs2, 0);
  } else if (insn.funct == mvin2_funct) {
    mvin(xs1, xs2, 1);
  } else if (insn.funct == mvin3_funct) {
    mvin(xs1, xs2, 2);
  } else if (insn.funct == mvout_funct) {
    mvout(xs1, xs2);
  } else if (insn.funct == preload_funct) {
    preload(xs1, xs2);
  } else if (insn.funct == config_funct) {
    config(xs1, xs2);
  } else if (insn.funct == compute_preloaded_funct) {
    compute(xs1, xs2, true);
  } else if (insn.funct == compute_accumulated_funct) {
    compute(xs1, xs2, false);
  } else if (insn.funct == loop_ws_config_bounds_funct) {
    loop_ws_config_bounds(xs1, xs2);
  } else if (insn.funct == loop_ws_config_addrs_AB_funct) {
    loop_ws_config_addrs_AB(xs1, xs2);
  } else if (insn.funct == loop_ws_config_addrs_DC_funct) {
    loop_ws_config_addrs_DC(xs1, xs2);
  } else if (insn.funct == loop_ws_config_strides_AB_funct) {
    loop_ws_config_strides_AB(xs1, xs2);
  } else if (insn.funct == loop_ws_config_strides_DC_funct) {
    loop_ws_config_strides_DC(xs1, xs2);
  } else if (insn.funct == loop_ws_funct) {
    loop_ws(xs1, xs2);
  }
  //==========================================================================
  // gemmini-cisc opcodes
  //==========================================================================
  /*
  else if (insn.funct == config_cisc_ex_funct) {
    config(xs1, xs2);
  }
  else if (insn.funct == config_addr_AB_funct) {
    gemmini_state.a_addr = xs1;
    gemmini_state.b_addr = xs2;
  }
  else if (insn.funct == config_addr_CD_funct ){
    gemmini_state.c_addr = xs1;
    gemmini_state.d_addr = xs2;
  }
  else if (insn.funct == config_size0_funct ){
    gemmini_state.m = xs1;
    gemmini_state.n = xs2;
  }
  else if (insn.funct == config_size1_funct ){
    gemmini_state.k = xs1;
  }
  else if (insn.funct == config_repeating_bias_funct){
    gemmini_state.repeating_bias = (bool)xs1;
  }
  else if (insn.funct == config_reset_funct) {
    reset();
  }
  else if (insn.funct == compute_cisc_funct) {
    compute_cisc();
  }
  */
  //==========================================================================
  else if (insn.funct == flush_funct) {
    dprintf("GEMMINI: flush\n");
  } else {
    dprintf("GEMMINI: encountered unknown instruction with funct: %d\n", insn.funct);
    illegal_instruction();
  }
  return 0;
}

#if defined(HAS_MVIN_SCALE) || defined(HAS_MVIN_ACC_SCALE)
scale_t_bits gemmini_t::scale_t_to_scale_t_bits(scale_t scale) {
    union {
        scale_t scale;
        scale_t_bits bits;
    } un;

    un.scale = scale;
    return un.bits;
}

scale_t gemmini_t::scale_t_bits_to_scale_t(scale_t_bits bits) {
    union {
        scale_t scale;
        scale_t_bits bits;
    } un;

    un.bits = bits;
    return un.scale;
}
#endif

acc_scale_t_bits gemmini_t::acc_scale_t_to_acc_scale_t_bits(acc_scale_t scale) {
    union {
        acc_scale_t scale;
        acc_scale_t_bits bits;
    } un;

    un.scale = scale;
    return un.bits;
}

acc_scale_t gemmini_t::acc_scale_t_bits_to_acc_scale_t(acc_scale_t_bits bits) {
    union {
        acc_scale_t scale;
        acc_scale_t_bits bits;
    } un;

    un.bits = bits;
    return un.scale;
}

// Applying activation from PE post-shifted output to scratchpad (for OS dataflow)
// or from accumulator to DRAM (after shifting, for WS dataflow)
elem_t gemmini_t::apply_activation(elem_t value) {
  if (gemmini_state.act == gemmini_state_t::RELU) {
    return value > 0 ? static_cast<elem_t>(value) : static_cast<elem_t>(0);
  } else if (gemmini_state.act == gemmini_state_t::RELU6) {
    auto positive = value > 0 ? value : static_cast<elem_t>(0);
    return value > (6 << gemmini_state.relu6_shift) ? static_cast<elem_t>(6 << gemmini_state.relu6_shift) : positive;
  } else if (gemmini_state.act == gemmini_state_t::NONE) {
    return static_cast<elem_t>(value);
  } else assert(false);
}

#ifdef HAS_MVIN_SCALE
elem_t gemmini_t::mvin_scale(elem_t value, scale_t scale) {
  acc_t scaled = MVIN_SCALE(value, scale);

#ifndef ELEM_T_IS_FLOAT
  // Saturate and cast element
  const auto elem_t_max = std::numeric_limits<elem_t>::max();
  const auto elem_t_min = std::numeric_limits<elem_t>::min();
  scaled = scaled > elem_t_max ? elem_t_max : (scaled < elem_t_min ? elem_t_min : scaled);
#endif

  return scaled;
}
#endif

#ifdef HAS_MVIN_ACC_SCALE
acc_t gemmini_t::mvin_scale_acc(acc_t value, scale_acc_t scale) {
  acc_t scaled = MVIN_SCALE_ACC(value, scale);

#ifndef ELEM_T_IS_FLOAT
  // Saturate and cast element
  const auto elem_t_max = std::numeric_limits<acc_t>::max();
  const auto elem_t_min = std::numeric_limits<acc_t>::min();
  scaled = scaled > elem_t_max ? elem_t_max : (scaled < elem_t_min ? elem_t_min : scaled);
#endif

  return scaled;
}
#endif

elem_t gemmini_t::acc_scale(acc_t value, acc_scale_t scale) {
  acc_t scaled = ACC_SCALE(value, scale);

#ifndef ELEM_T_IS_FLOAT
  // Saturate and cast element
  const auto elem_t_max = std::numeric_limits<elem_t>::max();
  const auto elem_t_min = std::numeric_limits<elem_t>::min();
  scaled = scaled > elem_t_max ? elem_t_max : (scaled < elem_t_min ? elem_t_min : scaled);
#endif

  return scaled;
}

elem_t gemmini_t::sys_shift(acc_t value, unsigned int shift) {
  acc_t shifted = ROUNDING_RIGHT_SHIFT(value, shift);

#ifndef ELEM_T_IS_FLOAT
  // Saturate and cast element
  const auto elem_t_max = std::numeric_limits<elem_t>::max();
  const auto elem_t_min = std::numeric_limits<elem_t>::min();
  shifted = shifted > elem_t_max ? elem_t_max : (shifted < elem_t_min ? elem_t_min : shifted);
#endif

  return shifted;
}

#ifdef ELEM_T_IS_FLOAT
elem_t gemmini_t::elem_t_bits_to_elem_t(elem_t_bits x) {
  union {
    elem_t_bits b;
    elem_t f;
  } un;

  un.b = x;
  return un.f;
}

elem_t_bits gemmini_t::elem_t_to_elem_t_bits(elem_t x) {
  union {
    elem_t_bits b;
    elem_t f;
  } un;

  un.f = x;
  return un.b;
}

acc_t gemmini_t::acc_t_bits_to_acc_t(acc_t_bits x) {
  union {
    acc_t_bits b;
    acc_t f;
  } un;

  un.b = x;
  return un.f;
}

acc_t_bits gemmini_t::acc_t_to_acc_t_bits(acc_t x) {
  union {
    acc_t_bits b;
    acc_t f;
  } un;

  un.f = x;
  return un.b;
}
#endif
