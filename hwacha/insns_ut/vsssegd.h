require_xpr64;
for (uint32_t j=0; j<INSN_VSEG; j++){
  MMU.store_uint64(SRS1 + SRS2, READ_SPR(INSN_VRD+j));
}
