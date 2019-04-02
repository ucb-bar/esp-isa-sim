#ifndef _SYSTOLIC_H
#define _SYSTOLIC_H

#include "extension.h"
#include "rocc.h"
#include <random>
#include <limits>

struct systolic_state_t
{
  void reset(uint32_t data_width, uint32_t dim, uint32_t sp_banks, uint32_t sp_bank_entries);

  reg_t output_sp_addr;
  reg_t preload_sp_addr;
  reg_t dataflow_mode;

  bool enable;
  std::vector<uint8_t> *spad;
  std::vector<std::vector<int32_t>> *pe_state;
};

class systolic_t : public rocc_t
{
public:
  systolic_t() : cause(0), aux(0), debug(false) {}
  const char* name() { return "systolic"; }
  reg_t custom3(rocc_insn_t insn, reg_t xs1, reg_t xs2);
  void reset();

  void mvin(reg_t dram_addr, reg_t sp_addr);
  void mvout(reg_t dram_addr, reg_t sp_addr);
  void preload(reg_t d_addr, reg_t c_addr);
  void setmode(reg_t mode);
  void compute(reg_t a_addr, reg_t b_addr, bool preload);

  uint32_t row_bytes() { return dim * (data_width / 8); }
  uint32_t data_width;
  uint32_t dim;
  uint32_t sp_banks;
  uint32_t sp_bank_entries;

private:
  systolic_state_t systolic_state;
  reg_t cause;
  reg_t aux;

  const unsigned mvin_funct = 2;
  const unsigned mvout_funct = 3;
  const unsigned compute_preloaded_funct = 4;
  const unsigned compute_accumulated_funct = 5;
  const unsigned preload_funct = 6;
  const unsigned setmode_funct = 0;

  bool debug;
};

#endif
