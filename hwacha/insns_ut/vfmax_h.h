require_fp;
bool greater = f32_lt_quiet(f32(HFRS2), f32(HFRS1)) ||
               (f32_eq(f32(HFRS2), f32(HFRS1)) && (f32(HFRS2).v & F32_SIGN));
WRITE_FRD(greater || isNaNF32UI(f32(HFRS2).v) ? f32(HFRS1) : f32(HFRS2));
if (isNaNF32UI(f32(HFRS1).v) && isNaNF32UI(f32(HFRS2).v))
  WRITE_HFRD(f32(defaultNaNF32UI));
set_fp_exceptions;
