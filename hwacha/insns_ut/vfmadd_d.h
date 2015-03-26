require_fp;
softfloat_roundingMode = VRM;
WRITE_FRD(f64_mulAdd(FRS1, FRS2, FRS3));
set_fp_exceptions;
