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
  load_stride = row_bytes;
  store_stride = row_bytes;
  spad = new std::vector<std::vector<input_t>>(sp_matrices*dim, std::vector<input_t>(dim));
  pe_state = new std::vector<std::vector<accum_t>>(dim, std::vector<accum_t>(dim));
  accumulator = new std::vector<std::vector<accum_t>>(accum_rows, std::vector<accum_t>(dim));

  printf("Systolic extension configured with:\n");
  printf("    dim = %u\n", dim);
  printf("    row_bytes = %u\n", row_bytes);
}

void systolic_t::reset() {
  systolic_state.reset();
}

// Move a systolic block from DRAM at dram_addr (byte addr) to the scratchpad at sp_addr (systolic-row addressed)
void systolic_t::mvin(reg_t dram_addr, reg_t sp_addr) {
  for (size_t row_idx = 0; row_idx < dim; ++row_idx) {
    for (size_t col_idx = 0; col_idx < dim; ++col_idx) {
      auto const sp_row_addr = sp_addr + row_idx;
      auto const dram_byte_addr = dram_addr + row_idx*systolic_state.load_stride + col_idx*sizeof(input_t);
      systolic_state.spad->at(sp_row_addr).at(col_idx) = 0;
      for (size_t byte_idx = 0; byte_idx < sizeof(input_t); ++byte_idx) {
        systolic_state.spad->at(sp_row_addr).at(col_idx) |=
                p->get_mmu()->load_uint8(dram_byte_addr + byte_idx) << (byte_idx*8);
      }
      #ifdef RISCV_ENABLE_SYSTOLIC_COMMITLOG
      printf("SYSTOLIC: mvin - value %08d from 0x%08lx to scratchpad addr 0x%08lx\n",
             systolic_state.spad->at(sp_row_addr).at(col_idx), dram_byte_addr, sp_row_addr);
      #endif
    }
  }
}

void systolic_t::mvout(reg_t dram_addr, reg_t sp_addr) {
  for (size_t row_idx = 0; row_idx < dim; ++row_idx) {
    for (size_t col_idx = 0; col_idx < dim; ++col_idx) {
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

void systolic_t::preload(reg_t d_addr, reg_t c_addr) {
  systolic_state.preload_sp_addr = d_addr;
  systolic_state.output_sp_addr = c_addr;
  #ifdef RISCV_ENABLE_SYSTOLIC_COMMITLOG
    printf("SYSTOLIC: preload - scratchpad output addr = 0x%08lx, scratchpad preload addr = 0x%08lx\n",
            systolic_state.output_sp_addr, systolic_state.preload_sp_addr);
  #endif
}

void systolic_t::setmode(reg_t rs1, reg_t rs2) {
  if ((rs1 & 0b11) == 0) { // rs1[1:0] == 2'b00, config_ex, configure execute pipeline
    systolic_state_t::Dataflow new_mode;
    systolic_state_t::Activation new_act;
    reg_t new_shift;

    auto rs1_2 = (rs1 >> 1) & 0b1; // extract rs1[2], 0 = output stationary, 1 = weight stationary
    if (rs1_2 == 0) {
      new_mode = systolic_state_t::OS;
    } else {
      new_mode = systolic_state_t::WS;
    }

    auto rs1_4_3 = (rs1 >> 2) & 0b11; // extract rs1[4:3], 0 = no activation, 1 = ReLU, 2 = ReLU6
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
    assert(new_shift >= 0 && new_shift < 32);

    #ifdef RISCV_ENABLE_SYSTOLIC_COMMITLOG
    printf("SYSTOLIC: config_ex - set dataflow mode from %d to %d\n", systolic_state.mode, new_mode);
    printf("SYSTOLIC: config_ex - set activation function from %d to %d\n", systolic_state.act, new_act);
    printf("SYSTOLIC: config_ex - set shift from %01lx to %01lx\n", systolic_state.shift, rs2);
    #endif
    systolic_state.mode = new_mode;
    systolic_state.act = new_act;
    systolic_state.shift = new_shift;
  } else if ((rs1 & 0b11) == 1) { // rs1[1:0] == 2'b01, config_mvin, configure load pipeline
    systolic_state.load_stride = rs2;
  } else if ((rs1 & 0b11) == 2) { // rs1[1:0] == 2'b10, config_mvout, configure store pipeline
    systolic_state.store_stride = rs2;
  }
}

void systolic_t::compute(reg_t a_addr, reg_t b_addr, bool preload) {
  #ifdef RISCV_ENABLE_SYSTOLIC_COMMITLOG
    printf("SYSTOLIC: compute - preload = %d, scratchpad A addr = 0x%08lx, scratchpad B addr 0x%08lx\n", preload, a_addr, b_addr);
  #endif
  if (preload) {
    for (size_t i = 0; i < dim; i++) {
      for (size_t j = 0; j < dim; j++) {
        if (~systolic_state.preload_sp_addr == 0) {
          systolic_state.pe_state->at(i).at(j) = 0;
          #ifdef RISCV_ENABLE_SYSTOLIC_COMMITLOG
            printf("SYSTOLIC: compute - writing preload value 0 to PE %02lu,%02lu\n", i, j);
          #endif
        } else {
          systolic_state.pe_state->at(i).at(j) = (accum_t)get_matrix_element(systolic_state.preload_sp_addr, i, j);
          #ifdef RISCV_ENABLE_SYSTOLIC_COMMITLOG
            printf("SYSTOLIC: compute - writing preload value %08d from scratchpad base address 0x%08lx to PE %02lu,%02lu\n",
                   systolic_state.pe_state->at(i).at(j), systolic_state.preload_sp_addr, i, j);
          #endif
        }
      }
    }
  }
  // Output stationary
  if (systolic_state.mode == 0) {
    for (size_t i = 0; i < dim; i++) {
      for (size_t j = 0; j < dim; j++) {
        for (size_t k = 0; k < dim; k++) {
          systolic_state.pe_state->at(i).at(j) +=
                  // A is stored transposed in the scratchpad
                  (accum_t)get_matrix_element(a_addr, i, k) * (accum_t)get_matrix_element(b_addr, k, j);
        }
        if (~systolic_state.output_sp_addr != 0) {
          #ifdef RISCV_ENABLE_SYSTOLIC_COMMITLOG
            printf("SYSTOLIC: compute - writing array state value %08d from PE %02lu,%02lu to scratchpad base address 0x%08lx\n",
                    systolic_state.pe_state->at(i).at(j), i, j, systolic_state.output_sp_addr*row_bytes + i*dim+ j);
          #endif
          if ((systolic_state.act == 1) && ((accum_t)(systolic_state.pe_state->at(i).at(j)) < 0)) {
            store_matrix_element(systolic_state.output_sp_addr, i, j, 0);
            #ifdef RISCV_ENABLE_SYSTOLIC_COMMITLOG
              printf("SYSTOLIC: compute - RELU zeroed the value %08d from PE %02lu,%02lu in scratchpad base address 0x%08lx\n",
                    systolic_state.pe_state->at(i).at(j), i, j, systolic_state.output_sp_addr*row_bytes + i*dim+ j);
            #endif
          } else {
            store_matrix_element(systolic_state.output_sp_addr, i, j, systolic_state.pe_state->at(i).at(j));
          }
        }
      }
    }
  }
  else {
    assert(false && "Weight stationary dataflow not supported");
  }
}

/**
 * Get an element of a matrix stored at the scratchpad address (base_sp_addr)
 * @param base_sp_addr
 * @param i Row index
 * @param j Column index
 * @return The extracted matrix element casted up to pe_datatype
 */
accum_t systolic_t::get_matrix_element(reg_t base_sp_addr, size_t i, size_t j) {
  // TODO: This doesn't sign extend the matrix element properly
  accum_t elem = 0;
  /*
  for (size_t byte = 0; byte < (data_width / 8); ++byte) {
    elem = elem | (systolic_state.spad->at(base_sp_addr*row_bytes + i*row_bytes + j*(data_width/8) + byte) << 8*byte);
  }
   */
  return elem;
}

void systolic_t::store_matrix_element(reg_t base_sp_addr, size_t i, size_t j, accum_t value) {
  /*
  for (size_t byte = 0; byte < (data_width / 8); ++byte) {
    systolic_state.spad->at(base_sp_addr*row_bytes + i*row_bytes + j*(data_width/8) + byte) =
            (uint8_t)(((value >> systolic_state.shift) >> 8*byte) & 0xFF);
  }
   */
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

std::vector<disasm_insn_t*> rocc_t::get_disasms()
{
  std::vector<disasm_insn_t*> insns;
  return insns;
}
