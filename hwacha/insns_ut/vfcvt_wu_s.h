require_fp;
softfloat_roundingMode = VRM;
WRITE_RD(sext32(f32_to_ui32(f32(FRS1), VRM, true)));
set_fp_exceptions;
