require_fp;
softfloat_roundingMode = VRM;
WRITE_FRD(f64_mul(f32_to_f64(f32(FRS1)), f32_to_f64(f32(FRS2))));
set_fp_exceptions;
