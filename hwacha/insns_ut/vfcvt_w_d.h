require_fp;
softfloat_roundingMode = VRM;
WRITE_RD(sext32(f64_to_i32(FRS1, VRM, true)));
set_fp_exceptions;
