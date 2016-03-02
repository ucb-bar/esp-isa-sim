require_rv64;
require_fp;
softfloat_roundingMode = VRM;
WRITE_RD(f64_to_i64(f64(FRS1), VRM, true));
set_fp_exceptions;
