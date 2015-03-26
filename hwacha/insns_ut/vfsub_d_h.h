require_fp;
softfloat_roundingMode = VRM;
WRITE_FRD(f64_mulAdd(f32_to_f64(HFRS1), 0x3ff0000000000000ULL, f32_to_f64(HFRS2) ^ (uint64_t)INT64_MIN));
set_fp_exceptions;
