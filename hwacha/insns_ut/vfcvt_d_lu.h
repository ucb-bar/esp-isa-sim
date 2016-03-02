require_rv64;
require_fp;
softfloat_roundingMode = VRM;
WRITE_FRD(ui64_to_f64(RS1).v);
set_fp_exceptions;
