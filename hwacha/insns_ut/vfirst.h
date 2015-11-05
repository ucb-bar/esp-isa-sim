WRITE_SRD(0);
for(uint32_t j=0; j<VL; j++){
  WRITE_UTIDX(j);
  if(VPRED){
    WRITE_SRD(VRS1);
    break;
  }
}
WRITE_UTIDX(0);
