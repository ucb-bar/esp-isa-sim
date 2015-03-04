for (uint32_t j=0; j<INSN_VSEG; j++){
  MMU.store_uint16(SRS1 + RS2, UT_READ_XPR(UTIDX,INSN_VRD+j));
}
