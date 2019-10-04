#ifndef _DECODE_HWACHA_UT_H
#define _DECODE_HWACHA_UT_H

#include "decode.h"
#include "decode_hwacha.h"
#include "hwacha.h"
#include "hwacha_xcpt.h"

#undef RS1
#undef RS2
#undef RS3
#undef WRITE_RD

//Do we need this duplication or could we just call read/write_xpr
static inline reg_t read_vrs1(hwacha_t* h, insn_t insn, uint32_t idx)
{
  if (INSN_VRS1 >= h->get_ct_state()->nxpr)
    h->take_exception(HWACHA_CAUSE_VF_ILLEGAL_REGID, VF_PC);
  return UT_VRS1(idx);
}

static inline reg_t read_vrs2(hwacha_t* h, insn_t insn, uint32_t idx)
{
  if (INSN_VRS2 >= h->get_ct_state()->nxpr)
    h->take_exception(HWACHA_CAUSE_VF_ILLEGAL_REGID, VF_PC);
  return UT_VRS2(idx);
}

static inline reg_t read_vrs3(hwacha_t* h, insn_t insn, uint32_t idx)
{
  if (INSN_VRS3 >= h->get_ct_state()->nxpr)
    h->take_exception(HWACHA_CAUSE_VF_ILLEGAL_REGID, VF_PC);
  return UT_VRS3(idx);
}

static inline void write_vrd(hwacha_t* h, insn_t insn, uint32_t idx, reg_t value)
{
  if (INSN_VRD >= h->get_ct_state()->nxpr)
    h->take_exception(HWACHA_CAUSE_VF_ILLEGAL_REGID, VF_PC);
  UT_WRITE_VRD(idx, value);
}
#define VRS1 read_vrs1(h, insn, UTIDX)
#define VRS2 read_vrs2(h, insn, UTIDX)
#define VRS3 read_vrs3(h, insn, UTIDX)
#define WRITE_VRD(value) write_vrd(h, insn, UTIDX, value)

#define PRS1 UT_PRS1(UTIDX)
#define PRS2 UT_PRS2(UTIDX)
#define PRS3 UT_PRS3(UTIDX)
#define WRITE_PRD(value) UT_WRITE_PRD(UTIDX, value)
#define READ_PRD UT_READ_PPR(UTIDX, INSN_VRD)

//Generalized register operations (vec/shared) depending on flags
static inline reg_t read_rs1(hwacha_t* h, insn_t insn, uint32_t idx)
{
  if (INSN_VS1)
    return VRS1;
  else
    return SRS1;
}

static inline reg_t read_rs2(hwacha_t* h, insn_t insn, uint32_t idx)
{
  if (INSN_VS2)
    return VRS2;
  else
    return SRS2;
}

static inline reg_t read_rs3(hwacha_t* h, insn_t insn, uint32_t idx)
{
  if (INSN_VS3)
    return VRS3;
  else
    return SRS3;
}

static inline void write_rd(hwacha_t* h, insn_t insn, uint32_t idx, reg_t value)
{
  if (INSN_VD)
    WRITE_VRD(value);
  else
    WRITE_SRD(value);
}

#define RS1 read_rs1(h, insn, UTIDX)
#define RS2 read_rs2(h, insn, UTIDX)
#define RS3 read_rs3(h, insn, UTIDX)
#define WRITE_RD(value) write_rd(h, insn, UTIDX, value)

#undef FRS1
#undef FRS2
#undef FRS3
#undef WRITE_FRD

#define FRS1 RS1
#define FRS2 RS2
#define FRS3 RS3
#define WRITE_FRD(value) WRITE_RD(*fregs(value).v)

static inline freg_t fregs(float32_t f)
{
	int32_t v = f.v;
	return { (uint64_t)v, (uint64_t)(-(v < 0)) };
}
static inline freg_t fregs(float64_t f)
{
	return { f.v, (uint64_t)(-((int64_t)f.v < 0)) };
}
static inline freg_t fregs(float128_t f)
{
	return f;
}

// we assume the vector unit has floating-point alus
#undef require_fp
#define require_fp

#include "cvt16.h"

#define HFRS1 cvt_hs(FRS1)
#define HFRS2 cvt_hs(FRS2)
#define HFRS3 cvt_hs(FRS3)

#define WRITE_SFRD(value) WRITE_RD((sreg_t)((int32_t)(value)))
#define WRITE_HFRD(value) WRITE_RD((sreg_t)((int16_t)cvt_sh(*freg(value).v, RM)))

#define VEC_SEG_LOAD VEC_UT_SEG_LOAD
#define VEC_SEG_ST_LOAD VEC_UT_SEG_ST_LOAD

#define VEC_UT_SEG_LOAD(dst, func, inc) \
  VEC_UT_SEG_ST_LOAD(dst, func, INSN_VSEG*inc, inc)

#ifdef RISCV_ENABLE_HCOMMITLOG
#define VEC_UT_SEG_ST_LOAD(dst, func, stride, inc) \
  if(VPRED) { \
    reg_t addr = ARS1+stride*UTIDX; \
    for (uint32_t j=0; j<INSN_VSEG; j++) { \
      printf("HMEM: read from %016lx\n", addr); \
      UT_WRITE_##dst(UTIDX, INSN_VRD+j, p->get_mmu()->func(addr)); \
      addr += inc; \
    } \
  }
#else
#define VEC_UT_SEG_ST_LOAD(dst, func, stride, inc) \
  if(VPRED) { \
    reg_t addr = ARS1+stride*UTIDX; \
    for (uint32_t j=0; j<INSN_VSEG; j++) { \
      UT_WRITE_##dst(UTIDX, INSN_VRD+j, p->get_mmu()->func(addr)); \
      addr += inc; \
    } \
  }
#endif


#define VEC_SEG_STORE VEC_UT_SEG_STORE
#define VEC_SEG_ST_STORE VEC_UT_SEG_ST_STORE

#define VEC_UT_SEG_STORE(src, func, inc) \
  VEC_UT_SEG_ST_STORE(src, func, INSN_VSEG*inc, inc)

#ifdef RISCV_ENABLE_HCOMMITLOG
#define VEC_UT_SEG_ST_STORE(src, func, stride, inc) \
  if(VPRED) { \
    reg_t addr = ARS1+stride*UTIDX; \
    for (uint32_t j=0; j<INSN_VSEG; j++) { \
      reg_t stval = UT_READ_##src(UTIDX, INSN_VRD+j); \
      printf("HMEM: write %016lx with %016lx\n", addr, stval); \
      p->get_mmu()->func(addr, stval); \
      addr += inc; \
    } \
  }
#else
#define VEC_UT_SEG_ST_STORE(src, func, stride, inc) \
  if(VPRED) { \
    reg_t addr = ARS1+stride*UTIDX; \
    for (uint32_t j=0; j<INSN_VSEG; j++) { \
      reg_t stval = UT_READ_##src(UTIDX, INSN_VRD+j); \
      p->get_mmu()->func(addr, stval); \
      addr += inc; \
    } \
  }
#endif

#endif
