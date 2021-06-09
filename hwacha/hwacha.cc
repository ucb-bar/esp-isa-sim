#include "hwacha.h"
#include "hwacha_xcpt.h"
#include "mmu.h"
#include "trap.h"
#include <stdexcept>

REGISTER_EXTENSION(hwacha, []() { return new hwacha_t; })

void ct_state_t::reset()
{
  nxpr = 256;
  nppr = 1;
  maxvl = 8;
  vl = 0;
  count = 0;
  prec = 64;
  
  //inherited from control processor
  uint32_t fflags; //accumulated and or'd into control threads on reading
  uint32_t frm; //only readable by work-thread

  enable = true;

  vf_pc = -1;

  //randomize non-zero registers
  for (size_t i = 1; i < MAX_SPR; i++) {
    SPR.write(i, reg_dis(gen));
  }
  for (size_t i = 0; i < MAX_APR; i++) {
    APR.write(i, reg_dis(gen));
  }
}

void ut_state_t::reset()
{
  //randomize non-zero registers
  for (size_t i = 0; i < MAX_XPR; i++) {
    XPR.write(i, reg_dis(gen));
  }
  for (size_t i = 0; i < MAX_PPR; i++) {
    PPR.write(i, bool_dis(gen));
  }
  run = false;
}

void hwacha_t::reset()
{
  ct_state.reset();
  for (size_t i=0; i<max_uts; i++)
    ut_state[i].reset();
}

static reg_t custom(processor_t* p, insn_t insn, reg_t pc)
{
  require_accelerator;
  hwacha_t* h = static_cast<hwacha_t*>(p->get_extension("hwacha"));
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
#ifdef RISCV_ENABLE_HCOMMITLOG
    fprintf(stderr,"H: INST ACCESS FAULT\n");
#endif
    h->take_exception(HWACHA_CAUSE_VF_FAULT_FETCH, h->get_ct_state()->vf_pc);
  }
  catch (trap_illegal_instruction& t)
  {
#ifdef RISCV_ENABLE_HCOMMITLOG
    fprintf(stderr,"H: ILL INST\n");
#endif
    h->take_exception(HWACHA_CAUSE_VF_ILLEGAL_INSTRUCTION, h->get_ct_state()->vf_pc);
  }
  catch (trap_load_address_misaligned& t)
  {
#ifdef RISCV_ENABLE_HCOMMITLOG
    fprintf(stderr,"H: LOAD MISALIGNED\n");
#endif
    h->take_exception(HWACHA_CAUSE_MISALIGNED_LOAD, t.get_tval());
  }
  catch (trap_store_address_misaligned& t)
  {
#ifdef RISCV_ENABLE_HCOMMITLOG
    fprintf(stderr,"H: STORE MISALIGNED\n");
#endif
    h->take_exception(HWACHA_CAUSE_MISALIGNED_STORE, t.get_tval());
  }
  catch (trap_load_access_fault& t)
  {
#ifdef RISCV_ENABLE_HCOMMITLOG
    fprintf(stderr,"H: LOAD ACCESS FAULT\n");
#endif
    h->take_exception(HWACHA_CAUSE_FAULT_LOAD, t.get_tval());
  }
  catch (trap_store_access_fault& t)
  {
#ifdef RISCV_ENABLE_HCOMMITLOG
    fprintf(stderr,"H: STORE ACCESS FAULT\n");
#endif
    h->take_exception(HWACHA_CAUSE_FAULT_STORE, t.get_tval());
  }

  if (!matched) {
#ifdef RISCV_ENABLE_HCOMMITLOG
    fprintf(stderr,"H: VERY ILL INST\n");
#endif
    h->take_exception(HWACHA_CAUSE_ILLEGAL_INSTRUCTION, uint32_t(insn.bits()));
  }

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
#ifdef RISCV_ENABLE_HCOMMITLOG
  fprintf(stderr,"H: EXCPT cause:%d aux %d \n", c, a);
#endif
  cause = c;
  aux = a;
  raise_interrupt();
  throw std::logic_error("unreachable!");
}
