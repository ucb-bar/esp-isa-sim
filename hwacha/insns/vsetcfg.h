uint32_t nxpr = (XS1 & 0xff) + (insn.i_imm() & 0xff) + 1;
uint32_t nppr = ((XS1 >> 8) & 0xf) + ((insn.i_imm() >> 8) & 0xf) + 1;
WRITE_NXPR(nxpr);
WRITE_NPPR(nppr);
uint32_t maxvl;
if (nxpr < 2)
  maxvl = 8 * 256;
else
  maxvl = 8 * (256 / nxpr);
WRITE_MAXVL(maxvl);
for(uint32_t i=0; i<maxvl; i++)
  WRITE_PPR_NO_PRED(i,0,1);
WRITE_VL(0);
WRITE_ENABLE(true);
