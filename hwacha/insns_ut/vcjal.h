if(COND){
  reg_t temp = VF_PC;
  WRITE_SRD(npc);
  npc = ((temp + insn.vc_imm()) & ~7);
}
