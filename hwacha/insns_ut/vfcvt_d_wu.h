require_fp;
softfloat_roundingMode = VRM;
WRITE_FRD(ui32_to_f64((uint32_t)RS1).v);
set_fp_exceptions;
