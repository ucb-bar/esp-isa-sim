require_fp;
WRITE_HFRD(f32_le_quiet(f32(HFRS2), f32(HFRS1)) || isNaNF32UI(f32(HFRS2).v) ? f32(HFRS1) : f32(HFRS2));
if ((isNaNF32UI(f32(HFRS1).v) && isNaNF32UI(f32(HFRS2).v)) || softfloat_exceptionFlags)
  WRITE_HFRD(f32(defaultNaNF32UI));
set_fp_exceptions;
