require_fp;
WRITE_HFRD(isNaNF32UI(HFRS2) || f32_lt_quiet(f32(HFRS1),f32(HFRS2)) /* && FRS1 not NaN */
      ? HFRS1 : HFRS2);
set_fp_exceptions;
