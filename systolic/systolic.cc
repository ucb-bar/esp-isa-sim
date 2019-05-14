#include "systolic.h"
#include "mmu.h"
#include "trap.h"
#include <stdexcept>
#include <iostream>
#include <assert.h>

#define RISCV_ENABLE_SYSTOLIC_COMMITLOG 1

REGISTER_EXTENSION(systolic, []() { return new systolic_t; })

void systolic_state_t::reset()
{
  enable = true;
  mode = OS;
  act = NONE;
  shift = 0;
  output_sp_addr = 0;
  load_stride = dim * sizeof(input_t);
  store_stride = dim * sizeof(input_t);
  spad = new std::vector<std::vector<input_t>>(sp_matrices*dim, std::vector<input_t>(dim));
  for (size_t row = 0; row < sp_matrices*dim; ++row) {
    for (size_t elem = 0; elem < dim; ++elem) {
      spad->at(row).at(elem) = 0;
    }
  }
  pe_state = new std::vector<std::vector<accum_t>>(dim, std::vector<accum_t>(dim));
  accumulator = new std::vector<std::vector<accum_t>>(accum_rows, std::vector<accum_t>(dim));
  for (size_t row = 0; row < accum_rows; ++row) {
    for (size_t elem = 0; elem < dim; ++elem) {
      accumulator->at(row).at(elem) = 0;
    }
  }

  printf("Systolic extension configured with:\n");
  printf("    dim = %u\n", dim);
}

void systolic_t::reset() {
  systolic_state.reset();
}

template <class T>
T systolic_t::read_from_dram(reg_t addr) {
  T value = 0;
  for (size_t byte_idx = 0; byte_idx < sizeof(T); ++byte_idx) {
    value |= p->get_mmu()->load_uint8(addr + byte_idx) << (byte_idx*8);
  }
  return value;
}

// Move a systolic block from DRAM at dram_addr (byte addr) to
// the scratchpad/accumulator at sp_addr (systolic-row addressed)
void systolic_t::mvin(reg_t dram_addr, reg_t sp_addr) {
  bool const accumulator = (((sp_addr >> 31) & 0x1) == 1);
  auto const base_row_addr = (sp_addr & 0x3FFFFFFF); // Strip accumulator addressing bits [31:30]

  for (size_t i = 0; i < dim; ++i) {
    auto const dram_row_addr = dram_addr + i*systolic_state.load_stride;
    for (size_t j = 0; j < dim; ++j) {
      if (accumulator) {
        auto const dram_byte_addr = dram_row_addr + j*sizeof(accum_t);
        auto value = read_from_dram<accum_t>(dram_byte_addr);
        systolic_state.accumulator->at(base_row_addr + i).at(j) = value;
      } else { // Scratchpad (mvin input_t)
        auto const dram_byte_addr = dram_row_addr + j*sizeof(input_t);
        auto value = read_from_dram<input_t>(dram_byte_addr);
        systolic_state.spad->at(base_row_addr + i).at(j) = value;
      }
    }
  }
  #ifdef RISCV_ENABLE_SYSTOLIC_COMMITLOG
  printf("SYSTOLIC: mvin - block from 0x%08lx to addr 0x%08lx\n", dram_addr, sp_addr);
  for (size_t i = 0; i < dim; ++i) {
    for (size_t j = 0; j < dim; ++j) {
      if (accumulator) {
        printf("%d ",systolic_state.accumulator->at(i).at(j));
      } else {
        printf("%d ",systolic_state.spad->at(i).at(j));
      }
    }
    printf("\n");
  }
  #endif
}

void systolic_t::mvout(reg_t dram_addr, reg_t sp_addr) {
  for (size_t row_idx = 0; row_idx < dim; ++row_idx) {
    for (size_t col_idx = 0; col_idx < dim; ++col_idx) {
      if (((sp_addr >> 31) & 0x1) == 1) { // Accumulator
        // Apply shift when moving out of accumulator using WS dataflow
        auto const acc_row_addr = (sp_addr & 0x3FFFFFFF) + row_idx;
        accum_t acc_value = systolic_state.accumulator->at(acc_row_addr).at(col_idx);
        input_t shifted = in_rounding_saturating_shift(acc_value, systolic_state.shift);
        input_t activated = apply_activation(shifted); // Activation is always applied in either WS/OS mode

        auto const dram_byte_addr = dram_addr + row_idx*systolic_state.store_stride + col_idx*sizeof(input_t);
        for (size_t byte_idx = 0; byte_idx < sizeof(input_t); ++byte_idx) {
          p->get_mmu()->store_uint8(dram_byte_addr + byte_idx,
                                    static_cast<uint8_t>((activated >> (byte_idx*8)) & 0xFF));
        }
        #ifdef RISCV_ENABLE_SYSTOLIC_COMMITLOG
        printf("SYSTOLIC: mvout - (value,shifted,activated) = (%08d, %08d, %08d) from accumulator addr 0x%08lx to 0x%08lx\n",
               acc_value, shifted, activated, acc_row_addr, dram_byte_addr);
        #endif
      } else { // Scratchpad, copy over directly
        auto const sp_row_addr = sp_addr + row_idx;
        auto const dram_byte_addr = dram_addr + row_idx*systolic_state.store_stride + col_idx*sizeof(input_t);
        for (size_t byte_idx = 0; byte_idx < sizeof(input_t); ++byte_idx) {
          p->get_mmu()->store_uint8(dram_byte_addr + byte_idx,
                                    static_cast<uint8_t>((systolic_state.spad->at(sp_row_addr).at(col_idx) >> (byte_idx*8)) & 0xFF));
        }
        #ifdef RISCV_ENABLE_SYSTOLIC_COMMITLOG
        printf("SYSTOLIC: mvout - value %08d from scratchpad addr 0x%08lx to 0x%08lx\n",
               systolic_state.spad->at(sp_row_addr).at(col_idx), sp_row_addr, dram_byte_addr);
        #endif
      }
    }
  }
}

void systolic_t::preload(reg_t bd_addr, reg_t c_addr) {
  // TODO: rename these state variables
  systolic_state.preload_sp_addr = static_cast<uint32_t>(bd_addr & 0xFFFFFFFF);
  systolic_state.output_sp_addr = static_cast<uint32_t>(c_addr & 0xFFFFFFFF);
  #ifdef RISCV_ENABLE_SYSTOLIC_COMMITLOG
    printf("SYSTOLIC: preload - scratchpad output addr = 0x%08x, scratchpad preload addr = 0x%08x\n",
            systolic_state.output_sp_addr, systolic_state.preload_sp_addr);
  #endif
}

void systolic_t::setmode(reg_t rs1, reg_t rs2) {
  if ((rs1 & 0b11) == 0) { // rs1[1:0] == 2'b00, config_ex, configure execute pipeline
    systolic_state_t::Dataflow new_mode;
    systolic_state_t::Activation new_act;
    reg_t new_shift;

    auto rs1_2 = (rs1 >> 2) & 0b1; // extract rs1[2], 0 = output stationary, 1 = weight stationary
    if (rs1_2 == 0) {
      new_mode = systolic_state_t::OS;
    } else {
      new_mode = systolic_state_t::WS;
    }

    auto rs1_4_3 = (rs1 >> 3) & 0b11; // extract rs1[4:3], 0 = no activation, 1 = ReLU, 2 = ReLU6
    if (rs1_4_3 == 0) {
      new_act = systolic_state_t::NONE;
    } else if (rs1_4_3 == 1) {
      new_act = systolic_state_t::RELU;
    } else if (rs1_4_3 == 2) {
      new_act = systolic_state_t::RELU6;
    } else {
      assert(false);
    }

    new_shift = rs2;
    assert(new_shift >= 0 && new_shift < sizeof(accum_t)*8);

    #ifdef RISCV_ENABLE_SYSTOLIC_COMMITLOG
    printf("SYSTOLIC: config_ex - set dataflow mode from %d to %d\n", systolic_state.mode, new_mode);
    printf("SYSTOLIC: config_ex - set activation function from %d to %d\n", systolic_state.act, new_act);
    printf("SYSTOLIC: config_ex - set shift from %lu to %lu\n", systolic_state.shift, rs2);
    #endif
    systolic_state.mode = new_mode;
    systolic_state.act = new_act;
    systolic_state.shift = new_shift;
  } else if ((rs1 & 0b11) == 1) { // rs1[1:0] == 2'b01, config_mvin, configure load pipeline
    #ifdef RISCV_ENABLE_SYSTOLIC_COMMITLOG
    printf("SYSTOLIC: config_mvin - set load stride from %lu to %lu\n", systolic_state.load_stride, rs2);
    #endif
    systolic_state.load_stride = rs2;
  } else if ((rs1 & 0b11) == 2) { // rs1[1:0] == 2'b10, config_mvout, configure store pipeline
    #ifdef RISCV_ENABLE_SYSTOLIC_COMMITLOG
    printf("SYSTOLIC: config_mvout - set store stride from %lu to %lu\n", systolic_state.store_stride, rs2);
    #endif
    systolic_state.store_stride = rs2;
  }
}

void systolic_t::compute(reg_t a_addr, reg_t bd_addr, bool preload) {
  auto a_addr_real = static_cast<uint32_t>(a_addr & 0xFFFFFFFF);
  auto bd_addr_real = static_cast<uint32_t>(bd_addr & 0xFFFFFFFF);

  #ifdef RISCV_ENABLE_SYSTOLIC_COMMITLOG
    printf("SYSTOLIC: compute - preload = %d, scratchpad A addr = 0x%08x,"
           "scratchpad B addr 0x%08x\n", preload, a_addr_real, bd_addr_real);
  #endif

  // Preload
  if (preload) {
    for (size_t i = 0; i < dim; i++) {
      for (size_t j = 0; j < dim; j++) {
        // In OS mode, pe_state stores the accumulator values
        // In WS mode, pe_state stores the persistent weight matrix
        if (~systolic_state.preload_sp_addr == 0) {
          systolic_state.pe_state->at(i).at(j) = 0;
          #ifdef RISCV_ENABLE_SYSTOLIC_COMMITLOG
          printf("SYSTOLIC: compute - PE %02lu,%02lu preloaded with 0\n", i, j);
          #endif
        } else {
          assert(((systolic_state.preload_sp_addr >> 30) & 0b11) == 0); // Preloads from accumulator not supported
          systolic_state.pe_state->at(i).at(j) = systolic_state.spad->at(systolic_state.preload_sp_addr + i).at(j);
          #ifdef RISCV_ENABLE_SYSTOLIC_COMMITLOG
          printf("SYSTOLIC: compute - PE %02lu,%02lu preloaded with %08d from scratchpad address 0x%08lx\n",
                 i, j, systolic_state.pe_state->at(i).at(j), systolic_state.preload_sp_addr + i);
          #endif
        }
      }
    }
  }

  // Compute
  // For OS, accumulate the PE results internally in pe_state
  // For WS, allocate a new results array which won't affect pe_state, seed the results array with the bias (D) matrix
  auto results = new std::vector<std::vector<accum_t>>(dim, std::vector<accum_t>(dim));
  for (size_t i = 0; i < dim; ++i) {
    for (size_t j = 0; j < dim; ++j) {
      results->at(i).at(j) = (~bd_addr_real == 0) ? 0 : systolic_state.spad->at(bd_addr_real + i).at(j);
    }
  }
  for (size_t i = 0; i < dim; ++i) {
    for (size_t j = 0; j < dim; ++j) {
      for (size_t k = 0; k < dim; ++k) {
        if (systolic_state.mode == systolic_state_t::WS) {
          results->at(i).at(j) += systolic_state.spad->at(a_addr_real + i).at(k) * systolic_state.pe_state->at(k).at(j);
        } else {
          systolic_state.pe_state->at(i).at(j) +=
                  systolic_state.spad->at(a_addr_real + i).at(k) * systolic_state.spad->at(bd_addr_real + k).at(j);
        }
      }
    }
  }

  for (size_t i = 0; i < dim; ++i) {
    for (size_t j = 0; j < dim; ++j) {
      #ifdef RISCV_ENABLE_SYSTOLIC_COMMITLOG
      printf("SYSTOLIC: compute - PE %02lu,%02lu computed matmul result %08d\n",
             i, j, systolic_state.pe_state->at(i).at(j));
      #endif
    }
  }

  // Write results
  if (~systolic_state.output_sp_addr != 0) {
    for (size_t i = 0; i < dim; ++i) {
      for (size_t j = 0; j < dim; ++j) {
        accum_t value = systolic_state.mode == systolic_state_t::OS ? systolic_state.pe_state->at(i).at(j) : results->at(i).at(j);
        if (((systolic_state.output_sp_addr >> 31) & 0x1) == 1) { // Move to accumulator
          output_t shifted = systolic_state.mode == systolic_state_t::OS ?
                  out_rounding_saturating_shift(value, systolic_state.shift) :
                  out_rounding_saturating_shift(value, 0);
          if (((systolic_state.output_sp_addr >> 30) & 0x1) == 1) { // Accumulate on top of existing accumulator value
            systolic_state.accumulator->at((systolic_state.output_sp_addr & 0x3FFFFFFF) + i).at(j) += shifted;
          } else { // Overwrite
            systolic_state.accumulator->at((systolic_state.output_sp_addr & 0x3FFFFFFF) + i).at(j) = shifted;
          }
        } else { // Move to scratchpad, apply activation along the way
          input_t shifted = systolic_state.mode == systolic_state_t::OS ?
                             in_rounding_saturating_shift(value, systolic_state.shift) :
                             in_rounding_saturating_shift(value, 0);
          input_t activated = apply_activation(shifted);
          systolic_state.spad->at(systolic_state.output_sp_addr + i).at(j) = activated;
          #ifdef RISCV_ENABLE_SYSTOLIC_COMMITLOG
          printf("SYSTOLIC: compute - PE %02lu,%02lu wrote (value,shifted,activated) = (%08d, %08d, %08d) to scratchpad address 0x%08lx\n",
                 i, j, value, shifted, activated, systolic_state.output_sp_addr + i);
          #endif
        }
      }
    }
  }
}

reg_t systolic_t::custom3(rocc_insn_t insn, reg_t xs1, reg_t xs2) {
  insn.funct = (insn.funct & 0b111); // Strip the dependency bits from the funct field
  if (insn.funct == mvin_funct)
    mvin(xs1, xs2);
  else if (insn.funct == mvout_funct)
    mvout(xs1, xs2);
  else if (insn.funct == preload_funct)
    preload(xs1, xs2);
  else if (insn.funct == setmode_funct)
    setmode(xs1, xs2);
  else if (insn.funct == compute_preloaded_funct)
    compute(xs1, xs2, true);
  else if (insn.funct == compute_accumulated_funct)
    compute(xs1, xs2, false);
  else
    illegal_instruction();
  return 0;
}

// Applying activation from PE post-shifted output to scratchpad (for OS dataflow)
// or from accumulator to DRAM (after shifting, for WS dataflow)
input_t systolic_t::apply_activation(input_t value) {
  if (systolic_state.act == systolic_state_t::RELU) {
    return value > 0 ? static_cast<input_t>(value) : static_cast<input_t>(0);
  } else if (systolic_state.act == systolic_state_t::RELU6) {
    auto positive = value > 0 ? value : static_cast<input_t>(0);
    return value > 6 ? static_cast<input_t>(6) : positive;
  } else if (systolic_state.act == systolic_state_t::NONE) {
    return static_cast<input_t>(value);
  } else assert(false);
}

output_t systolic_t::out_rounding_saturating_shift(accum_t value, uint64_t shift) {
  // Implementation taken from systolic-rocc-tests/include/systolic.h (matshift() function)
  int divisor = 1 << shift;
  // Bitshift and round element
  int64_t abs = value > 0 ? value : -value;
  int64_t shifted = (abs + (divisor/2)) / divisor;
  if (value < 0)
    shifted = -shifted;

  // Saturate and cast element
  auto elem_t_max = std::numeric_limits<output_t>::max();
  auto elem_t_min = std::numeric_limits<output_t>::min();
  int64_t elem = shifted > elem_t_max ? elem_t_max : (shifted < elem_t_min ? elem_t_min : shifted);
  return static_cast<output_t>(elem);
}

input_t systolic_t::in_rounding_saturating_shift(accum_t value, uint64_t shift) {
  // Implementation taken from systolic-rocc-tests/include/systolic.h (matshift() function)
  int divisor = 1 << shift;
  // Bitshift and round element
  int64_t abs = value > 0 ? value : -value;
  int64_t shifted = (abs + (divisor/2)) / divisor;
  if (value < 0)
    shifted = -shifted;

  // Saturate and cast element
  auto elem_t_max = std::numeric_limits<input_t>::max();
  auto elem_t_min = std::numeric_limits<input_t>::min();
  int64_t elem = shifted > elem_t_max ? elem_t_max : (shifted < elem_t_min ? elem_t_min : shifted);
  return static_cast<input_t>(elem);
}

std::vector<disasm_insn_t*> rocc_t::get_disasms()
{
  std::vector<disasm_insn_t*> insns;
  return insns;
}
