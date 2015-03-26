require_fp;
softfloat_roundingMode = VRM;
WRITE_RD(sext32(f32_to_i32(HFRS1, VRM, true)));
set_fp_exceptions;
