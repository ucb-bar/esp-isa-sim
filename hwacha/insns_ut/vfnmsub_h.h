require_fp;
softfloat_roundingMode = VRM;
WRITE_HFRD(f32_mulAdd(HFRS1 ^ (uint32_t)INT32_MIN, HFRS2, HFRS3));
set_fp_exceptions;
