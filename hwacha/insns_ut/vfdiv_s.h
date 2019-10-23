require_fp;
softfloat_roundingMode = VFRM;
WRITE_SFRD(f32_div(f32(FRS1), f32(FRS2)).v);
set_fp_exceptions;
