require_fp;
softfloat_roundingMode = VRM;
WRITE_SFRD(f32_mulAdd(f32(FRS1), f32(FRS2), f32(FRS3)).v);
set_fp_exceptions;
