/*
* UAE - The Un*x Amiga Emulator
*
* Vortex ATonce Plus emulation
* (80286 PC emulator board for the Amiga 500 68000 socket, 1991)
*
* The board consists of an 80C286-16, optional 80C287, 512KB local DRAM and
* a Xilinx XC2018 LCA acting as bridge/chipset. There is NO PC support
* hardware at all: every IN/OUT executed by the 286 (except the 287 range
* 0xF8-0xFF) freezes the CPU and is emulated by the Amiga-side 68k driver
* through a polling protocol on two "magic" addresses in CIA space.
*
* Hardware model reverse engineered from the original LCA bitstream
* (configuration "atplus2"), the GAL equations and the original v2.20/v3.00
* driver disassembly: https://github.com/na103/Dueottosei
* The address translation below is ported 1:1 from the formally verified
* RTL (xc2018/rtl/atplus2_core.v).
*
* Magic ports (GAL U22, decodes only A6/A7/A15 within /VPA space):
*   PORT_A = base+0x41 (byte): write = mode bits, read = status
*   PORT_B = base+0x80 (word): write = resume(+INT), read = stop + trap info
*   default base 0xBFF000 -> 0xBFF041/0xBFF080 (aliases all over CIA space)
*
* Uses the PCem v16 core already present for the A1060/A2088/A2286/A2386
* bridgeboards. Mutually exclusive with those boards (PCem is single
* instance).
*/

#include "sysconfig.h"
#include "sysdeps.h"

#ifdef WITH_X86

#include "options.h"
#include "custom.h"
#include "memory.h"
#include "newcpu.h"
#include "debug.h"
#include "x86.h"
#include "atonce.h"
#include "uae.h"
#include "rommgr.h"
#include "zfile.h"
#include "devices.h"
#include "autoconf.h"
#include "filesys.h"

#include "pcem/ibm.h"
#include "pcem/device.h"
#include "pcem/cpu.h"
#include "pcem/pic.h"
#include "pcem/timer.h"
#include "pcem/pcemglue.h"
#include "pcem/model.h"
#include "pcem/mem.h"
#include "pcem/codegen.h"
#include "pcem/x86.h"
#include "pcem/io.h"

/*
* INTEGRATION STATUS (2026-06-13): structurally complete, compiles into the
* expansion list as "ATonce Plus" (Vortex). Points to validate once running:
*   1. PCem core bring-up: minimal init (no pit/dma/kbc). Confirm exec386 runs
*      and the 286 starts from F000:FFF0 (= chip RAM 0x3FFF0 via the F000 map).
*   2. Trap/resume timing: atonce_trap ends the slice (cycles=0) AFTER the I/O
*      instruction; data flows through the pusha frame in shared RAM, so this is
*      equivalent to the hardware freeze-inside-OUT for BIOS service traps.
*      Verify raw (non-BIOS) IN/OUT paths behave too.
*   3. Magic-port base is hardwired to the v2.20 default (A6/A7/A15 in 0xBFxxxx).
*      The driver can relocate it; make it configurable if a disk uses another base.
*   4. 287/FPU: fpu_type=FPU_NONE; the err287/setup287 path is not wired yet.
*   Acceptance test: original floppies in Disk/ -> driver loads, Vortex BIOS POST
*   beacon (OUT 0xBB) reached, DOS prompt.
*/

#define ONBOARD_SIZE 0x80000   // 512KB local DRAM

// Cycles run synchronously inside a PORT_B resume: the 286 advances until its
// next I/O instruction traps (atonce_trap forces the slice to end) or this
// budget is exhausted. On real hardware the 286 runs FREELY at 16 MHz between
// I/O instructions; the 68k only intervenes on a trap. The 68k driver stops the
// 286 (PORT_B read) right after each resume, so atonce_hsync cannot continue a
// budget-exhausted run (it is frozen, not merely idle) -> the per-resume budget
// IS the throughput unit. PERFORMANCE: at 2000 the 286 effectively ran at only
// 0.2-0.85 MHz (resumes/s x 2000), making DOS sluggish. A trap ends the burst
// early anyway, so this is just the cap on no-I/O stretches; sized so the 286
// reaches ~16 MHz effective without long stretches blocking the Amiga thread.
#define ATONCE_RUN_CYCLES 65536

// PORT_A write bits (68k D0-D7 = 286 D8-D15, byte-swapped by U24/U25)
#define PORTA_DIN     0x01   // config DIN / runtime: 286 reset pulse
#define PORTA_SETUP   0x02   // 287 reset / MEMMODE setup mode
#define PORTA_A20     0x04   // gate A20 enable
#define PORTA_ACK287  0x08   // 287 error acknowledge
#define PORTA_PROG    0x10   // DONE / -PROG

// PORT_A read (status) bits
#define PORTA_ST_ERR287_OR_DIR 0x01  // pre-stop: 287 error; post-stop: 1=IN 0=OUT
#define PORTA_ST_TRAP          0x02  // I/O trap pending

struct atonce_s {
	int configured;          // LCA configured (DONE)
	uae_u8 mode;             // last PORT_A write
	int a20en;
	int setup;               // MEMMODE setup mode (PORT_A D1)
	int frozen;              // 286 stalled: trap pending or stopped by PORT_B read
	int stopped;             // explicitly stopped by PORT_B read
	int in_reset;            // 286 held in reset
	int trap_pending;
	uae_u16 trap_port;       // latched 10-bit port
	int trap_dir;            // 1 = IN
	int refresh_div;         // hsync counter for the idle video-refresh trap
	uae_u32 text_hash;       // last-rendered hash of the 286 text page (change detect)
	int refresh_heartbeat;   // frames since last refresh (slow cursor-blink heartbeat)
	int refresh_due;         // a video refresh is owed (set per frame, serviced
	                         // at the next safe point: idle hsync or 286 resume)
	int fake_active;         // a synthetic CRTC refresh trap is in flight
	int err287;
	// MEMMODE shift register, stages AH/BH/CH/DH (AH = last shifted bit)
	int mm_a, mm_b, mm_c, mm_d;
	// translation table: x86 64K bank -> Amiga bank base, -1 = onboard DRAM
	int bank_amiga[256];
	uae_u8 *dram;            // 512KB onboard DRAM (= PCem "ram")
	uae_u8 *vmem;            // DIAGNOSTIC: private backing for the 286's sub-1MB
	                         // chip windows (banks 0-6), so video/conventional
	                         // writes can't corrupt Amiga chip RAM. F000 mailbox
	                         // (chip 0x30000) stays on real chip RAM.
	int ext_memsize;         // configured extended memory (bytes), 0 = none
	uae_u8 *emem;            // DIAGNOSTIC: private flat buffer backing the 286's
	                         // >1MB extended memory (ATONCE_ISOLATE_EXT), with a
	                         // real exec pointer so getpccache works in the HMA.
	struct romconfig *rc;
};

// DIAGNOSTIC toggle: when 1, the 286's sub-1MB chip windows (except the F000
// mailbox) are backed by a private buffer instead of Amiga chip RAM. If the
// 68k Guru disappears, it proves the crash is the 286 stamping on OS chip RAM.
// Now 0: the wild-jump root cause (missing OUT-data shadow) is fixed, and the
// driver's video bridge reads the 286 framebuffer FROM real chip RAM (chip
// 0x18000) to render the Amiga screen, so the 286 must write video there (not
// into the private vmem) or the screen stays black.
#define ATONCE_ISOLATE_CHIP 0
#define ATONCE_VMEM_SIZE 0x70000   // covers chip banks 0..6 (0x00000-0x6FFFF)

// DIAGNOSTIC toggle: when 1, the 286's extended memory (>1MB) is backed by a
// private, flat buffer instead of the dynamic MEMMODE translation onto Amiga
// fast RAM. This buffer is mapped with a REAL exec pointer, so PCem's code fetch
// (getpccache) works when himem.sys relocates/executes in the HMA (>1MB). It
// also fully isolates the 286's extended memory from the Amiga (no sharing of
// fast RAM with the 286, as on the video-isolation test). Use to bring up XMS /
// himem.sys before wiring the extended memory back onto shared Amiga RAM.
#define ATONCE_ISOLATE_EXT 1
#define ATONCE_EMEM_SIZE 0xf00000  // covers the whole >1MB window (0x100000-0xFFFFFF)

static struct atonce_s *atonce;

int atonce_intr_vector = -1;  // consumed by picinterrupt() (pcem/pic.cpp patch)

// Allocates pcemmap (the PCem memory-map block table) + the PCem video infra.
// The bridgeboards reach it via their video card; the ATonce has no PCem video,
// so we call it directly (priv=NULL is safe: it is only stored as gfxboard_priv,
// never dereferenced unless PCem video is rendered, which we never do).
extern void initpcemvideo(void *p, bool swapped);

/*-------------------------------------------------------------------------*/
/* Address translation 286 -> Amiga, ported from atplus2_core.v             */
/* (formally equivalent to the golden LCA netlist).                         */
/* Translation depends only on x86 A16-A23 + board state, so it is          */
/* precomputed per 64K bank and rebuilt when MEMMODE/A20 change.            */
/*-------------------------------------------------------------------------*/

// Drive PCem's physical A20 mask from the LCA gate (PORT_A D2). On the ATonce
// there is no keyboard controller: the LCA A20 gate IS the A20 line. PCem masks
// bit 20 of the physical address via rammask (mem_a20_recalc), so this makes the
// 286 wrap 0x100000->0x000000 when A20 is gated off - exactly what himem.sys's
// A20-control test checks (else: "Unable to control A20 line!"). We own A20
// entirely: keep mem_a20_key cleared (PCem defaults it to 2 = A20 always on).
static void atonce_apply_a20(struct atonce_s *xb)
{
	mem_a20_key = 0;
	mem_a20_alt = xb->a20en ? 1 : 0;
	mem_a20_recalc();
}

static void atonce_rebuild_map(struct atonce_s *xb)
{
	for (int bank = 0; bank < 256; bank++) {
		int a16 = (bank >> 0) & 1, a17 = (bank >> 1) & 1;
		int a18 = (bank >> 2) & 1, a19 = (bank >> 3) & 1;
		int a20 = (bank >> 4) & 1, a21 = (bank >> 5) & 1;
		int a22 = (bank >> 6) & 1, a23 = (bank >> 7) & 1;

		// U15: A20 gated by PORT_A D2 (DI.Q)
		int a20g = a20 & xb->a20en;

		// U45/U43 glue: onboard DRAM select for x86 0-512K (memory cycles)
		if (!a23 && !a22 && !a21 && !a20g && !a19) {
			xb->bank_amiga[bank] = -1;
			continue;
		}

		// U28/U45 glue: P67 = A23 XOR NOR(A23,A22,A21,A20g); ~P67 = 1MB-8MB
		int p67 = a23 ^ ((a23 | a22 | a21 | a20g) ? 0 : 1);
		// LCA DI.X: EXT = M/IO & ~P67 & A20-enable
		int ext = (!p67 && xb->a20en) ? 1 : 0;

		int mm_a = xb->mm_a, mm_b = xb->mm_b, mm_c = xb->mm_c, mm_d = xb->mm_d;

		// CLB AF
		int af_x = (!a19 & a17) | (!a16 & a17) | (a18 & a17) | (a19 & !a16 & a18);
		// CLB FC (memory cycle: mio=1)
		int za16 = a16;
		// CLB BF
		int za17 = (af_x & a17) | (af_x & !ext) | (a17 & ext);
		// CLB BG
		int za18 = (!a19 & a18) | (ext & a18) | (a19 & !a17 & !ext & !a18);
		// CLB AH (G)
		int za19 = (a19 & ext) | (ext & mm_a);
		// CLB CI
		int ci_x = (a20g & !a21) | (mm_d & !a21) | (!a20g & !mm_d & a21);
		int za20 = (!a20g & !mm_d & ext) | (a20g & mm_d & ext);
		// CLB CJ
		int cj_x = (a22 & !mm_d) | (a22 & !a21) | (!a22 & mm_d & a21) | (!a22 & a21 & a20g);
		// CLB BH (G)
		int za21 = ext & ci_x & mm_b;
		// CLB DH (G)
		int za23 = mm_c & ext;
		// CLB BJ
		int za22 = za23 | (!za23 & cj_x & ext & mm_b);

		xb->bank_amiga[bank] =
			((za23 << 7) | (za22 << 6) | (za21 << 5) | (za20 << 4) |
			 (za19 << 3) | (za18 << 2) | (za17 << 1) | za16) << 16;
	}
}

/*-------------------------------------------------------------------------*/
/* x86-side memory access (PCem handlers). Byte order: the data bus is      */
/* byte-swapped by wiring (U24/U25), which makes byte addresses map 1:1;    */
/* reading consecutive Amiga bytes as a little-endian word is exactly what  */
/* the hardware produces.                                                   */
/*-------------------------------------------------------------------------*/

static uint8_t atonce_mem_readb(uint32_t addr, void *priv)
{
	struct atonce_s *xb = (struct atonce_s*)priv;
	if (!xb || !xb->dram) {
		write_log(_T("ATonce: ext readb @%06x with NULL ctx\n"), addr);
		return 0xff;
	}
	int base = xb->bank_amiga[(addr >> 16) & 0xff];
	if (base < 0)
		return xb->dram[addr & (ONBOARD_SIZE - 1)];
	return get_byte(base | (addr & 0xffff));
}
static uint16_t atonce_mem_readw(uint32_t addr, void *priv)
{
	return atonce_mem_readb(addr, priv) | (atonce_mem_readb(addr + 1, priv) << 8);
}
static uint32_t atonce_mem_readl(uint32_t addr, void *priv)
{
	return atonce_mem_readw(addr, priv) | (atonce_mem_readw(addr + 2, priv) << 16);
}
static void atonce_mem_writeb(uint32_t addr, uint8_t val, void *priv)
{
	struct atonce_s *xb = (struct atonce_s*)priv;
	if (!xb || !xb->dram) {
		write_log(_T("ATonce: ext writeb @%06x with NULL ctx\n"), addr);
		return;
	}
	int base = xb->bank_amiga[(addr >> 16) & 0xff];
	if (base < 0) {
		xb->dram[addr & (ONBOARD_SIZE - 1)] = val;
		return;
	}
	put_byte(base | (addr & 0xffff), val);
}
static void atonce_mem_writew(uint32_t addr, uint16_t val, void *priv)
{
	atonce_mem_writeb(addr, val & 0xff, priv);
	atonce_mem_writeb(addr + 1, val >> 8, priv);
}
static void atonce_mem_writel(uint32_t addr, uint32_t val, void *priv)
{
	atonce_mem_writew(addr, val & 0xffff, priv);
	atonce_mem_writew(addr + 2, val >> 16, priv);
}

static mem_mapping_t atonce_mapping;   // extended memory (1MB+), dynamic

/*-------------------------------------------------------------------------*/
/* Sub-1MB chip-RAM windows (static map). The 286 executes code from here    */
/* (BIOS at F000 = chip 0x30000, conventional 512-640K = chip 0x40000), and  */
/* PCem's code fetch (getpccache) needs a real pointer, not the function     */
/* handlers. So each window exposes WinUAE chip RAM directly via an "exec"   */
/* pointer; data goes through funcs that index the same buffer. The U24/U25  */
/* byte-swap is 1:1 at byte level, so a direct pointer matches the handlers. */
/* This map is MEMMODE-independent: for banks 0-0xF the LCA EXT term is 0.   */
/* priv = chipbase + chipoff - xbase, so priv[addr] is the chip byte.        */
/*-------------------------------------------------------------------------*/

static uint8_t atonce_chip_readb(uint32_t addr, void *priv)
{
	return ((uae_u8*)priv)[addr];
}
static uint16_t atonce_chip_readw(uint32_t addr, void *priv)
{
	uae_u8 *p = (uae_u8*)priv;
	return p[addr] | (p[addr + 1] << 8);
}
static uint32_t atonce_chip_readl(uint32_t addr, void *priv)
{
	uae_u8 *p = (uae_u8*)priv;
	return p[addr] | (p[addr + 1] << 8) | (p[addr + 2] << 16) | (p[addr + 3] << 24);
}
static void atonce_chip_writeb(uint32_t addr, uint8_t v, void *priv)
{
	uae_u8 *p = (uae_u8*)priv;
	p[addr] = v;
}
static void atonce_chip_writew(uint32_t addr, uint16_t v, void *priv)
{
	uae_u8 *p = (uae_u8*)priv;
	p[addr] = (uae_u8)v;
	p[addr + 1] = (uae_u8)(v >> 8);
}
static void atonce_chip_writel(uint32_t addr, uint32_t v, void *priv)
{
	uae_u8 *p = (uae_u8*)priv;
	p[addr] = (uae_u8)v;
	p[addr + 1] = (uae_u8)(v >> 8);
	p[addr + 2] = (uae_u8)(v >> 16);
	p[addr + 3] = (uae_u8)(v >> 24);
}

// Sub-1MB x86 bank -> Amiga chip bank. This MUST match the hardware LCA
// translation (atonce_rebuild_map, ported from the verified RTL atplus2_core.v):
// these exec-pointer windows and the function-handler map model the same chip.
//   x86 8->chip 0x40000  9->0x50000  A->0x20000  B->0x10000
//       C->0x20000  D->0x10000  E->0x20000  F->0x30000 (BIOS)
// NOTE banks A and C: the .dsg software translation table the 68k driver shifts
// in says 0x60000 (A) / 0x00000 (C), but those are SOFTWARE-only aliases; the
// LCA hardware decodes both to 0x20000 .
// Both banks are unused aliases (no EGA/VGA), so this is a fidelity fix, not a
// functional one for DOS - but the emulator models the hardware, so use the LCA value.
static const struct { uae_u32 xbase, size, chipoff; } atonce_win_map[] = {
	{ 0x80000, 0x20000, 0x40000 },  // banks 8,9 -> chip 0x40000,0x50000 (contiguous)
	{ 0xA0000, 0x10000, 0x20000 },  // bank A -> chip 0x20000 (LCA; .dsg software says 0x60000)
	{ 0xB0000, 0x10000, 0x10000 },  // bank B -> chip 0x10000 (MDA/CGA video shadow)
	{ 0xC0000, 0x10000, 0x20000 },  // bank C -> chip 0x20000 (LCA; .dsg software says 0x00000)
	{ 0xD0000, 0x10000, 0x10000 },  // bank D -> chip 0x10000 (hardware alias of B0000)
	{ 0xE0000, 0x10000, 0x20000 },  // bank E -> chip 0x20000
	{ 0xF0000, 0x10000, 0x30000 },  // bank F -> chip 0x30000 (BIOS; FFFF0->chip 0x3FFF0)
};
#define ATONCE_NWIN (sizeof(atonce_win_map) / sizeof(atonce_win_map[0]))
static mem_mapping_t atonce_win[ATONCE_NWIN];

static void atonce_map_windows(void)
{
	uae_u8 *chip = chipmem_bank.baseaddr;
	uae_u32 chipsize = chipmem_bank.allocated_size;
	if (!chip || chipsize < 0x60000) {
		write_log(_T("ATonce: chip RAM too small (%uK) for shared windows; ")
			_T("286 cannot fetch the BIOS\n"), chipsize / 1024);
		return;
	}
	for (unsigned i = 0; i < ATONCE_NWIN; i++) {
		uae_u8 *base = chip;
#if ATONCE_ISOLATE_CHIP
		// Back every window except the F000 mailbox (chip 0x30000) with a private
		// buffer, so the 286's video/conventional writes never touch Amiga chip RAM.
		if (atonce && atonce->vmem && atonce_win_map[i].chipoff != 0x30000)
			base = atonce->vmem;
#endif
		uae_u8 *lin = base + atonce_win_map[i].chipoff - atonce_win_map[i].xbase;
		// NOTE: mem_mapping_add (stock PCem), NOT mem_mapping_addx (WinUAE glue).
		// The 286 core's getpccache/readmembl consult read_mapping[]/_mem_exec[]
		// filled by the stock mem_mapping_add; the glue's addx fills a separate
		// pcemmap[] table and self-disables on re-add. The bridgeboards use add.
		mem_mapping_add(&atonce_win[i], atonce_win_map[i].xbase, atonce_win_map[i].size,
			atonce_chip_readb, atonce_chip_readw, atonce_chip_readl,
			atonce_chip_writeb, atonce_chip_writew, atonce_chip_writel,
			base + atonce_win_map[i].chipoff,   // exec ptr: indexed [addr - xbase]
			MEM_MAPPING_EXTERNAL, lin);          // priv:     indexed [addr]
	}
	// Mark 512K-1MB as EXTERNAL RAM, else mem_mapping_recalc refuses our
	// EXTERNAL mappings (default state is INTERNAL) and getpccache fails.
	mem_set_mem_state(0x80000, 0x80000, MEM_READ_EXTERNAL | MEM_WRITE_EXTERNAL);
	mem_mapping_t *w = &atonce_win[ATONCE_NWIN - 1];   // F000 window
	write_log(_T("ATonce: mapped %u windows, chip=%p %uK; ")
		_T("phys 0=%02x 10000=%02x 80000=%02x F0000=%02x FFFF0=%02x; ")
		_T("win[F].base=%x size=%x flags=%x en=%d directRB=%02x\n"),
		(unsigned)ATONCE_NWIN, chip, chipsize / 1024,
		mem_readb_phys(0), mem_readb_phys(0x10000), mem_readb_phys(0x80000),
		mem_readb_phys(0xF0000), mem_readb_phys(0xFFFF0),
		(unsigned)w->base, (unsigned)w->size, w->flags, w->enable,
		w->read_b(0xFFFF0, w->p));
}

/*-------------------------------------------------------------------------*/
/* x86-side I/O: every access freezes the 286 (the LCA holds READY) and     */
/* latches port + direction. The 68k driver polls PORT_A, stops/reads the   */
/* trap info from PORT_B, emulates the device and resumes with PORT_B       */
/* write (optionally delivering an INT whose stub patches the register      */
/* frame in shared RAM). The data returned to a trapped IN here is junk     */
/* (0xFF) exactly like on real hardware: the BIOS stub overwrites AX in     */
/* the pusha frame afterwards.                                              */
/*-------------------------------------------------------------------------*/

static void atonce_trap(struct atonce_s *xb, uae_u16 port, int dir)
{
	xb->trap_pending = 1;
	xb->trap_port = port & 0x3ff;
	xb->trap_dir = dir;
	xb->frozen = 1;
	// End the current PCem slice right after this instruction. exec386's inner
	// loop exits on cycdiff (= oldcyc - cycles) >= cycle_period, not on cycles
	// alone, so a huge-negative value forces cycdiff past any cycle_period and
	// bails out of both the inner and the outer (cycles>0) loop immediately.
	cycles = -0x40000000;
}

// Drive the 286 for up to `budget` cycles. PCem's exec386 paces itself against
// the timer subsystem: its inner loop runs until cycdiff >= cycle_period, where
// cycle_period = (timer_target - tsc) + 1, and timer_process() only advances
// timer_target while a timer is registered. We register no PCem timers (the PIT
// lives behind the I/O trap and is emulated by the 68k), so timer_target stays
// behind the ever-growing tsc, cycle_period goes <= 0, and exec386 spins
// forever in its outer loop without ever issuing an instruction. Keeping
// timer_target a budget ahead of tsc makes each burst actually execute, then
// return when the budget is spent or a trap forces it out (atonce_trap).
static void atonce_run(struct atonce_s *xb, int budget)
{
	(void)xb;
	timer_target = (uint32_t)tsc + (uint32_t)budget + 16;
	cycles = 0;
	exec386(budget);
}

static uint8_t atonce_io_inb(uint16_t addr, void *priv)
{
	atonce_trap((struct atonce_s*)priv, addr, 1);
	return 0xff;
}
static uint16_t atonce_io_inw(uint16_t addr, void *priv)
{
	atonce_trap((struct atonce_s*)priv, addr, 1);
	return 0xffff;
}
static uint32_t atonce_io_inl(uint16_t addr, void *priv)
{
	atonce_trap((struct atonce_s*)priv, addr, 1);
	return 0xffffffff;
}
// On a trapped 286 OUT the board latches the written data into the shared F000
// I/O shadow at F000:[port] (= chip 0x30000 + port). The 68k driver reads this
// shadow byte to recover the OUT value: e.g. the CRTC handler reads the register
// index from F000:03D4 and the data from F000:03D5 (it never reads the pusha
// frame for these). WinUAE previously discarded the OUT data, so the shadow kept
// the reject handler's 0xFF -> the driver's signed index bound-check (cmp.b #$f /
// bgt) let 0xFF through, indexed 0xFF*4 past its 16-entry jump table, and did a
// wild jmp (a4)=0x264ad7c4 -> stack-overflow MOVEM sled -> Task corruption ->
// Guru #3. Mirroring the data into the shadow on every OUT fixes that at the
// source. trap_port is the 10-bit PC I/O address, so the shadow lives in
// F000:0000-03FF, well below the BIOS mailboxes at F000:1400+. (For IN the data
// flows the other way, via the BIOS pusha-frame edit, and needs no shadow here.)
static void atonce_io_shadow(uae_u16 port, uae_u32 val, int size)
{
	uae_u8 *chip = chipmem_bank.baseaddr;
	if (!chip || chipmem_bank.allocated_size < 0x30400)
		return;
	uae_u8 *sh = chip + 0x30000;
	for (int i = 0; i < size; i++)
		sh[(port + i) & 0x3ff] = (uae_u8)(val >> (8 * i));
}

// Symmetric to the OUT shadow: a trapped 286 IN reads back the value the driver
// pre-staged in the shared F000 I/O shadow (F000:[port]). E.g. on an Amiga key
// the driver writes the PC scancode to F000:0060 and the 8042 status to
// F000:0064, then injects keyboard IRQ1; the 286's keyboard ISR does IN 0x60 /
// IN 0x64 and must read those, not junk. (Returning 0xff broke every device
// poll that reads back driver-maintained state: keyboard, 8042, PIC mask, RTC,
// CGA retrace 0x3da, ...). The 0xBx BIOS services are OUTs, not INs, so the
// pusha-frame path is unaffected.
static uae_u32 atonce_io_shadow_read(uae_u16 port, int size)
{
	uae_u8 *chip = chipmem_bank.baseaddr;
	if (!chip || chipmem_bank.allocated_size < 0x30400)
		return 0xffffffff;
	uae_u8 *sh = chip + 0x30000;
	uae_u32 v = 0;
	for (int i = 0; i < size; i++)
		v |= (uae_u32)sh[(port + i) & 0x3ff] << (8 * i);
	return v;
}

static void atonce_io_outb(uint16_t addr, uint8_t val, void *priv)
{
	atonce_io_shadow(addr & 0x3ff, val, 1);
	atonce_trap((struct atonce_s*)priv, addr, 0);
}
static void atonce_io_outw(uint16_t addr, uint16_t val, void *priv)
{
	atonce_io_shadow(addr & 0x3ff, val, 2);
	atonce_trap((struct atonce_s*)priv, addr, 0);
}
static void atonce_io_outl(uint16_t addr, uint32_t val, void *priv)
{
	atonce_io_shadow(addr & 0x3ff, val, 4);
	atonce_trap((struct atonce_s*)priv, addr, 0);
}

/*-------------------------------------------------------------------------*/
/* HDD bypass: serve the PC C: drive by reading the disk image the user attached  */
/* in WinUAE's HDD settings, replacing the 68k driver's scsi.device path. The VHD  */
/* MUST be attached as a WinUAE hardfile (the Amiga boots from its Amiga partition */
/* and loads the atonce driver from there); we read the PC partition through       */
/* WinUAE's own hardfile handle (get_hardfile_data/hdf_read), so there is no        */
/* file-lock / double-open conflict. Nothing geometry-related is hardcoded: we      */
/* parse the RDB and pick the partition whose first block is a PC MBR (0x55AA); its */
/* byte offset comes from the RDB DosEnvVec and its CHS geometry from the MBR        */
/* partition table's ending CHS. The BIOS forwards INT 13h as OUT 0xB0 (data) /      */
/* OUT 0xB9 (control) with the registers live; for drive >= 0x80 we do the I/O       */
/* against the hardfile, write the result into the 286 PUSHA frame and skip the trap.*/
/* NOTE: the board's 286 DRAM window at 0x900000-0x9FFFFF must not overlap OS-usable  */
/* Z2 fast RAM, or the OS may allocate there and corrupt (see atonce_init mapping).   */
/*-------------------------------------------------------------------------*/
#define ATONCE_HDD_DRIVE    0x80      // BIOS drive number of the first HDD (C:)

static struct hardfiledata *atonce_hfd;   // WinUAE hardfile backing the PC C: drive
static int      atonce_hdd_tried;
static uae_u64  atonce_part_off;          // byte offset of PC LBA 0 inside the hardfile
static int      atonce_hdd_heads;         // PC geometry (from the MBR ending CHS)
static int      atonce_hdd_spt;
static uae_u32  atonce_hdd_sectors;       // total PC sectors (for AH=08 max cylinder)

static uae_u32 atonce_be32(const uae_u8 *p)
{
	return ((uae_u32)p[0] << 24) | ((uae_u32)p[1] << 16) |
		((uae_u32)p[2] << 8) | (uae_u32)p[3];
}

// Resolve the PC C: drive from WinUAE's attached hardfiles (once). Returns 1 if found.
static int atonce_hdd_resolve(void)
{
	if (atonce_hdd_tried)
		return atonce_hfd != NULL;
	atonce_hdd_tried = 1;

	for (int nr = 0; nr < MAX_FILESYSTEM_UNITS; nr++) {
		struct hardfiledata *hfd = get_hardfile_data(nr);
		if (!hfd)
			continue;
		write_log(_T("ATonce HDD: examine unit %d '%s' handle_valid=%d virtsize=%llu\n"),
			nr, hfd->ci.rootdir, hfd->handle_valid, hfd->virtsize);
		if (!hfd->handle_valid)
			continue;

		uae_u8 blk[512];
		uae_u32 err = 0;
		// Find the RDSK block within the first 16 sectors.
		int found_rdb = 0;
		for (int b = 0; b < 16; b++) {
			if (hdf_read(hfd, blk, (uae_u64)b * 512, 512) != 512)
				break;
			if (blk[0] == 'R' && blk[1] == 'D' && blk[2] == 'S' && blk[3] == 'K') {
				found_rdb = 1;
				break;
			}
		}
		if (!found_rdb)
			continue;

		// Walk the partition list looking for one that starts with a PC MBR.
		uae_u32 part = atonce_be32(blk + 0x1c);   // rdb_PartitionList (RDSK off 0x1C)
		for (int guard = 0; part != 0xffffffff && guard < 64; guard++) {
			if (hdf_read(hfd, blk, (uae_u64)part * 512, 512) != 512)
				break;
			if (!(blk[0] == 'P' && blk[1] == 'A' && blk[2] == 'R' && blk[3] == 'T'))
				break;
			uae_u32 next     = atonce_be32(blk + 0x10);
			uae_u32 surfaces = atonce_be32(blk + 0x8c);   // de_Surfaces
			uae_u32 bpt      = atonce_be32(blk + 0x94);   // de_BlocksPerTrack
			uae_u32 lowcyl   = atonce_be32(blk + 0xa4);   // de_LowCyl
			uae_u32 highcyl  = atonce_be32(blk + 0xa8);   // de_HighCyl
			uae_u32 bsize    = atonce_be32(blk + 0x84) * 4; // de_SizeBlock (longs) -> bytes
			if (bsize < 512)
				bsize = 512;
			uae_u64 part_off = (uae_u64)lowcyl * surfaces * bpt * bsize;

			// Partition name (BSTR at pb_DriveName, off 0x24: len byte + chars).
			char pname[32];
			int pl = blk[0x24]; if (pl > 31) pl = 31;
			for (int i = 0; i < pl; i++) pname[i] = blk[0x25 + i];
			pname[pl] = 0;
			uae_u8 mbr[512];
			int rd = hdf_read(hfd, mbr, part_off, 512);
			write_log(_T("ATonce HDD: PART '%hs' surf=%u bpt=%u lcyl=%u hcyl=%u ")
				_T("off=%llu rd=%d sig=%02x%02x\n"),
				pname, surfaces, bpt, lowcyl, highcyl,
				(unsigned long long)part_off, rd, mbr[0x1fe], mbr[0x1ff]);
			if (rd == 512 && mbr[0x1fe] == 0x55 && mbr[0x1ff] == 0xaa) {
				// PC disk found. CHS geometry from a partition entry's ending CHS
				// (the standard heuristic DOS/fdisk use). Scan all 4 entries and
				// take the first non-empty one, preferring the active (0x80) one,
				// so an empty/extended entry 0 doesn't give bogus 0/0 geometry.
				const uae_u8 *pe = NULL;
				for (int e = 0; e < 4; e++) {
					const uae_u8 *p = mbr + 0x1be + e * 16;
					if (p[4] == 0)            // type 0 = unused entry
						continue;
					if (!pe)
						pe = p;               // first non-empty: fallback choice
					if (p[0] == 0x80) {       // active partition: preferred
						pe = p;
						break;
					}
				}
				int heads = pe ? pe[5] + 1 : 0;
				int spt   = pe ? (pe[6] & 0x3f) : 0;
				if (heads < 1) heads = 1;
				if (spt < 1)   spt = 1;
				atonce_hfd         = hfd;
				atonce_part_off    = part_off;
				atonce_hdd_heads   = heads;
				atonce_hdd_spt     = spt;
				atonce_hdd_sectors = (uae_u32)(((uae_u64)(highcyl - lowcyl + 1) *
					surfaces * bpt) * (bsize / 512));
				write_log(_T("ATonce HDD: unit %d '%s' PC disk at byte %llu, ")
					_T("CHS=%d/%d, %u sectors (%u MB)\n"),
					nr, hfd->ci.rootdir, (unsigned long long)part_off,
					heads, spt, atonce_hdd_sectors, atonce_hdd_sectors / 2048);
				return 1;
			}
			part = next;
		}
	}
	write_log(_T("ATonce HDD: no PC partition (MBR) found in any WinUAE hardfile; ")
		_T("falling back to the 68k driver\n"));
	return 0;
}

// Write the INT 13h result into the 286 PUSHA frame (the BIOS published its
// SS:SP, so it is at ss+SP). Byte offsets match the 68k driver's a3-relative
// reads (AH=status @+0x13, AL=count @+0x12, FLAGS @+0x18; CF bit0 = error);
// 286 byte addressing is 1:1 with the driver's view.
static void atonce_disk_result(uint8_t ah, uint8_t al, int error)
{
	uint32_t frame = ss + SP;
	mem_writeb_phys(frame + 0x13, ah);
	mem_writeb_phys(frame + 0x12, al);
	uint8_t fl = mem_readb_phys(frame + 0x18);
	if (error)
		fl |= 0x01;
	else
		fl &= ~0x01;
	mem_writeb_phys(frame + 0x18, fl);
}

// Returns 1 if the disk service was fully handled here (skip the 68k trap).
static int atonce_disk_bypass(uint16_t port)
{
	int drive = DL;
	if (drive < ATONCE_HDD_DRIVE || (drive & 0x7f) != 0)
		return 0;                 // floppy / non-C: HDD -> let the 68k driver run
	if (!atonce_hdd_resolve())
		return 0;                 // no PC disk configured -> fall back to the driver

	int func  = AH;
	int cyl   = CH | ((CL & 0xc0) << 2);
	int sec   = CL & 0x3f;
	int head  = DH;
	int count = AL;

	switch (func) {
	case 0x02:     // read sectors
	case 0x03: {   // write sectors
		if (sec == 0) { atonce_disk_result(0x01, 0, 1); return 1; }
		uint32_t lba = (uint32_t)(cyl * atonce_hdd_heads + head) * atonce_hdd_spt
			+ (sec - 1);
		uint32_t buf = es + BX;
		uae_u64 pos = atonce_part_off + (uae_u64)lba * 512u;
		int ok = 1, done = 0;
		uint8_t sb[512];
		uae_u32 err = 0;
		for (int s = 0; s < count; s++) {
			uae_u64 sp = pos + (uae_u64)s * 512;
			if (func == 0x02) {
				if (hdf_read(atonce_hfd, sb, sp, 512) != 512) { ok = 0; break; }
				for (int i = 0; i < 512; i++)
					mem_writeb_phys(buf + s * 512 + i, sb[i]);
			} else {
				for (int i = 0; i < 512; i++)
					sb[i] = mem_readb_phys(buf + s * 512 + i);
				if (hdf_write(atonce_hfd, sb, sp, 512) != 512) { ok = 0; break; }
			}
			done++;
		}
		atonce_disk_result(ok ? 0x00 : 0x04, (uint8_t)done, ok ? 0 : 1);
		return 1;
	}
	case 0x08: {   // get drive parameters
		int percyl = atonce_hdd_heads * atonce_hdd_spt;
		int maxcyl = (percyl > 0 ? (int)(atonce_hdd_sectors / percyl) : 1) - 1;
		if (maxcyl > 1023) maxcyl = 1023;
		uint32_t frame = ss + SP;
		uint8_t ch = (uint8_t)(maxcyl & 0xff);
		uint8_t cl = (uint8_t)(atonce_hdd_spt | ((maxcyl >> 2) & 0xc0));
		mem_writeb_phys(frame + 0x10, cl);                  // CL
		mem_writeb_phys(frame + 0x11, ch);                  // CH
		mem_writeb_phys(frame + 0x0e, 0x01);                // DL = 1 HDD
		mem_writeb_phys(frame + 0x0f, atonce_hdd_heads - 1); // DH = max head
		atonce_disk_result(0x00, 0, 0);
		return 1;
	}
	case 0x00:     // reset
	case 0x01:     // get last status
	case 0x04:     // verify (no transfer)
	case 0x09:     // set drive params
	case 0x0c:     // seek
	case 0x0d:     // alternate reset
	case 0x10:     // test drive ready
	case 0x11:     // recalibrate
		atonce_disk_result(0x00, (uint8_t)count, 0);
		return 1;
	default:       // unsupported function: claim success rather than stall the BIOS
		atonce_disk_result(0x00, (uint8_t)count, 0);
		return 1;
	}
}

// Entry point from pcem/x86.cpp's portin/portout. The 286's IN/OUT opcodes go
// through outb()/inb() -> portout()/portin() in x86.cpp, which are the *bridge-
// board* dispatchers and dereference bridges[0] (NULL for the ATonce). Those
// functions now fall through to here when no PC bridgeboard owns the x86 side.
// Every x86 I/O freezes the 286 (the LCA holds READY) and is emulated by the
// 68k driver; the 287 ports 0xf8-0xff are handled by the FPU and not trapped.
// dir: 1 = IN, 0 = OUT. On OUT the board latches the written data into the
// shared F000 I/O shadow (F000:[port] = chip 0x30000+port); the 68k driver
// reads it back (CRTC index from F000:03D4, data from F000:03D5, etc.). This is
// the ACTUAL OUT path: x86.cpp's portout/portin short-circuit here before the
// stock io_sethandlerx table, so the shadow must be read/written here. Returns
// the IN data (the driver-staged shadow byte/word) for dir=1; 0 for OUT.
uint32_t atonce_x86_io(uint16_t port, int dir, uint32_t val, int size)
{
	struct atonce_s *xb = atonce;
	if (!xb)
		return 0xffffffff;
	if ((port & 0xfff8) == 0xf8)
		return 0xffffffff;   // 287 coprocessor range: no trap (no FPU configured)
	if (!dir) {
		// HDD bypass: serve drive >= 0x80 from the host VHD and skip the 68k trap.
		if ((port == 0xb0 || port == 0xb9) && atonce_disk_bypass(port))
			return 0;
		atonce_io_shadow(port & 0x3ff, val, size);
		atonce_trap(xb, port, dir);
		return 0;
	}
	// IN: return the value the driver pre-staged in the F000 I/O shadow, then
	// trap so it can post-update device state (e.g. clear the 8042 data-ready).
	uint32_t v = atonce_io_shadow_read(port & 0x3ff, size);
	atonce_trap(xb, port, dir);
	return v;
}

/*-------------------------------------------------------------------------*/
/* 68k side: magic ports. Called from cia.cpp for accesses in CIA space.    */
/* GAL U22 decode: only A6/A7/A15 (+ /VPA + any data strobe):               */
/*   PORT_A: A15=1 A6=1 A7=0  -> (addr & 0x80c0) == 0x8040                  */
/*   PORT_B: A15=1 A7=1 A6=0  -> (addr & 0x80c0) == 0x8080                  */
/*-------------------------------------------------------------------------*/

static uae_u8 atonce_porta_read(struct atonce_s *xb)
{
	uae_u8 v = 0;
	if (xb->trap_pending)
		v |= PORTA_ST_TRAP;
	if (xb->stopped) {
		if (xb->trap_dir)
			v |= PORTA_ST_ERR287_OR_DIR;
	} else {
		if (xb->err287)
			v |= PORTA_ST_ERR287_OR_DIR;
	}
	return v;
}

static void atonce_porta_write(struct atonce_s *xb, uae_u8 v)
{
	uae_u8 old = xb->mode;
	xb->mode = v;

	// D2: gate A20
	int a20 = (v & PORTA_A20) ? 1 : 0;
	if (a20 != xb->a20en) {
		xb->a20en = a20;
		atonce_rebuild_map(xb);
		atonce_apply_a20(xb);
	}

	// D1: 287 reset / MEMMODE setup mode
	xb->setup = (v & PORTA_SETUP) ? 1 : 0;

	// D3: 287 error acknowledge
	if (v & PORTA_ACK287)
		xb->err287 = 0;

	// D0: at config time = DIN; at runtime, a 0->1 edge = explicit 286 reset
	// (the driver's boot-retry pulses it when the 286 fails to POST). Edge-
	// triggered so the per-poll mode-byte write does not reset every time.
	if (xb->configured && (v & PORTA_DIN) && !(old & PORTA_DIN)) {
		xb->in_reset = 1;
		resetx86();     // 286 restarts at FFFF0
	}
}

// Fabricate a synthetic CRTC register-12 (display-start-address-high) write so the
// 68k driver runs a FULL-screen change-detection re-render of the CGA text page,
// flushing it to the Amiga screen even when the 286 issues no real video I/O (DOS
// prompt idle, BIOS "press any key" boot wait).
//
// Why not OUT 0xBA: that handler (0x694C->0x5AAE) only re-renders ONE row (the
// bottom row, the post-scroll case) -- it never touches an arbitrary row like the
// boot prompt at row 14 or a pre-scroll "A>" prompt. The full-screen render is
// 0x5B70 (25 rows, chip 0x18000 vs shadow 0x10000 -> bitplane 0x20000), reached via
// the driver's CRTC path: a write to reg 12/13 in text mode routes to 0x5174, which
// only sets the full-render request flags ($16C6 bit2 / $15B2 bit2) -- it does NOT
// apply the data to the display start -- and the main loop services the request.
// This is exactly the path DOS scrolling uses (which renders correctly). So we
// pre-stage the CRTC index in the I/O shadow (chip 0x303D4 = F000:03D4) and trap on
// the data port 0x3D5 with the current start value (a no-op write, no scroll).
// Cheap hash of the 286's live CGA text page (x86 B8000 = chip 0x18000), plus the
// CRTC start address and video mode. Used to skip the (expensive, 68k-side
// graphics.library) full-screen re-render when nothing on screen changed.
static uae_u32 atonce_text_hash(void)
{
	uae_u8 *chip = chipmem_bank.baseaddr;
	if (!chip || chipmem_bank.allocated_size < 0x30400)
		return 0;
	const uae_u8 *p = chip + 0x18000;        // live CGA text page
	uae_u32 h = 2166136261u;                 // FNV-1a
	for (int i = 0; i < 80 * 25 * 2; i++) {
		h ^= p[i];
		h *= 16777619u;
	}
	// fold in CRTC start (hardware scroll) + mode so those also trigger a refresh
	h ^= (uae_u32)chip[0x3158c] | ((uae_u32)chip[0x3158d] << 8) |
	     ((uae_u32)chip[0x315b5] << 16);
	return h;
}

static void atonce_fire_refresh(struct atonce_s *xb)
{
	uae_u8 *chip = chipmem_bank.baseaddr;
	if (!chip || chipmem_bank.allocated_size < 0x30400)
		return;
	chip[0x303d4] = 0x0c;            // CRTC index = reg 12 (start address high)
	chip[0x303d5] = chip[0x3158c];   // data = current start high -> no actual scroll
	xb->refresh_due = 0;
	xb->fake_active = 1;
	xb->trap_port = 0x3d5;
	xb->trap_dir = 0;                // OUT
	xb->trap_pending = 1;
	xb->frozen = 1;
}

// A synthetic refresh trap must not be injected from a resume that is part of a
// multi-step Vortex service or a disk/CRTC sequence: those handlers depend on the
// 286 PUSHA frame (which we transiently redirect) and on running the 286 promptly,
// so interposing a render there can derail a boot. Only inject after a "plain"
// device trap (keyboard/PIC/timer/RTC idle polls), where a resume cleanly continues.
static int atonce_refresh_safe_port(uae_u16 p)
{
	if (p >= 0x0b0 && p <= 0x0bf) return 0;   // Vortex private services (disk/video)
	if (p >= 0x3f0 && p <= 0x3ff) return 0;   // floppy controller -> trackdisk
	if (p >= 0x1f0 && p <= 0x1f7) return 0;   // AT hard disk
	if (p == 0x3d4 || p == 0x3d5) return 0;   // CGA CRTC
	if (p == 0x3b4 || p == 0x3b5) return 0;   // MDA CRTC
	return 1;
}

static uae_u16 atonce_portb_read(struct atonce_s *xb)
{
	// stop the 286 + latch trap info.
	xb->frozen = 1;
	xb->stopped = 1;
	// readback format: driver does AND #$FF03 + ROR #8 to recover the port
	return ((xb->trap_port & 0xff) << 8) | ((xb->trap_port >> 8) & 3);
}

static void atonce_portb_write(struct atonce_s *xb, uae_u16 v)
{
	if (xb->setup) {
		// MEMMODE shift: data = 68k D12, LSB first, into AH->BH->CH->DH
		xb->mm_d = xb->mm_c;
		xb->mm_c = xb->mm_b;
		xb->mm_b = xb->mm_a;
		xb->mm_a = (v >> 12) & 1;
		atonce_rebuild_map(xb);
		// first shift releases the 286 reset line (LCA DA flip-flop)
		xb->in_reset = 0;
		return;
	}
	// resume the 286; bit0 = also deliver INT, vector in the high byte
	// was_fake marks a resume that consumes a synthetic video-refresh trap, so we
	// don't immediately re-fire another one on its own resume.
	int was_fake = xb->fake_active;
	xb->fake_active = 0;
	xb->frozen = 0;
	xb->stopped = 0;
	xb->trap_pending = 0;
	xb->in_reset = 0;
	// The first resume after config/MEMMODE means "let the 286 run". There is
	// no real bitstream to load here (we ARE the LCA logic), so this is where
	// the board becomes live and the 286 starts at its reset vector (FFFF0).
	if (!xb->configured) {
		xb->configured = 1;
		atonce_rebuild_map(xb);
		resetx86();
		// resetx86() resets PCem's _mem_state (back to INTERNAL), which drops our
		// EXTERNAL chip-RAM windows from _mem_exec -> the 286 can't fetch the BIOS
		// (Bad getpccache at FFFF0). Re-assert the state so recalc reinstalls the
		// still-listed mappings. (Don't re-add the mappings: that corrupts the list.)
		mem_set_mem_state(0x80000, 0x80000, MEM_READ_EXTERNAL | MEM_WRITE_EXTERNAL);
		mem_set_mem_state(0x100000, 0x1000000 - 0x100000,
			MEM_READ_EXTERNAL | MEM_WRITE_EXTERNAL);
	}
	if (v & 1) {
		atonce_intr_vector = (v >> 8) & 0xff;
		pic_intpending = 1;
	}
	// If a video refresh is owed (set once per frame by atonce_hsync), inject it
	// here instead of running the 286: a resume is a safe point (the trap just
	// cleared, no real trap in flight). The driver picks up the synthetic 0xBA on
	// its next PORT_A poll, renders, then resumes again (refresh no longer due) and
	// the 286 runs. This covers busy 286 I/O-poll loops (e.g. the BIOS "press any
	// key" boot wait) where atonce_hsync never catches an idle moment. Gate it to
	// plain device traps (atonce_refresh_safe_port): never interpose a render in a
	// disk/video/service sequence, which derails the boot. Skip on a fake's own
	// resume (was_fake) so two renders don't chain.
	if (xb->refresh_due && !was_fake && atonce_refresh_safe_port(xb->trap_port)) {
		atonce_fire_refresh(xb);
		return;
	}
	// Trap-driven execution: run the 286 until its next I/O instruction traps
	// (atonce_trap forces the slice to end) or the cycle budget is spent.
	atonce_run(xb, ATONCE_RUN_CYCLES);
}

// returns 1 if the access was claimed (magic port hit)
int atonce_cia_access(uaecptr addr, uae_u32 *value, int size, int iswrite)
{
	struct atonce_s *xb = atonce;
	if (!xb)
		return 0;
	if ((addr & 0xff0000) != 0xbf0000)
		return 0;
	int hit_a = (addr & 0x80c0) == 0x8040;
	int hit_b = (addr & 0x80c0) == 0x8080;
	if (!hit_a && !hit_b)
		return 0;
	if (hit_a) {
		if (iswrite)
			atonce_porta_write(xb, (uae_u8)*value);
		else
			*value = atonce_porta_read(xb);
	} else {
		if (iswrite)
			atonce_portb_write(xb, size == 2 ? (uae_u16)*value : (uae_u8)*value);
		else
			*value = atonce_portb_read(xb);
	}
	return 1;
}

/*-------------------------------------------------------------------------*/
/* Amiga side: 68k window 0x900000-0x9FFFFF exposes the onboard DRAM        */
/* (LCA comparator AJ cuts the motherboard strobes via U43/U42).            */
/*-------------------------------------------------------------------------*/

// The 0x900000 DRAM window stays mapped (map_banks) even while atonce is NULL:
// before the first atonce_init and after every atonce_free (the board re-inits
// once per Amiga reset). Kickstart/expansion scans and stray 68k reads can hit
// it in that gap, so every handler must tolerate atonce == NULL / dram == NULL.
// While the board is unconfigured (DONE not asserted) the 286 DRAM window must
// stay inert and let any Z2 fast RAM underneath show through, so the OS can use
// that RAM during boot (it shadows only once the driver brings the board up).
// fastmem_bank[0] is looked up live (not captured at init, where it isn't mapped
// yet) so the range test reflects the actual configured fast RAM.
static addrbank *atonce_dram_under(uaecptr addr)
{
	addrbank *fb = &fastmem_bank[0];
	if (fb->baseaddr && addr >= fb->start &&
		addr < fb->start + fb->allocated_size)
		return fb;
	return NULL;
}

static uae_u32 REGPARAM2 atonce_dram_lget(uaecptr addr)
{
	struct atonce_s *xb = atonce;
	if (!xb || !xb->dram)
		return 0xffffffff;
	if (!xb->configured) {
		addrbank *u = atonce_dram_under(addr);
		if (u) return u->lget(addr);
	}
	addr &= ONBOARD_SIZE - 1;
	return (xb->dram[addr] << 24) | (xb->dram[addr + 1] << 16) |
		(xb->dram[addr + 2] << 8) | xb->dram[addr + 3];
}
static uae_u32 REGPARAM2 atonce_dram_wget(uaecptr addr)
{
	struct atonce_s *xb = atonce;
	if (!xb || !xb->dram)
		return 0xffff;
	if (!xb->configured) {
		addrbank *u = atonce_dram_under(addr);
		if (u) return u->wget(addr);
	}
	addr &= ONBOARD_SIZE - 1;
	return (xb->dram[addr] << 8) | xb->dram[addr + 1];
}
static uae_u32 REGPARAM2 atonce_dram_bget(uaecptr addr)
{
	struct atonce_s *xb = atonce;
	if (!xb || !xb->dram)
		return 0xff;
	if (!xb->configured) {
		addrbank *u = atonce_dram_under(addr);
		if (u) return u->bget(addr);
	}
	return xb->dram[addr & (ONBOARD_SIZE - 1)];
}
static void REGPARAM2 atonce_dram_lput(uaecptr addr, uae_u32 l)
{
	struct atonce_s *xb = atonce;
	if (!xb || !xb->dram)
		return;
	if (!xb->configured) {
		addrbank *u = atonce_dram_under(addr);
		if (u) { u->lput(addr, l); return; }
	}
	addr &= ONBOARD_SIZE - 1;
	xb->dram[addr] = l >> 24;
	xb->dram[addr + 1] = l >> 16;
	xb->dram[addr + 2] = l >> 8;
	xb->dram[addr + 3] = (uae_u8)l;
}
static void REGPARAM2 atonce_dram_wput(uaecptr addr, uae_u32 w)
{
	struct atonce_s *xb = atonce;
	if (!xb || !xb->dram)
		return;
	if (!xb->configured) {
		addrbank *u = atonce_dram_under(addr);
		if (u) { u->wput(addr, w); return; }
	}
	addr &= ONBOARD_SIZE - 1;
	xb->dram[addr] = w >> 8;
	xb->dram[addr + 1] = (uae_u8)w;
}
static void REGPARAM2 atonce_dram_bput(uaecptr addr, uae_u32 b)
{
	struct atonce_s *xb = atonce;
	if (!xb || !xb->dram)
		return;
	if (!xb->configured) {
		addrbank *u = atonce_dram_under(addr);
		if (u) { u->bput(addr, b); return; }
	}
	xb->dram[addr & (ONBOARD_SIZE - 1)] = (uae_u8)b;
}

static addrbank atonce_dram_bank = {
	atonce_dram_lget, atonce_dram_wget, atonce_dram_bget,
	atonce_dram_lput, atonce_dram_wput, atonce_dram_bput,
	default_xlate, default_check, NULL, NULL, _T("ATonce Plus DRAM"),
	atonce_dram_lget, atonce_dram_wget,
	ABFLAG_RAM | ABFLAG_SAFE, 0, 0
};

/*-------------------------------------------------------------------------*/
/* Execution scheduling: the 286 advances mainly inside a PORT_B resume     */
/* (synchronous, low latency). This hsync slice is the continuation path:   */
/* it keeps a still-running 286 going across hsyncs when a resume slice did  */
/* not reach an I/O trap (long computation). It stays off while frozen, so   */
/* it never double-runs against the resume slice.                           */
/*-------------------------------------------------------------------------*/

static void atonce_hsync(void)
{
	static float totalcycles;
	struct atonce_s *xb = atonce;
	if (!xb || !xb->configured || xb->in_reset)
		return;

	// Idle video refresh: the 68k driver only re-renders the Amiga screen on real
	// 286 video activity (a CRTC scroll, or the bottom-row OUT 0xBA). At the DOS
	// prompt the 286 sits idle, or text lands on a row that never triggers a render,
	// so the boot prompt / pre-scroll "A>" / typed chars stay invisible. So we owe a
	// synthetic full-screen refresh (atonce_fire_refresh fakes a CRTC reg-12 write),
	// serviced at the next safe point.
	//
	// PERFORMANCE: that re-render runs on the 68k via graphics.library and is heavy
	// (25 rows, char->bitplane). Forcing it every frame (50Hz) saturated the 68k ->
	// the prompt felt sluggish, keys took ~1s to appear and arrived in bursts (the
	// 68k was too busy rendering to promptly inject the keyboard IRQ / service the
	// 286's key traps / show the new text). Fix: only refresh when the 286's text
	// page ACTUALLY changed (cheap FNV hash once per frame), plus a slow ~2Hz
	// heartbeat so the blinking cursor still shows. Idle = no render -> 68k free.
	if (++xb->refresh_div >= maxvpos) {
		xb->refresh_div = 0;
		uae_u32 h = atonce_text_hash();
		xb->refresh_heartbeat++;
		int changed = (h != xb->text_hash);
		if (changed || xb->refresh_heartbeat >= 25) {
			xb->text_hash = h;
			xb->refresh_heartbeat = 0;
			xb->refresh_due = 1;
		}
	}
	// If the 286 happens to be idle right now, fire it immediately (low latency).
	// Otherwise atonce_portb_write services refresh_due at the next resume, which
	// keeps firing even through a busy poll loop. atonce_fire_refresh fakes a CRTC
	// reg-12 write -> driver full-screen re-render; see its comment.
	if (xb->refresh_due && !xb->frozen && !xb->trap_pending) {
		atonce_fire_refresh(xb);
		return;                          // driver will render + resume the 286
	}

	if (xb->frozen)
		return;

	float cycles_to_run = (float)cpu_get_speed() / (vblank_hz * maxvpos);
	totalcycles += cycles_to_run;
	int cycs = (int)totalcycles;
	if (cycs > 0)
		atonce_run(xb, cycs);   // same timer_target priming as the resume path
	totalcycles -= (int)totalcycles;
}

static void atonce_reset(int hardreset)
{
	struct atonce_s *xb = atonce;
	if (!xb)
		return;
	xb->configured = 0;
	xb->frozen = 0;
	xb->stopped = 0;
	xb->trap_pending = 0;
	xb->in_reset = 1;
	xb->a20en = 0;
	xb->setup = 0;
	xb->mm_a = xb->mm_b = xb->mm_c = xb->mm_d = 0;
	atonce_intr_vector = -1;
	atonce_rebuild_map(xb);
	atonce_apply_a20(xb);
}

static void atonce_free(void)
{
	if (atonce) {
		pcem_close();
		xfree(atonce->vmem);
		xfree(atonce->emem);
		xfree(atonce);
		atonce = NULL;
	}
	// Re-resolve the PC C: drive next boot (the hardfile handle is owned by the
	// filesystem layer; the user may have changed WinUAE's HDD settings).
	atonce_hfd = NULL;
	atonce_hdd_tried = 0;
}

/*-------------------------------------------------------------------------*/
/* Device init (expansion system, non-autoconfig board).                    */
/*-------------------------------------------------------------------------*/

bool atonce_init(struct autoconfig_info *aci)
{
	aci->zorro = 0;
	aci->addrbank = &expamem_null;
	if (!aci->doinit)
		return true;

	struct atonce_s *xb = xcalloc(struct atonce_s, 1);
	atonce = xb;
	xb->rc = aci->rc;

	atonce_hdd_tried = (xb->rc->device_settings & 1) ? 0 : -1;

	// PCem setup: bare 286, no PC support devices (the LCA has none; the
	// PIT/PIC/DMA/KBC/RTC are all emulated by the 68k driver through traps)
	romset = ROM_AMI286;     // model index only; no ROM is loaded
	model = 1;               // [286] entry of models[] (model_init is empty)
	cpu_manufacturer = 0;
	cpu = 4;                 // cpus_286[4] = "286/16" (16 MHz); the board is 286-16
	fpu_type = FPU_NONE;     // TODO: optional 287 (FPU_287) + err287 path
	AT = 1;
	cpu_set();

	// allocate the PCem memory-map table (pcemmap) before any mem_mapping_add
	initpcemvideo(NULL, false);

	mem_size = ONBOARD_SIZE / 1024;
	mem_init();
	mem_alloc();
	xb->dram = ram;          // PCem-owned 512KB = the board's local DRAM
#if ATONCE_ISOLATE_CHIP
	if (!xb->vmem)
		xb->vmem = xcalloc(uae_u8, ATONCE_VMEM_SIZE);
#endif

	// Minimal PCem core bring-up. We deliberately do NOT init the PC support
	// chips (pit/dma/keyboard/nvr): the LCA has none, the 68k driver emulates
	// them via the I/O traps. pic is still init'd because the CPU core calls
	// picinterrupt() (patched in pcem/pic.cpp to deliver atonce_intr_vector).
	timer_reset();
	pic_init();
	pic_reset();
	codegen_init();          // CPU recompiler tables
	// Turbo ON: cpu_get_speed() (which paces atonce_run via atonce_hsync) returns
	// cpu_turbo_speed = cpu_s->rspeed = the full 16 MHz. With turbo OFF, PCem caps
	// cpu_nonturbo_speed at 8 MHz (cpu.cpp ~229) for any 286 -> the emulated 286
	// ran at half clock (the DOS prompt felt 2-3x slower than real hardware).
	cpu_set_turbo(1);

	// catch-all I/O traps, except 287 range 0xF8-0xFF
	io_sethandlerx(0x0000, 0x00f8,
		atonce_io_inb, atonce_io_inw, atonce_io_inl,
		atonce_io_outb, atonce_io_outw, atonce_io_outl, xb);
	io_sethandlerx(0x0100, 0xff00,
		atonce_io_inb, atonce_io_inw, atonce_io_inl,
		atonce_io_outb, atonce_io_outw, atonce_io_outl, xb);

	// Sub-1MB chip-RAM windows (static, with exec pointers so the 286 can run
	// the BIOS from F000). Must come after mem_init (chip RAM is already
	// allocated by the WinUAE memory system at this point).
	atonce_map_windows();

	// Extended memory above 1MB.
#if ATONCE_ISOLATE_EXT
	// DIAGNOSTIC: back the whole >1MB window with a private, flat buffer mapped
	// with a REAL exec pointer, so PCem's code fetch (getpccache) works when
	// himem.sys relocates/runs in the HMA (>1MB). Fully isolates the 286's
	// extended memory from Amiga RAM (no shared fast RAM, no MEMMODE translation).
	// The atonce_chip_* handlers are generic linear-buffer handlers:
	//   exec ptr = emem            -> getpccache fetches emem[addr - 0x100000]
	//   priv     = emem - 0x100000 -> handlers index priv[addr] = emem[addr-0x100000]
	if (!xb->emem)
		xb->emem = xcalloc(uae_u8, ATONCE_EMEM_SIZE);
	mem_mapping_add(&atonce_mapping, 0x100000, ATONCE_EMEM_SIZE,
		atonce_chip_readb, atonce_chip_readw, atonce_chip_readl,
		atonce_chip_writeb, atonce_chip_writew, atonce_chip_writel,
		xb->emem, MEM_MAPPING_EXTERNAL, xb->emem - 0x100000);
	mem_set_mem_state(0x100000, ATONCE_EMEM_SIZE,
		MEM_READ_EXTERNAL | MEM_WRITE_EXTERNAL);
	write_log(_T("ATonce: extended memory ISOLATED, private %uMB flat buffer at x86 0x100000 (exec=%p)\n"),
		ATONCE_EMEM_SIZE / (1024 * 1024), xb->emem);
#else
	// MEMMODE-placed, dynamic translation onto Amiga RAM, function handlers (no
	// exec ptr - code fetch from >1MB needs the private buffer above).
	mem_mapping_add(&atonce_mapping, 0x100000, 0x1000000 - 0x100000,
		atonce_mem_readb, atonce_mem_readw, atonce_mem_readl,
		atonce_mem_writeb, atonce_mem_writew, atonce_mem_writel,
		NULL, MEM_MAPPING_EXTERNAL, xb);
	mem_set_mem_state(0x100000, 0x1000000 - 0x100000,
		MEM_READ_EXTERNAL | MEM_WRITE_EXTERNAL);
#endif

	atonce_reset(1);

	// 68k window to the onboard DRAM at 0x900000-0x9FFFFF. The handlers delegate to
	// the Z2 fast RAM underneath (fastmem_bank[0], looked up live - see
	// atonce_dram_under) while the board is unconfigured, so the OS can use that fast
	// RAM during boot; the window shadows only once the driver brings the board up.
	// This mirrors real HW (board inert from power-on). Without it, the OS allocates
	// at 0x900000 during early boot (e.g. an attached hardfile's RDB filesystem load)
	// and the accesses hit the 286 DRAM -> corruption -> crash / "no system disk".
	map_banks(&atonce_dram_bank, 0x900000 >> 16, 0x100000 >> 16, ONBOARD_SIZE);

	device_add_hsync(atonce_hsync);
	device_add_reset(atonce_reset);
	device_add_exit(atonce_free, NULL);

	write_log(_T("ATonce Plus initialized\n"));
	return true;
}

bool atonce_available(void)
{
	return atonce != NULL;
}

#endif /* WITH_X86 */
