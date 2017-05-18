require_fp;
softfloat_roundingMode = VRM;
WRITE_HFRD(f32_div(f32(HFRS1), f32(HFRS2)));
set_fp_exceptions;
