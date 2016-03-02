require_fp;
WRITE_SFRD(isNaNF32UI(FRS2) || f32_lt_quiet(f32(FRS1),f32(FRS2)) /* && FRS1 not NaN */
      ? FRS1 : FRS2);
set_fp_exceptions;
