"""ROM-free regression checks of CPU, chipset, memory and RTC production paths."""

import os
from pathlib import Path
import platform
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
        for path in ("src/jit/arm/compemu_midfunc_arm.cpp",
                     "src/jit/arm/compemu_midfunc_arm64.cpp"):
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

    def test_threaded_cpu_progress_and_bounded_chipset_waits(self):
        text = source("src/newcpu.cpp")
        declarations = text[text.index("static struct alignas(64) {"):
                            text.index("static inline void cpu_thread_spin_pause()")]
        thread = function(text, "static void run_cpu_thread(")
        start = thread.index("\t\tfor (int i = 0; i < cycles_to_do; i++)")
        batch = thread[start:thread.index("\n\t\tcheck_debugger();", start)]
        run_cpp(r"""
constexpr int CYCLE_UNIT = 512;
constexpr int SPCFLAG_CPUINRESET = 1, SPCFLAG_MODE_CHANGE = 2, SPCFLAG_CALLBACK = 4;
constexpr int SPCFLAG_INT = 8;
using frame_time_t = uae_s64;
using evt_t = uae_u64;
struct { bool stopped, halted; int spcflags; } regs;
int countdown, pissoff_value = 100000, cpu_thread_reset;
int cpu_thread_ilvl, cpu_wakeup_sema;
""" + declarations +
                function(text, "static uae_s64 cpu_thread_progress(") +
                function(text, "static bool cpu_thread_unpaced(") +
                function(text, "static void cpu_thread_jit_count(") + r"""
evt_t chipset_cycles;
frame_time_t syncbase = 1000000, vsynctimebase = 20000;
int clock_reads, spins, requests;
bool indirect;
frame_time_t read_processor_time() { return ++clock_reads * 50; }
void cpu_thread_spin_pause() { ++spins; }
bool service_cpu_indirect_request() { ++requests; return indirect; }
evt_t get_cycles() { return chipset_cycles; }
void do_cycles(int n) { chipset_cycles += n; }
void check_uae_int_request() {}
int intlev() { return 0; }
void atomic_set(int *p, int value) { *p = value; }
void set_special(int flag) { regs.spcflags |= flag; }
void uae_sem_post(int *) {}
void run_batch(int cycles_to_do) {
    uae_s64 tokens_seen = 0, credit = 0;
    int cck_cnt = 0, intlev_prev = 0;
""" + batch + r"""
}
int main() {
    cpu_thread_paced = 1;
    uae_s64 seen = 0;
    cpu_thread_tokens = 3;
    assert(cpu_thread_progress(&seen) == 3 * CPU_THREAD_INSN_CREDIT);
    assert(seen == 3 && cpu_thread_progress(&seen) == 0);
    cpu_thread_tokens = 5;
    assert(cpu_thread_progress(&seen) == 2 * CPU_THREAD_INSN_CREDIT);

    cpu_thread_paced = 2;
    seen = countdown = pissoff_value;
    cpu_thread_jit_count(42);
    assert(cpu_thread_progress(&seen) == 42);
    assert(cpu_thread_progress(&seen) == 0);
    countdown = 10;
    seen = 10;
    countdown = pissoff_value - 17;
    assert(cpu_thread_progress(&seen) == 17);
    countdown = pissoff_value;
    assert(cpu_thread_progress(&seen) == 0);
    countdown = 1;
    cpu_thread_jit_count(2);
    assert(countdown == pissoff_value);

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
        generated = source("src/jit/x86/compemu_x86.cpp")
        handlers = "\n".join(function(generated, f"uae_u32 REGPARAM2 op_{op}_0_comp_ff(")
                             for op in ("e008", "e048", "e088"))
        emitter = function(source("src/jit/x86/compemu_midfunc_x86.cpp"),
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
        // Idle/pause and cycle resets rejoin the host calendar.
        host_us += 7000000;
        assert(rtc_microseconds() == host_us);
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
    def test_rtc_continuous_accesses_bound_wall_clock_drift(self):
        text = source("src/rtc.cpp")
        projection = text[text.index("struct rtc_timebase {"):text.index("static uae_s64 rtc_microseconds(")]
        run_cpp(r"""
constexpr int CYCLE_UNIT = 512;
""" + projection + r"""
int main() {
    const uae_s64 epoch = 1700000000000000;
    for (int hz : {3546895, 3579545}) for (int speed : {0, 1, 4, 16}) {
        rtc_timebase base{};
        uae_u64 cycles = 0;
        uae_s64 previous = rtc_project_microseconds(&base, epoch, cycles, hz, false);
        for (int i = 1; i <= 2000; ++i) {
            uae_s64 host = epoch + i * 5000;
            // Quarter speed, normal speed, four times speed, or stalled.
            cycles = (uae_u64)i * 5000 * hz * CYCLE_UNIT * speed / 4000000;
            uae_s64 sample = rtc_project_microseconds(&base, host, cycles, hz, false);
            assert(sample >= previous);
            assert(sample >= host - 20000 && sample <= host + 20000);
            previous = sample;
        }
        assert(previous >= epoch + 10000000 && previous <= epoch + 10020000);
    }
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
