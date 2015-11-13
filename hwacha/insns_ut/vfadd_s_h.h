softfloat_roundingMode = VRM;
WRITE_SFRD(f32_mulAdd(HFRS1, 0x3f800000, HFRS2));
set_fp_exceptions;
