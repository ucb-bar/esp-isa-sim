#ifndef _SYSTOLIC_H
#define _SYSTOLIC_H

#include "extension.h"
#include "rocc.h"
#include <random>
#include <limits>

static const uint32_t MAX_SCRATCHPAD_SIZE = 256;
static const uint32_t ARRAY_X_DIM = 16;
static const uint32_t ARRAY_Y_DIM = 16;

//static std::random_device rd;
//static std::mt19937 gen(rd());
//static std::uniform_int_distribution<reg_t> reg_dis(std::numeric_limits<reg_t>::min(),
//    std::numeric_limits<reg_t>::max());
//static std::uniform_int_distribution<int> bool_dis(0,1);

struct systolic_state_t
{
  void reset();

  uint8_t SCRATCHPAD[10*ARRAY_X_DIM*ARRAY_Y_DIM];
  int32_t PE_array_state[ARRAY_X_DIM][ARRAY_Y_DIM];

  uint32_t output_sp_addr;
  uint32_t preload_sp_addr;
  uint32_t dataflow_mode;
  uint32_t fflags;

  bool enable;
};


class systolic_t : public rocc_t
{
public:
  systolic_t() : cause(0), aux(0), debug(false) {}
  const char* name() { return "systolic"; }
  reg_t custom3(rocc_insn_t insn, reg_t xs1, reg_t xs2);
  void reset();
  //void set_debug(bool value) { debug = value; }
  //void set_processor(processor_t* _p) {
  //  if(_p->get_max_xlen() != 64) throw std::logic_error("systolic requires rv64");
  //  p = _p;
  //}

  systolic_state_t* get_sytolic_state() { return &systolic_state; }
  //reg_t get_cause() { return cause; }
  //reg_t get_aux() { return aux; }
  //void take_exception(reg_t, reg_t);
  //void clear_exception() { clear_interrupt(); }

  //bool get_debug() { return debug; }

private:
  systolic_state_t systolic_state;
  reg_t cause;
  reg_t aux;

  bool debug;
};

#endif
