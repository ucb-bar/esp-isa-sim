softfloat_roundingMode = VRM;
WRITE_FRD(f64_add(f32_to_f64(f32(FRS1)), f32_to_f64(f32(FRS2))).v);
set_fp_exceptions;
