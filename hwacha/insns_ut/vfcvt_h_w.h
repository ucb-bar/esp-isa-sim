require_fp;
softfloat_roundingMode = VRM;
WRITE_HFRD(i32_to_f32((int32_t)RS1));
set_fp_exceptions;
