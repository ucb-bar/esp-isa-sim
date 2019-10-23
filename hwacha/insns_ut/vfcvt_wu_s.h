require_fp;
softfloat_roundingMode = VFRM;
WRITE_RD(sext32(f32_to_ui32(f32(FRS1), VFRM, true)));
set_fp_exceptions;
