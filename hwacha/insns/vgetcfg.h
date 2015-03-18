//f(!ENABLED) 
//h->take_exception(HWACHA_CAUSE_ILLEGAL_INSTRUCTION, VF_PC);

WRITE_XRD((NXPR & 0x3f) | ((NPPR & 0x3f) << 6));
