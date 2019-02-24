switch (VXRM)
{
    case 0:
        WRITE_RD(sext_xlen(sext_xlen(RS1 + (1 << (xlen/2))) >> (RS2 & (xlen-1))));
        break;
    case 1:
        if (1 << (xlen/2))
           WRITE_RD(sext_xlen(sext_xlen(RS1 + (1 << (xlen/2))) >> (RS2 & (xlen-1))));
        else
           WRITE_RD(sext_xlen(sext_xlen(RS1) >> (RS2 & (xlen-1))));
        break;
    case 2:
        WRITE_RD(sext_xlen(sext_xlen(RS1) >> (RS2 & (xlen-1))));
        break;
    case 3:
        WRITE_RD(sext_xlen(sext_xlen(RS1 | ((RS1 & ((1<<(xlen/2))-1)) > 0)) >> (RS2 & (xlen-1))));
        break;
}
