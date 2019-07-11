require_fp;
softfloat_roundingMode = VRM;
size_t depth = VDEPTH + 1;
reg_t res = FRS3;
for(size_t d = 0; d < depth; d++) {
  reg_t a = read_vrs1(h, insn, d);
  reg_t b = read_xpr(h, insn, UTIDX, INSN_VRS2+d);
  res = *fregs(f32_mulAdd(f32(cvt_hs(a)), f32(cvt_hs(b)), f32(cvt_hs(res)))).v;
}
WRITE_RD(res);
set_fp_exceptions;
