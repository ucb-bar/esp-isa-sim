require_fp;
softfloat_roundingMode = VRM;
WRITE_FRD(f32_add(f32(FRS1), f32(FRS2)));
set_fp_exceptions;
