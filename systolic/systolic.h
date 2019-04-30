#ifndef _SYSTOLIC_H
#define _SYSTOLIC_H

#include "extension.h"
#include "rocc.h"
#include <random>
#include <limits>

typedef int16_t input_t; // Systolic array input datatype (feeding into PEs, moving out of accumulator)
typedef int16_t output_t; // Systolic array output datatype (coming down from PEs, moving into accumulator)
typedef int32_t accum_t; // Accumulator datatype (inside PEs for OS dataflow and for the external accumulator)
static const uint32_t dim = 4; // Square dimension of systolic array
static const uint32_t sp_matrices = 256; // Size the scratchpad to fit sp_matrices matrices
static const uint32_t row_bytes = dim * sizeof(input_t); // Bytes fed into the systolic array along one axis in one cycle
static const uint32_t accum_rows = 32; // Number of systolic array rows in the accumulator

struct systolic_state_t
{
  void reset();

  reg_t output_sp_addr;
  reg_t preload_sp_addr;
  reg_t mode;
  reg_t relu;
  reg_t shift;
  reg_t load_stride;
  reg_t store_stride;

  bool enable;
  std::vector<std::vector<input_t>> *spad; // Scratchpad constructed as systolic array rows
  std::vector<std::vector<accum_t>> *pe_state; // Stores the PE's internal accumulator state
  std::vector<std::vector<accum_t>> *accumulator;
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
  void setmode(reg_t relu, reg_t mode, reg_t shift);
  void set_load_stride(reg_t stride);
  void set_store_stride(reg_t stride);
  void compute(reg_t a_addr, reg_t b_addr, bool preload);

  accum_t get_matrix_element(reg_t base_sp_addr, size_t i, size_t j);
  void store_matrix_element(reg_t base_sp_addr, size_t i, size_t j, accum_t value);

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
  const unsigned mode_subfunct = 1;
  const unsigned setmode_subconfig = 1;
  const unsigned load_subconfig = 1;
  const unsigned store_subconfig = 2;

  bool debug;
};

#endif
