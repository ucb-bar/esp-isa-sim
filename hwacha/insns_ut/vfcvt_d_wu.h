require_fp;
softfloat_roundingMode = VFRM;
WRITE_FRD(ui32_to_f64((uint32_t)RS1));
set_fp_exceptions;
