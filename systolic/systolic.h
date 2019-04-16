#ifndef _SYSTOLIC_H
#define _SYSTOLIC_H

#include "extension.h"
#include "rocc.h"
#include <random>
#include <limits>

typedef int32_t pe_datatype;
static const uint32_t data_width = 16;
static const uint32_t dim = 4;
static const uint32_t sp_banks = 14;
static const uint32_t sp_bank_entries = dim;
static const uint32_t row_bytes = dim * (data_width / 8);

struct systolic_state_t
{
  void reset();

  reg_t output_sp_addr;
  reg_t preload_sp_addr;
  reg_t mode;
  reg_t shift;

  bool enable;
  std::vector<uint8_t> *spad;
  std::vector<std::vector<pe_datatype>> *pe_state;
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
  void setmode(reg_t mode, reg_t shift);
  void compute(reg_t a_addr, reg_t b_addr, bool preload);

  pe_datatype get_matrix_element(reg_t base_sp_addr, size_t i, size_t j);
  void store_matrix_element(reg_t base_sp_addr, size_t i, size_t j, pe_datatype value);

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
