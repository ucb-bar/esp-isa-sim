if(COND){
  reg_t temp = VF_PC;
  WRITE_SRD(npc);
  WRITE_VF_PC((temp + insn.vc_imm()) & ~7);
}
