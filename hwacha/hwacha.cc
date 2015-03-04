#include "hwacha.h"
#include "hwacha_xcpt.h"
#include "mmu.h"
#include "trap.h"
#include <stdexcept>

REGISTER_EXTENSION(hwacha, []() { return new hwacha_t; })

void ct_state_t::reset()
{
  nxpr = 128;
  nppr = 8;
  maxvl = 32;
  vl = 0;
  count = 0;
  prec = 64;

  enable = false;

  vf_pc = -1;

  SPR.reset();
  APR.reset();
}

void ut_state_t::reset()
{
  run = false;
  XPR.reset();
  PPR.reset();
}

void hwacha_t::reset()
{
  ct_state.reset();
  for (int i=0; i<max_uts; i++)
    ut_state[i].reset();
}

static reg_t custom(processor_t* p, insn_t insn, reg_t pc)
{
  require_accelerator;
  hwacha_t* h = static_cast<hwacha_t*>(p->get_extension());
  bool matched = false;
  reg_t npc = -1;

  try
  {
    #define DECLARE_INSN(name, match, mask) \
      extern reg_t hwacha_##name(processor_t*, insn_t, reg_t); \
      if ((insn.bits() & mask) == match) { \
        npc = hwacha_##name(p, insn, pc); \
        matched = true; \
      }
    #include "opcodes_hwacha.h"
    #undef DECLARE_INSN
  }
  catch (trap_instruction_access_fault& t)
  {
    h->take_exception(HWACHA_CAUSE_VF_FAULT_FETCH, h->get_ct_state()->vf_pc);
  }
  catch (trap_illegal_instruction& t)
  {
    h->take_exception(HWACHA_CAUSE_VF_ILLEGAL_INSTRUCTION, h->get_ct_state()->vf_pc);
  }
  catch (trap_load_address_misaligned& t)
  {
    h->take_exception(HWACHA_CAUSE_MISALIGNED_LOAD, t.get_badvaddr());
  }
  catch (trap_store_address_misaligned& t)
  {
    h->take_exception(HWACHA_CAUSE_MISALIGNED_STORE, t.get_badvaddr());
  }
  catch (trap_load_access_fault& t)
  {
    h->take_exception(HWACHA_CAUSE_FAULT_LOAD, t.get_badvaddr());
  }
  catch (trap_store_access_fault& t)
  {
    h->take_exception(HWACHA_CAUSE_FAULT_STORE, t.get_badvaddr());
  }

  if (!matched)
    h->take_exception(HWACHA_CAUSE_ILLEGAL_INSTRUCTION, uint32_t(insn.bits()));

  return npc;
}

std::vector<insn_desc_t> hwacha_t::get_instructions()
{
  std::vector<insn_desc_t> insns;
  insns.push_back((insn_desc_t){0x0b, 0x7f, &::illegal_instruction, custom});
  insns.push_back((insn_desc_t){0x2b, 0x7f, &::illegal_instruction, custom});
  insns.push_back((insn_desc_t){0x5b, 0x7f, &::illegal_instruction, custom});
  insns.push_back((insn_desc_t){0x7b, 0x7f, &::illegal_instruction, custom});
  return insns;
}

bool hwacha_t::vf_active()
{
  for (uint32_t i=0; i<get_ct_state()->vl; i++) {
    if (get_ut_state(i)->run)
      return true;
  }
  return false;
}

void hwacha_t::take_exception(reg_t c, reg_t a)
{
  cause = c;
  aux = a;
  printf("aux:%016" PRIx64 ":",aux);
if(cause == HWACHA_CAUSE_ILLEGAL_CFG)
  printf("ILL_CFG\n");
if(cause == HWACHA_CAUSE_ILLEGAL_INSTRUCTION)
  printf("ILL_INST\n");
if(cause == HWACHA_CAUSE_PRIVILEGED_INSTRUCTION)
  printf("PRIV_INST\n");
if(cause == HWACHA_CAUSE_TVEC_ILLEGAL_REGID)
  printf("ILL_REGID\n");
if(cause == HWACHA_CAUSE_VF_MISALIGNED_FETCH)
  printf("MIS_FETCh\n");
if(cause == HWACHA_CAUSE_VF_FAULT_FETCH)
  printf("FAULT_FETCH\n");
if(cause == HWACHA_CAUSE_VF_ILLEGAL_INSTRUCTION)
  printf("VF_ILL_INST\n");
if(cause == HWACHA_CAUSE_VF_ILLEGAL_REGID)
  printf("VF_ILL_REGID\n");
if(cause == HWACHA_CAUSE_MISALIGNED_LOAD)
  printf("MIS_LOAD\n");
if(cause == HWACHA_CAUSE_MISALIGNED_STORE)
  printf("MIS_STORE\n");
if(cause == HWACHA_CAUSE_FAULT_LOAD)
  printf("FAULT_LOAD\n");
if(cause == HWACHA_CAUSE_FAULT_STORE)
  printf("FAULT_STORE\n");
  

  raise_interrupt();
  if (!(p->get_state()->sr & SR_EI))
    throw std::logic_error("hwacha exception posted, but SR_EI bit not set!");
  throw std::logic_error("hwacha exception posted, but IM[COP] bit not set!");
}
