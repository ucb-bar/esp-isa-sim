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
  pe_state = new std::vector<std::vector<accum_t>>(dim, std::vector<accum_t>(dim));
  accumulator = new std::vector<std::vector<accum_t>>(accum_rows, std::vector<accum_t>(dim));

  printf("Systolic extension configured with:\n");
  printf("    dim = %u\n", dim);
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

  // Preload
  if (preload) {
    for (size_t i = 0; i < dim; i++) {
      for (size_t j = 0; j < dim; j++) {
        if (~systolic_state.preload_sp_addr == 0) {
          systolic_state.pe_state->at(i).at(j) = 0;
          #ifdef RISCV_ENABLE_SYSTOLIC_COMMITLOG
            printf("SYSTOLIC: compute - PE %02lu,%02lu preloaded with 0\n", i, j);
          #endif
        } else {
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
  if (systolic_state.mode == systolic_state_t::OS) {
    for (size_t i = 0; i < dim; ++i) {
      for (size_t j = 0; j < dim; ++j) {
        for (size_t k = 0; k < dim; ++k) {
          systolic_state.pe_state->at(i).at(j) += systolic_state.spad->at(a_addr + i).at(k) * systolic_state.spad->at(b_addr + k).at(j);
        }
      }
    }
  }
  else {
    assert(false && "Weight stationary dataflow not supported");
  }

  for (size_t i = 0; i < dim; ++i) {
    for (size_t j = 0; j < dim; ++j) {
      #ifdef RISCV_ENABLE_SYSTOLIC_COMMITLOG
      printf("SYSTOLIC: compute - PE %02lu,%02lu computed matmul result %08d\n",
             i, j, systolic_state.pe_state->at(i).at(j));
      #endif

    }
  }

  // Write results to scratchpad
  // TODO: handle writing to accumulator
  if (~systolic_state.output_sp_addr != 0) {
    for (size_t i = 0; i < dim; ++i) {
      for (size_t j = 0; j < dim; ++j) {
        // Perform rounding shift and saturating cast to output_t
        accum_t value = systolic_state.pe_state->at(i).at(j);
        output_t shifted = rounding_saturating_shift(value);

        // Apply activation function
        output_t activated = shifted; // default to no activation function
        if (systolic_state.act == systolic_state_t::RELU) {
          activated = shifted > 0 ? shifted : static_cast<output_t>(0);
        } else if (systolic_state.act == systolic_state_t::RELU6) {
          auto positive = shifted > 0 ? shifted : static_cast<output_t>(0);
          activated = shifted > 6 ? static_cast<output_t>(6) : positive;
        }

        #ifdef RISCV_ENABLE_SYSTOLIC_COMMITLOG
        printf("SYSTOLIC: compute - PE %02lu,%02lu applied shift and activation to get %08d\n",
               i, j, activated);
        #endif

        // Move to scratchpad // TODO: is there a special case when sizeof(output_t) != sizeof(input_t)
        systolic_state.spad->at(systolic_state.output_sp_addr + i).at(j) = activated;
        #ifdef RISCV_ENABLE_SYSTOLIC_COMMITLOG
          printf("SYSTOLIC: compute - PE %02lu,%02lu wrote value %08d to scratchpad address 0x%08lx\n",
                 i, j, systolic_state.pe_state->at(i).at(j), systolic_state.output_sp_addr + i);
        #endif
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

output_t systolic_t::rounding_saturating_shift(accum_t value) {
  // Implementation taken from systolic-rocc-tests/include/systolic.h (matshift() function)
  int divisor = 1 << systolic_state.shift;
  // Bitshift and round element
  int64_t abs = value > 0 ? value : -value;
  int64_t shifted = (abs + (divisor/2)) / divisor;
  if (value < 0)
    shifted = -shifted;

  // Saturate and cast element
  auto elem_t_max = std::numeric_limits<input_t>::max();
  auto elem_t_min = std::numeric_limits<input_t>::min();
  int64_t elem = shifted > elem_t_max ? elem_t_max : (shifted < elem_t_min ? elem_t_min : shifted);
  return elem;
}

std::vector<disasm_insn_t*> rocc_t::get_disasms()
{
  std::vector<disasm_insn_t*> insns;
  return insns;
}
