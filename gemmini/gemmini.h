#ifndef _GEMMINI_H
#define _GEMMINI_H

#include "extension.h"
#include "rocc.h"
#include <random>
#include <limits>
#include "gemmini_params.h"

typedef acc_t output_t; // Systolic array output datatype (coming down from PEs, moving into accumulator)
static const uint32_t sp_matrices = (BANK_NUM * BANK_ROWS) / DIM; // Size the scratchpad to fit sp_matrices matrices
static const uint32_t accum_rows = ACC_ROWS; // Number of systolic array rows in the accumulator
static const uint64_t addr_len = ADDR_LEN; // Number of bits used to address the scratchpad/accumulator

#ifdef RISCV_ENABLE_GEMMINI_COMMITLOG
#define dprintf(...) printf(__VA_ARGS__)
#else
#define dprintf(...)
#endif

struct gemmini_state_t
{
  enum Dataflow {OS, WS};
  enum Activation {NONE, RELU, RELU6};
  void reset();

  // 32-bit gemmini address space
  uint32_t output_sp_addr;
  uint32_t preload_sp_addr;
  uint16_t preload_cols, preload_rows;
  uint16_t output_cols, output_rows;
  Dataflow mode;
  Activation act;
  reg_t acc_shift, sys_shift, relu6_shift;
  reg_t load_stride;
  reg_t store_stride;
#ifdef HAS_MVIN_SCALE
  scale_t load_scale;
#endif
  uint16_t a_stride;
  uint8_t pool_stride;
  uint8_t pool_size;
  uint8_t pool_out_dim;
  uint8_t pool_porows;
  uint8_t pool_pocols;
  uint8_t pool_orows;
  uint8_t pool_ocols;
  uint8_t pool_lpad;
  uint8_t pool_upad;

  bool enable;
  std::vector<std::vector<elem_t>> *spad; // Scratchpad constructed as systolic array rows
  std::vector<std::vector<acc_t>> *pe_state; // Stores each PE's internal accumulator state
  std::vector<std::vector<acc_t>> *accumulator;
};

class gemmini_t : public rocc_t
{
public:
  gemmini_t() : cause(0), aux(0), debug(false) {}
  const char* name() { return "gemmini"; }
  reg_t custom3(rocc_insn_t insn, reg_t xs1, reg_t xs2);
  void reset();

  void mvin(reg_t dram_addr, reg_t sp_addr);
  void mvout(reg_t dram_addr, reg_t sp_addr);
  void preload(reg_t bd_addr, reg_t c_addr);
  void setmode(reg_t rs1, reg_t rs2);
  void compute(reg_t a_addr, reg_t bd_addr, bool preload);

private:
  gemmini_state_t gemmini_state;
  reg_t cause;
  reg_t aux;

  const unsigned setmode_funct = 0;
  const unsigned mvin_funct = 2;
  const unsigned mvout_funct = 3;
  const unsigned compute_preloaded_funct = 4;
  const unsigned compute_accumulated_funct = 5;
  const unsigned preload_funct = 6;
  const unsigned flush_funct = 7;

  bool debug;
  elem_t apply_activation(elem_t value);

  template <class T>
  T rounding_saturating_shift(acc_t value, uint64_t shift);

  template <class T>
  T read_from_dram(reg_t addr);

  template <class T>
  void write_to_dram(reg_t addr, T data);

#ifdef ELEM_T_IS_FLOAT
  elem_t elem_t_bits_to_elem_t(elem_t_bits x);
  elem_t_bits elem_t_to_elem_t_bits(elem_t x);
  acc_t acc_t_bits_to_acc_t(acc_t_bits x);
  acc_t_bits acc_t_to_acc_t_bits(acc_t x);
#endif

#if defined(HAS_MVIN_SCALE) || defined(HAS_MVIN_ACC_SCALE)
  scale_t_bits scale_t_to_scale_t_bits(scale_t scale);
  scale_t scale_t_bits_to_scale_t(scale_t_bits bits);
#endif
};

#endif
