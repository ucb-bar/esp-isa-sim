if(COND){
  reg_t temp = SRS1;
  WRITE_SRD(npc);
  npc = ((temp + insn.vc_imm()) & ~7);
}
