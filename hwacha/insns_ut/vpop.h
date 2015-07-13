reg_t index = (PRS1 ^ INSN_VS1) | (PRS2 ^ INSN_VS2 << 1) | (PRS3 ^ INSN_VS3 << 1);
WRITE_PPR_NO_PRED(UTIDX, INSN_VRD, POP_TABLE & (0x1 << index));
