require_fp;
softfloat_roundingMode = VFRM;
WRITE_SFRD(i32_to_f32((int32_t)RS1).v);
set_fp_exceptions;
