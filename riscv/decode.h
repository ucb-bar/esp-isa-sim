// See LICENSE for license details.

#ifndef _RISCV_DECODE_H
#define _RISCV_DECODE_H

#if (-1 != ~0) || ((-1 >> 1) != -1)
# error spike requires a two''s-complement c++ implementation
#endif

#include <cstdint>
#include <string.h>
#include "encoding.h"
#include "config.h"
#include "common.h"
#include <cinttypes>

typedef int64_t sreg_t;
typedef uint64_t reg_t;
typedef uint64_t freg_t;

const int NXPR = 32;
const int NFPR = 32;

#define FP_RD_NE  0
#define FP_RD_0   1
#define FP_RD_DN  2
#define FP_RD_UP  3
#define FP_RD_NMM 4

#define FSR_RD_SHIFT 5
#define FSR_RD   (0x7 << FSR_RD_SHIFT)

#define FPEXC_NX 0x01
#define FPEXC_UF 0x02
#define FPEXC_OF 0x04
#define FPEXC_DZ 0x08
#define FPEXC_NV 0x10

#define FSR_AEXC_SHIFT 0
#define FSR_NVA  (FPEXC_NV << FSR_AEXC_SHIFT)
#define FSR_OFA  (FPEXC_OF << FSR_AEXC_SHIFT)
#define FSR_UFA  (FPEXC_UF << FSR_AEXC_SHIFT)
#define FSR_DZA  (FPEXC_DZ << FSR_AEXC_SHIFT)
#define FSR_NXA  (FPEXC_NX << FSR_AEXC_SHIFT)
#define FSR_AEXC (FSR_NVA | FSR_OFA | FSR_UFA | FSR_DZA | FSR_NXA)

typedef uint64_t insn_bits_t;
class insn_t
{
public:
  insn_t() = default;
  insn_t(insn_bits_t bits) : b(bits) {}
  insn_bits_t bits() { return b; }
  int64_t i_imm() { return int64_t(b) >> 20; }
  int64_t s_imm() { return x(7, 5) + (xs(25, 7) << 5); }

  int64_t sb_imm() { return (x(8, 4) << 1) + (x(25,6) << 5) + (x(7,1) << 11) + (imm_sign() << 12); }
  int64_t u_imm() { return int64_t(b) >> 12 << 12; }
  int64_t uj_imm() { return (x(21, 10) << 1) + (x(20, 1) << 11) + (x(12, 8) << 12) + (imm_sign() << 20); }
  uint64_t rd() { return x(7, 5); }
  uint64_t rs1() { return x(15, 5); }
  uint64_t rs2() { return x(20, 5); }
  uint64_t rs3() { return x(27, 5); }
  int64_t v_imm() { return int64_t(b) >> 32; }
  int64_t vc_imm() { return int64_t(b) >> 35 << 3; }
  uint64_t svsrd() { return (x(22,20)<<5) | (x(11, 7)); }
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
  uint64_t vseg() { return x(50, 3); }
  uint64_t vpred() { return x(12, 4); }
  uint64_t vn() { return x(32, 1); }
  uint64_t vcond() { return x(33, 2); }
  uint64_t vd() { return x(63, 1); }
  uint64_t vs1() { return x(62, 1); }
  uint64_t vs2() { return x(61, 1); }
  uint64_t vs3() { return x(60, 1); }
  uint64_t vpop_table() { return x(50, 8); }
  uint64_t vshamt() { return x(32, 6); }
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
  uint64_t rm() { return x(12, 3); }
  uint64_t csr() { return x(20, 12); }
private:
  insn_bits_t b;
  uint64_t x(int lo, int len) { return (b >> lo) & ((insn_bits_t(1) << len)-1); }
  uint64_t xs(int lo, int len) { return int64_t(b) << (64-lo-len) >> (64-len); }
  uint64_t imm_sign() { return xs(63, 1); }
};

template <class T, size_t N, bool zero_reg>
class regfile_t
{
public:
  void reset()
  {
    memset(data, 0, sizeof(data));
  }
  void write(size_t i, T value)
  {
    if (!zero_reg || i != 0)
      data[i] = value;
  }
  const T& operator [] (size_t i) const
  {
    return data[i];
  }
private:
  T data[N];
};

// helpful macros, etc
#define MMU (*p->get_mmu())
#define STATE (*p->get_state())
#define RS1 STATE.XPR[insn.rs1()]
#define RS2 STATE.XPR[insn.rs2()]
#define WRITE_RD(value) STATE.XPR.write(insn.rd(), value)

#ifdef RISCV_ENABLE_COMMITLOG
  #undef WRITE_RD 
  #define WRITE_RD(value) ({ \
        reg_t wdata = value; /* value is a func with side-effects */ \
        STATE.log_reg_write = (commit_log_reg_t){insn.rd() << 1, wdata}; \
        STATE.XPR.write(insn.rd(), wdata); \
      })
#endif

#define FRS1 STATE.FPR[insn.rs1()]
#define FRS2 STATE.FPR[insn.rs2()]
#define FRS3 STATE.FPR[insn.rs3()]
#define WRITE_FRD(value) STATE.FPR.write(insn.rd(), value)
 
#ifdef RISCV_ENABLE_COMMITLOG
  #undef WRITE_FRD 
  #define WRITE_FRD(value) ({ \
        freg_t wdata = value; /* value is a func with side-effects */ \
        STATE.log_reg_write = (commit_log_reg_t){(insn.rd() << 1) | 1, wdata}; \
        STATE.FPR.write(insn.rd(), wdata); \
      })
#endif
 
#define SHAMT (insn.i_imm() & 0x3F)
#define BRANCH_TARGET (pc + insn.sb_imm())
#define JUMP_TARGET (pc + insn.uj_imm())
#define RM ({ int rm = insn.rm(); \
              if(rm == 7) rm = STATE.frm; \
              if(rm > 4) throw trap_illegal_instruction(); \
              rm; })

#define xpr64 (xprlen == 64)

#define require_supervisor if(unlikely(!(STATE.sr & SR_S))) throw trap_privileged_instruction()
#define require_xpr64 if(unlikely(!xpr64)) throw trap_illegal_instruction()
#define require_xpr32 if(unlikely(xpr64)) throw trap_illegal_instruction()
#ifndef RISCV_ENABLE_FPU
# define require_fp throw trap_illegal_instruction()
#else
# define require_fp if(unlikely(!(STATE.sr & SR_EF))) throw trap_fp_disabled()
#endif
#define require_accelerator if(unlikely(!(STATE.sr & SR_EA))) throw trap_accelerator_disabled()

#define cmp_trunc(reg) (reg_t(reg) << (64-xprlen))
#define set_fp_exceptions ({ STATE.fflags |= softfloat_exceptionFlags; \
                             softfloat_exceptionFlags = 0; })

#define sext32(x) ((sreg_t)(int32_t)(x))
#define zext32(x) ((reg_t)(uint32_t)(x))
#define sext_xprlen(x) (((sreg_t)(x) << (64-xprlen)) >> (64-xprlen))
#define zext_xprlen(x) (((reg_t)(x) << (64-xprlen)) >> (64-xprlen))

#define insn_length(x) \
  (((x) & 0x03) < 0x03 ? 2 : \
   ((x) & 0x1f) < 0x1f ? 4 : \
   ((x) & 0x3f) < 0x3f ? 6 : \
   8)

#define set_pc(x) \
  do { if ((x) & 3 /* For now... */) \
         throw trap_instruction_address_misaligned(x); \
       npc = sext_xprlen(x); \
     } while(0)

#define validate_csr(which, write) ({ \
  unsigned my_priv = (STATE.sr & SR_S) ? 1 : 0; \
  unsigned read_priv = ((which) >> 10) & 3; \
  unsigned write_priv = (((which) >> 8) & 3); \
  if (read_priv == 3) read_priv = write_priv, write_priv = -1; \
  if (my_priv < ((write) ? write_priv : read_priv)) \
    throw trap_privileged_instruction(); \
  (which); })

#endif
