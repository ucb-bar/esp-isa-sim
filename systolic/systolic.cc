#include "systolic.h"
#include "mmu.h"
#include "trap.h"
#include <stdexcept>
#include <iostream>
#include <assert.h>

#define RISCV_ENABLE_SYSTOLIC_COMMITLOG 1

REGISTER_EXTENSION(systolic, []() { return new systolic_t; })

void systolic_state_t::reset(uint32_t data_width, uint32_t dim, uint32_t sp_banks, uint32_t sp_bank_entries)
{
  assert(data_width != 0 && "Must pass --data-width=<dtype.width> (e.g. --data-width=8)");
  assert(dim != 0 && "Must pass --dim=<tileRows*meshRows> (e.g. --dim=16)");
  assert(sp_banks != 0 && "Must pass --sp_banks=<number of scratchpad banks> (e.g. --sp-banks=4)");

  assert(data_width > 0 && data_width <= 32 && "Systolic --data-width must be <= 32 bits");
  assert((data_width & (data_width - 1)) == 0 && "Systolic --data-width must be a power of 2");
  assert(dim > 0 && "Systolic --dim must be > 0");
  assert(dim * data_width % 8 == 0 && "Systolic row size must be byte-aligned");

  enable = true;
  dataflow_mode = 0;
  output_sp_addr = 0;
  auto row_bytes = data_width * dim / 8;
  // If the user hasn't specified sp_bank_entries, assume enough space for the entire depth of the systolic array
  auto sp_entries = sp_bank_entries == 0 ? row_bytes * dim : row_bytes * sp_bank_entries;
  spad = new std::vector<uint8_t>(sp_banks * sp_entries);
  pe_state = new std::vector<std::vector<int32_t>>(dim, std::vector<int32_t>(dim));

  printf("Systolic extension configured with:\n");
  printf("\tdata_width = %u\n", data_width);
  printf("\tdim = %u\n", dim);
  printf("\tsp_banks = %u\n", sp_banks);
  printf("\tsp_entries = %u\n", sp_entries);
}

void systolic_t::reset() {
  systolic_state.reset(data_width, dim, sp_banks, sp_bank_entries);
}

void systolic_t::mvin(reg_t dram_addr, reg_t sp_addr) {
  auto sp_byte_addr = sp_addr * row_bytes();
  #ifdef RISCV_ENABLE_SYSTOLIC_COMMITLOG
    printf("mvin: ");
  #endif
  for (uint32_t j = 0; j < row_bytes(); j++) {
    systolic_state.spad->at(sp_byte_addr) = p->get_mmu()->load_uint8(dram_addr);
    #ifdef RISCV_ENABLE_SYSTOLIC_COMMITLOG
    printf("spad[%lu] = %d, ", sp_byte_addr, systolic_state.spad->at(sp_byte_addr));
    //printf("SYSTOLIC: move in value %08d from %016lx to scratchpad %016lx\n",
    //systolic_state.spad->at(sp_addr), src_addr, sp_addr);
    #endif
    dram_addr++;
    sp_byte_addr++;
  }
}

void systolic_t::mvout(reg_t dram_addr, reg_t sp_addr) {
  auto sp_byte_addr = sp_addr * row_bytes();
  #ifdef RISCV_ENABLE_SYSTOLIC_COMMITLOG
    printf("mvout: ");
  #endif
  for (uint32_t j = 0; j < row_bytes(); j++) {
    p->get_mmu()->store_uint8(dram_addr, systolic_state.spad->at(sp_byte_addr));
    #ifdef RISCV_ENABLE_SYSTOLIC_COMMITLOG
      printf("SYSTOLIC: move out value %016x from scratchpad %016lx to %016lx\n",
             systolic_state.spad->at(sp_byte_addr), sp_byte_addr, dram_addr);
    #endif
    dram_addr++;
    sp_byte_addr++;
  }
}

//matmul.preload: input C scratchpad addr (XxX row major), output D scratchpad addr
void systolic_t::preload(reg_t d_addr, reg_t c_addr) {
  systolic_state.preload_sp_addr = d_addr;
  systolic_state.output_sp_addr = c_addr;
  #ifdef RISCV_ENABLE_SYSTOLIC_COMMITLOG
    printf("SYSTOLIC: set scratchpad output addr to %016lx\n", systolic_state.output_sp_addr);
    printf("SYSTOLIC: set scratchpad preload addr to %016lx\n", systolic_state.preload_sp_addr);
  #endif
}

//matmul.setmode: OS vs WS. takes in rs1, and interprets the LSB: 1 is OS, 0 is WS.
void systolic_t::setmode(reg_t mode) {
  assert(mode == 0 || mode == 1);
  #ifdef RISCV_ENABLE_SYSTOLIC_COMMITLOG
    printf("SYSTOLIC: set dataflow mode from %016lx to %016lx\n", systolic_state.dataflow_mode, mode);
  #endif
  systolic_state.dataflow_mode = mode;
}

void systolic_t::compute(reg_t a_addr, reg_t b_addr, bool preload) {
  if (preload) {
#ifdef RISCV_ENABLE_SYSTOLIC_COMMITLOG
    printf("SYSTOLIC: start matmul instruction (with preload) withh A_addr %016lx and B_addr %016lx instruction\n", a_addr, b_addr);
#endif
    for (size_t i = 0; i < dim; i++) {
      for (size_t j = 0; j < dim; j++) {
        systolic_state.pe_state->at(i).at(j) =
                (int32_t)systolic_state.spad->at(systolic_state.preload_sp_addr + i*dim + j);
#ifdef RISCV_ENABLE_SYSTOLIC_COMMITLOG
        //printf("SYSTOLIC: writing preload value %016x from scratchpad address %016x to PE %d,%d\n",
        //systolic_state.pe_state[i][j], systolic_state.preload_sp_addr + i*ARRAY_X_DIM + j, i, j);
#endif
      }
    }
  }
  auto a_byte_addr = a_addr * row_bytes();
  auto b_byte_addr = b_addr * row_bytes();
  for (size_t i = 0; i < dim; i++) {
    for (size_t j = 0; j < dim; j++) {
      for (size_t k = 0; k < dim; k++) {
        systolic_state.pe_state->at(i).at(j) +=
                systolic_state.spad->at(a_byte_addr + k*dim + i) * systolic_state.spad->at(b_byte_addr + k*dim + j);
      }
      if (systolic_state.output_sp_addr !=  0xFFFFFFFF)
      {
#ifdef RISCV_ENABLE_SYSTOLIC_COMMITLOG
        //printf("SYSTOLIC: writing array state value %016x from PE %d,%d to scratchpad address %016x\n",
        //systolic_state.pe_state[i][j], i, j, systolic_state.output_sp_addr + i*ARRAY_X_DIM + j);
#endif
        systolic_state.spad->at(systolic_state.output_sp_addr + i*dim + j) =
                (uint8_t) systolic_state.pe_state->at(i).at(j);
      }
    }
  }
}

reg_t systolic_t::custom3(rocc_insn_t insn, reg_t xs1, reg_t xs2) {
  #ifdef RISCV_ENABLE_SYSTOLIC_COMMITLOG
    printf("SYSTOLIC: ");
  #endif
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
  #ifdef RISCV_ENABLE_SYSTOLIC_COMMITLOG
    printf("\n");
  #endif
  return 0;
}

/*
std::vector<insn_desc_t> rocc_t::get_instructions()
{
  std::vector<insn_desc_t> insns;
  insns.push_back((insn_desc_t){0x0b, 0x7f, &::illegal_instruction, custom});
  insns.push_back((insn_desc_t){0x2b, 0x7f, &::illegal_instruction, custom});
  insns.push_back((insn_desc_t){0x5b, 0x7f, &::illegal_instruction, custom});
  insns.push_back((insn_desc_t){0x7b, 0x7f, &::illegal_instruction, custom});
  return insns;
}
*/
std::vector<disasm_insn_t*> rocc_t::get_disasms()
{
  std::vector<disasm_insn_t*> insns;
  return insns;
}
