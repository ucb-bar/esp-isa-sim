reg_t addr = ARS1+(UTIDX/8);
uint8_t temp = MMU.load_uint8(addr);
if(READ_PRD)
  MMU.store_uint8(addr, temp | (0x1 << (UTIDX%8)));
else
  MMU.store_uint8(addr, temp & ~(0x1 << (UTIDX%8)));
