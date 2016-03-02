require_fp;
softfloat_roundingMode = VRM;
WRITE_SFRD(f32_mulAdd(f32(FRS1 ^ (uint32_t)INT32_MIN), f32(FRS2), f32(FRS3 ^ (uint32_t)INT32_MIN)).v);
set_fp_exceptions;
