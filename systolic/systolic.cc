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
  assert(sp_banks > 0 && "Systolic sp_banks must be > 0");
  assert(data_width >= 8 && data_width <= 32 && "Systolic data_width must be <= 32 bits and >= 8 bits");
  assert((data_width & (data_width - 1)) == 0 && "Systolic data_width must be a power of 2");
  assert(dim > 0 && "Systolic --dim must be > 0");
  assert(dim * data_width % 8 == 0 && "Systolic row size must be byte-aligned");
  assert(sp_bank_entries >= dim && "Systolic sp_banks_entries must be >= dim");

  enable = true;
  dataflow_mode = 0;
  output_sp_addr = 0;
  spad = new std::vector<uint8_t>(sp_banks * row_bytes * sp_bank_entries);
  pe_state = new std::vector<std::vector<int32_t>>(dim, std::vector<int32_t>(dim));

  printf("Systolic extension configured with:\n");
  printf("    data_width = %u\n", data_width);
  printf("    dim = %u\n", dim);
  printf("    sp_banks = %u\n", sp_banks);
  printf("    row_bytes = %u\n", row_bytes);
  printf("    sp_bank_entries = %u\n", sp_bank_entries);
}

void systolic_t::reset() {
  systolic_state.reset();
}

void systolic_t::mvin(reg_t dram_addr, reg_t sp_addr) {
  auto sp_byte_addr = sp_addr * row_bytes;
  for (uint32_t j = 0; j < row_bytes; j++) {
    systolic_state.spad->at(sp_byte_addr) = p->get_mmu()->load_uint8(dram_addr);
    #ifdef RISCV_ENABLE_SYSTOLIC_COMMITLOG
      printf("SYSTOLIC: mvin - value %08d from 0x%08lx to scratchpad byte addr 0x%08lx\n",
              systolic_state.spad->at(sp_byte_addr), dram_addr, sp_byte_addr);
    #endif
    dram_addr++;
    sp_byte_addr++;
  }
}

void systolic_t::mvout(reg_t dram_addr, reg_t sp_addr) {
  auto sp_byte_addr = sp_addr * row_bytes;
  for (uint32_t j = 0; j < row_bytes; j++) {
    p->get_mmu()->store_uint8(dram_addr, systolic_state.spad->at(sp_byte_addr));
    #ifdef RISCV_ENABLE_SYSTOLIC_COMMITLOG
      printf("SYSTOLIC: mvout - value %08d from scratchpad byte addr 0x%08lx to 0x%08lx\n",
             systolic_state.spad->at(sp_byte_addr), sp_byte_addr, dram_addr);
    #endif
    dram_addr++;
    sp_byte_addr++;
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

void systolic_t::setmode(reg_t mode) {
  // matmul.setmode: OS vs WS. takes in rs1, and interprets the LSB: 1 is OS, 0 is WS.
  assert(mode == 0 || mode == 1);
  #ifdef RISCV_ENABLE_SYSTOLIC_COMMITLOG
    printf("SYSTOLIC: setmode - set dataflow mode from %01lx to %01lx\n", systolic_state.dataflow_mode, mode);
  #endif
  systolic_state.dataflow_mode = mode;
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
          systolic_state.pe_state->at(i).at(j) = get_matrix_element(systolic_state.preload_sp_addr, i, j);
          #ifdef RISCV_ENABLE_SYSTOLIC_COMMITLOG
            printf("SYSTOLIC: compute - writing preload value %08d from scratchpad base address 0x%08lx to PE %02lu,%02lu\n",
                   systolic_state.pe_state->at(i).at(j), systolic_state.preload_sp_addr, i, j);
          #endif
        }
      }
    }
  }
  // Output stationary
  if (systolic_state.dataflow_mode == 0) {
    for (size_t i = 0; i < dim; i++) {
      for (size_t j = 0; j < dim; j++) {
        for (size_t k = 0; k < dim; k++) {
          systolic_state.pe_state->at(i).at(j) +=
                  // A is stored transposed in the scratchpad
                  get_matrix_element(a_addr, k, i) * get_matrix_element(b_addr, k, j);
        }
        if (~systolic_state.output_sp_addr != 0) {
          #ifdef RISCV_ENABLE_SYSTOLIC_COMMITLOG
            printf("SYSTOLIC: compute - writing array state value %08d from PE %02lu,%02lu to scratchpad base address 0x%08lx\n",
                    systolic_state.pe_state->at(i).at(j), i, j, systolic_state.output_sp_addr*row_bytes + i*dim+ j);
          #endif
          store_matrix_element(systolic_state.output_sp_addr, i, j, systolic_state.pe_state->at(i).at(j));
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
pe_datatype systolic_t::get_matrix_element(reg_t base_sp_addr, size_t i, size_t j) {
  // TODO: This doesn't sign extend the matrix element properly
  pe_datatype elem = 0;
  for (size_t byte = 0; byte < (data_width / 8); ++byte) {
    elem = elem | (systolic_state.spad->at(base_sp_addr*row_bytes + i*row_bytes + j*(data_width/8) + byte) << 8*byte);
  }
  return elem;
}

void systolic_t::store_matrix_element(reg_t base_sp_addr, size_t i, size_t j, pe_datatype value) {
  for (size_t byte = 0; byte < (data_width / 8); ++byte) {
    systolic_state.spad->at(base_sp_addr*row_bytes + i*row_bytes + j*(data_width/8) + byte) =
            (uint8_t)((value >> 8*byte) & 0xFF);
  }
}

reg_t systolic_t::custom3(rocc_insn_t insn, reg_t xs1, reg_t xs2) {
  printf("insn.funct %d\n", insn.funct);
  if (insn.funct == mvin_funct)
    mvin(xs1, xs2);
  else if (insn.funct == mvout_funct)
    mvout(xs1, xs2);
  else if (insn.funct == preload_funct)
    preload(xs1, xs2);
  else if (insn.funct == setmode_funct)
    setmode(xs1);
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
