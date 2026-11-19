"""Execute production FPU EA emission and helper-call allocator sequencing."""

import os
from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


class FpuJitCompilerRegressions(unittest.TestCase):
    def test_explicit_fpsr_conditions_in_compiled_set_and_branch(self):
        source = (ROOT / "src/jit/x86_64/compemu_fpp_x86_64.cpp").read_text()

        def function(text, signature):
            start = text.index(signature)
            return text[start:text.index("\n}", start) + 2]

        predicates = "\n".join(function(source, signature) for signature in (
            "static void get_fp_condition(", "static void fp_signal_bsun(void)\n{",
            "void comp_fscc_opp(", "void comp_fbcc_opp("))
        source = (ROOT / "src/fpp.cpp").read_text()
        tables = source[source.index("static const bool condition_table_040_060"):
                        source.index("int fpp_cond(")]
        source = (ROOT / "src/jit/x86_64/compemu_midfunc_x86_64.cpp").read_text()
        override = "\n".join(function(source, signature) for signature in (
            "MIDFUNC(0,dont_care_flags_x87,", "MIDFUNC(4,fpu_cond_from_fpsr,"))
        driver = r"""
#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>
using uae_u8 = uint8_t;
using uae_u16 = uint16_t;
using uae_u32 = uint32_t;
using uae_u64 = uint64_t;
using uae_s16 = int16_t;
using uae_s32 = int32_t;
using uintptr = uintptr_t;
using IMM = uintptr;
using W4 = int;
enum {S1 = 20, S2, S3, S4, PC_P, TRASH = 0, VALID = 1};
struct { int flags_in_flags, flags_on_stack; } live;
struct { uae_u32 fpsr; struct { uae_u8 fp[8]; } fp_result; } regs;  // double
struct { bool compfpu = true; } currprefs;
static bool jit_fpu_enabled() { return currprefs.compfpu; }  // the host core: no exact core here
struct { bool fscc, fbcc; } jit_disable;
static bool failure, jit_fpu_fast_block, result_live, jit_fpu_cc_fpsr;
static bool jit_fp_result_live() { return result_live; }
static bool fpu_fpsr_cc_written;
#define FPSR_CC_N       0x08000000
#define FPSR_CC_Z       0x04000000
#define FPSR_CC_I       0x02000000
#define FPSR_CC_NAN     0x01000000
#define FPSR_CC_N_BIT   FPSR_CC_N
#define FPSR_CC_Z_BIT   FPSR_CC_Z
#define FPSR_CC_I_BIT   FPSR_CC_I
#define FPSR_CC_NAN_BIT FPSR_CC_NAN
#define FPSR_BSUN 0x00008000
#define FPU_COND_CHECK_RESULT 1
#define FPU_COND_SIGNAL 2
enum {NATIVE_CC_EQ = 4, NATIVE_CC_NE = 5};
static uintptr constants[32], vreg[32], not_taken, taken;
static uae_u32 m68k_pc_offset;
static uae_u8 *comp_pc_p = reinterpret_cast<uae_u8 *>(uintptr(0x123450000000));
static int displacement, branch_cc, represented_control, flags;
struct Op { int kind, d, s; uintptr value; };
enum {MOVI, MOVR, ADDI, CMOV, FPFLAGS, MOVB, CMP, JCC, JMP, LOAD, SHR, AND, BT, SBB, TEST, OR,
    ORI, ORM, CMPI, TESTB, STORE, STOREB, ORMI, TESTI};
static Op ops[160];
static uae_u8 code[160];
static int length;
static uae_u32 load(uintptr ad) { uae_u32 v; memcpy(&v, reinterpret_cast<void *>(ad), 4); return v; }
static int zf(uintptr v) { return uae_u32(v) ? 0 : 64; }
static void emit(int kind, int d = 0, int s = 0, uintptr value = 0) {
    ops[length++] = {kind, d, s, value};
}
static bool cc(int condition) {
    bool c = flags & 1, p = flags & 4, z = flags & 64;
    switch (condition) {
    case 2: return c; case 3: return !c; case 4: return z; case 5: return !z;
    case 6: return c || z; case 7: return !c && !z;
    case 10: return p; case 11: return !p; default: assert(false); return false;
    }
}
static void execute() {
    for (int pc = 0; pc < length; ++pc) {
        const Op &o = ops[pc];
        switch (o.kind) {
        case MOVI: vreg[o.d] = o.value; break;
        case MOVR: vreg[o.d] = vreg[o.s]; break;
        case ADDI: vreg[o.d] += o.value; break;
        case CMOV: if (cc(o.value)) vreg[o.d] = vreg[o.s]; break;
        case FPFLAGS:
            flags = (represented_control & 1) ? 0x45 :
                (represented_control & 4) ? 0x40 : (represented_control & 8) ? 1 : 0;
            break;
        case MOVB: vreg[o.d] = (vreg[o.d] & ~uintptr(255)) | (vreg[o.s] & 255); break;
        case CMP: flags = uintptr(fpu_fpsr_cc_written) == o.value ? 64 : 0; break;
        case JCC: if (cc(o.value)) pc += 1 + code[pc + 1]; else ++pc; break;
        case JMP: pc += 1 + code[pc + 1]; break;
        case LOAD: vreg[o.d] = load(o.value); break;
        case SHR: vreg[o.d] = uae_u32(vreg[o.d]) >> o.value; flags = zf(vreg[o.d]); break;
        case AND: vreg[o.d] &= o.value; flags = zf(vreg[o.d]); break;
        case BT: flags = (vreg[o.d] >> (vreg[o.s] & 31)) & 1; break;
        case SBB: vreg[o.d] = (flags & 1) ? uae_u32(-1) : 0; break;
        case TEST: flags = zf(vreg[o.d]); break;
        case OR: vreg[o.d] |= vreg[o.s]; flags = zf(vreg[o.d]); break;
        case ORI: vreg[o.d] |= o.value; flags = zf(vreg[o.d]); break;
        case ORM: vreg[o.d] |= load(o.value); flags = zf(vreg[o.d]); break;
        case CMPI:
            flags = zf(uae_u32(vreg[o.d]) - uae_u32(o.value)) | (uae_u32(vreg[o.d]) < o.value);
            break;
        case TESTB: flags = (*reinterpret_cast<uae_u8 *>(o.value) & o.s) ? 0 : 64; break;
        case STORE: { const uae_u32 v = uae_u32(vreg[o.s]); memcpy(reinterpret_cast<void *>(o.d == 1 ? o.value : uintptr(&regs.fpsr)), &v, 4); break; }
        case STOREB: fpu_fpsr_cc_written = o.value; break;
        case TESTI: flags = (vreg[o.d] & o.value) ? 0 : 64; break;
        case ORMI: { const uae_u32 v = load(o.value) | uae_u32(o.s); memcpy(reinterpret_cast<void *>(o.value), &v, 4); break; }
        default: assert(false);
        }
    }
}
static void mov_l_ri(int d, uintptr value) { constants[d] = value; emit(MOVI, d, 0, value); }
static void mov_l_rr(int d, int s) { emit(MOVR, d, s); }
static void add_l_ri(int d, uintptr value) { constants[d] += value; emit(ADDI, d, 0, value); }
static void mov_b_rr(int d, int s) { emit(MOVB, d, s); }
static void cmov_l_rr(int d, int s, int condition) { emit(CMOV, d, s, condition); }
static void clobber_flags() {
    if (live.flags_in_flags == VALID && live.flags_on_stack != VALID)
        live.flags_on_stack = VALID;
    live.flags_in_flags = TRASH;
}
static void fflags_into_flags(int) {
    assert(result_live);
    clobber_flags();
    live.flags_in_flags = VALID; live.flags_on_stack = TRASH;
    emit(FPFLAGS);
}
static uintptr get_const(int r) { return constants[r]; }
static int comp_get_iword(int) { return displacement & 0xffff; }
static int comp_get_ilong(int) { return displacement; }
static void test_l_rr(int d, int) { emit(TEST, d); }
static void register_branch(uintptr a, uintptr b, uae_u8 condition) {
    not_taken = a; taken = b; branch_cc = condition;
}
static int writereg(int r, int) { assert(live.flags_in_flags == TRASH); return r; }
static void unlock2(int) {}
static void raw_cmp_b_mi(uintptr ad, int value) {
    assert(ad == uintptr(&fpu_fpsr_cc_written)); emit(CMP, 0, 0, value);
}
static void raw_jcc_b_oponly(int condition) { emit(JCC, 0, 0, condition); }
static void raw_jnz_b_oponly() { raw_jcc_b_oponly(NATIVE_CC_NE); }
static void raw_jz_b_oponly() { raw_jcc_b_oponly(NATIVE_CC_EQ); }
static void raw_or_l_mi(uintptr ad, uintptr i) { assert(ad == uintptr(&regs.fpsr)); emit(ORMI, 0, int(i), ad); }
static void raw_jmp_b_oponly() { emit(JMP); }
static uae_u8 *get_target() { return code + length; }
static void skip_byte() { ++length; }
static void raw_mov_l_rm(int d, uintptr ad) { emit(LOAD, d, 0, ad); }
static void raw_or_l(int d, int s) { emit(OR, d, s); }
static void raw_or_l_ri(int d, uintptr i) { emit(ORI, d, 0, i); }
static void raw_or_l_rm(int d, uintptr ad) { emit(ORM, d, 0, ad); }
static void raw_cmp_l_ri(int d, uintptr i) { emit(CMPI, d, 0, i); }
static void raw_test_l_rr(int d, int s) { assert(d == s); emit(TEST, d); }
static void raw_test_b_mi(uintptr ad, int i) { emit(TESTB, 0, i, ad); }
static void raw_test_l_ri(int d, uintptr i) { emit(TESTI, d, 0, i); }
static void raw_mov_l_mr(uintptr ad, int s) { assert(ad == uintptr(&regs.fpsr)); emit(STORE, 0, s); }
static void raw_mov_b_mi(uintptr ad, int value) {
    assert(ad == uintptr(&fpu_fpsr_cc_written)); emit(STOREB, 0, 0, value);
}
static void raw_shrl_l_ri(int d, int i) { emit(SHR, d, 0, i); }
static void raw_and_l_ri(int d, int i) { emit(AND, d, 0, i); }
static void raw_mov_l_ri(int d, uintptr i) { emit(MOVI, d, 0, i); }
static void raw_bt_l_rr(int d, int s) { emit(BT, d, s); }
static void raw_sbb_l(int d, int s) { assert(d == s); emit(SBB, d); }
static void and_l_ri(int d, uintptr i) { emit(AND, d, 0, i); }
static void or_l(int d, int s) { emit(OR, d, s); }
static void mov_l_rm(int d, uintptr ad) { emit(LOAD, d, 0, ad); }
static void mov_l_mr(uintptr ad, int s) { emit(STORE, 1, s, ad); }
#define MIDFUNC(n, name, args) static void name args
#define FAIL(x) (failure = true)
#define JITPTR (uintptr)
static void fp_signal_bsun(void);
""" + tables + override + predicates + r"""
// Compile states: FP_RESULT in an x87 register; condition codes in the FPSR
// or a written-back FP_RESULT (run-time check); known to be in the FPSR.
enum {LIVE, CHECK, FPSR};
static bool representable(int control) {
    return !(control == 3 || control == 5 || control == 6 || control == 7 ||
        control == 11 || control >= 13);
}
static void compile(int mode, bool branch, bool long_branch, int condition, int offset) {
    length = 0; failure = false; m68k_pc_offset = 2; displacement = offset;
    result_live = mode == LIVE;
    jit_fpu_cc_fpsr = mode == FPSR;
    live.flags_in_flags = VALID; live.flags_on_stack = TRASH;
    if (branch)
        comp_fbcc_opp(0xf280 | condition | (long_branch ? 0x40 : 0));
    else
        comp_fscc_opp(0xf243, condition);
    assert(!failure);
    assert(live.flags_in_flags == TRASH && live.flags_on_stack == VALID);
    assert(mode == LIVE || jit_fpu_cc_fpsr);
}
// Run the block for every run-time state of a compile state. The result
// is checked by check(control); the FPSR must end with control.
struct Raw { uae_u64 m; };
static const std::vector<Raw> raws[5] = {
    {{0x3ff0000000000000ULL}, {0x0000000000000001ULL}, {0x0000000100000000ULL},
     {0x0008000000000000ULL}, {0x7fefffffffffffffULL}},                                // finite
    {{0x7ff8000000000000ULL}, {0x7ff0000000000001ULL}, {0x7ff0000100000000ULL}},        // NaN
    {{0x7ff0000000000000ULL}},                                                          // infinity
    {}, {{0}}};                                                                         // zero
static void set_result(const Raw &raw, bool negative) {
    const uae_u64 v = raw.m | (negative ? 0x8000000000000000ULL : 0);
    memcpy(regs.fp_result.fp, &v, 8);
}
template <typename F>
static void run(int mode, uae_u32 extra_fpsr, F check) {
    for (int state = 0; state <= 1; ++state) for (int control = 0; control < 16; ++control) {
        if (mode != CHECK && state == 0)
            continue;
        if (mode == LIVE && !representable(control))
            continue;
        const int kind = control & 7;
        const std::vector<Raw> none = {raws[4][0]};
        for (const Raw &raw : state == 0 ? (kind == 0 || kind == 1 || kind == 2 || kind == 4 ?
                raws[kind] : std::vector<Raw>()) : none) {
            const uae_u32 cc = uae_u32(control) << 24;
            fpu_fpsr_cc_written = state;
            regs.fpsr = 0x00ab1270 | (state == 0 ? uae_u32(~control & 15) << 24 : cc);
            set_result(raw, control & 8);
            represented_control = (control & 1) ? 1 : (control & 4) ? (control & 12) : control;
            vreg[3] = 0x12345678;
            execute();
            check(control);
            assert(regs.fpsr == (0x00ab1270 | cc | extra_fpsr * (control & 1)));
            assert(mode == LIVE || fpu_fpsr_cc_written);
        }
    }
}
int main() {
    for (const bool *table : {condition_table_6888x, condition_table_040_060}) {
        condition_table = table;
        for (int mode : {LIVE, CHECK, FPSR})
        for (bool branch : {false, true}) for (bool long_branch : {false, true})
        for (int condition = 0; condition < 16; ++condition)
        for (int offset : {-100, 100}) {
            compile(mode, branch, long_branch, condition, offset);
            if (branch) {
                assert(not_taken == uintptr(comp_pc_p) + (long_branch ? 6 : 4));
                assert(taken == uintptr(comp_pc_p) + 2 + offset);
                assert(branch_cc == 5);
            }
            run(mode, 0, [&](int control) {
                bool expected = table[control * 32 + condition];
                if (branch)
                    assert(cc(branch_cc) == expected);
                else
                    assert(vreg[3] == (expected ? 0x123456ffU : 0x12345600U));
            });
        }
    }
    // Signalling predicates: the interpreter outside blocks guarded against
    // the BSUN enable; inline inside them, setting BSUN and IOP on a NaN.
    condition_table = condition_table_6888x;
    for (int condition = 16; condition < 32; ++condition) {
        jit_fpu_fast_block = false;
        length = 0; failure = false;
        comp_fscc_opp(0xf240, condition);
        assert(failure && !length);
        failure = false;
        comp_fbcc_opp(0xf280 | condition);
        assert(failure && !length);
        jit_fpu_fast_block = true;
        for (int mode : {LIVE, CHECK, FPSR}) for (bool branch : {false, true}) {
            compile(mode, branch, false, condition, 100);
            run(mode, 0x8080, [&](int control) {
                bool expected = condition_table[control * 32 + condition];
                if (branch)
                    assert(cc(branch_cc) == expected);
                else
                    assert(vreg[3] == (expected ? 0x123456ffU : 0x12345600U));
            });
        }
    }
    for (int condition = 32; condition < 64; condition += 15) {
        length = 0; failure = false;
        comp_fscc_opp(0xf240, condition);
        assert(failure && !length);
    }
}
"""
        with tempfile.TemporaryDirectory(prefix=".fpu-predicates-", dir=ROOT / "tests") as directory:
            directory = Path(directory)
            cpp = directory / "driver.cpp"
            binary = directory / "predicates"
            cpp.write_text(driver)
            subprocess.run(
                ["c++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
                 str(cpp), "-o", str(binary)], check=True, timeout=30)
            subprocess.run([str(binary)], check=True, timeout=10)

    def test_decoded_ea_and_helper_call_state(self):
        source = (ROOT / "src/jit/x86_64/compemu_fpp_x86_64.cpp").read_text()
        start = source.index("static int get_fp_native_ea(")
        ea = source[start:source.index("\n}", start) + 2]
        start = source.index("static bool fp_native_disabled(")
        disabled = source[start:source.index("\n}", start) + 2]
        header = (ROOT / "src/jit/x86_64/compemu_x86_64.h").read_text()
        start = header.index("struct jit_disable_opcodes {")
        flags = header[start:header.index("\n};", start) + 3]
        source = (ROOT / "src/jit/x86_64/compemu_midfunc_x86_64.cpp").read_text()
        start = source.index("MIDFUNC(4,fpu_native_call,")
        call = source[start:source.index("\n}", start) + 2]
        driver = r"""
#include <cassert>
#include <cstdint>
#include <cstring>
using uae_u8 = uint8_t;
using uae_u16 = uint16_t;
using uae_u32 = uint32_t;
using uae_s16 = int16_t;
using uae_s32 = int32_t;
using uaecptr = uint32_t;
using uintptr = uintptr_t;
using IMM = uintptr_t;
using RR4 = int;
enum {S1 = 20, S2 = 21, PC_P = 16, REG_PAR1 = 0, REG_PAR2 = 1};
static uae_u32 vreg[32], guest[32], hreg[2];
static uae_u8 instructions[128], code[64];
static uae_u8 *comp_pc_p = instructions + 64, *start_pc_p = instructions;
static uae_u32 start_pc = 0x1000, m68k_pc_offset;
static int code_offset;
static void mov_l_ri(int d, uintptr value) { vreg[d] = value; }
static void mov_l_rr(int d, int s) { vreg[d] = vreg[s]; }
static void lea_l_brr(int d, int s, uintptr offset) { vreg[d] = vreg[s] + offset; }
static uae_u16 comp_get_iword(int offset) {
    return (comp_pc_p[offset] << 8) | comp_pc_p[offset + 1];
}
static uae_u32 comp_get_ilong(int offset) {
    return (uae_u32(comp_get_iword(offset)) << 16) | comp_get_iword(offset + 2);
}
static void calc_disp_ea_020(int base, uae_u16 ext, int d, int) {
    vreg[d] = vreg[base] + int8_t(ext);
}
static int get_fp_ad(uae_u32 opcode) { mov_l_rr(S1, (opcode & 7) + 8); return S1; }
""" + flags + "\nstatic jit_disable_opcodes jit_disable;\n" + disabled + "\n" + ea + r"""
enum {INMEM, DIRTY};
struct State { int status; uintptr val; };
struct { State state[32]; } live;
struct { uae_u32 instruction_pc, pc; uae_u16 opcode, ir; bool fp_exception; } regs;
static int flushed, disassociated;
static bool prepared, called, guarded;
static void clobber_flags() {}
static void remove_all_offsets() {}
static void flush_reg(int r) { guest[r] = vreg[r]; ++flushed; }
static int readreg_specific(int r, int, int hardware) { hreg[hardware] = vreg[r]; return hardware; }
static void prepare_for_call_1() { assert(flushed == 16); prepared = true; }
static void unlock2(int) {}
static void prepare_for_call_2() { assert(prepared); }
static bool isinreg(int r) { return live.state[r].status == DIRTY; }
static void disassociate(int r) { assert(isinreg(r)); ++disassociated; }
static void set_status(int r, int status) { live.state[r].status = status; }
static void raw_mov_l_ri(int r, uintptr value) { hreg[r] = value; }
static void raw_mov_l_mi(uintptr ad, uintptr value) { *reinterpret_cast<uae_u32 *>(ad) = value; }
static void raw_mov_w_mi(uintptr ad, uintptr value) { *reinterpret_cast<uae_u16 *>(ad) = value; }
static void raw_dec_sp(int) {}
static void raw_inc_sp(int) {}
static const int STACK_SHADOW_SPACE = 0;
static void fpuop_arithmetic_jit(uae_u32 packed, uaecptr ea) {
    assert(packed == 0xf2296000 && ea == 0x1234);
    assert(regs.instruction_pc == 0x1040 && regs.pc == 0x104a);
    assert(regs.opcode == 0xf229 && regs.ir == 0xf229);
    for (int i = 0; i < 16; ++i) {
        assert(live.state[i].status == INMEM && live.state[i].val == 0);
        assert(guest[i] == 0x100U + i);
    }
    assert(live.state[PC_P].status == INMEM && !live.state[PC_P].val);
    guest[0] = 0xdeadbeef;
    called = true;
}
static void compemu_raw_call(uintptr function) {
    assert(function == reinterpret_cast<uintptr>(fpuop_arithmetic_jit));
    fpuop_arithmetic_jit(hreg[0], hreg[1]);
}
static void raw_cmp_b_mi(uintptr ad, int value) {
    assert(called && ad == reinterpret_cast<uintptr>(&regs.fp_exception) && value == 0);
    guarded = true;
}
static void raw_jz_b_oponly() { assert(guarded); code[code_offset++] = 0x74; }
static uae_u8 *get_target() { return code + code_offset; }
static void skip_byte() { ++code_offset; }
static void *popall_do_nothing = nullptr;
static void compemu_raw_jmp(uintptr) { code_offset += 5; }
#define MIDFUNC(n, name, args) static void name args
#define USE_NORMAL_CALLING_CONVENTION 0
""" + call + r"""
int main()
{
    jit_disable.fmove = true;
    assert(fp_native_disabled(0x0400) && fp_native_disabled(0x0444));
    assert(fp_native_disabled(0x6000) && !fp_native_disabled(0x0422));
    jit_disable.fmove = false;
    jit_disable.fadd = true;
    assert(fp_native_disabled(0x0422) && fp_native_disabled(0x0462) && fp_native_disabled(0x0466));
    assert(!fp_native_disabled(0x0423));
    vreg[8] = 0x200;
    m68k_pc_offset = 4;
    assert(get_fp_native_ea(0xf210, 0x4800) == S1 && vreg[S1] == 0x200);
    assert(vreg[8] == 0x200 && m68k_pc_offset == 4);
    assert(get_fp_native_ea(0xf218, 0x4800) == S1 && vreg[S1] == 0x200);
    assert(vreg[8] == 0x200);
    assert(get_fp_native_ea(0xf220, 0x4800) == S1 && vreg[S1] == 0x1f4);
    assert(vreg[8] == 0x200);
    vreg[15] = 0x202;
    assert(get_fp_native_ea(0xf227, 0x5800) == S1 && vreg[S1] == 0x200);
    comp_pc_p[4] = 0xff; comp_pc_p[5] = 0xf0;
    m68k_pc_offset = 4;
    assert(get_fp_native_ea(0xf228, 0x4000) == S1 && vreg[S1] == 0x1f0);
    assert(m68k_pc_offset == 6);
    m68k_pc_offset = 4;
    assert(get_fp_native_ea(0xf23a, 0x4000) == S1 && vreg[S1] == 0x1034);
    assert(m68k_pc_offset == 6);
    m68k_pc_offset = 4;
    assert(get_fp_native_ea(0xf23c, 0x5800) == S1 && vreg[S1] == 0x1045);
    assert(m68k_pc_offset == 6);
    m68k_pc_offset = 4;
    assert(get_fp_native_ea(0xf23c, 0x4800) == S1 && vreg[S1] == 0x1044);
    assert(m68k_pc_offset == 16);
    assert(get_fp_native_ea(0xf23c, 0x6800) == -1);
    for (int i = 0; i <= PC_P; ++i) {
        live.state[i] = {DIRTY, 0x123};
        vreg[i] = 0x100 + i;
    }
    vreg[S1] = 0x1234;
    fpu_native_call(0xf2296000, S1, 0x1040, 0x104a);
    assert(flushed == 16 && disassociated == 17 && called && guarded);
    assert(code[0] == 0x74 && code[1] == 5 && guest[0] == 0xdeadbeef);
}
"""
        with tempfile.TemporaryDirectory(prefix=".fpu-jit-", dir=ROOT / "tests") as directory:
            directory = Path(directory)
            cpp = directory / "driver.cpp"
            binary = directory / "compiler"
            cpp.write_text(driver)
            subprocess.run(
                ["c++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
                 str(cpp), "-o", str(binary)],
                env=dict(os.environ, TMPDIR=str(directory)), check=True, timeout=30)
            subprocess.run([str(binary)], check=True, timeout=10)


if __name__ == "__main__":
    unittest.main()
