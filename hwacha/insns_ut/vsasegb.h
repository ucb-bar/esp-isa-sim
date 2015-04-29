for (uint32_t j=0; j<INSN_VSEG; j++){
  MMU.store_uint8(ARS1, READ_SPR(INSN_VRS2+j));
}
