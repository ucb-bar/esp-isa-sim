require_fp;
softfloat_roundingMode = VRM;
WRITE_HFRD(ui32_to_f32((uint32_t)RS1).v);
set_fp_exceptions;
