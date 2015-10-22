reg_t index = (PRS1) | (PRS2 << 1) | (PRS3 << 2);
WRITE_PPR_NO_PRED(UTIDX, INSN_VRD, !!(POP_TABLE & (0x1 << index)));
