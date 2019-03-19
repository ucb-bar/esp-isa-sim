if(!ENABLED) 
  h->take_exception(HWACHA_CAUSE_ILLEGAL_INSTRUCTION, VF_PC);

uint32_t vxsat = (uint32_t)XS1;
WRITE_VXSAT(vxsat);

#ifdef RISCV_ENABLE_HCOMMITLOG
  printf("H: VSETXSAT %d\n", vxsat);
#endif
