if(!ENABLED) 
  h->take_exception(HWACHA_CAUSE_ILLEGAL_INSTRUCTION, VF_PC);

uint32_t vxrm = (uint32_t)XS1;
WRITE_VXRM(vxrm);

#ifdef RISCV_ENABLE_HCOMMITLOG
  printf("H: VSETXRM %d\n", vxrm);
#endif
