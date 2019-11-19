softfloat_roundingMode = VFRM;
WRITE_FRD(f64_add(f32_to_f64(f32(FRS1)), f32_to_f64(f32(FRS2))));
set_fp_exceptions;
