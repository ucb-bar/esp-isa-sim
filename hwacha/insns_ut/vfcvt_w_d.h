require_fp;
softfloat_roundingMode = VFRM;
WRITE_RD(sext32(f64_to_i32(f64(FRS1), VFRM, true)));
set_fp_exceptions;
