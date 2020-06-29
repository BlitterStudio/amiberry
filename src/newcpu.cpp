/*
  * UAE - The Un*x Amiga Emulator
  *
  * MC68000 emulation
  *
  * (c) 1995 Bernd Schmidt
  */

#include "sysdeps.h"

#include "options.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "cpummu.h"
#include "autoconf.h"
#include "gui.h"
#include "savestate.h"
#include "blitter.h"
#include "ar.h"
#include "inputdevice.h"
#include "audio.h"
#include "threaddep/thread.h"
#include "bsdsocket.h"
#include "statusline.h"
#ifdef JIT
#include "jit/compemu.h"
#include <signal.h>
#else
/* Need to have these somewhere */
bool check_prefs_changed_comp (bool checkonly) { return false; }
#endif

/* Opcode of faulting instruction */
static uae_u32 last_op_for_exception_3;
/* PC at fault time */
static uaecptr last_addr_for_exception_3;
/* Address that generated the exception */
static uaecptr last_fault_for_exception_3;
/* read (0) or write (1) access */
static bool last_writeaccess_for_exception_3;
/* size */
static bool last_size_for_exception_3;
/* FC */
static int last_fc_for_exception_3;
/* Data (1) or instruction fetch (0) */
static int last_di_for_exception_3;
/* not instruction */
static bool last_notinstruction_for_exception_3;
/* set when writing exception stack frame */
static int exception_in_exception;

int cpu_cycles;
int m68k_pc_indirect;

static int cpu_prefs_changed_flag;

const int areg_byteinc[] = {1, 1, 1, 1, 1, 1, 1, 2};
const int imm8_table[] = {8, 1, 2, 3, 4, 5, 6, 7};

int movem_index1[256];
int movem_index2[256];
int movem_next[256];

cpuop_func* cpufunctbl[65536];

struct cputbl_data
{
	uae_s16 length;
	uae_s8 disp020[2];
	uae_u8 branch;
};

static struct cputbl_data cpudatatbl[65536];

struct mmufixup mmufixup[1];

#define COUNT_INSTRS 0

static uae_u64 fake_srp_030, fake_crp_030;
static uae_u32 fake_tt0_030, fake_tt1_030, fake_tc_030;
static uae_u16 fake_mmusr_030;

int cpu_last_stop_vpos, cpu_stopped_lines;

#if COUNT_INSTRS
static unsigned long instrcount[65536];
static uae_u16 opcodenums[65536];

static int compfn (const void *el1, const void *el2)
{
	return instrcount[*(const uae_u16 *)el1] < instrcount[*(const uae_u16 *)el2];
}

static TCHAR *icountfilename (void)
{
	TCHAR *name = getenv ("INSNCOUNT");
	if (name)
		return name;
	return (TCHAR *)(COUNT_INSTRS == 2 ? _T("frequent.68k") : _T("insncount"));
}

void dump_counts (void)
{
	FILE *f = fopen (icountfilename (), "w");
	unsigned long total;
	int i;

	write_log (_T("Writing instruction count file...\n"));
	for (i = 0; i < 65536; i++) {
		opcodenums[i] = i;
		total += instrcount[i];
	}
	qsort (opcodenums, 65536, sizeof (uae_u16), compfn);

	fprintf (f, "Total: %lu\n", total);
	for (i=0; i < 65536; i++) {
		unsigned long cnt = instrcount[opcodenums[i]];
		struct instr *dp;
		struct mnemolookup *lookup;
		if (!cnt)
			break;
		dp = table68k + opcodenums[i];
		for (lookup = lookuptab;lookup->mnemo != dp->mnemo; lookup++)
			;
		fprintf (f, "%04x: %8lu %s\n", opcodenums[i], cnt, lookup->name);
	}
	fclose (f);
}

STATIC_INLINE void count_instr (unsigned int opcode)
{
  instrcount[opcode]++;
}
#else
void dump_counts(void)
{
}

STATIC_INLINE void count_instr(unsigned int opcode)
{
}
#endif

uae_u32 (*x_get_long)(uaecptr);
uae_u32 (*x_get_word)(uaecptr);
uae_u32 (*x_get_byte)(uaecptr);
void (*x_put_long)(uaecptr, uae_u32);
void (*x_put_word)(uaecptr, uae_u32);
void (*x_put_byte)(uaecptr, uae_u32);

// indirect memory access functions
static void set_x_funcs(void)
{
	if (currprefs.cpu_model < 68020)
	{
		// 68000/010
		if (currprefs.cpu_compatible)
		{
			// cpu_compatible only
			x_put_long = put_long_compatible;
			x_put_word = put_word_compatible;
			x_put_byte = put_byte_compatible;
			x_get_long = get_long_compatible;
			x_get_word = get_word_compatible;
			x_get_byte = get_byte_compatible;
		}
		else
		{
			x_put_long = put_long;
			x_put_word = put_word;
			x_put_byte = put_byte;
			x_get_long = get_long;
			x_get_word = get_word;
			x_get_byte = get_byte;
		}
	}
	else
	{
		// 68020+ no ce
		if (currprefs.cachesize)
		{
			x_put_long = put_long_jit;
			x_put_word = put_word_jit;
			x_put_byte = put_byte_jit;
			x_get_long = get_long_jit;
			x_get_word = get_word_jit;
			x_get_byte = get_byte_jit;
		}
		else
		{
			x_put_long = put_long;
			x_put_word = put_word;
			x_put_byte = put_byte;
			x_get_long = get_long;
			x_get_word = get_word;
			x_get_byte = get_byte;
		}
	}
}

static void flush_cpu_caches(bool force)
{
	if (currprefs.cpu_model == 68020)
	{
		regs.cacr &= ~0x08;
		regs.cacr &= ~0x04;
	}
	else if (currprefs.cpu_model == 68030)
	{
		regs.cacr &= ~0x08;
		regs.cacr &= ~0x04;
		regs.cacr &= ~0x800;
		regs.cacr &= ~0x400;
	}
}

void flush_cpu_caches_040(uae_u16 opcode)
{
	// 0 (1) = data, 1 (2) = instruction
	int cache = (opcode >> 6) & 3;
	int scope = (opcode >> 3) & 3;

	for (int k = 0; k < 2; k++)
	{
		if (cache & (1 << k))
		{
			if (scope == 3)
			{
				// all
				if (!k)
				{
				}
				else
				{
					// instruction
					flush_cpu_caches(true);
				}
			}
		}
	}
}

void set_cpu_caches(bool flush)
{
#ifdef JIT
	if (currprefs.cachesize)
	{
		if (currprefs.cpu_model < 68040)
		{
			set_cache_state(regs.cacr & 1);
			if (regs.cacr & 0x08)
			{
				flush_icache(3);
			}
		}
		else
		{
			set_cache_state((regs.cacr & 0x8000) ? 1 : 0);
		}
	}
#endif
	flush_cpu_caches(flush);
}

static uae_u32 REGPARAM2 op_illg_1(uae_u32 opcode)
{
	op_illg(opcode);
	return 4;
}

// generic+direct, generic+direct+jit, more compatible
static const struct cputbl* cputbls[5][3] =
{
	// 68000
	{op_smalltbl_5_ff, op_smalltbl_45_ff, op_smalltbl_12_ff},
	// 68010
	{op_smalltbl_4_ff, op_smalltbl_44_ff, op_smalltbl_11_ff},
	// 68020
	{op_smalltbl_3_ff, op_smalltbl_43_ff, nullptr},
	// 68030
	{op_smalltbl_2_ff, op_smalltbl_42_ff, nullptr},
	// 68040
	{op_smalltbl_1_ff, op_smalltbl_41_ff, nullptr},
};

static void build_cpufunctbl(void)
{
	int i;
	unsigned long opcode;
	const struct cputbl* tbl = nullptr;
	int lvl, mode;

	if (!currprefs.cachesize)
	{
		if (currprefs.cpu_compatible && currprefs.cpu_model < 68020)
		{
			mode = 2;
		}
		else
		{
			mode = 0;
		}
		m68k_pc_indirect = mode != 0 ? 1 : 0;
	}
	else
	{
		mode = 1;
		m68k_pc_indirect = 0;
	}
	lvl = (currprefs.cpu_model - 68000) / 10;
	if (lvl >= 5)
		lvl = 4;
	tbl = cputbls[lvl][mode];

	if (tbl == nullptr)
	{
		write_log(_T("no CPU emulation cores available CPU=%d!"), currprefs.cpu_model);
		abort();
	}

	for (opcode = 0; opcode < 65536; opcode++)
		cpufunctbl[opcode] = op_illg_1;
	for (i = 0; tbl[i].handler != nullptr; i++)
	{
		opcode = tbl[i].opcode;
		cpufunctbl[opcode] = tbl[i].handler;
		cpudatatbl[opcode].length = tbl[i].length;
		cpudatatbl[opcode].disp020[0] = tbl[i].disp020[0];
		cpudatatbl[opcode].disp020[1] = tbl[i].disp020[1];
		cpudatatbl[opcode].branch = tbl[i].branch;
	}

	/* hack fpu to 68000/68010 mode */
	if (currprefs.fpu_model && currprefs.cpu_model < 68020)
	{
		tbl = op_smalltbl_3_ff;
		for (i = 0; tbl[i].handler != nullptr; i++)
		{
			if ((tbl[i].opcode & 0xfe00) == 0xf200)
			{
				cpufunctbl[tbl[i].opcode] = tbl[i].handler;
				cpudatatbl[tbl[i].opcode].length = tbl[i].length;
				cpudatatbl[tbl[i].opcode].disp020[0] = tbl[i].disp020[0];
				cpudatatbl[tbl[i].opcode].disp020[1] = tbl[i].disp020[1];
				cpudatatbl[tbl[i].opcode].branch = tbl[i].branch;
			}
		}
	}

	for (opcode = 0; opcode < 65536; opcode++)
	{
		cpuop_func* f;
		struct instr* table = &table68k[opcode];

		if (table->mnemo == i_ILLG)
			continue;

		/* unimplemented opcode? */
		if (table->unimpclev > 0 && lvl >= table->unimpclev)
		{
			cpufunctbl[opcode] = op_illg_1;
			continue;
		}

		if (currprefs.fpu_model && currprefs.cpu_model < 68020)
		{
			/* more hack fpu to 68000/68010 mode */
			if (table->clev > lvl && (opcode & 0xfe00) != 0xf200)
				continue;
		}
		else if (table->clev > lvl)
		{
			continue;
		}

		if (table->handler != -1)
		{
			int idx = table->handler;
			f = cpufunctbl[idx];
			if (f == op_illg_1)
				abort();
			cpufunctbl[opcode] = f;
			memcpy(&cpudatatbl[opcode], &cpudatatbl[idx], sizeof(struct cputbl_data));
		}
	}
#ifdef JIT
	build_comp();
#endif

	write_log(_T("CPU=%d, FPU=%d%s, JIT%s=%d."),
	          currprefs.cpu_model,
	          currprefs.fpu_model, currprefs.fpu_model ? _T(" (host)") : _T(""),
	          currprefs.cachesize ? (currprefs.compfpu ? _T("=CPU/FPU") : _T("=CPU")) : _T(""),
	          currprefs.cachesize);

	regs.address_space_mask = 0xffffffff;
	if (currprefs.cpu_compatible)
	{
		if (currprefs.address_space_24 && currprefs.cpu_model >= 68040)
			currprefs.address_space_24 = false;
	}

	if (currprefs.cpu_compatible)
	{
		write_log(_T(" prefetch"));
	}
	if (currprefs.m68k_speed < 0)
		write_log(_T(" fast"));
	if (currprefs.fpu_no_unimplemented && currprefs.fpu_model)
	{
		write_log(_T(" no unimplemented floating point instructions"));
	}
	if (currprefs.address_space_24)
	{
		regs.address_space_mask = 0x00ffffff;
		write_log(_T(" 24-bit"));
	}
	write_log(_T("\n"));

	set_cpu_caches(true);
}

static uae_u32 cycles_shift;
static uae_u32 cycles_shift_2;

static void update_68k_cycles(void)
{
	cycles_shift = 0;
	cycles_shift_2 = 0;
	if (currprefs.m68k_speed >= 0)
	{
		if (currprefs.m68k_speed == M68K_SPEED_14MHZ_CYCLES)
			cycles_shift = 1;
		else if (currprefs.m68k_speed == M68K_SPEED_25MHZ_CYCLES)
		{
			if (currprefs.cpu_model >= 68040)
			{
				cycles_shift = 4;
			}
			else
			{
				cycles_shift = 2;
				cycles_shift_2 = 5;
			}
		}
	}

	if (currprefs.m68k_speed < 0 || currprefs.cachesize > 0)
		do_cycles = do_cycles_cpu_fastest;
	else
		do_cycles = do_cycles_cpu_norm;

	set_config_changed();
}

static void prefs_changed_cpu(void)
{
	fixup_cpu(&changed_prefs);
	check_prefs_changed_comp(false);
	currprefs.cpu_model = changed_prefs.cpu_model;
	currprefs.fpu_model = changed_prefs.fpu_model;
	//if (currprefs.mmu_model != changed_prefs.mmu_model) {
	//	int oldmmu = currprefs.mmu_model;
	//	currprefs.mmu_model = changed_prefs.mmu_model;
	//	if (currprefs.mmu_model >= 68040) {
	//		uae_u32 tcr = regs.tcr;
	//		mmu_reset();
	//		mmu_set_tc(tcr);
	//		mmu_set_super(regs.s != 0);
	//		mmu_tt_modified();
	//	}
	//	else if (currprefs.mmu_model == 68030) {
	//		mmu030_reset(-1);
	//		mmu030_flush_atc_all();
	//		tc_030 = fake_tc_030;
	//		tt0_030 = fake_tt0_030;
	//		tt1_030 = fake_tt1_030;
	//		srp_030 = fake_srp_030;
	//		crp_030 = fake_crp_030;
	//		mmu030_decode_tc(tc_030, false);
	//	}
	//	else if (oldmmu == 68030) {
	//		fake_tc_030 = tc_030;
	//		fake_tt0_030 = tt0_030;
	//		fake_tt1_030 = tt1_030;
	//		fake_srp_030 = srp_030;
	//		fake_crp_030 = crp_030;
	//	}
	//}
	currprefs.mmu_ec = changed_prefs.mmu_ec;
	if (currprefs.cpu_compatible != changed_prefs.cpu_compatible)
	{
		currprefs.cpu_compatible = changed_prefs.cpu_compatible;
		flush_cpu_caches(true);
		//invalidate_cpu_data_caches();
	}
	if (currprefs.cpu_data_cache != changed_prefs.cpu_data_cache) {
		currprefs.cpu_data_cache = changed_prefs.cpu_data_cache;
		//invalidate_cpu_data_caches();
	}
	currprefs.address_space_24 = changed_prefs.address_space_24;
	currprefs.fpu_no_unimplemented = changed_prefs.fpu_no_unimplemented;
	currprefs.address_space_24 = changed_prefs.address_space_24;
	currprefs.cpu_cycle_exact = changed_prefs.cpu_cycle_exact;
	currprefs.cpu_memory_cycle_exact = changed_prefs.cpu_memory_cycle_exact;
	currprefs.int_no_unimplemented = changed_prefs.int_no_unimplemented;
	currprefs.fpu_no_unimplemented = changed_prefs.fpu_no_unimplemented;
	currprefs.blitter_cycle_exact = changed_prefs.blitter_cycle_exact;
}

static int check_prefs_changed_cpu2(void)
{
	int changed = 0;

#ifdef JIT
	changed = check_prefs_changed_comp(true) ? 1 : 0;
#endif
	if (changed
		|| currprefs.cpu_model != changed_prefs.cpu_model
		|| currprefs.fpu_model != changed_prefs.fpu_model
		|| currprefs.mmu_model != changed_prefs.mmu_model
		|| currprefs.mmu_ec != changed_prefs.mmu_ec
		|| currprefs.cpu_data_cache != changed_prefs.cpu_data_cache
		|| currprefs.int_no_unimplemented != changed_prefs.int_no_unimplemented
		|| currprefs.fpu_no_unimplemented != changed_prefs.fpu_no_unimplemented
		|| currprefs.cpu_compatible != changed_prefs.cpu_compatible
		|| currprefs.cpu_cycle_exact != changed_prefs.cpu_cycle_exact
		|| currprefs.cpu_memory_cycle_exact != changed_prefs.cpu_memory_cycle_exact
		|| currprefs.fpu_mode != changed_prefs.fpu_mode)
	{
		cpu_prefs_changed_flag |= 1;
	}
	if (changed
		|| currprefs.m68k_speed != changed_prefs.m68k_speed
		|| currprefs.m68k_speed_throttle != changed_prefs.m68k_speed_throttle
		|| currprefs.cpu_clock_multiplier != changed_prefs.cpu_clock_multiplier
		|| currprefs.reset_delay != changed_prefs.reset_delay
		|| currprefs.cpu_frequency != changed_prefs.cpu_frequency)
	{
		cpu_prefs_changed_flag |= 2;
	}
	return cpu_prefs_changed_flag;
}

void check_prefs_changed_cpu(void)
{
	if (!config_changed)
		return;

	currprefs.cpu_idle = changed_prefs.cpu_idle;
	currprefs.ppc_cpu_idle = changed_prefs.ppc_cpu_idle;
	currprefs.reset_delay = changed_prefs.reset_delay;
	currprefs.cpuboard_settings = changed_prefs.cpuboard_settings;
	
	if (check_prefs_changed_cpu2())
	{
		set_special(SPCFLAG_MODE_CHANGE);
		reset_frame_rate_hack();
	}
}

void init_m68k(void)
{
	prefs_changed_cpu();
	update_68k_cycles();

	for (int i = 0; i < 256; i++)
	{
		int j;
		for (j = 0; j < 8; j++)
		{
			if (i & (1 << j)) break;
		}
		movem_index1[i] = j;
		movem_index2[i] = 7 - j;
		movem_next[i] = i & (~(1 << j));
	}

#if COUNT_INSTRS
	memset (instrcount, 0, sizeof instrcount);
#endif

	read_table68k();
	do_merges();

	write_log (_T("%d CPU functions\n"), nr_cpuop_funcs);
}

struct regstruct regs;

STATIC_INLINE int in_rom(uaecptr pc)
{
	return (munge24(pc) & 0xFFF80000) == 0xF80000;
}

STATIC_INLINE int in_rtarea(uaecptr pc)
{
	return (munge24(pc) & 0xFFFF0000) == rtarea_base && (uae_boot_rom_type || currprefs.uaeboard > 0);
}

STATIC_INLINE uae_u32 adjust_cycles(uae_u32 cycles)
{
	uae_u32 res = cycles >> cycles_shift;
	if (cycles_shift_2)
		return res + (cycles >> cycles_shift_2);
	return res;
}

void m68k_cancel_idle(void)
{
	cpu_last_stop_vpos = -1;
}

static void m68k_set_stop(void)
{
	if (regs.stopped)
		return;
	regs.stopped = 1;
	set_special(SPCFLAG_STOP);
	if (cpu_last_stop_vpos >= 0) {
		cpu_last_stop_vpos = vpos;
	}
}

static void m68k_unset_stop(void)
{
	regs.stopped = 0;
	unset_special(SPCFLAG_STOP);
	if (cpu_last_stop_vpos >= 0) {
		cpu_stopped_lines += vpos - cpu_last_stop_vpos;
		cpu_last_stop_vpos = vpos;
	}
}

static void activate_trace(void)
{
	unset_special(SPCFLAG_TRACE);
	set_special(SPCFLAG_DOTRACE);
}

// make sure interrupt is checked immediately after current instruction
static void doint_imm(void)
{
	doint();
	if (!currprefs.cachesize && !(regs.spcflags & SPCFLAG_INT) && (regs.spcflags & SPCFLAG_DOINT))
		set_special(SPCFLAG_INT);
}

void REGPARAM2 MakeSR(void)
{
	regs.sr = ((regs.t1 << 15) | (regs.t0 << 14)
		| (regs.s << 13) | (regs.m << 12) | (regs.intmask << 8)
		| (GET_XFLG() << 4) | (GET_NFLG() << 3)
		| (GET_ZFLG() << 2) | (GET_VFLG() << 1)
		| GET_CFLG());
}

static void MakeFromSR_x(int t0trace)
{
	int oldm = regs.m;
	int olds = regs.s;
	int oldt0 = regs.t0;
	int oldt1 = regs.t1;

	SET_XFLG((regs.sr >> 4) & 1);
	SET_NFLG((regs.sr >> 3) & 1);
	SET_ZFLG((regs.sr >> 2) & 1);
	SET_VFLG((regs.sr >> 1) & 1);
	SET_CFLG(regs.sr & 1);
	if (regs.t1 == ((regs.sr >> 15) & 1) &&
		regs.t0 == ((regs.sr >> 14) & 1) &&
		regs.s == ((regs.sr >> 13) & 1) &&
		regs.m == ((regs.sr >> 12) & 1) &&
		regs.intmask == ((regs.sr >> 8) & 7))
		return;
	regs.t1 = (regs.sr >> 15) & 1;
	regs.t0 = (regs.sr >> 14) & 1;
	regs.s = (regs.sr >> 13) & 1;
	regs.m = (regs.sr >> 12) & 1;
	regs.intmask = (regs.sr >> 8) & 7;

	if (currprefs.cpu_model >= 68020)
	{
		if (olds != regs.s)
		{
			if (olds)
			{
				if (oldm)
					regs.msp = m68k_areg(regs, 7);
				else
					regs.isp = m68k_areg(regs, 7);
				m68k_areg(regs, 7) = regs.usp;
			}
			else
			{
				regs.usp = m68k_areg(regs, 7);
				m68k_areg(regs, 7) = regs.m ? regs.msp : regs.isp;
			}
		}
		else if (olds && oldm != regs.m)
		{
			if (oldm)
			{
				regs.msp = m68k_areg(regs, 7);
				m68k_areg(regs, 7) = regs.isp;
			}
			else
			{
				regs.isp = m68k_areg(regs, 7);
				m68k_areg(regs, 7) = regs.msp;
			}
		}
	}
	else
	{
		regs.t0 = regs.m = 0;
		if (olds != regs.s)
		{
			if (olds)
			{
				regs.isp = m68k_areg(regs, 7);
				m68k_areg(regs, 7) = regs.usp;
			}
			else
			{
				regs.usp = m68k_areg(regs, 7);
				m68k_areg(regs, 7) = regs.isp;
			}
		}
	}

	doint_imm();
	if (regs.t1 || regs.t0)
	{
		set_special(SPCFLAG_TRACE);
	}
	else
	{
		/* Keep SPCFLAG_DOTRACE, we still want a trace exception for
SR-modifying instructions (including STOP).  */
		unset_special(SPCFLAG_TRACE);
	}
	// Stop SR-modification does not generate T0
	// If this SR modification set Tx bit, no trace until next instruction.
	if ((oldt0 && t0trace && currprefs.cpu_model >= 68020) || oldt1)
	{
		// Always trace if Tx bits were already set, even if this SR modification cleared them.
		activate_trace();
	}
}

void REGPARAM2 MakeFromSR_T0(void)
{
	MakeFromSR_x(1);
}

void REGPARAM2 MakeFromSR(void)
{
	MakeFromSR_x(0);
}

static void exception_check_trace(int nr)
{
	unset_special(SPCFLAG_TRACE | SPCFLAG_DOTRACE);
  if (regs.t1) {
		/* trace stays pending if exception is div by zero, chk,
* trapv or trap #x
*/
		if (nr == 5 || nr == 6 || nr == 7 || (nr >= 32 && nr <= 47))
			set_special(SPCFLAG_DOTRACE);
	}
	regs.t1 = regs.t0 = 0;
}

static int iack_cycle(int nr)
{
	int vector;

	// non-autovectored
	vector = x_get_byte(0x00fffff1 | ((nr - 24) << 1));
	return vector;
}

static void add_approximate_exception_cycles(int nr)
{
	int cycles;

	if (currprefs.cpu_model > 68000)
	{
		// 68020 exceptions
		if (nr >= 24 && nr <= 31)
		{
			/* Interrupts */
			cycles = 26 * CYCLE_UNIT / 2;
		}
		else if (nr >= 32 && nr <= 47)
		{
			/* Trap */
			cycles = 20 * CYCLE_UNIT / 2;
		}
		else
		{
			switch (nr)
			{
			case 2: cycles = 43 * CYCLE_UNIT / 2;
				break; /* Bus error */
			case 3: cycles = 43 * CYCLE_UNIT / 2;
				break; /* Address error ??? */
			case 4: cycles = 20 * CYCLE_UNIT / 2;
				break; /* Illegal instruction */
			case 5: cycles = 32 * CYCLE_UNIT / 2;
				break; /* Division by zero */
			case 6: cycles = 32 * CYCLE_UNIT / 2;
				break; /* CHK */
			case 7: cycles = 32 * CYCLE_UNIT / 2;
				break; /* TRAPV */
			case 8: cycles = 20 * CYCLE_UNIT / 2;
				break; /* Privilege violation */
			case 9: cycles = 25 * CYCLE_UNIT / 2;
				break; /* Trace */
			case 10: cycles = 20 * CYCLE_UNIT / 2;
				break; /* Line-A */
			case 11: cycles = 20 * CYCLE_UNIT / 2;
				break; /* Line-F */
			default:
				cycles = 4 * CYCLE_UNIT / 2;
				break;
			}
		}
	}
	else
	{
		// 68000 exceptions
		if (nr >= 24 && nr <= 31)
		{
			/* Interrupts */
			cycles = (44 + 4) * CYCLE_UNIT / 2;
		}
		else if (nr >= 32 && nr <= 47)
		{
			/* Trap (total is 34, but cpuemux.c already adds 4) */
			cycles = (34 - 4) * CYCLE_UNIT / 2;
		}
		else
		{
			switch (nr)
			{
			case 2: cycles = 50 * CYCLE_UNIT / 2;
				break; /* Bus error */
			case 3: cycles = 50 * CYCLE_UNIT / 2;
				break; /* Address error */
			case 4: cycles = 34 * CYCLE_UNIT / 2;
				break; /* Illegal instruction */
			case 5: cycles = 38 * CYCLE_UNIT / 2;
				break; /* Division by zero */
			case 6: cycles = 40 * CYCLE_UNIT / 2;
				break; /* CHK */
			case 7: cycles = 34 * CYCLE_UNIT / 2;
				break; /* TRAPV */
			case 8: cycles = 34 * CYCLE_UNIT / 2;
				break; /* Privilege violation */
			case 9: cycles = 34 * CYCLE_UNIT / 2;
				break; /* Trace */
			case 10: cycles = 34 * CYCLE_UNIT / 2;
				break; /* Line-A */
			case 11: cycles = 34 * CYCLE_UNIT / 2;
				break; /* Line-F */
			default:
				cycles = 4 * CYCLE_UNIT / 2;
				break;
			}
		}
	}
	cycles = adjust_cycles(cycles);
	x_do_cycles(cycles);
}

static void exception3_notinstruction(uae_u32 opcode, uaecptr addr);

static void Exception_normal (int nr)
{
	uae_u32 newpc;
	uae_u32 currpc = m68k_getpc();
	uae_u32 nextpc;
	int sv = regs.s;
	int interrupt;
	int vector_nr = nr;

	interrupt = nr >= 24 && nr < 24 + 8;

	if (interrupt && currprefs.cpu_model <= 68010)
		vector_nr = iack_cycle(nr);

	MakeSR();

	if (!regs.s)
	{
		regs.usp = m68k_areg(regs, 7);
		if (currprefs.cpu_model >= 68020)
		{
			m68k_areg(regs, 7) = regs.m ? regs.msp : regs.isp;
		}
		else
		{
			m68k_areg(regs, 7) = regs.isp;
		}
		regs.s = 1;
	}

	if ((m68k_areg(regs, 7) & 1) && currprefs.cpu_model < 68020)
	{
		if (nr == 2 || nr == 3)
			cpu_halt(CPU_HALT_DOUBLE_FAULT);
		else
			exception3_notinstruction(regs.ir, m68k_areg(regs, 7));
		return;
	}
	if ((nr == 2 || nr == 3) && exception_in_exception < 0)
	{
		cpu_halt(CPU_HALT_DOUBLE_FAULT);
		return;
	}

	if (!currprefs.cpu_compatible)
	{
		addrbank* ab = &get_mem_bank(m68k_areg(regs, 7) - 4);
		// Not plain RAM check because some CPU type tests that
		// don't need to return set stack to ROM..
		if (!ab || ab == &dummy_bank || (ab->flags & ABFLAG_IO))
		{
			cpu_halt(CPU_HALT_SSP_IN_NON_EXISTING_ADDRESS);
			return;
		}
	}

	bool used_exception_build_stack_frame = false;

	add_approximate_exception_cycles(nr);
	if (currprefs.cpu_model > 68000)
	{
		uae_u32 oldpc = regs.instruction_pc;
		nextpc = exception_pc(nr);
		if (nr == 2 || nr == 3)
		{
			int i;
			if (currprefs.cpu_model >= 68040)
			{
				if (nr == 2)
				{
					// 68040 bus error (not really, some garbage?)
					for (i = 0; i < 18; i++)
					{
						m68k_areg(regs, 7) -= 2;
						x_put_word(m68k_areg(regs, 7), 0);
					}
					m68k_areg(regs, 7) -= 4;
					x_put_long(m68k_areg(regs, 7), last_fault_for_exception_3);
					m68k_areg(regs, 7) -= 2;
					x_put_word(m68k_areg(regs, 7), 0);
					m68k_areg(regs, 7) -= 2;
					x_put_word(m68k_areg(regs, 7), 0);
					m68k_areg(regs, 7) -= 2;
					x_put_word(m68k_areg(regs, 7), 0);
					m68k_areg(regs, 7) -= 2;
					x_put_word(m68k_areg(regs, 7), 0x0140 | (sv ? 6 : 2)); /* SSW */
					m68k_areg(regs, 7) -= 4;
					x_put_long(m68k_areg(regs, 7), last_addr_for_exception_3);
					m68k_areg(regs, 7) -= 2;
					x_put_word(m68k_areg(regs, 7), 0x7000 + vector_nr * 4);
					m68k_areg(regs, 7) -= 4;
					x_put_long(m68k_areg(regs, 7), regs.instruction_pc);
					m68k_areg(regs, 7) -= 2;
					x_put_word(m68k_areg(regs, 7), regs.sr);
					goto kludge_me_do;
				}
				// 68040/060 odd PC address error
				Exception_build_stack_frame(last_fault_for_exception_3, currpc, 0, nr, 0x02);
				used_exception_build_stack_frame = true;
			}
			else if (currprefs.cpu_model >= 68020)
			{
				// 68020/030 odd PC address error (partially implemented only)
				// annoyingly this generates frame B, not A.
    		uae_u16 ssw = (sv ? 4 : 0) | last_fc_for_exception_3;
    		ssw |= MMU030_SSW_RW | MMU030_SSW_SIZE_W;
				regs.mmu_fault_addr = last_fault_for_exception_3;
				Exception_build_stack_frame(last_fault_for_exception_3, currpc, ssw, nr, 0x0b);
				used_exception_build_stack_frame = true;
	    } else {
				// 68010 bus/address error (partially implemented only)
    		uae_u16 ssw = (sv ? 4 : 0) | last_fc_for_exception_3;
				ssw |= last_di_for_exception_3 ? 0x0000 : 0x2000; // IF
				ssw |= (!last_writeaccess_for_exception_3 && last_di_for_exception_3) ? 0x1000 : 0x000; // DF
				ssw |= (last_op_for_exception_3 & 0x10000) ? 0x0400 : 0x0000; // HB
				ssw |= last_size_for_exception_3 == 0 ? 0x0200 : 0x0000; // BY
				ssw |= last_writeaccess_for_exception_3 ? 0x0000 : 0x0100; // RW
				if (last_op_for_exception_3 & 0x20000)
					ssw &= 0x00ff;
				regs.mmu_fault_addr = last_addr_for_exception_3;
				Exception_build_stack_frame(oldpc, currpc, ssw, nr, 0x08);
				used_exception_build_stack_frame = true;
			}
	  } else if (regs.m && interrupt) { /* M + Interrupt */
			m68k_areg(regs, 7) -= 2;
			x_put_word(m68k_areg(regs, 7), vector_nr * 4);
			m68k_areg(regs, 7) -= 4;
			x_put_long(m68k_areg(regs, 7), currpc);
			m68k_areg(regs, 7) -= 2;
			x_put_word(m68k_areg(regs, 7), regs.sr);
			regs.sr |= (1 << 13);
			regs.msp = m68k_areg(regs, 7);
			regs.m = 0;
			m68k_areg(regs, 7) = regs.isp;
			m68k_areg(regs, 7) -= 2;
			x_put_word(m68k_areg(regs, 7), 0x1000 + vector_nr * 4);
		}
		else
		{
			Exception_build_stack_frame_common(oldpc, currpc, nr);
			used_exception_build_stack_frame = true;
		}
	}
	else
	{
		nextpc = m68k_getpc();
    if (nr == 2 || nr == 3) {
	    // 68000 bus/address error
			uae_u16 mode = (sv ? 4 : 0) | last_fc_for_exception_3;
			mode |= last_writeaccess_for_exception_3 ? 0 : 16;
			mode |= last_notinstruction_for_exception_3 ? 8 : 0;
			exception_in_exception = -1;
			Exception_build_68000_address_error_stack_frame(mode, last_op_for_exception_3, last_fault_for_exception_3, last_addr_for_exception_3);
			goto kludge_me_do;
		}
	}
	if (!used_exception_build_stack_frame)
	{
		m68k_areg(regs, 7) -= 4;
		x_put_long(m68k_areg(regs, 7), nextpc);
		m68k_areg(regs, 7) -= 2;
		x_put_word(m68k_areg(regs, 7), regs.sr);
	}
kludge_me_do:
	newpc = x_get_long(regs.vbr + 4 * vector_nr);
	exception_in_exception = 0;
	if (newpc & 1)
	{
		if (nr == 2 || nr == 3)
			cpu_halt(CPU_HALT_DOUBLE_FAULT);
		else
			exception3_notinstruction(regs.ir, newpc);
		return;
	}
	if (interrupt)
		regs.intmask = nr - 24;
	m68k_setpc(newpc);
#ifdef JIT
	set_special(SPCFLAG_END_COMPILE);
#endif
	fill_prefetch();
	exception_check_trace(nr);
}

// address = format $2 stack frame address field
static void ExceptionX (int nr)
{
	regs.exception = nr;

#ifdef JIT
  if (currprefs.cachesize)
	  regs.instruction_pc = m68k_getpc ();
#endif

	{
		Exception_normal(nr);
	}
	regs.exception = 0;
}

void REGPARAM2 Exception_cpu(int nr)
{
	bool t0 = currprefs.cpu_model >= 68020 && regs.t0 && !regs.t1;
	ExceptionX (nr);
	// Check T0 trace
	// RTE format error ignores T0 trace
	if (nr != 14) {
	  if (t0) {
		activate_trace();
	}
}
}
void REGPARAM2 Exception (int nr)
{
	ExceptionX (nr);
}

static void bus_error(void)
{
	TRY(prb2)
	{
		Exception(2);
	} CATCH(prb2)
	{
		cpu_halt(CPU_HALT_BUS_ERROR_DOUBLE_FAULT);
	}
	ENDTRY
}

static void do_interrupt(int nr)
{
	m68k_unset_stop();

	for (;;)
	{
		Exception(nr + 24);
		if (!currprefs.cpu_compatible)
			break;
		nr = intlev();
		if (nr <= 0 || regs.intmask >= nr)
			break;
	}

	doint();
}

void NMI(void)
{
	do_interrupt(7);
}

static void maybe_disable_fpu(void)
{
	if (!currprefs.fpu_model) {
		regs.pcr |= 2;
	}
}

static void m68k_reset_sr(void)
{
	SET_XFLG((regs.sr >> 4) & 1);
	SET_NFLG((regs.sr >> 3) & 1);
	SET_ZFLG((regs.sr >> 2) & 1);
	SET_VFLG((regs.sr >> 1) & 1);
	SET_CFLG(regs.sr & 1);
	regs.t1 = (regs.sr >> 15) & 1;
	regs.t0 = (regs.sr >> 14) & 1;
	regs.s = (regs.sr >> 13) & 1;
	regs.m = (regs.sr >> 12) & 1;
	regs.intmask = (regs.sr >> 8) & 7;
	/* set stack pointer */
	if (regs.s)
		m68k_areg(regs, 7) = regs.isp;
	else
		m68k_areg(regs, 7) = regs.usp;
}

static void m68k_reset(bool hardreset)
{
	uae_u32 v;

	regs.pissoff = 0;
	cpu_cycles = 0;

	regs.halted = 0;
	gui_data.cpu_halted = 0;
	gui_led(LED_CPU, 0, -1);

	regs.spcflags = 0;

#ifdef SAVESTATE
  if (isrestore ()) {
		m68k_reset_sr();
		m68k_setpc_normal(regs.pc);
		return;
	} else {
		set_special(SPCFLAG_CHECK);
	}
#endif
	regs.s = 1;
	v = get_long(4);
	m68k_areg(regs, 7) = get_long(0);

	m68k_setpc_normal(v);
	regs.m = 0;
	regs.stopped = 0;
	regs.t1 = 0;
	regs.t0 = 0;
	SET_ZFLG(0);
	SET_XFLG(0);
	SET_CFLG(0);
	SET_VFLG(0);
	SET_NFLG(0);
	regs.intmask = 7;
	regs.vbr = regs.sfc = regs.dfc = 0;
	regs.irc = 0xffff;
#ifdef FPUEMU
	fpu_reset();
#endif
	regs.caar = regs.cacr = 0;
	regs.itt0 = regs.itt1 = regs.dtt0 = regs.dtt1 = 0;
	regs.tcr = regs.mmusr = regs.urp = regs.srp = regs.buscr = 0;
	if (currprefs.cpu_model == 68020)
	{
		regs.cacr |= 8;
		set_cpu_caches(false);
	}

	mmufixup[0].reg = -1;
	if (currprefs.cpu_model >= 68040)
	{
		set_cpu_caches(false);
	}
	/* only (E)nable bit is zeroed when CPU is reset, A3000 SuperKickstart expects this */
	fake_tc_030 &= ~0x80000000;
	fake_tt0_030 &= ~0x80000000;
	fake_tt1_030 &= ~0x80000000;
	if (hardreset || regs.halted)
	{
		fake_srp_030 = fake_crp_030 = 0;
		fake_tt0_030 = fake_tt1_030 = fake_tc_030 = 0;
	}
	fake_mmusr_030 = 0;

	regs.pcr = 0;

	fill_prefetch();
}

uae_u32 REGPARAM2 op_illg(uae_u32 opcode)
{
	uaecptr pc = m68k_getpc();
	int inrom = in_rom(pc);
	int inrt = in_rtarea(pc);

	if ((opcode == 0x4afc || opcode == 0xfc4a) && !valid_address(pc, 4) && valid_address(pc - 4, 4))
	{
		// PC fell off the end of RAM
		bus_error();
		return 4;
	}

	if (cloanto_rom && (opcode & 0xF100) == 0x7100)
	{
		m68k_dreg(regs, (opcode >> 9) & 7) = static_cast<uae_s8>(opcode & 0xFF);
		m68k_incpc_normal(2);
		fill_prefetch();
		return 4;
	}

	if (opcode == 0x4E7B && inrom)
	{
		if (get_long(0x10) == 0)
		{
			notify_user(NUMSG_KS68020);
			uae_restart(-1, nullptr);
			m68k_setstopped();
			return 4;
		}
	}

#ifdef AUTOCONFIG
	if (opcode == 0xFF0D && inrt)
	{
		/* User-mode STOP replacement */
		m68k_setstopped();
		return 4;
	}

	if ((opcode & 0xF000) == 0xA000 && inrt)
	{
		/* Calltrap. */
		m68k_incpc_normal(2);
		m68k_handle_trap(opcode & 0xFFF);
		fill_prefetch();
		return 4;
	}
#endif

  if ((opcode & 0xF000) == 0xF000) {
		// Missing MMU or FPU cpSAVE/cpRESTORE privilege check
		if (privileged_copro_instruction(opcode)) {
			Exception(8);
		} else {
		Exception(0xB);
		}
		return 4;
	}
	if ((opcode & 0xF000) == 0xA000)
	{
		Exception(0xA);
		return 4;
	}

	Exception(4);
	return 4;
}

#ifdef CPUEMU_0

static bool mmu_op30_invea(uae_u32 opcode)
{
	int eamode = (opcode >> 3) & 7;
	int rreg = opcode & 7;

	// Dn, An, (An)+, -(An), immediate and PC-relative not allowed
	if (eamode == 0 || eamode == 1 || eamode == 3 || eamode == 4 || (eamode == 7 && rreg > 1))
		return true;
	return false;
}

static bool mmu_op30fake_pmove(uaecptr pc, uae_u32 opcode, uae_u16 next, uaecptr extra)
{
	int preg = (next >> 10) & 31;
	int rw = (next >> 9) & 1;
	int fd = (next >> 8) & 1;
	int unused = (next & 0xff);
	const TCHAR* reg = nullptr;
	uae_u32 otc = fake_tc_030;
	int siz;

	if (mmu_op30_invea(opcode))
		return true;
	// unused low 8 bits must be zeroed
	if (unused)
		return true;
	// read and fd set?
	if (rw && fd)
		return true;

	switch (preg)
	{
	case 0x10: // TC
		reg = _T("TC");
		siz = 4;
		if (rw)
			x_put_long(extra, fake_tc_030);
		else
			fake_tc_030 = x_get_long(extra);
		break;
	case 0x12: // SRP
		reg = _T("SRP");
		siz = 8;
		if (rw)
		{
			x_put_long(extra, fake_srp_030 >> 32);
			x_put_long(extra + 4, static_cast<uae_u32>(fake_srp_030));
		}
		else
		{
			fake_srp_030 = static_cast<uae_u64>(x_get_long(extra)) << 32;
			fake_srp_030 |= x_get_long(extra + 4);
		}
		break;
	case 0x13: // CRP
		reg = _T("CRP");
		siz = 8;
		if (rw)
		{
			x_put_long(extra, fake_crp_030 >> 32);
			x_put_long(extra + 4, static_cast<uae_u32>(fake_crp_030));
		}
		else
		{
			fake_crp_030 = static_cast<uae_u64>(x_get_long(extra)) << 32;
			fake_crp_030 |= x_get_long(extra + 4);
		}
		break;
	case 0x18: // MMUSR
		if (fd)
		{
			// FD must be always zero when MMUSR read or write
			return true;
		}
		reg = _T("MMUSR");
		siz = 2;
		if (rw)
			x_put_word(extra, fake_mmusr_030);
		else
			fake_mmusr_030 = x_get_word(extra);
		break;
	case 0x02: // TT0
		reg = _T("TT0");
		siz = 4;
		if (rw)
			x_put_long(extra, fake_tt0_030);
		else
			fake_tt0_030 = x_get_long(extra);
		break;
	case 0x03: // TT1
		reg = _T("TT1");
		siz = 4;
		if (rw)
			x_put_long(extra, fake_tt1_030);
		else
			fake_tt1_030 = x_get_long(extra);
		break;
	}

	if (!reg)
		return true;
	return false;
}

static bool mmu_op30fake_ptest(uaecptr pc, uae_u32 opcode, uae_u16 next, uaecptr extra)
{
	int level = (next & 0x1C00) >> 10;
	int a = (next >> 8) & 1;

	if (mmu_op30_invea(opcode))
		return true;
	if (!level && a)
		return true;

	fake_mmusr_030 = 0;
	return false;
}

static bool mmu_op30fake_pload(uaecptr pc, uae_u32 opcode, uae_u16 next, uaecptr extra)
{
	int unused = (next & (0x100 | 0x80 | 0x40 | 0x20));

	if (mmu_op30_invea(opcode))
		return true;
	if (unused)
		return true;
	return false;
}

static bool mmu_op30fake_pflush(uaecptr pc, uae_u32 opcode, uae_u16 next, uaecptr extra)
{
	int flushmode = (next >> 8) & 31;
	int fc_bits = next & 0x7f;

	switch (flushmode)
	{
	case 0x00:
	case 0x02:
		return mmu_op30fake_pload(pc, opcode, next, extra);
	case 0x18:
		if (mmu_op30_invea(opcode))
			return true;
		break;
	case 0x10:
		break;
	case 0x04:
		if (fc_bits)
			return true;
		break;
	default:
		return true;
	}
	return false;
}

// 68030 (68851) MMU instructions only
bool mmu_op30(uaecptr pc, uae_u32 opcode, uae_u16 extra, uaecptr extraa)
{
	int type = extra >> 13;
	bool fline = false;

	switch (type)
	{
	case 0:
	case 2:
	case 3:
		fline = mmu_op30fake_pmove(pc, opcode, extra, extraa);
		break;
	case 1:
		fline = mmu_op30fake_pflush(pc, opcode, extra, extraa);
		break;
	case 4:
		fline = mmu_op30fake_ptest(pc, opcode, extra, extraa);
		break;
	}
	if (fline)
	{
		m68k_setpc(pc);
		op_illg(opcode);
	}
	return fline;
}

/* check if an address matches a ttr */
static int fake_mmu_do_match_ttr(uae_u32 ttr, uaecptr addr, bool super)
{
	if (ttr & MMU_TTR_BIT_ENABLED)
	{
		/* TTR enabled */
		uae_u8 msb, mask;

		msb = ((addr ^ ttr) & MMU_TTR_LOGICAL_BASE) >> 24;
		mask = (ttr & MMU_TTR_LOGICAL_MASK) >> 16;

		if (!(msb & ~mask))
		{
			if ((ttr & MMU_TTR_BIT_SFIELD_ENABLED) == 0)
			{
				if (((ttr & MMU_TTR_BIT_SFIELD_SUPER) == 0) != (super == 0))
				{
					return TTR_NO_MATCH;
				}
			}

			return (ttr & MMU_TTR_BIT_WRITE_PROTECT) ? TTR_NO_WRITE : TTR_OK_MATCH;
		}
	}
	return TTR_NO_MATCH;
}

static int fake_mmu_match_ttr(uaecptr addr, bool super, bool data)
{
	int res;

	if (data)
	{
		res = fake_mmu_do_match_ttr(regs.dtt0, addr, super);
		if (res == TTR_NO_MATCH)
			res = fake_mmu_do_match_ttr(regs.dtt1, addr, super);
	}
	else
	{
		res = fake_mmu_do_match_ttr(regs.itt0, addr, super);
		if (res == TTR_NO_MATCH)
			res = fake_mmu_do_match_ttr(regs.itt1, addr, super);
	}
	return res;
}

// 68040+ MMU instructions only
void mmu_op(uae_u32 opcode, uae_u32 extra)
{
	if ((opcode & 0xFE0) == 0x0500)
	{
		/* PFLUSH */
		regs.mmusr = 0;
		return;
	}
	if ((opcode & 0x0FD8) == 0x0548)
	{
		/* PTEST */
		int regno = opcode & 7;
		uae_u32 addr = m68k_areg(regs, regno);
		bool write = (opcode & 32) == 0;
		bool super = (regs.dfc & 4) != 0;
		bool data = (regs.dfc & 3) != 2;

		regs.mmusr = 0;
		if (fake_mmu_match_ttr(addr, super, data) != TTR_NO_MATCH)
		{
			regs.mmusr = MMU_MMUSR_T | MMU_MMUSR_R;
		}
		regs.mmusr |= addr & 0xfffff000;
		return;
	}
	m68k_setpc_normal(m68k_getpc() - 2);
	op_illg(opcode);
}

#endif

static void do_trace(void)
{
  if (regs.t0 && !regs.t1 && currprefs.cpu_model >= 68020) {
		// this is obsolete
		return;
	}
	if (regs.t1)
	{
		activate_trace();
	}
}

static void check_uae_int_request(void)
{
	if (uae_int_requested)
	{
		bool irq = false;
		if (uae_int_requested & 0x00ff)
		{
			INTREQ_f(0x8000 | 0x0008);
			irq = true;
		}
		if (uae_int_requested & 0xff00)
		{
			INTREQ_f(0x8000 | 0x2000);
			irq = true;
		}
		if (uae_int_requested & 0xff0000)
		{
			atomic_and(&uae_int_requested, ~0x010000);
		}
		if (irq)
		{
			doint();
		}
	}
}

int cpu_sleep_millis(int ms)
{
	return sleep_millis_main(ms);
}

static bool haltloop(void)
{
	while (regs.halted)
	{
		static int prevvpos;
		if (vpos == 0 && prevvpos)
		{
			prevvpos = 0;
			cpu_sleep_millis(8);
		}
		if (vpos)
			prevvpos = 1;
		x_do_cycles(8 * CYCLE_UNIT);

		if (regs.spcflags & SPCFLAG_COPPER)
			do_copper();

		if (regs.spcflags)
		{
			if ((regs.spcflags & (SPCFLAG_BRK | SPCFLAG_MODE_CHANGE)))
				return true;
		}
	}

	return false;
}

void doint(void)
{
	if (currprefs.cpu_compatible)
		set_special(SPCFLAG_INT);
	else
		set_special(SPCFLAG_DOINT);
}

static int do_specialties(int cycles)
{
	if (regs.spcflags & SPCFLAG_MODE_CHANGE)
		return 1;

	if (regs.spcflags & SPCFLAG_CHECK)
	{
		if (regs.halted)
		{
			unset_special(SPCFLAG_CHECK);
			if (haltloop())
				return 1;
		}
		unset_special(SPCFLAG_CHECK);
	}

#ifdef ACTION_REPLAY
#ifdef ACTION_REPLAY_HRTMON
	if ((regs.spcflags & SPCFLAG_ACTION_REPLAY) && hrtmon_flag != ACTION_REPLAY_INACTIVE)
	{
		int isinhrt = (m68k_getpc() >= hrtmem_start && m68k_getpc() < hrtmem_start + hrtmem_size);
		/* exit from HRTMon? */
		if (hrtmon_flag == ACTION_REPLAY_ACTIVE && !isinhrt)
			hrtmon_hide();
		/* HRTMon breakpoint? (not via IRQ7) */
		if (hrtmon_flag == ACTION_REPLAY_ACTIVATE)
			hrtmon_enter();
	}
#endif
	if ((regs.spcflags & SPCFLAG_ACTION_REPLAY) && action_replay_flag != ACTION_REPLAY_INACTIVE)
	{
		/*if (action_replay_flag == ACTION_REPLAY_ACTIVE && !is_ar_pc_in_rom ())*/
		/*	write_log (_T("PC:%p\n"), m68k_getpc ());*/

		if (action_replay_flag == ACTION_REPLAY_ACTIVATE || action_replay_flag == ACTION_REPLAY_DORESET)
			action_replay_enter();
		if ((action_replay_flag == ACTION_REPLAY_HIDE || action_replay_flag == ACTION_REPLAY_ACTIVE) && !
			is_ar_pc_in_rom())
		{
			action_replay_hide();
			unset_special(SPCFLAG_ACTION_REPLAY);
		}
		if (action_replay_flag == ACTION_REPLAY_WAIT_PC)
		{
			/*write_log (_T("Waiting for PC: %p, current PC= %p\n"), wait_for_pc, m68k_getpc ());*/
			if (m68k_getpc() == wait_for_pc)
			{
				action_replay_flag = ACTION_REPLAY_ACTIVATE; /* Activate after next instruction. */
			}
		}
	}
#endif

	if (regs.spcflags & SPCFLAG_COPPER)
		do_copper();

#ifdef JIT
	unset_special(SPCFLAG_END_COMPILE); /* has done its job */
#endif

	while ((regs.spcflags & SPCFLAG_BLTNASTY) && dmaen(DMA_BLITTER) && cycles > 0)
	{
		int c = blitnasty();
		if (c > 0)
		{
			cycles -= c * CYCLE_UNIT * 2;
			if (cycles < CYCLE_UNIT)
				cycles = 0;
		}
		else
		{
			c = 4;
		}
		x_do_cycles(c * CYCLE_UNIT);
		if (regs.spcflags & SPCFLAG_COPPER)
			do_copper();
	}

	if (regs.spcflags & SPCFLAG_DOTRACE)
		Exception(9);

	if ((regs.spcflags & SPCFLAG_STOP) && regs.s == 0 && currprefs.cpu_model <= 68010)
	{
		// 68000/68010 undocumented special case:
		// if STOP clears S-bit and T was not set:
		// cause privilege violation exception, PC pointing to following instruction.
		// If T was set before STOP: STOP works as documented.
		m68k_unset_stop();
		Exception(8);
	}

	bool first = true;
	while ((regs.spcflags & SPCFLAG_STOP) && !(regs.spcflags & SPCFLAG_BRK))
	{
		check_uae_int_request();

		if (!first)
			x_do_cycles(4 * CYCLE_UNIT);
		first = false;
		if (regs.spcflags & SPCFLAG_COPPER)
			do_copper();

		if (regs.spcflags & (SPCFLAG_INT | SPCFLAG_DOINT))
		{
			int intr = intlev();
			unset_special(SPCFLAG_INT | SPCFLAG_DOINT);
			if (intr > 0 && intr > regs.intmask)
				do_interrupt(intr);
		}

		if (regs.spcflags & SPCFLAG_MODE_CHANGE)
		{
			m68k_resumestopped();
			return 1;
		}
	}

	if (regs.spcflags & SPCFLAG_TRACE)
		do_trace();

	if (regs.spcflags & SPCFLAG_INT)
	{
		int intr = intlev();
		unset_special(SPCFLAG_INT | SPCFLAG_DOINT);
		if (intr > 0 && (intr > regs.intmask || intr == 7))
			do_interrupt(intr);
	}

	if (regs.spcflags & SPCFLAG_DOINT)
	{
		unset_special(SPCFLAG_DOINT);
		set_special(SPCFLAG_INT);
	}

	if (regs.spcflags & SPCFLAG_BRK)
	{
		unset_special(SPCFLAG_BRK);
	}

	return 0;
}

/* It's really sad to have two almost identical functions for this, but we
do it all for performance... :(
This version emulates 68000's prefetch "cache" */
static void m68k_run_1(void)
{
	struct regstruct* r = &regs;
	bool exit = false;

	while (!exit)
	{
		TRY(prb)
		{
			while (!exit)
			{
				r->opcode = r->ir;

#if defined (CPU_arm) && defined(USE_ARMNEON)
				// Well not really since pli is ArmV7...
				/* Load ARM code for next opcode into L2 cache during execute of do_cycles() */
				__asm__ volatile ("pli [%[radr]]\n\t":: [radr]"r"(cpufunctbl[r->opcode]):);
#endif
				count_instr(r->opcode);

				do_cycles(cpu_cycles);
				r->instruction_pc = m68k_getpc();
				cpu_cycles = (*cpufunctbl[r->opcode])(r->opcode);
				cpu_cycles = adjust_cycles(cpu_cycles);
				if (r->spcflags)
				{
					if (do_specialties(cpu_cycles))
						exit = true;
				}
				if (!currprefs.cpu_compatible)
					exit = true;
			}
		} CATCH(prb)
		{
			bus_error();
			if (r->spcflags)
			{
				if (do_specialties(cpu_cycles))
					exit = true;
			}
		}
		ENDTRY
	}
}

#ifdef JIT  /* Completely different run_2 replacement */

void execute_exception(uae_u32 cycles)
{
	countdown -= cycles;
	Exception_cpu(regs.jit_exception);
	regs.jit_exception = 0;
	cpu_cycles = adjust_cycles(4 * CYCLE_UNIT / 2);
	do_cycles(cpu_cycles);
	// after leaving this function, we fall back to execute_normal()
}

void do_nothing(void)
{
	/* What did you expect this to do? */
	do_cycles(0);
	/* I bet you didn't expect *that* ;-) */
}

void exec_nostats(void)
{
	struct regstruct* r = &regs;

	for (;;)
	{
		r->opcode = get_diword(0);
		cpu_cycles = (*cpufunctbl[r->opcode])(r->opcode);
		cpu_cycles = adjust_cycles(cpu_cycles);

		do_cycles(cpu_cycles);

		if (end_block(r->opcode) || r->spcflags || uae_int_requested)
			return; /* We will deal with the spcflags in the caller */
	}
}

void execute_normal(void)
{
	struct regstruct* r = &regs;
	int blocklen;
	cpu_history pc_hist[MAXRUN];
	int total_cycles;

	if (check_for_cache_miss())
		return;

	total_cycles = 0;
	blocklen = 0;
	start_pc_p = r->pc_oldp;
	start_pc = r->pc;
	for (;;)
	{
		/* Take note: This is the do-it-normal loop */
		regs.instruction_pc = m68k_getpc();
		r->opcode = get_diword(0);

		special_mem = 0;
		pc_hist[blocklen].location = (uae_u16*)r->pc_p;

		cpu_cycles = (*cpufunctbl[r->opcode])(r->opcode);
		cpu_cycles = adjust_cycles(cpu_cycles);
		do_cycles(cpu_cycles);
		total_cycles += cpu_cycles;
		pc_hist[blocklen].specmem = special_mem;
		blocklen++;
		if (end_block(r->opcode) || blocklen >= MAXRUN || r->spcflags || uae_int_requested)
		{
			compile_block(pc_hist, blocklen, total_cycles);
			return; /* We will deal with the spcflags in the caller */
		}
		/* No need to check regs.spcflags, because if they were set,
we'd have ended up inside that "if" */
	}
}

typedef void compiled_handler(void);

static void m68k_run_jit(void)
{
	for (;;)
	{
		((compiled_handler*)pushall_call_handler)();
		/* Whenever we return from that, we should check spcflags */
		check_uae_int_request();
		if (regs.spcflags)
		{
			if (do_specialties(0))
			{
				return;
			}
		}
	}
}
#endif /* JIT */

void cpu_halt(int id)
{
	// id > 0: emulation halted.
	if (!regs.halted)
	{
		write_log(_T("CPU halted: reason = %d PC=%08x\n"), id, M68K_GETPC);
		regs.halted = id;
		gui_data.cpu_halted = id;
		gui_led(LED_CPU, 0, -1);
		regs.intmask = 7;
		MakeSR();
		audio_deactivate();
	}
	set_special(SPCFLAG_CHECK);
}

/* Same thing, but don't use prefetch to get opcode.  */
static void m68k_run_2(void)
{
	struct regstruct* r = &regs;
	bool exit = false;

	while (!exit)
	{
		TRY(prb)
		{
			while (!exit)
			{
				r->instruction_pc = m68k_getpc();

				r->opcode = get_diword(0);
#if defined (CPU_arm) && defined(USE_ARMNEON)
				// Well not really since pli is ArmV7...
				/* Load ARM code for next opcode into L2 cache during execute of do_cycles() */
				__asm__ volatile ("pli [%[radr]]\n\t"::[radr]"r"(cpufunctbl[r->opcode]):);
#endif
				count_instr(r->opcode);

				do_cycles(cpu_cycles);

				cpu_cycles = (*cpufunctbl[r->opcode])(r->opcode);
				cpu_cycles = adjust_cycles(cpu_cycles);

				if (r->spcflags)
				{
					if (do_specialties(cpu_cycles))
						exit = true;
				}
			}
		} CATCH(prb)
		{
			bus_error();
			if (r->spcflags)
			{
				if (do_specialties(cpu_cycles))
					exit = true;
			}
		}
		ENDTRY
	}
}

static int in_m68k_go = 0;

static bool cpu_hardreset, cpu_keyboardreset;

bool is_hardreset(void)
{
	return cpu_hardreset;
}
bool is_keyboardreset(void)
{
	return  cpu_keyboardreset;
}

#ifdef USE_JIT_FPU
static uae_u8 fp_buffer[9 * 16];
#endif

void m68k_go(int may_quit)
{
	int hardboot = 1;
	int startup = 1;

	if (in_m68k_go || !may_quit)
	{
		write_log(_T("Bug! m68k_go is not reentrant.\n"));
		abort();
	}

#ifdef USE_JIT_FPU
#ifdef CPU_AARCH64
	save_host_fp_regs(fp_buffer);
#else
	// This caused crashes in RockChip 32-bit platforms unless it was inlined like this
	__asm__ volatile ("vstmia %[fp_buffer]!, {d7-d15}"::[fp_buffer] "r" (fp_buffer));
#endif
#endif

	reset_frame_rate_hack();
	update_68k_cycles();

	cpu_prefs_changed_flag = 0;
	in_m68k_go++;
	for (;;)
	{
		int restored = 0;
		void (*run_func)(void);

		if (quit_program > 0)
		{
			cpu_keyboardreset = quit_program == UAE_RESET_KEYBOARD;
			cpu_hardreset = ((quit_program == UAE_RESET_HARD ? 1 : 0) | hardboot) != 0;

			if (quit_program == UAE_QUIT)
				break;

			hsync_counter = 0;
			quit_program = 0;
			hardboot = 0;

#ifdef SAVESTATE
			if (savestate_state == STATE_DORESTORE)
				savestate_state = STATE_RESTORE;
			if (savestate_state == STATE_RESTORE)
				restore_state(savestate_fname);
#endif
			prefs_changed_cpu();
			build_cpufunctbl();
			set_x_funcs();
			set_cycles(0);
			custom_reset(cpu_hardreset != 0, cpu_keyboardreset);
			m68k_reset(cpu_hardreset != 0);
			if (cpu_hardreset)
			{
				memory_clear();
				write_log(_T("hardreset, memory cleared\n"));
			}
			cpu_hardreset = false;
#ifdef SAVESTATE
			/* We may have been restoring state, but we're done now.  */
			if (isrestore())
			{
				savestate_restore_finish();
				startup = 1;
				restored = 1;
			}
#endif
			if (currprefs.produce_sound == 0)
				eventtab[ev_audio].active = false;
			m68k_setpc_normal(regs.pc);
			check_prefs_changed_audio();

			if (!restored || hsync_counter == 0)
				savestate_check();
			statusline_clear();
		}

		if (regs.spcflags & SPCFLAG_MODE_CHANGE)
		{
			if (cpu_prefs_changed_flag & 1)
			{
				uaecptr pc = m68k_getpc();
				prefs_changed_cpu();
				custom_cpuchange();
				build_cpufunctbl();
				m68k_setpc_normal(pc);
				fill_prefetch();
				update_68k_cycles();
			}
			if (cpu_prefs_changed_flag & 2)
			{
				fixup_cpu(&changed_prefs);
				currprefs.m68k_speed = changed_prefs.m68k_speed;
				update_68k_cycles();
			}
			cpu_prefs_changed_flag = 0;
		}

		set_x_funcs();
		if (startup)
		{
			custom_prepare();
			protect_roms(true);
		}
		startup = 0;
		unset_special(SPCFLAG_MODE_CHANGE);

#ifdef SAVESTATE
		if (restored)
		{
			restored = 0;
			savestate_restore_final();
		}
#endif

		if (!regs.halted)
		{
			// check that PC points to something that looks like memory.
			uaecptr pc = m68k_getpc();
			addrbank* ab = &get_mem_bank(pc);
			if (ab == nullptr || ab == &dummy_bank || (!currprefs.cpu_compatible && !valid_address(pc, 2)) || (pc & 1))
			{
				cpu_halt(CPU_HALT_INVALID_START_ADDRESS);
			}
		}
		if (regs.halted)
		{
			cpu_halt(regs.halted);
		}

		run_func =
			currprefs.cpu_compatible && currprefs.cpu_model <= 68010
				? m68k_run_1
				:
#ifdef JIT
				currprefs.cpu_model >= 68020 && currprefs.cachesize
					? m68k_run_jit
					:
#endif
					m68k_run_2;
		run_func();
	}
	protect_roms(false);

	// Prepare for a restart: reset pc
	regs.pc = 0;
	regs.pc_p = nullptr;
	regs.pc_oldp = nullptr;

#ifdef USE_JIT_FPU
#ifdef CPU_AARCH64
	restore_host_fp_regs(fp_buffer);
#else
	// This caused crashes in RockChip platforms unless it was inlined like this
	__asm__ volatile ("vldmia %[fp_buffer]!, {d7-d15}" ::[fp_buffer] "r"(fp_buffer));
#endif
#endif

	in_m68k_go--;
}

#ifdef SAVESTATE

/* CPU save/restore code */

#define CPUTYPE_EC 1
#define CPUMODE_HALT 1

uae_u8* restore_cpu(uae_u8* src)
{
	int flags, model;
	uae_u32 l;

	currprefs.cpu_model = changed_prefs.cpu_model = model = restore_u32();
	flags = restore_u32();
	changed_prefs.address_space_24 = false;
	if (flags & CPUTYPE_EC)
		changed_prefs.address_space_24 = true;
	currprefs.address_space_24 = changed_prefs.address_space_24;
	currprefs.cpu_compatible = changed_prefs.cpu_compatible;
	for (int i = 0; i < 15; i++)
		regs.regs[i] = restore_u32();
	regs.pc = restore_u32();
	regs.irc = restore_u16();
	regs.ir = restore_u16();
	regs.usp = restore_u32();
	regs.isp = restore_u32();
	regs.sr = restore_u16();
	l = restore_u32();
	if (l & CPUMODE_HALT)
	{
		regs.stopped = 1;
	}
	else
	{
		regs.stopped = 0;
	}
	if (model >= 68010)
	{
		regs.dfc = restore_u32();
		regs.sfc = restore_u32();
		regs.vbr = restore_u32();
	}
	if (model >= 68020)
	{
		regs.caar = restore_u32();
		regs.cacr = restore_u32();
		regs.msp = restore_u32();
	}
	if (model >= 68030)
	{
		fake_crp_030 = restore_u64();
		fake_srp_030 = restore_u64();
		fake_tt0_030 = restore_u32();
		fake_tt1_030 = restore_u32();
		fake_tc_030 = restore_u32();
		fake_mmusr_030 = restore_u16();
	}
	if (model >= 68040)
	{
		regs.itt0 = restore_u32();
		regs.itt1 = restore_u32();
		regs.dtt0 = restore_u32();
		regs.dtt1 = restore_u32();
		regs.tcr = restore_u32();
		regs.urp = restore_u32();
		regs.srp = restore_u32();
	}
	if (flags & 0x80000000)
	{
		int khz = restore_u32();
		restore_u32();
		if (khz > 0 && khz < 800000)
			currprefs.m68k_speed = changed_prefs.m68k_speed = 0;
	}
	set_cpu_caches(true);

	m68k_reset_sr();

	write_log(_T("CPU: %d%s%03d, PC=%08X\n"),
	          model / 1000, flags & 1 ? _T("EC") : _T(""), model % 1000, regs.pc);

	return src;
}

static void fill_prefetch_quick(void)
{
	if (currprefs.cpu_model >= 68020)
	{
		fill_prefetch();
		return;
	}
	// old statefile compatibility, this needs to done,
	// even in 68000 cycle-exact mode
	regs.ir = get_word(m68k_getpc());
	regs.irc = get_word(m68k_getpc() + 2);
}

void restore_cpu_finish(void)
{
	if (!currprefs.fpu_model)
		fpu_reset();
	init_m68k();
	m68k_setpc_normal(regs.pc);
	doint();
	fill_prefetch_quick();
	set_cycles(0);
	events_schedule();
	if (regs.stopped)
		set_special(SPCFLAG_STOP);
}

uae_u8* restore_cpu_extra(uae_u8* src)
{
	restore_u32();
	uae_u32 flags = restore_u32();

	currprefs.cpu_compatible = changed_prefs.cpu_compatible = (flags & 2) ? true : false;

	currprefs.m68k_speed = changed_prefs.m68k_speed = 0;
	if (flags & 4)
		currprefs.m68k_speed = changed_prefs.m68k_speed = -1;
	if (flags & 16)
		currprefs.m68k_speed = changed_prefs.m68k_speed = (flags >> 24) * CYCLE_UNIT;

	return src;
}

uae_u8* save_cpu_extra(int* len, uae_u8* dstptr)
{
	uae_u8 *dstbak, *dst;
	uae_u32 flags;

	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc(uae_u8, 1000);
	save_u32(0); // version
	flags = 0;
	flags |= currprefs.cpu_compatible ? 2 : 0;
	flags |= currprefs.m68k_speed < 0 ? 4 : 0;
	flags |= currprefs.cachesize > 0 ? 8 : 0;
	flags |= currprefs.m68k_speed > 0 ? 16 : 0;
	if (currprefs.m68k_speed > 0)
		flags |= (currprefs.m68k_speed / CYCLE_UNIT) << 24;
	save_u32(flags);
	*len = dst - dstbak;
	return dstbak;
}

uae_u8* save_cpu(int* len, uae_u8* dstptr)
{
	uae_u8 *dstbak, *dst;
	int model, khz;

	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc(uae_u8, 1000);
	model = currprefs.cpu_model;
	save_u32(model); /* MODEL */
	save_u32(0x80000000 | (currprefs.address_space_24 ? 1 : 0)); /* FLAGS */
	for (int i = 0; i < 15; i++)
		save_u32(regs.regs[i]); /* D0-D7 A0-A6 */
	save_u32(m68k_getpc ()); /* PC */
	save_u16(regs.irc); /* prefetch */
	save_u16(regs.ir); /* instruction prefetch */
	MakeSR();
	save_u32(!regs.s ? regs.regs[15] : regs.usp); /* USP */
	save_u32(regs.s ? regs.regs[15] : regs.isp); /* ISP */
	save_u16(regs.sr); /* SR/CCR */
	save_u32(regs.stopped ? CPUMODE_HALT : 0); /* flags */
	if (model >= 68010)
	{
		save_u32(regs.dfc); /* DFC */
		save_u32(regs.sfc); /* SFC */
		save_u32(regs.vbr); /* VBR */
	}
	if (model >= 68020)
	{
		save_u32(regs.caar); /* CAAR */
		save_u32(regs.cacr); /* CACR */
		save_u32(regs.msp); /* MSP */
	}
	if (model >= 68030)
	{
		save_u64(fake_crp_030); /* CRP */
		save_u64(fake_srp_030); /* SRP */
		save_u32(fake_tt0_030); /* TT0/AC0 */
		save_u32(fake_tt1_030); /* TT1/AC1 */
		save_u32(fake_tc_030); /* TCR */
		save_u16(fake_mmusr_030); /* MMUSR/ACUSR */
	}
	if (model >= 68040)
	{
		save_u32(regs.itt0); /* ITT0 */
		save_u32(regs.itt1); /* ITT1 */
		save_u32(regs.dtt0); /* DTT0 */
		save_u32(regs.dtt1); /* DTT1 */
		save_u32(regs.tcr); /* TCR */
		save_u32(regs.urp); /* URP */
		save_u32(regs.srp); /* SRP */
	}
	khz = -1;
	if (currprefs.m68k_speed == 0)
	{
		khz = currprefs.ntscmode ? 715909 : 709379;
		if (currprefs.cpu_model >= 68020)
			khz *= 2;
	}
	save_u32(khz); // clock rate in KHz: -1 = fastest possible 
	save_u32(0); // spare
	*len = dst - dstbak;
	return dstbak;
}

#endif /* SAVESTATE */

static void exception3f (uae_u32 opcode, uaecptr addr, bool writeaccess, bool instructionaccess, bool notinstruction, uaecptr pc, int size, bool plus2, int fc)
{
	if (currprefs.cpu_model >= 68040)
		addr &= ~1;
	if (currprefs.cpu_model >= 68020)
	{
		if (pc == 0xffffffff)
			last_addr_for_exception_3 = regs.instruction_pc;
		else
			last_addr_for_exception_3 = pc;
	}
	else if (pc == 0xffffffff)
	{
		last_addr_for_exception_3 = m68k_getpc();
		if (plus2)
			last_addr_for_exception_3 += 2;
	}
	else
	{
		last_addr_for_exception_3 = pc;
	}
	last_fault_for_exception_3 = addr;
	last_op_for_exception_3 = opcode;
	last_writeaccess_for_exception_3 = writeaccess;
	last_fc_for_exception_3 = fc >= 0 ? fc : (instructionaccess ? 2 : 1);
	last_notinstruction_for_exception_3 = notinstruction;
	last_size_for_exception_3 = size;
	Exception(3);
}

static void exception3_notinstruction(uae_u32 opcode, uaecptr addr)
{
	exception3f (opcode, addr, true, false, true, 0xffffffff, 1, false, -1);
}
void exception3_read(uae_u32 opcode, uaecptr addr, int size, int fc)
{
	bool ni = false;
	if (currprefs.cpu_model == 68000 && currprefs.cpu_compatible) {
		if (generates_group1_exception(regs.ir) && !(opcode & 0x20000)) {
			ni = true;
			fc = -1;
		}
		if (opcode & 0x10000)
			ni = true;
		opcode = regs.ir;
}
	exception3f (opcode, addr, false, 0, ni, 0xffffffff, size, false, fc);
}
void exception3_write(uae_u32 opcode, uaecptr addr, int size, uae_u32 val, int fc)
{
	bool ni = false;
	if (currprefs.cpu_model == 68000 && currprefs.cpu_compatible) {
		if (generates_group1_exception(regs.ir) && !(opcode & 0x20000)) {
			ni = true;
			fc = -1;
		}
		if (opcode & 0x10000)
			ni = true;
		opcode = regs.ir;
}
	exception3f (opcode, addr, true, 0, ni, 0xffffffff, size, false, fc);
	regs.write_buffer = val;
}
void exception3i(uae_u32 opcode, uaecptr addr)
{
	exception3f (opcode, addr, 0, 1, false, 0xffffffff, 1, true, -1);
}
void exception3b(uae_u32 opcode, uaecptr addr, bool w, bool i, uaecptr pc)
{
	exception3f (opcode, addr, w, i, false, pc, 1, true, -1);
}

void exception2_setup(uaecptr addr, bool read, int size, uae_u32 fc)
{
	uae_u32 opcode = last_op_for_exception_3;
	last_addr_for_exception_3 = m68k_getpc();
	last_fault_for_exception_3 = addr;
	last_writeaccess_for_exception_3 = read == 0;
	last_fc_for_exception_3 = fc;
	last_op_for_exception_3 = regs.opcode;
	last_notinstruction_for_exception_3 = exception_in_exception != 0;
	last_size_for_exception_3 = size;
	last_di_for_exception_3 = 1;

	if (currprefs.cpu_model == 68000 && currprefs.cpu_compatible) {
		if (generates_group1_exception(regs.ir) && !(opcode & 0x20000)) {
			last_notinstruction_for_exception_3 = true;
			fc = -1;
		}
		if (opcode & 0x10000)
			last_notinstruction_for_exception_3 = true;
		if (!(opcode & 0x20000))
			last_op_for_exception_3 = regs.ir;
	}
}

void exception2(uaecptr addr, bool read, int size, uae_u32 fc)
{
	exception2_setup(addr, read, size == 1 ? 0 : (size == 2 ? 1 : 2), fc);
	THROW(2);
}

void cpureset(void)
{
	/* RESET hasn't increased PC yet, 1 word offset */
	uaecptr pc;
	uaecptr ksboot = 0xf80002 - 2;
	uae_u16 ins;
	addrbank* ab;

	maybe_disable_fpu();
	set_special(SPCFLAG_CHECK);
	send_internalevent(INTERNALEVENT_CPURESET);
	if (currprefs.cpu_compatible && currprefs.cpu_model <= 68020)
	{
		custom_reset(false, false);
		return;
	}
	pc = m68k_getpc() + 2;
	ab = &get_mem_bank(pc);
	if (ab->check(pc, 2))
	{
		write_log(_T("CPU reset PC=%x (%s)..\n"), pc - 2, ab->name);
		ins = get_word(pc);
		custom_reset(false, false);
		// did memory disappear under us?
		if (ab == &get_mem_bank(pc))
			return;
		// it did
		if ((ins & ~7) == 0x4ed0)
		{
			int reg = ins & 7;
			uae_u32 addr = m68k_areg(regs, reg);
			if (addr < 0x80000)
				addr += 0xf80000;
			write_log(_T("reset/jmp (ax) combination at %08x emulated -> %x\n"), pc, addr);
			m68k_setpc_normal(addr - 2);
			return;
		}
	}
	// the best we can do, jump directly to ROM entrypoint
	// (which is probably what program wanted anyway)
	write_log(_T("CPU Reset PC=%x (%s), invalid memory -> %x.\n"), pc, ab->name, ksboot + 2);
	custom_reset(false, false);
	m68k_setpc_normal(ksboot);
}

void m68k_setstopped(void)
{
	/* A traced STOP instruction drops through immediately without
actually stopping.  */
	if ((regs.spcflags & SPCFLAG_DOTRACE) == 0)
	{
		m68k_set_stop();
	}
	else
	{
		m68k_resumestopped();
	}
}

void m68k_resumestopped(void)
{
	if (!regs.stopped)
		return;
	fill_prefetch();
	m68k_unset_stop();
}

void check_t0_trace(void)
{
	if (regs.t0 && !regs.t1 && currprefs.cpu_model >= 68020) {
		unset_special(SPCFLAG_TRACE);
		set_special(SPCFLAG_DOTRACE);
	}
}

void fill_prefetch(void)
{
	if (currprefs.cachesize)
		return;
	if (!currprefs.cpu_compatible)
		return;
	uaecptr pc = m68k_getpc();
	regs.ir = x_get_word(pc);
	regs.irc = x_get_word(pc + 2);
}
