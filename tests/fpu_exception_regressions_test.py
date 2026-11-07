"""Execute production FPU exception/branch/frame handlers without a guest."""

import os
from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


class FpuExceptionRegressions(unittest.TestCase):
    def test_prefetch_abort_and_fnop_fpiar(self):
        source = (ROOT / "src/fpp.cpp").read_text()

        def function(signature):
            start = source.index(signature)
            return source[start:source.index("\n}", start) + 2]

        production = "\n".join(function(signature) for signature in (
            "static bool fp_exception_pending(bool pre)",
            "static void fp_unimp_instruction_exception_pending(",
            "void fpuop_trapcc (", "void fpuop_bcc(",
            "void fpuop_restore (",
        ))
        driver = r"""
#include <cassert>
#include <cstdint>
using uae_u16 = uint16_t;
using uae_u32 = uint32_t;
using uae_s16 = int16_t;
using uae_s32 = int32_t;
using uaecptr = uint32_t;
#define write_log(...) ((void)0)
#define _T(x) x
#define M68K_GETPC pc
#define m68k_areg(r, n) ((r).a[n])
struct fpdata { int value; };
struct {
    int fpu_model = 68882, cpu_model = 68030;
    bool fpu_no_unimplemented = true;
} currprefs;
struct {
    bool fp_exception, fpu_exp_pre, fp_unimp_ins, fp_branch, fp_ea_set;
    int fp_exp_pend, fp_unimp_pend, fpu_state, fpu_exp_state;
    uae_u32 instruction_pc, fpiar, fpsr, fpcr, fp_ea, fp_opword, a[8];
    fpdata fp[8];
} regs;
struct { int reg; uae_u32 value; } mmufixup[1];
struct { uae_u32 eo[3], et[3], fpt[3], cmdreg1b, ccr; } fsave_data;
static bool support_exceptions = true, fpu_mmu_fixup;
static int warned, vector, condition;
static uae_u32 pc = 0x200, frame;
static void Exception(int v) { assert(regs.fp_exception); vector = v; }
static void Exception_cpu_oldpc(int v, uaecptr) { Exception(v); }
static bool fault_if_no_fpu_u(uae_u32, uae_u16, uaecptr, bool, uaecptr) { return false; }
static bool fault_if_no_fpu(uae_u32, uae_u16, uaecptr, bool, uaecptr) { return false; }
static bool fault_if_no_6888x(uae_u32, uae_u16, uaecptr) { return false; }
static void maybe_idle_state() {}
static int fpp_cond(int) { return condition; }
static void fpu_op_illg(uae_u32, uaecptr, bool, uaecptr) { assert(false); }
static void m68k_setpc(uaecptr p) { pc = p; }
static uaecptr m68k_getpc() { return pc; }
static int get_fp_ad(uae_u32, uaecptr *ad, bool *) { *ad = 0x100; return 1; }
static uae_u32 x_cp_get_long(uaecptr ad) { return ad == 0x100 ? frame : 0; }
static int get_fpu_version(int model) { return model == 68040 ? 0x40 : 0x1f; }
static void fpu_null() { regs.fpu_state = 0; }
static void fpp_to_exten_fmovem(fpdata *, uae_u32, uae_u32, uae_u32) {}
static void fpp_denormalize(fpdata *, uae_u32) {}
static void fpsr_clear_status() {}
static bool fp_arithmetic(fpdata *, fpdata *, int) { return false; }
static bool fpsr_check_arithmetic_exception(
    uae_u32, fpdata *, uae_u32, uae_u16, uae_u32, bool, uaecptr) { return false; }
static int fpsr_get_vector(uae_u32) { return 0; }
// Cycle-exact FPU timing does not take part in these checks.
static void fpu_timing_begin() {}
static void fpu_timing_cond(int, int) {}
static void fpu_timing_save_restore(bool, bool) {}
"""
        assertions = r"""
int main()
{
    for (int model : {68882, 68040, 68060}) {
        currprefs.fpu_model = model;
        currprefs.cpu_model = model == 68882 ? 68030 : model;
        for (int pending : {48, 49, 50, 51, 52, 53, 54}) {
            regs = {};
            vector = 0;
            regs.fp_exp_pend = pending;
            assert(fp_exception_pending(true));
            assert(vector == pending && regs.fp_exception && regs.fpu_exp_pre);
            assert(regs.fp_exp_pend == (model == 68882 ? pending : 0));
        }
        regs = {};
        regs.fp_unimp_pend = 1;
        assert(fp_exception_pending(false));
        assert(vector == 55 && regs.fp_exception && !regs.fpu_exp_pre);
        assert(!regs.fp_unimp_pend);
        regs = {};
        regs.fp_unimp_ins = true;
        fp_unimp_instruction_exception_pending();
        assert(vector == 11 && regs.fp_exception && !regs.fp_unimp_ins);
        regs = {};
        condition = 1;
        fpuop_trapcc(0xf27c, 0x200, 1);
        assert(vector == 7 && regs.fp_exception);
        regs = {};
        vector = condition = 0;
        fpuop_trapcc(0xf27c, 0x200, 0);
        assert(!vector && !regs.fp_exception);
    }
    struct BadFrame { int model; uae_u32 frame; };
    for (BadFrame bad : {BadFrame{68882, 0x99000000},
                         BadFrame{68882, 0x1f010000},
                         BadFrame{68040, 0x99000000},
                         BadFrame{68040, 0x40010000},
                         BadFrame{68060, 0x00000100}}) {
        currprefs.fpu_model = bad.model;
        regs = {};
        vector = 0;
        frame = bad.frame;
        fpuop_restore(0xf350);
        assert(vector == 14 && regs.fp_exception);
    }
    currprefs.fpu_model = 68060;
    regs = {};
    condition = 0;
    regs.fpiar = 0xc0ffee;
    fpuop_bcc(0xf280, 0x202, 0);
    assert(regs.fpiar == 0xc0ffee && !regs.fp_exception && !regs.fp_branch);
    regs.fp_exp_pend = 52;
    fpuop_bcc(0xf280, 0x202, 0);
    assert(vector == 52 && regs.fp_exception && regs.fpiar == 0xc0ffee);
}
"""
        with tempfile.TemporaryDirectory(prefix=".fpu-exceptions-", dir=ROOT / "tests") as directory:
            directory = Path(directory)
            cpp = directory / "driver.cpp"
            binary = directory / "exceptions"
            cpp.write_text("#include <initializer_list>\n" + driver + production + assertions)
            subprocess.run(
                ["c++", "-std=c++17", "-Wall", "-Wextra", "-Werror", "-Wno-unused-variable",
                 str(cpp), "-o", str(binary)],
                env=dict(os.environ, TMPDIR=str(directory)), check=True, timeout=30)
            subprocess.run([str(binary)], check=True, timeout=10)


if __name__ == "__main__":
    unittest.main()
