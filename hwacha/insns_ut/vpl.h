reg_t addr = ARS1+(UTIDX/8);
uint8_t temp = MMU.load_uint8(addr);
WRITE_PRD(temp & (0x1 << (UTIDX%8)));
