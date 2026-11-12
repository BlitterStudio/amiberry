"""ROM-free regression checks of CPU, chipset, memory and RTC production paths."""

import os
from pathlib import Path
import platform
import re
import shlex
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
HEADER = r"""
#include <cassert>
#include <cstdint>
#include <ctime>
#include <vector>
#include <atomic>
#include <thread>
#include <cstring>
using uae_u8 = uint8_t;
using uae_s8 = int8_t;
using uae_u16 = uint16_t;
using uae_s16 = int16_t;
using uae_u32 = uint32_t;
using uae_s32 = int32_t;
using uae_u64 = uint64_t;
using uae_s64 = int64_t;
using uaecptr = uint32_t;
"""


def source(path):
    return (ROOT / path).read_text(encoding="utf-8")


def function(text, signature):
    start = text.index(signature)
    while ";" in text[start:text.index("{", start)]:
        start = text.index(signature, start + len(signature))
    return text[start:text.index("\n}", start) + 2]


def served_credit(text):
    """The progress credit of served requests (newcpu.cpp): what
    service_cpu_indirect_request() adds to and the chipset loop takes."""
    insn = re.search(r"^#define CPU_THREAD_INSN_CREDIT\b.*$", text, re.M)[0]
    start = text.index("static uae_s64 cpu_thread_served_credit;")
    return ("\n" + insn + "\n" + text[start:text.index("\n", start) + 1] +
            function(text, "static inline uae_s64 cpu_thread_take_served_credit("))


def wake_main(text):
    """cpu_thread_wake_main() and the nap state it signals (newcpu.cpp)."""
    start = text.index("static std::mutex cpu_thread_nap_lock;")
    end = text.index("static void cpu_thread_wake_main();", start)
    return ("\n#ifndef NOINLINE\n#define NOINLINE __attribute__((noinline))\n#endif\n" + text[start:end] +
            function(text, "static NOINLINE void cpu_thread_wake_main("))


def run_cpp(code):
    with tempfile.TemporaryDirectory(prefix="amiberry-regression-") as directory:
        cpp = Path(directory) / "test.cpp"
        binary = Path(directory) / "test"
        cpp.write_text(HEADER + code, encoding="utf-8")
        command = shlex.split(os.environ.get("CXX", "c++"))
        subprocess.run(
            command + ["-std=c++17", "-O2", "-pthread", str(cpp), "-o", str(binary)],
            check=True,
        )
        subprocess.run([str(binary)], check=True, timeout=30)


class EmulatorRegressions(unittest.TestCase):
    def test_aga_fine_scroll_and_sprite_comparisons_keep_hardware_positions(self):
        text = source("src/drawing.cpp")
        run_cpp(r"""
constexpr int RES_SUPERHIRES = 2;
bool aga_mode = true, ecs_denise;
struct { int collision_level = 3; } currprefs;
int hresolution, denise_res, denise_spr_nr_armed = 1, denise_spr_nr_armeds;
int bplcon1_denise, bplcon1_shift[2], bplcon1_shift_full[2];
int bplcon1_shift_full_masked[2], bplcon1_shift_mask, bplcon1_shift_mask_full;
int fetchmode_mask_denise = 15, reswitch_unalign;
bool lts_request;
int denise_xposmask = 0x7fe, matches;
struct denise_spr {
    bool armed, shiftercopydone;
    int xpos;
    uae_u64 dataas64, databs64, dataa64, datab64;
} sprite;
denise_spr *dprspt[2] = {&sprite, nullptr};
void spr_arms(denise_spr *, int state) { assert(state == 1); matches++; }
""" + function(text, "static bool aga_subpixel_collisions(") +
                function(text, "static void update_bplcon1(") +
                function(text, "static void matchsprites2_aga(") + r"""
int main() {
    for (hresolution = 0; hresolution <= 2; ++hresolution)
        for (int fine = 0; fine < 4; ++fine) {
            bplcon1_denise = fine << 8;
            update_bplcon1();
            assert(bplcon1_shift_full[0] == fine);
            assert(bplcon1_shift_full_masked[0] == (fine & ~(3 >> hresolution)));
            denise_xposmask = 0x7fc | (3 & ~(3 >> hresolution));
            sprite = {true, false, 80 + fine, 0, 0, 1, 0};
            matches = 0;
            for (int cnt = 80; cnt < 84; ++cnt) {
                matchsprites2_aga(cnt);
                assert(matches == (cnt >= 80 + fine ? 1 : 0));
            }
        }
}
""")

    def test_aga_collision_sampling_precedes_host_decimation(self):
        text = source("src/drawing.cpp")
        run_cpp(r"""
#define RES_SUPERHIRES 2
#define RES_MAX 2
#define BLANK_COLOR 0
bool aga_mode = true;
struct { int collision_level = 3, gfx_lores_mode = 0; } currprefs;
int hresolution, hresolution_inv, hresolution_add;
int denise_res, denise_res_size, denise_spr_nr_armed = 1, denise_spr_nr_armeds = 1;
int bplcon1_denise;
bool denise_odd_even, denise_bplfmode64;
int bplshiftcnt[2], bplcon1_shift_full[2], bplcon1_shift_mask_full = 63;
bool bpldat_copy[2];
int aga_unalign0, aga_unalign1, denise_hcounter, denise_hcounter_cmp;
int denise_hcounter_next, internal_pixel_cnt, denise_pixtotal;
int denise_hbstrt_lores = -1, denise_hbstop_lores = -1;
int denise_hstrt = -1, denise_hstop = -1, denise_phbstrt = -1, denise_phbstop = -1;
int denise_strlong_hd = -1, denise_brdstop = -1, denise_brdstrt = -1;
bool exthblankon_aga, denise_blank_active, sprites_hidden, sprites_hidden2;
bool denise_hdiw = true, bpl1dat_trigger = true;
uae_u32 bordercolor, fullblack, last_bpl_pix, clxdat, bplcoltable[256];
uae_u8 loaded_pix, loaded_pixs[4];
struct { uae_u32 acolors[256]; } denise_colors;
uae_u32 dtbuf[2][4], dtgbuf[2][4], output1[8], output2[8];
uae_u32 *buf1 = output1, *buf2 = output2, *denise_pixtotal_max_buf_1x = output1 + 8;
uae_u8 *gbuf;
int lol, samples, sprite_samples, shifts;
void do_hbstrt(int) {} void do_hbstop(int) {}
void do_aga_unaligned0() {} void do_aga_unaligned1(int) {}
void do_hstrt_aga(int) {} void do_hstop_aga(int) {}
void do_phbstrt_aga(int) {} void do_phbstop_aga(int) {} void copy_strlong() {}
void matchsprites_aga(int cnt) { assert(cnt == 80 + samples++); }
uae_u8 getbpl_aga() { return 1; }
void shiftbpl8_64() { shifts++; } void shiftbpl8_32() { shifts++; }
void shiftbpl8e_64() {} void shiftbpl8e_32() {}
void shiftbpl8o_64() {} void shiftbpl8o_32() {}
void copybpl8_64() {} void copybpl8e_64() {} void copybpl8o_64() {}
uae_u32 decode_pixel_aga(uae_u8 p) { return p; }
uae_u32 denise_render_sprites_aga(int add) {
    assert(add == 1); sprite_samples++; return 0x10001;
}
uae_u8 denise_render_sprites2(uae_u8 p, uae_u32) { return p; }
uae_u32 filter_pixel(uae_u32 p, uae_u32) { return p; }
""" + function(text, "static bool aga_subpixel_collisions(") +
                function(text, "static void lts_unaligned_aga(int cnt,") + r"""
int main() {
    for (hresolution = 0; hresolution <= 2; ++hresolution)
        for (denise_res = 0; denise_res <= 2; ++denise_res) {
            hresolution_inv = 2 - hresolution;
            hresolution_add = 1 << hresolution;
            denise_res_size = 1 << denise_res;
            bplshiftcnt[0] = bplshiftcnt[1] = 0;
            samples = sprite_samples = shifts = internal_pixel_cnt = 0;
            buf1 = output1; buf2 = output2;
            lts_unaligned_aga(20, 21, 0);
            assert(samples == 4 && sprite_samples == 4);
            assert(shifts == (1 << denise_res));
            assert(internal_pixel_cnt == 4);
            assert(buf1 - output1 == (1 << hresolution));
        }
    currprefs.collision_level = 0;
    hresolution = 0;
    assert(!aga_subpixel_collisions());
    currprefs.collision_level = 3;
    denise_spr_nr_armed = 0;
    denise_spr_nr_armeds = 0;
    denise_res = 0;
    assert(!aga_subpixel_collisions());
    bplcon1_denise = 0x0100;
    assert(aga_subpixel_collisions());
    aga_mode = false;
    assert(!aga_subpixel_collisions());
}
""")

    def test_ecs_shres_only_serializes_available_sprite_channels(self):
        run_cpp(r"""
constexpr int MAX_SPRITES = 8;
struct denise_spr { int num, pix, shift; uae_u32 dataas, databs; } dspr[MAX_SPRITES];
int denise_spr_shiftsize = 4;
uae_u32 disarmed;
void spr_disarms(uae_u32 mask) { disarmed |= mask; }
""" + function(source("src/drawing.cpp"), "static uae_u32 denise_render_sprites_ecs_shres(") + r"""
int main() {
    for (int channel = 0; channel < MAX_SPRITES; ++channel) {
        for (int i = 0; i < MAX_SPRITES; ++i)
            dspr[i] = {i, i == channel ? 1 : 0, 4, 0, 0};
        disarmed = 0;
        uae_u32 expected = channel < 4 ?
            (1u << (channel * 2)) | (1u << (16 + channel)) : 0;
        assert(denise_render_sprites_ecs_shres() == expected);
        assert(disarmed == 0);
        assert(denise_render_sprites_ecs_shres() == expected);
        assert(disarmed == 0);
        assert(denise_render_sprites_ecs_shres() == 0);
    }
}
""")

    def test_sr_lowering_checks_pending_irq_before_next_instruction(self):
        run_cpp(r"""
#define JIT
#define SET_XFLG(x) ((void)(x))
#define SET_NFLG(x) ((void)(x))
#define SET_ZFLG(x) ((void)(x))
#define SET_VFLG(x) ((void)(x))
#define SET_CFLG(x) ((void)(x))
#define m68k_areg(r, n) ((r).areg[n])
constexpr int SPCFLAG_INT = 1, SPCFLAG_DOINT = 2;
constexpr int SPCFLAG_END_COMPILE = 4, SPCFLAG_TRACE = 8;
struct {
    int m, s, t0, t1, intmask, ipl[2], ipl_pin, sr, spcflags;
    uae_u32 msp, isp, usp, areg[8];
    bool stopped;
} regs;
struct { int cachesize, cpu_model, mmu_model; } currprefs;
bool m68k_accurate_ipl;
void set_special(int f) { regs.spcflags |= f; }
void unset_special(int f) { regs.spcflags &= ~f; }
void unset_special_cpu(int f) { regs.spcflags &= ~f; }
void mmu_set_super(bool) {}
void activate_trace() {}
""" + function(source("src/newcpu.cpp"), "static void MakeFromSR_x(") + r"""
int main() {
    for (int cache : {0, 8192}) {
        currprefs = {cache, 68020, 0};
        for (int level = 1; level <= 6; ++level) {
            regs = {};
            regs.s = 1;
            regs.intmask = 7;
            regs.ipl_pin = level;
            regs.sr = 0x2000 | ((level - 1) << 8);
            MakeFromSR_x(0);
            assert(regs.intmask == level - 1);
            assert(regs.spcflags & SPCFLAG_INT);
            assert(!(regs.spcflags & SPCFLAG_DOINT));
            regs.spcflags = 0;
            regs.sr = 0x2700;
            MakeFromSR_x(0);
            assert(!(regs.spcflags & (SPCFLAG_INT | SPCFLAG_DOINT)));
        }
        regs = {};
        regs.s = 1;
        regs.intmask = 7;
        regs.ipl_pin = 3;
        regs.ipl[0] = 3;
        regs.sr = 0x2000;
        m68k_accurate_ipl = true;
        MakeFromSR_x(-1);
        assert(regs.ipl[0] == 0);
        assert(!(regs.spcflags & (SPCFLAG_INT | SPCFLAG_DOINT)));
        m68k_accurate_ipl = false;
    }
}
""")

    def test_custom_aga_write_width_and_full_queue_fallback(self):
        text = source("src/custom.cpp")
        put = function(text, "static int REGPARAM2 custom_wput_1(")
        start = put.index("\tif (!(noget & 0x8000)")
        end = put.index("\n\treturn ret;", start)
        run_cpp(r"""
struct { uae_u16 chipset_latch_rw = 0xabcd; } regs;
struct { bool cpu_memory_cycle_exact; } currprefs;
bool aga_mode = true, full;
int custom_fastmode, fetchmode_fmode_bpl, fetchmode_fmode_spr;
uae_u32 rga_denise_cycle_line = 42;
uae_u64 queued, fallback;
bool denise_update_reg_queued(uae_u16, uae_u64 value, uae_u32) {
    queued = value; return !full;
}
void denise_update_reg_queue(uae_u16, uae_u64 value, uae_u32) { fallback = value; }
void custom_wput_dma64(int, uaecptr, uae_u32, int) { assert(false); }
void write_drga(int, uaecptr, uae_u32) { assert(false); }
""" + function(text, "static uae_u64 wordto64convert(") + r"""
void submit(uaecptr addr, uae_u32 value, int c, int noget) {
""" + put[start:end] + r"""
}
int main() {
    for (int mode = 0; mode <= 3; ++mode) for (bool queue_full : {false, true}) {
        fetchmode_fmode_bpl = fetchmode_fmode_spr = mode;
        full = queue_full;
        for (int type : {0x12, 0x22}) {
            uae_u64 expected = mode == 0 ? 0x1234000012340000 :
                               mode == 1 ? 0x1234123412341234 : 0xabcd123412341234;
            queued = fallback = 0;
            submit(type == 0x12 ? 0x110 : 0x144, 0x1234, type, 0);
            assert(queued == expected);
            assert(fallback == (queue_full ? expected : 0));
        }
    }
    aga_mode = false;
    full = false;
    submit(0x144, 0x1234, 0x22, 0);
    assert(queued == 0x1234);
}
""")

    def test_denise_fast_queue_order_width_and_publication(self):
        text = source("src/drawing.cpp")
        registers = source("src/include/custom.h")
        start = registers.index("struct denise_rga\n")
        end = registers.index("\n};", start) + 3
        queue_start = text.index("struct denise_rga_queue\n")
        queue_end = text.index("\n};", queue_start) + 3
        indices = text.index("static std::atomic<int> rga_denise_fast_read")
        indices_end = text.index(";", text.index("static struct denise_rga rga_denise_fast", indices)) + 1
        run_cpp(r"""
using nln_how = int;
struct linestate;
""" + registers[start:end] + "\n" + text[queue_start:queue_end] + "\n" +
                text[indices:indices_end] + r"""
bool aga_mode = true, threaded;
int spr_unalign_reg[2];
uae_u16 spr_unalign_val[2];
uae_u64 spr_unalign_val64[2];
int aga_unalign0, aga_unalign1, reswitch_unalign;
int vpos, linear_vpos_start, linear_vpos_vb_start, linear_vpos_vb_end;
constexpr int DENISE_RGA_SLOT_CHUNKS_MASK = 7;
int rga_queue_write;
denise_rga_queue rga_queue[8];
std::vector<uae_u64> applied;
uae_u64 sprite;
void sprwrite(int, uae_u16 v) { sprite = v; }
void sprwrite_64(int, uae_u64 v) { sprite = v; }
void expand_drga_early2x(denise_rga *r) {
    if (aga_mode && r->rga == 0x144) {
        spr_unalign_reg[1] = r->rga;
        spr_unalign_val64[1] = r->v64;
    }
}
void expand_drga_early(denise_rga *r) {
    if (r->rga == 0x140) {
        spr_unalign_reg[0] = r->rga;
        spr_unalign_val[0] = r->v;
    }
}
void expand_drga(denise_rga *r) {
    assert(spr_unalign_reg[0] == 0 && spr_unalign_reg[1] == 0);
    if (r->rga == 0x144) assert(sprite == r->v64);
    applied.push_back(r->v64);
}
void update_bplcon1() {}
void do_aga_unaligned0() {}
void do_aga_unaligned1(int) {}
void do_ecs_unaligned1() {}
void do_ecs_unaligned() {}
void check_lts_request() {}
bool multithread_denise_active() { return threaded; }
bool waitqueue_nolock() { return true; }
void addtowritequeue() {}
void updatelinedata(int) {}
""" + "\n".join(function(text, signature) for signature in (
            "static void flush_sprite_writes(", "static void flush_fast_rga(",
            "static void denise_update_reg(", "bool denise_update_reg_queued(",
            "void denise_update_reg_queue(")) + r"""
int main() {
    const uae_u64 wide = 0x1122334455667788;
    for (int i = 0; i < DENISE_RGA_SLOT_FAST_TOTAL - 1; ++i)
        assert(denise_update_reg_queued(0x144, wide + i, 42));
    assert(!denise_update_reg_queued(0x144, wide, 42));
    flush_fast_rga(42);
    assert(applied.empty());
    flush_fast_rga(43);
    assert(applied.size() == DENISE_RGA_SLOT_FAST_TOTAL - 1);
    for (unsigned i = 0; i < applied.size(); ++i) assert(applied[i] == wide + i);
    applied.clear();
    assert(denise_update_reg_queued(0x140, 123, 43));
    denise_update_reg(0x144, wide, 44);
    assert(applied.size() == 2 && applied[0] == 123 && applied[1] == wide);
    threaded = true;
    denise_update_reg_queue(0x144, wide, 45);
    assert(rga_queue[0].val == wide);
    threaded = false;
    denise_update_reg_queue(0x144, wide, 45);
    assert(applied.back() == wide);
    applied.clear();
    constexpr unsigned count = 100000;
    std::thread producer([] {
        for (unsigned i = 0; i < count; ++i) {
            uae_u64 value = (uae_u64(i) << 32) | i;
            while (!denise_update_reg_queued(0x144, value, i)) std::this_thread::yield();
        }
    });
    while (applied.size() < count) {
        flush_fast_rga(0x7fffffff);
        std::this_thread::yield();
    }
    producer.join();
    for (unsigned i = 0; i < count; ++i)
        assert(applied[i] == ((uae_u64(i) << 32) | i));
}
""")

    def test_mmu030_exception_instruction_address(self):
        run_cpp(r"""
struct {
    uae_u32 instruction_pc = 0x1000, trace_pc = 0x9000, mmu_ssw;
    uae_u32 usp, msp, isp, vbr, mmu_fault_addr, fp_ea, pcr, a[8];
    bool s = true, m = false, fpu_exp_pre, fp_unimp_ins;
    int fp_unimp_pend, intmask, ir;
} regs;
struct { int cpu_model = 68030, fpu_model = 68882; } currprefs;
constexpr int MMU030_STATEFLAG1_LASTWRITE = 1, MMU030_SSW_RW = 2;
constexpr int MMU030_SSW_SIZE_W = 4, CPU_HALT_DOUBLE_FAULT = 1;
uae_u32 pc = 0x1002, mmu030_state[2], mmu030_data_buffer_out;
uae_u32 last_fault_for_exception_3 = 0x1234, frame_address, frame_pc;
int frame_format, mmu030_opcode_stageb = 42;
#define m68k_areg(r, n) ((r).a[n])
uae_u32 m68k_getpc() { return pc; }
void m68k_setpci(uae_u32 value) { pc = value; }
int iack_cycle(int nr) { return nr; }
void exception_debug(int) {}
void MakeSR() {}
void mmu_set_super(bool) {}
uae_u32 x_get_long(uae_u32) { return 0x2000; }
void cpu_halt(int) { assert(false); }
void exception3_read_special(int, uaecptr, int, int) { assert(false); }
void fill_prefetch() {}
void exception_check_trace(int) {}
void Exception_build_stack_frame(uae_u32 addr, uae_u32 resume, uae_u32, int, int format) {
    frame_address = addr; frame_pc = resume; frame_format = format;
}
""" + function(source("src/newcpu_common.cpp"),
               "void Exception_build_stack_frame_common(") + "\n" +
                function(source("src/newcpu.cpp"), "static void Exception_mmu030 (") + r"""
int main() {
    for (int nr : {5, 6, 7}) {
        pc = 0x1002;
        Exception_mmu030(nr);
        assert(frame_address == 0x1000 && frame_pc == 0x1002 && frame_format == 2);
        assert(pc == 0x2000 && mmu030_opcode_stageb == -1);
    }
    pc = 0x1002;
    Exception_mmu030(9);
    assert(frame_address == regs.trace_pc && frame_pc == 0x1002 && frame_format == 2);
    pc = 0x1002;
    Exception_mmu030(4);
    assert(frame_format == 0 && frame_pc == 0x1002);
}
""")

    def test_68060_rte_rejects_throwaway_frame(self):
        run_cpp(r"""
#define REGPARAM2
constexpr int CYCLE_UNIT = 2;
struct { bool s = true; uae_u16 sr; uae_u32 a[8]; } regs;
#define m68k_areg(r, n) ((r).a[n])
int frame, exception, reads;
uae_u32 pc = 0x1000;
uae_u32 m68k_getpc() { return pc; }
void m68k_setpc_j(uae_u32 value) { pc = value; }
void Exception(int n) { exception = n; }
void Exception_cpu(int n) { exception = n; }
void MakeFromSR_T0() {}
void MakeFromSR() {}
void exception3_read_prefetch(uae_u32, uaecptr) { assert(false); }
#define SET_ZFLG(v) ((void)0)
#define SET_NFLG(v) ((void)0)
uae_u16 get_word(uaecptr addr) {
    assert(++reads < 10);
    return addr % 8 == 6 ? frame << 12 : 0x2000;
}
uae_u32 get_long(uaecptr) { return 0x4000; }
""" + function(source("src/cpuemu_0.cpp"),
               "uae_u32 REGPARAM2 op_4e73_0_ff(") + r"""
int main() {
    for (int format : {0, 1, 2, 3, 4, 7, 8, 15}) {
        frame = format;
        exception = reads = 0;
        regs.a[7] = 0x8000;
        pc = 0x1000;
        op_4e73_0_ff(0x4e73);
        if (format == 0 || format == 2 || format == 3 || format == 4) {
            assert(exception == 0 && pc == 0x4000);
            assert(regs.a[7] == 0x8000u + (format == 0 ? 8 : format == 4 ? 16 : 12));
        } else {
            assert(exception == 14 && pc == 0x1000 && regs.a[7] == 0x8000);
        }
    }
}
""")

    def test_cia_reads_do_not_invent_elapsed_time(self):
        text = function(source("src/cia.cpp"), "static uae_u8 ReadCIAReg(")
        start = text.index("\t\tuae_u16 tval")
        timer = text[start:text.index("\n\t}\n\tcase 8:", start)]
        run_cpp(r"""
constexpr int CR_START = 1, CR_RUNMODE = 8, CR_INMODE = 32, CR_INMODE1 = 64;
constexpr int SPCFLAG_CPU_SLOW = 1;
struct { int cachesize, m68k_speed; } currprefs;
struct Timer { uae_u32 timer, passed, latch, timerval_prev; uae_u8 cr; };
int slow_cpu_access = 1, cia_timer_hack_adjust = 1, slow_requests;
void set_special(int flag) { assert(flag == SPCFLAG_CPU_SLOW); ++slow_requests; }
uae_u8 read_timer(Timer *t, int reg) {
""" + timer + r"""
}
int main() {
    for (int jit : {0, 8192}) for (int speed : {0, -1}) {
        currprefs = {jit, speed};
        for (uae_u32 value : {0u, 1u, 2u, 255u, 256u, 0x1234u, 65535u}) {
            for (int mode : {CR_START, CR_START | CR_RUNMODE, CR_START | CR_INMODE1}) {
                Timer t{value, 0, value, 0xffffffff, uae_u8(mode)};
                for (int i = 0; i < 10; ++i) {
                    assert(read_timer(&t, 4) == (value & 255));
                    assert(read_timer(&t, 5) == (value >> 8));
                    assert(t.timer == value);
                }
                if (value) {
                    t.passed = 1;
                    assert(read_timer(&t, 4) == ((value - 1) & 255));
                    assert(read_timer(&t, 5) == ((value - 1) >> 8));
                }
            }
        }
    }
    assert(slow_requests > 0);
}
""")

    def test_divu_overflow_flags_for_all_initial_conditions(self):
        run_cpp(r"""
struct { int cpu_model; } currprefs;
int n, z, v, c, x;
#define SET_NFLG(a) (n = (a))
#define SET_ZFLG(a) (z = (a))
#define SET_VFLG(a) (v = (a))
#define SET_CFLG(a) (c = (a))
""" + function(source("src/newcpu_common.cpp"), "void setdivuflags(") + r"""
int main() {
    for (int cpu : {68000, 68010, 68020, 68030, 68040, 68060}) {
        currprefs.cpu_model = cpu;
        for (uae_u32 dividend : {0x10000u, 0x7fffffffu, 0x80000000u, 0xffffffffu}) {
            for (uae_u16 divisor : {uae_u16(1), uae_u16(0x8000), uae_u16(0xffff)}) {
                for (int flags = 0; flags < 32; ++flags) {
                    n = flags & 1; z = (flags >> 1) & 1;
                    v = (flags >> 2) & 1; c = (flags >> 3) & 1; x = (flags >> 4) & 1;
                    int old_n = n, old_z = z, old_x = x;
                    setdivuflags(dividend, divisor);
                    assert(v == 1 && c == 0 && x == old_x);
                    if (cpu >= 68040) {
                        assert(n == old_n && z == old_z);
                    } else {
                        bool negative = (uae_s32)dividend < 0;
                        int expected_n = cpu == 68000 ? 1 :
                            cpu == 68010 ? !(negative && (uae_s16)divisor < 0) : negative;
                        assert(n == expected_n && z == 0);
                    }
                }
            }
        }
    }
}
""")

    def test_host_interrupt_atomics_return_previous_flags(self):
        text = source("src/threaddep/thread.h")
        run_cpp(r"""
using uae_atomic = uae_u32;
""" + function(text, "static uae_atomic atomic_and(") +
                function(text, "static uae_atomic atomic_or(") + r"""
int main() {
    volatile uae_atomic flags = 0;
    assert(atomic_or(&flags, 5) == 0 && flags == 5);
    assert(atomic_or(&flags, 2) == 5 && flags == 7);
    assert(atomic_and(&flags, 6) == 7 && flags == 6);
    assert(atomic_and(&flags, 0) == 6 && flags == 0);
    assert(atomic_and(&flags, 0) == 0 && flags == 0);
}
""")

    def test_arm_movem_word_special_memory_extends_each_register(self):
        text = source("src/jit/arm/compemu_arm.cpp")
        handlers = "\n".join(function(text, f"uae_u32 REGPARAM2 op_4c90_0_comp_{mode}(")
                             for mode in ("ff", "nf"))
        run_cpp(r"""
#define REGPARAM2
constexpr int SYNC_PC_OFFSET = 100;
bool special_mem = true;
uae_u32 values[32], m68k_pc_offset;
uae_u16 mask, word;
int reads;
uae_u16 comp_get_iword(int) { return mask; }
int alloc_scratch() { return 16; }
void get_n_addr(int, int) { assert(false); }
void jnf_MVMEL_w(int, int, int) { assert(false); }
void mov_l_rr(int d, int s) { values[d] = values[s]; }
void readword(int address, int dest) {
    assert(values[address] == 0x1000u + reads * 2);
    ++reads;
    values[dest] = word;
}
void sign_extend_16_rr(int d, int s) { values[d] = (uae_s32)(uae_s16)values[s]; }
void arm_ADD_l_ri8(int d, int value) { values[d] += value; }
void sync_m68k_pc() { assert(false); }
""" + handlers + r"""
int main() {
    for (auto handler : {op_4c90_0_comp_ff, op_4c90_0_comp_nf}) {
        for (uae_u16 bits : {0, 1, 0x8000, 0xffff}) {
            for (uae_u16 input : {0, 0x7fff, 0x8000, 0xffff}) {
                for (auto &value : values) value = 0xbadf00d;
                values[8] = 0x1000;
                mask = bits; word = input; reads = 0; m68k_pc_offset = 0;
                handler(0x4c90);
                int expected_reads = 0;
                for (int i = 0; i < 16; ++i) {
                    if (bits & (1 << i)) {
                        assert(values[i] == (uae_u32)(uae_s32)(uae_s16)input);
                        ++expected_reads;
                    } else {
                        assert(values[i] == (i == 8 ? 0x1000u : 0xbadf00du));
                    }
                }
                assert(reads == expected_reads);
            }
        }
    }
}
""")

    def test_arm_constant_bitfield_offsets_match_register_offsets(self):
        for path in ("src/jit/arm/compemu_midfunc_arm64.cpp",):
            with self.subTest(path=path):
                run_cpp(r"""
#define MIDFUNC(n, name, args) void generated args
#define RW4 int
#define RR4 int
#define COMPCALL(name) name
constexpr int REG_WORK1 = 31;
struct { struct { uae_u32 val; } state[32]; } live;
uae_u32 values[32];
bool constant;
bool isconst(int) { return constant; }
int readreg(int r) { return r; }
int rmw(int r) { return r; }
void unlock2(int) {}
void arm_ADD_l_ri(int d, uae_u32 value) { values[d] += value; }
void ASR_rri(int d, int s, int shift) { values[d] = (uae_s32)values[s] >> shift; }
void ASR_wwi(int d, int s, int shift) { ASR_rri(d, s, shift); }
void ADD_rrrLSLi(int d, int s, int t, int shift) { values[d] = values[s] + (values[t] << shift); }
void ADD_wwwLSLi(int d, int s, int t, int shift) { ADD_rrrLSLi(d, s, t, shift); }
""" + function(source(path), "MIDFUNC(2,arm_ADD_ldiv8,") + r"""
int main() {
    for (uae_s32 offset : {INT32_MIN, -129, -33, -32, -31, -1, 0, 1, 31, 32, INT32_MAX}) {
        uae_s64 rounded = (uae_s64(offset) - (offset < 0 ? 31 : 0)) / 32 * 4;
        for (bool mode : {false, true}) {
            constant = mode;
            values[0] = 0x1000;
            values[1] = live.state[1].val = offset;
            generated(0, 1);
            assert(values[0] == 0x1000u + (uae_u32)rounded);
        }
    }
}
""")

    def test_arm_bit_operations_keep_other_live_flags(self):
        text = source("src/jit/arm/compemu_arm.cpp")
        for opcode in ("100", "110", "140", "150", "180", "190", "1c0", "1d0"):
            self.assertNotIn("dont_care_flags();",
                             function(text, f"uae_u32 REGPARAM2 op_{opcode}_0_comp_nf("))
        generator = source("src/jit/arm/gencomp_arm.c")
        for operation in ("bchg", "bclr", "bset", "btst"):
            self.assertNotRegex(function(generator, f"static void gen_{operation}("),
                                r'comprintf\("[^"]*dont_care_flags')

    def test_threaded_jit_kicks_end_compiled_code_without_losing_cycles(self):
        text = source("src/newcpu.cpp")
        declarations = text[text.index("#ifdef WITH_THREADED_CPU\n#if defined(__GNUC__)") + len("#ifdef WITH_THREADED_CPU\n"):
                            text.index("static inline void cpu_thread_spin_pause()")]
        kick_state = text[text.index("// Kicking the threaded JIT."):
                          text.index("// A consistent JIT total;")]
        events = source("src/include/events.h")
        self.assertIn("cpu_thread_kick();", function(events, "STATIC_INLINE void cycles_do_special ("))
        thread = function(text, "static void run_cpu_thread(")
        self.assertRegex(thread, r"paced == 2 && CPU_THREAD_RARE\(regs\.spcflags & CPU_THREAD_KICK_FLAGS\)\)\s*cpu_thread_kick\(\);")
        self.assertIn("cpu_thread_jit_unkick();", thread)
        for path in ("src/osdep/sigsegv_handler.cpp",):
            self.assertNotIn("countdown = 0;", source(path))
        run_cpp(r"""
#define WITH_THREADED_CPU
constexpr int CYCLE_UNIT = 512;
int countdown, pissoff_value = 1000000;
thread_local bool cpu_worker;
using evt_t = uae_u64;
void cpu_thread_spin_pause() {}
uae_s64 cpu_thread_jit_own_total() { return 0; }
void cpu_thread_timed_check(uae_s64) { assert(false); }   // timed pacing only
""" + declarations + kick_state +
                function(text, "static bool cpu_thread_jit_total(") +
                function(text, "static uae_s64 cpu_thread_progress(") +
                function(text, "static void cpu_thread_jit_set_countdown(") +
                function(text, "static void cpu_thread_jit_refill(") +
                function(text, "static void cpu_thread_jit_break(") +
                function(text, "void cpu_thread_kick()") +
                function(text, "static void cpu_thread_jit_unkick()") +
                function(text, "static void cpu_thread_jit_count(") + r"""
int main() {
    // Single-threaded: a kick ends the dispatch and keeps the count exact.
    cpu_thread_paced = 2;
    uae_s64 seen = 0;
    countdown = cpu_thread_jit_base = pissoff_value;
    countdown -= 100;
    cpu_thread_kick();
    assert(countdown == CPU_THREAD_KICK && cpu_thread_progress(&seen) == 100);
    cpu_thread_kick();   // kicked already
    assert(countdown == CPU_THREAD_KICK);
    countdown -= 7;      // the block running ends: still negative
    assert(countdown < 0 && cpu_thread_progress(&seen) == 7);
    cpu_thread_jit_count(5);   // interpreted instructions consume the kick
    assert(countdown == pissoff_value && cpu_thread_progress(&seen) == 5);
    cpu_thread_kick();
    cpu_thread_jit_break();    // a kick is left alone
    assert(countdown == CPU_THREAD_KICK && cpu_thread_progress(&seen) == 0);
    cpu_thread_jit_refill();
    assert(countdown == pissoff_value && cpu_thread_progress(&seen) == 0);
    cpu_worker = true;         // the CPU thread kicks itself without a swap
    countdown -= 3;
    cpu_thread_kick();
    assert(countdown == -1 && cpu_thread_progress(&seen) == 3);
    cpu_worker = false;
    cpu_thread_kick();
    assert(countdown == CPU_THREAD_KICK && cpu_thread_progress(&seen) == 0);
    cpu_thread_jit_unkick();   // nothing left for the next CPU loop
    assert(countdown == 0 && cpu_thread_paced == 0);
    cpu_thread_kick();         // not running the JIT: no kick
    assert(countdown == 0);

    // Concurrent: compiled blocks subtract without a lock while another
    // thread kicks. Kicks end dispatches; not one cycle is lost or made up.
    cpu_thread_paced = 2;
    seen = 0;
    countdown = cpu_thread_jit_base = pissoff_value;
    std::atomic<bool> done{false};
    uae_s64 executed = 0;
    int kicked_exits = 0, natural_exits = 0;
    std::thread worker([&] {
        cpu_worker = true;
        uae_u32 r = 1;
        for (int i = 0; i < 3000000; i++) {
            r = r * 1103515245u + 12345u;
            const int c = (r >> 16) % 64 + 1;
            const int cd = __atomic_load_n(&countdown, __ATOMIC_RELAXED);
            __atomic_store_n(&countdown, cd - c, __ATOMIC_RELAXED);
            executed += c;
            if (cd - c < 0) {
                if (cd - c < CPU_THREAD_KICKED)
                    ++kicked_exits;
                else
                    ++natural_exits;
                cpu_thread_jit_refill();
            }
            if ((i & 1023) == 0) {
                cpu_thread_jit_count(c);
                executed += c;
            }
            if ((i & 8191) == 0)
                cpu_thread_kick();
        }
        done = true;
    });
    uae_s64 total = 0;
    int kicks = 0;
    while (!done) {
        cpu_thread_kick();
        ++kicks;
        total += cpu_thread_progress(&seen);
        for (int k = 0; k < 200; k++)
            __atomic_signal_fence(__ATOMIC_SEQ_CST);
    }
    worker.join();
    total += cpu_thread_progress(&seen);
    assert(total == executed);
    assert(kicks > 0 && kicked_exits > 0);
    return 0;
}
""")

    def test_threaded_cpu_progress_and_bounded_chipset_waits(self):
        text = source("src/newcpu.cpp")
        pacing = "#ifdef WITH_THREADED_CPU\n#if defined(__GNUC__)"
        declarations = text[text.index(pacing) + len("#ifdef WITH_THREADED_CPU\n"):
                            text.index("static inline void cpu_thread_spin_pause()")]
        kick_state = text[text.index("// Kicking the threaded JIT."):
                          text.index("// A consistent JIT total;")]
        thread = function(text, "static void run_cpu_thread(")
        start = thread.index("\t\tfor (int i = 0; i < cycles_to_do; i++)")
        batch = thread[start:thread.index("\n\t\tcheck_debugger();", start)]
        run_cpp(r"""
#define WITH_THREADED_CPU
constexpr int CYCLE_UNIT = 512;
constexpr int SPCFLAG_CPUINRESET = 1, SPCFLAG_MODE_CHANGE = 2, SPCFLAG_CALLBACK = 4;
constexpr int SPCFLAG_INT = 8, SPCFLAG_CPU_THREAD_SYNC = 16, SPCFLAG_BRK = 32;
using frame_time_t = uae_s64;
using evt_t = uae_u64;
struct { bool stopped, halted; int spcflags; } regs;
int countdown, pissoff_value = 100000, cpu_thread_reset, m68k_reset_delay;
int cpu_thread_ilvl, cpu_wakeup_sema;
uae_u32 cpu_thread_indirect_mode;
int clock_reads, spins, requests;
void cpu_thread_spin_pause() { ++spins; }
bool cpu_worker;
uae_u32 cpu_thread_blit_held;
""" + declarations + kick_state +
                function(text, "static bool cpu_thread_jit_total(") +
                function(text, "static uae_s64 cpu_thread_progress(") +
                function(text, "static bool cpu_thread_unpaced(") +
                function(text, "static void cpu_thread_jit_set_countdown(") +
                function(text, "static void cpu_thread_jit_refill(") +
                function(text, "static void cpu_thread_jit_break(") +
                function(text, "static void cpu_thread_jit_count(") + r"""
evt_t chipset_cycles;
frame_time_t syncbase = 1000000, vsynctimebase = 20000;
bool indirect;
frame_time_t read_processor_time() { return ++clock_reads * 50; }
bool service_cpu_indirect_request(bool = false) { ++requests; return indirect; }
evt_t get_cycles() { return chipset_cycles; }
void do_cycles(int n) { chipset_cycles += n; }
void check_uae_int_request() {}
int intlev() { return 0; }
void atomic_set(int *p, int value) { *p = value; }
void set_special(int flag) { regs.spcflags |= flag; }
void uae_sem_post(int *) {}
bool cpu_thread_interrupt_wakes(int) { return false; }
void cpu_thread_main_wait_wake() {}
void cpu_thread_flush_resume() {}
struct { unsigned armed; } cpu_thread_snap_main;   // no register snapshot armed
void cpu_thread_snap_publish(bool) { assert(false); }
""" + served_credit(text) + r"""
void run_batch(int cycles_to_do) {
    uae_s64 tokens_seen = 0, credit = 0;
    int cck_cnt = 0, intlev_prev = 0;
""" + batch + r"""
}
int main() {
    cpu_thread_paced = 1;
    uae_s64 seen = 0;
    cpu_thread_tokens = 3;
    assert(cpu_thread_progress(&seen) == 3);
    assert(seen == 3 && cpu_thread_progress(&seen) == 0);
    cpu_thread_tokens = 5;
    assert(cpu_thread_progress(&seen) == 2);
    // A stale total adds nothing and does not move the mark back.
    cpu_thread_tokens = 4;
    assert(cpu_thread_progress(&seen) == 0 && seen == 5);
    cpu_thread_tokens = 6;
    assert(cpu_thread_progress(&seen) == 1);

    // The JIT: progress is live, read from countdown at any time.
    cpu_thread_paced = 2;
    seen = 0;
    countdown = cpu_thread_jit_base = pissoff_value;
    assert(cpu_thread_progress(&seen) == 0);
    countdown -= 11;   // a compiled block
    assert(cpu_thread_progress(&seen) == 11);
    cpu_thread_jit_count(42);
    assert(cpu_thread_progress(&seen) == 42);
    assert(cpu_thread_progress(&seen) == 0);
    // Work is counted once however many refills happen between chipset
    // samples; a refill itself adds no progress.
    countdown -= 17;
    cpu_thread_jit_refill();
    countdown -= pissoff_value + 3;
    cpu_thread_jit_refill();
    assert(countdown == pissoff_value);
    assert(cpu_thread_progress(&seen) == 17 + pissoff_value + 3);
    cpu_thread_jit_refill();
    assert(cpu_thread_progress(&seen) == 0);
    countdown -= 31;
    cpu_thread_jit_break();
    assert(countdown == -1 && cpu_thread_progress(&seen) == 31);
    cpu_thread_jit_break();   // already leaving: nothing changes
    assert(countdown == -1 && cpu_thread_progress(&seen) == 0);
    cpu_thread_jit_refill();
    assert(cpu_thread_progress(&seen) == 0);
    cpu_thread_jit_set_countdown(1);
    assert(cpu_thread_progress(&seen) == 0);
    cpu_thread_jit_count(2);
    assert(countdown == pissoff_value);
    assert(cpu_thread_progress(&seen) == 2);
    // A sample taken while the CPU thread changes countdown is skipped.
    ++cpu_thread_jit_seq;
    countdown -= 5;
    spins = 0;
    assert(cpu_thread_progress(&seen) == 0 && spins > 0);
    ++cpu_thread_jit_seq;
    assert(cpu_thread_progress(&seen) == 5);
    assert(!(cpu_thread_jit_seq & 1));

    cpu_thread_paced = 1;
    cpu_thread_tokens = 0;
    assert(!cpu_thread_unpaced());
    // A descheduled CPU must end the batch rather than run the chipset ahead.
    run_batch(128);
    assert(chipset_cycles == CYCLE_UNIT && spins > 0);
    assert(clock_reads > 1 && clock_reads < 100);
    for (int state = 0; state < 6; ++state) {
        regs = {};
        cpu_thread_reset = 0;
        if (state == 0) regs.stopped = true;
        if (state == 1) regs.halted = true;
        if (state == 2) regs.spcflags = SPCFLAG_CPUINRESET;
        if (state == 3) regs.spcflags = SPCFLAG_MODE_CHANGE;
        if (state == 4) regs.spcflags = SPCFLAG_CALLBACK;
        if (state == 5) cpu_thread_reset = 1;
        chipset_cycles = 0;
        clock_reads = spins = 0;
        assert(cpu_thread_unpaced());
        run_batch(128);
        assert(chipset_cycles == 128 * CYCLE_UNIT);
        assert(clock_reads == 0 && spins == 0);
    }
    regs = {};
    cpu_thread_reset = 0;
    chipset_cycles = 0;
    clock_reads = spins = requests = 0;
    indirect = true;
    run_batch(128);
    assert(chipset_cycles == 128 * CYCLE_UNIT && requests > 0);
    assert(spins == 0);
}
""")

    def test_threaded_cpu_unbounded_jit_fallback_yields_for_pause(self):
        text = source("src/newcpu.cpp")
        run_cpp(r"""
#define WITH_THREADED_CPU
constexpr int SPCFLAG_MODE_CHANGE = 1, SPCFLAG_CPU_THREAD_SYNC = 2, CYCLE_UNIT = 512;
struct regstruct { uaecptr instruction_pc; uae_u32 opcode; int spcflags; } regs;
bool cpu_threaded_run = true;
int cpu_thread_pause_requested, cpu_cycles, uae_int_requested, steps, accounted;
bool changing_mode;
uaecptr m68k_getpc() { return 0x30000; }
uae_u32 get_jit_opcode() { return 0; }
void instruction(uae_u32) {
    if (++steps == 5) {
        if (changing_mode) regs.spcflags = SPCFLAG_MODE_CHANGE;
        else { cpu_thread_pause_requested = 1; regs.spcflags = SPCFLAG_CPU_THREAD_SYNC; }
    }
    assert(steps <= 5);
}
void (*cpufunctbl[])(uae_u32) = {instruction};
bool end_block(uae_u32) { return false; } // e.g. a constant-jump loop
void do_cycles(int) { assert(false); }
void cpu_thread_jit_count(int) { ++accounted; }
""" + function(text, "static inline bool cpu_thread_leave_jit(") +
                function(text, "static bool cpu_thread_exit_dispatch(") +
                function(text, "void exec_nostats (") + r"""
int main() {
    for (bool mode_change : {false, true}) {
        changing_mode = mode_change;
        steps = accounted = cpu_thread_pause_requested = regs.spcflags = 0;
        exec_nostats();
        assert(steps == 5 && accounted == 5);
    }
}
""")

    def test_threaded_cpu_pause_does_not_abandon_expired_jit_recompilation(self):
        text = source("src/newcpu.cpp")
        run_cpp(r"""
#define WITH_THREADED_CPU
constexpr int SPCFLAG_MODE_CHANGE = 1, SPCFLAG_CPU_THREAD_SYNC = 2, CYCLE_UNIT = 512, MAXRUN = 4;
bool cpu_threaded_run = true;
struct regstruct {
    uaecptr instruction_pc, pc;
    uae_u32 opcode;
    uae_u16 *pc_oldp, *pc_p;
    int spcflags;
} regs;
struct cpu_history { uae_u16 *location; int specmem; };
int cpu_cycles, uae_int_requested, steps, accounted, compiled, hot_count;
int cpu_thread_pause_requested, special_mem, special_mem_default;
uae_u16 *start_pc_p;
uaecptr start_pc;
uae_u16 words[MAXRUN + 1];
bool check_for_cache_miss() { return false; }
uaecptr m68k_getpc() { return regs.pc; }
uae_u32 get_jit_opcode() { return 0; }
void instruction(uae_u32) {
    ++steps;
    regs.pc += 2;
    regs.pc_p = words + steps;
    if (steps == 2) { cpu_thread_pause_requested = 1; regs.spcflags |= SPCFLAG_CPU_THREAD_SYNC; }
    assert(steps <= MAXRUN);
}
void (*cpufunctbl[])(uae_u32) = {instruction};
bool end_block(uae_u32) { return false; }
void do_cycles(int) { assert(false); }
void cpu_thread_jit_count(int) { ++accounted; }
void compile_block(cpu_history *history, int length, int cycles) {
    assert(history[0].location == words);
    assert(length == steps && cycles == steps * 4 * CYCLE_UNIT);
    assert(hot_count == -1); // The generated handler already decremented it.
    hot_count = 7;
    ++compiled;
}
""" + function(text, "static inline bool cpu_thread_leave_jit(") +
                function(text, "static bool cpu_thread_exit_dispatch(") +
                function(text, "void execute_normal(") + r"""
int main() {
    for (int scenario : {0, 1, 2}) {
        regs = {};
        regs.pc = 0x1000;
        regs.pc_oldp = regs.pc_p = words;
        steps = accounted = compiled = 0;
        hot_count = -1;
        cpu_thread_pause_requested = scenario == 0;
        regs.spcflags = scenario == 2 ? SPCFLAG_MODE_CHANGE
            : scenario == 0 ? SPCFLAG_CPU_THREAD_SYNC : 0;
        execute_normal();
        assert(cpu_thread_exit_dispatch());
        assert(compiled == 1 && hot_count == 7);
        // Any special flag (pause raises SPCFLAG_CPU_THREAD_SYNC) ends the
        // recording, but the expired block must still be compiled.
        assert(steps == (scenario == 1 ? 2 : 1));
        assert(accounted == steps);
    }
}
""")

    def test_threaded_cpu_safepoints_nesting_and_fast_worker_exit(self):
        text = source("src/newcpu.cpp")
        threaded = text[text.index("template <typename Busy>\nstatic bool cpu_thread_poll_while("):]
        run_cpp(r"""
#include <mutex>
#include <condition_variable>
#include <chrono>
#define WITH_THREADED_CPU
#define DEBUGGER
#define _T(x) x
#define TRY(x) try
#define CATCH(x) catch (int x)
#define ENDTRY
constexpr int SPCFLAG_MODE_CHANGE = 1, SPCFLAG_CALLBACK = 2, SPCFLAG_BRK = 4;
constexpr int SPCFLAG_CPU_THREAD_SYNC = 8;
struct { volatile int spcflags; } regs;
int debugging = 1;
struct uae_sem_t {
    std::mutex mutex;
    std::condition_variable changed;
    unsigned count = 0;
};
uae_sem_t cpu_park_sema, cpu_resume_sema, cpu_wakeup_sema, cpu_request_sema;
void uae_sem_post(uae_sem_t *s) {
    std::lock_guard<std::mutex> lock(s->mutex);
    ++s->count;
    s->changed.notify_one();
}
void uae_sem_wait(uae_sem_t *s) {
    std::unique_lock<std::mutex> lock(s->mutex);
    s->changed.wait(lock, [s] { return s->count != 0; });
    --s->count;
}
int uae_sem_trywait(uae_sem_t *s) {
    std::lock_guard<std::mutex> lock(s->mutex);
    if (!s->count) return -1;
    --s->count;
    return 0;
}
void uae_sem_trywait_delay(uae_sem_t *s, int ms) {
    std::unique_lock<std::mutex> lock(s->mutex);
    if (s->changed.wait_for(lock, std::chrono::milliseconds(ms),
                           [s] { return s->count != 0; }))
        --s->count;
}
int cpu_thread_active, cpu_thread_pause_depth;
int cpu_thread_pause_requested, cpu_thread_parked;
int cpu_thread_debugger_waiting;
thread_local bool cpu_worker;
bool is_mainthread() { return !cpu_worker; }
int (*cpu_thread_function)(void *);
bool service_cpu_indirect_request() { return false; }
void service_cpu_thread_reset() {}
void unset_special(int flag) { __atomic_fetch_and(&regs.spcflags, ~flag, __ATOMIC_SEQ_CST); }
void set_special(int flag) { __atomic_fetch_or(&regs.spcflags, flag, __ATOMIC_SEQ_CST); }
void device_call_main_thread_callbacks() {}
void write_log(const char *, ...) {}
void cpu_thread_main_sleep(int ms) { uae_sem_trywait_delay(&cpu_request_sema, ms); }
void cpu_thread_spin_pause() { std::this_thread::yield(); }
uint64_t SDL_GetTicksNS() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
constexpr uint64_t CPU_THREAD_PARK_SPIN_NS = 50000, CPU_THREAD_IDLE_SPIN_NS = 500000;
enum { CPU_THREAD_RUNNING, CPU_THREAD_PARKED, CPU_THREAD_IDLE, CPU_THREAD_IDLE_HELD };
using uae_atomic = int;
bool cpu_threaded_run = true;
void cpu_thread_timed_resync() {}
#define CPU_THREAD_RARE(x) __builtin_expect(!!(x), 0)
struct { bool loop_running, resume_deferred; } cpu_thread_main;
struct { bool armed; } cpu_thread_snap_main;
void cpu_thread_snap_publish(bool) {}
void debug();
int cpu_thread_main_sleeping;
""" + wake_main(text) + "template <typename Busy>\n" + function(threaded, "static bool cpu_thread_poll_while(") +
                function(threaded, "static void cpu_thread_safepoint(") +
                function(threaded, "static void cpu_thread_idle_wait(") +
                function(threaded, "static bool cpu_thread_hold_idle(") +
                function(threaded, "static void cpu_thread_release_idle(") +
                function(text, "static bool cpu_thread_debugger_wait(") +
                function(text, "static void cpu_thread_debugger_resume(") +
                function(text, "static bool take_special(") +
                function(text, "static void check_debugger(") +
                function(text, "bool cpu_thread_pause(") +
                function(text, "static void cpu_thread_release_pause(") +
                function(text, "void cpu_thread_resume(") +
                function(text, "static inline void cpu_thread_flush_resume(") +
                function(text, "static int cpu_thread_entry(") + r"""
std::atomic<bool> finish, cpu_stopped;
std::atomic<unsigned> instructions;
std::vector<unsigned> debug_stops;
void debug() {
    assert(!cpu_worker);
    if (!cpu_thread_pause()) {
        // The worker finished after its last single step armed one more stop.
        assert(__atomic_load_n(&cpu_thread_active, __ATOMIC_ACQUIRE) == 2);
        return;
    }
    assert(__atomic_load_n(&cpu_thread_parked, __ATOMIC_ACQUIRE));
    debug_stops.push_back(instructions);
    // Single-step arms the next boundary before the parked worker resumes.
    set_special(SPCFLAG_BRK);
    cpu_thread_resume();
}
int execute(void *) {
    assert(!cpu_thread_pause());
    while (!finish.load()) {
        // As the fast paths do: a park request is seen only through the flag.
        if (regs.spcflags & SPCFLAG_CPU_THREAD_SYNC) {
            unset_special(SPCFLAG_CPU_THREAD_SYNC);
            cpu_thread_safepoint();
        }
        ++instructions;
    }
    return 0;
}
void started() {
    while (!__atomic_load_n(&cpu_thread_active, __ATOMIC_ACQUIRE))
        std::this_thread::yield();
}
int main() {
    assert(!cpu_thread_pause());
    cpu_thread_function = execute;
    std::thread worker([] { cpu_thread_entry(nullptr); });
    started();
    for (unsigned attempt = 0; attempt < 100; ++attempt) {
        assert(cpu_thread_pause());
        assert(__atomic_load_n(&cpu_thread_parked, __ATOMIC_ACQUIRE));
        assert(cpu_thread_pause());
        unsigned frozen = instructions.load();
        cpu_thread_resume();
        std::this_thread::sleep_for(std::chrono::microseconds(20));
        assert(instructions == frozen);
        cpu_thread_resume();
        while (instructions == frozen)
            std::this_thread::yield();
    }
    // Inside the chipset loop, back-to-back guards share one park: the worker
    // stays frozen until the loop flushes the deferred resume.
    cpu_thread_main.loop_running = true;
    for (unsigned attempt = 0; attempt < 100; ++attempt) {
        assert(cpu_thread_pause());
        unsigned frozen = instructions.load();
        cpu_thread_resume();
        assert(cpu_thread_main.resume_deferred);
        assert(cpu_thread_pause());
        assert(!cpu_thread_main.resume_deferred);
        cpu_thread_resume();
        std::this_thread::sleep_for(std::chrono::microseconds(20));
        assert(instructions == frozen);
        assert(__atomic_load_n(&cpu_thread_parked, __ATOMIC_ACQUIRE) == CPU_THREAD_PARKED);
        cpu_thread_flush_resume();
        assert(!cpu_thread_main.resume_deferred);
        while (instructions == frozen)
            std::this_thread::yield();
    }
    cpu_thread_flush_resume();
    cpu_thread_main.loop_running = false;
    finish = true;
    worker.join();
    assert(cpu_thread_active == 2 && !cpu_thread_parked);
    assert(!cpu_thread_pause());

    // A worker sleeping in STOP is held where it is, without being woken up,
    // and stays held when an interrupt wakes it before the resume.
    cpu_thread_active = 0;
    finish = false;
    instructions = 0;
    cpu_thread_function = [](void *) {
        while (!finish.load()) {
            if (regs.spcflags & SPCFLAG_CPU_THREAD_SYNC) {
                unset_special(SPCFLAG_CPU_THREAD_SYNC);
                cpu_thread_safepoint();
            }
            if (cpu_stopped.load()) {
                cpu_thread_idle_wait(true);
                continue;
            }
            ++instructions;
        }
        return 0;
    };
    std::thread idling([] { cpu_thread_entry(nullptr); });
    started();
    unsigned held = 0;
    for (unsigned attempt = 0; attempt < 300; ++attempt) {
        cpu_stopped = attempt % 3 != 0;
        std::this_thread::sleep_for(std::chrono::microseconds(attempt % 7 * 20));
        assert(cpu_thread_pause());
        int state = __atomic_load_n(&cpu_thread_parked, __ATOMIC_ACQUIRE);
        assert(state == CPU_THREAD_PARKED || state == CPU_THREAD_IDLE_HELD);
        held += state == CPU_THREAD_IDLE_HELD;
        assert(cpu_thread_pause());
        cpu_thread_resume();
        unsigned frozen = instructions.load();
        cpu_stopped = false;
        uae_sem_post(&cpu_wakeup_sema);
        std::this_thread::sleep_for(std::chrono::microseconds(20));
        assert(instructions == frozen);
        cpu_thread_resume();
        while (instructions == frozen)
            std::this_thread::yield();
    }
    assert(held > 0);
    finish = true;
    uae_sem_post(&cpu_wakeup_sema);
    idling.join();
    assert(cpu_thread_active == 2 && !cpu_thread_parked);
    // Exiting before the starter samples state must not look like "not started".
    cpu_thread_active = 0;
    cpu_thread_function = [](void *) { return 0; };
    std::thread immediate([] { cpu_thread_entry(nullptr); });
    immediate.join();
    started();
    assert(cpu_thread_active == 2);

    cpu_thread_active = 0;
    instructions = 0;
    set_special(SPCFLAG_BRK);
    cpu_thread_function = [](void *) {
        while (instructions < 3) {
            check_debugger();
            ++instructions;
        }
        return 0;
    };
    std::thread stepping([] { cpu_thread_entry(nullptr); });
    started();
    while (__atomic_load_n(&cpu_thread_active, __ATOMIC_ACQUIRE) == 1) {
        check_debugger();
        std::this_thread::yield();
    }
    stepping.join();
    assert((debug_stops == std::vector<unsigned>{0, 1, 2}));
}
""")

    def test_threaded_cpu_register_snapshot(self):
        text = source("src/newcpu.cpp")
        snapshot = text[text.index("// Register snapshot."):
                        text.index("uae_u32 process_cpu_indirect_memory_read(")]
        run_cpp(r"""
#include <algorithm>
#define NOINLINE __attribute__((noinline))
#define CPU_THREAD_RARE(x) __builtin_expect(!!(x), 0)
uint64_t host_ns;
uint64_t SDL_GetTicksNS() { return host_ns; }
struct addrbank { int id; };
addrbank custom_bank{1}, cia_bank{2}, chip_bank{3};
struct { int cs_fatgaryrev = -1; } currprefs;
std::atomic<uae_u32> beam{0x2c00};
uae_u16 chip_regs[4] = {0x0200, 0x4020, 0x0020, 0x1100};
uae_u8 pra = 0xfc;
bool pra_ok = true;
unsigned pra_reads;
bool custom_cpu_thread_pos(uae_u16 *vposr, uae_u16 *vhposr) {
    const uae_u32 b = beam.load(std::memory_order_relaxed);
    *vposr = b >> 16;
    *vhposr = (uae_u16)b;
    return true;
}
void custom_cpu_thread_regs(uae_u16 *v) { memcpy(v, chip_regs, sizeof chip_regs); }
bool cia_cpu_thread_pra(uae_u8 *v) { ++pra_reads; *v = pra; return pra_ok; }
""" + snapshot + r"""
bool rd(const addrbank &ab, uaecptr addr, int size, uae_u32 *v) {
    return cpu_thread_snap_read(&ab, addr, size, v);
}
uae_u32 must(const addrbank &ab, uaecptr addr, int size) {
    uae_u32 v = 0xdeadbeef;
    assert(rd(ab, addr, size, &v));
    return v;
}
int main() {
    uae_u32 v;
    cpu_thread_snap_reset();
    // Nothing is used before a read through the mailbox arms it.
    assert(!rd(custom_bank, 0xdff006, 1, &v));
    cpu_thread_snap_publish(true);
    assert(!rd(custom_bank, 0xdff006, 1, &v));

    beam = 0x00012c40;
    cpu_thread_snap_arm(&custom_bank, 0xdff006, 1);
    // reads from the copy take a budget refilled from the colour clocks
    // published: give the checks below a line's worth
    for (int t = 0; t < CPU_THREAD_SNAP_BUDGET_TICKS; t++)
        cpu_thread_snap_publish(true);
    assert(must(custom_bank, 0xdff006, 1) == 0x2c40);
    assert(must(custom_bank, 0xdff004, 2) == 0x00012c40);   // VPOSR:VHPOSR together
    assert(must(custom_bank, 0xdff006, 0) == 0x2c);
    assert(must(custom_bank, 0xdff007, 0) == 0x40);
    assert(must(custom_bank, 0xc0f006, 1) == 0x2c40);       // register mirrors
    assert(!rd(custom_bank, 0xdff007, 1, &v));              // odd word: two registers
    currprefs.cs_fatgaryrev = 0;                            // Fat Gary: unmapped below 0xdf8000
    assert(!rd(custom_bank, 0xdf7006, 1, &v));
    currprefs.cs_fatgaryrev = -1;
    assert(must(custom_bank, 0xdf7006, 1) == 0x2c40);
    assert(!rd(chip_bank, 0xdff006, 1, &v));
    assert(!rd(custom_bank, 0xdff018, 1, &v));              // SERDATR has side effects
    // The beam position is copied every colour clock.
    beam = 0x00012c41;
    cpu_thread_snap_publish(true);
    assert(must(custom_bank, 0xdff006, 1) == 0x2c41);
    // Only armed groups are used.
    assert(!rd(custom_bank, 0xdff01e, 1, &v));
    cpu_thread_snap_arm(&custom_bank, 0xdff01e, 1);
    cpu_thread_snap_publish(false);
    assert(must(custom_bank, 0xdff002, 1) == 0x0200);
    assert(must(custom_bank, 0xdff01c, 2) == 0x40200020);
    assert(must(custom_bank, 0xdff010, 1) == 0x1100);
    assert(!rd(custom_bank, 0xdff002, 2, &v));              // DMACONR + VPOSR: two copies
    // A change is published at the next colour clock.
    chip_regs[2] = 0x0024;
    cpu_thread_snap_publish(true);
    assert(must(custom_bank, 0xdff01e, 1) == 0x0024);
    // While the chipset time stands still for longer than a line of host
    // time, reads go through the mailbox until it moves on.
    for (int i = 0; i < 2 * CPU_THREAD_SNAP_CLOCK_READS; i++)
        must(custom_bank, 0xdff006, 1);
    host_ns += CPU_THREAD_SNAP_FROZEN_NS - 1;
    for (int i = 0; i < CPU_THREAD_SNAP_CLOCK_READS; i++)
        must(custom_bank, 0xdff006, 1);
    host_ns += 1;
    {
        int i = 0;
        while (rd(custom_bank, 0xdff006, 1, &v))
            assert(++i <= CPU_THREAD_SNAP_CLOCK_READS);
    }
    assert(!rd(custom_bank, 0xdff01e, 1, &v));
    cpu_thread_snap_publish(true);
    assert(must(custom_bank, 0xdff006, 1) == 0x2c41);
    host_ns += 10 * CPU_THREAD_SNAP_FROZEN_NS;
    assert(must(custom_bank, 0xdff01e, 1) == 0x0024);      // checked again only later
    // After an interrupt only a copy published later is used.
    const uae_u32 seq = cpu_thread_snap_regs.seq;
    cpu_thread_snap_interrupt();
    assert(!rd(custom_bank, 0xdff01e, 1, &v));
    assert(must(custom_bank, 0xdff006, 1) == 0x2c41);       // the beam is not affected
    for (int i = 1; i < CPU_THREAD_SNAP_REFRESH && cpu_thread_snap_regs.seq == seq; i++)
        cpu_thread_snap_publish(true);                     // unchanged: republished
    assert(cpu_thread_snap_regs.seq != seq);
    assert(must(custom_bank, 0xdff01e, 1) == 0x0024);
    cpu_thread_snap_interrupt();
    assert(!rd(custom_bank, 0xdff01e, 1, &v));
    cpu_thread_snap_publish(false);                        // a served request republishes
    assert(must(custom_bank, 0xdff01e, 1) == 0x0024);
    // CIA-A PRA, byte reads of 0xbfe001 only
    assert(!rd(cia_bank, 0xbfe001, 0, &v));
    cpu_thread_snap_arm(&cia_bank, 0xbfe001, 0);
    cpu_thread_snap_publish(false);
    assert(must(cia_bank, 0xbfe001, 0) == 0xfc);
    assert(!rd(cia_bank, 0xbfe041, 0, &v));
    assert(!rd(cia_bank, 0xbfe101, 0, &v));
    assert(!rd(cia_bank, 0xbfe000, 1, &v));
    assert(!rd(cia_bank, 0xbfd000, 0, &v));
    pra = 0xbc;
    const unsigned before = pra_reads;
    for (int i = 0; i < CPU_THREAD_SNAP_CIA_REFRESH; i++)
        cpu_thread_snap_publish(true);
    assert(pra_reads == before + 1);
    assert(must(cia_bank, 0xbfe001, 0) == 0xbc);
    // CIA reads take a budget refilled from the colour clocks published, as
    // the CPU on the main thread pays E-clock time for each one.
    while (rd(cia_bank, 0xbfe001, 0, &v)) {}
    assert(must(custom_bank, 0xdff01e, 1) == 0x0024);      // custom registers have none
    cpu_thread_snap_publish(true);
    {
        int i = 0;
        while (rd(cia_bank, 0xbfe001, 0, &v))
            ++i;
        assert(i == cpu_thread_snap_rate.cia);
    }
    for (int t = 0; t < 4 * CPU_THREAD_SNAP_BUDGET_TICKS; t++)
        cpu_thread_snap_publish(true);
    {
        int i = 0;                                          // at most a line's worth kept
        while (rd(cia_bank, 0xbfe001, 0, &v))
            ++i;
        assert(i == CPU_THREAD_SNAP_BUDGET_TICKS * cpu_thread_snap_rate.cia);
    }
    cpu_thread_snap_publish(true);
    // The custom registers have a budget of their own: a polling loop with an
    // iteration bound must not run out of iterations at one chipset time.
    while (rd(custom_bank, 0xdff006, 1, &v)) {}
    assert(!rd(custom_bank, 0xdff01e, 1, &v));             // beam and regs share it
    assert(must(cia_bank, 0xbfe001, 0) == 0xbc);           // the CIA's is separate
    cpu_thread_snap_publish(true);
    {
        int i = 0;
        while (rd(custom_bank, 0xdff006, 1, &v))
            ++i;
        assert(i == cpu_thread_snap_rate.custom);
    }
    for (int t = 0; t < 4 * CPU_THREAD_SNAP_BUDGET_TICKS; t++)
        cpu_thread_snap_publish(true);
    {
        int i = 0;                                          // at most a line's worth kept
        while (rd(custom_bank, 0xdff006, 1, &v))
            ++i;
        assert(i == CPU_THREAD_SNAP_BUDGET_TICKS * cpu_thread_snap_rate.custom);
    }
    // A longword read takes two, like the two bus accesses it is.
    cpu_thread_snap_publish(true);
    {
        int i = 0;
        while (rd(custom_bank, 0xdff004, 2, &v))
            ++i;
        assert(i == cpu_thread_snap_rate.custom / 2);
    }
    // The budgets follow the host's speed, between a floor and a ceiling;
    // only a faster timing changes them.
    assert(cpu_thread_snap_rate.custom == CPU_THREAD_SNAP_CUSTOM_MIN);  // untimed
    assert(cpu_thread_snap_rate.cia == CPU_THREAD_SNAP_CIA_MIN);
    cpu_thread_snap_set_rate(0);
    assert(cpu_thread_snap_rate.custom == CPU_THREAD_SNAP_CUSTOM_MIN);
    cpu_thread_snap_set_rate(CPU_THREAD_SNAP_CIA_NS * 1000 / 3);        // 3 CIA reads
    assert(cpu_thread_snap_rate.cia == 3);
    assert(cpu_thread_snap_rate.custom == CPU_THREAD_SNAP_CUSTOM_MIN);
    cpu_thread_snap_set_rate(1000000);                                  // slower: kept
    assert(cpu_thread_snap_rate.cia == 3);
    cpu_thread_snap_set_rate(CPU_THREAD_SNAP_CUSTOM_NS * 1000 / (CPU_THREAD_SNAP_CUSTOM_MIN + 8));
    assert(cpu_thread_snap_rate.custom == CPU_THREAD_SNAP_CUSTOM_MIN + 8);
    cpu_thread_snap_set_rate(1);
    assert(cpu_thread_snap_rate.custom == CPU_THREAD_SNAP_CUSTOM_MAX);
    assert(cpu_thread_snap_rate.cia == CPU_THREAD_SNAP_CIA_MAX);
    for (int t = 0; t < 4 * CPU_THREAD_SNAP_BUDGET_TICKS; t++)
        cpu_thread_snap_publish(true);
    {
        int i = 0;
        while (rd(custom_bank, 0xdff006, 1, &v))
            ++i;
        assert(i == CPU_THREAD_SNAP_BUDGET_TICKS * CPU_THREAD_SNAP_CUSTOM_MAX);
    }
    cpu_thread_snap_publish(true);
    pra_ok = false;                                        // input recording started
    cpu_thread_snap_publish(false);
    assert(!rd(cia_bank, 0xbfe001, 0, &v));
    assert(must(custom_bank, 0xdff01e, 1) == 0x0024);
    pra_ok = true;
    // Unused copies expire.
    for (int i = 0; i < CPU_THREAD_SNAP_TTL; i++)
        cpu_thread_snap_publish(true);
    assert(!cpu_thread_snap_main.armed);
    assert(!rd(custom_bank, 0xdff006, 1, &v));
    assert(!rd(custom_bank, 0xdff01e, 1, &v));
    assert(!rd(cia_bank, 0xbfe001, 0, &v));
    // A stopped CPU thread finds nothing on its next start.
    cpu_thread_snap_arm(&custom_bank, 0xdff006, 1);
    cpu_thread_snap_publish(false);
    cpu_thread_snap_reset();
    assert(!rd(custom_bank, 0xdff006, 1, &v));

    // Concurrent copies are never torn: VPOSR and VHPOSR always match.
    beam = 0x0000ffff;
    cpu_thread_snap_arm(&custom_bank, 0xdff006, 1);
    cpu_thread_snap_publish(false);
    std::atomic<bool> done{false};
    std::thread chipset([&] {
        for (uae_u32 n = 0; n < 2000000; n++) {
            beam = (n & 0xffff) << 16 | (uae_u16)~n;
            cpu_thread_snap_publish(!(n % 64));   // colour clocks refill the read budget
        }
        done = true;
    });
    unsigned hits = 0;
    while (!done) {
        if (rd(custom_bank, 0xdff004, 2, &v)) {
            assert((uae_u16)v == (uae_u16)~(v >> 16));
            ++hits;
        }
    }
    chipset.join();
    assert(hits > 0);
}
""")

    def test_threaded_cpu_mailbox_completes_during_mode_changes(self):
        text = source("src/newcpu.cpp")
        run_cpp(r"""
#include <mutex>
#include <condition_variable>
#include <chrono>
#define TRY(x) try
#define CATCH(x) catch (int x)
#define THROW(x) throw (x)
#define ENDTRY
using uae_atomic = int;
struct uae_sem_t {
    std::mutex mutex;
    std::condition_variable changed;
    unsigned count = 0;
};
uae_sem_t cpu_reply_sema, cpu_request_sema;
void uae_sem_post(uae_sem_t *s) {
    std::lock_guard<std::mutex> lock(s->mutex);
    ++s->count;
    s->changed.notify_one();
}
volatile uae_atomic cpu_thread_indirect_mode = 0xff;
std::atomic<unsigned> lost_wakeups, worker_sleeps;
bool missed(uae_sem_t *s) {
    uae_u32 mode = __atomic_load_n(&cpu_thread_indirect_mode, __ATOMIC_SEQ_CST);
    return s == &cpu_request_sema ? mode == 1 || mode == 2 : mode >= 0xfd;
}
void uae_sem_trywait_delay(uae_sem_t *s, int ms) {
    if (s == &cpu_reply_sema) ++worker_sleeps;
    std::unique_lock<std::mutex> lock(s->mutex);
    if (s->changed.wait_for(lock, std::chrono::milliseconds(ms),
                           [s] { return s->count != 0; }))
        --s->count;
    else if (missed(s))
        ++lost_wakeups;   // slept a whole timeout with the peer's work pending
}
volatile uae_u32 cpu_thread_indirect_addr, cpu_thread_indirect_val, cpu_thread_indirect_size;
struct { uaecptr addr; uae_u32 value; bool read, instruction; int size, exception;
         bool hardware; } cpu_thread_fault;
bool cpu_thread_servicing;
uae_u32 cpu_thread_requests_served;
int cpu_thread_main_sleeping, cpu_thread_worker_sleeping, cpu_thread_reset;
constexpr uae_u64 CPU_THREAD_REPLY_SPIN_NS = 100000;
thread_local bool worker;
bool is_cpu_thread() { return worker; }
bool cpu_thread_diag_enabled() { return false; }
uae_u64 SDL_GetTicksNS() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
void cpu_thread_diag_log_wait(uae_u64) {}
void cpu_thread_spin_pause() { std::this_thread::yield(); }
int bankindex(uaecptr) { return 0; }
void hardware_exception2(uaecptr, uae_u32, bool, bool, int) { throw 2; }
unsigned reads, writes;
uae_u32 stored;
uae_u32 read(uaecptr addr) {
    assert(!worker && cpu_thread_servicing);
    ++reads;
    if (addr == 1) {
        cpu_thread_fault = {addr, 0, true, false, 1, 2, true};
        throw 2;
    }
    if (addr == 2) throw 3;
    return stored;
}
void write(uaecptr, uae_u32 value) {
    assert(!worker && cpu_thread_servicing);
    ++writes;
    stored = value;
}
struct addrbank {
    uae_u32 (*bget)(uaecptr) = read, (*wget)(uaecptr) = read, (*lget)(uaecptr) = read;
    void (*bput)(uaecptr, uae_u32) = write, (*wput)(uaecptr, uae_u32) = write,
         (*lput)(uaecptr, uae_u32) = write;
} bank;
addrbank *thread_mem_banks[] = {&bank};
// the register snapshot has its own test; nothing here is in it
bool cpu_thread_snap_read(const addrbank *, uaecptr, int, uae_u32 *) { return false; }
struct { unsigned armed; } cpu_thread_snap_main;
void cpu_thread_snap_arm(const addrbank *, uaecptr, int) {}
void cpu_thread_snap_publish(bool) { assert(false); }
uae_u32 cpu_thread_blit_held;   // no nasty blit here
bool cpu_thread_blit_nasty() { return false; }
// A pending mode change cannot cancel an instruction's hardware transaction.
constexpr unsigned SPCFLAG_MODE_CHANGE = 1, SPCFLAG_CALLBACK = 2;
struct { unsigned spcflags = SPCFLAG_MODE_CHANGE; } regs;
#define CPU_THREAD_RARE(x) (x)
#define NOINLINE __attribute__((noinline))
constexpr int CYCLE_UNIT = 512;
constexpr uae_u64 CPU_THREAD_SETTLE_NS = 5000;
""" + function(text, "static inline bool cpu_thread_reply_ready(") +
                "template <typename Ready>\n" +
                function(text, "static bool cpu_thread_settle(") +
                function(text, "static NOINLINE void cpu_thread_wait_reply_slow(") +
                function(text, "static inline void cpu_thread_wait_reply(") +
                wake_main(text) +
                function(text, "static inline void cpu_thread_post_request(") +
                function(text, "static inline void cpu_thread_post_reply(") +
                function(text, "static bool cpu_thread_main_has_work(") +
                function(text, "static void cpu_thread_main_sleep(") +
                function(text, "uae_u32 process_cpu_indirect_memory_read(") +
                function(text, "void process_cpu_indirect_memory_write(") +
                served_credit(text) +
                function(text, "static inline bool service_cpu_indirect_request(") + r"""
void transactions(unsigned count) {
    worker = true;
    for (unsigned value = 0; value < count; ++value) {
        process_cpu_indirect_memory_write(0, value, 2);
        assert(process_cpu_indirect_memory_read(0, 2) == value);
    }
}
int main() {
    std::atomic<bool> done{false};
    std::thread cpu([&] {
        transactions(100);
        worker = true;
        for (unsigned address : {1, 2}) {
            try {
                process_cpu_indirect_memory_read(address, 1);
                assert(false);
            } catch (int exc) {
                assert(exc == (address == 1 ? 2 : 3));
            }
        }
        // A fault response must not poison the next successful transaction.
        assert(process_cpu_indirect_memory_read(0, 2) == 99);
        done = true;
    });
    while (!done) {
        service_cpu_indirect_request();
        std::this_thread::yield();
    }
    cpu.join();
    assert(reads == 103 && writes == 100);
    assert(cpu_thread_requests_served == 203);
    assert(!cpu_thread_servicing);

    // Idle main thread: it sleeps between requests, and replies late enough
    // that the CPU thread sleeps too. Both wakeups must be delivered.
    regs.spcflags = 0;
    done = false;
    std::thread idle([&] { transactions(200); done = true; });
    for (unsigned served = 0; !done;) {
        if (service_cpu_indirect_request())
            continue;
        cpu_thread_main_sleep(1);
        if (__atomic_load_n(&cpu_thread_indirect_mode, __ATOMIC_ACQUIRE) <= 2
            && ++served % 8 == 0)
            std::this_thread::sleep_for(std::chrono::microseconds(300));
    }
    idle.join();
    assert(reads == 303 && writes == 300);
    assert(worker_sleeps > 0);
    assert(lost_wakeups == 0);   // SEQ_CST handshake: no wakeup is lost
    assert(!cpu_thread_main_sleeping && !cpu_thread_worker_sleeping);
}
""")

    def test_native_fgetexp_fallback_preserves_special_values(self):
        run_cpp(r"""
#include <cmath>
#include <limits>
using fptype = double;
struct fpdata { fptype fp; };
const fptype nan_value = std::numeric_limits<fptype>::quiet_NaN();
const fptype *fp_nan = &nan_value;
#define FP_FN(fn) std::fn
bool fp_is_zero(const fpdata *p) { return p->fp == 0; }
bool fp_is_nan(const fpdata *p) { return std::isnan(p->fp); }
bool fp_is_infinity(const fpdata *p) { return std::isinf(p->fp); }
void fp_normal_prec() {}
void fp_reset_normal_prec() {}
""" + function(source("src/fpp_native.cpp"), "static void fp_getexp(") + r"""
int main() {
    const double infinity = std::numeric_limits<double>::infinity();
    for (double value : {0.0, -0.0, 1.0, -1.0, 0.125, -8.0,
                        nan_value, infinity, -infinity}) {
        fpdata operand{value}, result{};
        fp_getexp(&result, &operand);
        if (!std::isfinite(value))
            assert(std::isnan(result.fp));
        else if (value == 0)
            assert(result.fp == 0 && std::signbit(result.fp) == std::signbit(value));
        else
            assert(result.fp == std::floor(std::log2(std::abs(value))));
    }
}
""")

    @unittest.skipUnless(platform.machine().lower() in ("x86_64", "amd64"),
                         "executes x86 SHR and the x86 JIT flag emitter")
    def test_x86_lsr_immediate_flags(self):
        generated = source("src/jit/x86_64/compemu_x86_64.cpp")
        handlers = "\n".join(function(generated, f"uae_u32 REGPARAM2 op_{op}_0_comp_ff(")
                             for op in ("e008", "e048", "e088"))
        emitter = function(source("src/jit/x86_64/compemu_midfunc_x86_64.cpp"),
                           "MIDFUNC(0,clear_overflow,")
        run_cpp(r"""
#include <sys/mman.h>
#define REGPARAM2
#define MIDFUNC(n, name, args) void emit_##name args
std::vector<uae_u8> bytes;
void make_flags_live_internal() {}
void raw_pushfl() { bytes.push_back(0x9c); }
void raw_popfl() { bytes.push_back(0x9d); }
void emit_byte(uae_u8 v) { bytes.push_back(v); }
""" + emitter + r"""
using Code = void (*)();
Code clear_code;
uae_u32 values[32], carry;
unsigned long flags;
uae_u32 m68k_pc_offset, needed_flags = 31;
int failure;
constexpr int S1 = 16, FLAG_CZNV = 15, SYNC_PC_OFFSET = 1024;
int imm8_table[] = {8, 1, 2, 3, 4, 5, 6, 7};
void dont_care_flags() {}
void start_needflags() {}
void live_flags() {}
void end_needflags() {}
void sync_m68k_pc() {}
void mov_l_ri(int d, uae_u32 v) { values[d] = v; }
void mov_b_rr(int d, int s) { values[d] = (values[d] & ~255u) | (values[s] & 255); }
void mov_w_rr(int d, int s) { values[d] = (values[d] & ~65535u) | (values[s] & 65535); }
void mov_l_rr(int d, int s) { values[d] = values[s]; }
void duplicate_carry() { carry = flags & 1; }
void clear_overflow() {
    unsigned long result;
    asm volatile("pushq %1; popfq; call *%2; pushfq; popq %0"
                 : "=&r"(result) : "r"(flags), "r"(clear_code) : "cc", "memory");
    flags = result;
}
void shrl_b_ri(int d, int count) {
    uae_u8 value = values[d];
    asm volatile("shrb %%cl,%b0; pushfq; popq %1"
                 : "+q"(value), "=r"(flags) : "c"(count) : "cc");
    values[d] = (values[d] & ~255u) | value;
}
void shrl_w_ri(int d, int count) {
    uae_u16 value = values[d];
    asm volatile("shrw %%cl,%w0; pushfq; popq %1"
                 : "+r"(value), "=r"(flags) : "c"(count) : "cc");
    values[d] = (values[d] & ~65535u) | value;
}
void shrl_l_ri(int d, int count) {
    asm volatile("shrl %%cl,%0; pushfq; popq %1"
                 : "+r"(values[d]), "=r"(flags) : "c"(count) : "cc");
}
""" + handlers + r"""
int main() {
    emit_clear_overflow();
    bytes.push_back(0xc3);
    void *page = mmap(nullptr, 4096, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(page != MAP_FAILED);
    std::memcpy(page, bytes.data(), bytes.size());
    assert(mprotect(page, 4096, PROT_READ | PROT_EXEC) == 0);
    clear_code = reinterpret_cast<Code>(page);
    using Handler = uae_u32 (*)(uae_u32);
    Handler shifts[] = {op_e008_0_comp_ff, op_e048_0_comp_ff, op_e088_0_comp_ff};
    int widths[] = {8, 16, 32};
    for (int w = 0; w < 3; ++w) for (int count = 1; count <= 8; ++count) {
        for (uae_u32 value : {0u, 1u, 0x80000000u, 0xffffffc1u, 0xffffffffu}) {
            uae_u32 mask = w == 2 ? ~0u : (1u << widths[w]) - 1;
            values[0] = value;
            shifts[w]((count & 7) << 9);
            assert(values[0] == ((value & ~mask) | ((value & mask) >> count)));
            assert((flags & 0x800) == 0);
            assert(carry == (((value & mask) >> (count - 1)) & 1));
            assert(bool(flags & 0x40) == (((value & mask) >> count) == 0));
        }
    }
    assert(munmap(page, 4096) == 0);
}
""")

    def test_rtc_active_accesses_preserve_carry_pulses_between_host_frames(self):
        text = source("src/rtc.cpp")
        projection = text[text.index("struct rtc_timebase {"):text.index("static uae_s64 rtc_microseconds(")]
        run_cpp(r"""
constexpr int CYCLE_UNIT = 512;
struct { int cs_rtc_adjust; } currprefs;
""" + projection + r"""
rtc_timebase base{};
uae_s64 host_us;
uae_u64 cycles;
int hz;
bool warp;
uae_s64 rtc_microseconds() {
    return rtc_project_microseconds(&base, host_us, cycles, hz, warp);
}
""" + source("src/include/rtc.h") + "\n" +
                text[text.index("static void clock_advance("):] + r"""
int main() {
    const uae_s64 epoch = 1700000000000000;
    for (int rate : {3546895, 3579545}) {
        hz = rate;
        base = {};
        cycles = 0;
        warp = false;
        rtc_msm_data msm{};
        msm.clock_control_f = 4;
        int pulses = 0, carried = 0;
        bool was_busy = false;
        for (uae_s64 elapsed = 0; elapsed < 3000000; elapsed += 16) {
            host_us = epoch + 305000 + elapsed / 20000 * 20000;
            assert(host_us % 1000000 >= 5000); // raw host polling misses every pulse
            cycles = elapsed * (uae_u64)hz * CYCLE_UNIT / 1000000;
            bool busy = (get_clock_msm(&msm, 13, nullptr) & 2) != 0;
            if (busy && !was_busy) {
                ++pulses;
                assert(msm.clock.value.tm_sec == ((epoch / 1000000 + ++carried) % 60));
            }
            was_busy = busy;
            assert(busy == (msm.clock.phase_us < 244));
            assert(msm.clock.phase_us == (305000 + elapsed - (elapsed ? 1 : 0)) % 1000000 ||
                   msm.clock.phase_us == (305000 + elapsed) % 1000000);
        }
        assert(pulses == 3);
        // No emulated time, no RTC time (a real pause calls rtc_rebase).
        uae_s64 before = rtc_microseconds();
        host_us += 7000000;
        assert(rtc_microseconds() == before);
        // A cycle counter reset rejoins the host calendar.
        cycles = 0;
        host_us += 1;
        assert(rtc_microseconds() == host_us);
        // Warp retains the wall-clock RTC instead of accelerating it.
        warp = true;
        for (int i = 0; i < 100; ++i) {
            cycles += (uae_u64)hz * CYCLE_UNIT * 4;
            host_us += 5000;
            assert(rtc_microseconds() == host_us);
        }
        warp = false;
        // Fractional cycles must accumulate rather than lose microseconds.
        base = {};
        cycles = 0;
        uae_s64 start = rtc_microseconds();
        for (int i = 1; i < 10000; ++i) {
            cycles += 3265;
            assert(rtc_microseconds() == start + cycles * 1000000 / ((uae_u64)hz * CYCLE_UNIT));
        }
        rtc_base = base;
        rtc_rebase();
        assert(!rtc_base.initialized);
        host_us += 500000; // even a subsecond pause rejoins wall time
        assert(rtc_project_microseconds(&rtc_base, host_us, cycles, hz, false) == host_us);
    }
}
""")
    def test_rtc_follows_emulated_time_and_slews_to_the_host(self):
        text = source("src/rtc.cpp")
        projection = text[text.index("struct rtc_timebase {"):text.index("static uae_s64 rtc_microseconds(")]
        run_cpp(r"""
#include <algorithm>
constexpr int CYCLE_UNIT = 512;
""" + projection + r"""
int main() {
    const uae_s64 epoch = 1700000000000000;
    for (int hz : {3546895, 3579545}) {
        const uae_u64 per_s = (uae_u64)hz * CYCLE_UNIT;
        // Quarter, normal, four and sixteen times speed, and stalled: for
        // 10 s of host time the RTC advances by the emulated time, plus at
        // most 1 % slew towards the host.
        for (int speed : {0, 1, 4, 16, 64}) {
            rtc_timebase base{};
            uae_u64 cycles = 0;
            uae_s64 previous = rtc_project_microseconds(&base, epoch, cycles, hz, false);
            assert(previous == epoch);
            for (int i = 1; i <= 2000; ++i) {
                uae_s64 host = epoch + i * 5000;
                cycles = (uae_u64)i * 5000 * per_s * speed / 4000000;
                uae_s64 sample = rtc_project_microseconds(&base, host, cycles, hz, false);
                assert(sample >= previous);
                previous = sample;
            }
            const uae_s64 emulated = 10000000LL * speed / 4;
            const uae_s64 got = previous - epoch;
            assert(got >= emulated - emulated / 100 - 1 && got <= emulated + emulated / 100 + 1);
            if (speed == 0)
                assert(got == 0);
        }
        // 0.5 % slower than real time for 10 minutes: the slew keeps the
        // RTC within about a second of the host clock.
        {
            rtc_timebase base{};
            rtc_project_microseconds(&base, epoch, 0, hz, false);
            uae_s64 sample = 0;
            for (int i = 1; i <= 60000; ++i) {
                uae_s64 host = epoch + (uae_s64)i * 10000;
                uae_u64 cycles = (uae_u64)i * 10000 * per_s * 995 / 1000 / 1000000;
                sample = rtc_project_microseconds(&base, host, cycles, hz, false);
            }
            uae_s64 drift = epoch + 600000000LL - sample;
            assert(drift >= 0 && drift <= 1100000);
        }
        // A host clock jump of over an hour re-anchors; warp keeps the host clock.
        {
            rtc_timebase base{};
            rtc_project_microseconds(&base, epoch, 0, hz, false);
            uae_s64 host = epoch + 2LL * 3600 * 1000000;
            assert(rtc_project_microseconds(&base, host, per_s / 1000000 * 20000, hz, false) == host);
            host += 5000;
            assert(rtc_project_microseconds(&base, host, per_s / 1000000 * 20000 * 50, hz, true) == host);
        }
    }
}
""")

    def test_rtc_file_calendar_restores_guest_time(self):
        text = source("src/rtc.cpp")
        run_cpp(r"""
#include <cstdlib>
struct { int cs_rtc_adjust; } currprefs;
uae_s64 now_us = 1700000000700000;  // 2023-11-14 22:13:20.7 UTC
uae_s64 rtc_microseconds() { return now_us; }
""" + source("src/include/rtc.h") + "\n" +
                text[text.index("static void clock_advance("):] + r"""
// MSM6242B digits 0-12: S1 S10 MI1 MI10 H1 H10 D1 D10 MO1 MO10 Y1 Y10 W
static void msm_digits(uae_u8 *d, const struct tm &t, bool mode24) {
    int h = t.tm_hour, h10;
    if (mode24) { h10 = h / 10; }
    else { int h12 = h % 12 ? h % 12 : 12; h10 = h12 / 10 | (h >= 12 ? 4 : 0); h = h12; }
    int y = t.tm_year % 100;
    uae_u8 v[13] = { uae_u8(t.tm_sec % 10), uae_u8(t.tm_sec / 10), uae_u8(t.tm_min % 10), uae_u8(t.tm_min / 10),
        uae_u8(h % 10), uae_u8(h10), uae_u8(t.tm_mday % 10), uae_u8(t.tm_mday / 10),
        uae_u8((t.tm_mon + 1) % 10), uae_u8((t.tm_mon + 1) / 10), uae_u8(y % 10), uae_u8(y / 10), uae_u8(t.tm_wday) };
    memcpy(d, v, 13);
}
int main() {
    setenv("TZ", "UTC", 1);
    tzset();
    const uae_s64 age = 3600LL * 1000000;
    uae_u8 d[16] = {};
    // The host's calendar when the file was written: keep following the host.
    {
        time_t then = (time_t)((now_us - age) / 1000000);
        struct tm t = *gmtime(&then);
        msm_digits(d, t, true);
        rtc_msm_data data{};
        data.clock_control_f = 4;
        auto &c = data.clock;
        assert(!rtc_restore_calendar(&data, d, age));
        assert(!c.initialized && !c.written);
        d[12] = (t.tm_wday + 1) % 7;
        assert(rtc_restore_calendar(&data, d, age));
        assert(c.written && c.value.tm_wday == d[12]);
        rtc_ricoh_data rf{};
        rf.clock_control_d = 8;
        rf.rtc_alarm[10] = 1;
        rf.rtc_alarm[11] = (t.tm_year + 1) & 3;
        uae_u8 digits[13];
        for (int i = 0; i < 13; ++i)
            digits[i] = get_clock_ricoh(&rf, i, &t);
        assert(rtc_restore_calendar(&rf, digits, age));
        assert(rf.clock.written && rf.rtc_alarm[11] == ((t.tm_year + 1) & 3));
    }
    // A guest-set time comes back moved on by the time the file is old.
    for (bool mode24 : {true, false}) for (int hour : {0, 4, 12, 23}) {
        struct tm t = {};
        t.tm_year = 101; t.tm_mon = 1; t.tm_mday = 3; t.tm_hour = hour; t.tm_min = 5; t.tm_sec = 6; t.tm_wday = 6;
        msm_digits(d, t, mode24);
        rtc_msm_data data{};
        data.clock_control_f = mode24 ? 4 : 0;
        auto &c = data.clock;
        assert(rtc_restore_calendar(&data, d, age));
        assert(c.initialized && c.written);
        assert(c.value.tm_year == 101 && c.value.tm_mon == 1);
        assert(c.value.tm_hour == (hour + 1) % 24);
        assert(c.value.tm_mday == (hour == 23 ? 4 : 3));
        assert(c.value.tm_min == 5 && c.value.tm_sec == 6);
        assert(c.phase_us == now_us % 1000000);
    }
    // RF5C01A layout: the weekday is register 6, then day, month, year.
    {
        struct tm t = {};
        t.tm_year = 99; t.tm_mon = 11; t.tm_mday = 31; t.tm_hour = 23; t.tm_min = 59; t.tm_sec = 30; t.tm_wday = 5;
        uae_u8 m[13];
        msm_digits(m, t, true);
        uae_u8 rf[13] = { m[0], m[1], m[2], m[3], m[4], m[5], m[12], m[6], m[7], m[8], m[9], m[10], m[11] };
        rtc_ricoh_data data{};
        data.clock_control_d = 8;
        data.rtc_alarm[10] = 1;
        data.rtc_alarm[11] = 3;
        auto &c = data.clock;
        assert(rtc_restore_calendar(&data, rf, age));
        assert(c.value.tm_year == 100 && c.value.tm_mon == 0 && c.value.tm_mday == 1);
        assert(c.value.tm_hour == 0 && c.value.tm_min == 59 && c.value.tm_sec == 30);
        assert(data.rtc_alarm[11] == 0);
    }
    // Digits that are no time are ignored.
    {
        uae_u8 bad[13] = { 9, 9, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0 };  // 99 seconds
        rtc_msm_data data{};
        data.clock_control_f = 4;
        assert(!rtc_restore_calendar(&data, bad, age) && !data.clock.initialized);
    }
    // Both hour modes retain an independent weekday, stopped calendars,
    // and the Ricoh's independently programmed leap counter.
    for (bool mode24 : {false, true}) {
        struct tm t = {};
        t.tm_year = 101; t.tm_mon = 1; t.tm_mday = 28;
        t.tm_hour = 23; t.tm_min = 59; t.tm_sec = 59; t.tm_wday = 2;
        msm_digits(d, t, mode24);
        for (uae_s64 offline : {0LL, 1000000LL, 3600000000LL}) {
            for (int stop : {1, 2, 3}) {
                rtc_msm_data data{};
                data.clock_control_f = (mode24 ? 4 : 0) | stop;
                assert(rtc_restore_calendar(&data, d, offline));
                assert(data.clock.value.tm_hour == 23 && data.clock.value.tm_sec == 59);
                assert(data.clock.value.tm_mday == 28 && data.clock.value.tm_wday == 2);
                now_us += 2000000;
                get_clock_msm(&data, 13, nullptr);
                assert(data.clock.value.tm_sec == 59);
                put_clock_msm(&data, 15, mode24 ? 4 : 0);
                now_us += 1000000;
                get_clock_msm(&data, 13, nullptr);
                assert(data.clock.value.tm_sec == 0 && data.clock.value.tm_mon == 2);
                assert(data.clock.value.tm_mday == 1 && data.clock.value.tm_wday == 3);
            }
        }
        {
            rtc_msm_data data{};
            data.clock_control_f = mode24 ? 4 : 0;
            assert(rtc_restore_calendar(&data, d, 0));
            assert(data.clock.value.tm_wday == 2);
        }
        for (bool stopped : {false, true}) {
            rtc_msm_data data{};
            data.clock_control_d = 1;
            data.clock_control_f = (mode24 ? 4 : 0) | (stopped ? 2 : 0);
            assert(rtc_restore_calendar(&data, d, age));
            assert(data.clock.value.tm_sec == 59 && data.clock.held_value.tm_sec == 59);
            assert(data.clock.carry_pending == !stopped);
            put_clock_msm(&data, 13, 0);
            assert(data.clock.value.tm_sec == (stopped ? 59 : 0));
            assert(data.clock.value.tm_mon == (stopped ? 1 : 2));
            assert(data.clock.value.tm_wday == (stopped ? 2 : 3));
        }
        uae_u8 rf[13] = { d[0], d[1], d[2], d[3], d[4],
            uae_u8(mode24 ? d[5] : (d[5] & 1) | ((d[5] & 4) ? 2 : 0)), d[12],
            d[6], d[7], d[8], d[9], d[10], d[11] };
        for (int bank = 0; bank < 4; ++bank) for (bool running : {false, true}) {
            rtc_ricoh_data data{};
            data.clock_control_d = bank | (running ? 8 : 0);
            data.rtc_alarm[10] = mode24 ? 1 : 0;
            data.rtc_alarm[11] = 0;  // 2001 is a leap year on this programmed chip.
            assert(rtc_restore_calendar(&data, rf, 1000000));
            assert(data.clock.value.tm_mday == (running ? 29 : 28));
            assert(data.clock.value.tm_wday == (running ? 3 : 2));
            assert(data.clock.value.tm_sec == (running ? 0 : 59));
            assert(data.rtc_alarm[11] == 0);
            if (!running) {
                now_us += 2000000;
                put_clock_ricoh(&data, 13, 8 | bank);
                now_us += 1000000;
                get_clock_ricoh(&data, 13, nullptr);
                assert(data.clock.value.tm_mday == 29 && data.clock.value.tm_wday == 3);
                assert(data.clock.value.tm_sec == 0);
            }
        }
    }
    // Even a calendar matching the host must remain frozen when stopped.
    {
        time_t then = (time_t)((now_us - age) / 1000000);
        struct tm t = *localtime(&then);
        msm_digits(d, t, true);
        rtc_msm_data data{};
        data.clock_control_f = 6;
        assert(rtc_restore_calendar(&data, d, age));
        assert(data.clock.value.tm_hour == t.tm_hour && data.clock.value.tm_min == t.tm_min);
    }
    // The chip counts calendar days, not the host's daylight-saving jumps.
    {
        setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0/2", 1);
        tzset();
        struct tm t = {};
        t.tm_year = 124; t.tm_mon = 2; t.tm_mday = 9; t.tm_hour = 12; t.tm_wday = 2;
        msm_digits(d, t, true);
        rtc_msm_data data{};
        data.clock_control_f = 4;
        assert(rtc_restore_calendar(&data, d, 86400000000LL));
        assert(data.clock.value.tm_mday == 10 && data.clock.value.tm_hour == 12);
        assert(data.clock.value.tm_wday == 3);
    }
}
""")

    def test_rtc_savestate_samples_and_restores_the_save_instant(self):
        rtc = source("src/rtc.cpp")
        cia = source("src/cia.cpp")
        serializers = "\n".join(function(cia, signature) for signature in (
            "static uae_u8 getclockreg(", "static void save_rtc_tm(",
            "static uae_u8 *restore_rtc_tm(", "void restore_rtc_start(",
            "uae_u8 *save_rtc(", "uae_u8 *restore_rtc(",
        ))
        run_cpp(r"""
#include <cstdlib>
struct { int cs_rtc, cs_rtc_adjust; } currprefs;
uae_s64 now_us = 1700000000700000;
uae_s64 rtc_microseconds() { return now_us; }
""" + source("src/include/rtc.h") + "\n" +
                rtc[rtc.index("static void clock_advance("):] + r"""
rtc_msm_data rtc_msm{};
rtc_ricoh_data rtc_ricoh{};
bool rtc_state_restored;
#define xmalloc(type, count) ((type *)malloc(sizeof(type) * (count)))
void push32(uae_u8 *&p, uae_u32 v) {
    for (int i = 3; i >= 0; --i) *p++ = v >> (i * 8);
}
uae_u32 pull32(uae_u8 *&p) {
    uae_u32 v = 0;
    for (int i = 0; i < 4; ++i) v = (v << 8) | *p++;
    return v;
}
void push16(uae_u8 *&p, uae_u16 v) { *p++ = v >> 8; *p++ = v; }
uae_u16 pull16(uae_u8 *&p) { uae_u16 v = *p++; return (v << 8) | *p++; }
#define save_u32(v) push32(dst, (v))
#define restore_u32() pull32(src)
#define save_u16(v) push16(dst, (v))
#define restore_u16() pull16(src)
#define save_u8(v) (*dst++ = (v))
#define restore_u8() (*src++)
""" + serializers + r"""
void same_tm(const struct tm &a, const struct tm &b) {
    assert(a.tm_sec == b.tm_sec && a.tm_min == b.tm_min && a.tm_hour == b.tm_hour);
    assert(a.tm_mday == b.tm_mday && a.tm_mon == b.tm_mon && a.tm_year == b.tm_year);
    assert(a.tm_wday == b.tm_wday);
}
int main() {
    for (int chip : {1, 2, 3}) for (int state = 0; state < 5; ++state) {
        currprefs.cs_rtc = chip;
        now_us = 1700000000700000;
        rtc_msm = {};
        rtc_ricoh = {};
        const bool ricoh = chip == 2;
        const bool stopped = ricoh ? state == 4 : state == 1 || state == 2 || state == 4;
        const bool held = !ricoh && state >= 3;
        rtc_msm.clock_control_d = held ? 1 : 0;
        rtc_msm.clock_control_f = 4 | (state == 2 ? 1 : stopped ? 2 : 0);
        rtc_ricoh.clock_control_d = state < 4 ? 8 | state : 3;
        rtc_ricoh.rtc_alarm[10] = 1;
        rtc_ricoh.rtc_alarm[11] = 1;
        rtc_ricoh.rtc_memory[3] = 0xab;
        auto &c = ricoh ? rtc_ricoh.clock : rtc_msm.clock;
        c.initialized = c.written = c.hour12_written = true;
        c.hour12 = 11;
        c.last_us = now_us;
        c.phase_us = 700000;
        c.value.tm_year = 123; c.value.tm_mon = 11; c.value.tm_mday = 31;
        c.value.tm_hour = 23; c.value.tm_min = 59; c.value.tm_sec = 58; c.value.tm_wday = 2;
        c.held_value = c.value;
        now_us += 5500000;
        uae_u8 buffer[200];
        size_t length = 0;
        assert(save_rtc(&length, buffer) == buffer && length <= sizeof buffer);
        assert(c.last_us == now_us);
        if (stopped || held) {
            assert(c.value.tm_year == 123 && c.value.tm_sec == 58 && c.phase_us == (stopped ? 700000 : 200000));
            assert(c.hour12_written && c.carry_pending == (held && !stopped));
        } else {
            assert(c.value.tm_year == 124 && c.value.tm_mon == 0 && c.value.tm_mday == 1);
            assert(c.value.tm_hour == 0 && c.value.tm_min == 0 && c.value.tm_sec == 4);
            assert(c.value.tm_wday == 3 && c.phase_us == 200000 && !c.hour12_written);
        }
        const auto expected = c;
        const auto msm = rtc_msm;
        const auto rf = rtc_ricoh;
        now_us += 3600000000LL;
        rtc_msm = {};
        rtc_ricoh = {};
        restore_rtc_start();
        assert(!rtc_msm.clock.initialized && !rtc_ricoh.clock.initialized && !rtc_state_restored);
        assert(restore_rtc(buffer) == buffer + length && rtc_state_restored);
        assert(c.initialized && c.written && c.last_us == now_us);
        same_tm(c.value, expected.value);
        same_tm(c.held_value, expected.held_value);
        assert(c.phase_us == expected.phase_us && c.hour12 == expected.hour12);
        assert(c.hour12_written == expected.hour12_written && c.carry_pending == expected.carry_pending);
        if (ricoh) {
            assert(rtc_ricoh.clock_control_d == rf.clock_control_d);
            assert(rtc_ricoh.rtc_alarm[11] == (stopped ? 1 : 2));
            assert(!memcmp(rtc_ricoh.rtc_alarm, rf.rtc_alarm, sizeof rf.rtc_alarm));
            assert(!memcmp(rtc_ricoh.rtc_memory, rf.rtc_memory, sizeof rf.rtc_memory));
        } else {
            assert(rtc_msm.clock_control_d == msm.clock_control_d);
            assert(rtc_msm.clock_control_f == msm.clock_control_f);
        }
    }
    restore_rtc_start();
    currprefs.cs_rtc = 1;
    rtc_msm.clock_control_d = 0;
    rtc_msm.clock_control_f = 4;
    getclockreg(13, nullptr);
    const time_t host = now_us / 1000000;
    assert(rtc_msm.clock.initialized && !rtc_msm.clock.written);
    same_tm(rtc_msm.clock.value, *localtime(&host));
}
""")

    def test_rtc_hold_writes_and_banked_leap_reads(self):
        text = source("src/rtc.cpp")
        run_cpp(r"""
struct { int cs_rtc_adjust; } currprefs;
uae_s64 now_us = 1700000000700000;
uae_s64 rtc_microseconds() { return now_us; }
""" + source("src/include/rtc.h") + "\n" +
                text[text.index("static void clock_advance("):] + r"""
int main() {
    rtc_msm_data msm{};
    msm.clock_control_f = 4;
    clock_sample(&msm.clock, true, nullptr);
    msm.clock.value.tm_sec = 10;
    msm.clock.written = true;
    put_clock_msm(&msm, 13, 1);
    now_us += 400000;
    assert(get_clock_msm(&msm, 0, nullptr) == 0);
    put_clock_msm(&msm, 12, 4);
    assert(get_clock_msm(&msm, 0, nullptr) == 0);
    assert(get_clock_msm(&msm, 12, nullptr) == 4);
    assert(get_clock_msm(&msm, 13, nullptr) == 1);
    put_clock_msm(&msm, 13, 0);
    assert(get_clock_msm(&msm, 0, nullptr) == 1);
    assert(get_clock_msm(&msm, 12, nullptr) == 4);
    put_clock_msm(&msm, 13, 1);
    now_us += 100000;
    put_clock_msm(&msm, 0, 7);
    assert(get_clock_msm(&msm, 0, nullptr) == 7);
    put_clock_msm(&msm, 13, 0);
    assert(get_clock_msm(&msm, 0, nullptr) == 7);

    // A held 12-hour write retains its staging across a carry.
    put_clock_msm(&msm, 15, 0);
    put_clock_msm(&msm, 13, 1);
    put_clock_msm(&msm, 4, 1);
    now_us += 1000000;
    put_clock_msm(&msm, 5, 1);
    assert(get_clock_msm(&msm, 4, nullptr) == 1);
    assert(get_clock_msm(&msm, 5, nullptr) == 1);
    put_clock_msm(&msm, 13, 0);
    assert(get_clock_msm(&msm, 4, nullptr) == 1);
    assert(get_clock_msm(&msm, 5, nullptr) == 1);

    for (int bank : {1, 2, 3}) {
        rtc_ricoh_data ricoh{};
        ricoh.clock_control_d = 8 | bank;
        ricoh.clock.initialized = ricoh.clock.written = true;
        ricoh.clock.last_us = now_us;
        ricoh.clock.value.tm_year = 123;
        ricoh.clock.value.tm_mon = 11;
        ricoh.clock.value.tm_mday = 31;
        ricoh.clock.value.tm_hour = 23;
        ricoh.clock.value.tm_min = 59;
        ricoh.clock.value.tm_sec = 59;
        ricoh.rtc_alarm[11] = 3;
        ricoh.rtc_memory[11] = 0xab;
        now_us += 2000000;
        int value = get_clock_ricoh(&ricoh, 11, nullptr);
        assert(value == (bank == 1 ? 0 : bank == 2 ? 11 : 10));
        assert(ricoh.rtc_alarm[11] == 0);
        assert(ricoh.clock.value.tm_year == 124);
        assert(ricoh.clock.value.tm_sec == 1);
    }
}
""")
        for path, signatures in {
            "src/devices.cpp": ("void devices_reset(", "void devices_restore_start(", "void devices_unpause("),
            "src/inputdevice.cpp": ("void warpmode(",),
        }.items():
            for signature in signatures:
                self.assertIn("rtc_rebase();", function(source(path), signature))

    def test_rtc_digits_and_hour_modes(self):
        text = source("src/rtc.cpp")
        run_cpp(r"""
struct { int cs_rtc_adjust; } currprefs;
uae_s64 rtc_microseconds() { return 1700000000000000; }
""" + source("src/include/rtc.h") + "\n" +
                text[text.index("static void clock_advance("):] + r"""
int main() {
    rtc_msm_data msm{};
    rtc_ricoh_data ricoh{};
    tm date{};
    for (int year : {1999, 2000, 2026, 2099, 2100}) {
        date.tm_year = year - 1900;
        for (bool mode : {false, true}) {
            msm.yearmode = mode;
            assert(get_clock_msm(&msm, 10, &date) == year % 10);
            assert(get_clock_msm(&msm, 11, &date) == (year % 100) / 10);
        }
    }
    for (int hour = 0; hour < 24; ++hour) {
        date.tm_hour = hour;
        for (bool mode24 : {false, true}) {
            msm.clock_control_f = mode24 ? 4 : 0;
            ricoh.rtc_alarm[10] = mode24 ? 1 : 0;
            int expected = mode24 ? hour : (hour % 12 ? hour % 12 : 12);
            assert(get_clock_msm(&msm, 4, &date) == expected % 10);
            assert(get_clock_msm(&msm, 5, &date) ==
                   (expected / 10 | (!mode24 && hour >= 12 ? 4 : 0)));
            assert(get_clock_ricoh(&ricoh, 4, &date) == expected % 10);
            assert(get_clock_ricoh(&ricoh, 5, &date) ==
                   (expected / 10 | (!mode24 && hour >= 12 ? 2 : 0)));
        }
    }
    ricoh.clock_control_d = 2;
    ricoh.rtc_memory[3] = 0xab;
    assert(get_clock_ricoh(&ricoh, 3, &date) == 11);
    ricoh.clock_control_d = 3;
    assert(get_clock_ricoh(&ricoh, 3, &date) == 10);
}
""")

    def test_rtc_writable_counters_hold_stop_phase_and_calendar(self):
        text = source("src/rtc.cpp")
        run_cpp(r"""
struct { int cs_rtc_adjust; } currprefs;
uae_s64 now_us = 1700000000700000;
uae_s64 rtc_microseconds() { return now_us; }
""" + source("src/include/rtc.h") + "\n" +
                text[text.index("static void clock_advance("):] + r"""
void msm_set(rtc_msm_data *data, tm value) {
    put_clock_msm(data, 15, 7);
    for (int i = 0; i < 13; ++i)
        put_clock_msm(data, i, get_clock_msm(data, i, &value));
}
void ricoh_set(rtc_ricoh_data *data, tm value, int leap) {
    put_clock_ricoh(data, 13, 1);
    put_clock_ricoh(data, 11, leap);
    put_clock_ricoh(data, 13, 0);
    put_clock_ricoh(data, 15, 2);
    for (int i = 0; i < 13; ++i)
        put_clock_ricoh(data, i, get_clock_ricoh(data, i, &value));
}
int main() {
    rtc_msm_data msm{};
    msm.clock_control_d = 1;
    msm.clock_control_f = 4;
    assert(get_clock_msm(&msm, 8, nullptr) > 0);
    put_clock_msm(&msm, 13, 0);
    rtc_ricoh_data ricoh{};
    ricoh.clock_control_d = 8;
    ricoh.rtc_alarm[10] = 1;
    tm date{};
    date.tm_sec = 42; date.tm_min = 37; date.tm_hour = 13;
    date.tm_mday = 14; date.tm_mon = 6; date.tm_year = 95; date.tm_wday = 5;
    msm_set(&msm, date);
    ricoh_set(&ricoh, date, 3);
    now_us += 1500000;
    for (int i = 0; i < 13; ++i) {
        assert(get_clock_msm(&msm, i, nullptr) == get_clock_msm(&msm, i, &date));
        assert(get_clock_ricoh(&ricoh, i, nullptr) == get_clock_ricoh(&ricoh, i, &date));
    }
    put_clock_msm(&msm, 15, 4);
    put_clock_ricoh(&ricoh, 13, 8);
    now_us += 999999;
    assert(get_clock_msm(&msm, 0, nullptr) == 2);
    assert(get_clock_ricoh(&ricoh, 0, nullptr) == 2);
    ++now_us;
    assert(get_clock_msm(&msm, 0, nullptr) == 3);
    assert(get_clock_ricoh(&ricoh, 0, nullptr) == 3);
    assert(get_clock_msm(&msm, 13, nullptr) & 2);
    rtc_msm_data pulse = msm;
    uae_s64 pulse_start = now_us;
    for (int phase : {1, 243, 244, 999999, 1000000, 1000243, 1000244}) {
        now_us = pulse_start + phase;
        assert(bool(get_clock_msm(&pulse, 13, nullptr) & 2) == (phase % 1000000 < 244));
    }
    now_us = pulse_start;
    now_us += 450000;
    put_clock_msm(&msm, 13, 1);
    now_us += 850000;
    assert(get_clock_msm(&msm, 0, nullptr) == 3);
    put_clock_msm(&msm, 13, 0);
    assert(get_clock_msm(&msm, 0, nullptr) == 4);
    now_us += 700000;
    assert(get_clock_msm(&msm, 0, nullptr) == 5);
    assert(get_clock_ricoh(&ricoh, 0, nullptr) == 5);
    for (int month : {1, 3, 11}) for (int year : {23, 24, 99}) {
        date.tm_sec = date.tm_min = 59; date.tm_hour = 23;
        date.tm_mon = month; date.tm_year = year; date.tm_wday = 6;
        date.tm_mday = month == 1 ? 28 : month == 3 ? 30 : 31;
        msm_set(&msm, date);
        ricoh_set(&ricoh, date, year & 3);
        put_clock_msm(&msm, 15, 4);
        put_clock_ricoh(&ricoh, 13, 8);
        now_us += 1000000;
        int day = month == 1 && year == 24 ? 29 : 1;
        assert(get_clock_msm(&msm, 6, nullptr) == day % 10);
        assert(get_clock_msm(&msm, 7, nullptr) == day / 10);
        assert(get_clock_msm(&msm, 12, nullptr) == 0);
        assert(get_clock_ricoh(&ricoh, 7, nullptr) == day % 10);
        assert(get_clock_ricoh(&ricoh, 8, nullptr) == day / 10);
        assert(get_clock_ricoh(&ricoh, 6, nullptr) == 0);
        if (month == 11) {
            put_clock_ricoh(&ricoh, 13, 9);
            assert(get_clock_ricoh(&ricoh, 11, nullptr) == ((year + 1) & 3));
        }
    }
    for (int seconds : {20, 40}) {
        date.tm_sec = seconds; date.tm_min = 37; date.tm_hour = 13;
        date.tm_mday = 14; date.tm_mon = 6; date.tm_year = 95;
        msm_set(&msm, date);
        put_clock_msm(&msm, 15, 4);
        put_clock_msm(&msm, 13, 8);
        assert(get_clock_msm(&msm, 0, nullptr) == 0);
        assert(get_clock_msm(&msm, 1, nullptr) == 0);
        assert(get_clock_msm(&msm, 2, nullptr) == (seconds < 30 ? 7 : 8));
        assert(!(get_clock_msm(&msm, 13, nullptr) & 8));
    }
    for (int initial = 0; initial < 24; ++initial) {
        for (int hour = 0; hour < 24; ++hour) for (int order = 0; order < 2; ++order) {
            date.tm_hour = initial;
            msm_set(&msm, date);
            ricoh.rtc_alarm[10] = 1;
            ricoh_set(&ricoh, date, 3);
            put_clock_msm(&msm, 15, 3);
            put_clock_ricoh(&ricoh, 13, 1);
            put_clock_ricoh(&ricoh, 10, 0);
            put_clock_ricoh(&ricoh, 13, 0);
            date.tm_hour = hour;
            for (int j = 0; j < 2; ++j) {
                int reg = 4 + (j ^ order);
                put_clock_msm(&msm, reg, get_clock_msm(&msm, reg, &date));
                put_clock_ricoh(&ricoh, reg, get_clock_ricoh(&ricoh, reg, &date));
            }
            assert(msm.clock.value.tm_hour == hour);
            assert(ricoh.clock.value.tm_hour == hour);
        }
    }
    for (int hour = 0; hour < 24; ++hour) {
        put_clock_msm(&msm, 15, 3);
        put_clock_ricoh(&ricoh, 13, 1);
        put_clock_ricoh(&ricoh, 10, 0);
        put_clock_ricoh(&ricoh, 13, 0);
        date.tm_hour = hour;
        date.tm_min = date.tm_sec = 59;
        for (int i = 0; i < 13; ++i) {
            put_clock_msm(&msm, i, get_clock_msm(&msm, i, &date));
            put_clock_ricoh(&ricoh, i, get_clock_ricoh(&ricoh, i, &date));
        }
        for (int i : {4, 5}) {
            assert(get_clock_msm(&msm, i, nullptr) == get_clock_msm(&msm, i, &date));
            assert(get_clock_ricoh(&ricoh, i, nullptr) == get_clock_ricoh(&ricoh, i, &date));
        }
        put_clock_msm(&msm, 15, 0);
        put_clock_ricoh(&ricoh, 13, 8);
        now_us += 1000000;
        tm next = date;
        next.tm_hour = (hour + 1) % 24;
        for (int i : {4, 5}) {
            assert(get_clock_msm(&msm, i, nullptr) == get_clock_msm(&msm, i, &next));
            assert(get_clock_ricoh(&ricoh, i, nullptr) == get_clock_ricoh(&ricoh, i, &next));
        }
        put_clock_ricoh(&ricoh, 13, 0);
        put_clock_ricoh(&ricoh, 15, 2);
    }
    ricoh.rtc_alarm[10] = 1;
    date.tm_hour = 23; date.tm_min = date.tm_sec = 59;
    date.tm_mday = 28; date.tm_mon = 1; date.tm_year = 24;
    ricoh_set(&ricoh, date, 3);
    put_clock_ricoh(&ricoh, 13, 8);
    now_us += 1000000;
    assert(get_clock_ricoh(&ricoh, 7, nullptr) == 1);
    assert(get_clock_ricoh(&ricoh, 9, nullptr) == 3);
    put_clock_ricoh(&ricoh, 13, 10);
    put_clock_ricoh(&ricoh, 3, 0xb);
    put_clock_ricoh(&ricoh, 13, 11);
    put_clock_ricoh(&ricoh, 3, 0xa);
    assert(get_clock_ricoh(&ricoh, 3, nullptr) == 0xa);
    put_clock_ricoh(&ricoh, 13, 10);
    assert(get_clock_ricoh(&ricoh, 3, nullptr) == 0xb);
    put_clock_ricoh(&ricoh, 13, 9);
    for (int leap = 0; leap < 4; ++leap) {
        put_clock_ricoh(&ricoh, 11, leap);
        assert(get_clock_ricoh(&ricoh, 11, nullptr) == leap);
    }
}
""")

    def test_misc_dispatch_ignores_inactive_events(self):
        text = source("src/events.cpp")
        start = text.index("static int event2max;")
        end = text.index("void event2_newevent_xx_ce", start)
        run_cpp(r"""
using evt_t = uint64_t;
constexpr evt_t EVT_MAX = ~evt_t(0);
constexpr int ev_misc = 0;
struct ev { bool active; evt_t oldcycles, evtime; } eventtab[1];
struct ev2 { bool active; evt_t evtime; void (*handler)(uae_u32); uae_u32 data; };
ev2 eventtab2[4];
evt_t now;
evt_t get_cycles() { return now; }
""" + text[start:end] + r"""
int calls[4];
void callback(uae_u32 n) { ++calls[n]; }
void reschedule(uae_u32 n) {
    ++calls[n];
    eventtab2[1] = {true, now + 5, callback, 1};
}
int main() {
    event2max = 4;
    firstvent2idx = -1;
    now = 42;
    for (int i = 0; i < 4; ++i) eventtab2[i] = {false, now, callback, uae_u32(i)};
    eventtab2[1].active = true;
    MISC_handler();
    assert(calls[0] == 0 && calls[1] == 1 && calls[2] == 0 && calls[3] == 0);
    MISC_handler();
    assert(calls[1] == 1);
    eventtab2[1] = {true, now, reschedule, 1};
    events_schedule_misc();
    MISC_handler();
    assert(calls[1] == 2 && eventtab2[1].active);
    assert(eventtab[ev_misc].active && eventtab[ev_misc].evtime == now + 5);
    now += 5;
    MISC_handler();
    assert(calls[1] == 3 && !eventtab2[1].active);
}
""")

    def test_mmu_atc_page_offsets_and_function_codes(self):
        text = source("src/cpummu30.cpp")
        run_cpp(r"""
constexpr int ATC030_NUM_ENTRIES = 4, TT_OK_MATCH = 1;
constexpr int MMUSR_INVALID = 1, MMUSR_TRANSP_ACCESS = 2, MMUSR_BUS_ERROR = 4;
constexpr int MMUSR_WRITE_PROTECTED = 8, MMUSR_MODIFIED = 16;
struct ATC {
    struct { uae_u32 addr, fc; bool valid; } logical;
    struct { bool bus_error, write_protect, modified; } physical;
};
struct {
    int status;
    struct { struct { uae_u32 imask; } page; } translation;
    ATC atc[ATC030_NUM_ENTRIES];
} mmu030;
int searches;
void mmu030_flush_cache(uae_u32) {}
int mmu030_match_ttr(uaecptr, uae_u32, bool) { return 0; }
bool mmu_op30_invea(uae_u32) { return false; }
bool mmu_op30_helper_get_fc(uae_u16 next, uae_u32 *fc) { *fc = next & 7; return true; }
void mmu030_table_search(uaecptr, uae_u32 fc, bool, int) {
    ++searches;
    assert(!mmu030.atc[0].logical.valid);
    assert(mmu030.atc[1].logical.valid && mmu030.atc[1].logical.fc != fc);
}
""" + function(text, "static void mmu030_flush_atc_page_fc(") + "\n" +
                function(text, "static bool mmu_op30_pload (") + "\n" +
                function(text, "static void mmu030_ptest_atc_search(") + r"""
int main() {
    for (uae_u32 size : {256u, 4096u, 32768u}) {
        mmu030.translation.page.imask = ~(size - 1);
        mmu030.atc[0] = {{0x10000, 1, true}, {false, true, true}};
        mmu030.atc[1] = {{0x10000, 5, true}, {false, false, false}};
        for (uae_u32 offset : {0u, 1u, size - 1}) {
            mmu030_ptest_atc_search(0x10000 + offset, 1, false);
            assert(mmu030.status == (MMUSR_WRITE_PROTECTED | MMUSR_MODIFIED));
        }
        mmu030_ptest_atc_search(0x10000, 2, false);
        assert(mmu030.status == MMUSR_INVALID);
        assert(!mmu_op30_pload(0, 0, 1, 0x10001));
        assert(!mmu030.atc[0].logical.valid && mmu030.atc[1].logical.valid);
    }
    assert(searches == 3);
}
""")

    def test_mmu030_locked_access_routes_and_cache_checks(self):
        text = source("src/cpummu30.cpp")
        accessors = "\n".join(function(text, signature) for signature in (
            "void mmu030_put_long(", "void mmu030_put_word(", "void mmu030_put_byte(",
            "uae_u32 mmu030_get_long(", "uae_u16 mmu030_get_word(", "uae_u8 mmu030_get_byte(",
            "uae_u8 uae_mmu030_check_fc("))
        run_cpp(r"""
constexpr int sz_byte = 0, sz_word = 1, sz_long = 2;
constexpr int MMU030_SSW_SIZE_B = 1, MMU030_SSW_SIZE_W = 2, MMU030_SSW_SIZE_L = 4;
constexpr int MMU030_SSW_RM = 16, CACHE_ENABLE_ALL = 0;
int mmu030_cache_state, generic_calls, atc_calls, last_size, last_flags;
bool islrmw030, tt_enabled = true, atc_present = true, last_write;
struct { bool enabled = true; } mmu030;
struct { uae_u32 fc030 = 5; } regs;
uae_u8 ce_cachable[65536];
int mmu030_size[] = {1, 2, 4};
uae_u32 last_address;
int mmu030_match_ttr_access(uaecptr, uae_u32, bool) { return 1; }
int mmu030_match_lrmw_ttr_access(uaecptr, uae_u32) { return 0; }
int mmu030_logical_is_in_atc(uaecptr, uae_u32, bool write) {
    ++atc_calls; last_write = write; return atc_present ? 0 : -1;
}
void mmu030_table_search(uaecptr, uae_u32, bool write, int) {
    last_write = write; atc_present = true;
}
uae_u32 mmu030_get_atc_generic(uaecptr addr, int, uae_u32, int flags, bool write) {
    assert(write && (flags & MMU030_SSW_RM));
    return 0x20000 | (addr & 65535);
}
uae_u32 mmu030_get_atc(uaecptr addr, int, uae_u32, int) { return addr + 65536; }
uae_u32 mmu030_put_atc(uaecptr addr, int, uae_u32, int) { return addr + 65536; }
void cacheablecheck(uaecptr) {}
void mmu030_put_generic(uaecptr addr, uae_u32, uae_u32, int size, int flags) {
    ++generic_calls; last_size = size; last_flags = flags; last_address = addr;
}
uae_u32 mmu030_get_generic(uaecptr addr, uae_u32, int size, int flags) {
    ++generic_calls; last_size = size; last_flags = flags; last_address = addr;
    return 0x42;
}
void x_phys_put_long(uaecptr addr, uae_u32) { last_address = addr; }
void x_phys_put_word(uaecptr addr, uae_u16) { last_address = addr; }
void x_phys_put_byte(uaecptr addr, uae_u8) { last_address = addr; }
uae_u32 x_phys_get_long(uaecptr addr) { last_address = addr; return 0x24; }
uae_u16 x_phys_get_word(uaecptr addr) { last_address = addr; return 0x24; }
uae_u8 x_phys_get_byte(uaecptr addr) { last_address = addr; return 0x24; }
""" + accessors + r"""
int main() {
    for (bool locked : {false, true}) {
        islrmw030 = locked;
        generic_calls = 0;
        mmu030_put_byte(0x10010, 1, 5);
        assert(last_address == 0x10010 && (!locked || (last_size == 0 && (last_flags & 16))));
        mmu030_put_word(0x10010, 1, 5);
        assert(!locked || (last_size == 1 && (last_flags & 16)));
        mmu030_put_long(0x10010, 1, 5);
        assert(!locked || (last_size == 2 && (last_flags & 16)));
        assert(mmu030_get_byte(0x10010, 5) == (locked ? 0x42 : 0x24));
        assert(mmu030_get_word(0x10010, 5) == (locked ? 0x42 : 0x24));
        assert(mmu030_get_long(0x10010, 5) == (locked ? 0x42 : 0x24));
        assert(generic_calls == (locked ? 6 : 0));
    }
    ce_cachable[1] = 3;
    ce_cachable[2] = 7;
    for (bool cached : {false, true}) for (bool write : {false, true}) {
        islrmw030 = true;
        atc_present = cached;
        assert(uae_mmu030_check_fc(0x10010, write, sz_long) == 7);
        assert(last_write);
    }
    islrmw030 = false;
    assert(uae_mmu030_check_fc(0x10010, false, sz_byte) == 3);
}
""")


if __name__ == "__main__":
    unittest.main()
