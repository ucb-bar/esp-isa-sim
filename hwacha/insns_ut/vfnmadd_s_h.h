require_fp;
softfloat_roundingMode = RM;
WRITE_FRD(f32_mulAdd(HFRS1 ^ (uint32_t)INT32_MIN, HFRS2, HFRS3 ^ (uint32_t)INT32_MIN));
set_fp_exceptions;
