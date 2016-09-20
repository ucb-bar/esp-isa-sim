#include "hwacha.h"

static const char* xpr[] = {
  "zero", "ra", "sp",  "gp",  "tp", "t0",  "t1",  "t2",
  "s0",   "s1", "a0",  "a1",  "a2", "a3",  "a4",  "a5",
  "a6",   "a7", "s2",  "s3",  "s4", "s5",  "s6",  "s7",
  "s8",   "s9", "s10", "s11", "t3", "t4",  "t5",  "t6"
};

static const char* fpr[] = {
  "ft0", "ft1", "ft2",  "ft3",  "ft4", "ft5", "ft6",  "ft7",
  "fs0", "fs1", "fa0",  "fa1",  "fa2", "fa3", "fa4",  "fa5",
  "fa6", "fa7", "fs2",  "fs3",  "fs4", "fs5", "fs6",  "fs7",
  "fs8", "fs9", "fs10", "fs11", "ft8", "ft9", "ft10", "ft11"
};

static const char* vvpr[] = {
 "vv0",  "vv1",  "vv2",  "vv3",  "vv4",  "vv5",  "vv6",  "vv7",
 "vv8",  "vv9",  "vv10",  "vv11",  "vv12",  "vv13",  "vv14",  "vv15",
 "vv16",  "vv17",  "vv18",  "vv19",  "vv20",  "vv21",  "vv22",  "vv23",
 "vv24",  "vv25",  "vv26",  "vv27",  "vv28",  "vv29",  "vv30",  "vv31",
 "vv32",  "vv33",  "vv34",  "vv35",  "vv36",  "vv37",  "vv38",  "vv39",
 "vv40",  "vv41",  "vv42",  "vv43",  "vv44",  "vv45",  "vv46",  "vv47",
 "vv48",  "vv49",  "vv50",  "vv51",  "vv52",  "vv53",  "vv54",  "vv55",
 "vv56",  "vv57",  "vv58",  "vv59",  "vv60",  "vv61",  "vv62",  "vv63",
 "vv64",  "vv65",  "vv66",  "vv67",  "vv68",  "vv69",  "vv70",  "vv71",
 "vv72",  "vv73",  "vv74",  "vv75",  "vv76",  "vv77",  "vv78",  "vv79",
 "vv80",  "vv81",  "vv82",  "vv83",  "vv84",  "vv85",  "vv86",  "vv87",
 "vv88",  "vv89",  "vv90",  "vv91",  "vv92",  "vv93",  "vv94",  "vv95",
 "vv96",  "vv97",  "vv98",  "vv99", "vv100", "vv101", "vv102", "vv103",
 "vv104", "vv105", "vv106", "vv107", "vv108", "vv109", "vv110", "vv111",
 "vv112", "vv113", "vv114", "vv115", "vv116", "vv117", "vv118", "vv119",
 "vv120", "vv121", "vv122", "vv123", "vv124", "vv125", "vv126", "vv127",
 "vv128", "vv129", "vv130", "vv131", "vv132", "vv133", "vv134", "vv135",
 "vv136", "vv137", "vv138", "vv139", "vv140", "vv141", "vv142", "vv143",
 "vv144", "vv145", "vv146", "vv147", "vv148", "vv149", "vv150", "vv151",
 "vv152", "vv153", "vv154", "vv155", "vv156", "vv157", "vv158", "vv159",
 "vv160", "vv161", "vv162", "vv163", "vv164", "vv165", "vv166", "vv167",
 "vv168", "vv169", "vv170", "vv171", "vv172", "vv173", "vv174", "vv175",
 "vv176", "vv177", "vv178", "vv179", "vv180", "vv181", "vv182", "vv183",
 "vv184", "vv185", "vv186", "vv187", "vv188", "vv189", "vv190", "vv191",
 "vv192", "vv193", "vv194", "vv195", "vv196", "vv197", "vv198", "vv199",
 "vv200", "vv201", "vv202", "vv203", "vv204", "vv205", "vv206", "vv207",
 "vv208", "vv209", "vv210", "vv211", "vv212", "vv213", "vv214", "vv215",
 "vv216", "vv217", "vv218", "vv219", "vv220", "vv221", "vv222", "vv223",
 "vv224", "vv225", "vv226", "vv227", "vv228", "vv229", "vv230", "vv231",
 "vv232", "vv233", "vv234", "vv235", "vv236", "vv237", "vv238", "vv239",
 "vv240", "vv241", "vv242", "vv243", "vv244", "vv245", "vv246", "vv247",
 "vv248", "vv249", "vv250", "vv251", "vv252", "vv253", "vv254", "vv255"
};

static const char* vapr[] = {
 "vva",  "va1",  "va2",  "va3",  "va4",  "va5",  "va6",  "va7",
 "va8",  "va9",  "va10",  "va11",  "va12",  "va13",  "va14",  "va15",
 "va16",  "va17",  "va18",  "va19",  "va20",  "va21",  "va22",  "va23",
 "va24",  "va25",  "va26",  "va27",  "va28",  "va29",  "va30",  "va31"
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
    return vspr[insn.svsrd()];
  }
} svsrd;

struct : public arg_t {
  std::string to_string(insn_t insn) const {
    return vapr[insn.svard()];
  }
} svard;

struct : public arg_t {
  std::string to_string(insn_t insn) const {
    return vvpr[insn.vrd()];
  }
} vvrd;

struct : public arg_t {
  std::string to_string(insn_t insn) const {
    return vvpr[insn.vrs1()];
  }
} vvrs1;

struct : public arg_t {
  std::string to_string(insn_t insn) const {
    return vvpr[insn.vrs2()];
  }
} vvrs2;

struct : public arg_t {
  std::string to_string(insn_t insn) const {
    return vspr[insn.vrd()];
  }
} vsrd;

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
    return insn.vd() ? vvpr[insn.vrd()] : vspr[insn.vrd()];
  }
} vdrd;

struct : public arg_t {
  std::string to_string(insn_t insn) const {
    return insn.vs1() ? vvpr[insn.vrs1()] : vspr[insn.vrs1()];
  }
} vdrs1;

struct : public arg_t {
  std::string to_string(insn_t insn) const {
    return insn.vs2() ? vvpr[insn.vrs2()] : vspr[insn.vrs2()];
  }
} vdrs2;

struct : public arg_t {
  std::string to_string(insn_t insn) const {
    return insn.vs3() ? vvpr[insn.vrs3()] : vspr[insn.vrs3()];
  }
} vdrs3;

struct : public arg_t {
  std::string to_string(insn_t insn) const {
    return vppr[insn.vrd()];
  }
} vprd;

struct : public arg_t {
  std::string to_string(insn_t insn) const {
    return vppr[insn.vrs1()];
  }
} vprs1;

struct : public arg_t {
  std::string to_string(insn_t insn) const {
    return vppr[insn.vrs2()];
  }
} vprs2;

struct : public arg_t {
  std::string to_string(insn_t insn) const {
    return vppr[insn.vrs3()];
  }
} vprs3;

struct : public arg_t {
  std::string to_string(insn_t insn) const {
    return std::to_string(insn.i_imm() & 0x3f);
  }
} nxregs;

struct : public arg_t {
  std::string to_string(insn_t insn) const {
    return std::to_string((insn.i_imm()>>8) & 0xf);
  }
} npregs;

struct : public arg_t {
  std::string to_string(insn_t insn) const {
    return std::to_string((int)insn.s_imm()) + '(' + xpr[insn.rs1()] + ')';
  }
} vf_addr;

struct : public arg_t {
  std::string to_string(insn_t insn) const {
    return std::string("0(") + vvpr[insn.vrs1()] + ')';
  }
} vamo_address;

struct : public arg_t {
  std::string to_string(insn_t insn) const {
    return std::to_string((int)insn.v_imm());
  }
} vimm;

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

  DISASM_INSN("vsetcfg", vsetcfg, 0, {&npregs, &nxregs});
  DISASM_INSN("vsetvl", vsetvl, 0, {&xrd, &xrs1});
  DISASM_INSN("vgetcfg", vgetcfg, 0, {&xrd});
  DISASM_INSN("vgetvl", vgetvl, 0, {&xrd});

  DISASM_INSN("vmcs", vmcs, 0, {&svsrd, &xrs1});
  DISASM_INSN("vmca", vmca, 0, {&svard, &xrs1});
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
    insns.push_back(new disasm_insn_t(name, match_##code, mask_##code | (extra), __VA_ARGS__)); \
    ut_disassembler->add_insn(new disasm_insn_t(name, match_##code, mask_##code | (extra), __VA_ARGS__));

  #define DEFINE_RTYPE(code) DISASM_UT_INSN(#code, code, 0, {&vdrd, &vdrs1, &vdrs2})
  #define DEFINE_R1TYPE(code) DISASM_UT_INSN(#code, code, 0, {&vdrd, &vdrs1})
  #define DEFINE_R3TYPE(code) DISASM_UT_INSN(#code, code, 0, {&vdrd, &vdrs1, &vdrs2, &vdrs3})
  #define DEFINE_ITYPE(code) DISASM_UT_INSN(#code, code, 0, {&vsrd, &vsrs1, &vimm})
  #define DEFINE_XLOAD(code) DISASM_UT_INSN(#code, code, 0, {&vvrd, &vars1, &vvrs2})
  #define DEFINE_XSTORE(code) DISASM_UT_INSN(#code, code, 0, {&vvrs2, &vars1, $vvrs2})
  #define DEFINE_XAMO(code) DISASM_UT_INSN(#code, code, 0, {&vvrd, &vamo_address, &vdrs2})

  DISASM_UT_INSN("vstop", vstop, 0, {});
  DISASM_UT_INSN("veidx", veidx, 0, {&vvrd});
  DISASM_UT_INSN("vpop", vpop, 0, {&vprd, &vprs1, &vprs2, &vprs3});

  DEFINE_ITYPE(vaddi);
  DEFINE_ITYPE(vslli);
  DEFINE_ITYPE(vslti);
  DEFINE_ITYPE(vsltiu);
  DEFINE_ITYPE(vxori);
  DEFINE_ITYPE(vsrli);
  DEFINE_ITYPE(vsrai);
  DEFINE_ITYPE(vori);
  DEFINE_ITYPE(vandi);
  DEFINE_ITYPE(vaddiw);
  DEFINE_ITYPE(vslliw);
  DEFINE_ITYPE(vsrliw);
  DEFINE_ITYPE(vsraiw);

  DEFINE_RTYPE(vadd);
  DEFINE_RTYPE(vsub);
  DEFINE_RTYPE(vsll);
  DEFINE_RTYPE(vslt);
  DEFINE_RTYPE(vsltu);
  DEFINE_RTYPE(vxor);
  DEFINE_RTYPE(vsrl);
  DEFINE_RTYPE(vsra);
  DEFINE_RTYPE(vor);
  DEFINE_RTYPE(vand);
  DEFINE_RTYPE(vmul);
  DEFINE_RTYPE(vmulh);
  DEFINE_RTYPE(vmulhu);
  DEFINE_RTYPE(vmulhsu);
  DEFINE_RTYPE(vdiv);
  DEFINE_RTYPE(vdivu);
  DEFINE_RTYPE(vrem);
  DEFINE_RTYPE(vremu);
  DEFINE_RTYPE(vaddw);
  DEFINE_RTYPE(vsubw);
  DEFINE_RTYPE(vsllw);
  DEFINE_RTYPE(vsrlw);
  DEFINE_RTYPE(vsraw);
  DEFINE_RTYPE(vmulw);
  DEFINE_RTYPE(vdivw);
  DEFINE_RTYPE(vdivuw);
  DEFINE_RTYPE(vremw);
  DEFINE_RTYPE(vremuw);

  DEFINE_RTYPE(vfadd_s);
  DEFINE_RTYPE(vfsub_s);
  DEFINE_RTYPE(vfmul_s);
  DEFINE_RTYPE(vfdiv_s);
  DEFINE_R1TYPE(vfsqrt_s);
  DEFINE_RTYPE(vfmin_s);
  DEFINE_RTYPE(vfmax_s);
  DEFINE_R3TYPE(vfmadd_s);
  DEFINE_R3TYPE(vfmsub_s);
  DEFINE_R3TYPE(vfnmadd_s);
  DEFINE_R3TYPE(vfnmsub_s);
  DEFINE_RTYPE(vfsgnj_s);
  DEFINE_RTYPE(vfsgnjn_s);
  DEFINE_RTYPE(vfsgnjx_s);
  DEFINE_R1TYPE(vfcvt_s_d);
  DEFINE_R1TYPE(vfcvt_s_l);
  DEFINE_R1TYPE(vfcvt_s_lu);
  DEFINE_R1TYPE(vfcvt_s_w);
  DEFINE_R1TYPE(vfcvt_s_wu);
  DEFINE_R1TYPE(vfcvt_s_wu);
  DEFINE_R1TYPE(vfcvt_l_s);
  DEFINE_R1TYPE(vfcvt_lu_s);
  DEFINE_R1TYPE(vfcvt_w_s);
  DEFINE_R1TYPE(vfcvt_wu_s);
  DEFINE_R1TYPE(vfclass_s);
  DEFINE_R1TYPE(vcmpfeq_s);
  DEFINE_R1TYPE(vcmpflt_s);
  DEFINE_R1TYPE(vcmpfle_s);

  DEFINE_RTYPE(vfadd_d);
  DEFINE_RTYPE(vfsub_d);
  DEFINE_RTYPE(vfmul_d);
  DEFINE_RTYPE(vfdiv_d);
  DEFINE_R1TYPE(vfsqrt_d);
  DEFINE_RTYPE(vfmin_d);
  DEFINE_RTYPE(vfmax_d);
  DEFINE_R3TYPE(vfmadd_d);
  DEFINE_R3TYPE(vfmsub_d);
  DEFINE_R3TYPE(vfnmadd_d);
  DEFINE_R3TYPE(vfnmsub_d);
  DEFINE_RTYPE(vfsgnj_d);
  DEFINE_RTYPE(vfsgnjn_d);
  DEFINE_RTYPE(vfsgnjx_d);
  DEFINE_R1TYPE(vfcvt_d_s);
  DEFINE_R1TYPE(vfcvt_d_l);
  DEFINE_R1TYPE(vfcvt_d_lu);
  DEFINE_R1TYPE(vfcvt_d_w);
  DEFINE_R1TYPE(vfcvt_d_wu);
  DEFINE_R1TYPE(vfcvt_d_wu);
  DEFINE_R1TYPE(vfcvt_l_d);
  DEFINE_R1TYPE(vfcvt_lu_d);
  DEFINE_R1TYPE(vfcvt_w_d);
  DEFINE_R1TYPE(vfcvt_wu_d);
  DEFINE_R1TYPE(vfclass_d);
  DEFINE_R1TYPE(vcmpfeq_d);
  DEFINE_R1TYPE(vcmpflt_d);
  DEFINE_R1TYPE(vcmpfle_d);

  DEFINE_XAMO(vamoadd_w)
  DEFINE_XAMO(vamoswap_w)
  DEFINE_XAMO(vamoand_w)
  DEFINE_XAMO(vamoor_w)
  DEFINE_XAMO(vamoxor_w)
  DEFINE_XAMO(vamomin_w)
  DEFINE_XAMO(vamomax_w)
  DEFINE_XAMO(vamominu_w)
  DEFINE_XAMO(vamomaxu_w)
  DEFINE_XAMO(vamoadd_d)
  DEFINE_XAMO(vamoswap_d)
  DEFINE_XAMO(vamoand_d)
  DEFINE_XAMO(vamoor_d)
  DEFINE_XAMO(vamoxor_d)
  DEFINE_XAMO(vamomin_d)
  DEFINE_XAMO(vamomax_d)
  DEFINE_XAMO(vamominu_d)
  DEFINE_XAMO(vamomaxu_d)

  const uint64_t mask_vseglen = 0x7UL << 29;

  #define DISASM_VMEM_INSN(name1, name2, code, ...) \
    DISASM_UT_INSN(name1, code, mask_vseglen, __VA_ARGS__) \
    DISASM_UT_INSN(name2, code, 0, __VA_ARGS__) \

  DISASM_VMEM_INSN("vld",    "vlsegd",    vlsegd,    {&vvrd, &vsrs1});
  DISASM_VMEM_INSN("vlw",    "vlsegw",    vlsegw,    {&vvrd, &vsrs1});
  DISASM_VMEM_INSN("vlwu",   "vlsegwu",   vlsegwu,   {&vvrd, &vsrs1});
  DISASM_VMEM_INSN("vlh",    "vlsegh",    vlsegh,    {&vvrd, &vsrs1});
  DISASM_VMEM_INSN("vlhu",   "vlseghu",   vlseghu,   {&vvrd, &vsrs1});
  DISASM_VMEM_INSN("vlb",    "vlsegb",    vlsegb,    {&vvrd, &vsrs1});
  DISASM_VMEM_INSN("vlbu",   "vlsegbu",   vlsegbu,   {&vvrd, &vsrs1});

  DISASM_VMEM_INSN("vlstd",  "vlsegstd",  vlsegstd,  {&vvrd, &vsrs1, &vsrs2});
  DISASM_VMEM_INSN("vlstw",  "vlsegstw",  vlsegstw,  {&vvrd, &vsrs1, &vsrs2});
  DISASM_VMEM_INSN("vlstwu", "vlsegstwu", vlsegstwu, {&vvrd, &vsrs1, &vsrs2});
  DISASM_VMEM_INSN("vlsth",  "vlsegsth",  vlsegsth,  {&vvrd, &vsrs1, &vsrs2});
  DISASM_VMEM_INSN("vlsthu", "vlsegsthu", vlsegsthu, {&vvrd, &vsrs1, &vsrs2});
  DISASM_VMEM_INSN("vlstb",  "vlsegstb",  vlsegstb,  {&vvrd, &vsrs1, &vsrs2});
  DISASM_VMEM_INSN("vlstbu", "vlsegstbu", vlsegstbu, {&vvrd, &vsrs1, &vsrs2});

  DISASM_VMEM_INSN("vsd",    "vssegd",    vssegd,    {&vvrd, &vsrs1});
  DISASM_VMEM_INSN("vsw",    "vssegw",    vssegw,    {&vvrd, &vsrs1});
  DISASM_VMEM_INSN("vsh",    "vssegh",    vssegh,    {&vvrd, &vsrs1});
  DISASM_VMEM_INSN("vsb",    "vssegb",    vssegb,    {&vvrd, &vsrs1});

  DISASM_VMEM_INSN("vsstd",  "vssegstd",  vssegstd,  {&vvrd, &vsrs1, &vsrs2});
  DISASM_VMEM_INSN("vsstw",  "vssegstw",  vssegstw,  {&vvrd, &vsrs1, &vsrs2});
  DISASM_VMEM_INSN("vssth",  "vssegsth",  vssegsth,  {&vvrd, &vsrs1, &vsrs2});
  DISASM_VMEM_INSN("vsstb",  "vssegstb",  vssegstb,  {&vvrd, &vsrs1, &vsrs2});

  DISASM_VMEM_INSN("vlxd",    "vlsegxd",    vlsegxd, {&vvrd, &vsrs1, &vvrs2});
  DISASM_VMEM_INSN("vlxw",    "vlsegxw",    vlsegxw, {&vvrd, &vsrs1, &vvrs2});
  DISASM_VMEM_INSN("vlxwu",   "vlsegxwu",   vlsegxwu,{&vvrd, &vsrs1, &vvrs2});
  DISASM_VMEM_INSN("vlxh",    "vlsegxh",    vlsegxh, {&vvrd, &vsrs1, &vvrs2});
  DISASM_VMEM_INSN("vlxhu",   "vlsegxhu",   vlsegxhu,{&vvrd, &vsrs1, &vvrs2});
  DISASM_VMEM_INSN("vlxb",    "vlsegxb",    vlsegxb, {&vvrd, &vsrs1, &vvrs2});
  DISASM_VMEM_INSN("vlxbu",   "vlsegxbu",   vlsegxbu,{&vvrd, &vsrs1, &vvrs2});

  DISASM_VMEM_INSN("vsxd",    "vssegxd",    vssegxd, {&vvrd, &vsrs1, &vvrs2});
  DISASM_VMEM_INSN("vsxw",    "vssegxw",    vssegxw, {&vvrd, &vsrs1, &vvrs2});
  DISASM_VMEM_INSN("vsxh",    "vssegxh",    vssegxh, {&vvrd, &vsrs1, &vvrs2});
  DISASM_VMEM_INSN("vsxb",    "vssegxb",    vssegxb, {&vvrd, &vsrs1, &vvrs2});

  return insns;
}
