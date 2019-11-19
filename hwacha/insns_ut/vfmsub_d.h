require_fp;
softfloat_roundingMode = VFRM;
WRITE_FRD(f64_mulAdd(f64(FRS1), f64(FRS2), f64(f64(FRS3).v ^ F64_SIGN)));
set_fp_exceptions;
