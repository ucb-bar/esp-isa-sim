require_fp;
softfloat_roundingMode = VFRM;
WRITE_SFRD(f32_mul(f32(HFRS1), f32(HFRS2)).v);
set_fp_exceptions;
