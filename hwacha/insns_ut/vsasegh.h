for (uint32_t j=0; j<INSN_VSEG; j++){
  MMU.store_uint16(ARS1, READ_SPR(INSN_VRD+j));
}
