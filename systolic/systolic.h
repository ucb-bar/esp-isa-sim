#ifndef _SYSTOLIC_H
#define _SYSTOLIC_H

#include "extension.h"
#include "rocc.h"
#include <random>
#include <limits>

typedef int8_t input_t; // Systolic array input datatype (feeding into PEs, moving out of accumulator)
typedef int16_t output_t; // Systolic array output datatype (coming down from PEs, moving into accumulator)
typedef int32_t accum_t; // Accumulator datatype (inside PEs for OS dataflow and for the external accumulator)
static const uint32_t dim = 16; // Square dimension of systolic array
static const uint32_t sp_matrices = 10*1024; // Size the scratchpad to fit sp_matrices matrices
static const uint32_t accum_rows = 128; // Number of systolic array rows in the accumulator

struct systolic_state_t
{
  enum Dataflow {OS, WS};
  enum Activation {NONE, RELU, RELU6};
  void reset();

  // 32-bit systolic address space
  uint32_t output_sp_addr;
  uint32_t preload_sp_addr;
  Dataflow mode;
  Activation act;
  reg_t shift;
  reg_t load_stride;
  reg_t store_stride;

  bool enable;
  std::vector<std::vector<input_t>> *spad; // Scratchpad constructed as systolic array rows
  std::vector<std::vector<accum_t>> *pe_state; // Stores each PE's internal accumulator state
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
  void preload(reg_t bd_addr, reg_t c_addr);
  void setmode(reg_t rs1, reg_t rs2);
  void compute(reg_t a_addr, reg_t bd_addr, bool preload);

private:
  systolic_state_t systolic_state;
  reg_t cause;
  reg_t aux;

  const unsigned setmode_funct = 0;
  const unsigned mvin_funct = 2;
  const unsigned mvout_funct = 3;
  const unsigned compute_preloaded_funct = 4;
  const unsigned compute_accumulated_funct = 5;
  const unsigned preload_funct = 6;

  bool debug;
  output_t out_rounding_saturating_shift(accum_t value, uint64_t shift);
  input_t in_rounding_saturating_shift(accum_t value, uint64_t shift);
  input_t apply_activation(input_t value);
  template <class T>
  T read_from_dram(reg_t addr);

  template <class T>
  void write_to_dram(reg_t addr, T data);
};

#endif
