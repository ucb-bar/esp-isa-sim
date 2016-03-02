require_fp;
softfloat_roundingMode = VRM;
WRITE_HFRD(f32_mulAdd(f32(HFRS1), f32(HFRS2), f32(HFRS3 ^ (uint32_t)INT32_MIN)).v);
set_fp_exceptions;
