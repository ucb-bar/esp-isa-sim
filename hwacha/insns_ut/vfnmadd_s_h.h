require_fp;
softfloat_roundingMode = VRM;
WRITE_SFRD(f32_mulAdd(HFRS1 ^ (uint32_t)INT32_MIN, HFRS2, HFRS3 ^ (uint32_t)INT32_MIN));
set_fp_exceptions;
