"""Execute production FPU EA emission and helper-call allocator sequencing."""

import os
from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


class FpuJitCompilerRegressions(unittest.TestCase):
    def test_explicit_fpsr_conditions_in_compiled_set_and_branch(self):
        source = (ROOT / "src/jit/x86/compemu_fpp_x86.cpp").read_text()

        def function(text, signature):
            start = text.index(signature)
            return text[start:text.index("\n}", start) + 2]

        predicates = "\n".join(function(source, signature) for signature in (
            "static void get_fp_condition(", "void comp_fscc_opp(", "void comp_fbcc_opp("))
        source = (ROOT / "src/fpp.cpp").read_text()
        tables = source[source.index("static const bool condition_table_040_060"):
                        source.index("int fpp_cond(")]
        source = (ROOT / "src/jit/x86/compemu_midfunc_x86.cpp").read_text()
        override = function(source, "MIDFUNC(3,fpu_cond_from_fpsr,")
        driver = r"""
#include <cassert>
#include <cstdint>
#include <vector>
using uae_u8 = uint8_t;
using uae_u16 = uint16_t;
using uae_u32 = uint32_t;
using uae_s16 = int16_t;
using uae_s32 = int32_t;
using uintptr = uintptr_t;
using IMM = uintptr;
using RW4 = int;
using W4 = int;
enum {S1 = 20, S2, S3, S4, PC_P, TRASH = 0, VALID = 1};
struct { int flags_in_flags, flags_on_stack; } live;
struct { uae_u32 fpsr; } regs;
struct { bool compfpu = true; } currprefs;
struct { bool fscc, fbcc; } jit_disable;
static bool fpu_fpsr_cc_written, failure;
static uintptr constants[32], vreg[32], not_taken, taken;
static uae_u32 m68k_pc_offset;
static uae_u8 *comp_pc_p = reinterpret_cast<uae_u8 *>(uintptr(0x123450000000));
static int displacement, branch_cc, represented_control, flags;
struct Op { int kind, d, s; uintptr value; };
enum {MOVI, MOVR, ADDI, CMOV, FPFLAGS, MOVB, CMP, JZ, LOAD, SHR, AND, BT, SBB, TEST};
static Op ops[128];
static uae_u8 code[128];
static int length;
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
        case CMP: flags = !fpu_fpsr_cc_written ? 64 : 0; break;
        case JZ: if (flags & 64) pc += 1 + code[pc + 1]; else ++pc; break;
        case LOAD: vreg[o.d] = regs.fpsr; break;
        case SHR: vreg[o.d] >>= o.value; break;
        case AND: vreg[o.d] &= o.value; break;
        case BT: flags = (vreg[o.d] >> (vreg[o.s] & 31)) & 1; break;
        case SBB: vreg[o.d] = (flags & 1) ? uae_u32(-1) : 0; break;
        case TEST: flags = vreg[o.d] ? 0 : 64; break;
        default: assert(false);
        }
    }
}
static void mov_l_ri(int d, uintptr value) { constants[d] = value; emit(MOVI, d, 0, value); }
static void mov_l_rr(int d, int s) { emit(MOVR, d, s); }
static void add_l_ri(int d, uintptr value) { constants[d] += value; emit(ADDI, d, 0, value); }
static void mov_b_rr(int d, int s) { emit(MOVB, d, s); }
static void cmov_l_rr(int d, int s, int condition) { emit(CMOV, d, s, condition); }
static void fflags_into_flags(int) {
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
static int rmw(int r, int, int) {
    assert(live.flags_in_flags == TRASH && live.flags_on_stack == VALID);
    return r;
}
static int writereg(int r, int) { return r; }
static void unlock2(int) {}
static void raw_cmp_b_mi(uintptr, int) { emit(CMP); }
static void raw_jz_b_oponly() { emit(JZ); }
static uae_u8 *get_target() { return code + length; }
static void skip_byte() { ++length; }
static void raw_mov_l_rm(int d, uintptr) { emit(LOAD, d); }
static void raw_shrl_l_ri(int d, int i) { emit(SHR, d, 0, i); }
static void raw_and_l_ri(int d, int i) { emit(AND, d, 0, i); }
static void raw_mov_l_ri(int d, uintptr i) { emit(MOVI, d, 0, i); }
static void raw_bt_l_rr(int d, int s) { emit(BT, d, s); }
static void raw_sbb_l(int d, int s) { assert(d == s); emit(SBB, d); }
#define MIDFUNC(n, name, args) static void name args
#define FAIL(x) (failure = true)
#define JITPTR (uintptr)
""" + tables + override + predicates + r"""
int main() {
    for (const bool *table : {condition_table_6888x, condition_table_040_060}) {
        condition_table = table;
        for (bool branch : {false, true}) for (bool long_branch : {false, true})
        for (int condition = 0; condition < 16; ++condition)
        for (int offset : {-100, 100}) {
            length = 0; failure = false; m68k_pc_offset = 2; displacement = offset;
            if (branch)
                comp_fbcc_opp(0xf280 | condition | (long_branch ? 0x40 : 0));
            else
                comp_fscc_opp(0xf243, condition);
            assert(!failure);
            assert(live.flags_in_flags == TRASH && live.flags_on_stack == VALID);
            if (branch) {
                assert(not_taken == uintptr(comp_pc_p) + (long_branch ? 6 : 4));
                assert(taken == uintptr(comp_pc_p) + 2 + offset);
                assert(branch_cc == 5);
            }
            // Reuse the emitted block after explicit FPSR writes and arithmetic.
            for (bool written : {false, true}) for (int control = 0; control < 16; ++control) {
                if (!written && (control == 3 || control == 5 || control == 6 ||
                    control == 7 || control == 11 || control >= 13))
                    continue;
                fpu_fpsr_cc_written = written;
                regs.fpsr = uae_u32(control) << 24;
                represented_control = (control & 1) ? 1 :
                    (control & 4) ? (control & 12) : control;
                vreg[3] = 0x12345678;
                execute();
                bool expected = table[control * 32 + condition];
                if (branch)
                    assert(cc(branch_cc) == expected);
                else
                    assert(vreg[3] == (expected ? 0x123456ffU : 0x12345600U));
                assert(regs.fpsr == uae_u32(control) << 24);
            }
        }
    }
    for (int condition = 16; condition < 32; ++condition) {
        length = 0; failure = false;
        comp_fscc_opp(0xf240, condition);
        assert(failure && !length);
        failure = false;
        comp_fbcc_opp(0xf280 | condition);
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
        source = (ROOT / "src/jit/x86/compemu_fpp_x86.cpp").read_text()
        start = source.index("static int get_fp_native_ea(")
        ea = source[start:source.index("\n}", start) + 2]
        start = source.index("static bool fp_native_disabled(")
        disabled = source[start:source.index("\n}", start) + 2]
        header = (ROOT / "src/jit/x86/compemu_x86.h").read_text()
        start = header.index("struct jit_disable_opcodes {")
        flags = header[start:header.index("\n};", start) + 3]
        source = (ROOT / "src/jit/x86/compemu_midfunc_x86.cpp").read_text()
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
