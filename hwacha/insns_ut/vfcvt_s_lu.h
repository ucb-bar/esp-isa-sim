require_rv64;
require_fp;
softfloat_roundingMode = VRM;
WRITE_SFRD(ui64_to_f32(RS1).v);
set_fp_exceptions;
