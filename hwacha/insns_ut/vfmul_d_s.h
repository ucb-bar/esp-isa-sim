require_fp;
softfloat_roundingMode = RM;
WRITE_FRD(f64_mulAdd(f32_to_f64(FRS1), f32_to_f64(FRS2), (f32_to_f64(FRS1) ^ f32_to_f64(FRS2)) & (uint64_t)INT64_MIN));
set_fp_exceptions;
