require_rv64;
require_fp;
softfloat_roundingMode = VFRM;
WRITE_RD(f32_to_i64(f32(FRS1), VFRM, true));
set_fp_exceptions;
