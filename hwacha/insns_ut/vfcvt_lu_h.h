require_rv64;
require_fp;
softfloat_roundingMode = VFRM;
WRITE_RD(f32_to_ui64(f32(HFRS1), VFRM, true));
set_fp_exceptions;
