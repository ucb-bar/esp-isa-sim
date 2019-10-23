require_rv64;
require_fp;
softfloat_roundingMode = VFRM;
WRITE_RD(f64_to_ui64(f64(FRS1), VFRM, true));
set_fp_exceptions;
