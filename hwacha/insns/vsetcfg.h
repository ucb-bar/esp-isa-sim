uint32_t nxpr = ((XS1 & 0x1ff) | (insn.i_imm() & 0x1ff)) +
  ((XS1 >> 14) & 0x1ff) + ((XS1 >> 23) & 0x1ff);
uint32_t nppr = ((XS1 >> 9) & 0x1f) | ((insn.i_imm() >> 9) & 0x1f);
WRITE_NXPR(nxpr);
WRITE_NPPR(nppr);
uint32_t maxvl_xpr, maxvl_ppr;
if (nxpr < 2) maxvl_xpr = 8 * 256;
else maxvl_xpr = 8 * (256 / nxpr);
if (nppr < 2) maxvl_ppr = 8 * 256;
else maxvl_ppr = 8 * (256 / nppr);
uint32_t maxvl = std::min(maxvl_xpr, maxvl_ppr);
WRITE_MAXVL(maxvl);
WRITE_VL(0);
WRITE_ENABLE(true);
