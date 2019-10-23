{
    unsigned int shift = RS2 & (xlen - 1);
    sreg_t result = round_vxrm(RS1, VXRM, shift);
    result = sext_xlen(result) >> shift;
    // Saturation
    if (result < INT32_MIN) {
        result = INT32_MIN;
        WRITE_VXSAT(1);
    } else if (result > INT32_MAX) {
        result = INT32_MAX;
        WRITE_VXSAT(1);
    }
    WRITE_RD(result);
}
