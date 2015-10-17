require_rv64;
if(VPRED) {
  for (uint32_t j=0; j<INSN_VSEG; j++){
    UT_WRITE_XPR(UTIDX, INSN_VRD+j, MMU.load_int64(SRS1 + RS2));
  }
}
