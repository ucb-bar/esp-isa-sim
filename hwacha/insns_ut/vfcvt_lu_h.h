require_rv64;
require_fp;
softfloat_roundingMode = VRM;
WRITE_RD(f32_to_ui64(HFRS1, VRM, true));
set_fp_exceptions;
