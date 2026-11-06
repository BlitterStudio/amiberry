"""Execute production FPU frame selection and format-9 stack construction."""

import os
from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


class FpuFrameRegressions(unittest.TestCase):
    def test_external_fpu_mid_instruction_frame(self):
        source = (ROOT / "src/newcpu_common.cpp").read_text()
        start = source.index("void Exception_build_stack_frame_common(")
        common = source[start:source.index("\n}", start) + 2]
        start = source.index("\tcase 0x9: // coprocessor mid-instruction")
        frame = source[start:source.index("\n\t\tbreak;", start)]
        frame = frame[frame.index("\n") + 1:]
        tail_start = source.index(
            "\tm68k_areg(regs, 7) -= 2;\n\tx_put_word(m68k_areg(regs, 7), (format << 12)")
        tail = source[tail_start:source.index("\n}", tail_start)]
        driver = r"""
#include <cassert>
#include <cstdint>
#include <initializer_list>
using uae_u32 = uint32_t;
#define m68k_areg(r, n) ((r).a[n])
struct {
    bool fpu_exp_pre, fp_unimp_ins;
    uae_u32 a[8], sr, instruction_pc, trace_pc, fp_ea, fp_opword;
    uae_u32 mmu_ssw, pcr, fp_unimp_pend;
} regs;
struct { int cpu_model, fpu_model; } currprefs;
static uint16_t memory[256];
static int selected_format, selected_vector;
static uae_u32 selected_instruction, selected_pc;
static void x_put_word(uae_u32 ad, uae_u32 value) { assert(ad < 512); memory[ad / 2] = value; }
static void x_put_long(uae_u32 ad, uae_u32 value) { x_put_word(ad, value >> 16); x_put_word(ad + 2, value); }
static uae_u32 get_long(unsigned index) { return (uae_u32(memory[index]) << 16) | memory[index + 1]; }
static void build_format9(uae_u32 oldpc, uae_u32 currpc, int nr)
{
    const int format = 9;
""" + frame + tail + r"""
}
static void Exception_build_stack_frame(uae_u32 oldpc, uae_u32 pc, uae_u32, int nr, int format)
{
    selected_instruction = oldpc;
    selected_pc = pc;
    selected_format = format;
    selected_vector = nr;
    if (format == 9) build_format9(oldpc, pc, nr);
}
""" + common + r"""
int main()
{
    for (int cpu : {68020, 68030, 68040, 68060}) {
        currprefs.cpu_model = cpu;
        currprefs.fpu_model = cpu < 68040 ? 68882 : cpu;
        for (bool pre : {false, true}) {
            for (int nr = 48; nr <= 55; ++nr) {
                regs = {};
                regs.fpu_exp_pre = pre;
                regs.a[7] = 512;
                regs.sr = 0x200b;
                regs.instruction_pc = 0x100;
                regs.fp_opword = 0xf200;
                regs.fp_ea = 0x12345678;
                Exception_build_stack_frame_common(0xdeadbeef, 0x10a, 0, nr, nr);
                assert(selected_vector == nr);
                assert(selected_format == (pre ? 0 : cpu < 68040 ? 9 : 3));
                assert(selected_pc == (pre ? 0x100 : 0x10a));
                if (!pre && cpu < 68040) {
                    assert(selected_instruction == 0x100 && regs.a[7] == 492);
                    const unsigned sp = regs.a[7] / 2;
                    assert(memory[sp] == 0x200b && get_long(sp + 1) == 0x10a);
                    assert(memory[sp + 3] == (0x9000 | nr * 4));
                    assert(get_long(sp + 4) == 0x100);
                    assert(memory[sp + 6] == 0 && memory[sp + 7] == 0xf200);
                    assert(get_long(sp + 8) == 0x12345678);
                }
            }
        }
    }
}
"""
        with tempfile.TemporaryDirectory(prefix=".fpu-frames-", dir=ROOT / "tests") as directory:
            directory = Path(directory)
            cpp = directory / "driver.cpp"
            binary = directory / "frames"
            cpp.write_text(driver)
            subprocess.run(
                ["c++", "-std=c++17", "-Wall", "-Wextra", "-Werror", "-Wno-unused-parameter",
                 str(cpp), "-o", str(binary)],
                env=dict(os.environ, TMPDIR=str(directory)), check=True, timeout=30)
            subprocess.run([str(binary)], check=True, timeout=10)


if __name__ == "__main__":
    unittest.main()
