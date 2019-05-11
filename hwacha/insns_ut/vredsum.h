if (UTIDX == 0) {
  WRITE_UTRED(RS2);
}
for(uint32_t j=0; j<VL; j++){
  if(VPRED){
    WRITE_UTRED(UTRED + RS1);
  }
}
WRITE_RD(UTRED);
