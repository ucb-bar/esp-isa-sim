softfloat_roundingMode = RM;
WRITE_FRD(f32_mulAdd(HFRS1, 0x3f800000, HFRS2));
set_fp_exceptions;
