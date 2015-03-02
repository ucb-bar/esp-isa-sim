if(COND){
  reg_t temp = SRS1;
  WRITE_SRD(npc);
  WRITE_VF_PC((temp + insn.vc_imm()) & ~7);
}
