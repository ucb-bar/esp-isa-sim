require_fp;
softfloat_roundingMode = VFRM;
WRITE_SFRD(ui32_to_f32((uint32_t)RS1).v);
set_fp_exceptions;
