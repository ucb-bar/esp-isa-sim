#include "systolic.h"
#include "mmu.h"
#include "trap.h"
#include <stdexcept>

REGISTER_EXTENSION(systolic, []() { return new systolic_t; })

void systolic_state_t::reset()
{
  enable = true;

  dataflow_mode = 0;
  output_sp_addr = 0;
  //randomize non-zero registers
  for (size_t i = 1; i < MAX_SCRATCHPAD_SIZE; i++) {
    //SCRATCHPAD[i] = reg_dis(gen);
    SCRATCHPAD[i] = 0;
  }
}

void systolic_t::reset()
{
  systolic_state.reset();
}


//reg_t rocc_t::custom3(rocc_insn_t insn, reg_t xs1, reg_t xs2)
reg_t systolic_t::custom3(rocc_insn_t insn, reg_t xs1, reg_t xs2)
{
    if (insn.funct == 1)
    {
      illegal_instruction(); 
    }
    //mvin transfer X bytes from dram to scratchpad. X is a the number of bytes as the width of the systolic array
    else if (insn.funct == 2)
    {
      reg_t src_addr = xs1;
      reg_t sp_addr = xs2;
      for (uint32_t j=0; j<ARRAY_X_DIM; j++) {
        printf("SYSTOLIC: move in from %016lx to scratchpad %016lx\n", src_addr, sp_addr);
        systolic_state.SCRATCHPAD[sp_addr] = (int32_t)p->get_mmu()->load_uint8(src_addr);
        src_addr++; 
        sp_addr++; 
      }
    } 
    //mvout
    else if (insn.funct == 3)
    {
      reg_t dst_addr = xs1; 
      reg_t sp_addr = xs2;
      for (uint32_t j=0; j<ARRAY_Y_DIM; j++) { 
        printf("SYSTOLIC: move out from scratchpad %016lx to %016lx\n", sp_addr, dst_addr);
        p->get_mmu()->store_uint8(dst_addr, systolic_state.SCRATCHPAD[sp_addr]);
        dst_addr++;
        sp_addr++;
      }
    }
    //compute with preload
    else if (insn.funct == 4)
    {
      for (uint32_t i=0; i<ARRAY_X_DIM; i++) {
        for (uint32_t j=0; j<ARRAY_Y_DIM; j++) {
           systolic_state.PE_array_state[i][j] = systolic_state.SCRATCHPAD[systolic_state.preload_sp_addr + i*ARRAY_X_DIM + j];
        }
      }
      reg_t a_addr = xs1;
      reg_t b_addr = xs2;
      for (uint32_t i=0; i<ARRAY_X_DIM; i++) {
        for (uint32_t j=0; j<ARRAY_Y_DIM; j++) {
           systolic_state.PE_array_state[i][j] += systolic_state.SCRATCHPAD[a_addr+i] * systolic_state.SCRATCHPAD[b_addr+j];
           if (systolic_state.output_sp_addr !=  0xFFFFFFFF)
             systolic_state.SCRATCHPAD[systolic_state.output_sp_addr + i*ARRAY_X_DIM + j] = systolic_state.PE_array_state[i][j]; 
        }
      }
    }
    //compute with acculmulation of previous value. A and B are row major
    else if (insn.funct == 5)
    {
      reg_t a_addr = xs1;
      reg_t b_addr = xs2;
      for (uint32_t i=0; i<ARRAY_X_DIM; i++) {
        for (uint32_t j=0; j<ARRAY_Y_DIM; j++) {
           systolic_state.PE_array_state[i][j] += systolic_state.SCRATCHPAD[a_addr+i] * systolic_state.SCRATCHPAD[b_addr+j];
           if (systolic_state.output_sp_addr !=  0xFFFFFFFF)
             systolic_state.SCRATCHPAD[systolic_state.output_sp_addr + i*ARRAY_X_DIM + j] = systolic_state.PE_array_state[i][j]; 
        }
      }
    }
    //matmul.preload: input C scratchpad addr (XxX row major), output D scratchpad addr
    else if (insn.funct == 8)
    {
      systolic_state.output_sp_addr = xs1;
      systolic_state.preload_sp_addr = xs2;
    }
    //matmul.setmode: OS vs WS. takes in rs1, and interprets the LSB: 1 is OS, 0 is WS.
    else if (insn.funct == 9)
    {
      systolic_state.dataflow_mode = xs1;
    }
    else
    {
      illegal_instruction();
    }
  //return pc+4; 
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
