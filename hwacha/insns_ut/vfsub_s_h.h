require_fp;
softfloat_roundingMode = VRM;
WRITE_SFRD(f32_mulAdd(HFRS1, 0x3f800000, HFRS2 ^ (uint32_t)INT32_MIN));
set_fp_exceptions;
