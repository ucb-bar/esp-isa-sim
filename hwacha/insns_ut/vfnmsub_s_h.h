require_fp;
softfloat_roundingMode = VFRM;
WRITE_SFRD(f32_mulAdd(f32(f32(HFRS1).v ^ F32_SIGN), f32(HFRS2), f32(HFRS3)).v);
set_fp_exceptions;
