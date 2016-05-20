if(!ENABLED) 
  h->take_exception(HWACHA_CAUSE_ILLEGAL_INSTRUCTION, VF_PC);

uint32_t vl = std::min(MAXVL, (uint32_t)XS1);
WRITE_VL(vl);
WRITE_XRD(vl);

#ifdef RISCV_ENABLE_HCOMMITLOG
  printf("H: VSETVL maxvl=%ld vl=%ld\n", MAXVL,vl);
#endif
