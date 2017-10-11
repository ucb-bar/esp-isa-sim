#ifndef _HWACHA_H
#define _HWACHA_H

#include "extension.h"
#include <random>
#include <limits>

static const uint32_t MAX_XPR = 256;
static const uint32_t MAX_PPR = 16;
static const uint32_t MAX_SPR = 256;
static const uint32_t MAX_APR = 32;

static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_int_distribution<reg_t> reg_dis(std::numeric_limits<reg_t>::min(),
    std::numeric_limits<reg_t>::max());
static std::uniform_int_distribution<int> bool_dis(0,1);

class hwacha_insn_t: public insn_t
{
public:
  hwacha_insn_t(insn_t insn) : b(insn.bits()) {}
  insn_bits_t bits() { return b; }
  int64_t v_imm() { return int64_t(b) >> 32; }
  int64_t vc_imm() { return int64_t(b) >> 35 << 3; }
  uint64_t svsrd() { return (x(20,3)<<5) | (x(7, 5)); }
  uint64_t svard() { return rd(); }
  uint64_t vrd() { return x(16, 8); }
  uint64_t vrs1() { return x(24, 8); }
  uint64_t vrs2() { return x(33, 8); }
  uint64_t vrs3() { return x(41, 8); }
  uint64_t vprd() { return x(16, 4); }
  uint64_t vprs1() { return x(24, 4); }
  uint64_t vprs2() { return x(33, 4); }
  uint64_t vprs3() { return x(41, 4); }
  uint64_t vars1() { return x(24, 5); }
  uint64_t vars2() { return x(33, 5); }
  uint64_t vseg() { return x(45, 3); }
  uint64_t vpred() { return x(12, 4); }
  uint64_t vn() { return x(32, 1); }
  uint64_t vcond() { return x(33, 2); }
  uint64_t vd() { return x(63, 1); }
  uint64_t vs1() { return x(62, 1); }
  uint64_t vs2() { return x(61, 1); }
  uint64_t vs3() { return x(60, 1); }
  uint64_t vpop_table() { return x(50, 8); }
  uint64_t vshamt() { return x(32, 6); }
  uint64_t vrm() { return x(50, 3); }
  uint64_t vopc() { return x(7, 5); }
  bool v_is_scalar(){
    bool scalar_opc = 
      vopc() == 0x04 || //scalar imm arith
      vopc() == 0x05 || //auipc
      vopc() == 0x06 || //scalar-32 imm arith
      vopc() == 0x0D || //lui
      vopc() == 0x18 || //fence/stop
      vopc() == 0x19 || //jalr
      vopc() == 0x1B;   //jal

    //includes scalar loads and reg-reg ops with scalar dest
    bool dyn_scalar_op =
      !scalar_opc && vd() == 0;

    return scalar_opc || dyn_scalar_op;
  }
private:
  insn_bits_t b;
  uint64_t x(int lo, int len) { return (b >> lo) & ((insn_bits_t(1) << len)-1); }
  uint64_t xs(int lo, int len) { return int64_t(b) << (64-lo-len) >> (64-len); }
  uint64_t imm_sign() { return xs(63, 1); }
};

class hwacha_disasm_insn_t : public disasm_insn_t
{
public:
  hwacha_disasm_insn_t(const char* name, uint64_t match, uint64_t mask,
                const std::vector<const arg_t*>& args)
    : disasm_insn_t(name,match,mask,args) {}

  bool operator == (hwacha_insn_t insn) const
  {
    return (insn.bits() & get_mask()) == get_match();
  }
};

class hwacha_disassembler_t
{
public:
  hwacha_disassembler_t() {}
  ~hwacha_disassembler_t() {};

  std::string disassemble(hwacha_insn_t insn) const
  {
    const hwacha_disasm_insn_t* disasm_insn = lookup(insn);
    return disasm_insn ? disasm_insn->to_string(insn) : "unknown";
  }

  void add_insn(hwacha_disasm_insn_t* insn, bool priority = false)
  {
    size_t idx = HASH_SIZE;
    if (insn->get_mask() % HASH_SIZE == HASH_SIZE - 1)
      idx = insn->get_match() % HASH_SIZE;
    if(priority)
      chain[idx].insert(chain[idx].begin(), insn);
    else
      chain[idx].push_back(insn);
  }
 private:
  static const int HASH_SIZE = 256;
  std::vector<const hwacha_disasm_insn_t*> chain[HASH_SIZE+1];

  const hwacha_disasm_insn_t* lookup(hwacha_insn_t insn) const
  {
    size_t idx = insn.bits() % HASH_SIZE;
    for (size_t j = 0; j < chain[idx].size(); j++)
      if(*chain[idx][j] == insn)
        return chain[idx][j];
  
    idx = HASH_SIZE;
    for (size_t j = 0; j < chain[idx].size(); j++)
      if(*chain[idx][j] == insn)
        return chain[idx][j];
  
    return NULL;
  }
};


struct ct_state_t
{
  void reset();

  uint32_t nxpr;
  uint32_t nppr;
  uint32_t maxvl;
  uint32_t vl;
  uint32_t count;
  uint32_t prec;

  uint32_t fflags;
  uint32_t frm;

  bool enable;

  reg_t vf_pc;
  regfile_t<reg_t, MAX_SPR, true> SPR;
  regfile_t<reg_t, MAX_APR, false> APR;
};

struct ut_state_t
{
  void reset();

  bool run;
  regfile_t<reg_t, MAX_XPR, false> XPR;
  regfile_t<bool, MAX_PPR, false> PPR;
};

class hwacha_t : public extension_t
{
public:
  hwacha_t() : cause(0), aux(0), debug(false) {
    ut_disassembler = new hwacha_disassembler_t();
  }
  std::vector<insn_desc_t> get_instructions();
  std::vector<disasm_insn_t*> get_disasms();
  const char* name() { return "hwacha"; }
  void reset();
  void set_debug(bool value) { debug = value; }
  void set_processor(processor_t* _p) {
    if(_p->get_max_xlen() != 64) throw std::logic_error("hwacha requires rv64");
    p = _p;
  }

  ct_state_t* get_ct_state() { return &ct_state; }
  ut_state_t* get_ut_state(int idx) { return &ut_state[idx]; }
  bool vf_active();
  reg_t get_cause() { return cause; }
  reg_t get_aux() { return aux; }
  void take_exception(reg_t, reg_t);
  void clear_exception() { clear_interrupt(); }

  bool get_debug() { return debug; }
  hwacha_disassembler_t* get_ut_disassembler() { return ut_disassembler; }

  static const int max_uts = 2048;

private:
  ct_state_t ct_state;
  ut_state_t ut_state[max_uts];
  reg_t cause;
  reg_t aux;

  hwacha_disassembler_t* ut_disassembler;
  bool debug;
};

#endif
