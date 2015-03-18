#ifndef _DECODE_HWACHA_H
#define _DECODE_HWACHA_H

#include "hwacha.h"
#include "hwacha_xcpt.h"
#include "mmu.h"

#define XS1 (xs1)
#define XS2 (xs2)
#define WRITE_XRD(value) (xd = value)

//Control Thread Management
#define NXPR (h->get_ct_state()->nxpr)
#define NPPR (h->get_ct_state()->nppr)
#define MAXVL (h->get_ct_state()->maxvl)
#define VL (h->get_ct_state()->vl)
#define UTIDX (h->get_ct_state()->count)
#define PREC (h->get_ct_state()->prec)
#define VF_PC (h->get_ct_state()->vf_pc)
#define ENABLED (h->get_ct_state()->enable)
#define WRITE_ENABLE(en) (h->get_ct_state()->enable = en)
#define WRITE_NXPR(nxprnext) (h->get_ct_state()->nxpr = (nxprnext))
#define WRITE_NPPR(npprnext) (h->get_ct_state()->nppr = (npprnext))
#define WRITE_MAXVL(maxvlnext) (h->get_ct_state()->maxvl = (maxvlnext))
#define WRITE_VL(vlnext) (h->get_ct_state()->vl = (vlnext))
#define WRITE_UTIDX(value) (h->get_ct_state()->count = (value))
#define WRITE_VF_PC(pcnext) (h->get_ct_state()->vf_pc = (pcnext))
#define WRITE_PREC(precision) (h->get_ct_state()->prec = (precision))

#define VRM ({ int rm = insn.vrm(); \
              if(rm == 7) rm = h->get_ct_state()->frm; \
              if(rm > 4) throw trap_illegal_instruction(); \
              rm; })

#define POP_TABLE (insn.vpop_table())
#define VSHAMT (insn.vshamt())

//Shared regs
#define INSN_VRS1 (insn.vrs1())
#define INSN_VRS2 (insn.vrs2())
#define INSN_VRS3 (insn.vrs3())
#define INSN_VRD (insn.vrd())
#define INSN_VD    (insn.vd())
#define INSN_VS1   (insn.vs1())
#define INSN_VS2   (insn.vs2())
#define INSN_VS3   (insn.vs3())
#define INSN_SVSRD (insn.svsrd())

static inline reg_t read_spr(hwacha_t* h, insn_t insn, size_t src)
{
  if (src >= 256)
    h->take_exception(HWACHA_CAUSE_TVEC_ILLEGAL_REGID, uint64_t(insn.bits()));
  return (h->get_ct_state()->SPR[src]);
}

static inline void write_spr(hwacha_t* h, insn_t insn, size_t dst, reg_t value)
{
  if (dst >= 256)
    h->take_exception(HWACHA_CAUSE_TVEC_ILLEGAL_REGID, uint64_t(insn.bits()));
  h->get_ct_state()->SPR.write(dst, value);
}

#define READ_SPR(src) read_spr(h, insn, src)
#define WRITE_SPR(dst, value) write_spr(h, insn, dst, value)
#define SRS1 (READ_SPR(INSN_VRS1))
#define SRS2 (READ_SPR(INSN_VRS2))
#define SRS3 (READ_SPR(INSN_VRS3))
#define WRITE_SRD(value) (WRITE_SPR(INSN_VRD, value))
#define WRITE_SVSRD(value) (WRITE_SPR(INSN_SVSRD, value))

//Address regs
#define INSN_VARS1 (insn.vars1())
#define INSN_VARS2 (insn.vars2())
#define INSN_SVARD (insn.svard())
static inline reg_t read_apr(hwacha_t* h, insn_t insn, size_t src)
{
  if (src >= 32)
    h->take_exception(HWACHA_CAUSE_TVEC_ILLEGAL_REGID, uint64_t(insn.bits()));
  return (h->get_ct_state()->APR[src]);
}

static inline void write_apr(hwacha_t* h, insn_t insn, size_t dst, reg_t value)
{
  if (dst >= 32)
    h->take_exception(HWACHA_CAUSE_TVEC_ILLEGAL_REGID, uint64_t(insn.bits()));
  h->get_ct_state()->APR.write(dst, value);
}

#define READ_APR(src) read_apr(h, insn, src)
#define WRITE_APR(dst, value) write_apr(h, insn, dst, value)
#define ARS1 (READ_APR(INSN_VARS1))
#define ARS2 (READ_APR(INSN_VARS2))
#define WRITE_SVARD(value) (WRITE_APR(INSN_SVARD, value))

//Work Thread Management
//predicate registers
#define INSN_VPRS1 (insn.vprs1())
#define INSN_VPRS2 (insn.vprs2())
#define INSN_VPRS3 (insn.vprs3())
#define INSN_VPRD  (insn.vprd())
#define INSN_VPRED (insn.vpred())
#define INSN_VN    (insn.vn())
#define INSN_COND  (insn.vcond())

static inline reg_t read_ppr(hwacha_t* h, insn_t insn, uint32_t idx, size_t src)
{
  if (src >= h->get_ct_state()->nppr)
    h->take_exception(HWACHA_CAUSE_TVEC_ILLEGAL_REGID, uint64_t(insn.bits()));
  return (h->get_ut_state(idx)->PPR[src]);
}

#define UT_READ_PPR(idx, src) read_ppr(h, insn, idx, src)
#define UT_PRS1(idx) (UT_READ_PPR(idx, INSN_VPRS1))
#define UT_PRS2(idx) (UT_READ_PPR(idx, INSN_VPRS2))
#define UT_PRS3(idx) (UT_READ_PPR(idx, INSN_VPRS3))

static inline bool cond_all(hwacha_t* h, insn_t insn, size_t pred){
  for(uint32_t i = 0; i<VL; i++){
    if (!(UT_READ_PPR(i, pred) ^ INSN_VN))
      return false;
  }
  return true;
}

static inline bool cond_any(hwacha_t* h, insn_t insn, size_t pred){
  for(uint32_t i = 0; i<VL; i++){
    if (UT_READ_PPR(i, pred) ^ INSN_VN)
      return true;
  }
  return false;
}

static inline bool cond(hwacha_t* h, insn_t insn, size_t pred, uint32_t c){
  if(c == 0)//all
    return cond_all(h,insn,pred);
  if(c == 1)//any
    return cond_any(h,insn,pred);
  h->take_exception(HWACHA_CAUSE_VF_ILLEGAL_INSTRUCTION, uint64_t(insn.bits()));
  return false;
}


#define VPRED (UT_READ_PPR(UTIDX,INSN_VPRED) ^ INSN_VN)
#define COND  cond(h, insn, INSN_VPRED, INSN_COND)

static inline void write_ppr(hwacha_t* h, insn_t insn, uint32_t idx, size_t dst, bool value, bool pred)
{
  if (dst >= h->get_ct_state()->nppr)
    h->take_exception(HWACHA_CAUSE_TVEC_ILLEGAL_REGID, uint64_t(insn.bits()));
  if(pred || VPRED)
    h->get_ut_state(idx)->PPR.write(dst, value);
}

#define WRITE_PPR_NO_PRED(idx,dst,value) write_ppr(h, insn, idx, dst, value, 1)
#define UT_WRITE_PPR(idx, dst, value) write_ppr(h, insn, idx, dst, value, 0)
#define UT_WRITE_PRD(idx, value) (UT_WRITE_PPR(idx, INSN_VRD, value))

//General Registers 
#define INSN_VRS1 (insn.vrs1())
#define INSN_VRS2 (insn.vrs2())
#define INSN_VRS3 (insn.vrs3())
#define INSN_VRD (insn.vrd())
#define INSN_VSEG (insn.vseg()+1)

static inline reg_t read_xpr(hwacha_t* h, insn_t insn, uint32_t idx, size_t src)
{
  if (src >= h->get_ct_state()->nxpr)
    h->take_exception(HWACHA_CAUSE_TVEC_ILLEGAL_REGID, uint64_t(insn.bits()));
  return (h->get_ut_state(idx)->XPR[src]);
}

static inline void write_xpr(hwacha_t* h, insn_t insn, uint32_t idx, size_t dst, reg_t value)
{
  if (dst >= h->get_ct_state()->nxpr)
    h->take_exception(HWACHA_CAUSE_TVEC_ILLEGAL_REGID, uint64_t(insn.bits()));
  if(VPRED){
    h->get_ut_state(idx)->XPR.write(dst, value);
  }
}

#define UT_READ_XPR(idx, src) read_xpr(h, insn, idx, src)
#define UT_WRITE_XPR(idx, dst, value) write_xpr(h, insn, idx, dst, value)
#define UT_VRS1(idx) (UT_READ_XPR(idx, INSN_VRS1))
#define UT_VRS2(idx) (UT_READ_XPR(idx, INSN_VRS2))
#define UT_VRS3(idx) (UT_READ_XPR(idx, INSN_VRS3))
#define UT_WRITE_VRD(idx, value) (UT_WRITE_XPR(idx, INSN_VRD, value))


#define require_supervisor_hwacha \
  if (get_field(p->get_state()->mstatus, MSTATUS_PRV) < PRV_S) \
    h->take_exception(HWACHA_CAUSE_PRIVILEGED_INSTRUCTION, uint32_t(insn.bits()));

#endif
