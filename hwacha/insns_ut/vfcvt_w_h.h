require_fp;
softfloat_roundingMode = VFRM;
WRITE_RD(sext32(f32_to_i32(f32(HFRS1), VFRM, true)));
set_fp_exceptions;
