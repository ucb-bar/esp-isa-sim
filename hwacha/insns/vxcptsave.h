require_supervisor_hwacha;
reg_t addr = XS1;

#define STORE_B(addr, value) \
  p->get_mmu()->store_uint8(addr, value); \
  addr += 1; \

#define STORE_W(addr, value) \
  p->get_mmu()->store_uint32(addr, value); \
  addr += 4; \

#define STORE_D(addr, value) \
  p->get_mmu()->store_uint64(addr, value); \
  addr += 8; \

//Control Thread State
STORE_W(addr, NXPR);
STORE_W(addr, NPPR);
STORE_W(addr, MAXVL);
STORE_W(addr, VL);
STORE_W(addr, UTIDX);
addr += 4;
STORE_D(addr, VF_PC);

for (uint32_t s=0; s<MAX_SPR; s++){
  STORE_D(addr, READ_SPR(s));
}

for (uint32_t a=0; a<MAX_APR; a++){
  STORE_D(addr, READ_APR(a));
}

//Worker Thread State
for (uint32_t x=0; x<NXPR; x++) {
  for (uint32_t i=0; i<VL; i++) {
    STORE_D(addr, UT_READ_XPR(i, x));
  }
}

for (uint32_t pred=0; pred<NPPR; pred++) {
  for (uint32_t i=0; i<VL; i++) {
    STORE_D(addr, UT_READ_PPR(i, pred));
  }
}

for (uint32_t i=0; i<VL; i++) {
  STORE_B(addr, h->get_ut_state(i)->run);
}

#undef STORE_B
#undef STORE_W
#undef STORE_D

#include "insns/vxcptkill.h"
