require_fp;
softfloat_roundingMode = VFRM;
WRITE_HFRD(f32_mulAdd(f32(HFRS1), f32(HFRS2), f32(f32(HFRS3).v ^ F32_SIGN)));
set_fp_exceptions;
