require_rv64;
for (uint32_t j=0; j<INSN_VSEG; j++){
  MMU.store_uint64(SRS1, READ_SPR(INSN_VRS2+j));
}
