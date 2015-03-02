for (uint32_t j=0; j<INSN_VSEG; j++){
  MMU.store_uint8(ARS1 + ARS2, READ_SPR(INSN_VRD+j));
}
