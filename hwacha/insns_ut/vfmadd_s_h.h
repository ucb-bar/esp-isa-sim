require_fp;
softfloat_roundingMode = VRM;
WRITE_SFRD(f32_mulAdd(f32(HFRS1), f32(HFRS2), f32(HFRS3)).v);
set_fp_exceptions;
