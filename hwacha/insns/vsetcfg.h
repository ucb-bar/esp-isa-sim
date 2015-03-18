uint32_t nxpr = (XS1 & 0x3f) + (insn.i_imm() & 0x3f);
uint32_t nppr = ((XS1 >> 8) & 0xf) + ((insn.i_imm() >> 8) & 0xf) + 1;
if (nxpr > 256)
  h->take_exception(HWACHA_CAUSE_ILLEGAL_CFG, 0);
if (nppr > 16)
  h->take_exception(HWACHA_CAUSE_ILLEGAL_CFG, 1);
WRITE_NXPR(nxpr);
WRITE_NPPR(nppr);
uint32_t maxvl;
if (nxpr < 2)
  maxvl = 8 * 256;
else
  maxvl = 8 * (256 / (nxpr-1));
WRITE_MAXVL(maxvl);
for(uint32_t i=0; i<maxvl; i++)
  WRITE_PPR_NO_PRED(i,0,1);
WRITE_VL(0);
WRITE_ENABLE(true);
