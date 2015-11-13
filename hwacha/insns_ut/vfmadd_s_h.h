require_fp;
softfloat_roundingMode = VRM;
WRITE_SFRD(f32_mulAdd(HFRS1, HFRS2, HFRS3));
set_fp_exceptions;
