{
    unsigned int shift = RS2 & (xlen - 1);
    sreg_t result = round_vxrm(RS1, VXRM, shift);
    result = sext_xlen(result) >> shift;
    // Saturation
    if (result < INT16_MIN) {
        result = INT16_MIN;
        WRITE_VXSAT(1);
    } else if (result > INT16_MAX) {
        result = INT16_MAX;
        WRITE_VXSAT(1);
    }
    WRITE_RD(result);
}
