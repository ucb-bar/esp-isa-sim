require_xpr64;
for (uint32_t j=0; j<INSN_VSEG; j++){
  WRITE_SPR(INSN_VRD+j, MMU.load_int64(SRS1 + SRS2));
}
