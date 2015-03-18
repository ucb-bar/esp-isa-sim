#include "hwacha.h"

static const char* xpr[] = {
  "zero", "ra", "s0", "s1",  "s2",  "s3",  "s4",  "s5",
  "s6",   "s7", "s8", "s9", "s10", "s11",  "sp",  "tp",
  "v0",   "v1", "a0", "a1",  "a2",  "a3",  "a4",  "a5",
  "a6",   "a7", "t0", "t1",  "t2",  "t3",  "t4",  "gp"
};

static const char* fpr[] = {
  "fs0", "fs1",  "fs2",  "fs3",  "fs4",  "fs5",  "fs6",  "fs7",
  "fs8", "fs9", "fs10", "fs11", "fs12", "fs13", "fs14", "fs15",
  "fv0", "fv1", "fa0",   "fa1",  "fa2",  "fa3",  "fa4",  "fa5",
  "fa6", "fa7", "ft0",   "ft1",  "ft2",  "ft3",  "ft4",  "ft5"
};

static const char* vxpr[] = {
 "vx0",  "vx1",  "vx2",  "vx3",  "vx4",  "vx5",  "vx6",  "vx7",
 "vx8",  "vx9",  "vx10",  "vx11",  "vx12",  "vx13",  "vx14",  "vx15",
 "vx16",  "vx17",  "vx18",  "vx19",  "vx20",  "vx21",  "vx22",  "vx23",
 "vx24",  "vx25",  "vx26",  "vx27",  "vx28",  "vx29",  "vx30",  "vx31",
 "vx32",  "vx33",  "vx34",  "vx35",  "vx36",  "vx37",  "vx38",  "vx39",
 "vx40",  "vx41",  "vx42",  "vx43",  "vx44",  "vx45",  "vx46",  "vx47",
 "vx48",  "vx49",  "vx50",  "vx51",  "vx52",  "vx53",  "vx54",  "vx55",
 "vx56",  "vx57",  "vx58",  "vx59",  "vx60",  "vx61",  "vx62",  "vx63",
 "vx64",  "vx65",  "vx66",  "vx67",  "vx68",  "vx69",  "vx70",  "vx71",
 "vx72",  "vx73",  "vx74",  "vx75",  "vx76",  "vx77",  "vx78",  "vx79",
 "vx80",  "vx81",  "vx82",  "vx83",  "vx84",  "vx85",  "vx86",  "vx87",
 "vx88",  "vx89",  "vx90",  "vx91",  "vx92",  "vx93",  "vx94",  "vx95",
 "vx96",  "vx97",  "vx98",  "vx99", "vx100", "vx101", "vx102", "vx103",
 "vx104", "vx105", "vx106", "vx107", "vx108", "vx109", "vx110", "vx111",
 "vx112", "vx113", "vx114", "vx115", "vx116", "vx117", "vx118", "vx119",
 "vx120", "vx121", "vx122", "vx123", "vx124", "vx125", "vx126", "vx127",
 "vx128", "vx129", "vx130", "vx131", "vx132", "vx133", "vx134", "vx135",
 "vx136", "vx137", "vx138", "vx139", "vx140", "vx141", "vx142", "vx143",
 "vx144", "vx145", "vx146", "vx147", "vx148", "vx149", "vx150", "vx151",
 "vx152", "vx153", "vx154", "vx155", "vx156", "vx157", "vx158", "vx159",
 "vx160", "vx161", "vx162", "vx163", "vx164", "vx165", "vx166", "vx167",
 "vx168", "vx169", "vx170", "vx171", "vx172", "vx173", "vx174", "vx175",
 "vx176", "vx177", "vx178", "vx179", "vx180", "vx181", "vx182", "vx183",
 "vx184", "vx185", "vx186", "vx187", "vx188", "vx189", "vx190", "vx191",
 "vx192", "vx193", "vx194", "vx195", "vx196", "vx197", "vx198", "vx199",
 "vx200", "vx201", "vx202", "vx203", "vx204", "vx205", "vx206", "vx207",
 "vx208", "vx209", "vx210", "vx211", "vx212", "vx213", "vx214", "vx215",
 "vx216", "vx217", "vx218", "vx219", "vx220", "vx221", "vx222", "vx223",
 "vx224", "vx225", "vx226", "vx227", "vx228", "vx229", "vx230", "vx231",
 "vx232", "vx233", "vx234", "vx235", "vx236", "vx237", "vx238", "vx239",
 "vx240", "vx241", "vx242", "vx243", "vx244", "vx245", "vx246", "vx247",
 "vx248", "vx249", "vx250", "vx251", "vx252", "vx253", "vx254", "vx255"
};

static const char* vspr[] = {
 "vs0",  "vs1",  "vs2",  "vs3",  "vs4",  "vs5",  "vs6",  "vs7",
 "vs8",  "vs9",  "vs10",  "vs11",  "vs12",  "vs13",  "vs14",  "vs15",
 "vs16",  "vs17",  "vs18",  "vs19",  "vs20",  "vs21",  "vs22",  "vs23",
 "vs24",  "vs25",  "vs26",  "vs27",  "vs28",  "vs29",  "vs30",  "vs31",
 "vs32",  "vs33",  "vs34",  "vs35",  "vs36",  "vs37",  "vs38",  "vs39",
 "vs40",  "vs41",  "vs42",  "vs43",  "vs44",  "vs45",  "vs46",  "vs47",
 "vs48",  "vs49",  "vs50",  "vs51",  "vs52",  "vs53",  "vs54",  "vs55",
 "vs56",  "vs57",  "vs58",  "vs59",  "vs60",  "vs61",  "vs62",  "vs63",
 "vs64",  "vs65",  "vs66",  "vs67",  "vs68",  "vs69",  "vs70",  "vs71",
 "vs72",  "vs73",  "vs74",  "vs75",  "vs76",  "vs77",  "vs78",  "vs79",
 "vs80",  "vs81",  "vs82",  "vs83",  "vs84",  "vs85",  "vs86",  "vs87",
 "vs88",  "vs89",  "vs90",  "vs91",  "vs92",  "vs93",  "vs94",  "vs95",
 "vs96",  "vs97",  "vs98",  "vs99", "vs100", "vs101", "vs102", "vs103",
 "vs104", "vs105", "vs106", "vs107", "vs108", "vs109", "vs110", "vs111",
 "vs112", "vs113", "vs114", "vs115", "vs116", "vs117", "vs118", "vs119",
 "vs120", "vs121", "vs122", "vs123", "vs124", "vs125", "vs126", "vs127",
 "vs128", "vs129", "vs130", "vs131", "vs132", "vs133", "vs134", "vs135",
 "vs136", "vs137", "vs138", "vs139", "vs140", "vs141", "vs142", "vs143",
 "vs144", "vs145", "vs146", "vs147", "vs148", "vs149", "vs150", "vs151",
 "vs152", "vs153", "vs154", "vs155", "vs156", "vs157", "vs158", "vs159",
 "vs160", "vs161", "vs162", "vs163", "vs164", "vs165", "vs166", "vs167",
 "vs168", "vs169", "vs170", "vs171", "vs172", "vs173", "vs174", "vs175",
 "vs176", "vs177", "vs178", "vs179", "vs180", "vs181", "vs182", "vs183",
 "vs184", "vs185", "vs186", "vs187", "vs188", "vs189", "vs190", "vs191",
 "vs192", "vs193", "vs194", "vs195", "vs196", "vs197", "vs198", "vs199",
 "vs200", "vs201", "vs202", "vs203", "vs204", "vs205", "vs206", "vs207",
 "vs208", "vs209", "vs210", "vs211", "vs212", "vs213", "vs214", "vs215",
 "vs216", "vs217", "vs218", "vs219", "vs220", "vs221", "vs222", "vs223",
 "vs224", "vs225", "vs226", "vs227", "vs228", "vs229", "vs230", "vs231",
 "vs232", "vs233", "vs234", "vs235", "vs236", "vs237", "vs238", "vs239",
 "vs240", "vs241", "vs242", "vs243", "vs244", "vs245", "vs246", "vs247",
 "vs248", "vs249", "vs250", "vs251", "vs252", "vs253", "vs254", "vs255"
};

static const char* vppr[] = {
  "vp0",   "vp1",   "vp2",   "vp3",   "vp4",   "vp5",   "vp6",   "vp7",
  "vp8",   "vp9",   "vp10",  "vp11",  "vp12",  "vp13",  "vp14",  "vp15"
};

struct : public arg_t {
  std::string to_string(insn_t insn) const {
    return xpr[insn.rs1()];
  }
} xrs1;

struct : public arg_t {
  std::string to_string(insn_t insn) const {
    return xpr[insn.rs2()];
  }
} xrs2;

struct : public arg_t {
  std::string to_string(insn_t insn) const {
    return xpr[insn.rd()];
  }
} xrd;

struct : public arg_t {
  std::string to_string(insn_t insn) const {
    return fpr[insn.rd()];
  }
} frd;

struct : public arg_t {
  std::string to_string(insn_t insn) const {
    return fpr[insn.rs1()];
  }
} frs1;

struct : public arg_t {
  std::string to_string(insn_t insn) const {
    return fpr[insn.rs2()];
  }
} frs2;

struct : public arg_t {
  std::string to_string(insn_t insn) const {
    return fpr[insn.rs3()];
  }
} frs3;

struct : public arg_t {
  std::string to_string(insn_t insn) const {
    return xpr[insn.svsrd()];
  }
} svsrd;

struct : public arg_t {
  std::string to_string(insn_t insn) const {
    return xpr[insn.svard()];
  }
} svard;

struct : public arg_t {
  std::string to_string(insn_t insn) const {
    return vxpr[insn.vrd()];
  }
} vxrd;

struct : public arg_t {
  std::string to_string(insn_t insn) const {
    return vxpr[insn.vrs1()];
  }
} vxrs1;

struct : public arg_t {
  std::string to_string(insn_t insn) const {
    return vxpr[insn.vrs2()];
  }
} vxrs2;

struct : public arg_t {
  std::string to_string(insn_t insn) const {
    return vspr[insn.vrs1()];
  }
} vsrs1;

struct : public arg_t {
  std::string to_string(insn_t insn) const {
    return vspr[insn.vrs2()];
  }
} vsrs2;

struct : public arg_t {
  std::string to_string(insn_t insn) const {
    return std::to_string(insn.i_imm() & 0x3f);
  }
} nxregs;

struct : public arg_t {
  std::string to_string(insn_t insn) const {
    return std::to_string((int)insn.s_imm()) + '(' + xpr[insn.rs1()] + ')';
  }
} vf_addr;

std::vector<disasm_insn_t*> hwacha_t::get_disasms()
{
  std::vector<disasm_insn_t*> insns;

  #define DECLARE_INSN(code, match, mask) \
   const uint32_t match_##code = match; \
   const uint32_t mask_##code = mask;
  #include "opcodes_hwacha.h"
  #undef DECLARE_INSN

  #define DISASM_INSN(name, code, extra, ...) \
    insns.push_back(new disasm_insn_t(name, match_##code, mask_##code | (extra), __VA_ARGS__));

  DISASM_INSN("vsetcfg", vsetcfg, 0, {&xrs1, &nxregs});
  DISASM_INSN("vsetvl", vsetvl, 0, {&xrd, &xrs1});
  DISASM_INSN("vgetcfg", vgetcfg, 0, {&xrd});
  DISASM_INSN("vgetvl", vgetvl, 0, {&xrd});

  DISASM_INSN("vmss", vmss, 0, {&svsrd, &xrs1});
  DISASM_INSN("vmsa", vmsa, 0, {&svard, &xrs1});
  DISASM_INSN("vf", vf, 0, {&vf_addr});

  DISASM_INSN("vxcptcause", vxcptcause, 0, {&xrd});
  DISASM_INSN("vxcptaux", vxcptaux, 0, {&xrd});
  DISASM_INSN("vxcptevac", vxcptevac, 0, {&xrs1});
  DISASM_INSN("vxcpthold", vxcpthold, 0, {&xrs1});
  DISASM_INSN("vxcptkill", vxcptkill, 0, {});

  #define DECLARE_INSN(code, match, mask) \
   const uint64_t match_##code = match; \
   const uint64_t mask_##code = mask;
  #include "opcodes_hwacha_ut.h"
  #undef DECLARE_INSN

  #define DISASM_UT_INSN(name, code, extra, ...) \
    ut_disassembler.add_insn(new disasm_insn_t(name, match_##code, mask_##code | (extra), __VA_ARGS__));

  DISASM_UT_INSN("vstop", vstop, 0, {});
  DISASM_UT_INSN("veidx", veidx, 0, {&vxrd});

  const uint64_t mask_vseglen = 0x7UL << 29;

  #define DISASM_VMEM_INSN(name1, name2, code, ...) \
    DISASM_UT_INSN(name1, code, mask_vseglen, __VA_ARGS__) \
    DISASM_UT_INSN(name2, code, 0, __VA_ARGS__) \

  DISASM_VMEM_INSN("vld",    "vlsegd",    vlsegd,    {&vxrd, &vsrs1});
  DISASM_VMEM_INSN("vlw",    "vlsegw",    vlsegw,    {&vxrd, &vsrs1});
  DISASM_VMEM_INSN("vlwu",   "vlsegwu",   vlsegwu,   {&vxrd, &vsrs1});
  DISASM_VMEM_INSN("vlh",    "vlsegh",    vlsegh,    {&vxrd, &vsrs1});
  DISASM_VMEM_INSN("vlhu",   "vlseghu",   vlseghu,   {&vxrd, &vsrs1});
  DISASM_VMEM_INSN("vlb",    "vlsegb",    vlsegb,    {&vxrd, &vsrs1});
  DISASM_VMEM_INSN("vlbu",   "vlsegbu",   vlsegbu,   {&vxrd, &vsrs1});

  DISASM_VMEM_INSN("vlstd",  "vlsegstd",  vlsegstd,  {&vxrd, &vsrs1, &vsrs2});
  DISASM_VMEM_INSN("vlstw",  "vlsegstw",  vlsegstw,  {&vxrd, &vsrs1, &vsrs2});
  DISASM_VMEM_INSN("vlstwu", "vlsegstwu", vlsegstwu, {&vxrd, &vsrs1, &vsrs2});
  DISASM_VMEM_INSN("vlsth",  "vlsegsth",  vlsegsth,  {&vxrd, &vsrs1, &vsrs2});
  DISASM_VMEM_INSN("vlsthu", "vlsegsthu", vlsegsthu, {&vxrd, &vsrs1, &vsrs2});
  DISASM_VMEM_INSN("vlstb",  "vlsegstb",  vlsegstb,  {&vxrd, &vsrs1, &vsrs2});
  DISASM_VMEM_INSN("vlstbu", "vlsegstbu", vlsegstbu, {&vxrd, &vsrs1, &vsrs2});

  DISASM_VMEM_INSN("vsd",    "vssegd",    vssegd,    {&vxrd, &vsrs1});
  DISASM_VMEM_INSN("vsw",    "vssegw",    vssegw,    {&vxrd, &vsrs1});
  DISASM_VMEM_INSN("vsh",    "vssegh",    vssegh,    {&vxrd, &vsrs1});
  DISASM_VMEM_INSN("vsb",    "vssegb",    vssegb,    {&vxrd, &vsrs1});

  DISASM_VMEM_INSN("vsstd",  "vssegstd",  vssegstd,  {&vxrd, &vsrs1, &vsrs2});
  DISASM_VMEM_INSN("vsstw",  "vssegstw",  vssegstw,  {&vxrd, &vsrs1, &vsrs2});
  DISASM_VMEM_INSN("vssth",  "vssegsth",  vssegsth,  {&vxrd, &vsrs1, &vsrs2});
  DISASM_VMEM_INSN("vsstb",  "vssegstb",  vssegstb,  {&vxrd, &vsrs1, &vsrs2});

  DISASM_VMEM_INSN("vlxd",    "vlsegxd",    vlsegxd, {&vxrd, &vsrs1, &vxrs2});
  DISASM_VMEM_INSN("vlxw",    "vlsegxw",    vlsegxw, {&vxrd, &vsrs1, &vxrs2});
  DISASM_VMEM_INSN("vlxwu",   "vlsegxwu",   vlsegxwu,{&vxrd, &vsrs1, &vxrs2});
  DISASM_VMEM_INSN("vlxh",    "vlsegxh",    vlsegxh, {&vxrd, &vsrs1, &vxrs2});
  DISASM_VMEM_INSN("vlxhu",   "vlsegxhu",   vlsegxhu,{&vxrd, &vsrs1, &vxrs2});
  DISASM_VMEM_INSN("vlxb",    "vlsegxb",    vlsegxb, {&vxrd, &vsrs1, &vxrs2});
  DISASM_VMEM_INSN("vlxbu",   "vlsegxbu",   vlsegxbu,{&vxrd, &vsrs1, &vxrs2});

  DISASM_VMEM_INSN("vsxd",    "vssegxd",    vssegxd, {&vxrd, &vsrs1, &vxrs2});
  DISASM_VMEM_INSN("vsxw",    "vssegxw",    vssegxw, {&vxrd, &vsrs1, &vxrs2});
  DISASM_VMEM_INSN("vsxh",    "vssegxh",    vssegxh, {&vxrd, &vsrs1, &vxrs2});
  DISASM_VMEM_INSN("vsxb",    "vssegxb",    vssegxb, {&vxrd, &vsrs1, &vxrs2});

  return insns;
}
