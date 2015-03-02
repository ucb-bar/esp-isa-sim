require_xpr64;
if(VPRED){
reg_t v = MMU.load_uint64(RS1);
MMU.store_uint64(RS1, std::min(RS2,v));
WRITE_RD(v);
}
