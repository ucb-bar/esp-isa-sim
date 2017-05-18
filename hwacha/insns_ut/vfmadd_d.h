require_fp;
softfloat_roundingMode = VRM;
WRITE_FRD(f64_mulAdd(f64(FRS1), f64(FRS2), f64(FRS3)));
set_fp_exceptions;
