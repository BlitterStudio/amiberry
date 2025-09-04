/*
* UAE - The Un*x Amiga Emulator
*
* MC68000 emulation
*
* (c) 1995 Bernd Schmidt
*/

#define MMUOP_DEBUG 0
#define DEBUG_CD32CDTVIO 0
#define EXCEPTION3_DEBUGGER 0
#define CPUTRACE_DEBUG 0

#define VALIDATE_68030_DATACACHE 0
#define VALIDATE_68040_DATACACHE 0
#define DISABLE_68040_COPYBACK 0

#define MORE_ACCURATE_68020_PIPELINE 1

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "events.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "cpummu.h"
#include "cpummu030.h"
#include "cputbl.h"
#include "cpu_prefetch.h"
#include "autoconf.h"
#include "traps.h"
#include "debug.h"
#include "debugmem.h"
#include "gui.h"
#include "savestate.h"
#include "blitter.h"
#include "ar.h"
#include "inputrecord.h"
#include "inputdevice.h"
#include "audio.h"
#include "fpp.h"
#include "statusline.h"
#ifdef WITH_PPC
#include "uae/ppc.h"
#endif
#include "cpuboard.h"
#include "threaddep/thread.h"
#ifdef WITH_X86
#include "x86.h"
#endif
#include "devices.h"
#ifdef WITH_DRACO
#include "draco.h"
#endif
#ifdef JIT
#include "jit/compemu.h"
#include <signal.h>
#else
/* Need to have these somewhere */
bool check_prefs_changed_comp (bool checkonly) { return false; }
#endif
/* For faster JIT cycles handling */
int pissoff = 0;

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
/* secondary SR for handling 68040 bug */
static uae_u16 last_sr_for_exception3;

static void exception3_read_special(uae_u32 opcode, uaecptr addr, int size, int fc);

int mmu_enabled, mmu_triggered;
int cpu_cycles;
int hardware_bus_error;
static int baseclock;
int m68k_pc_indirect;
bool m68k_interrupt_delay;
static bool m68k_accurate_ipl;
static bool m68k_reset_delay;
static bool ismoves_nommu;
static bool need_opcode_swap;

static volatile uae_atomic uae_interrupt;
static volatile uae_atomic uae_interrupts2[IRQ_SOURCE_MAX];
static volatile uae_atomic uae_interrupts6[IRQ_SOURCE_MAX];

static int cacheisets04060, cacheisets04060mask, cacheitag04060mask;
static int cachedsets04060, cachedsets04060mask, cachedtag04060mask;

static int cpu_prefs_changed_flag;

int cpuipldelay2, cpuipldelay4;
int cpucycleunit;
int cpu_tracer;

const int areg_byteinc[] = { 1, 1, 1, 1, 1, 1, 1, 2 };
const int imm8_table[] = { 8, 1, 2, 3, 4, 5, 6, 7 };

int movem_index1[256];
int movem_index2[256];
int movem_next[256];

cpuop_func *cpufunctbl[65536];
cpuop_func_noret *cpufunctbl_noret[65536];
cpuop_func *loop_mode_table[65536];

struct cputbl_data
{
	uae_s16 length;
	uae_s8 disp020[2];
	uae_s8 branch;
};
static struct cputbl_data cpudatatbl[65536];

struct mmufixup mmufixup[2];

#define COUNT_INSTRS 0
#define MC68060_PCR   0x04300000
#define MC68EC060_PCR 0x04310000

static uae_u64 fake_srp_030, fake_crp_030;
static uae_u32 fake_tt0_030, fake_tt1_030, fake_tc_030;
static uae_u16 fake_mmusr_030;

static struct cache020 caches020[CACHELINES020];
static struct cache030 icaches030[CACHELINES030];
static struct cache030 dcaches030[CACHELINES030];
static int icachelinecnt, icachehalfline;
static int dcachelinecnt;
static struct cache040 icaches040[CACHESETS060];
static struct cache040 dcaches040[CACHESETS060];
static int cache_lastline; 

static int fallback_cpu_model, fallback_mmu_model, fallback_fpu_model;
static bool fallback_cpu_compatible, fallback_cpu_address_space_24;
static struct regstruct fallback_regs;
static int fallback_new_cpu_model;

int cpu_last_stop_vpos, cpu_stopped_lines;

void (*flush_icache)(int);

#if COUNT_INSTRS
static unsigned long int instrcount[65536];
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
	return COUNT_INSTRS == 2 ? "frequent.68k" : "insncount";
}

void dump_counts (void)
{
	FILE *f = fopen (icountfilename (), "w");
	unsigned long int total;
	int i;

	write_log (_T("Writing instruction count file...\n"));
	for (i = 0; i < 65536; i++) {
		opcodenums[i] = i;
		total += instrcount[i];
	}
	qsort (opcodenums, 65536, sizeof (uae_u16), compfn);

	fprintf (f, "Total: %lu\n", total);
	for (i=0; i < 65536; i++) {
		unsigned long int cnt = instrcount[opcodenums[i]];
		struct instr *dp;
		struct mnemolookup *lookup;
		if (!cnt)
			break;
		dp = table68k + opcodenums[i];
		for (lookup = lookuptab;lookup->mnemo != dp->mnemo; lookup++)
			;
		fprintf (f, "%04x: %lu %s\n", opcodenums[i], cnt, lookup->name);
	}
	fclose (f);
}
#else
void dump_counts ()
{
}
#endif

/*

 ok, all this to "record" current instruction state
 for later 100% cycle-exact restoring

 */

static uae_u32 (*x2_prefetch)(int);
static uae_u32 (*x2_prefetch_long)(int);
static uae_u32 (*x2_next_iword)();
static uae_u32 (*x2_next_ilong)();
static uae_u32 (*x2_get_ilong)(int);
static uae_u32 (*x2_get_iword)(int);
static uae_u32 (*x2_get_ibyte)(int);
static uae_u32 (*x2_get_long)(uaecptr);
static uae_u32 (*x2_get_word)(uaecptr);
static uae_u32 (*x2_get_byte)(uaecptr);
static void (*x2_put_long)(uaecptr,uae_u32);
static void (*x2_put_word)(uaecptr,uae_u32);
static void (*x2_put_byte)(uaecptr,uae_u32);
static void (*x2_do_cycles)(int);
static void (*x2_do_cycles_pre)(int);
static void (*x2_do_cycles_post)(int, uae_u32);

uae_u32 (*x_prefetch)(int);
uae_u32 (*x_next_iword)();
uae_u32 (*x_next_ilong)();
uae_u32 (*x_get_ilong)(int);
uae_u32 (*x_get_iword)(int);
uae_u32 (*x_get_ibyte)(int);
uae_u32 (*x_get_long)(uaecptr);
uae_u32 (*x_get_word)(uaecptr);
uae_u32 (*x_get_byte)(uaecptr);
void (*x_put_long)(uaecptr,uae_u32);
void (*x_put_word)(uaecptr,uae_u32);
void (*x_put_byte)(uaecptr,uae_u32);

uae_u32 (*x_cp_next_iword)();
uae_u32 (*x_cp_next_ilong)();
uae_u32 (*x_cp_get_long)(uaecptr);
uae_u32 (*x_cp_get_word)(uaecptr);
uae_u32 (*x_cp_get_byte)(uaecptr);
void (*x_cp_put_long)(uaecptr,uae_u32);
void (*x_cp_put_word)(uaecptr,uae_u32);
void (*x_cp_put_byte)(uaecptr,uae_u32);
uae_u32 (REGPARAM3 *x_cp_get_disp_ea_020)(uae_u32 base, int idx) REGPARAM;

void (*x_do_cycles)(int);
void (*x_do_cycles_pre)(int);
void (*x_do_cycles_post)(int, uae_u32);

uae_u32(*x_phys_get_iword)(uaecptr);
uae_u32(*x_phys_get_ilong)(uaecptr);
uae_u32(*x_phys_get_byte)(uaecptr);
uae_u32(*x_phys_get_word)(uaecptr);
uae_u32(*x_phys_get_long)(uaecptr);
void(*x_phys_put_byte)(uaecptr, uae_u32);
void(*x_phys_put_word)(uaecptr, uae_u32);
void(*x_phys_put_long)(uaecptr, uae_u32);

bool(*is_super_access)(bool);

static void set_x_cp_funcs()
{
	x_cp_put_long = x_put_long;
	x_cp_put_word = x_put_word;
	x_cp_put_byte = x_put_byte;
	x_cp_get_long = x_get_long;
	x_cp_get_word = x_get_word;
	x_cp_get_byte = x_get_byte;
	x_cp_next_iword = x_next_iword;
	x_cp_next_ilong = x_next_ilong;
	x_cp_get_disp_ea_020 = x_get_disp_ea_020;

	if (currprefs.mmu_model == 68030) {
		if (currprefs.cpu_compatible) {
			x_cp_put_long = put_long_mmu030c_state;
			x_cp_put_word = put_word_mmu030c_state;
			x_cp_put_byte = put_byte_mmu030c_state;
			x_cp_get_long = get_long_mmu030c_state;
			x_cp_get_word = get_word_mmu030c_state;
			x_cp_get_byte = get_byte_mmu030c_state;
			x_cp_next_iword = next_iword_mmu030c_state;
			x_cp_next_ilong = next_ilong_mmu030c_state;
			x_cp_get_disp_ea_020 = get_disp_ea_020_mmu030c;
		} else {
			x_cp_put_long = put_long_mmu030_state;
			x_cp_put_word = put_word_mmu030_state;
			x_cp_put_byte = put_byte_mmu030_state;
			x_cp_get_long = get_long_mmu030_state;
			x_cp_get_word = get_word_mmu030_state;
			x_cp_get_byte = get_byte_mmu030_state;
			x_cp_next_iword = next_iword_mmu030_state;
			x_cp_next_ilong = next_ilong_mmu030_state;
			x_cp_get_disp_ea_020 = get_disp_ea_020_mmu030;
		}
	}
}

static struct cputracestruct cputrace;

#if CPUTRACE_DEBUG
static void validate_trace (void)
{
	for (int i = 0; i < cputrace.memoryoffset; i++) {
		struct cputracememory *ctm = &cputrace.ctm[i];
		if (ctm->data == 0xdeadf00d) {
			write_log (_T("unfinished write operation %d %08x\n"), i, ctm->addr);
		}
	}
}
#endif

static void debug_trace ()
{
	if (cputrace.writecounter > 10000 || cputrace.readcounter > 10000)
		write_log (_T("cputrace.readcounter=%d cputrace.writecounter=%d\n"), cputrace.readcounter, cputrace.writecounter);
}

STATIC_INLINE void clear_trace ()
{
#if CPUTRACE_DEBUG
	validate_trace ();
#endif
	if (cputrace.memoryoffset == MAX_CPUTRACESIZE)
		return;
	struct cputracememory *ctm = &cputrace.ctm[cputrace.memoryoffset++];
	if (cputrace.memoryoffset == MAX_CPUTRACESIZE) {
		write_log(_T("CPUTRACE overflow, stopping tracing.\n"));
		return;
	}
	ctm->mode = 0;
	cputrace.cyclecounter = 0;
	cputrace.cyclecounter_pre = cputrace.cyclecounter_post = 0;
}
static void set_trace (uaecptr addr, int accessmode, int size)
{
#if CPUTRACE_DEBUG
	validate_trace ();
#endif
	if (cputrace.memoryoffset == MAX_CPUTRACESIZE)
		return;
	struct cputracememory *ctm = &cputrace.ctm[cputrace.memoryoffset++];
	if (cputrace.memoryoffset == MAX_CPUTRACESIZE) {
		write_log(_T("CPUTRACE overflow, stopping tracing.\n"));
		return;
	}
	ctm->addr = addr;
	ctm->data = 0xdeadf00d;
	ctm->mode = accessmode | (size << 4);
	ctm->flags = 1;
	cputrace.cyclecounter_pre = -1;
	if (accessmode == 1)
		cputrace.writecounter++;
	else
		cputrace.readcounter++;
	debug_trace ();
}
static void add_trace (uaecptr addr, uae_u32 val, int accessmode, int size)
{
	if (cputrace.memoryoffset < 1) {
#if CPUTRACE_DEBUG
		write_log (_T("add_trace memoryoffset=%d!\n"), cputrace.memoryoffset);
#endif
		return;
	}
	int mode = accessmode | (size << 4);
	struct cputracememory *ctm = &cputrace.ctm[cputrace.memoryoffset - 1];
	ctm->addr = addr;
	ctm->data = val;
	if (!ctm->mode) {
		ctm->mode = mode;
		if (accessmode == 1)
			cputrace.writecounter++;
		else
			cputrace.readcounter++;
	}
	ctm->flags = 0;
	debug_trace ();
	cputrace.cyclecounter_pre = cputrace.cyclecounter_post = 0;
}


static void check_trace2 ()
{
	if (cputrace.readcounter || cputrace.writecounter ||
		cputrace.cyclecounter || cputrace.cyclecounter_pre || cputrace.cyclecounter_post)
		write_log (_T("CPU tracer invalid state during playback!\n"));
}

static bool check_trace ()
{
	if (!cpu_tracer)
		return true;
	if (!cputrace.readcounter && !cputrace.writecounter && !cputrace.cyclecounter) {
		if (cpu_tracer != -2) {
			write_log (_T("CPU trace: dma_cycle() enabled. %08x %08x NOW=%08x\n"),
				cputrace.cyclecounter_pre, cputrace.cyclecounter_post, get_cycles ());
			cpu_tracer = -2; // dma_cycle() allowed to work now
		}
	}
	if (cputrace.readcounter || cputrace.writecounter ||
		cputrace.cyclecounter || cputrace.cyclecounter_pre || cputrace.cyclecounter_post)
		return false;
	x_prefetch = x2_prefetch;
	x_get_ilong = x2_get_ilong;
	x_get_iword = x2_get_iword;
	x_get_ibyte = x2_get_ibyte;
	x_next_iword = x2_next_iword;
	x_next_ilong = x2_next_ilong;
	x_put_long = x2_put_long;
	x_put_word = x2_put_word;
	x_put_byte = x2_put_byte;
	x_get_long = x2_get_long;
	x_get_word = x2_get_word;
	x_get_byte = x2_get_byte;
	x_do_cycles = x2_do_cycles;
	x_do_cycles_pre = x2_do_cycles_pre;
	x_do_cycles_post = x2_do_cycles_post;
	set_x_cp_funcs();
	write_log(_T("CPU tracer playback complete. STARTCYCLES=%016llx NOWCYCLES=%016llx\n"), cputrace.startcycles, get_cycles());
	cputrace.needendcycles = 1;
	cpu_tracer = 0;
	return true;
}

static bool get_trace(uaecptr addr, int accessmode, int size, uae_u32 *data)
{
	int mode = accessmode | (size << 4);
	for (int i = 0; i < cputrace.memoryoffset; i++) {
		struct cputracememory *ctm = &cputrace.ctm[i];
		if (ctm->addr == addr && ctm->mode == mode) {
			ctm->mode = 0;
			write_log(_T("CPU trace: GET %d: PC=%08x %08x=%08x %d %d %08x/%08x/%08x %d/%d (%08x)\n"),
				i, cputrace.pc, addr, ctm->data, accessmode, size,
				cputrace.cyclecounter, cputrace.cyclecounter_pre, cputrace.cyclecounter_post,
				cputrace.readcounter, cputrace.writecounter, get_cycles ());
			if (accessmode == 1)
				cputrace.writecounter--;
			else
				cputrace.readcounter--;
			if (cputrace.writecounter == 0 && cputrace.readcounter == 0) {
				if (ctm->flags & 4) {
					if (cputrace.cyclecounter_post) {
						int c = cputrace.cyclecounter_post;
						cputrace.cyclecounter_post = 0;
						x_do_cycles(c);
						check_trace();
						*data = ctm->data;
						if ((ctm->flags & 1) && !(ctm->flags & 2)) {
							return true; // argh, need to rerun the memory access..
						}
					}
					// if cyclecounter_pre > 0, it gets handled during memory access rerun
					check_trace();
					*data = ctm->data;
					if ((ctm->flags & 1) && !(ctm->flags & 2)) {
						return true; // argh, need to rerun the memory access..
					}
				} else {
					if (cputrace.cyclecounter_post) {
						int c = cputrace.cyclecounter_post;
						cputrace.cyclecounter_post = 0;
						x_do_cycles(c);
					} else if (cputrace.cyclecounter_pre) {
						check_trace();
						*data = ctm->data;
						return true; // argh, need to rerun the memory access..
					}
				}
			}
			check_trace();
			*data = ctm->data;
			return false;
		}
	}
	if (cputrace.cyclecounter_post) {
		int c = cputrace.cyclecounter_post;
		cputrace.cyclecounter_post = 0;
		check_trace();
		check_trace2();
		x_do_cycles(c);
		return false;
	}
	if ((cputrace.writecounter > 0 || cputrace.readcounter > 0) && cputrace.cyclecounter_pre) {
		gui_message(_T("CPU trace: GET %08x %d %d (%d %d %d) NOT FOUND!\n"),
			addr, accessmode, size, cputrace.readcounter, cputrace.writecounter, cputrace.cyclecounter_pre);
	}
	check_trace();
	*data = 0;
	return true;
}

static uae_u32 cputracefunc_x_prefetch (int o)
{
	uae_u32 pc = m68k_getpc ();
	set_trace (pc + o, 2, 2);
	uae_u32 v = x2_prefetch (o);
	add_trace (pc + o, v, 2, 2);
	return v;
}
static uae_u32 cputracefunc2_x_prefetch (int o)
{
	uae_u32 v;
	if (get_trace (m68k_getpc () + o, 2, 2, &v)) {
		v = x2_prefetch (o);
		check_trace2 ();
	}
	return v;
}

static uae_u32 cputracefunc_x_next_iword ()
{
	uae_u32 pc = m68k_getpc ();
	set_trace (pc, 2, 2);
	uae_u32 v = x2_next_iword ();
	add_trace (pc, v, 2, 2);
	return v;
}
static uae_u32 cputracefunc_x_next_ilong ()
{
	uae_u32 pc = m68k_getpc ();
	set_trace (pc, 2, 4);
	uae_u32 v = x2_next_ilong ();
	add_trace (pc, v, 2, 4);
	return v;
}
static uae_u32 cputracefunc2_x_next_iword ()
{
	uae_u32 v;
	if (get_trace (m68k_getpc (), 2, 2, &v)) {
		v = x2_next_iword ();
		check_trace2 ();
	}
	return v;
}
static uae_u32 cputracefunc2_x_next_ilong ()
{
	uae_u32 v;
	if (get_trace (m68k_getpc (), 2, 4, &v)) {
		v = x2_next_ilong ();
		check_trace2 ();
	}
	return v;
}

static uae_u32 cputracefunc_x_get_ilong (int o)
{
	uae_u32 pc = m68k_getpc ();
	set_trace (pc + o, 2, 4);
	uae_u32 v = x2_get_ilong (o);
	add_trace (pc + o, v, 2, 4);
	return v;
}
static uae_u32 cputracefunc_x_get_iword (int o)
{
	uae_u32 pc = m68k_getpc ();
	set_trace (pc + o, 2, 2);
	uae_u32 v = x2_get_iword (o);
	add_trace (pc + o, v, 2, 2);
	return v;
}
static uae_u32 cputracefunc_x_get_ibyte (int o)
{
	uae_u32 pc = m68k_getpc ();
	set_trace (pc + o, 2, 1);
	uae_u32 v = x2_get_ibyte (o);
	add_trace (pc + o, v, 2, 1);
	return v;
}
static uae_u32 cputracefunc2_x_get_ilong (int o)
{
	uae_u32 v;
	if (get_trace (m68k_getpc () + o, 2, 4, &v)) {
		v = x2_get_ilong (o);
		check_trace2 ();
	}
	return v;
}
static uae_u32 cputracefunc2_x_get_iword (int o)
{
	uae_u32 v;
	if (get_trace (m68k_getpc () + o, 2, 2, &v)) {
		v = x2_get_iword (o);
		check_trace2 ();
	}
	return v;
}
static uae_u32 cputracefunc2_x_get_ibyte (int o)
{
	uae_u32 v;
	if (get_trace (m68k_getpc () + o, 2, 1, &v)) {
		v = x2_get_ibyte (o);
		check_trace2 ();
	}
	return v;
}

static uae_u32 cputracefunc_x_get_long (uaecptr o)
{
	set_trace (o, 0, 4);
	uae_u32 v = x2_get_long (o);
	add_trace (o, v, 0, 4);
	return v;
}
static uae_u32 cputracefunc_x_get_word (uaecptr o)
{
	set_trace (o, 0, 2);
	uae_u32 v = x2_get_word (o);
	add_trace (o, v, 0, 2);
	return v;
}
static uae_u32 cputracefunc_x_get_byte (uaecptr o)
{
	set_trace (o, 0, 1);
	uae_u32 v = x2_get_byte (o);
	add_trace (o, v, 0, 1);
	return v;
}
static uae_u32 cputracefunc2_x_get_long (uaecptr o)
{
	uae_u32 v;
	if (get_trace (o, 0, 4, &v)) {
		v = x2_get_long (o);
		check_trace2 ();
	}
	return v;
}
static uae_u32 cputracefunc2_x_get_word (uaecptr o)
{
	uae_u32 v;
	if (get_trace (o, 0, 2, &v)) {
		v = x2_get_word (o);
		check_trace2 ();
	}
	return v;
}
static uae_u32 cputracefunc2_x_get_byte (uaecptr o)
{
	uae_u32 v;
	if (get_trace (o, 0, 1, &v)) {
		v = x2_get_byte (o);
		check_trace2 ();
	}
	return v;
}

static void cputracefunc_x_put_long (uaecptr o, uae_u32 val)
{
	clear_trace ();
	add_trace (o, val, 1, 4);
	x2_put_long (o, val);
}
static void cputracefunc_x_put_word (uaecptr o, uae_u32 val)
{
	clear_trace ();
	add_trace (o, val, 1, 2);
	x2_put_word (o, val);
}
static void cputracefunc_x_put_byte (uaecptr o, uae_u32 val)
{
	clear_trace ();
	add_trace (o, val, 1, 1);
	x2_put_byte (o, val);
}
static void cputracefunc2_x_put_long (uaecptr o, uae_u32 val)
{
	uae_u32 v;
	if (get_trace (o, 1, 4, &v)) {
		x2_put_long (o, val);
		check_trace2 ();
	}
	if (v != val)
		write_log (_T("cputracefunc2_x_put_long %d <> %d\n"), v, val);
}
static void cputracefunc2_x_put_word (uaecptr o, uae_u32 val)
{
	uae_u32 v;
	if (get_trace (o, 1, 2, &v)) {
		x2_put_word (o, val);
		check_trace2 ();
	}
	if (v != val)
		write_log (_T("cputracefunc2_x_put_word %d <> %d\n"), v, val);
}
static void cputracefunc2_x_put_byte (uaecptr o, uae_u32 val)
{
	uae_u32 v;
	if (get_trace (o, 1, 1, &v)) {
		x2_put_byte (o, val);
		check_trace2 ();
	}
	if (v != val)
		write_log (_T("cputracefunc2_x_put_byte %d <> %d\n"), v, val);
}

static void cputracefunc_x_do_cycles(int cycles)
{
	while (cycles >= CYCLE_UNIT) {
		cputrace.cyclecounter += CYCLE_UNIT;
		cycles -= CYCLE_UNIT;
		x2_do_cycles(CYCLE_UNIT);
	}
	if (cycles > 0) {
		cputrace.cyclecounter += cycles;
		x2_do_cycles(cycles);
	}
}

static void cputracefunc2_x_do_cycles(int cycles)
{
	if (cputrace.cyclecounter > cycles) {
		cputrace.cyclecounter -= cycles;
		return;
	}
	cycles -= cputrace.cyclecounter;
	cputrace.cyclecounter = 0;
	check_trace();
	x_do_cycles = x2_do_cycles;
	if (cycles > 0)
		x_do_cycles(cycles);
}

static void cputracefunc_x_do_cycles_pre(int cycles)
{
	cputrace.cyclecounter_post = 0;
	cputrace.cyclecounter_pre = 0;
	while (cycles >= CYCLE_UNIT) {
		cycles -= CYCLE_UNIT;
		cputrace.cyclecounter_pre += CYCLE_UNIT;
		x2_do_cycles_pre(CYCLE_UNIT);
	}
	if (cycles > 0) {
		x2_do_cycles_pre(cycles);
		cputrace.cyclecounter_pre += cycles;
	}
	cputrace.cyclecounter_pre = 0;
}
// cyclecounter_pre = how many cycles we need to SWALLOW
// -1 = rerun whole access
static void cputracefunc2_x_do_cycles_pre (int cycles)
{
	if (cputrace.cyclecounter_pre == -1) {
		cputrace.cyclecounter_pre = 0;
		check_trace ();
		check_trace2 ();
		x_do_cycles (cycles);
		return;
	}
	if (cputrace.cyclecounter_pre > cycles) {
		cputrace.cyclecounter_pre -= cycles;
		return;
	}
	cycles -= cputrace.cyclecounter_pre;
	cputrace.cyclecounter_pre = 0;
	check_trace ();
	if (cycles > 0)
		x_do_cycles (cycles);
}

static void cputracefunc_x_do_cycles_post(int cycles, uae_u32 v)
{
	if (cputrace.memoryoffset < 1) {
#if CPUTRACE_DEBUG
		write_log (_T("cputracefunc_x_do_cycles_post memoryoffset=%d!\n"), cputrace.memoryoffset);
#endif
		x2_do_cycles_post(cycles, v);
		return;
	}
	struct cputracememory *ctm = &cputrace.ctm[cputrace.memoryoffset - 1];
	ctm->data = v;
	ctm->flags = 0;
	cputrace.cyclecounter_post = cycles;
	cputrace.cyclecounter_pre = 0;
	while (cycles >= CYCLE_UNIT) {
		cycles -= CYCLE_UNIT;
		cputrace.cyclecounter_post -= CYCLE_UNIT;
		x2_do_cycles_post(CYCLE_UNIT, v);
	}
	if (cycles > 0) {
		cputrace.cyclecounter_post -= cycles;
		x2_do_cycles_post(cycles, v);
	}
	cputrace.cyclecounter_post = 0;
}
// cyclecounter_post = how many cycles we need to WAIT
static void cputracefunc2_x_do_cycles_post (int cycles, uae_u32 v)
{
	int c;
	if (cputrace.cyclecounter_post) {
		c = cputrace.cyclecounter_post;
		cputrace.cyclecounter_post = 0;
	} else {
		c = cycles;
	}
	check_trace ();
	if (c > 0)
		x_do_cycles (c);
}

static void do_cycles_post (int cycles, uae_u32 v)
{
	do_cycles(cycles);
}
static void do_cycles_ce_post (int cycles, uae_u32 v)
{
	do_cycles_ce (cycles);
}
static void do_cycles_ce020_post (int cycles, uae_u32 v)
{
	do_cycles_ce020 (cycles);
}

static uae_u8 dcache_check_nommu(uaecptr addr, bool write, uae_u32 size)
{
	return ce_cachable[addr >> 16];
}

static void mem_access_delay_long_write_ce030_cicheck(uaecptr addr, uae_u32 v)
{
	mem_access_delay_long_write_ce020(addr, v);
	mmu030_cache_state = ce_cachable[addr >> 16];
}
static void mem_access_delay_word_write_ce030_cicheck(uaecptr addr, uae_u32 v)
{
	mem_access_delay_word_write_ce020(addr, v);
	mmu030_cache_state = ce_cachable[addr >> 16];
}
static void mem_access_delay_byte_write_ce030_cicheck(uaecptr addr, uae_u32 v)
{
	mem_access_delay_byte_write_ce020(addr, v);
	mmu030_cache_state = ce_cachable[addr >> 16];
}

static void put_long030_cicheck(uaecptr addr, uae_u32 v)
{
	put_long(addr, v);
	mmu030_cache_state = ce_cachable[addr >> 16];
}
static void put_word030_cicheck(uaecptr addr, uae_u32 v)
{
	put_word(addr, v);
	mmu030_cache_state = ce_cachable[addr >> 16];
}
static void put_byte030_cicheck(uaecptr addr, uae_u32 v)
{
	put_byte(addr, v);
	mmu030_cache_state = ce_cachable[addr >> 16];
}

static uae_u32 (*icache_fetch)(uaecptr);
static uae_u16 (*icache_fetch_word)(uaecptr);
static uae_u32 (*dcache_lget)(uaecptr);
static uae_u32 (*dcache_wget)(uaecptr);
static uae_u32 (*dcache_bget)(uaecptr);
static uae_u8 (*dcache_check)(uaecptr, bool, uae_u32);
static void (*dcache_lput)(uaecptr, uae_u32);
static void (*dcache_wput)(uaecptr, uae_u32);
static void (*dcache_bput)(uaecptr, uae_u32);

uae_u32(*read_data_030_bget)(uaecptr);
uae_u32(*read_data_030_wget)(uaecptr);
uae_u32(*read_data_030_lget)(uaecptr);
void(*write_data_030_bput)(uaecptr,uae_u32);
void(*write_data_030_wput)(uaecptr,uae_u32);
void(*write_data_030_lput)(uaecptr,uae_u32);

uae_u32(*read_data_030_fc_bget)(uaecptr, uae_u32);
uae_u32(*read_data_030_fc_wget)(uaecptr, uae_u32);
uae_u32(*read_data_030_fc_lget)(uaecptr, uae_u32);
void(*write_data_030_fc_bput)(uaecptr, uae_u32, uae_u32);
void(*write_data_030_fc_wput)(uaecptr, uae_u32, uae_u32);
void(*write_data_030_fc_lput)(uaecptr, uae_u32, uae_u32);

 
static void set_x_ifetches()
{
	if (m68k_pc_indirect) {
		if (currprefs.cachesize) {
			// indirect via addrbank
			x_get_ilong = get_iilong_jit;
			x_get_iword = get_iiword_jit;
			x_get_ibyte = get_iibyte_jit;
			x_next_iword = next_iiword_jit;
			x_next_ilong = next_iilong_jit;
		} else {
			// indirect via addrbank
			x_get_ilong = get_iilong;
			x_get_iword = get_iiword;
			x_get_ibyte = get_iibyte;
			x_next_iword = next_iiword;
			x_next_ilong = next_iilong;
		}
	} else {
		// direct to memory
		x_get_ilong = get_dilong;
		x_get_iword = get_diword;
		x_get_ibyte = get_dibyte;
		x_next_iword = next_diword;
		x_next_ilong = next_dilong;
	}
}

static bool is_super_access_68000(bool read)
{
	return regs.s;
}
static bool nommu_is_super_access(bool read)
{
	if (!ismoves_nommu) {
		return regs.s;
	} else {
		uae_u32 fc = read ? regs.sfc : regs.dfc;
		return (fc & 4) != 0;
	}
}

// indirect memory access functions
static void set_x_funcs ()
{
	if (currprefs.cpu_model >= 68010) {
		if (currprefs.mmu_model == 68030) {
			is_super_access = mmu030_is_super_access;
		} else if (currprefs.mmu_model >= 68040) {
			is_super_access = mmu_is_super_access;
		} else {
			is_super_access = nommu_is_super_access;
		}
	} else {
		is_super_access = is_super_access_68000;
	}

	if (currprefs.mmu_model) {
		if (currprefs.cpu_model == 68060) {

			x_prefetch = get_iword_mmu060;
			x_get_ilong = get_ilong_mmu060;
			x_get_iword = get_iword_mmu060;
			x_get_ibyte = get_ibyte_mmu060;
			x_next_iword = next_iword_mmu060;
			x_next_ilong = next_ilong_mmu060;
			x_put_long = put_long_mmu060;
			x_put_word = put_word_mmu060;
			x_put_byte = put_byte_mmu060;
			x_get_long = get_long_mmu060;
			x_get_word = get_word_mmu060;
			x_get_byte = get_byte_mmu060;

		} else if (currprefs.cpu_model == 68040) {

			x_prefetch = get_iword_mmu040;
			x_get_ilong = get_ilong_mmu040;
			x_get_iword = get_iword_mmu040;
			x_get_ibyte = get_ibyte_mmu040;
			x_next_iword = next_iword_mmu040;
			x_next_ilong = next_ilong_mmu040;
			x_put_long = put_long_mmu040;
			x_put_word = put_word_mmu040;
			x_put_byte = put_byte_mmu040;
			x_get_long = get_long_mmu040;
			x_get_word = get_word_mmu040;
			x_get_byte = get_byte_mmu040;

		} else {

			if (currprefs.cpu_memory_cycle_exact) {
				x_prefetch = get_iword_mmu030c_state;
				x_get_ilong = get_ilong_mmu030c_state;
				x_get_iword = get_iword_mmu030c_state;
				x_get_ibyte = nullptr;
				x_next_iword = next_iword_mmu030c_state;
				x_next_ilong = next_ilong_mmu030c_state;
			} else if (currprefs.cpu_compatible) {
				x_prefetch = get_iword_mmu030c_state;
				x_get_ilong = get_ilong_mmu030c_state;
				x_get_iword = get_iword_mmu030c_state;
				x_get_ibyte = nullptr;
				x_next_iword = next_iword_mmu030c_state;
				x_next_ilong = next_ilong_mmu030c_state;
			} else {
				x_prefetch = get_iword_mmu030;
				x_get_ilong = get_ilong_mmu030;
				x_get_iword = get_iword_mmu030;
				x_get_ibyte = get_ibyte_mmu030;
				x_next_iword = next_iword_mmu030;
				x_next_ilong = next_ilong_mmu030;
			}
			x_put_long = put_long_mmu030;
			x_put_word = put_word_mmu030;
			x_put_byte = put_byte_mmu030;
			x_get_long = get_long_mmu030;
			x_get_word = get_word_mmu030;
			x_get_byte = get_byte_mmu030;
			if (currprefs.cpu_data_cache) {
				x_put_long = put_long_dc030;
				x_put_word = put_word_dc030;
				x_put_byte = put_byte_dc030;
				x_get_long = get_long_dc030;
				x_get_word = get_word_dc030;
				x_get_byte = get_byte_dc030;
			}
		}
		if (currprefs.cpu_cycle_exact) {
			x_do_cycles = do_cycles_ce020;
			x_do_cycles_pre = do_cycles_ce020;
			x_do_cycles_post = do_cycles_ce020_post;
		} else {
			x_do_cycles = do_cycles;
			x_do_cycles_pre = do_cycles;
			x_do_cycles_post = do_cycles_post;
		}
	} else if (currprefs.cpu_model < 68020) {
		// 68000/010
		if (currprefs.cpu_cycle_exact) {
			x_prefetch = get_word_ce000_prefetch;
			x_get_ilong = nullptr;
			x_get_iword = get_wordi_ce000;
			x_get_ibyte = nullptr;
			x_next_iword = nullptr;
			x_next_ilong = nullptr;
			x_put_long = put_long_ce000;
			x_put_word = put_word_ce000;
			x_put_byte = put_byte_ce000;
			x_get_long = get_long_ce000;
			x_get_word = get_word_ce000;
			x_get_byte = get_byte_ce000;
			x_do_cycles = do_cycles_ce;
			x_do_cycles_pre = do_cycles_ce;
			x_do_cycles_post = do_cycles_ce_post;
		} else if (currprefs.cpu_memory_cycle_exact) {
			// cpu_memory_cycle_exact + cpu_compatible
			x_prefetch = get_word_000_prefetch;
			x_get_ilong = nullptr;
			x_get_iword = get_iiword;
			x_get_ibyte = get_iibyte;
			x_next_iword = nullptr;
			x_next_ilong = nullptr;
			x_put_long = put_long_ce000;
			x_put_word = put_word_ce000;
			x_put_byte = put_byte_ce000;
			x_get_long = get_long_ce000;
			x_get_word = get_word_ce000;
			x_get_byte = get_byte_ce000;
			x_do_cycles = do_cycles;
			x_do_cycles_pre = do_cycles;
			x_do_cycles_post = do_cycles_post;
		} else if (currprefs.cpu_compatible) {
			// cpu_compatible only
			x_prefetch = get_word_000_prefetch;
			x_get_ilong = nullptr;
			x_get_iword = get_iiword;
			x_get_ibyte = get_iibyte;
			x_next_iword = nullptr;
			x_next_ilong = nullptr;
			x_put_long = put_long_compatible;
			x_put_word = put_word_compatible;
			x_put_byte = put_byte_compatible;
			x_get_long = get_long_compatible;
			x_get_word = get_word_compatible;
			x_get_byte = get_byte_compatible;
			x_do_cycles = do_cycles;
			x_do_cycles_pre = do_cycles;
			x_do_cycles_post = do_cycles_post;
		} else {
			x_prefetch = nullptr;
			x_get_ilong = get_dilong;
			x_get_iword = get_diword;
			x_get_ibyte = get_dibyte;
			x_next_iword = next_diword;
			x_next_ilong = next_dilong;
			x_put_long = put_long;
			x_put_word = put_word;
			x_put_byte = put_byte;
			x_get_long = get_long;
			x_get_word = get_word;
			x_get_byte = get_byte;
			x_do_cycles = do_cycles;
			x_do_cycles_pre = do_cycles;
			x_do_cycles_post = do_cycles_post;
		}
	} else if (!currprefs.cpu_cycle_exact) {
		// 68020+ no ce
		if (currprefs.cpu_memory_cycle_exact) {
			// cpu_memory_cycle_exact + cpu_compatible
			if (currprefs.cpu_model == 68020 && !currprefs.cachesize) {
				x_prefetch = get_word_020_prefetch;
				x_get_ilong = get_long_020_prefetch;
				x_get_iword = get_word_020_prefetch;
				x_get_ibyte = nullptr;
				x_next_iword = next_iword_020_prefetch;
				x_next_ilong = next_ilong_020_prefetch;
				x_put_long = put_long_ce020;
				x_put_word = put_word_ce020;
				x_put_byte = put_byte_ce020;
				x_get_long = get_long_ce020;
				x_get_word = get_word_ce020;
				x_get_byte = get_byte_ce020;
				x_do_cycles = do_cycles;
				x_do_cycles_pre = do_cycles;
				x_do_cycles_post = do_cycles_post;
			} else if (currprefs.cpu_model == 68030 && !currprefs.cachesize) {
				x_prefetch = get_word_030_prefetch;
				x_get_ilong = get_long_030_prefetch;
				x_get_iword = get_word_030_prefetch;
				x_get_ibyte = nullptr;
				x_next_iword = next_iword_030_prefetch;
				x_next_ilong = next_ilong_030_prefetch;
				x_put_long = put_long_ce030;
				x_put_word = put_word_ce030;
				x_put_byte = put_byte_ce030;
				x_get_long = get_long_ce030;
				x_get_word = get_word_ce030;
				x_get_byte = get_byte_ce030;
				x_do_cycles = do_cycles;
				x_do_cycles_pre = do_cycles;
				x_do_cycles_post = do_cycles_post;
			} else if (currprefs.cpu_model < 68040) {
				// JIT or 68030+ does not have real prefetch only emulation
				x_prefetch = nullptr;
				set_x_ifetches();
				x_put_long = put_long;
				x_put_word = put_word;
				x_put_byte = put_byte;
				x_get_long = get_long;
				x_get_word = get_word;
				x_get_byte = get_byte;
				x_do_cycles = do_cycles;
				x_do_cycles_pre = do_cycles;
				x_do_cycles_post = do_cycles_post;
			} else {
				// 68040+ (same as below)
				x_prefetch = nullptr;
				x_get_ilong = get_ilong_cache_040;
				x_get_iword = get_iword_cache_040;
				x_get_ibyte = nullptr;
				x_next_iword = next_iword_cache040;
				x_next_ilong = next_ilong_cache040;
				if (currprefs.cpu_data_cache) {
					x_put_long = put_long_cache_040;
					x_put_word = put_word_cache_040;
					x_put_byte = put_byte_cache_040;
					x_get_long = get_long_cache_040;
					x_get_word = get_word_cache_040;
					x_get_byte = get_byte_cache_040;
				} else {
					x_get_byte = mem_access_delay_byte_read_c040;
					x_get_word = mem_access_delay_word_read_c040;
					x_get_long = mem_access_delay_long_read_c040;
					x_put_byte = mem_access_delay_byte_write_c040;
					x_put_word = mem_access_delay_word_write_c040;
					x_put_long = mem_access_delay_long_write_c040;
				}
				x_do_cycles = do_cycles;
				x_do_cycles_pre = do_cycles;
				x_do_cycles_post = do_cycles_post;
			}
		} else if (currprefs.cpu_compatible) {
			// cpu_compatible only
			if (currprefs.cpu_model == 68020 && !currprefs.cachesize) {
				x_prefetch = get_word_020_prefetch;
				x_get_ilong = get_long_020_prefetch;
				x_get_iword = get_word_020_prefetch;
				x_get_ibyte = nullptr;
				x_next_iword = next_iword_020_prefetch;
				x_next_ilong = next_ilong_020_prefetch;
				x_put_long = put_long_compatible;
				x_put_word = put_word_compatible;
				x_put_byte = put_byte_compatible;
				x_get_long = get_long_compatible;
				x_get_word = get_word_compatible;
				x_get_byte = get_byte_compatible;
				x_do_cycles = do_cycles;
				x_do_cycles_pre = do_cycles;
				x_do_cycles_post = do_cycles_post;
			} else if (currprefs.cpu_model == 68030 && !currprefs.cachesize) {
				x_prefetch = get_word_030_prefetch;
				x_get_ilong = get_long_030_prefetch;
				x_get_iword = get_word_030_prefetch;
				x_get_ibyte = nullptr;
				x_next_iword = next_iword_030_prefetch;
				x_next_ilong = next_ilong_030_prefetch;
				x_put_long = put_long_030;
				x_put_word = put_word_030;
				x_put_byte = put_byte_030;
				x_get_long = get_long_030;
				x_get_word = get_word_030;
				x_get_byte = get_byte_030;
				x_do_cycles = do_cycles;
				x_do_cycles_pre = do_cycles;
				x_do_cycles_post = do_cycles_post;
			} else if (currprefs.cpu_model < 68040) {
				// JIT or 68030+ does not have real prefetch only emulation
				x_prefetch = nullptr;
				set_x_ifetches();
				x_put_long = put_long;
				x_put_word = put_word;
				x_put_byte = put_byte;
				x_get_long = get_long;
				x_get_word = get_word;
				x_get_byte = get_byte;
				x_do_cycles = do_cycles;
				x_do_cycles_pre = do_cycles;
				x_do_cycles_post = do_cycles_post;
			} else {
				x_prefetch = nullptr;
				x_get_ilong = get_ilong_cache_040;
				x_get_iword = get_iword_cache_040;
				x_get_ibyte = nullptr;
				x_next_iword = next_iword_cache040;
				x_next_ilong = next_ilong_cache040;
				if (currprefs.cpu_data_cache) {
					x_put_long = put_long_cache_040;
					x_put_word = put_word_cache_040;
					x_put_byte = put_byte_cache_040;
					x_get_long = get_long_cache_040;
					x_get_word = get_word_cache_040;
					x_get_byte = get_byte_cache_040;
				} else {
					x_put_long = put_long;
					x_put_word = put_word;
					x_put_byte = put_byte;
					x_get_long = get_long;
					x_get_word = get_word;
					x_get_byte = get_byte;
				}
				x_do_cycles = do_cycles;
				x_do_cycles_pre = do_cycles;
				x_do_cycles_post = do_cycles_post;
			}
		} else {
			x_prefetch = nullptr;
			set_x_ifetches();
			if (currprefs.cachesize) {
				x_put_long = put_long_jit;
				x_put_word = put_word_jit;
				x_put_byte = put_byte_jit;
				x_get_long = get_long_jit;
				x_get_word = get_word_jit;
				x_get_byte = get_byte_jit;
			} else {
				x_put_long = put_long;
				x_put_word = put_word;
				x_put_byte = put_byte;
				x_get_long = get_long;
				x_get_word = get_word;
				x_get_byte = get_byte;
			}
			x_do_cycles = do_cycles;
			x_do_cycles_pre = do_cycles;
			x_do_cycles_post = do_cycles_post;
		}
		// 68020+ cycle exact
	} else if (currprefs.cpu_model == 68020) {
		x_prefetch = get_word_ce020_prefetch;
		x_get_ilong = get_long_ce020_prefetch;
		x_get_iword = get_word_ce020_prefetch;
		x_get_ibyte = nullptr;
		x_next_iword = next_iword_020ce;
		x_next_ilong = next_ilong_020ce;
		x_put_long = put_long_ce020;
		x_put_word = put_word_ce020;
		x_put_byte = put_byte_ce020;
		x_get_long = get_long_ce020;
		x_get_word = get_word_ce020;
		x_get_byte = get_byte_ce020;
		x_do_cycles = do_cycles_ce020;
		x_do_cycles_pre = do_cycles_ce020;
		x_do_cycles_post = do_cycles_ce020_post;
	} else if (currprefs.cpu_model == 68030) {
		x_prefetch = get_word_ce030_prefetch;
		x_get_ilong = get_long_ce030_prefetch;
		x_get_iword = get_word_ce030_prefetch;
		x_get_ibyte = nullptr;
		x_next_iword = next_iword_030ce;
		x_next_ilong = next_ilong_030ce;
		if (currprefs.cpu_data_cache) {
			x_put_long = put_long_dc030;
			x_put_word = put_word_dc030;
			x_put_byte = put_byte_dc030;
			x_get_long = get_long_dc030;
			x_get_word = get_word_dc030;
			x_get_byte = get_byte_dc030;
		} else {
			x_put_long = put_long_ce030;
			x_put_word = put_word_ce030;
			x_put_byte = put_byte_ce030;
			x_get_long = get_long_ce030;
			x_get_word = get_word_ce030;
			x_get_byte = get_byte_ce030;
		}
		x_do_cycles = do_cycles_ce020;
		x_do_cycles_pre = do_cycles_ce020;
		x_do_cycles_post = do_cycles_ce020_post;
	} else if (currprefs.cpu_model >= 68040) {
		x_prefetch = nullptr;
		x_get_ilong = get_ilong_cache_040;
		x_get_iword = get_iword_cache_040;
		x_get_ibyte = nullptr;
		x_next_iword = next_iword_cache040;
		x_next_ilong = next_ilong_cache040;
		if (currprefs.cpu_data_cache) {
			x_put_long = put_long_cache_040;
			x_put_word = put_word_cache_040;
			x_put_byte = put_byte_cache_040;
			x_get_long = get_long_cache_040;
			x_get_word = get_word_cache_040;
			x_get_byte = get_byte_cache_040;
		} else {
			x_get_byte = mem_access_delay_byte_read_c040;
			x_get_word = mem_access_delay_word_read_c040;
			x_get_long = mem_access_delay_long_read_c040;
			x_put_byte = mem_access_delay_byte_write_c040;
			x_put_word = mem_access_delay_word_write_c040;
			x_put_long = mem_access_delay_long_write_c040;
		}
		x_do_cycles = do_cycles_ce020;
		x_do_cycles_pre = do_cycles_ce020;
		x_do_cycles_post = do_cycles_ce020_post;
	}
	x2_prefetch = x_prefetch;
	x2_get_ilong = x_get_ilong;
	x2_get_iword = x_get_iword;
	x2_get_ibyte = x_get_ibyte;
	x2_next_iword = x_next_iword;
	x2_next_ilong = x_next_ilong;
	x2_put_long = x_put_long;
	x2_put_word = x_put_word;
	x2_put_byte = x_put_byte;
	x2_get_long = x_get_long;
	x2_get_word = x_get_word;
	x2_get_byte = x_get_byte;
	x2_do_cycles = x_do_cycles;
	x2_do_cycles_pre = x_do_cycles_pre;
	x2_do_cycles_post = x_do_cycles_post;

	if (cpu_tracer > 0) {
		x_prefetch = cputracefunc_x_prefetch;
		x_get_ilong = cputracefunc_x_get_ilong;
		x_get_iword = cputracefunc_x_get_iword;
		x_get_ibyte = cputracefunc_x_get_ibyte;
		x_next_iword = cputracefunc_x_next_iword;
		x_next_ilong = cputracefunc_x_next_ilong;
		x_put_long = cputracefunc_x_put_long;
		x_put_word = cputracefunc_x_put_word;
		x_put_byte = cputracefunc_x_put_byte;
		x_get_long = cputracefunc_x_get_long;
		x_get_word = cputracefunc_x_get_word;
		x_get_byte = cputracefunc_x_get_byte;
		x_do_cycles = cputracefunc_x_do_cycles;
		x_do_cycles_pre = cputracefunc_x_do_cycles_pre;
		x_do_cycles_post = cputracefunc_x_do_cycles_post;
	} else if (cpu_tracer < 0) {
		if (!check_trace ()) {
			x_prefetch = cputracefunc2_x_prefetch;
			x_get_ilong = cputracefunc2_x_get_ilong;
			x_get_iword = cputracefunc2_x_get_iword;
			x_get_ibyte = cputracefunc2_x_get_ibyte;
			x_next_iword = cputracefunc2_x_next_iword;
			x_next_ilong = cputracefunc2_x_next_ilong;
			x_put_long = cputracefunc2_x_put_long;
			x_put_word = cputracefunc2_x_put_word;
			x_put_byte = cputracefunc2_x_put_byte;
			x_get_long = cputracefunc2_x_get_long;
			x_get_word = cputracefunc2_x_get_word;
			x_get_byte = cputracefunc2_x_get_byte;
			x_do_cycles = cputracefunc2_x_do_cycles;
			x_do_cycles_pre = cputracefunc2_x_do_cycles_pre;
			x_do_cycles_post = cputracefunc2_x_do_cycles_post;
		}
	}

	set_x_cp_funcs();
	mmu_set_funcs();
	mmu030_set_funcs();

	dcache_lput = put_long;
	dcache_wput = put_word;
	dcache_bput = put_byte;
	dcache_lget = get_long;
	dcache_wget = get_word;
	dcache_bget = get_byte;
	dcache_check = dcache_check_nommu;

	icache_fetch = get_longi;
	icache_fetch_word = nullptr;
	if (currprefs.cpu_cycle_exact) {
		icache_fetch = mem_access_delay_longi_read_ce020;
	}
	if (currprefs.cpu_model >= 68040 && currprefs.cpu_memory_cycle_exact) {
		icache_fetch = mem_access_delay_longi_read_c040;
		dcache_bget = mem_access_delay_byte_read_c040;
		dcache_wget = mem_access_delay_word_read_c040;
		dcache_lget = mem_access_delay_long_read_c040;
		dcache_bput = mem_access_delay_byte_write_c040;
		dcache_wput = mem_access_delay_word_write_c040;
		dcache_lput = mem_access_delay_long_write_c040;
	}

	if (currprefs.cpu_model == 68030) {

		if (currprefs.cpu_data_cache) {
			read_data_030_bget = read_dcache030_mmu_bget;
			read_data_030_wget = read_dcache030_mmu_wget;
			read_data_030_lget = read_dcache030_mmu_lget;
			write_data_030_bput = write_dcache030_mmu_bput;
			write_data_030_wput = write_dcache030_mmu_wput;
			write_data_030_lput = write_dcache030_mmu_lput;

			read_data_030_fc_bget = read_dcache030_bget;
			read_data_030_fc_wget = read_dcache030_wget;
			read_data_030_fc_lget = read_dcache030_lget;
			write_data_030_fc_bput = write_dcache030_bput;
			write_data_030_fc_wput = write_dcache030_wput;
			write_data_030_fc_lput = write_dcache030_lput;
		} else {
			read_data_030_bget = dcache_bget;
			read_data_030_wget = dcache_wget;
			read_data_030_lget = dcache_lget;
			write_data_030_bput = dcache_bput;
			write_data_030_wput = dcache_wput;
			write_data_030_lput = dcache_lput;

			read_data_030_fc_bget = mmu030_get_fc_byte;
			read_data_030_fc_wget = mmu030_get_fc_word;
			read_data_030_fc_lget = mmu030_get_fc_long;
			write_data_030_fc_bput = mmu030_put_fc_byte;
			write_data_030_fc_wput = mmu030_put_fc_word;
			write_data_030_fc_lput = mmu030_put_fc_long;
		}

		if (currprefs.mmu_model) {
			if (currprefs.cpu_compatible) {
				icache_fetch = uae_mmu030_get_ilong_fc;
				icache_fetch_word = uae_mmu030_get_iword_fc;
			} else {
				icache_fetch = uae_mmu030_get_ilong;
				icache_fetch_word = uae_mmu030_get_iword_fc;
			}
			dcache_lput = uae_mmu030_put_long_fc;
			dcache_wput = uae_mmu030_put_word_fc;
			dcache_bput = uae_mmu030_put_byte_fc;
			dcache_lget = uae_mmu030_get_long_fc;
			dcache_wget = uae_mmu030_get_word_fc;
			dcache_bget = uae_mmu030_get_byte_fc;
			if (currprefs.cpu_data_cache) {
				read_data_030_bget = read_dcache030_mmu_bget;
				read_data_030_wget = read_dcache030_mmu_wget;
				read_data_030_lget = read_dcache030_mmu_lget;
				write_data_030_bput = write_dcache030_mmu_bput;
				write_data_030_wput = write_dcache030_mmu_wput;
				write_data_030_lput = write_dcache030_mmu_lput;
				dcache_check = uae_mmu030_check_fc;
			} else {
				read_data_030_bget = uae_mmu030_get_byte;
				read_data_030_wget = uae_mmu030_get_word;
				read_data_030_lget = uae_mmu030_get_long;
				write_data_030_bput = uae_mmu030_put_byte;
				write_data_030_wput = uae_mmu030_put_word;
				write_data_030_lput = uae_mmu030_put_long;
			}
		} else if (currprefs.cpu_memory_cycle_exact) {
			icache_fetch = mem_access_delay_longi_read_ce020;
			dcache_lget = mem_access_delay_long_read_ce020;
			dcache_wget = mem_access_delay_word_read_ce020;
			dcache_bget = mem_access_delay_byte_read_ce020;
			if (currprefs.cpu_data_cache) {
				dcache_lput = mem_access_delay_long_write_ce030_cicheck;
				dcache_wput = mem_access_delay_word_write_ce030_cicheck;
				dcache_bput = mem_access_delay_byte_write_ce030_cicheck;
			} else {
				dcache_lput = mem_access_delay_long_write_ce020;
				dcache_wput = mem_access_delay_word_write_ce020;
				dcache_bput = mem_access_delay_byte_write_ce020;
			}
		} else if (currprefs.cpu_data_cache) {
			dcache_lput = put_long030_cicheck;
			dcache_wput = put_word030_cicheck;
			dcache_bput = put_byte030_cicheck;
		}
	}
}

bool can_cpu_tracer ()
{
	return (currprefs.cpu_model == 68000 || currprefs.cpu_model == 68020) && currprefs.cpu_memory_cycle_exact;
}

bool is_cpu_tracer ()
{
	return cpu_tracer > 0;
}
bool set_cpu_tracer (bool state)
{
	if (cpu_tracer < 0)
		return false;
	int old = cpu_tracer;
	if (input_record)
		state = true;
	cpu_tracer = 0;
	if (state && can_cpu_tracer ()) {
		cpu_tracer = 1;
		set_x_funcs ();
		if (old != cpu_tracer)
			write_log (_T("CPU tracer enabled\n"));
	}
	if (old > 0 && state == false) {
		set_x_funcs ();
		write_log (_T("CPU tracer disabled\n"));
	}
	return is_cpu_tracer ();
}

static void invalidate_cpu_data_caches()
{
	if (currprefs.cpu_model == 68030) {
		for (auto & i : dcaches030) {
			i.valid[0] = false;
			i.valid[1] = false;
			i.valid[2] = false;
			i.valid[3] = false;
		}
	} else if (currprefs.cpu_model >= 68040) {
		dcachelinecnt = 0;
		for (int i = 0; i < CACHESETS060; i++) {
			for (int j = 0; j < CACHELINES040; j++) {
				dcaches040[i].valid[j] = false;
			}
		}
	}
}

#ifdef DEBUGGER
static void flush_cpu_cache_debug(uaecptr addr, int size)
{
	debugmem_flushcache(0, -1);
	debug_smc_clear(0, -1);
}
#endif

void flush_cpu_caches(bool force)
{
	bool doflush = currprefs.cpu_compatible || currprefs.cpu_memory_cycle_exact;

	if (currprefs.cpu_model == 68020) {
		if ((regs.cacr & 0x08) || force) { // clear instr cache
			for (auto & i : caches020)
				i.valid = false;
			regs.cacr &= ~0x08;
#ifdef DEBUGGER
			flush_cpu_cache_debug(0, -1);
#endif
		}
		if (regs.cacr & 0x04) { // clear entry in instr cache
			caches020[(regs.caar >> 2) & (CACHELINES020 - 1)].valid = false;
			regs.cacr &= ~0x04;
#ifdef DEBUGGER
			flush_cpu_cache_debug(regs.caar, CACHELINES020);
#endif
		}
	} else if (currprefs.cpu_model == 68030) {
		if ((regs.cacr & 0x08) || force) { // clear instr cache
			if (doflush) {
				for (auto & i : icaches030) {
					i.valid[0] = false;
					i.valid[1] = false;
					i.valid[2] = false;
					i.valid[3] = false;
				}
			}
			regs.cacr &= ~0x08;
#ifdef DEBUGGER
			flush_cpu_cache_debug(0, -1);
#endif
		}
		if (regs.cacr & 0x04) { // clear entry in instr cache
			icaches030[(regs.caar >> 4) & (CACHELINES030 - 1)].valid[(regs.caar >> 2) & 3] = false;
			regs.cacr &= ~0x04;
#ifdef DEBUGGER
			flush_cpu_cache_debug(regs.caar, CACHELINES030);
#endif
		}
		if ((regs.cacr & 0x800) || force) { // clear data cache
			if (doflush) {
				for (auto & i : dcaches030) {
					i.valid[0] = false;
					i.valid[1] = false;
					i.valid[2] = false;
					i.valid[3] = false;
				}
			}
			regs.cacr &= ~0x800;
		}
		if (regs.cacr & 0x400) { // clear entry in data cache
			dcaches030[(regs.caar >> 4) & (CACHELINES030 - 1)].valid[(regs.caar >> 2) & 3] = false;
			regs.cacr &= ~0x400;
		}
	} else if (currprefs.cpu_model >= 68040) {
		if (doflush && force) {
			mmu_flush_cache();
			icachelinecnt = 0;
			icachehalfline = 0;
			for (int i = 0; i < CACHESETS060; i++) {
				for (int j = 0; j < CACHELINES040; j++) {
					icaches040[i].valid[j] = false;
				}
			}
#ifdef DEBUGGER
			flush_cpu_cache_debug(0, -1);
#endif
		}
	}
}

#if VALIDATE_68040_DATACACHE > 1
static void validate_dcache040(void)
{
	for (int i = 0; i < cachedsets04060; i++) {
		struct cache040 *c = &dcaches040[i];
		for (int j = 0; j < CACHELINES040; j++) {
			if (c->valid[j]) {
				uae_u32 addr = (c->tag[j] & cachedtag04060mask) | (i << 4);
				if (addr < 0x200000 || (addr >= 0xd80000 && addr < 0xe00000) || (addr >= 0xe80000 && addr < 0xf00000) || (addr >= 0xa00000 && addr < 0xc00000)) {
					write_log(_T("Chip RAM or IO address cached! %08x\n"), addr);
				}
				for (int k = 0; k < 4; k++) {
					if (!c->dirty[j][k]) {
						uae_u32 v = get_long(addr + k * 4);
						if (v != c->data[j][k]) {
							write_log(_T("Address %08x data cache mismatch %08x != %08x\n"), addr, v, c->data[j][k]);
						}
					}
				}
			}
		}
	}
}
#endif

static void dcache040_push_line(int index, int line, bool writethrough, bool invalidate)
{
	struct cache040 *c = &dcaches040[index];
#if VALIDATE_68040_DATACACHE
	if (!c->valid[line]) {
		write_log("dcache040_push_line pushing invalid line!\n");
	}
#endif
	if (c->gdirty[line]) {
		uae_u32 addr = (c->tag[line] & cachedtag04060mask) | (index << 4);
		for (int i = 0; i < 4; i++) {
			if (c->dirty[line][i] || (!writethrough && currprefs.cpu_model == 68060)) {
				dcache_lput(addr + i * 4, c->data[line][i]);
				c->dirty[line][i] = false;
			}
		}
		c->gdirty[line] = false;
	}
	if (invalidate)
		c->valid[line] = false;

#if VALIDATE_68040_DATACACHE > 1
	validate_dcache040();
#endif
}

static void flush_cpu_caches_040_2(int cache, int scope, uaecptr addr, bool push, bool pushinv)
{
#if VALIDATE_68040_DATACACHE
	write_log(_T("push %d %d %d %08x %d %d\n"), cache, scope, areg, addr, push, pushinv);
#endif

	if ((cache & 1) && !currprefs.cpu_data_cache) {
		cache &= ~1;
	}
	if (cache & 2) {
		regs.prefetch020addr = 0xffffffff;
	}
	for (int k = 0; k < 2; k++) {
		if (cache & (1 << k)) {
			if (scope == 3) {
				// all
				if (!k) {
					// data
					for (int i = 0; i < cachedsets04060; i++) {
						struct cache040 *c = &dcaches040[i];
						for (int j = 0; j < CACHELINES040; j++) {
							if (c->valid[j]) {
								if (push) {
									dcache040_push_line(i, j, false, pushinv);
								} else {
									c->valid[j] = false;
								}
							}
						}
					}
					dcachelinecnt = 0;
				} else {
					// instruction
					flush_cpu_caches(true);
				}
			} else {
				uae_u32 pagesize;
				if (scope == 2) {
					// page
					pagesize = mmu_pagesize_8k ? 8192 : 4096;
				} else {
					// line
					pagesize = 16;
				}
				addr &= ~(pagesize - 1);
				for (uae_u32 j = 0; j < pagesize; j += 16, addr += 16) {
					int index;
					uae_u32 tag;
					uae_u32 tagmask;
					struct cache040 *c;
					if (k) {
						tagmask = cacheitag04060mask;
						index = (addr >> 4) & cacheisets04060mask;
						c = &icaches040[index];
#ifdef DEBUGGER
						flush_cpu_cache_debug(addr, 16);
#endif
					} else {
						tagmask = cachedtag04060mask;
						index = (addr >> 4) & cachedsets04060mask;
						c = &dcaches040[index];
					}
					tag = addr & tagmask;
					for (int i = 0; i < CACHELINES040; i++) {
						if (c->valid[i] && c->tag[i] == tag) {
							if (push) {
								dcache040_push_line(index, i, false, pushinv);
							} else {
								c->valid[i] = false;
							}
						}
					}
				}
			}
		}
	}
}

void flush_cpu_caches_040(uae_u16 opcode)
{
	// 0 (1) = data, 1 (2) = instruction
	int cache = (opcode >> 6) & 3;
	int scope = (opcode >> 3) & 3;
	int areg = opcode & 7;
	uaecptr addr = m68k_areg(regs, areg);
	bool push = (opcode & 0x20) != 0;
	bool pushinv = (regs.cacr & 0x01000000) == 0; // 68060 DPI

	flush_cpu_caches_040_2(cache, scope, addr, push, pushinv);
	mmu_flush_cache();
}

void cpu_invalidate_cache(uaecptr addr, int size)
{
	if (!currprefs.cpu_data_cache)
		return;
	if (currprefs.cpu_model == 68030) {
		uaecptr end = addr + size;
		addr &= ~3;
		while (addr < end) {
			dcaches030[(addr >> 4) & (CACHELINES030 - 1)].valid[(addr >> 2) & 3] = false;
			addr += 4;
		}
	} else if (currprefs.cpu_model >= 68040) {
		uaecptr end = addr + size;
		while (addr < end) {
			flush_cpu_caches_040_2(0, 1, addr, true, true);
			addr += 16;
		}
	}
}


void set_cpu_caches (bool flush)
{
	regs.prefetch020addr = 0xffffffff;
	regs.cacheholdingaddr020 = 0xffffffff;
	cache_default_data &= ~CACHE_DISABLE_ALLOCATE;

	// 68060 FIC 1/2 instruction cache
	cacheisets04060 = currprefs.cpu_model == 68060 && !(regs.cacr & 0x00002000) ? CACHESETS060 : CACHESETS040;
	cacheisets04060mask = cacheisets04060 - 1;
	cacheitag04060mask = ~((cacheisets04060 << 4) - 1);
	// 68060 FOC 1/2 data cache
	cachedsets04060 = currprefs.cpu_model == 68060 && !(regs.cacr & 0x08000000) ? CACHESETS060 : CACHESETS040;
	cachedsets04060mask = cachedsets04060 - 1;
	cachedtag04060mask = ~((cachedsets04060 << 4) - 1);
	cache_lastline = 0;

#ifdef JIT
	if (currprefs.cachesize) {
		if (currprefs.cpu_model < 68040) {
			set_cache_state (regs.cacr & 1);
			if (regs.cacr & 0x08) {
				flush_icache (3);
			}
		} else {
			set_cache_state ((regs.cacr & 0x8000) ? 1 : 0);
		}
	}
#endif
	flush_cpu_caches(flush);
}

STATIC_INLINE void count_instr (uae_u32 opcode)
{
}

static uae_u32 opcode_swap(uae_u16 opcode)
{
	if (!need_opcode_swap)
		return opcode;
	return do_byteswap_16(opcode);
}

uae_u32 REGPARAM2 op_illg_1(uae_u32 opcode)
{
	opcode = opcode_swap(opcode);
	op_illg(opcode);
	return 4;
}
void REGPARAM2 op_illg_1_noret(uae_u32 opcode)
{
	op_illg_1(opcode);
}
uae_u32 REGPARAM2 op_unimpl_1 (uae_u32 opcode)
{
	opcode = opcode_swap(opcode);
	if ((opcode & 0xf000) == 0xf000 || currprefs.cpu_model < 68060)
		op_illg(opcode);
	else
		op_unimpl(opcode);
	return 4;
}
void REGPARAM2 op_unimpl_1_noret(uae_u32 opcode)
{
	op_unimpl_1(opcode);
}


// generic+direct, generic+direct+jit, generic+indirect, more compatible, cycle-exact, mmu, mmu+more compatible, mmu+mc+ce
static const struct cputbl *cputbls[6][8] =
{
	// 68000
	{ op_smalltbl_5, op_smalltbl_45, op_smalltbl_55, op_smalltbl_12, op_smalltbl_14, nullptr, nullptr, nullptr},
	// 68010
	{ op_smalltbl_4, op_smalltbl_44, op_smalltbl_54, op_smalltbl_11, op_smalltbl_13, nullptr, nullptr, nullptr},
	// 68020
	{ op_smalltbl_3, op_smalltbl_43, op_smalltbl_53, op_smalltbl_20, op_smalltbl_21, nullptr, nullptr, nullptr},
	// 68030
	{ op_smalltbl_2, op_smalltbl_42, op_smalltbl_52, op_smalltbl_22, op_smalltbl_23, op_smalltbl_32, op_smalltbl_34, op_smalltbl_35 },
	// 68040
	{ op_smalltbl_1, op_smalltbl_41, op_smalltbl_51, op_smalltbl_25, op_smalltbl_25, op_smalltbl_31, op_smalltbl_31, op_smalltbl_31 },
	// 68060
	{ op_smalltbl_0, op_smalltbl_40, op_smalltbl_50, op_smalltbl_24, op_smalltbl_24, op_smalltbl_33, op_smalltbl_33, op_smalltbl_33 }
};

#ifdef JIT

const struct cputbl *uaegetjitcputbl(void)
{
	int lvl = (currprefs.cpu_model - 68000) / 10;
	if (lvl > 4)
		lvl--;
	int index = currprefs.comptrustbyte ? 0 : 1;
	return cputbls[lvl][index];
}

const struct cputbl *getjitcputbl(int cpulvl, int direct)
{
	return cputbls[cpulvl][1 + direct];
}

#endif

static void build_cpufunctbl ()
{
	int i, opcnt;
	uae_u32 opcode;
	const struct cputbl *tbl = nullptr;
	int lvl, mode, jit;

	jit = 0;
	if (!currprefs.cachesize) {
		if (currprefs.mmu_model) {
			if (currprefs.cpu_cycle_exact)
				mode = 7;
			else if (currprefs.cpu_compatible)
				mode = 6;
			else
				mode = 5;
		} else if (currprefs.cpu_cycle_exact) {
			mode = 4;
		} else if (currprefs.cpu_compatible) {
			mode = 3;
		} else {
			mode = 0;
		}
		m68k_pc_indirect = mode != 0 ? 1 : 0;
	} else {
		mode = 1;
		m68k_pc_indirect = 0;
		jit = 1;
		if (currprefs.comptrustbyte) {
			mode = 2;
			m68k_pc_indirect = -1;
		}
	}
	lvl = (currprefs.cpu_model - 68000) / 10;
	if (lvl == 6)
		lvl = 5;
	tbl = cputbls[lvl][mode];

	if (tbl == nullptr) {
		write_log (_T("no CPU emulation cores available CPU=%d!"), currprefs.cpu_model);
		abort ();
	}

	for (opcode = 0; opcode < 65536; opcode++) {
		cpufunctbl[opcode] = op_illg_1;
		cpufunctbl_noret[opcode] = op_illg_1_noret;
	}
	for (i = 0; tbl[i].handler_ff != nullptr || tbl[i].handler_ff_noret != nullptr; i++) {
		opcode = tbl[i].opcode;
		cpufunctbl[opcode] = tbl[i].handler_ff;
		cpufunctbl_noret[opcode] = tbl[i].handler_ff_noret;
		cpudatatbl[opcode].length = tbl[i].length;
		cpudatatbl[opcode].disp020[0] = tbl[i].disp020[0];
		cpudatatbl[opcode].disp020[1] = tbl[i].disp020[1];
		cpudatatbl[opcode].branch = tbl[i].branch;
	}

	/* hack fpu to 68000/68010 mode */
	if (currprefs.fpu_model && currprefs.cpu_model < 68020) {
		tbl = op_smalltbl_3;
		for (i = 0; tbl[i].handler_ff != nullptr || tbl[i].handler_ff_noret != nullptr; i++) {
			if ((tbl[i].opcode & 0xfe00) == 0xf200) {
				cpufunctbl[tbl[i].opcode] = tbl[i].handler_ff;
				cpufunctbl_noret[tbl[i].opcode] = tbl[i].handler_ff_noret;
				cpudatatbl[tbl[i].opcode].length = tbl[i].length;
				cpudatatbl[tbl[i].opcode].disp020[0] = tbl[i].disp020[0];
				cpudatatbl[tbl[i].opcode].disp020[1] = tbl[i].disp020[1];
				cpudatatbl[tbl[i].opcode].branch = tbl[i].branch;
			}
		}
	}

	opcnt = 0;
	for (opcode = 0; opcode < 65536; opcode++) {
		struct instr *table = &table68k[opcode];

		if (table->mnemo == i_ILLG)
			continue;

		/* unimplemented opcode? */
		if (table->unimpclev > 0 && lvl >= table->unimpclev) {
			if (currprefs.cpu_model == 68060) {
				// remove unimplemented integer instructions
				// unimpclev == 5: not implemented in 68060,
				// generates unimplemented instruction exception.
				if (currprefs.int_no_unimplemented && table->unimpclev == 5) {
					cpufunctbl[opcode] = op_unimpl_1;
					cpufunctbl_noret[opcode] = op_unimpl_1_noret;
					continue;
				}
				// remove unimplemented instruction that were removed in previous models,
				// generates normal illegal instruction exception.
				// unimplclev < 5: instruction was removed in 68040 or previous model.
				// clev=4: implemented in 68040 or later. unimpclev=5: not in 68060
				if (table->unimpclev < 5 || (table->clev == 4 && table->unimpclev == 5)) {
					cpufunctbl[opcode] = op_illg_1;
					cpufunctbl_noret[opcode] = op_illg_1_noret;
					continue;
				}
			} else {
				cpufunctbl[opcode] = op_illg_1;
				cpufunctbl_noret[opcode] = op_illg_1_noret;
				continue;
			}
		}

		if (currprefs.fpu_model && currprefs.cpu_model < 68020) {
			/* more hack fpu to 68000/68010 mode */
			if (table->clev > lvl && (opcode & 0xfe00) != 0xf200)
				continue;
		} else if (table->clev > lvl) {
			continue;
		}

		if (table->handler != -1) {
			int idx = table->handler;
			if (cpufunctbl[idx] == op_illg_1 || cpufunctbl_noret[idx] == op_illg_1_noret)
				abort ();
			cpufunctbl[opcode] = cpufunctbl[idx];
			cpufunctbl_noret[opcode] = cpufunctbl_noret[idx];
			memcpy(&cpudatatbl[opcode], &cpudatatbl[idx], sizeof(struct cputbl_data));
			opcnt++;
		}

		if (opcode_loop_mode(opcode)) {
			loop_mode_table[opcode] = cpufunctbl[opcode];
		}

	}

	need_opcode_swap = false;
#ifdef HAVE_GET_WORD_UNSWAPPED
	if (jit) {
		cpuop_func **tmp = xmalloc(cpuop_func*, 65536);
		memcpy(tmp, cpufunctbl, sizeof(cpuop_func*) * 65536);
		for (int i = 0; i < 65536; i++) {
			int offset = do_byteswap_16(i);
			cpufunctbl[offset] = tmp[i];
		}
		xfree(tmp);
		need_opcode_swap = 1;
	}
#endif

	write_log (_T("Building CPU, %d opcodes (%d %d %d)\n"),
		opcnt, lvl,
		currprefs.cpu_cycle_exact ? -2 : currprefs.cpu_memory_cycle_exact ? -1 : currprefs.cpu_compatible ? 1 : 0, currprefs.address_space_24);
#ifdef JIT
	write_log(_T("JIT: &countdown =  %p\n"), &countdown);
	write_log(_T("JIT: &build_comp = %p\n"), &build_comp);
	build_comp ();
#endif

	write_log(_T("CPU=%d, FPU=%d%s, MMU=%d, JIT%s=%d."),
		currprefs.cpu_model,
		currprefs.fpu_model, currprefs.fpu_model ? (currprefs.fpu_mode > 0 ? _T(" (softfloat)") : (currprefs.fpu_mode < 0 ? _T(" (host 80b)") : _T(" (host 64b)"))) : _T(""),
		currprefs.mmu_model,
		currprefs.cachesize ? (currprefs.compfpu ? _T("=CPU/FPU") : _T("=CPU")) : _T(""),
		currprefs.cachesize);

	regs.address_space_mask = 0xffffffff;
	if (currprefs.cpu_compatible) {
		if (currprefs.address_space_24 && currprefs.cpu_model >= 68040)
			currprefs.address_space_24 = false;
	}
	m68k_interrupt_delay = false;
	m68k_accurate_ipl = false;
	if (currprefs.cpu_cycle_exact) {
		if (tbl == op_smalltbl_14 || tbl == op_smalltbl_13 || tbl == op_smalltbl_21 || tbl == op_smalltbl_23)
			m68k_interrupt_delay = true;
		if (tbl == op_smalltbl_14 || tbl == op_smalltbl_13)
			m68k_accurate_ipl = true;
	} else if (currprefs.cpu_compatible) {
		if (currprefs.cpu_model <= 68010 && currprefs.m68k_speed == 0) {
			m68k_interrupt_delay = true;
		}
	}

	if (currprefs.cpu_cycle_exact) {
		if (currprefs.cpu_model == 68000)
			write_log(_T(" prefetch and cycle-exact"));
		else
			write_log(_T(" ~cycle-exact"));
	} else if (currprefs.cpu_memory_cycle_exact) {
			write_log(_T(" ~memory-cycle-exact"));
	} else if (currprefs.cpu_compatible) {
		if (currprefs.cpu_model <= 68020) {
			write_log(_T(" prefetch"));
		} else {
			write_log(_T(" fake prefetch"));
		}
	}
	if (currprefs.m68k_speed < 0)
		write_log(_T(" fast"));
	if (currprefs.int_no_unimplemented && currprefs.cpu_model == 68060) {
		write_log(_T(" no unimplemented integer instructions"));
	}
	if (currprefs.fpu_no_unimplemented && currprefs.fpu_model) {
		write_log(_T(" no unimplemented floating point instructions"));
	}
	if (currprefs.address_space_24) {
		regs.address_space_mask = 0x00ffffff;
		write_log(_T(" 24-bit"));
	}
	write_log(_T("\n"));

	cpuipldelay2 = 2 * cpucycleunit;
	cpuipldelay4 = 4 * cpucycleunit;

	set_cpu_caches (true);
	target_cpu_speed();
}

#define CYCLES_DIV 8192
static uae_u32 cycles_mult;

static void update_68k_cycles ()
{
	cycles_mult = 0;

	if (currprefs.m68k_speed == 0) { // approximate
		cycles_mult = CYCLES_DIV;
		if (currprefs.cpu_model >= 68040) {
			if (currprefs.mmu_model) {
				cycles_mult = CYCLES_DIV / 24;
			} else {
				cycles_mult = CYCLES_DIV / 16;
			}
		} else if (currprefs.cpu_model >= 68020) {
			if (currprefs.mmu_model) {
				cycles_mult = CYCLES_DIV / 12;
			} else {
				cycles_mult = CYCLES_DIV / 8;
			}
		}

		if (!currprefs.cpu_cycle_exact) {
			if (currprefs.m68k_speed_throttle < 0) {
				cycles_mult = static_cast<uae_u32>((cycles_mult * 1000) / (1000 + currprefs.m68k_speed_throttle));
			} else if (currprefs.m68k_speed_throttle > 0) {
				cycles_mult = static_cast<uae_u32>((cycles_mult * 1000) / (1000 + currprefs.m68k_speed_throttle));
			}
		}
	} else if (currprefs.m68k_speed < 0) {
		cycles_mult = CYCLES_DIV / 21;
	} else {
		if (currprefs.m68k_speed >= 0 && !currprefs.cpu_cycle_exact && !currprefs.cpu_compatible) {
			if (currprefs.m68k_speed_throttle < 0) {
				cycles_mult = static_cast<uae_u32>((CYCLES_DIV * 1000 / (1000 + currprefs.m68k_speed_throttle)));
			} else if (currprefs.m68k_speed_throttle > 0) {
				cycles_mult = static_cast<uae_u32>((CYCLES_DIV * 1000 / (1000 + currprefs.m68k_speed_throttle)));
			}
		}
	}
	cycles_mult &= ~0x7f;
	if (cycles_mult < 0x80) {
		cycles_mult = 0x80;
	}

	currprefs.cpu_clock_multiplier = changed_prefs.cpu_clock_multiplier;
	currprefs.cpu_frequency = changed_prefs.cpu_frequency;

	baseclock = (currprefs.ntscmode ? CHIPSET_CLOCK_NTSC : CHIPSET_CLOCK_PAL) * 8;
	cpucycleunit = CYCLE_UNIT / 2;
	if (currprefs.cpu_clock_multiplier) {
		if (currprefs.cpu_clock_multiplier >= 256) {
			cpucycleunit = CYCLE_UNIT / (currprefs.cpu_clock_multiplier >> 8);
		} else {
			cpucycleunit = CYCLE_UNIT * currprefs.cpu_clock_multiplier;
		}
		if (currprefs.cpu_model >= 68040) {
			cpucycleunit /= 2;
		}
	} else if (currprefs.cpu_frequency) {
		cpucycleunit = CYCLE_UNIT * baseclock / currprefs.cpu_frequency;
	} else if (currprefs.cpu_memory_cycle_exact && currprefs.cpu_clock_multiplier == 0) {
		if (currprefs.cpu_model >= 68040) {
			cpucycleunit = CYCLE_UNIT / 16;
		} if (currprefs.cpu_model == 68030) {
			cpucycleunit = CYCLE_UNIT / 8;
		} else if (currprefs.cpu_model == 68020) {
			cpucycleunit = CYCLE_UNIT / 4;
		} else {
			cpucycleunit = CYCLE_UNIT / 2;
		}
	}
	if (cpucycleunit < 1)
		cpucycleunit = 1;

	write_log (_T("CPU cycleunit: %d (%.3f)\n"), cpucycleunit, static_cast<float>(cpucycleunit) / CYCLE_UNIT);
	set_config_changed ();
}

static void prefs_changed_cpu ()
{
	fixup_cpu (&changed_prefs);
	check_prefs_changed_comp(false);
	currprefs.cpu_model = changed_prefs.cpu_model;
	currprefs.fpu_model = changed_prefs.fpu_model;
	currprefs.fpu_revision = changed_prefs.fpu_revision;
	if (currprefs.mmu_model != changed_prefs.mmu_model) {
		int oldmmu = currprefs.mmu_model;
		currprefs.mmu_model = changed_prefs.mmu_model;
		if (currprefs.mmu_model >= 68040) {
			uae_u32 tcr = regs.tcr;
			mmu_reset();
			mmu_set_tc(tcr);
			mmu_set_super(regs.s != 0);
			mmu_tt_modified();
		} else if (currprefs.mmu_model == 68030) {
			mmu030_reset(-1);
			mmu030_flush_atc_all();
			tc_030 = fake_tc_030;
			tt0_030 = fake_tt0_030;
			tt1_030 = fake_tt1_030;
			srp_030 = fake_srp_030;
			crp_030 = fake_crp_030;
			mmu030_decode_tc(tc_030, false);
		} else if (oldmmu == 68030) {
			fake_tc_030 = tc_030;
			fake_tt0_030 = tt0_030;
			fake_tt1_030 = tt1_030;
			fake_srp_030 = srp_030;
			fake_crp_030 = crp_030;
		}
	}
	currprefs.mmu_ec = changed_prefs.mmu_ec;
	if (currprefs.cpu_compatible != changed_prefs.cpu_compatible) {
		currprefs.cpu_compatible = changed_prefs.cpu_compatible;
		flush_cpu_caches(true);
		invalidate_cpu_data_caches();
	}
	if (currprefs.cpu_data_cache != changed_prefs.cpu_data_cache) {
		currprefs.cpu_data_cache = changed_prefs.cpu_data_cache;
		invalidate_cpu_data_caches();
	}
	currprefs.address_space_24 = changed_prefs.address_space_24;
	currprefs.cpu_cycle_exact = changed_prefs.cpu_cycle_exact;
	currprefs.cpu_memory_cycle_exact = changed_prefs.cpu_memory_cycle_exact;
	currprefs.int_no_unimplemented = changed_prefs.int_no_unimplemented;
	currprefs.fpu_no_unimplemented = changed_prefs.fpu_no_unimplemented;
	currprefs.blitter_cycle_exact = changed_prefs.blitter_cycle_exact;
	mman_set_barriers(false);
}

static int check_prefs_changed_cpu2()
{
	int changed = 0;

#ifdef JIT
	changed = check_prefs_changed_comp(true) ? 1 : 0;
#endif
	if (changed
		|| currprefs.cpu_model != changed_prefs.cpu_model
		|| currprefs.fpu_model != changed_prefs.fpu_model
		|| currprefs.fpu_revision != changed_prefs.fpu_revision
		|| currprefs.mmu_model != changed_prefs.mmu_model
		|| currprefs.mmu_ec != changed_prefs.mmu_ec
		|| currprefs.cpu_data_cache != changed_prefs.cpu_data_cache
		|| currprefs.int_no_unimplemented != changed_prefs.int_no_unimplemented
		|| currprefs.fpu_no_unimplemented != changed_prefs.fpu_no_unimplemented
		|| currprefs.cpu_compatible != changed_prefs.cpu_compatible
		|| currprefs.cpu_cycle_exact != changed_prefs.cpu_cycle_exact
		|| currprefs.cpu_memory_cycle_exact != changed_prefs.cpu_memory_cycle_exact
		|| currprefs.fpu_mode != changed_prefs.fpu_mode) {
			cpu_prefs_changed_flag |= 1;
	}
	if (changed
		|| currprefs.m68k_speed != changed_prefs.m68k_speed
		|| currprefs.m68k_speed_throttle != changed_prefs.m68k_speed_throttle
		|| currprefs.cpu_clock_multiplier != changed_prefs.cpu_clock_multiplier
		|| currprefs.reset_delay != changed_prefs.reset_delay
		|| currprefs.cpu_frequency != changed_prefs.cpu_frequency) {
			cpu_prefs_changed_flag |= 2;
	}
	return cpu_prefs_changed_flag;
}

void check_prefs_changed_cpu()
{
	if (!config_changed)
		return;

	currprefs.cpu_idle = changed_prefs.cpu_idle;
	currprefs.ppc_cpu_idle = changed_prefs.ppc_cpu_idle;
	currprefs.reset_delay = changed_prefs.reset_delay;
	currprefs.cpuboard_settings = changed_prefs.cpuboard_settings;

	if (check_prefs_changed_cpu2()) {
		set_special(SPCFLAG_MODE_CHANGE);
		reset_frame_rate_hack();
	}
}

void init_m68k ()
{
	prefs_changed_cpu ();
	update_68k_cycles ();

	for (int i = 0 ; i < 256 ; i++) {
		int j;
		for (j = 0 ; j < 8 ; j++) {
			if (i & (1 << j)) break;
		}
		movem_index1[i] = j;
		movem_index2[i] = 7 - j;
		movem_next[i] = i & (~(1 << j));
	}

#if COUNT_INSTRS
	{
		FILE *f = fopen (icountfilename (), "r");
		memset (instrcount, 0, sizeof instrcount);
		if (f) {
			uae_u32 opcode, count, total;
			TCHAR name[20];
			write_log (_T("Reading instruction count file...\n"));
			fscanf (f, "Total: %lu\n", &total);
			while (fscanf (f, "%x: %lu %s\n", &opcode, &count, name) == 3) {
				instrcount[opcode] = count;
			}
			fclose (f);
		}
	}
#endif

	init_table68k();

	write_log (_T("%d CPU functions\n"), nr_cpuop_funcs);
}

struct regstruct regs, mmu_backup_regs;
struct flag_struct regflags;
static int m68kpc_offset;

STATIC_INLINE int in_rom (uaecptr pc)
{
	return (munge24 (pc) & 0xFFF80000) == 0xF80000;
}

STATIC_INLINE int in_rtarea (uaecptr pc)
{
	return (munge24 (pc) & 0xFFFF0000) == rtarea_base && (uae_boot_rom_type || currprefs.uaeboard > 0);
}

STATIC_INLINE void wait_memory_cycles ()
{
	if (regs.memory_waitstate_cycles) {
		x_do_cycles(regs.memory_waitstate_cycles);
		regs.memory_waitstate_cycles = 0;
	}
	if (regs.ce020extracycles >= 16) {
		regs.ce020extracycles = 0;
		x_do_cycles(2 * cpucycleunit);
	}
}

STATIC_INLINE int adjust_cycles (int cycles)
{
	int mc = regs.memory_waitstate_cycles;
	regs.memory_waitstate_cycles = 0;
	cycles *= cycles_mult;
	cycles /= CYCLES_DIV;
	return cycles + mc;
}

void m68k_cancel_idle()
{
	cpu_last_stop_vpos = -1;
}

static void m68k_set_stop(int stoptype)
{
	if (regs.stopped)
		return;
	regs.stopped = stoptype;
	if (cpu_last_stop_vpos >= 0) {
		cpu_last_stop_vpos = vpos;
	}
}

static void m68k_unset_stop()
{
	regs.stopped = 0;
	if (cpu_last_stop_vpos >= 0) {
		cpu_stopped_lines += vpos - cpu_last_stop_vpos;
		cpu_last_stop_vpos = vpos;
	}
}

static void activate_trace()
{
	unset_special (SPCFLAG_TRACE);
	set_special (SPCFLAG_DOTRACE);
}

// make sure interrupt is checked immediately after current instruction
void checkint()
{
	doint();
	if (!m68k_accurate_ipl && !currprefs.cachesize && !(regs.spcflags & SPCFLAG_INT) && (regs.spcflags & SPCFLAG_DOINT))
		set_special(SPCFLAG_INT);
}

void REGPARAM2 MakeSR()
{
	regs.sr = ((regs.t1 << 15) | (regs.t0 << 14)
		| (regs.s << 13) | (regs.m << 12) | (regs.intmask << 8)
		| (GET_XFLG() << 4) | (GET_NFLG() << 3)
		| (GET_ZFLG() << 2) | (GET_VFLG() << 1)
		|  GET_CFLG());
}

static void SetSR(uae_u16 sr)
{
	regs.sr &= 0xff00;
	regs.sr |= sr;

	SET_XFLG((regs.sr >> 4) & 1);
	SET_NFLG((regs.sr >> 3) & 1);
	SET_ZFLG((regs.sr >> 2) & 1);
	SET_VFLG((regs.sr >> 1) & 1);
	SET_CFLG(regs.sr & 1);
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
		regs.s  == ((regs.sr >> 13) & 1) &&
		regs.m  == ((regs.sr >> 12) & 1) &&
		regs.intmask == ((regs.sr >> 8) & 7))
		return;

	regs.t1 = (regs.sr >> 15) & 1;
	regs.t0 = (regs.sr >> 14) & 1;
	regs.s  = (regs.sr >> 13) & 1;
	regs.m  = (regs.sr >> 12) & 1;

	if (regs.intmask != ((regs.sr >> 8) & 7)) {
		int newimask = (regs.sr >> 8) & 7;
		if (m68k_accurate_ipl) {
			// STOP intmask change enabling already active interrupt: delay it by 1 STOP round
			if (t0trace < 0 && regs.ipl[0] <= regs.intmask && regs.ipl[0] > newimask && regs.ipl[0] < 7) {
				regs.ipl[0] = 0;
			}
		} else {
			if (regs.ipl_pin <= regs.intmask && regs.ipl_pin > newimask) {
				if (!currprefs.cachesize) {
					set_special(SPCFLAG_INT);
				} else {
					set_special(SPCFLAG_DOINT);
				}
			}
		}
		regs.intmask = newimask;
	}

	if (currprefs.cpu_model >= 68020) {
		/* 68060 does not have MSP but does have M-bit.. */
		if (currprefs.cpu_model >= 68060)
			regs.msp = regs.isp;
		if (olds != regs.s) {
			if (olds) {
				if (oldm)
					regs.msp = m68k_areg (regs, 7);
				else
					regs.isp = m68k_areg (regs, 7);
				m68k_areg (regs, 7) = regs.usp;
			} else {
				regs.usp = m68k_areg (regs, 7);
				m68k_areg (regs, 7) = regs.m ? regs.msp : regs.isp;
			}
		} else if (olds && oldm != regs.m) {
			if (oldm) {
				regs.msp = m68k_areg (regs, 7);
				m68k_areg (regs, 7) = regs.isp;
			} else {
				regs.isp = m68k_areg (regs, 7);
				m68k_areg (regs, 7) = regs.msp;
			}
		}
		if (currprefs.cpu_model >= 68060)
			regs.t0 = 0;
	} else {
		regs.t0 = regs.m = 0;
		if (olds != regs.s) {
			if (olds) {
				regs.isp = m68k_areg (regs, 7);
				m68k_areg (regs, 7) = regs.usp;
			} else {
				regs.usp = m68k_areg (regs, 7);
				m68k_areg (regs, 7) = regs.isp;
			}
		}
	}
	if (currprefs.mmu_model)
		mmu_set_super (regs.s != 0);

#ifdef JIT
	// if JIT enabled and T1, T0 or M changes: end compile.
	if (currprefs.cachesize && (oldt0 != regs.t0 || oldt1 != regs.t1 || oldm != regs.m)) {
		set_special(SPCFLAG_END_COMPILE);
	}
#endif

	if (regs.t1 || regs.t0) {
		set_special (SPCFLAG_TRACE);
	} else {
		/* Keep SPCFLAG_DOTRACE, we still want a trace exception for
		SR-modifying instructions (including STOP).  */
		unset_special (SPCFLAG_TRACE);
	}
	// Stop SR-modification does not generate T0
	// If this SR modification set Tx bit, no trace until next instruction.
	if (!regs.stopped && ((oldt0 && t0trace && currprefs.cpu_model >= 68020) || oldt1)) {
		// Always trace if Tx bits were already set, even if this SR modification cleared them.
		activate_trace();
	}
}

void REGPARAM2 MakeFromSR_T0()
{
	MakeFromSR_x(1);
}
void REGPARAM2 MakeFromSR()
{
	MakeFromSR_x(0);
}
void REGPARAM2 MakeFromSR_STOP()
{
	MakeFromSR_x(-1);
}

static bool internalexception(const int nr)
{
	return nr == 5 || nr == 6 || nr == 7 || (nr >= 32 && nr <= 47);
}

static void exception_check_trace (int nr)
{
	unset_special (SPCFLAG_TRACE | SPCFLAG_DOTRACE);
	if (regs.t1) {
		/* trace stays pending if exception is div by zero, chk,
		* trapv or trap #x. Except if 68040 or 68060.
		*/
		if (currprefs.cpu_model < 68040 && internalexception(nr)) {
			set_special(SPCFLAG_DOTRACE);
		}
		// 68010 and RTE format error: trace is not cleared
		if (nr == 14 && currprefs.cpu_model == 68010) {
			set_special(SPCFLAG_DOTRACE);
		}
	}
	regs.t1 = regs.t0 = 0;
}

static void exception_debug (int nr)
{
#ifdef DEBUGGER
	if (!exception_debugging)
		return;
	console_out_f (_T("Exception %d, PC=%08X\n"), nr, M68K_GETPC);
#endif
}

#ifdef CPUEMU_13

/* cycle-exact exception handler, 68000 only */

/*

68000 Address/Bus Error:

- [memory access causing bus/address error]
- 8 idle cycles (+4 if bus error)
- write PC low word
- write SR
- write PC high word
- write instruction word
- write fault address low word
- write status code
- write fault address high word
- read exception address high word
- read exception address low word
- prefetch
- 2 idle cycles
- prefetch

68010 Address/Bus Error:

- [memory access causing bus/address error]
- 8 idle cycles (+4 if bus error)
- write word 28
- write word 26
- write word 27
- write word 25
- write word 23
- write word 24
- write word 22
- write word 21
- write word 20
- write word 19
- write word 18
- write word 17
- write word 16
- write word 15
- write word 13
- write word 14
- write instruction buffer
- (skipped)
- write data input buffer
- (skipped)
- write data output buffer
- (skipped)
- write fault address low word
- write fault address high word
- write special status word
- write PC low word
- write SR
- write PC high word
- write frame format
- read exception address high word
- read exception address low word
- prefetch
- 2 idle cycles
- prefetch


Division by Zero:

- 4 idle cycles (EA + 4 cycles in cpuemu)
- write PC low word
- write SR
- write PC high word
- read exception address high word
- read exception address low word
- prefetch
- 2 idle cycles
- prefetch

Traps:

- 4 idle cycles
- write PC low word
- write SR
- write PC high word
- read exception address high word
- read exception address low word
- prefetch
- 2 idle cycles
- prefetch

TrapV:

(- normal prefetch done by TRAPV)
- write PC low word
- write SR
- write PC high word
- read exception address high word
- read exception address low word
- prefetch
- 2 idle cycles
- prefetch

CHK:

- 4 idle cycles (EA + 4/6 cycles in cpuemu)
- write PC low word
- write SR
- write PC high word
- read exception address high word
- read exception address low word
- prefetch
- 2 idle cycles
- prefetch

Illegal Instruction:
Privilege violation:
Trace:
Line A:
Line F:

- 4 idle cycles
- write PC low word
- write SR
- write PC high word
- read exception address high word
- read exception address low word
- prefetch
- 2 idle cycles
- prefetch

Interrupt:

- 6 idle cycles
- write PC low word
- read exception number byte from (0xfffff1 | (interrupt number << 1))
- 4 idle cycles
- write SR
- write PC high word
- read exception address high word
- read exception address low word
- prefetch
- 2 idle cycles
- prefetch

68010:

...
- write SR
- write PC high word
- write frame format
- read exception address high word
...

*/

static int iack_cycle(int nr)
{
	int vector = nr;

	if (true) {
		// non-autovectored
		if (currprefs.cpu_model >= 68020) {
			return vector;
		}
		// this is basically normal memory access and takes 4 cycles (without wait states).
		vector = x_get_byte(0x00fffff1 | ((nr - 24) << 1));
		x_do_cycles(4 * cpucycleunit);
	} else {
		// autovectored
	}
	return vector;
}

static void Exception_ce000 (int nr)
{
	uae_u32 currpc = m68k_getpc (), newpc;
	int sv = regs.s;
	int start, interrupt;
	int vector_nr = nr;
	int frame_id = 0;

	start = 6;
	interrupt = nr >= 24 && nr < 24 + 8;
	if (!interrupt) {
		start = 4;
		if (nr == 7) { // TRAPV
			start = 0;
		} else if (nr == 3) {
			if (currprefs.cpu_model == 68000)
				start = 8;
			else
				start = 4;
		} else if (nr == 2) {
			if (currprefs.cpu_model == 68000)
				start = 12;
			else
				start = 8;
		}
	}

	if (start)
		x_do_cycles (start * cpucycleunit);

	exception_debug (nr);
	MakeSR ();

	bool g1 = generates_group1_exception(regs.ir);
	if (!regs.s) {
		regs.usp = m68k_areg (regs, 7);
		m68k_areg (regs, 7) = regs.isp;
		regs.s = 1;
	}
	if (nr == 2 || nr == 3) { /* 2=bus error, 3=address error */
		if ((m68k_areg(regs, 7) & 1) || exception_in_exception < 0) {
			cpu_halt (CPU_HALT_DOUBLE_FAULT);
			return;
		}
#ifdef DEBUGGER
		write_log(_T("Exception %d (%08x %x) at %x -> %x!\n"),
			nr, last_op_for_exception_3, last_addr_for_exception_3, currpc, get_long_debug(4 * nr));
#endif
		if (currprefs.cpu_model == 68000) {
			// 68000 bus/address error
			uae_u16 mode = (sv ? 4 : 0) | last_fc_for_exception_3;
			mode |= last_writeaccess_for_exception_3 ? 0 : 16;
			mode |= last_notinstruction_for_exception_3 ? 8 : 0;
			// undocumented bits contain opcode
			mode |= last_op_for_exception_3 & ~31;
			m68k_areg(regs, 7) -= 7 * 2;
			exception_in_exception = -1;
			x_put_word(m68k_areg(regs, 7) + 12, last_addr_for_exception_3);
			x_put_word(m68k_areg(regs, 7) + 8, regs.sr);
			x_put_word(m68k_areg(regs, 7) + 10, last_addr_for_exception_3 >> 16);
			x_put_word(m68k_areg(regs, 7) + 6, last_op_for_exception_3);
			x_put_word(m68k_areg(regs, 7) + 4, last_fault_for_exception_3);
			x_put_word(m68k_areg(regs, 7) + 0, mode);
			x_put_word(m68k_areg(regs, 7) + 2, last_fault_for_exception_3 >> 16);
			goto kludge_me_do;
		} else {
			// 68010 bus/address error (partially implemented only)
			uae_u16 in = regs.read_buffer;
			uae_u16 out = regs.write_buffer;
			uae_u16 ssw = (sv ? 4 : 0) | last_fc_for_exception_3;
			ssw |= last_di_for_exception_3 > 0 ? 0x0000 : (last_di_for_exception_3 < 0 ? (0x2000 | 0x1000) : 0x2000);
			ssw |= (!last_writeaccess_for_exception_3 && last_di_for_exception_3) ? 0x1000 : 0x000; // DF
			ssw |= (last_op_for_exception_3 & 0x10000) ? 0x0400 : 0x0000; // HB
			ssw |= last_size_for_exception_3 == 0 ? 0x0200 : 0x0000; // BY
			ssw |= last_writeaccess_for_exception_3 ? 0 : 0x0100; // RW
			if (last_op_for_exception_3 & 0x20000)
				ssw &= 0x00ff;
			m68k_areg(regs, 7) -= (29 - 4) * 2;
			exception_in_exception = -1;
			frame_id = 8;
			for (int i = 0; i < 15; i++) {
				x_put_word(m68k_areg(regs, 7) + 20 + i * 2, ((i + 1) << 8) | ((i + 2) << 0));
			}
			x_put_word(m68k_areg(regs, 7) + 18, 0); // version
			x_put_word(m68k_areg(regs, 7) + 16, regs.irc); // instruction input buffer
			x_put_word(m68k_areg(regs, 7) + 12, in); // data input buffer
			x_put_word(m68k_areg(regs, 7) + 8, out); // data output buffer
			x_put_word(m68k_areg(regs, 7) + 4, last_fault_for_exception_3); // fault addr
			x_put_word(m68k_areg(regs, 7) + 2, last_fault_for_exception_3 >> 16);
			x_put_word(m68k_areg(regs, 7) + 0, ssw); // ssw
		}
	}
	if (currprefs.cpu_model == 68010) {
		// 68010 creates only format 0 and 8 stack frames
		m68k_areg (regs, 7) -= 4 * 2;
		if (m68k_areg(regs, 7) & 1) {
			exception3_notinstruction(regs.ir, m68k_areg(regs, 7) + 4);
			return;
		}
		exception_in_exception = 1;
		x_put_word (m68k_areg (regs, 7) + 4, currpc); // write low address
		if (interrupt) {
			vector_nr = iack_cycle(nr);
		}
		x_put_word (m68k_areg (regs, 7) + 0, regs.sr); // write SR
		x_put_word (m68k_areg (regs, 7) + 2, currpc >> 16); // write high address
		x_put_word (m68k_areg (regs, 7) + 6, (frame_id << 12) | (vector_nr * 4));
	} else {
		m68k_areg (regs, 7) -= 3 * 2;
		if (m68k_areg(regs, 7) & 1) {
			exception3_notinstruction(regs.ir, m68k_areg(regs, 7) + 4);
			return;
		}
		exception_in_exception = 1;
		x_put_word (m68k_areg (regs, 7) + 4, currpc); // write low address
		if (interrupt) {
			vector_nr = iack_cycle(nr);
		}
		x_put_word (m68k_areg (regs, 7) + 0, regs.sr); // write SR
		x_put_word (m68k_areg (regs, 7) + 2, currpc >> 16); // write high address
	}
kludge_me_do:
	if ((regs.vbr & 1) && currprefs.cpu_model <= 68010) {
		cpu_halt(CPU_HALT_DOUBLE_FAULT);
		return;
	}
	if (interrupt)
		regs.intmask = nr - 24;
	newpc = x_get_word (regs.vbr + 4 * vector_nr) << 16; // read high address
	newpc |= x_get_word (regs.vbr + 4 * vector_nr + 2); // read low address
	exception_in_exception = 0;
	if (newpc & 1) {
		if (nr == 2 || nr == 3) {
			cpu_halt(CPU_HALT_DOUBLE_FAULT);
			return;
		}
		if (currprefs.cpu_model == 68000) {
			// if exception vector is odd:
			// opcode is last opcode executed, address is address of exception vector
			// pc is last prefetch address
			regs.t1 = 0;
			MakeSR();
			m68k_setpc(regs.vbr + 4 * vector_nr);
			if (interrupt) {
				regs.ir = nr;
				exception3_read_access(regs.ir | 0x20000 | 0x10000, newpc, sz_word, 2);
			} else {
				exception3_read_access(regs.ir | 0x40000 | 0x20000 | (g1 ? 0x10000 : 0), newpc, sz_word, 2);
			}
		} else if (currprefs.cpu_model == 68010) {
			// offset, not vbr + offset
			regs.t1 = 0;
			MakeSR();
			regs.write_buffer = 4 * vector_nr;
			regs.read_buffer = newpc;
			regs.irc = regs.read_buffer;
			exception3_read_access(regs.opcode, newpc, sz_word, 2);
		} else {
			exception_check_trace(nr);
			exception3_notinstruction(regs.ir, newpc);
		}
		return;
	}
	m68k_setpc(newpc);
#ifdef DEBUGGER
	branch_stack_push(currpc, currpc);
#endif
	regs.ir = x_get_word(m68k_getpc()); // prefetch 1
	if (hardware_bus_error) {
		if (nr == 2 || nr == 3) {
			cpu_halt(CPU_HALT_DOUBLE_FAULT);
			return;
		}
		exception2_fetch(regs.irc, 0, 0);
		return;
	}
	regs.ird = regs.ir;
	x_do_cycles(2 * cpucycleunit);
	if (m68k_accurate_ipl) {
		ipl_fetch_next();
	}
	regs.irc = x_get_word(m68k_getpc() + 2); // prefetch 2
	if (hardware_bus_error) {
		if (nr == 2 || nr == 3) {
			cpu_halt(CPU_HALT_DOUBLE_FAULT);
			return;
		}
		exception2_fetch(regs.ir, 0, 2);
		return;
	}
#ifdef JIT
	if (currprefs.cachesize) {
		set_special(SPCFLAG_END_COMPILE);
	}
#endif
	exception_check_trace(nr);
}
#endif

// 68030 MMU
static void Exception_mmu030 (int nr, uaecptr oldpc)
{
	uae_u32 currpc = m68k_getpc (), newpc;
	int interrupt, vector_nr = nr;

	interrupt = nr >= 24 && nr < 24 + 8;
	if (interrupt)
		vector_nr = iack_cycle(nr);

	exception_debug (nr);
	MakeSR ();

	if (!regs.s) {
		regs.usp = m68k_areg (regs, 7);
		m68k_areg(regs, 7) = regs.m ? regs.msp : regs.isp;
		regs.s = 1;
		mmu_set_super (true);
	}

	newpc = x_get_long (regs.vbr + 4 * vector_nr);

	if (regs.m && interrupt) { /* M + Interrupt */
		Exception_build_stack_frame(oldpc, currpc, regs.mmu_ssw, vector_nr, 0x0);
		MakeSR ();
		regs.m = 0;
		regs.msp = m68k_areg (regs, 7);
		m68k_areg (regs, 7) = regs.isp;
		Exception_build_stack_frame(oldpc, currpc, regs.mmu_ssw, vector_nr, 0x1);
	} else if (nr == 2) {
		if (mmu030_state[1] & MMU030_STATEFLAG1_LASTWRITE) {
			Exception_build_stack_frame(oldpc, currpc, regs.mmu_ssw, vector_nr, 0xA);
		} else {
			Exception_build_stack_frame(oldpc, currpc, regs.mmu_ssw, vector_nr, 0xB);
		}
	} else if (nr == 3) {
		regs.mmu_fault_addr = last_fault_for_exception_3;
		mmu030_state[0] = mmu030_state[1] = 0;
		mmu030_data_buffer_out = 0;
		Exception_build_stack_frame(last_fault_for_exception_3, currpc, MMU030_SSW_RW | MMU030_SSW_SIZE_W | (regs.s ? 6 : 2), vector_nr,  0xB);
	} else {
		Exception_build_stack_frame_common(oldpc, currpc, regs.mmu_ssw, nr, vector_nr);
	}

	if (newpc & 1) {
		if (nr == 2 || nr == 3)
			cpu_halt (CPU_HALT_DOUBLE_FAULT);
		else
			exception3_read_special(regs.ir, newpc, 1, 1);
		return;
	}
	if (interrupt)
		regs.intmask = nr - 24;
	m68k_setpci (newpc);
	fill_prefetch ();
	exception_check_trace (nr);
}

// 68040/060 MMU
static void Exception_mmu (int nr, uaecptr oldpc)
{
	uae_u32 currpc = m68k_getpc (), newpc;
	int interrupt;
	int vector_nr = nr;

	interrupt = nr >= 24 && nr < 24 + 8;
	if (interrupt)
		vector_nr = iack_cycle(nr);

	// exception vector fetch and exception stack frame
	// operations don't allocate new cachelines
	cache_default_data |= CACHE_DISABLE_ALLOCATE;

	exception_debug (nr);
	MakeSR ();

	if (!regs.s) {
		regs.usp = m68k_areg (regs, 7);
		if (currprefs.cpu_model >= 68020 && currprefs.cpu_model < 68060) {
			m68k_areg (regs, 7) = regs.m ? regs.msp : regs.isp;
		} else {
			m68k_areg (regs, 7) = regs.isp;
		}
		regs.s = 1;
		mmu_set_super (true);
	}

	newpc = x_get_long (regs.vbr + 4 * vector_nr);
#if 0
	write_log (_T("Exception %d: %08x -> %08x\n"), nr, currpc, newpc);
#endif

	if (nr == 2) { // bus error
        //write_log (_T("Exception_mmu %08x %08x %08x\n"), currpc, oldpc, regs.mmu_fault_addr);
		if (currprefs.mmu_model == 68040)
			Exception_build_stack_frame(oldpc, currpc, regs.mmu_ssw, vector_nr, 0x7);
		else
			Exception_build_stack_frame(regs.mmu_fault_addr, currpc, regs.mmu_fslw, vector_nr, 0x4);
	} else if (nr == 3) { // address error
        Exception_build_stack_frame(last_fault_for_exception_3, currpc, 0, vector_nr, 0x2);
#ifdef DEBUGGER
		write_log (_T("Exception %d (%x) at %x -> %x!\n"), nr, last_fault_for_exception_3, currpc, get_long_debug (regs.vbr + 4 * nr));
#endif
	} else if (regs.m && interrupt) { /* M + Interrupt */
		Exception_build_stack_frame(oldpc, currpc, regs.mmu_ssw, vector_nr, 0x0);
		MakeSR();
		regs.m = 0;
		if (currprefs.cpu_model < 68060) {
			regs.msp = m68k_areg(regs, 7);
			Exception_build_stack_frame(oldpc, currpc, regs.mmu_ssw, vector_nr, 0x1);
		}
	} else {
		Exception_build_stack_frame_common(oldpc, currpc, regs.mmu_ssw, nr, vector_nr);
	}

	if (newpc & 1) {
		if (nr == 2 || nr == 3)
			cpu_halt (CPU_HALT_DOUBLE_FAULT);
		else
			exception3_read_special(regs.ir, newpc, 2, 1);
		return;
	}

	cache_default_data &= ~CACHE_DISABLE_ALLOCATE;

	m68k_setpci (newpc);
	if (interrupt)
		regs.intmask = nr - 24;
	fill_prefetch ();
	exception_check_trace (nr);
}

static void add_approximate_exception_cycles(int nr)
{
	int cycles;

	if (currprefs.cpu_model == 68000) {
		if (nr >= 24 && nr <= 31) {
			/* Interrupts */
			cycles = 44;
		} else if (nr >= 32 && nr <= 47) {
			/* Trap */
			cycles = 34;
		} else {
			switch (nr)
			{
			case 2: cycles = 58; break;		/* Bus error */
			case 3: cycles = 54; break;		/* Address error */
			case 4: cycles = 34; break;		/* Illegal instruction */
			case 5: cycles = 34; break;		/* Division by zero */
			case 6: cycles = 34; break;		/* CHK */
			case 7: cycles = 30; break;		/* TRAPV */
			case 8: cycles = 34; break;		/* Privilege violation */
			case 9: cycles = 34; break;		/* Trace */
			case 10: cycles = 34; break;	/* Line-A */
			case 11: cycles = 34; break;	/* Line-F */
			default:
				cycles = 4;
				break;
			}
		}
	} else if (currprefs.cpu_model == 68010) {
		if (nr >= 24 && nr <= 31) {
			/* Interrupts */
			cycles = 48;
		} else if (nr >= 32 && nr <= 47) {
			/* Trap */
			cycles = 38;
		} else {
			switch (nr)
			{
			case 2: cycles = 140; break;	/* Bus error */
			case 3: cycles = 136; break;	/* Address error */
			case 4: cycles = 38; break;		/* Illegal instruction */
			case 5: cycles = 38; break;		/* Division by zero */
			case 6: cycles = 38; break;		/* CHK */
			case 7: cycles = 40; break;		/* TRAPV */
			case 8: cycles = 38; break;		/* Privilege violation */
			case 9: cycles = 38; break;		/* Trace */
			case 10: cycles = 38; break;	/* Line-A */
			case 11: cycles = 38; break;	/* Line-F */
			case 14: cycles = 38; break;	/* RTE frame error */
			default:
				cycles = 4;
				break;
			}
		}
	} else {
		return;
	}
	cycles = adjust_cycles(cycles * CYCLE_UNIT / 2);
	x_do_cycles(cycles);
}

static void Exception_normal (int nr)
{
	uae_u32 newpc;
	uae_u32 currpc = m68k_getpc();
	uae_u32 nextpc;
	int sv = regs.s;
	int interrupt;
	int vector_nr = nr;
	bool g1 = false;

	cache_default_data |= CACHE_DISABLE_ALLOCATE;

	interrupt = nr >= 24 && nr < 24 + 8;
	if (interrupt)
		vector_nr = iack_cycle(nr);

	if (currprefs.cpu_model <= 68010) {
		g1 = generates_group1_exception(regs.ir);
	}

	exception_debug (nr);
	MakeSR ();

	if (!regs.s) {
		regs.usp = m68k_areg (regs, 7);
		if (currprefs.cpu_model >= 68020 && currprefs.cpu_model < 68060) {
			m68k_areg (regs, 7) = regs.m ? regs.msp : regs.isp;
		} else {
			m68k_areg (regs, 7) = regs.isp;
		}
		regs.s = 1;
		if (currprefs.mmu_model)
			mmu_set_super (regs.s != 0);
	}

	if ((m68k_areg(regs, 7) & 1) && currprefs.cpu_model < 68020) {
		if (nr == 2 || nr == 3)
			cpu_halt (CPU_HALT_DOUBLE_FAULT);
		else
			exception3_notinstruction(regs.ir, m68k_areg(regs, 7));
		return;
	}
	if ((nr == 2 || nr == 3) && exception_in_exception < 0) {
		cpu_halt (CPU_HALT_DOUBLE_FAULT);
		return;
	}

	if (!currprefs.cpu_compatible) {
		addrbank *ab = &get_mem_bank(m68k_areg(regs, 7) - 4);
		// Not plain RAM check because some CPU type tests that
		// don't need to return set stack to ROM..
		if (!ab || ab == &dummy_bank || (ab->flags & ABFLAG_IO)) {
			cpu_halt(CPU_HALT_SSP_IN_NON_EXISTING_ADDRESS);
			return;
		}
	}

	bool used_exception_build_stack_frame = false;

	if (currprefs.cpu_model > 68000) {
		uae_u32 oldpc = regs.instruction_pc;
		nextpc = exception_pc (nr);
		if (nr == 2 || nr == 3) {
			int i;
			if (currprefs.cpu_model >= 68040) {
				if (nr == 2) {
					if (currprefs.mmu_model) {
						// 68040/060 mmu bus error
						for (i = 0 ; i < 7 ; i++) {
							m68k_areg (regs, 7) -= 4;
							x_put_long (m68k_areg (regs, 7), 0);
						}
						m68k_areg (regs, 7) -= 4;
						x_put_long (m68k_areg (regs, 7), regs.wb3_data);
						m68k_areg (regs, 7) -= 4;
						x_put_long (m68k_areg (regs, 7), regs.mmu_fault_addr);
						m68k_areg (regs, 7) -= 4;
						x_put_long (m68k_areg (regs, 7), regs.mmu_fault_addr);
						m68k_areg (regs, 7) -= 2;
						x_put_word (m68k_areg (regs, 7), 0);
						m68k_areg (regs, 7) -= 2;
						x_put_word (m68k_areg (regs, 7), 0);
						m68k_areg (regs, 7) -= 2;
						x_put_word (m68k_areg (regs, 7), regs.wb3_status);
						regs.wb3_status = 0;
						m68k_areg (regs, 7) -= 2;
						x_put_word (m68k_areg (regs, 7), regs.mmu_ssw);
						m68k_areg (regs, 7) -= 4;
						x_put_long (m68k_areg (regs, 7), regs.mmu_fault_addr);

						m68k_areg (regs, 7) -= 2;
						x_put_word (m68k_areg (regs, 7), 0x7000 + vector_nr * 4);
						m68k_areg (regs, 7) -= 4;
						x_put_long (m68k_areg (regs, 7), regs.instruction_pc);
						m68k_areg (regs, 7) -= 2;
						x_put_word (m68k_areg (regs, 7), regs.sr);
						newpc = x_get_long (regs.vbr + 4 * vector_nr);
						if (newpc & 1) {
							if (nr == 2 || nr == 3)
								cpu_halt (CPU_HALT_DOUBLE_FAULT);
							else
								exception3_read_special(regs.ir, newpc, 2, 1);
							return;
						}
						m68k_setpc (newpc);
#ifdef JIT
						if (currprefs.cachesize) {
							set_special(SPCFLAG_END_COMPILE);
						}
#endif
						exception_check_trace (nr);
						return;

					} else {

						// 68040 bus error (not really, some garbage?)
						for (i = 0 ; i < 18 ; i++) {
							m68k_areg (regs, 7) -= 2;
							x_put_word (m68k_areg (regs, 7), 0);
						}
						m68k_areg (regs, 7) -= 4;
						x_put_long (m68k_areg (regs, 7), last_fault_for_exception_3);
						m68k_areg (regs, 7) -= 2;
						x_put_word (m68k_areg (regs, 7), 0);
						m68k_areg (regs, 7) -= 2;
						x_put_word (m68k_areg (regs, 7), 0);
						m68k_areg (regs, 7) -= 2;
						x_put_word (m68k_areg (regs, 7), 0);
						m68k_areg (regs, 7) -= 2;
						x_put_word (m68k_areg (regs, 7), 0x0140 | (sv ? 6 : 2)); /* SSW */
						m68k_areg (regs, 7) -= 4;
						x_put_long (m68k_areg (regs, 7), last_addr_for_exception_3);
						m68k_areg (regs, 7) -= 2;
						x_put_word (m68k_areg (regs, 7), 0x7000 + vector_nr * 4);
						m68k_areg (regs, 7) -= 4;
						x_put_long (m68k_areg (regs, 7), regs.instruction_pc);
						m68k_areg (regs, 7) -= 2;
						x_put_word (m68k_areg (regs, 7), regs.sr);
						goto kludge_me_do;

					}

				} else {
					// 68040/060 odd PC address error
					Exception_build_stack_frame(last_fault_for_exception_3, currpc, 0, vector_nr, 0x02);
					used_exception_build_stack_frame = true;
				}
			} else if (currprefs.cpu_model >= 68020) {
				// 68020/030 odd PC address error (partially implemented only)
				// annoyingly this generates frame B, not A.
				uae_u16 ssw = (sv ? 4 : 0) | last_fc_for_exception_3;
				ssw |= MMU030_SSW_RW | MMU030_SSW_SIZE_W;
				regs.mmu_fault_addr = last_fault_for_exception_3;
				mmu030_state[0] = mmu030_state[1] = 0;
				mmu030_data_buffer_out = 0;
				Exception_build_stack_frame(last_fault_for_exception_3, currpc, ssw, vector_nr, 0x0b);
				used_exception_build_stack_frame = true;
			} else {
				// 68010 bus/address error (partially implemented only)
				uae_u16 ssw = (sv ? 4 : 0) | last_fc_for_exception_3;
				ssw |= last_di_for_exception_3 > 0 ? 0x0000 : (last_di_for_exception_3 < 0 ? (0x2000 | 0x1000) : 0x2000);
				ssw |= (!last_writeaccess_for_exception_3 && last_di_for_exception_3) ? 0x1000 : 0x000; // DF
				ssw |= (last_op_for_exception_3 & 0x10000) ? 0x0400 : 0x0000; // HB
				ssw |= last_size_for_exception_3 == 0 ? 0x0200 : 0x0000; // BY
				ssw |= last_writeaccess_for_exception_3 ? 0x0000 : 0x0100; // RW
				if (last_op_for_exception_3 & 0x20000)
					ssw &= 0x00ff;
				regs.mmu_fault_addr = last_fault_for_exception_3;
				Exception_build_stack_frame(oldpc, currpc, ssw, vector_nr, 0x08);
				used_exception_build_stack_frame = true;
			}
#ifdef DEBUGGER
			write_log (_T("Exception %d (%x) at %x -> %x!\n"), nr, regs.instruction_pc, currpc, get_long_debug (regs.vbr + 4 * vector_nr));
#endif
		} else if (regs.m && interrupt) { /* M + Interrupt */
			m68k_areg (regs, 7) -= 2;
			x_put_word (m68k_areg (regs, 7), vector_nr * 4);
			if (currprefs.cpu_model < 68060) {
				m68k_areg (regs, 7) -= 4;
				x_put_long (m68k_areg (regs, 7), currpc);
				m68k_areg (regs, 7) -= 2;
				x_put_word (m68k_areg (regs, 7), regs.sr);
				regs.sr |= (1 << 13);
				regs.msp = m68k_areg(regs, 7);
				regs.m = 0;
				m68k_areg(regs, 7) = regs.isp;
				m68k_areg (regs, 7) -= 2;
				x_put_word (m68k_areg (regs, 7), 0x1000 + vector_nr * 4);
			}
		} else {
			Exception_build_stack_frame_common(oldpc, currpc, regs.mmu_ssw, nr, vector_nr);
			used_exception_build_stack_frame = true;
		}
 	} else {
		nextpc = m68k_getpc ();
		if (nr == 2 || nr == 3) {
			// 68000 bus/address error
			uae_u16 mode = (sv ? 4 : 0) | last_fc_for_exception_3;
			mode |= last_writeaccess_for_exception_3 ? 0 : 16;
			mode |= last_notinstruction_for_exception_3 ? 8 : 0;
			exception_in_exception = -1;
			Exception_build_68000_address_error_stack_frame(mode, last_op_for_exception_3, last_fault_for_exception_3, last_addr_for_exception_3);
#ifdef DEBUGGER
			write_log (_T("Exception %d (%x) at %x -> %x!\n"), nr, last_fault_for_exception_3, currpc, get_long_debug (regs.vbr + 4 * vector_nr));
#endif
			goto kludge_me_do;
		}
	}
	if (!used_exception_build_stack_frame) {
		m68k_areg (regs, 7) -= 4;
		x_put_long (m68k_areg (regs, 7), nextpc);
		m68k_areg (regs, 7) -= 2;
		x_put_word (m68k_areg (regs, 7), regs.sr);
	}
	if (currprefs.cpu_model == 68060 && interrupt) {
		regs.m = 0;
	}
	if (currprefs.cpu_model == 68040 && nr == 3 && (last_op_for_exception_3 & 0x10000)) {
		// Weird 68040 bug with RTR and RTE. New SR when exception starts. Stacked SR is different!
		// Just replace it in stack, it is safe enough because we are in address error exception
		// any other exception would halt the CPU.
		x_put_word(m68k_areg(regs, 7), last_sr_for_exception3);
	}
kludge_me_do:
	if ((regs.vbr & 1) && currprefs.cpu_model <= 68010) {
		cpu_halt(CPU_HALT_DOUBLE_FAULT);
		return;
	}
	if (interrupt)
		regs.intmask = nr - 24;
	newpc = x_get_long (regs.vbr + 4 * vector_nr);
	exception_in_exception = 0;
	if (newpc & 1) {
		if (nr == 2 || nr == 3) {
			cpu_halt(CPU_HALT_DOUBLE_FAULT);
			return;
		}
		// 4 idle, write pc low, write sr, write pc high, read vector high, read vector low
		x_do_cycles(adjust_cycles(6 * 4 * CYCLE_UNIT / 2));
		if (currprefs.cpu_model == 68000) {
			regs.t1 = 0;
			MakeSR();
			m68k_setpc(regs.vbr + 4 * vector_nr);
			if (interrupt) {
				regs.ir = nr;
				exception3_read_access(regs.ir | 0x20000 | 0x10000, newpc, sz_word, 2);
			} else {
				exception3_read_access(regs.ir | 0x40000 | 0x20000 | (g1 ? 0x10000 : 0), newpc, sz_word, 2);
			}
		} else if (currprefs.cpu_model == 68010) {
			regs.t1 = 0;
			MakeSR();
			regs.write_buffer = 4 * vector_nr;
			regs.read_buffer = newpc;
			regs.irc = regs.read_buffer;
			exception3_read_access(regs.ir, newpc, sz_word, 2);
		} else {
			exception_check_trace(nr);
			exception3_notinstruction(regs.ir, newpc);
		}
		return;
	}
	add_approximate_exception_cycles(nr);
	m68k_setpc (newpc);
	cache_default_data &= ~CACHE_DISABLE_ALLOCATE;
#ifdef JIT
	if (currprefs.cachesize) {
		set_special(SPCFLAG_END_COMPILE);
	}
#endif
#ifdef DEBUGGER
	branch_stack_push(currpc, nextpc);
#endif
	regs.ipl_pin = intlev();
	ipl_fetch_now();
	fill_prefetch ();
	exception_check_trace (nr);
}

// address = format $2 stack frame address field
static void ExceptionX (int nr, uaecptr address, uaecptr oldpc)
{
	uaecptr pc = m68k_getpc();
	regs.exception = nr;
	regs.loop_mode = 0;

#ifdef DEBUGGER
	if (debug_dma) {
		record_dma_event_data(DMA_EVENT_CPUINS, 0x20000);
	}
#endif
	if (cpu_tracer) {
		cputrace.state = nr;
	}
	if (!regs.s) {
		regs.instruction_pc_user_exception = pc;
	}
	if (oldpc != 0xffffffff) {
		regs.instruction_pc = oldpc;
	}
#ifdef DEBUGGER
	debug_exception(nr);
#endif
	m68k_resumestopped();

#ifdef CPUEMU_13
	if (currprefs.cpu_cycle_exact && currprefs.cpu_model <= 68010)
		Exception_ce000 (nr);
	else
#endif
	{
		if (currprefs.cpu_model == 68060) {
			regs.buscr &= 0xa0000000;
			regs.buscr |= regs.buscr >> 1;
		}
		if (currprefs.mmu_model) {
			if (currprefs.cpu_model == 68030)
				Exception_mmu030(nr, m68k_getpc());
			else
				Exception_mmu(nr, m68k_getpc());
		} else {
			Exception_normal(nr);
		}
	}
	regs.exception = 0;
	if (cpu_tracer) {
		cputrace.state = 0;
	}
}

void REGPARAM2 Exception_cpu_oldpc(int nr, uaecptr oldpc)
{
	bool t0 = currprefs.cpu_model >= 68020 && regs.t0 && !regs.t1;
	ExceptionX(nr, 0xffffffff, oldpc);
	// Check T0 trace
	// RTE format error ignores T0 trace
	if (nr != 14) {
		if (currprefs.cpu_model >= 68040 && internalexception(nr)) {
			t0 = false;
		}
		if (t0) {
			activate_trace();
		}
	}
}
void REGPARAM2 Exception_cpu(int nr)
{
	Exception_cpu_oldpc(nr, 0xffffffff);
}
void REGPARAM2 Exception(int nr)
{
	ExceptionX(nr, 0xffffffff, 0xffffffff);
}
void REGPARAM2 ExceptionL(int nr, uaecptr address)
{
	ExceptionX(nr, address, 0xffffffff);
}

static void bus_error()
{
	TRY (prb2) {
		Exception (2);
	} CATCH (prb2) {
		cpu_halt (CPU_HALT_BUS_ERROR_DOUBLE_FAULT);
	} ENDTRY
}

static int get_ipl()
{
	return regs.ipl[0];
}

static void do_interrupt (int nr)
{
#ifdef DEBUGGER
	if (debug_dma)
		record_dma_event(DMA_EVENT_CPUIRQ);
#endif
	if (inputrecord_debug & 2) {
		if (input_record > 0)
			inprec_recorddebug_cpu(2, 0);
		else if (input_play > 0)
			inprec_playdebug_cpu(2, 0);
	}

	assert (nr < 8 && nr >= 0);

	intlev_ack(nr);

	for (;;) {
		Exception (nr + 24);
		if (!currprefs.cpu_compatible || currprefs.cpu_model == 68060)
			break;
		if (m68k_interrupt_delay)
			nr = get_ipl();
		else
			nr = intlev();
		if (nr <= 0 || regs.intmask >= nr)
			break;
	}

	doint ();
}

void NMI ()
{
	do_interrupt (7);
}

static void cpu_halt_clear(void)
{
	regs.halted = 0;
	if (gui_data.cpu_halted) {
		gui_data.cpu_halted = 0;
		gui_led(LED_CPU, 0, -1);
	}
}

static void maybe_disable_fpu()
{
	if (currprefs.cpu_model == 68060 && currprefs.cpuboard_type == 0 && (rtarea_base != 0xf00000 || !need_uae_boot_rom(&currprefs))) {
		// disable FPU at reset if no 68060 accelerator board and no $f0 ROM.
		regs.pcr |= 2;
	}
	if (!currprefs.fpu_model) {
		regs.pcr |= 2;
	}
}

static void m68k_reset_sr()
{
	SET_XFLG ((regs.sr >> 4) & 1);
	SET_NFLG ((regs.sr >> 3) & 1);
	SET_ZFLG ((regs.sr >> 2) & 1);
	SET_VFLG ((regs.sr >> 1) & 1);
	SET_CFLG (regs.sr & 1);
	regs.t1 = (regs.sr >> 15) & 1;
	regs.t0 = (regs.sr >> 14) & 1;
	regs.s  = (regs.sr >> 13) & 1;
	regs.m  = (regs.sr >> 12) & 1;
	regs.intmask = (regs.sr >> 8) & 7;
	/* set stack pointer */
	if (regs.s)
		m68k_areg (regs, 7) = regs.isp;
	else
		m68k_areg (regs, 7) = regs.usp;
}

static void m68k_reset2(bool hardreset)
{
	uae_u32 v;

	cpu_halt_clear();

	regs.spcflags = 0;
	m68k_reset_delay = false;
	regs.ipl[0] = regs.ipl[1] = regs.ipl_pin = 0;
	for (int i = 0; i < IRQ_SOURCE_MAX; i++) {
		uae_interrupts2[i] = 0;
		uae_interrupts6[i] = 0;
		uae_interrupt = 0;
	}

	// Force config changes (CPU speed) into effect on hard reset
	update_68k_cycles();

#ifdef SAVESTATE
	if (isrestore()) {
		m68k_reset_sr();
		m68k_setpc_normal(regs.pc);
		return;
	} else {
		m68k_reset_delay = currprefs.reset_delay;
		set_special(SPCFLAG_CHECK);
	}
#endif
	regs.s = 1;
	if (currprefs.cpuboard_type) {
		uaecptr stack;
		v = cpuboard_get_reset_pc(&stack);
		m68k_areg(regs, 7) = stack;
	} else {
		v = get_long(4);
		m68k_areg(regs, 7) = get_long(0);
	}

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
	regs.lastipl = 0;
	regs.vbr = regs.sfc = regs.dfc = 0;
	regs.irc = 0xffff;
#ifdef FPUEMU
	fpu_reset();
#endif
	regs.caar = regs.cacr = 0;
	regs.itt0 = regs.itt1 = regs.dtt0 = regs.dtt1 = 0;
	regs.tcr = regs.mmusr = regs.urp = regs.srp = regs.buscr = 0;
	mmu_tt_modified();
	if (currprefs.cpu_model == 68020) {
		regs.cacr |= 8;
		set_cpu_caches (false);
	}

	mmufixup[0].reg = -1;
	mmufixup[1].reg = -1;
	mmu030_cache_state = CACHE_ENABLE_ALL;
	mmu_cache_state = CACHE_ENABLE_ALL;
	if (currprefs.cpu_model >= 68040) {
		set_cpu_caches(false);
	}
	if (currprefs.mmu_model >= 68040) {
		mmu_reset();
		mmu_set_tc(regs.tcr);
		mmu_set_super(regs.s != 0);
	} else if (currprefs.mmu_model == 68030) {
		mmu030_reset(hardreset || regs.halted);
	} else {
		a3000_fakekick (0);
		/* only (E)nable bit is zeroed when CPU is reset, A3000 SuperKickstart expects this */
		fake_tc_030 &= ~0x80000000;
		fake_tt0_030 &= ~0x80000000;
		fake_tt1_030 &= ~0x80000000;
		if (hardreset || regs.halted) {
			fake_srp_030 = fake_crp_030 = 0;
			fake_tt0_030 = fake_tt1_030 = fake_tc_030 = 0;
		}
		fake_mmusr_030 = 0;
	}

	/* 68060 FPU is not compatible with 68040,
	* 68060 accelerators' boot ROM disables the FPU
	*/
	regs.pcr = 0;
	if (currprefs.cpu_model == 68060) {
		regs.pcr = currprefs.fpu_model == 68060 ? MC68060_PCR : MC68EC060_PCR;
		regs.pcr |= (currprefs.cpu060_revision & 0xff) << 8;
		maybe_disable_fpu();
	}
//	regs.ce020memcycles = 0;
	regs.ce020startcycle = regs.ce020endcycle = 0;

	fill_prefetch();
}

void m68k_reset()
{
	m68k_reset2(false);
}

void cpu_change(const int newmodel)
{
	if (newmodel == currprefs.cpu_model)
		return;
	fallback_new_cpu_model = newmodel;
	cpu_halt(CPU_HALT_ACCELERATOR_CPU_FALLBACK);
}

void cpu_fallback(int mode)
{
	int fallbackmodel;
	if (currprefs.chipset_mask & CSMASK_AGA) {
		fallbackmodel = 68020;
	} else {
		fallbackmodel = 68000;
	}
	if (mode < 0) {
		if (currprefs.cpu_model > fallbackmodel) {
			cpu_change(fallbackmodel);
		} else if (fallback_new_cpu_model) {
			cpu_change(fallback_new_cpu_model);
		}
	} else if (mode == 0) {
		cpu_change(fallbackmodel);
	} else if (mode) {
		if (fallback_cpu_model) {
			cpu_change(fallback_cpu_model);
		}
	}
}

static void cpu_do_fallback()
{
	bool fallbackmode = false;
	if ((fallback_new_cpu_model < 68020 && !(currprefs.chipset_mask & CSMASK_AGA)) || (fallback_new_cpu_model == 68020 && (currprefs.chipset_mask & CSMASK_AGA))) {
		// -> 68000/68010 or 68EC020
		fallback_cpu_model = currprefs.cpu_model;
		fallback_fpu_model = currprefs.fpu_model;
		fallback_mmu_model = currprefs.mmu_model;
		fallback_cpu_compatible = currprefs.cpu_compatible;
		fallback_cpu_address_space_24 = currprefs.address_space_24;
		changed_prefs.cpu_model = currprefs.cpu_model_fallback && fallback_new_cpu_model <= 68020 ? currprefs.cpu_model_fallback : fallback_new_cpu_model;
		changed_prefs.fpu_model = 0;
		changed_prefs.mmu_model = 0;
		changed_prefs.cpu_compatible = true;
		changed_prefs.address_space_24 = true;
		memcpy(&fallback_regs, &regs, sizeof(struct regstruct));
		fallback_regs.pc = M68K_GETPC;
		fallbackmode = true;
	} else {
		// -> 68020+
		changed_prefs.cpu_model = fallback_cpu_model;
		changed_prefs.fpu_model = fallback_fpu_model;
		changed_prefs.mmu_model = fallback_mmu_model;
		changed_prefs.cpu_compatible = fallback_cpu_compatible;
		changed_prefs.address_space_24 = fallback_cpu_address_space_24;
		fallback_cpu_model = 0;
	}
	init_m68k();
	build_cpufunctbl();
	m68k_reset2(false);
	if (!fallbackmode) {
		// restore original 68020+
		memcpy(&regs, &fallback_regs, sizeof(regs));
		restore_banks();
		memory_restore();
#ifdef DEBUGGER
		memory_map_dump();
#endif
		m68k_setpc(fallback_regs.pc);
	} else {
		// 68000/010/EC020
		memory_restore();
		expansion_cpu_fallback();
#ifdef DEBUGGER
		memory_map_dump();
#endif
	}
}

static void m68k_reset_restore()
{
	// hardreset and 68000/68020 fallback mode? Restore original mode.
	if (fallback_cpu_model) {
		fallback_new_cpu_model = fallback_cpu_model;
		fallback_regs.pc = 0;
		cpu_do_fallback();
	}
}

void REGPARAM2 op_unimpl (uae_u32 opcode)
{
	static int warned;
	if (warned < 1000) {
		write_log (_T("68060 unimplemented opcode %04X, PC=%08x SP=%08x\n"), opcode, regs.instruction_pc, regs.regs[15]);
		warned++;
	}
	ExceptionL (61, regs.instruction_pc);
}

uae_u32 REGPARAM2 op_illg (uae_u32 opcode)
{
	uaecptr pc = m68k_getpc ();
	static int warned;
	int inrom = in_rom (pc);
	int inrt = in_rtarea (pc);

	if (opcode == 0x4afc || opcode == 0xfc4a) {
		if (!valid_address(pc, 4) && valid_address(pc - 4, 4)) {
			// PC fell off the end of RAM
			bus_error();
			return 4;
		}
	}

	// BKPT?
	if (opcode >= 0x4848 && opcode <= 0x484f && currprefs.cpu_model >= 68020) {
		// some boards hang because there is no break point cycle acknowledge
		if (currprefs.cs_bkpthang) {
			cpu_halt(CPU_HALT_BKPT);
			return 4;
		}
	}

#ifdef DEBUGGER
	if (debugmem_illg(opcode)) {
		m68k_incpc_normal(2);
		return 4;
	}
#endif

	if (cloanto_rom && (opcode & 0xF100) == 0x7100) {
		m68k_dreg (regs, (opcode >> 9) & 7) = static_cast<uae_s8>(opcode & 0xFF);
		m68k_incpc_normal (2);
		fill_prefetch ();
		return 4;
	}

	if (opcode == 0x4E7B && inrom) {
		if (get_long (0x10) == 0) {
			notify_user (NUMSG_KS68020);
			uae_restart(&currprefs, -1, nullptr);
			m68k_setstopped(1);
			return 4;
		}
	}

#ifdef AUTOCONFIG
	if (opcode == 0xFF0D && inrt) {
		/* User-mode STOP replacement */
		m68k_setstopped(1);
		return 4;
	}

	if ((opcode & 0xF000) == 0xA000 && inrt) {
		/* Calltrap. */
		m68k_incpc_normal (2);
		m68k_handle_trap(opcode & 0xFFF);
		fill_prefetch ();
		return 4;
	}
#endif

	if ((opcode & 0xF000) == 0xF000) {
		// Missing MMU or FPU cpSAVE/cpRESTORE privilege check
		if (privileged_copro_instruction(opcode)) {
			Exception(8);
		} else {
			if (warned < 20) {
#ifdef DEBUGGER
				write_log(_T("B-Trap %04X at %08X -> %08X\n"), opcode, pc, get_long_debug(regs.vbr + 0x2c));
#endif
				warned++;
			}
			Exception(0xB);
		}
		//activate_debugger_new();
		return 4;
	}
	if ((opcode & 0xF000) == 0xA000) {
		if (warned < 20) {
#ifdef DEBUGGER
			write_log(_T("A-Trap %04X at %08X -> %08X\n"), opcode, pc, get_long_debug(regs.vbr + 0x28));
#endif
			warned++;
		}
		Exception (0xA);
		//activate_debugger_new();
		return 4;
	}
	if (warned < 20) {
#ifdef DEBUGGER
		write_log (_T("Illegal instruction: %04x at %08X -> %08X\n"), opcode, pc, get_long_debug(regs.vbr + 0x10));
#endif
		warned++;
		//activate_debugger_new();
	}

	Exception (4);
	return 4;
}
void REGPARAM2 op_illg_noret(uae_u32 opcode)
{
	op_illg(opcode);
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

static bool mmu_op30fake_pmove (uaecptr pc, uae_u32 opcode, uae_u16 next, uaecptr extra)
{
	int preg = (next >> 10) & 31;
	int rw = (next >> 9) & 1;
	int fd = (next >> 8) & 1;
	int unused = (next & 0xff);
	const TCHAR *reg = nullptr;
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
			x_put_long (extra, fake_tc_030);
		else
			fake_tc_030 = x_get_long (extra);
		break;
	case 0x12: // SRP
		reg = _T("SRP");
		siz = 8;
		if (rw) {
			x_put_long (extra, fake_srp_030 >> 32);
			x_put_long (extra + 4, static_cast<uae_u32>(fake_srp_030));
		} else {
			fake_srp_030 = static_cast<uae_u64>(x_get_long(extra)) << 32;
			fake_srp_030 |= x_get_long (extra + 4);
		}
		break;
	case 0x13: // CRP
		reg = _T("CRP");
		siz = 8;
		if (rw) {
			x_put_long (extra, fake_crp_030 >> 32);
			x_put_long (extra + 4, static_cast<uae_u32>(fake_crp_030));
		} else {
			fake_crp_030 = static_cast<uae_u64>(x_get_long(extra)) << 32;
			fake_crp_030 |= x_get_long (extra + 4);
		}
		break;
	case 0x18: // MMUSR
		if (fd) {
			// FD must be always zero when MMUSR read or write
			return true;
		}
		reg = _T("MMUSR");
		siz = 2;
		if (rw)
			x_put_word (extra, fake_mmusr_030);
		else
			fake_mmusr_030 = x_get_word (extra);
		break;
	case 0x02: // TT0
		reg = _T("TT0");
		siz = 4;
		if (rw)
			x_put_long (extra, fake_tt0_030);
		else
			fake_tt0_030 = x_get_long (extra);
		break;
	case 0x03: // TT1
		reg = _T("TT1");
		siz = 4;
		if (rw)
			x_put_long (extra, fake_tt1_030);
		else
			fake_tt1_030 = x_get_long (extra);
		break;
	}

	if (!reg)
		return true;

#if MMUOP_DEBUG > 0
	{
		uae_u32 val;
		if (siz == 8) {
			uae_u32 val2 = x_get_long (extra);
			val = x_get_long (extra + 4);
			if (rw)
				write_log (_T("PMOVE %s,%08X%08X"), reg, val2, val);
			else
				write_log (_T("PMOVE %08X%08X,%s"), val2, val, reg);
		} else {
			if (siz == 4)
				val = x_get_long (extra);
			else
				val = x_get_word (extra);
			if (rw)
				write_log (_T("PMOVE %s,%08X"), reg, val);
			else
				write_log (_T("PMOVE %08X,%s"), val, reg);
		}
		write_log (_T(" PC=%08X\n"), pc);
	}
#endif
	if ((currprefs.cs_mbdmac & 1) && currprefs.mbresmem_low.size > 0) {
		if (otc != fake_tc_030) {
			a3000_fakekick (fake_tc_030 & 0x80000000);
		}
	}
	return false;
}

static bool mmu_op30fake_ptest (uaecptr pc, uae_u32 opcode, uae_u16 next, uaecptr extra)
{
	int eamode = (opcode >> 3) & 7;
 	int rreg = opcode & 7;
    int level = (next&0x1C00)>>10;
    int a = (next >> 8) & 1;

	if (mmu_op30_invea(opcode))
		return true;
    if (!level && a)
		return true;

#if MMUOP_DEBUG > 0
	TCHAR tmp[10];

	tmp[0] = 0;
	if ((next >> 8) & 1)
		_stprintf (tmp, _T(",A%d"), (next >> 4) & 15);
	write_log (_T("PTEST%c %02X,%08X,#%X%s PC=%08X\n"),
		((next >> 9) & 1) ? 'W' : 'R', (next & 15), extra, (next >> 10) & 7, tmp, pc);
#endif
	fake_mmusr_030 = 0;
	return false;
}

static bool mmu_op30fake_pload (uaecptr pc, uae_u32 opcode, uae_u16 next, uaecptr extra)
{
  	int unused = (next & (0x100 | 0x80 | 0x40 | 0x20));

	if (mmu_op30_invea(opcode))
		return true;
	if (unused)
		return true;
	write_log(_T("PLOAD\n"));
	return false;
}

static bool mmu_op30fake_pflush (uaecptr pc, uae_u32 opcode, uae_u16 next, uaecptr extra)
{
	int flushmode = (next >> 8) & 31;
	int fc = next & 31;
	int mask = (next >> 5) & 3;
	int fc_bits = next & 0x7f;
	TCHAR fname[100];

	switch (flushmode)
	{
	case 0x00:
	case 0x02:
		return mmu_op30fake_pload(pc, opcode, next, extra);
	case 0x18:
		if (mmu_op30_invea(opcode))
			return true;
		_sntprintf (fname, sizeof fname, _T("FC=%x MASK=%x EA=%08x"), fc, mask, 0);
		break;
	case 0x10:
		_sntprintf (fname, sizeof fname, _T("FC=%x MASK=%x"), fc, mask);
		break;
	case 0x04:
		if (fc_bits)
			return true;
		_tcscpy (fname, _T("ALL"));
		break;
	default:
		return true;
	}
#if MMUOP_DEBUG > 0
	write_log (_T("PFLUSH %s PC=%08X\n"), fname, pc);
#endif
	return false;
}

// 68030 (68851) MMU instructions only
bool mmu_op30(uaecptr pc, uae_u32 opcode, uae_u16 extra, uaecptr extraa)
{
	int type = extra >> 13;
	int fline = 0;

	switch (type)
	{
	case 0:
	case 2:
	case 3:
		if (currprefs.mmu_model)
			fline = mmu_op30_pmove(pc, opcode, extra, extraa);
		else
			fline = mmu_op30fake_pmove(pc, opcode, extra, extraa);
	break;
	case 1:
		if (currprefs.mmu_model)
			fline = mmu_op30_pflush(pc, opcode, extra, extraa);
		else
			fline = mmu_op30fake_pflush(pc, opcode, extra, extraa);
	break;
	case 4:
		if (currprefs.mmu_model)
			fline = mmu_op30_ptest(pc, opcode, extra, extraa);
		else
			fline = mmu_op30fake_ptest(pc, opcode, extra, extraa);
	break;
	}
	if (fline > 0) {
		m68k_setpc(pc);
		op_illg(opcode);
	}
	return fline != 0;
}

/* check if an address matches a ttr */
static int fake_mmu_do_match_ttr(uae_u32 ttr, uaecptr addr, bool super)
{
	if (ttr & MMU_TTR_BIT_ENABLED)	{	/* TTR enabled */
		uae_u8 msb, mask;

		msb = ((addr ^ ttr) & MMU_TTR_LOGICAL_BASE) >> 24;
		mask = (ttr & MMU_TTR_LOGICAL_MASK) >> 16;

		if (!(msb & ~mask)) {

			if ((ttr & MMU_TTR_BIT_SFIELD_ENABLED) == 0) {
				if (((ttr & MMU_TTR_BIT_SFIELD_SUPER) == 0) != (super == 0)) {
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

	if (data) {
		res = fake_mmu_do_match_ttr(regs.dtt0, addr, super);
		if (res == TTR_NO_MATCH)
			res = fake_mmu_do_match_ttr(regs.dtt1, addr, super);
	} else {
		res = fake_mmu_do_match_ttr(regs.itt0, addr, super);
		if (res == TTR_NO_MATCH)
			res = fake_mmu_do_match_ttr(regs.itt1, addr, super);
	}
	return res;
}

// 68040+ MMU instructions only
void mmu_op (uae_u32 opcode, uae_u32 extra)
{
	if (currprefs.mmu_model) {
		mmu_op_real (opcode, extra);
		return;
	}
#if MMUOP_DEBUG > 1
	write_log (_T("mmu_op %04X PC=%08X\n"), opcode, m68k_getpc ());
#endif
	if ((opcode & 0xFE0) == 0x0500) {
		/* PFLUSH */
		regs.mmusr = 0;
#if MMUOP_DEBUG > 0
		write_log (_T("PFLUSH\n"));
#endif
		return;
	} else if ((opcode & 0x0FD8) == 0x0548) {
		if (currprefs.cpu_model < 68060) { /* PTEST not in 68060 */
			/* PTEST */
			int regno = opcode & 7;
			uae_u32 addr = m68k_areg(regs, regno);
			bool write = (opcode & 32) == 0;
			bool super = (regs.dfc & 4) != 0;
			bool data = (regs.dfc & 3) != 2;

			regs.mmusr = 0;
			if (fake_mmu_match_ttr(addr, super, data) != TTR_NO_MATCH) {
				regs.mmusr = MMU_MMUSR_T | MMU_MMUSR_R;
			}
			regs.mmusr |= addr & 0xfffff000;
#if MMUOP_DEBUG > 0
			write_log (_T("PTEST%c %08x\n"), write ? 'W' : 'R', addr);
#endif
			return;
		}
	} else if ((opcode & 0x0FB8) == 0x0588) {
		/* PLPA */
		if (currprefs.cpu_model == 68060) {
			int regno = opcode & 7;
			uae_u32 addr = m68k_areg (regs, regno);
			int write = (opcode & 0x40) == 0;
			bool data = (regs.dfc & 3) != 2;
			bool super = (regs.dfc & 4) != 0;

			if (fake_mmu_match_ttr(addr, super, data) == TTR_NO_MATCH) {
				m68k_areg (regs, regno) = addr;
			}
#if MMUOP_DEBUG > 0
			write_log (_T("PLPA\n"));
#endif
			return;
		}
	}
#if MMUOP_DEBUG > 0
	write_log (_T("Unknown MMU OP %04X\n"), opcode);
#endif
	m68k_setpc_normal (m68k_getpc () - 2);
	op_illg (opcode);
}

#endif

static void do_trace()
{
	if (regs.stopped) {
		return;
	}
	// need to store PC because of branch instructions
	regs.trace_pc = m68k_getpc();
	if (regs.t0 && !regs.t1 && currprefs.cpu_model >= 68020) {
		// this is obsolete
		return;
	}
	if (regs.t1) {
		activate_trace();
	}
}

static void int_request_do(bool i6)
{
	if (i6) {
		if (currprefs.cs_compatible == CP_DRACO || currprefs.cs_compatible == CP_CASABLANCA) {
#ifdef WITH_DRACO
			draco_ext_interrupt(true);
#endif
		} else {
			INTREQ_f(0x8000 | 0x2000);
		}
	} else {
		if (currprefs.cs_compatible == CP_DRACO || currprefs.cs_compatible == CP_CASABLANCA) {
#ifdef WITH_DRACO
			draco_ext_interrupt(false);
#endif
		} else {
			INTREQ_f(0x8000 | 0x0008);
		}
	}
}

static void check_uae_int_request(void)
{
	bool irq2 = false;
	bool irq6 = false;
	if (atomic_and(&uae_interrupt, 0)) {
		for (int i = 0; i < IRQ_SOURCE_MAX; i++) {
			if (!irq2 && uae_interrupts2[i]) {
				uae_atomic v = atomic_and(&uae_interrupts2[i], 0);
				if (v) {
					int_request_do(false);
					irq2 = true;
				}
			}
			if (!irq6 && uae_interrupts6[i]) {
				uae_atomic v = atomic_and(&uae_interrupts6[i], 0);
				if (v) {
					int_request_do(true);
					irq6 = true;
				}
			}
		}
	}
	if (uae_int_requested) {
		if (!irq2 && (uae_int_requested & 0x00ff)) {
			int_request_do(false);
			irq2 = true;
		}
		if (!irq6 && (uae_int_requested & 0xff00)) {
			int_request_do(true);
			irq6 = true;
		}
		if (uae_int_requested & 0xff0000) {
#ifdef WITH_PPC
			if (!cpuboard_is_ppcboard_irq()) {
#endif
				atomic_and(&uae_int_requested, ~0x010000);
#ifdef WITH_PPC
			}
#endif
		}
	}
	if (irq2 || irq6) {
		doint();
	}
}

void safe_interrupt_set(int num, int id, bool i6)
{
	if (!is_mainthread()) {
		set_special_exter(SPCFLAG_UAEINT);
		volatile uae_atomic *p;
		if (i6)
			p = &uae_interrupts6[num];
		else
			p = &uae_interrupts2[num];
		atomic_or(p, 1 << id);
		atomic_or(&uae_interrupt, 1);
	} else {
		if (currprefs.cs_compatible == CP_DRACO || currprefs.cs_compatible == CP_CASABLANCA) {
#ifdef WITH_DRACO
			draco_ext_interrupt(i6);
#endif
		} else {
			int inum = i6 ? 13 : 3;
			uae_u16 v = 1 << inum;
			if (currprefs.cpu_cycle_exact || currprefs.cpu_compatible) {
				INTREQ_INT(inum, 0);
			} else if (!(intreq & v)) {
				INTREQ_0(0x8000 | v);
			}
		}
	}
}

int cpu_sleep_millis(int ms)
{
	int ret = 0;
#ifdef WITH_PPC
	int state = ppc_state;
	if (state)
		uae_ppc_spinlock_release();
#endif
#if defined (WITH_X86) || defined (AMIBERRY)
//	if (x86_turbo_on) {
//		execute_other_cpu(read_processor_time() + vsynctimebase / 20);
//	} else {
		ret = sleep_millis_main(ms);
//	}
#endif
#ifdef WITH_PPC
	if (state)
		uae_ppc_spinlock_get();
#endif
	return ret;
}

#define PPC_HALTLOOP_SCANLINES 25
// ppc_cpu_idle
// 0 = busy
// 1-9 = wait, levels
// 10 = max wait

static bool haltloop_do(int vsynctimeline, frame_time_t rpt_end, int lines)
{
	int ovpos = vpos;
	while (lines-- >= 0) {
		ovpos = vpos;
		while (ovpos == vpos) {
			x_do_cycles(8 * CYCLE_UNIT);
			unset_special(SPCFLAG_UAEINT);
			check_uae_int_request();
#ifdef WITH_PPC
			ppc_interrupt(intlev());
			uae_ppc_execute_check();
#endif
			if (regs.spcflags & (SPCFLAG_BRK | SPCFLAG_MODE_CHANGE)) {
				if (regs.spcflags & SPCFLAG_BRK) {
					unset_special(SPCFLAG_BRK);
	#ifdef DEBUGGER
					if (debugging)
						debug();
	#endif
				}
				return true;
			}
		}

		// sync chipset with real time
		for (;;) {
			check_uae_int_request();
#ifdef WITH_PPC
			ppc_interrupt(intlev());
			uae_ppc_execute_check();
#endif
			if (event_wait)
				break;
			frame_time_t d = read_processor_time() - rpt_end;
			if (d < -2 * vsynctimeline || d >= 0)
				break;
		}
	}
	return false;
}

static bool haltloop()
{
#ifdef WITH_PPC
	if (regs.halted < 0) {
		int rpt_end = 0;
		int ovpos = vpos;

		while (regs.halted) {
			int vsynctimeline = static_cast<int>(vsynctimebase / (maxvpos_display + 1));
			int lines;
			frame_time_t rpt_scanline = read_processor_time();
			frame_time_t rpt_end = rpt_scanline + vsynctimeline;

			// See expansion handling.
			// Dialog must be opened from main thread.
			if (regs.halted == -2) {
				regs.halted = -1;
				notify_user (NUMSG_UAEBOOTROM_PPC);
			}

			if (currprefs.ppc_cpu_idle) {

				int maxlines = 100 - (currprefs.ppc_cpu_idle - 1) * 10;
				int i;

				event_wait = false;
				for (i = 0; i < ev_max; i++) {
					if (i == ev_audio)
						continue;
					if (!eventtab[i].active)
						continue;
					if (eventtab[i].evtime - currcycle < maxlines * maxhpos * CYCLE_UNIT)
						break;
				}
				if (currprefs.ppc_cpu_idle >= 10 || (i == ev_max && vpos > 0 && vpos < maxvpos - maxlines)) {
					cpu_sleep_millis(1);
				}
				check_uae_int_request();
				uae_ppc_execute_check();

				lines = static_cast<int>(read_processor_time() - rpt_scanline) / vsynctimeline + 1;

			} else {

				event_wait = true;
				lines = 0;

			}

			if (lines > maxvpos / 2)
				lines = maxvpos / 2;

			if (haltloop_do(vsynctimeline, rpt_end, lines))
				return true;

		}

	} else  {
#endif
		while (regs.halted) {
			static int prevvpos;
			if (vpos == 0 && prevvpos) {
				prevvpos = 0;
				cpu_sleep_millis(8);
			}
			if (vpos)
				prevvpos = 1;
			x_do_cycles(8 * CYCLE_UNIT);

			if (regs.spcflags) {
				if ((regs.spcflags & (SPCFLAG_BRK | SPCFLAG_MODE_CHANGE)))
					return true;
			}
		}
#ifdef WITH_PPC
	}
#endif

	return false;
}

#ifdef WITH_PPC
static bool uae_ppc_poll_check_halt()
{
	if (regs.halted) {
		if (haltloop())
			return true;
	}
	return false;
}
#endif


// check if interrupt active
static int time_for_interrupt()
{
	int ipl = get_ipl();
	if (ipl > regs.intmask || ipl == 7) {
		return ipl;
	}
	return 0;
}

// ipl check mid next memory cycle
void ipl_fetch_next_pre()
{
	ipl_fetch_next();
	regs.ipl_evt_pre = get_cycles();
	regs.ipl_evt_pre_mode = 1;
}

void ipl_fetch_now_pre()
{
	ipl_fetch_now();
	regs.ipl_evt_pre = get_cycles();
	regs.ipl_evt_pre_mode = 0;
}

// ipl check was early enough, interrupt possible after current instruction
void ipl_fetch_now()
{
	evt_t c = get_cycles();

	regs.ipl_evt = c;
	regs.ipl[0] = regs.ipl_pin;
	regs.ipl[1] = 0;
}

// ipl check max 4 cycles before end of instruction.
// interrupt starts after current instruction if IPL was changed earlier.
// if not early enough: interrupt starts after following instruction.
void ipl_fetch_next()
{
	evt_t c = get_cycles();

	evt_t cd = c - regs.ipl_pin_change_evt;
	evt_t cdp = c - regs.ipl_pin_change_evt_p;
	if (cd >= cpuipldelay4) {
		regs.ipl[0] = regs.ipl_pin;
		regs.ipl[1] = 0;
	} else if (cdp >= cpuipldelay2) {
		regs.ipl[0] = regs.ipl_pin_p;
		regs.ipl[1] = 0;
	} else {
		regs.ipl[1] = regs.ipl_pin;
	}
}

void intlev_load()
{
	if (m68k_accurate_ipl) {
		ipl_fetch_now();
	}
	doint();
}

static void update_ipl(int ipl)
{
	evt_t c = get_cycles();
	regs.ipl_pin_change_evt_p = regs.ipl_pin_change_evt;
	regs.ipl_pin_p = regs.ipl_pin;
	regs.ipl_pin_change_evt = c;
	regs.ipl_pin = ipl;
	if (m68k_accurate_ipl) {
		// check if 68000/010 interrupt was detected mid memory access,
		// 2 cycles from start of memory cycle
		if (ipl > 0 && c == regs.ipl_evt_pre + cpuipldelay2) {
			if (regs.ipl_evt_pre_mode) {
				ipl_fetch_next();
			} else {
				ipl_fetch_now();
			}
		}
	}
#ifdef DEBUGGER
	if (debug_dma) {
		record_dma_ipl();
	}
#endif
}

static void doint_delayed(uae_u32 v)
{
	update_ipl(v);
}

void doint()
{
#ifdef WITH_PPC
	if (ppc_state) {
		if (!ppc_interrupt(intlev()))
			return;
	}
#endif
	int ipl = intlev();

	if (regs.ipl_pin != ipl) {

		// Paula does low to high IPL changes about 1.5 CPU clocks later than high to low.
		// -> CPU detects IPL change 1 CCK later if any IPL pin has high to low transition.
		// (In real world IPL is active low and delay is added if 0 to 1 transition)
		if (currprefs.cs_ipldelay && m68k_accurate_ipl && regs.ipl_pin >= 0 && ipl >= 0 && (
			((regs.ipl_pin & 1) && !(ipl & 1)) ||
			((regs.ipl_pin & 2) && !(ipl & 2)) ||
			((regs.ipl_pin & 4) && !(ipl & 4))
			)) {
				event2_newevent_xx(-1, CYCLE_UNIT, ipl, doint_delayed);
				return;
		}

		update_ipl(ipl);
	}

	if (m68k_interrupt_delay) {
		if (!m68k_accurate_ipl && regs.ipl_pin > regs.intmask) {
			set_special(SPCFLAG_INT);
		}
		return;
	}

	if (regs.ipl_pin > regs.intmask || currprefs.cachesize) {
		if (currprefs.cpu_compatible && currprefs.cpu_model < 68020)
			set_special(SPCFLAG_INT);
		else
			set_special(SPCFLAG_DOINT);
	}
}

static void check_debugger()
{
	if (regs.spcflags & SPCFLAG_BRK) {
		unset_special(SPCFLAG_BRK);
#ifdef DEBUGGER
		if (debugging) {
			debug();
		}
#endif
	}
}

static void debug_cpu_stop()
{
#ifdef DEBUGGER
	record_dma_event(DMA_EVENT_CPUSTOP);
	if (time_for_interrupt()) {
		record_dma_event(DMA_EVENT_CPUSTOPIPL);
	}
#endif
}

static int do_specialties (int cycles)
{
	uaecptr pc = m68k_getpc();
	uae_atomic spcflags = regs.spcflags;

	if (spcflags & SPCFLAG_MODE_CHANGE)
		return 1;
	
	while (spcflags & SPCFLAG_CPUINRESET) {
		cpu_halt_clear();
		x_do_cycles(4 * CYCLE_UNIT);
		spcflags = regs.spcflags;
		if (!(spcflags & SPCFLAG_CPUINRESET) || (spcflags & SPCFLAG_BRK) || (spcflags & SPCFLAG_MODE_CHANGE)) {
			break;
		}
	}

	if (spcflags & SPCFLAG_CHECK) {
		if (regs.halted) {
			if (regs.halted == CPU_HALT_ACCELERATOR_CPU_FALLBACK) {
				return 1;
			}
			unset_special(SPCFLAG_CHECK);
			if (haltloop())
				return 1;
		}
		if (m68k_reset_delay) {
			int vsynccnt = 60;
			int vsyncstate = -1;
			while (vsynccnt > 0 && !quit_program) {
				x_do_cycles(8 * CYCLE_UNIT);
				spcflags = regs.spcflags;
				if (vsync_counter != vsyncstate) {
					vsyncstate = vsync_counter;
					vsynccnt--;
				}
			}
		}
		m68k_reset_delay = false;
		unset_special(SPCFLAG_CHECK);
	}

#ifdef ACTION_REPLAY
#ifdef ACTION_REPLAY_HRTMON
	if ((spcflags & SPCFLAG_ACTION_REPLAY) && hrtmon_flag != ACTION_REPLAY_INACTIVE) {
		int isinhrt = pc >= hrtmem_start && pc < hrtmem_start + hrtmem_size;
		/* exit from HRTMon? */
		if (hrtmon_flag == ACTION_REPLAY_ACTIVE && !isinhrt)
			hrtmon_hide();
		/* HRTMon breakpoint? (not via IRQ7) */
		if (hrtmon_flag == ACTION_REPLAY_IDLE && isinhrt)
			hrtmon_breakenter();
		if (hrtmon_flag == ACTION_REPLAY_ACTIVATE)
			hrtmon_enter();
	}
#endif
	if ((spcflags & SPCFLAG_ACTION_REPLAY) && action_replay_flag != ACTION_REPLAY_INACTIVE) {
		/*if (action_replay_flag == ACTION_REPLAY_ACTIVE && !is_ar_pc_in_rom ())*/
		/*	write_log (_T("PC:%p\n"), m68k_getpc ());*/

		if (action_replay_flag == ACTION_REPLAY_ACTIVATE || action_replay_flag == ACTION_REPLAY_DORESET)
			action_replay_enter();
		if ((action_replay_flag == ACTION_REPLAY_HIDE || action_replay_flag == ACTION_REPLAY_ACTIVE) && !is_ar_pc_in_rom()) {
			action_replay_hide();
			unset_special (SPCFLAG_ACTION_REPLAY);
		}
		if (action_replay_flag == ACTION_REPLAY_WAIT_PC) {
			/*write_log (_T("Waiting for PC: %p, current PC= %p\n"), wait_for_pc, m68k_getpc ());*/
			if (pc == wait_for_pc) {
				action_replay_flag = ACTION_REPLAY_ACTIVATE; /* Activate after next instruction. */
			}
		}
	}
#endif

#ifdef JIT
	if (spcflags & SPCFLAG_END_COMPILE) {
		unset_special(SPCFLAG_END_COMPILE);
	}
#endif

	while ((spcflags & SPCFLAG_BLTNASTY) && dmaen (DMA_BLITTER) && cycles > 0 && ((currprefs.waiting_blits && currprefs.cpu_model >= 68020) || !currprefs.blitter_cycle_exact)) {
		int c = blitnasty();
		if (c < 0) {
			break;
		} else if (c > 0) {
			cycles -= c * CYCLE_UNIT * 2;
			if (cycles < CYCLE_UNIT)
				cycles = 0;
		} else {
			c = 4;
		}
		x_do_cycles(c * CYCLE_UNIT);
		spcflags = regs.spcflags;
#ifdef WITH_PPC
		if (ppc_state)  {
			if (uae_ppc_poll_check_halt())
				return true;
			uae_ppc_execute_check();
		}
#endif
	}

	if (spcflags & SPCFLAG_MMURESTART) {
		// can't have interrupt when 040/060 CPU reruns faulted instruction
		unset_special(SPCFLAG_MMURESTART);

		if (spcflags & SPCFLAG_TRACE) {
			do_trace();
		}
	} else {

		if (spcflags & SPCFLAG_DOTRACE) {
			Exception(9);
		}

		if (spcflags & SPCFLAG_TRAP) {
			unset_special (SPCFLAG_TRAP);
			Exception(3);
		}

		if (spcflags & SPCFLAG_TRACE) {
			do_trace();
		}

		if (spcflags & SPCFLAG_UAEINT) {
			check_uae_int_request();
			unset_special(SPCFLAG_UAEINT);
		}

		if (m68k_interrupt_delay) {
			int ipl = time_for_interrupt();
			if (ipl) {
				unset_special(SPCFLAG_INT);
				do_interrupt(ipl);
			}
		} else {
			if (spcflags & SPCFLAG_INT) {
				int intr = intlev();
				unset_special (SPCFLAG_INT | SPCFLAG_DOINT);
				if (intr > regs.intmask || (intr == 7 && intr > regs.lastipl)) {
					do_interrupt(intr);
				}
				regs.lastipl = intr;
			}
		}

		if (spcflags & SPCFLAG_DOINT) {
			unset_special(SPCFLAG_DOINT);
			set_special(SPCFLAG_INT);
		}
	}

	if (spcflags & SPCFLAG_BRK) {
		unset_special(SPCFLAG_BRK);
#ifdef DEBUGGER
		if (debugging) {
			debug();
		}
#endif
	}

	return 0;
}

//static uae_u32 pcs[1000];

#if DEBUG_CD32CDTVIO

static uae_u32 cd32nextpc, cd32request;

static void out_cd32io2 (void)
{
	uae_u32 request = cd32request;
	write_log (_T("%08x returned\n"), request);
	//write_log (_T("ACTUAL=%d ERROR=%d\n"), get_long (request + 32), get_byte (request + 31));
	cd32nextpc = 0;
	cd32request = 0;
}

static void out_cd32io (uae_u32 pc)
{
	TCHAR out[100];
	int ioreq = 0;
	uae_u32 request = m68k_areg (regs, 1);

	if (pc == cd32nextpc) {
		out_cd32io2 ();
		return;
	}
	out[0] = 0;
	switch (pc)
	{
	case 0xe57cc0:
	case 0xf04c34:
		_stprintf (out, _T("opendevice"));
		break;
	case 0xe57ce6:
	case 0xf04c56:
		_stprintf (out, _T("closedevice"));
		break;
	case 0xe57e44:
	case 0xf04f2c:
		_stprintf (out, _T("beginio"));
		ioreq = 1;
		break;
	case 0xe57ef2:
	case 0xf0500e:
		_stprintf (out, _T("abortio"));
		ioreq = -1;
		break;
	}
	if (out[0] == 0)
		return;
	if (cd32request)
		write_log (_T("old request still not returned!\n"));
	cd32request = request;
	cd32nextpc = get_long (m68k_areg (regs, 7));
	write_log (_T("%s A1=%08X\n"), out, request);
	if (ioreq) {
		static int cnt = 0;
		int cmd = get_word (request + 28);
#if 0
		if (cmd == 33) {
			uaecptr data = get_long (request + 40);
			write_log (_T("CD_CONFIG:\n"));
			for (int i = 0; i < 16; i++) {
				write_log (_T("%08X=%08X\n"), get_long (data), get_long (data + 4));
				data += 8;
			}
		}
#endif
#if 0
		if (cmd == 37) {
			cnt--;
			if (cnt <= 0)
				activate_debugger ();
		}
#endif
		write_log (_T("CMD=%d DATA=%08X LEN=%d %OFF=%d PC=%x\n"),
			cmd, get_long (request + 40),
			get_long (request + 36), get_long (request + 44), M68K_GETPC);
	}
	if (ioreq < 0)
		;//activate_debugger ();
}

#endif

#ifndef CPUEMU_11

static void m68k_run_1 (void)
{
}

#else

/* It's really sad to have two almost identical functions for this, but we
do it all for performance... :(
This version emulates 68000's prefetch "cache" */
static void m68k_run_1 ()
{
	struct regstruct *r = &regs;
	bool exit = false;

	while (!exit) {
		check_debugger();
		TRY (prb) {
			while (!exit) {
				r->opcode = r->ir;

				count_instr (r->opcode);

#if DEBUG_CD32CDTVIO
				out_cd32io (m68k_getpc ());
#endif

#if 0
				int pc = m68k_getpc ();
				if (pc == 0xdff002)
					write_log (_T("hip\n"));
				if (pc != pcs[0] && (pc < 0xd00000 || pc > 0x1000000)) {
					memmove (pcs + 1, pcs, 998 * 4);
					pcs[0] = pc;
					//write_log (_T("%08X-%04X "), pc, r->opcode);
				}
#endif
#ifdef DEBUGGER
				if (debug_opcode_watch) {
					debug_trainer_match();
				}
#endif
				r->instruction_pc = m68k_getpc ();
				cpu_cycles = (*cpufunctbl[r->opcode])(r->opcode) & 0xffff;
				if (!regs.loop_mode)
					regs.ird = regs.opcode;
				cpu_cycles = adjust_cycles (cpu_cycles);
				do_cycles(cpu_cycles);
				regs.instruction_cnt++;
				if (r->spcflags || regs.ipl[0] > 0) {
					if (do_specialties (cpu_cycles))
						exit = true;
				}
				regs.ipl[0] = regs.ipl_pin;
				if (!currprefs.cpu_compatible || (currprefs.cpu_cycle_exact && currprefs.cpu_model <= 68010))
					exit = true;
			}
		} CATCH (prb) {
			bus_error();
			if (r->spcflags) {
				if (do_specialties(cpu_cycles))
					exit = true;
			}
			regs.ipl[0] = regs.ipl[1] = regs.ipl_pin;
		} ENDTRY
	}
}

#endif /* CPUEMU_11 */

#ifndef CPUEMU_13

static void m68k_run_1_ce (void)
{
}

#else

/* cycle-exact m68k_run () */

static void m68k_run_1_ce ()
{
	struct regstruct *r = &regs;
	bool first = true;
	bool exit = false;

	while (!exit) {
		check_debugger();
		TRY (prb) {
			if (first) {
				if (cpu_tracer < 0) {
					memcpy (&r->regs, &cputrace.regs, 16 * sizeof (uae_u32));
					r->ir = cputrace.ir;
					r->irc = cputrace.irc;
					r->ird = cputrace.ird;
					r->sr = cputrace.sr;
					r->usp = cputrace.usp;
					r->isp = cputrace.isp;
					r->intmask = cputrace.intmask;
					r->stopped = cputrace.stopped;
					r->read_buffer = cputrace.read_buffer;
					r->write_buffer = cputrace.write_buffer;
					m68k_setpc (cputrace.pc);
					if (!r->stopped) {
						if (cputrace.state > 1) {
							write_log (_T("CPU TRACE: EXCEPTION %d\n"), cputrace.state);
							Exception (cputrace.state);
						} else if (cputrace.state == 1) {
							write_log (_T("CPU TRACE: %04X\n"), cputrace.opcode);
							(*cpufunctbl_noret[cputrace.opcode])(cputrace.opcode);
						}
					} else {
						write_log (_T("CPU TRACE: STOPPED\n"));
					}
					set_cpu_tracer (false);
					goto cont;
				}
				set_cpu_tracer (false);
				first = false;
			}

			while (!exit) {
				r->opcode = r->ir;

#if DEBUG_CD32CDTVIO
				out_cd32io (m68k_getpc ());
#endif
				if (cpu_tracer) {
					memcpy (&cputrace.regs, &r->regs, 16 * sizeof (uae_u32));
					cputrace.opcode = r->opcode;
					cputrace.ir = r->ir;
					cputrace.irc = r->irc;
					cputrace.ird = r->ird;
					cputrace.sr = r->sr;
					cputrace.usp = r->usp;
					cputrace.isp = r->isp;
					cputrace.intmask = r->intmask;
					cputrace.stopped = r->stopped;
					cputrace.read_buffer = r->read_buffer;
					cputrace.write_buffer = r->write_buffer;
					cputrace.state = 1;
					cputrace.pc = m68k_getpc ();
					cputrace.startcycles = get_cycles ();
					cputrace.memoryoffset = 0;
					cputrace.cyclecounter = cputrace.cyclecounter_pre = cputrace.cyclecounter_post = 0;
					cputrace.readcounter = cputrace.writecounter = 0;
				}

				if (inputrecord_debug & 4) {
					if (input_record > 0)
						inprec_recorddebug_cpu(1, r->opcode);
					else if (input_play > 0)
						inprec_playdebug_cpu(1, r->opcode);
				}

#ifdef DEBUGGER
				if (debug_opcode_watch) {
					debug_trainer_match();
				}
#endif

				r->instruction_pc = m68k_getpc ();
#ifdef DEBUGGER
				if (debug_dma) {
					record_dma_event_data(DMA_EVENT_CPUINS, r->opcode);
				}
#endif

				(*cpufunctbl_noret[r->opcode])(r->opcode);
				if (!regs.loop_mode)
					regs.ird = regs.opcode;
				regs.instruction_cnt++;
				wait_memory_cycles();
				if (cpu_tracer) {
					cputrace.state = 0;
				}
cont:
				if (cputrace.needendcycles) {
					cputrace.needendcycles = 0;
					write_log(_T("STARTCYCLES=%016llx ENDCYCLES=%016llx\n"), cputrace.startcycles, get_cycles());
#ifdef DEBUGGER
					log_dma_record ();
#endif
				}

				if (r->spcflags || regs.ipl[0] > 0) {
					if (do_specialties (0))
						exit = true;
				}

				regs.ipl[0] = regs.ipl[1];
				regs.ipl[1] = 0;

				if (!currprefs.cpu_cycle_exact || currprefs.cpu_model > 68010)
					exit = true;
			}
		} CATCH (prb) {
			bus_error();
			if (r->spcflags) {
				if (do_specialties(0))
					exit = true;
			}
		} ENDTRY
	}
}

#endif

#if defined(CPUEMU_20) && defined(JIT)
// emulate simple prefetch
static uae_u16 get_word_020_prefetchf (uae_u32 pc)
{
	if (pc == regs.prefetch020addr) {
		uae_u16 v = regs.prefetch020[0];
		regs.prefetch020[0] = regs.prefetch020[1];
		regs.prefetch020[1] = regs.prefetch020[2];
		regs.prefetch020[2] = x_get_word (pc + 6);
		regs.prefetch020addr += 2;
		return v;
	} else if (pc == regs.prefetch020addr + 2) {
		uae_u16 v = regs.prefetch020[1];
		regs.prefetch020[0] = regs.prefetch020[2];
		regs.prefetch020[1] = x_get_word (pc + 4);
		regs.prefetch020[2] = x_get_word (pc + 6);
		regs.prefetch020addr = pc + 2;
		return v;
	} else if (pc == regs.prefetch020addr + 4) {
		uae_u16 v = regs.prefetch020[2];
		regs.prefetch020[0] = x_get_word (pc + 2);
		regs.prefetch020[1] = x_get_word (pc + 4);
		regs.prefetch020[2] = x_get_word (pc + 6);
		regs.prefetch020addr = pc + 2;
		return v;
	} else {
		regs.prefetch020addr = pc + 2;
		regs.prefetch020[0] = x_get_word (pc + 2);
		regs.prefetch020[1] = x_get_word (pc + 4);
		regs.prefetch020[2] = x_get_word (pc + 6);
		return x_get_word (pc);
	}
}
#endif

#ifdef WITH_THREADED_CPU
static volatile int cpu_thread_active;
static uae_sem_t cpu_in_sema, cpu_out_sema, cpu_wakeup_sema;

static volatile int cpu_thread_ilvl;
static volatile uae_u32 cpu_thread_indirect_mode;
static volatile uae_u32 cpu_thread_indirect_addr;
static volatile uae_u32 cpu_thread_indirect_val;
static volatile uae_u32 cpu_thread_indirect_size;
static volatile uae_u32 cpu_thread_reset;
static SDL_Thread* cpu_thread;
static SDL_threadID cpu_thread_tid;

static bool m68k_cs_initialized;

static int do_specialties_thread()
{
	uae_atomic spcflags = regs.spcflags;

	if (spcflags & SPCFLAG_MODE_CHANGE)
		return 1;

#ifdef JIT
	if (currprefs.cachesize) {
		unset_special(SPCFLAG_END_COMPILE);
	}
#endif

	if (spcflags & SPCFLAG_DOTRACE)
		Exception(9);

	if (spcflags & SPCFLAG_TRAP) {
		unset_special(SPCFLAG_TRAP);
		Exception(3);
	}

	if (spcflags & SPCFLAG_TRACE)
		do_trace();

	for (;;) {

		if (spcflags & (SPCFLAG_BRK | SPCFLAG_MODE_CHANGE)) {
			return 1;
		}

		int ilvl = cpu_thread_ilvl;
		if (ilvl > 0 && (ilvl > regs.intmask || ilvl == 7)) {
			do_interrupt(ilvl);
		}

		break;
	}

	return 0;
}

static void init_cpu_thread()
{
	if (!currprefs.cpu_thread)
		return;
	if (m68k_cs_initialized)
		return;
	uae_sem_init(&cpu_in_sema, 0, 0);
	uae_sem_init(&cpu_out_sema, 0, 0);
	uae_sem_init(&cpu_wakeup_sema, 0, 0);
	m68k_cs_initialized = true;
}

extern addrbank *thread_mem_banks[MEMORY_BANKS];

uae_u32 process_cpu_indirect_memory_read(uae_u32 addr, int size)
{
	// Do direct access if call is from filesystem etc thread
	if (cpu_thread_tid != uae_thread_get_id(nullptr)) {
		uae_u32 data = 0;
		addrbank *ab = thread_mem_banks[bankindex(addr)];
		switch (size)
		{
		case 0:
			data = ab->bget(addr) & 0xff;
			break;
		case 1:
			data = ab->wget(addr) & 0xffff;
			break;
		case 2:
			data = ab->lget(addr);
			break;
		}
		return data;
	}

	cpu_thread_indirect_mode = 2;
	cpu_thread_indirect_addr = addr;
	cpu_thread_indirect_size = size;
	uae_sem_post(&cpu_out_sema);
	uae_sem_wait(&cpu_in_sema);
	cpu_thread_indirect_mode = 0xfe;
	return cpu_thread_indirect_val;
}

void process_cpu_indirect_memory_write(uae_u32 addr, uae_u32 data, int size)
{
	if (cpu_thread_tid != uae_thread_get_id(nullptr)) {
		addrbank *ab = thread_mem_banks[bankindex(addr)];
		switch (size)
		{
		case 0:
			ab->bput(addr, data & 0xff);
			break;
		case 1:
			ab->wput(addr, data & 0xffff);
			break;
		case 2:
			ab->lput(addr, data);
			break;
		}
		return;
	}
	cpu_thread_indirect_mode = 1;
	cpu_thread_indirect_addr = addr;
	cpu_thread_indirect_size = size;
	cpu_thread_indirect_val = data;
	uae_sem_post(&cpu_out_sema);
	uae_sem_wait(&cpu_in_sema);
	cpu_thread_indirect_mode = 0xff;
}

static void run_cpu_thread(int (*f)(void *))
{
	uae_u32 framecnt = -1;
	int vp = 0;
	int intlev_prev = 0;

	cpu_thread_active = 0;
	uae_sem_init(&cpu_in_sema, 0, 0);
	uae_sem_init(&cpu_out_sema, 0, 0);
	uae_sem_init(&cpu_wakeup_sema, 0, 0);

	if (!uae_start_thread(_T("cpu"), f, nullptr, &cpu_thread))
		return;
	while (!cpu_thread_active) {
		sleep_millis(1);
	}

	while (!(regs.spcflags & SPCFLAG_MODE_CHANGE)) {
		int maxperloop = 10;

		while (!uae_sem_trywait(&cpu_out_sema)) {
			uae_u32 addr, data, size, mode;

			addr = cpu_thread_indirect_addr;
			data = cpu_thread_indirect_val;
			size = cpu_thread_indirect_size;
			mode = cpu_thread_indirect_mode;

			switch(mode)
			{
				case 1:
				{
					addrbank *ab = thread_mem_banks[bankindex(addr)];
					switch (size)
					{
					case 0:
						ab->bput(addr, data & 0xff);
						break;
					case 1:
						ab->wput(addr, data & 0xffff);
						break;
					case 2:
						ab->lput(addr, data);
						break;
					}
					uae_sem_post(&cpu_in_sema);
					break;
				}
				case 2:
				{
					addrbank *ab = thread_mem_banks[bankindex(addr)];
					switch (size)
					{
					case 0:
						data = ab->bget(addr) & 0xff;
						break;
					case 1:
						data = ab->wget(addr) & 0xffff;
						break;
					case 2:
						data = ab->lget(addr);
						break;
					}
					cpu_thread_indirect_val = data;
					uae_sem_post(&cpu_in_sema);
					break;
				}
				default:
					write_log(_T("cpu_thread_indirect_mode=%08x!\n"), mode);
					break;
			}

			if (maxperloop-- < 0)
				break;
		}

		if (framecnt != vsync_counter) {
			framecnt = vsync_counter;
		}

		if (cpu_thread_reset) {
			bool hardreset = cpu_thread_reset & 2;
			bool keyboardreset = cpu_thread_reset & 4;
			custom_reset(hardreset, keyboardreset);
			cpu_thread_reset = 0;
			uae_sem_post(&cpu_in_sema);
		}

		if (regs.spcflags & SPCFLAG_BRK) {
			unset_special(SPCFLAG_BRK);
#ifdef DEBUGGER
			if (debugging) {
				debug();
			}
#endif
		}

		if (vp == vpos) {

			do_cycles((maxhpos / 2) * CYCLE_UNIT);

			check_uae_int_request();
			if (regs.spcflags & (SPCFLAG_INT | SPCFLAG_DOINT)) {
				int intr = intlev();
				unset_special(SPCFLAG_INT | SPCFLAG_DOINT);
				if (intr > 0) {
					cpu_thread_ilvl = intr;
					cycles_do_special();
					uae_sem_post(&cpu_wakeup_sema);
				} else {
					cpu_thread_ilvl = 0;
				}
			}
			continue;
		}

		frame_time_t next = vsyncmintimepre + (vsynctimebase * vpos / (maxvpos + 1));
		frame_time_t c = read_processor_time();
		if (next - c > 0 && next - c < vsyncmaxtime * 2)
			continue;

		vp = vpos;

	}

	while (cpu_thread_active) {
		uae_sem_post(&cpu_in_sema);
		uae_sem_post(&cpu_wakeup_sema);
		sleep_millis(1);
	}

}

#endif

static void custom_reset_cpu(bool hardreset, bool keyboardreset)
{
#ifdef WITH_THREADED_CPU
	if (cpu_thread_tid != uae_thread_get_id(cpu_thread)) {
		custom_reset(hardreset, keyboardreset);
		return;
	}
	cpu_thread_reset = 1 | (hardreset ? 2 : 0) | (keyboardreset ? 4 : 0);
	uae_sem_post(&cpu_wakeup_sema);
	uae_sem_wait(&cpu_in_sema);
#else
	custom_reset(hardreset, keyboardreset);
#endif
}

#ifdef JIT  /* Completely different run_2 replacement */

#ifdef CPU_AARCH64 // Used by the AARCH64 JIT implementation
void execute_exception(uae_u32 cycles)
{
	countdown -= cycles;
	Exception_cpu(regs.jit_exception);
	regs.jit_exception = 0;
	cpu_cycles = adjust_cycles(4 * CYCLE_UNIT / 2);
	do_cycles(cpu_cycles);
	// after leaving this function, we fall back to execute_normal()
}
#endif

void do_nothing (void)
{
	if (!currprefs.cpu_thread) {
		/* What did you expect this to do? */
		do_cycles (0);
		/* I bet you didn't expect *that* ;-) */
	}
}

static uae_u32 get_jit_opcode(void)
{
	uae_u32 opcode;
	if (currprefs.cpu_compatible) {
		opcode = get_word_020_prefetchf(m68k_getpc());
#ifdef HAVE_GET_WORD_UNSWAPPED
		opcode = do_byteswap_16(opcode);
#endif
	} else {
#ifdef HAVE_GET_WORD_UNSWAPPED
		opcode = do_get_mem_word_unswapped((uae_u16 *)get_real_address(m68k_getpc()));
#else
		opcode = x_get_iword(0);
#endif
	}
	return opcode;
}

void exec_nostats (void)
{
	struct regstruct *r = &regs;

	for (;;)
	{
		r->opcode = get_jit_opcode();

		(*cpufunctbl[r->opcode])(r->opcode);

		cpu_cycles = 4 * CYCLE_UNIT; // adjust_cycles(cpu_cycles);

		if (!currprefs.cpu_thread) {
			do_cycles (cpu_cycles);

#ifdef WITH_PPC
			if (ppc_state)
				ppc_interrupt(intlev());
#endif
		}

		if (end_block(r->opcode) || r->spcflags || uae_int_requested)
			return; /* We will deal with the spcflags in the caller */
	}
}

void execute_normal(void)
{
	struct regstruct *r = &regs;
	int blocklen;
	cpu_history pc_hist[MAXRUN];
	int total_cycles;

	if (check_for_cache_miss ())
		return;

	total_cycles = 0;
	blocklen = 0;
	start_pc_p = r->pc_oldp;
	start_pc = r->pc;
	for (;;) {
		/* Take note: This is the do-it-normal loop */
		r->opcode = get_jit_opcode();

		special_mem = special_mem_default;
		pc_hist[blocklen].location = (uae_u16*)r->pc_p;

		(*cpufunctbl[r->opcode])(r->opcode);

		cpu_cycles = 4 * CYCLE_UNIT;

//		cpu_cycles = adjust_cycles(cpu_cycles);
		if (!currprefs.cpu_thread) {
			do_cycles (cpu_cycles);
		}
		total_cycles += cpu_cycles;

		pc_hist[blocklen].specmem = special_mem;
		blocklen++;
		if (end_block (r->opcode) || blocklen >= MAXRUN || r->spcflags || uae_int_requested) {
			compile_block (pc_hist, blocklen, total_cycles);
			return; /* We will deal with the spcflags in the caller */
		}
		/* No need to check regs.spcflags, because if they were set,
		we'd have ended up inside that "if" */

#ifdef WITH_PPC
		if (ppc_state)
			ppc_interrupt(intlev());
#endif
	}
}

typedef void compiled_handler (void);

#ifdef WITH_THREADED_CPU
static int cpu_thread_run_jit(void *v)
{
	cpu_thread_tid = uae_thread_get_id(cpu_thread);
	cpu_thread_active = 1;
#ifdef USE_STRUCTURED_EXCEPTION_HANDLING
	__try
#endif
	{
		for (;;) {
			((compiled_handler*)(pushall_call_handler))();
			/* Whenever we return from that, we should check spcflags */
			if (regs.spcflags || cpu_thread_ilvl > 0) {
				if (do_specialties_thread()) {
					break;
				}
			}
		}
	}
#ifdef USE_STRUCTURED_EXCEPTION_HANDLING
#ifdef JIT
	__except (EvalException(GetExceptionInformation()))
#else
	__except (DummyException(GetExceptionInformation(), GetExceptionCode()))
#endif
	{
		// EvalException does the good stuff...
	}
#endif
	cpu_thread_active = 0;
	return 0;
}
#endif

static void m68k_run_jit(void)
{
#ifdef WITH_THREADED_CPU
	if (currprefs.cpu_thread) {
		run_cpu_thread(cpu_thread_run_jit);
		return;
	}
#endif

	if (regs.spcflags) {
		if (do_specialties(0)) {
			return;
		}
	}

	for (;;) {
#ifdef USE_STRUCTURED_EXCEPTION_HANDLING
		__try {
#endif
			for (;;) {
				((compiled_handler*)(pushall_call_handler))();
				/* Whenever we return from that, we should check spcflags */
				check_uae_int_request();
				if (regs.spcflags) {
					if (do_specialties(0)) {
						STOPTRY;
						return;
					}
				}
				// If T0, T1 or M got set: run normal emulation loop
				if (regs.t0 || regs.t1 || regs.m) {
					flush_icache(3);
					struct regstruct *r = &regs;
					bool exit = false;
					check_debugger();
					while (!exit && (regs.t0 || regs.t1 || regs.m)) {
						r->instruction_pc = m68k_getpc();
						r->opcode = x_get_iword(0);
						(*cpufunctbl[r->opcode])(r->opcode);
						count_instr(r->opcode);
						do_cycles(4 * CYCLE_UNIT);
						if (r->spcflags) {
							if (do_specialties(cpu_cycles))
								exit = true;
						}
					}
					unset_special(SPCFLAG_END_COMPILE);
				}
			}
#ifdef USE_STRUCTURED_EXCEPTION_HANDLING
		} __except (EvalException(GetExceptionInformation())) {
			// Something very bad happened, generate fake bus error exception
			// Either emulation continues normally or crashes.
			// Without this it would have crashed in any case..
			uaecptr pc = M68K_GETPC;
			write_log(_T("Unhandled JIT exception! PC=%08x\n"), pc);
			if (pc & 1)
				Exception(3);
			else
				Exception(2);
		}
#endif
	}

}
#endif /* JIT */

#ifndef CPUEMU_0

static void m68k_run_2 (void)
{
}

#else

static void opcodedebug (uae_u32 pc, uae_u16 opcode, bool full)
{
	struct mnemolookup *lookup;
	struct instr *dp;
	uae_u32 addr;
	int fault;

	if (cpufunctbl[opcode] == op_illg_1)
		opcode = 0x4AFC;
	dp = table68k + opcode;
	for (lookup = lookuptab;lookup->mnemo != dp->mnemo; lookup++)
		;
	fault = 0;
	TRY(prb) {
		addr = mmu_translate (pc, 0, (regs.mmu_ssw & 4) ? true : false, false, false, sz_word);
	} CATCH (prb) {
		fault = 1;
	} ENDTRY
	if (!fault) {
#ifdef DEBUGGER
		TCHAR buf[100];
		if (full)
			write_log (_T("mmufixup=%d %04x %04x\n"), mmufixup[0].reg, regs.wb3_status, regs.mmu_ssw);
		m68k_disasm_2(buf, sizeof buf / sizeof (TCHAR), addr, nullptr, 0, nullptr, 1, nullptr, nullptr, 0xffffffff, 0);
		write_log (_T("%s\n"), buf);
		if (full)
			m68k_dumpstate(nullptr, 0xffffffff);
#endif
	}
}

static void check_halt()
{
	if (regs.halted)
		do_specialties (0);
}

void cpu_inreset()
{
	set_special(SPCFLAG_CPUINRESET);
	regs.s = 1;
	regs.intmask = 7;
	MakeSR();
}

void cpu_halt(int id)
{
	// id < 0: m68k halted, PPC active.
	// id > 0: emulation halted.
	if (!regs.halted) {
		write_log(_T("CPU halted: reason = %d PC=%08x\n"), id, M68K_GETPC);
		if (currprefs.crash_auto_reset) {
			write_log(_T("Forcing hard reset\n"));
			uae_reset(true, false);
			quit_program = -quit_program;
			set_special(SPCFLAG_BRK | SPCFLAG_MODE_CHANGE);
			return;
		}
		regs.halted = id;
		gui_data.cpu_halted = id;
		gui_led(LED_CPU, 0, -1);
		if (id >= 0) {
			regs.intmask = 7;
			MakeSR();
			audio_deactivate();
#ifdef DEBUGGER
			if (debugging)
				activate_debugger();
#endif
		}
	}
	set_special(SPCFLAG_CHECK);
}

#ifdef CPUEMU_33

/* MMU 68060  */
static void m68k_run_mmu060 ()
{
	struct flag_struct f;
	int halt = 0;

	check_halt();
	while (!halt) {
		check_debugger();
		TRY (prb) {
			for (;;) {
#if defined(CPU_i386) || defined(CPU_x86_64)
				f.cznv = regflags.cznv;
#else // we assume CPU_arm or CPU_AARCH64 here
				f.nzcv = regflags.nzcv;
#endif
				f.x = regflags.x;
				regs.instruction_pc = m68k_getpc ();

				do_cycles (cpu_cycles);

				mmu_opcode = -1;
				mmu060_state = 0;
				mmu_opcode = regs.opcode = x_prefetch (0);
				mmu060_state = 1;

				count_instr (regs.opcode);
				cpu_cycles = (*cpufunctbl[regs.opcode])(regs.opcode);

				cpu_cycles = adjust_cycles(cpu_cycles);
				regs.instruction_cnt++;

				if (regs.spcflags) {
					if (do_specialties(cpu_cycles)) {
						STOPTRY;
						return;
					}
				}
			}
		} CATCH (prb) {
			m68k_setpci (regs.instruction_pc);
#if defined(CPU_i386) || defined(CPU_x86_64)
				regflags.cznv = f.cznv;
#else // we assume CPU_arm or CPU_AARCH64 here
				regflags.nzcv = f.nzcv;
#endif
			regflags.x = f.x;
			cpu_restore_fixup();
			TRY (prb2) {
				Exception (prb);
			} CATCH (prb2) {
				halt = 1;
			} ENDTRY
		} ENDTRY
	}
	cpu_halt(halt);
}

#endif

#ifdef CPUEMU_31

/* Aranym MMU 68040  */
static void m68k_run_mmu040 ()
{
	struct flag_struct f{};
	int halt = 0;

	check_halt();
	while (!halt) {
		check_debugger();
		TRY (prb) {
			for (;;) {
#if defined(CPU_i386) || defined(CPU_x86_64)
				f.cznv = regflags.cznv;
#else // we assume CPU_arm or CPU_AARCH64 here
				f.nzcv = regflags.nzcv;
#endif
				f.x = regflags.x;
				mmu_restart = true;
				regs.instruction_pc = m68k_getpc ();

				do_cycles (cpu_cycles);

				mmu_opcode = -1;
				mmu_opcode = regs.opcode = x_prefetch (0);
				count_instr (regs.opcode);
				cpu_cycles = (*cpufunctbl[regs.opcode])(regs.opcode);
				cpu_cycles = adjust_cycles(cpu_cycles);
				regs.instruction_cnt++;

				if (regs.spcflags) {
					if (do_specialties(cpu_cycles)) {
						STOPTRY;
						return;
					}
				}
			}
		} CATCH (prb) {

			if (mmu_restart) {
				/* restore state if instruction restart */
#if defined(CPU_i386) || defined(CPU_x86_64)
				regflags.cznv = f.cznv;
#else // we assume CPU_arm or CPU_AARCH64 here
				regflags.nzcv = f.nzcv;
#endif
				regflags.x = f.x;
				m68k_setpci (regs.instruction_pc);
			}
			cpu_restore_fixup();
			TRY (prb2) {
				Exception (prb);
			} CATCH (prb2) {
				halt = 1;
			} ENDTRY
		} ENDTRY
	}
	cpu_halt(halt);
}

#endif

#ifdef CPUEMU_32

// Previous MMU 68030
static void m68k_run_mmu030 (void)
{
	struct flag_struct f;
	int halt = 0;

	mmu030_opcode_stageb = -1;
	mmu030_fake_prefetch = -1;
	check_halt();
	while(!halt) {
		check_debugger();
		TRY (prb) {
			for (;;) {
				int cnt;
insretry:
				regs.instruction_pc = m68k_getpc ();
#if defined(CPU_i386) || defined(CPU_x86_64)
				f.cznv = regflags.cznv;
#else // we assume CPU_arm or CPU_AARCH64 here
				f.nzcv = regflags.nzcv;
#endif
				f.x = regflags.x;

				mmu030_state[0] = mmu030_state[1] = mmu030_state[2] = 0;
				mmu030_opcode = -1;
				if (mmu030_fake_prefetch >= 0) {
					// use fake prefetch opcode only if mapping changed
					uaecptr new_addr = mmu030_translate(regs.instruction_pc, regs.s != 0, false, false);
					if (mmu030_fake_prefetch_addr != new_addr) {
						regs.opcode = mmu030_fake_prefetch;
						write_log(_T("MMU030 fake prefetch remap: %04x, %08x -> %08x\n"), mmu030_fake_prefetch, mmu030_fake_prefetch_addr, new_addr);
					} else {
						if (mmu030_opcode_stageb < 0) {
							regs.opcode = x_prefetch (0);
						} else {
							regs.opcode = mmu030_opcode_stageb;
							mmu030_opcode_stageb = -1;
						}
					}
					mmu030_fake_prefetch = -1;
				} else if (mmu030_opcode_stageb < 0) {
					if (currprefs.cpu_compatible)
						regs.opcode = regs.irc;
					else
						regs.opcode = x_prefetch (0);
				} else {
					regs.opcode = mmu030_opcode_stageb;
					mmu030_opcode_stageb = -1;
				}

				mmu030_opcode = regs.opcode;
				mmu030_idx_done = 0;

				cnt = 50;
				for (;;) {
					regs.opcode = regs.irc = mmu030_opcode;
					mmu030_idx = 0;

					mmu030_retry = false;

					if (!currprefs.cpu_cycle_exact) {

						count_instr (regs.opcode);
						do_cycles (cpu_cycles);

						cpu_cycles = (*cpufunctbl[regs.opcode])(regs.opcode);

					} else {

						(*cpufunctbl_noret[regs.opcode])(regs.opcode);

						wait_memory_cycles();
					}

					cnt--; // so that we don't get in infinite loop if things go horribly wrong
					if (!mmu030_retry)
						break;
					if (cnt < 0) {
						cpu_halt (CPU_HALT_CPU_STUCK);
						break;
					}
					if (mmu030_retry && mmu030_opcode == -1)
						goto insretry; // urgh
				}

				mmu030_opcode = -1;

				if (!currprefs.cpu_cycle_exact) {

					cpu_cycles = adjust_cycles (cpu_cycles);
					regs.instruction_cnt++;
					if (regs.spcflags) {
						if (do_specialties(cpu_cycles)) {
							STOPTRY;
							return;
						}
					}

				} else {

					regs.instruction_cnt++;
					regs.ipl[0] = regs.ipl_pin;
					if (regs.spcflags || time_for_interrupt ()) {
						if (do_specialties(0)) {
							STOPTRY;
							return;
						}
					}

				}

			}

		} CATCH (prb) {

			if (mmu030_opcode == -1) {
				// full prefetch fill access fault
				mmufixup[0].reg = -1;
				mmufixup[1].reg = -1;
			} else if (mmu030_state[1] & MMU030_STATEFLAG1_LASTWRITE) {
				mmufixup[0].reg = -1;
				mmufixup[1].reg = -1;
			} else {
#if defined(CPU_i386) || defined(CPU_x86_64)
				regflags.cznv = f.cznv;
#else // we assume CPU_arm or CPU_AARCH64 here
				regflags.nzcv = f.nzcv;
#endif
				regflags.x = f.x;
				cpu_restore_fixup();
			}

			m68k_setpci (regs.instruction_pc);

			TRY (prb2) {
				Exception (prb);
			} CATCH (prb2) {
				halt = 1;
			} ENDTRY
		} ENDTRY
	}
	cpu_halt (halt);
}

#endif


/* "cycle exact" 68040/060 */

static void m68k_run_3ce ()
{
	struct regstruct *r = &regs;
	bool exit = false;
	int extracycles = 0;

	while (!exit) {
		check_debugger();
		TRY(prb) {
			while (!exit) {
				evt_t c = get_cycles();
				r->instruction_pc = m68k_getpc();
				r->opcode = get_iword_cache_040(0);
				// "prefetch"
				if (regs.cacr & 0x8000)
					fill_icache040(r->instruction_pc + 16);
#ifdef DEBUGGER
				if (debug_opcode_watch) {
					debug_trainer_match();
				}
#endif
				(*cpufunctbl_noret[r->opcode])(r->opcode);

				if (r->spcflags) {
					if (do_specialties (0))
						exit = true;
				}

				// workaround for situation when all accesses are cached
				if (c == get_cycles()) {
					extracycles++;
					if (extracycles >= 4) {
						extracycles = 0;
						x_do_cycles(CYCLE_UNIT);
					}
				}

				regs.instruction_cnt++;
			}
		} CATCH(prb) {
			bus_error();
			if (r->spcflags) {
				if (do_specialties(0))
					exit = true;
			}
		} ENDTRY
	}
}

/* "prefetch" 68040/060 */

static void m68k_run_3p()
{
	struct regstruct *r = &regs;
	bool exit = false;
	int cycles;

	while (!exit)  {
		check_debugger();
		TRY(prb) {
			while (!exit) {
				r->instruction_pc = m68k_getpc();
				r->opcode = get_iword_cache_040(0);
				// "prefetch"
				if (regs.cacr & 0x8000)
					fill_icache040(r->instruction_pc + 16);
#ifdef DEBUGGER
				if (debug_opcode_watch) {
					debug_trainer_match();
				}
#endif

				(*cpufunctbl_noret[r->opcode])(r->opcode);

				cpu_cycles = 2 * CYCLE_UNIT;
				cycles = adjust_cycles(cpu_cycles);
				regs.instruction_cnt++;
				x_do_cycles(cycles);

				if (r->spcflags) {
					if (do_specialties(0))
						exit = true;
				}

			}
		} CATCH(prb) {
			bus_error();
			if (r->spcflags) {
				if (do_specialties(0))
					exit = true;
			}
		} ENDTRY
	}
}

/* "cycle exact" 68020/030  */

static void m68k_run_2ce ()
{
	struct regstruct *r = &regs;
	bool exit = false;
	bool first = true;

	while (!exit) {
		check_debugger();
		TRY(prb) {
			if (first) {
				if (cpu_tracer < 0) {
					memcpy (&r->regs, &cputrace.regs, 16 * sizeof (uae_u32));
					r->ir = cputrace.ir;
					r->irc = cputrace.irc;
					r->sr = cputrace.sr;
					r->usp = cputrace.usp;
					r->isp = cputrace.isp;
					r->intmask = cputrace.intmask;
					r->stopped = cputrace.stopped;

					r->msp = cputrace.msp;
					r->vbr = cputrace.vbr;
					r->caar = cputrace.caar;
					r->cacr = cputrace.cacr;
					r->cacheholdingdata020 = cputrace.cacheholdingdata020;
					r->cacheholdingaddr020 = cputrace.cacheholdingaddr020;
					r->prefetch020addr = cputrace.prefetch020addr;
					memcpy(&r->prefetch020, &cputrace.prefetch020, CPU_PIPELINE_MAX * sizeof(uae_u16));
					memcpy(&r->prefetch020_valid, &cputrace.prefetch020_valid, CPU_PIPELINE_MAX * sizeof(uae_u8));
					memcpy(&caches020, &cputrace.caches020, sizeof caches020);

					m68k_setpc(cputrace.pc);
					if (!r->stopped) {
						if (cputrace.state > 1)
							Exception (cputrace.state);
						else if (cputrace.state == 1)
							(*cpufunctbl_noret[cputrace.opcode])(cputrace.opcode);
					}
					set_cpu_tracer (false);
					goto cont;
				}
				set_cpu_tracer (false);
				first = false;
			}

			while (!exit) {
#if 0
				static int prevopcode;
#endif
				r->instruction_pc = m68k_getpc ();

#if 0
				if (regs.irc == 0xfffb) {
					gui_message (_T("OPCODE %04X HAS FAULTY PREFETCH! PC=%08X"), prevopcode, r->instruction_pc);
				}
#endif

				//write_log (_T("%x %04x\n"), r->instruction_pc, regs.irc);

				r->opcode = regs.irc;
#if 0
				prevopcode = r->opcode;
				regs.irc = 0xfffb;
#endif
				//write_log (_T("%08x %04x\n"), r->instruction_pc, opcode);

#if DEBUG_CD32CDTVIO
				out_cd32io (r->instruction_pc);
#endif

				if (cpu_tracer) {

#if CPUTRACE_DEBUG
					validate_trace ();
#endif
					memcpy (&cputrace.regs, &r->regs, 16 * sizeof (uae_u32));
					cputrace.opcode = r->opcode;
					cputrace.ir = r->ir;
					cputrace.irc = r->irc;
					cputrace.sr = r->sr;
					cputrace.usp = r->usp;
					cputrace.isp = r->isp;
					cputrace.intmask = r->intmask;
					cputrace.stopped = r->stopped;
					cputrace.state = 1;
					cputrace.pc = m68k_getpc ();

					cputrace.msp = r->msp;
					cputrace.vbr = r->vbr;
					cputrace.caar = r->caar;
					cputrace.cacr = r->cacr;
					cputrace.cacheholdingdata020 = r->cacheholdingdata020;
					cputrace.cacheholdingaddr020 = r->cacheholdingaddr020;
					cputrace.prefetch020addr = r->prefetch020addr;
					memcpy(&cputrace.prefetch020, &r->prefetch020, CPU_PIPELINE_MAX * sizeof (uae_u16));
					memcpy(&cputrace.prefetch020_valid, &r->prefetch020_valid, CPU_PIPELINE_MAX * sizeof(uae_u8));
					memcpy(&cputrace.caches020, &caches020, sizeof caches020);

					cputrace.memoryoffset = 0;
					cputrace.cyclecounter = cputrace.cyclecounter_pre = cputrace.cyclecounter_post = 0;
					cputrace.readcounter = cputrace.writecounter = 0;
				}

				if (inputrecord_debug & 4) {
					if (input_record > 0)
						inprec_recorddebug_cpu(1, r->opcode);
					else if (input_play > 0)
						inprec_playdebug_cpu(1, r->opcode);
				}
#ifdef DEBUGGER
				if (debug_opcode_watch) {
					debug_trainer_match();
				}
#endif

				(*cpufunctbl_noret[r->opcode])(r->opcode);

				wait_memory_cycles();
				regs.instruction_cnt++;
				regs.ce020extracycles++;

		cont:
				if (r->spcflags || regs.ipl[0] > 0) {
					if (do_specialties (0))
						exit = true;
				}
				ipl_fetch_now();


			}
		} CATCH(prb) {
			bus_error();
			ipl_fetch_now();
			if (r->spcflags || time_for_interrupt()) {
				if (do_specialties(0))
					exit = true;
			}
		} ENDTRY
	}
}

#ifdef CPUEMU_20

// full prefetch 020 (more compatible)
static void m68k_run_2p ()
{
	struct regstruct *r = &regs;
	bool exit = false;
	bool first = true;

	while (!exit) {
		check_debugger();
		TRY(prb) {

			if (first) {
				if (cpu_tracer < 0) {
					memcpy (&r->regs, &cputrace.regs, 16 * sizeof (uae_u32));
					r->ir = cputrace.ir;
					r->irc = cputrace.irc;
					r->sr = cputrace.sr;
					r->usp = cputrace.usp;
					r->isp = cputrace.isp;
					r->intmask = cputrace.intmask;
					r->stopped = cputrace.stopped;

					r->msp = cputrace.msp;
					r->vbr = cputrace.vbr;
					r->caar = cputrace.caar;
					r->cacr = cputrace.cacr;
					r->cacheholdingdata020 = cputrace.cacheholdingdata020;
					r->cacheholdingaddr020 = cputrace.cacheholdingaddr020;
					r->prefetch020addr = cputrace.prefetch020addr;
					memcpy(&r->prefetch020, &cputrace.prefetch020, CPU_PIPELINE_MAX * sizeof(uae_u16));
					memcpy(&r->prefetch020_valid, &cputrace.prefetch020_valid, CPU_PIPELINE_MAX * sizeof(uae_u8));
					memcpy(&caches020, &cputrace.caches020, sizeof caches020);

					m68k_setpc (cputrace.pc);
					if (!r->stopped) {
						if (cputrace.state > 1)
							Exception (cputrace.state);
						else if (cputrace.state == 1)
							(*cpufunctbl[cputrace.opcode])(cputrace.opcode);
					}
					set_cpu_tracer (false);
					goto cont;
				}
				set_cpu_tracer (false);
				first = false;
			}

			while (!exit) {
				r->instruction_pc = m68k_getpc ();
				r->opcode = regs.irc;

#if DEBUG_CD32CDTVIO
				out_cd32io (m68k_getpc ());
#endif

				if (cpu_tracer) {

#if CPUTRACE_DEBUG
					validate_trace ();
#endif
					memcpy (&cputrace.regs, &r->regs, 16 * sizeof (uae_u32));
					cputrace.opcode = r->opcode;
					cputrace.ir = r->ir;
					cputrace.irc = r->irc;
					cputrace.sr = r->sr;
					cputrace.usp = r->usp;
					cputrace.isp = r->isp;
					cputrace.intmask = r->intmask;
					cputrace.stopped = r->stopped;
					cputrace.state = 1;
					cputrace.pc = m68k_getpc ();

					cputrace.msp = r->msp;
					cputrace.vbr = r->vbr;
					cputrace.caar = r->caar;
					cputrace.cacr = r->cacr;
					cputrace.cacheholdingdata020 = r->cacheholdingdata020;
					cputrace.cacheholdingaddr020 = r->cacheholdingaddr020;
					cputrace.prefetch020addr = r->prefetch020addr;
					memcpy(&cputrace.prefetch020, &r->prefetch020, CPU_PIPELINE_MAX * sizeof(uae_u16));
					memcpy(&cputrace.prefetch020_valid, &r->prefetch020_valid, CPU_PIPELINE_MAX * sizeof(uae_u8));
					memcpy(&cputrace.caches020, &caches020, sizeof caches020);

					cputrace.memoryoffset = 0;
					cputrace.cyclecounter = cputrace.cyclecounter_pre = cputrace.cyclecounter_post = 0;
					cputrace.readcounter = cputrace.writecounter = 0;
				}

				if (inputrecord_debug & 4) {
					if (input_record > 0)
						inprec_recorddebug_cpu(1, r->opcode);
					else if (input_play > 0)
						inprec_playdebug_cpu(1, r->opcode);
				}
#ifdef DEBUGGER
				if (debug_opcode_watch) {
					debug_trainer_match();
				}
#endif

				if (currprefs.cpu_memory_cycle_exact) {

					evt_t c = get_cycles();
					(*cpufunctbl[r->opcode])(r->opcode);
					c = get_cycles() - c;
					cpu_cycles = 0;
					if (c <= cpucycleunit) {
						cpu_cycles = cpucycleunit;
					}
					regs.instruction_cnt++;

				} else {

					cpu_cycles = (*cpufunctbl[r->opcode])(r->opcode);
					cpu_cycles = adjust_cycles (cpu_cycles);
					regs.instruction_cnt++;

				}

				if (cpu_cycles > 0)
					x_do_cycles(cpu_cycles);

cont:
				if (r->spcflags || regs.ipl[0] > 0) {
					if (do_specialties (cpu_cycles))
						exit = true;
				}
				ipl_fetch_now();
			}
		} CATCH(prb) {
			bus_error();
			if (r->spcflags) {
				if (do_specialties(cpu_cycles))
					exit = true;
			}
			ipl_fetch_now();
		} ENDTRY
	}
}

#endif

#ifdef WITH_THREADED_CPU
static int cpu_thread_run_2(void *v)
{
	bool exit = false;
	struct regstruct *r = &regs;

	cpu_thread_tid = uae_thread_get_id(cpu_thread);

	cpu_thread_active = 1;
	while (!exit) {
		TRY(prb)
		{
			while (!exit) {
				r->instruction_pc = m68k_getpc();

				r->opcode = x_get_iword(0);

				(*cpufunctbl[r->opcode])(r->opcode);

				if (regs.spcflags || cpu_thread_ilvl > 0) {
					if (do_specialties_thread())
						exit = true;
				}

			}
		} CATCH(prb)
		{
			bus_error();
			if (r->spcflags) {
				if (do_specialties_thread())
					exit = true;
			}
		} ENDTRY
	}
	cpu_thread_active = 0;
	return 0;
}
#endif

/* Same thing, but don't use prefetch to get opcode.  */
static void m68k_run_2_000()
{
	struct regstruct *r = &regs;
	bool exit = false;

	while (!exit) {
		check_debugger();
		TRY(prb) {
			while (!exit) {
				r->instruction_pc = m68k_getpc ();

				r->opcode = x_get_iword(0);
				count_instr (r->opcode);
#ifdef DEBUGGER
				if (debug_opcode_watch) {
					debug_trainer_match();
				}
#endif

				cpu_cycles = (*cpufunctbl[r->opcode])(r->opcode) & 0xffff;
				cpu_cycles = adjust_cycles (cpu_cycles);
				do_cycles(cpu_cycles);

				if (r->spcflags) {
					if (do_specialties (cpu_cycles))
						exit = true;
				}
			}
		} CATCH(prb) {
			bus_error();
			if (r->spcflags) {
				if (do_specialties(cpu_cycles))
					exit = true;
			}
		} ENDTRY
	}
}

static void m68k_run_2_020()
{
#ifdef WITH_THREADED_CPU
	if (currprefs.cpu_thread) {
		run_cpu_thread(cpu_thread_run_2);
		return;
	}
#endif

	struct regstruct *r = &regs;
	bool exit = false;

	while (!exit) {
		check_debugger();
		TRY(prb) {
			while (!exit) {
				r->instruction_pc = m68k_getpc();

				r->opcode = x_get_iword(0);
				count_instr(r->opcode);

#ifdef DEBUGGER
				if (debug_opcode_watch) {
					debug_trainer_match();
				}
#endif

				cpu_cycles = (*cpufunctbl[r->opcode])(r->opcode) >> 16;
				cpu_cycles = adjust_cycles(cpu_cycles);
				do_cycles(cpu_cycles);

				if (r->spcflags) {
					if (do_specialties(cpu_cycles))
						exit = true;
				}
			}
		} CATCH(prb) {
			bus_error();
			if (r->spcflags) {
				if (do_specialties(cpu_cycles))
					exit = true;
			}
		} ENDTRY
	}
}


/* fake MMU 68k  */
#if 0
static void m68k_run_mmu (void)
{
	for (;;) {
		regs.opcode = get_iiword (0);
		do_cycles (cpu_cycles);
		mmu_backup_regs = regs;
		cpu_cycles = (*cpufunctbl[regs.opcode])(regs.opcode);
		cpu_cycles = adjust_cycles (cpu_cycles);
		if (mmu_triggered)
			mmu_do_hit ();
		if (regs.spcflags) {
			if (do_specialties (cpu_cycles))
				return;
		}
	}
}
#endif

#endif /* CPUEMU_0 */

int in_m68k_go = 0;

#if 0
static void exception2_handle (uaecptr addr, uaecptr fault)
{
	last_addr_for_exception_3 = addr;
	last_fault_for_exception_3 = fault;
	last_writeaccess_for_exception_3 = 0;
	last_fc_for_exception_3 = 0;
	Exception (2);
}
#endif

static bool cpu_hardreset, cpu_keyboardreset;

bool is_hardreset()
{
	return cpu_hardreset;
}
bool is_keyboardreset()
{
	return  cpu_keyboardreset;
}

static void warpmode_reset()
{
	if (currprefs.turbo_boot && currprefs.turbo_emulation < 2) {
		warpmode(1);
		currprefs.turbo_emulation = changed_prefs.turbo_emulation = 2;
	}
}

void m68k_go (int may_quit)
{
	int hardboot = 1;

#ifdef WITH_THREADED_CPU
	init_cpu_thread();
#endif
	if (in_m68k_go || !may_quit) {
		write_log (_T("Bug! m68k_go is not reentrant.\n"));
		abort ();
	}

	reset_frame_rate_hack ();
	update_68k_cycles ();
	start_cycles = 0;

	set_cpu_tracer (false);

	cpu_prefs_changed_flag = 0;
	in_m68k_go++;
	for (;;) {
		int restored = 0;
		void (*run_func)(void);

		cputrace.state = -1;

		if (regs.halted == CPU_HALT_ACCELERATOR_CPU_FALLBACK) {
			cpu_halt_clear();
			cpu_do_fallback();
		}

		if (currprefs.inprecfile[0] && input_play) {
			inprec_open (currprefs.inprecfile, nullptr);
			changed_prefs.inprecfile[0] = currprefs.inprecfile[0] = 0;
			quit_program = UAE_RESET;
		}
		if (input_play || input_record)
			inprec_startup ();

		if (quit_program > 0) {
			cpu_keyboardreset = quit_program == UAE_RESET_KEYBOARD;
			cpu_hardreset = ((quit_program == UAE_RESET_HARD ? 1 : 0) || hardboot) != 0;
			hardboot |= quit_program == UAE_RESET_HARD ? 1 : 0;

			if (quit_program == UAE_QUIT)
				break;

			hsync_counter = 0;
			vsync_counter = 0;
			quit_program = 0;

#ifdef SAVESTATE
			if (savestate_state == STATE_DORESTORE) {
				savestate_state = STATE_RESTORE;
			}
			if (savestate_state == STATE_RESTORE) {
				restore_state (savestate_fname);
				cpu_hardreset = true;
			} else if (savestate_state == STATE_REWIND) {
				savestate_rewind ();
			}
#endif
			if (cpu_hardreset) {
				m68k_reset_restore();
			}
			prefs_changed_cpu();
			build_cpufunctbl();
			set_x_funcs();
			set_cycles (start_cycles);
			custom_reset (cpu_hardreset != 0, cpu_keyboardreset);
			m68k_reset2 (cpu_hardreset != 0);
			if (cpu_hardreset) {
				memory_clear ();
				write_log (_T("hardreset, memory cleared\n"));
			}
#ifdef DEBUGGER
			if (debug_dma) {
				record_dma_reset(1);
				record_dma_reset(1);
			}
#endif
#ifdef SAVESTATE
			/* We may have been restoring state, but we're done now.  */
			if (isrestore ()) {
				restored = savestate_restore_finish ();
#ifdef DEBUGGER
				memory_map_dump ();
#endif
				if (currprefs.mmu_model == 68030) {
					mmu030_decode_tc (tc_030, true);
				} else if (currprefs.mmu_model >= 68040) {
					mmu_set_tc (regs.tcr);
				}
				hardboot = 1;
			}
#endif
			if (currprefs.produce_sound == 0)
				eventtab[ev_audio].active = false;
			m68k_setpc_normal (regs.pc);
			check_prefs_changed_audio ();

			if (!restored || hsync_counter == 0)
				savestate_check ();
			if (input_record == INPREC_RECORD_START)
				input_record = INPREC_RECORD_NORMAL;
			statusline_clear();
		} else {
			if (input_record == INPREC_RECORD_START) {
				input_record = INPREC_RECORD_NORMAL;
				savestate_init ();
				hsync_counter = 0;
				vsync_counter = 0;
				savestate_check ();
			}
		}

		if (changed_prefs.inprecfile[0] && input_record)
			inprec_prepare_record (savestate_fname[0] ? savestate_fname : nullptr);
#ifdef DEBUGGER
		if (changed_prefs.trainerfile[0])
			debug_init_trainer(changed_prefs.trainerfile);
#endif
		set_cpu_tracer (false);

#ifdef DEBUGGER
		if (debugging)
			debug ();
#endif
		if (regs.spcflags & SPCFLAG_MODE_CHANGE) {
			if (cpu_prefs_changed_flag & 1) {
				uaecptr pc = m68k_getpc();
				prefs_changed_cpu();
				fpu_modechange();
				custom_cpuchange();
				build_cpufunctbl();
				m68k_setpc_normal(pc);
				fill_prefetch();
				update_68k_cycles();
				init_custom();
			}
			if (cpu_prefs_changed_flag & 2) {
				fixup_cpu(&changed_prefs);
				currprefs.m68k_speed = changed_prefs.m68k_speed;
				currprefs.m68k_speed_throttle = changed_prefs.m68k_speed_throttle;
				update_68k_cycles();
				target_cpu_speed();
			}
			cpu_prefs_changed_flag = 0;
		}

		set_x_funcs();
		if (hardboot) {
			custom_prepare();
			mman_set_barriers(false);
			protect_roms(true);
		}
		if ((cpu_keyboardreset || hardboot) && !restored) {
			warpmode_reset();
		}
		cpu_hardreset = false;
		cpu_keyboardreset = false;
		event_wait = true;
		unset_special(SPCFLAG_MODE_CHANGE);

		if (!restored && hardboot) {
			uaerandomizeseed();
			uae_u32 s = uaerandgetseed();
			uaesetrandseed(s);
			write_log("rndseed = %08x (%u)\n", s, s);
			// add random delay before CPU starts
			int t = uaerand() & 0x7fff;
			while (t > 255) {
				x_do_cycles(255 * CYCLE_UNIT);
				t -= 255;
			}
			x_do_cycles(t * CYCLE_UNIT);
		}

		hardboot = 0;

#ifdef SAVESTATE
		if (restored) {
			restored = 0;
			savestate_restore_final();
		}
#endif

		if (!regs.halted) {
			// check that PC points to something that looks like memory.
			uaecptr pc = m68k_getpc();
			addrbank *ab = get_mem_bank_real(pc);
			if (ab == nullptr || ab == &dummy_bank || (!currprefs.cpu_compatible && !valid_address(pc, 2)) || (pc & 1)) {
				cpu_halt(CPU_HALT_INVALID_START_ADDRESS);
			}
		}
		if (regs.halted) {
			cpu_halt (regs.halted);
			if (regs.halted < 0) {
				haltloop();
				continue;
			}
		}

#if 0
		if (mmu_enabled && !currprefs.cachesize) {
			run_func = m68k_run_mmu;
		} else {
#endif
			run_func = currprefs.cpu_cycle_exact && currprefs.cpu_model <= 68010 ? m68k_run_1_ce :
				currprefs.cpu_compatible && currprefs.cpu_model <= 68010 ? m68k_run_1 :
#ifdef JIT
				currprefs.cpu_model >= 68020 && currprefs.cachesize ? m68k_run_jit :
#endif
				currprefs.cpu_model == 68030 && currprefs.mmu_model ? m68k_run_mmu030 :
				currprefs.cpu_model == 68040 && currprefs.mmu_model ? m68k_run_mmu040 :
				currprefs.cpu_model == 68060 && currprefs.mmu_model ? m68k_run_mmu060 :

				currprefs.cpu_model >= 68040 && currprefs.cpu_cycle_exact ? m68k_run_3ce :
				currprefs.cpu_model >= 68020 && currprefs.cpu_cycle_exact ? m68k_run_2ce :

				currprefs.cpu_model <= 68020 && currprefs.cpu_compatible ? m68k_run_2p :
				currprefs.cpu_model == 68030 && currprefs.cpu_compatible ? m68k_run_2p :
				currprefs.cpu_model >= 68040 && currprefs.cpu_compatible ? m68k_run_3p :

				currprefs.cpu_model < 68020 ? m68k_run_2_000 : m68k_run_2_020;
#if 0
		}
#endif
		run_func();

		custom_end_drawing();

		if (quit_program < 0) {
			quit_program = -quit_program;
		}
		if (quit_program == UAE_QUIT)
			break;
	}
	protect_roms(false);
	mman_set_barriers(true);
	in_m68k_go--;
}



void m68k_disasm_ea (uaecptr addr, uaecptr *nextpc, int cnt, uae_u32 *seaddr, uae_u32 *deaddr, uaecptr lastpc)
{
	TCHAR *buf;

	if (!cnt)
		return;
	int pcnt = cnt > 0 ? cnt : -cnt;
	buf = xcalloc (TCHAR, (MAX_LINEWIDTH + 1) * pcnt);
	if (!buf)
		return;
#ifdef DEBUGGER
	m68k_disasm_2(buf, MAX_LINEWIDTH * pcnt, addr, nullptr, 0, nextpc, cnt, seaddr, deaddr, lastpc, 1);
#endif
	xfree (buf);
}
void m68k_disasm (uaecptr addr, uaecptr *nextpc, uaecptr lastpc, int cnt)
{
	TCHAR *buf;

	if (!cnt)
		return;
	int pcnt = cnt > 0 ? cnt : -cnt;
	buf = xcalloc (TCHAR, (MAX_LINEWIDTH + 1) * pcnt);
	if (!buf)
		return;
#ifdef DEBUGGER
	m68k_disasm_2(buf, MAX_LINEWIDTH * pcnt, addr, nullptr, 0, nextpc, cnt, nullptr, nullptr, lastpc, 0);
#endif
	console_out_f (_T("%s"), buf);
	xfree (buf);
}

void m68k_dumpstate(uaecptr *nextpc, uaecptr prevpc)
{
	int i, j;
	uaecptr pc = M68K_GETPC;

	MakeSR();
	for (i = 0; i < 8; i++){
		console_out_f (_T("  D%d %08X "), i, m68k_dreg (regs, i));
		if ((i & 3) == 3) console_out_f (_T("\n"));
	}
	for (i = 0; i < 8; i++){
		console_out_f (_T("  A%d %08X "), i, m68k_areg (regs, i));
		if ((i & 3) == 3) console_out_f (_T("\n"));
	}
	if (regs.s == 0)
		regs.usp = m68k_areg (regs, 7);
	if (regs.s && regs.m)
		regs.msp = m68k_areg (regs, 7);
	if (regs.s && regs.m == 0)
		regs.isp = m68k_areg (regs, 7);
	j = 2;
	console_out_f (_T("USP  %08X ISP  %08X "), regs.usp, regs.isp);
#ifdef DEBUGGER
	for (i = 0; m2cregs[i].regno>= 0; i++) {
		if (!movec_illg (m2cregs[i].regno)) {
			if (!_tcscmp (m2cregs[i].regname, _T("USP")) || !_tcscmp (m2cregs[i].regname, _T("ISP")))
				continue;
			if (j > 0 && (j % 4) == 0)
				console_out_f (_T("\n"));
			console_out_f (_T("%-4s %08X "), m2cregs[i].regname, val_move2c (m2cregs[i].regno));
			j++;
		}
	}
#endif
	if (j > 0)
		console_out_f (_T("\n"));
		console_out_f (_T("SR=%04X T=%d%d S=%d M=%d X=%d N=%d Z=%d V=%d C=%d IM=%d STP=%d\n"),
		regs.sr, regs.t1, regs.t0, regs.s, regs.m,
		GET_XFLG(), GET_NFLG(), GET_ZFLG(),
		GET_VFLG(), GET_CFLG(),
		regs.intmask, regs.stopped);
#ifdef FPUEMU
	if (currprefs.fpu_model) {
		uae_u32 fpsr;
		for (i = 0; i < 8; i++) {
			console_out_f(_T("%d: "), i);
			console_out_f (_T("%s "), fpp_print(&regs.fp[i], -1));
			console_out_f (_T("%s "), fpp_print(&regs.fp[i], 0));
			console_out_f (_T("\n"));
		}
		fpsr = fpp_get_fpsr ();
		console_out_f (_T("FPSR: %08X FPCR: %08x FPIAR: %08x N=%d Z=%d I=%d NAN=%d\n"),
			fpsr, regs.fpcr, regs.fpiar,
			(fpsr & 0x8000000) != 0,
			(fpsr & 0x4000000) != 0,
			(fpsr & 0x2000000) != 0,
			(fpsr & 0x1000000) != 0);
	}
#endif
	if (currprefs.mmu_model == 68030) {
		console_out_f (_T("SRP: %llX CRP: %llX\n"), srp_030, crp_030);
		console_out_f (_T("TT0: %08X TT1: %08X TC: %08X\n"), tt0_030, tt1_030, tc_030);
	}
	if (currprefs.cpu_compatible) {
		console_out_f(_T("Prefetch"));
		if (currprefs.cpu_model == 68020 || currprefs.cpu_model == 68030) {
			console_out_f(_T(" %08x %08x (%d)"),
				regs.cacheholdingaddr020, regs.cacheholdingdata020, regs.cacheholdingdata_valid);
		}
		for (int i = 0; i < 3; i++) {
			uae_u16 w;
#ifdef DEBUGGER
			if (!debug_get_prefetch(i, &w))
				break;
#endif
			struct instr *dp;
			struct mnemolookup *lookup;
			dp = table68k + w;
			for (lookup = lookuptab; lookup->mnemo != dp->mnemo; lookup++)
				;
			console_out_f(_T(" %04x (%s)"), w, lookup->name);
		}
		console_out_f (_T(" Chip latch %08X\n"), regs.chipset_latch_rw);
	}
	if (prevpc != 0xffffffff && pc - prevpc < 100) {
		while (prevpc < pc) {
			m68k_disasm(prevpc, &prevpc, 0xffffffff, 1);
		}
	}
	m68k_disasm (pc, nextpc, pc, 1);
	if (nextpc)
		console_out_f (_T("Next PC: %08x\n"), *nextpc);
}

void m68k_dumpcache (bool dc)
{
	if (!currprefs.cpu_compatible)
		return;
	if (currprefs.cpu_model == 68020) {
		for (int i = 0; i < CACHELINES020; i += 4) {
			for (int j = 0; j < 4; j++) {
				int s = i + j;
				uaecptr addr;
				int fc;
				struct cache020 *c = &caches020[s];
				fc = c->tag & 1;
				addr = c->tag & ~1;
				addr |= s << 2;
				console_out_f (_T("%08X%c:%08X%c"), addr, fc ? 'S' : 'U', c->data, c->valid ? '*' : ' ');
			}
			console_out_f (_T("\n"));
		}
	} else if (currprefs.cpu_model == 68030) {
		for (int i = 0; i < CACHELINES030; i++) {
			struct cache030 *c = dc ? &dcaches030[i] : &icaches030[i];
			int fc;
			uaecptr addr;
			if (!dc) {
				fc = (c->tag & 1) ? 6 : 2;
			} else {
				fc = c->fc;
			}
			addr = c->tag & ~1;
			addr |= i << 4;
			console_out_f (_T("%08X %d: "), addr, fc);
			for (int j = 0; j < 4; j++) {
				console_out_f (_T("%08X%c "), c->data[j], c->valid[j] ? '*' : ' ');
			}
			console_out_f (_T("\n"));
		}
	} else if (currprefs.cpu_model >= 68040) {
		uae_u32 tagmask = dc ? cachedtag04060mask : cacheitag04060mask;
		for (int i = 0; i < cachedsets04060; i++) {
			struct cache040 *c = dc ? &dcaches040[i] : &icaches040[i];
			for (int j = 0; j < CACHELINES040; j++) {
				if (c->valid[j]) {
					uae_u32 addr = (c->tag[j] & tagmask) | (i << 4);
					write_log(_T("%02d:%d %08x = %08x%c %08x%c %08x%c %08x%c\n"),
						i, j, addr,
						c->data[j][0], c->dirty[j][0] ? '*' : ' ',
						c->data[j][1], c->dirty[j][1] ? '*' : ' ',
						c->data[j][2], c->dirty[j][2] ? '*' : ' ',
						c->data[j][3], c->dirty[j][3] ? '*' : ' ');
				}
			}
		}
	}
}

#ifdef SAVESTATE

/* CPU save/restore code */

#define CPUTYPE_EC 1
#define CPUMODE_HALT 1
#define CPUMODE_HALT2 2

uae_u8 *restore_cpu (uae_u8 *src)
{
	int flags, model;
	uae_u32 l;

	changed_prefs.fpu_model = currprefs.fpu_model = 0;
	changed_prefs.mmu_model = currprefs.mmu_model = 0;
	currprefs.cpu_model = changed_prefs.cpu_model = model = restore_u32 ();
	flags = restore_u32 ();
	changed_prefs.address_space_24 = false;
	if (flags & CPUTYPE_EC)
		changed_prefs.address_space_24 = true;
	currprefs.address_space_24 = changed_prefs.address_space_24;
	currprefs.cpu_compatible = changed_prefs.cpu_compatible;
	currprefs.cpu_cycle_exact = changed_prefs.cpu_cycle_exact;
	currprefs.cpu_memory_cycle_exact = changed_prefs.cpu_memory_cycle_exact;
	currprefs.blitter_cycle_exact = changed_prefs.blitter_cycle_exact;
	currprefs.cpu_frequency = changed_prefs.cpu_frequency = 0;
	currprefs.cpu_clock_multiplier = changed_prefs.cpu_clock_multiplier = 0;
	for (int i = 0; i < 15; i++)
		regs.regs[i] = restore_u32 ();
	regs.pc = restore_u32 ();
	regs.irc = restore_u16 ();
	regs.ir = restore_u16 ();
	regs.usp = restore_u32 ();
	regs.isp = restore_u32 ();
	regs.sr = restore_u16 ();
	l = restore_u32 ();
	if (l & CPUMODE_HALT) {
		regs.stopped = (l & CPUMODE_HALT2) ? 2 : 1;
	} else {
		regs.stopped = 0;
	}
	if (model >= 68010) {
		regs.dfc = restore_u32 ();
		regs.sfc = restore_u32 ();
		regs.vbr = restore_u32 ();
	}
	if (model >= 68020) {
		regs.caar = restore_u32 ();
		regs.cacr = restore_u32 ();
		regs.msp = restore_u32 ();
	}
	if (model >= 68030) {
		crp_030 = fake_crp_030 = restore_u64 ();
		srp_030 = fake_srp_030 = restore_u64 ();
		tt0_030 = fake_tt0_030 = restore_u32 ();
		tt1_030 = fake_tt1_030 = restore_u32 ();
		tc_030 = fake_tc_030 = restore_u32 ();
		mmusr_030 = fake_mmusr_030 = restore_u16 ();
	}
	if (model >= 68040) {
		regs.itt0 = restore_u32 ();
		regs.itt1 = restore_u32 ();
		regs.dtt0 = restore_u32 ();
		regs.dtt1 = restore_u32 ();
		regs.tcr = restore_u32 ();
		regs.urp = restore_u32 ();
		regs.srp = restore_u32 ();
	}
	if (model >= 68060) {
		regs.buscr = restore_u32 ();
		regs.pcr = restore_u32 ();
	}
	if (flags & 0x80000000) {
		int khz = restore_u32 ();
		restore_u32 ();
		if (khz > 0 && khz < 800000)
			currprefs.m68k_speed = changed_prefs.m68k_speed = 0;
	}
	set_cpu_caches (true);
	if (flags & 0x40000000) {
		if (model == 68020) {
			for (auto & i : caches020) {
				i.data = restore_u32 ();
				i.tag = restore_u32 ();
				i.valid = restore_u8 () != 0;
			}
			regs.prefetch020addr = restore_u32 ();
			regs.cacheholdingaddr020 = restore_u32 ();
			regs.cacheholdingdata020 = restore_u32 ();
			if (flags & 0x20000000) {
				if (flags & 0x4000000) {
					// 3.6 new (back to 16 bits)
					for (int i = 0; i < CPU_PIPELINE_MAX; i++) {
						uae_u32 v = restore_u32();
						regs.prefetch020[i] = v >> 16;
						regs.prefetch020_valid[i] = (v & 1) != 0;
					}
				} else {
					// old
					uae_u32 v = restore_u32();
					regs.prefetch020[0] = v >> 16;
					regs.prefetch020[1] = static_cast<uae_u16>(v);
					v = restore_u32();
					regs.prefetch020[2] = v >> 16;
					regs.prefetch020[3] = static_cast<uae_u16>(v);
					restore_u32();
					restore_u32();
					regs.prefetch020_valid[0] = true;
					regs.prefetch020_valid[1] = true;
					regs.prefetch020_valid[2] = true;
					regs.prefetch020_valid[3] = true;
				}
			}
		} else if (model == 68030) {
			for (auto & i : icaches030) {
				for (int j = 0; j < 4; j++) {
					i.data[j] = restore_u32 ();
					i.valid[j] = restore_u8 () != 0;
				}
				i.tag = restore_u32 ();
			}
			for (auto & i : dcaches030) {
				for (int j = 0; j < 4; j++) {
					i.data[j] = restore_u32 ();
					i.valid[j] = restore_u8 () != 0;
				}
				i.tag = restore_u32 ();
			}
			regs.prefetch020addr = restore_u32 ();
			regs.cacheholdingaddr020 = restore_u32 ();
			regs.cacheholdingdata020 = restore_u32 ();
			if (flags & 0x4000000) {
				for (int i = 0; i < CPU_PIPELINE_MAX; i++) {
					uae_u32 v = restore_u32();
					regs.prefetch020[i] = v >> 16;
					regs.prefetch020_valid[i] = (v & 1) != 0;
				}
			} else {
				for (int i = 0; i < CPU_PIPELINE_MAX; i++) {
					regs.prefetch020[i] = restore_u32 ();
					regs.prefetch020_valid[i] = false;
				}
			}
		} else if (model == 68040) {
			if (flags & 0x8000000) {
				for (int i = 0; i < ((model == 68060 && (flags & 0x4000000)) ? CACHESETS060 : CACHESETS040); i++) {
					for (int j = 0; j < CACHELINES040; j++) {
						struct cache040 *c = &icaches040[i];
						c->data[j][0] = restore_u32();
						c->data[j][1] = restore_u32();
						c->data[j][2] = restore_u32();
						c->data[j][3] = restore_u32();
						c->tag[j] = restore_u32();
						c->valid[j] = restore_u16() & 1;
					}
				}
				regs.prefetch020addr = restore_u32();
				regs.cacheholdingaddr020 = restore_u32();
				regs.cacheholdingdata020 = restore_u32();
				for (int i = 0; i < CPU_PIPELINE_MAX; i++)
					regs.prefetch040[i] = restore_u32();
				if (flags & 0x4000000) {
					for (int i = 0; i < (model == 68060 ? CACHESETS060 : CACHESETS040); i++) {
						for (int j = 0; j < CACHELINES040; j++) {
							struct cache040 *c = &dcaches040[i];
							c->data[j][0] = restore_u32();
							c->data[j][1] = restore_u32();
							c->data[j][2] = restore_u32();
							c->data[j][3] = restore_u32();
							c->tag[j] = restore_u32();
							uae_u16 v = restore_u16();
							c->valid[j] = (v & 1) != 0;
							c->dirty[j][0] = (v & 0x10) != 0;
							c->dirty[j][1] = (v & 0x20) != 0;
							c->dirty[j][2] = (v & 0x40) != 0;
							c->dirty[j][3] = (v & 0x80) != 0;
							c->gdirty[j] = c->dirty[j][0] || c->dirty[j][1] || c->dirty[j][2] || c->dirty[j][3];
						}
					}
				}
			}
		}
		if (model >= 68020) {
			restore_u32 (); // regs.ce020memcycles
			regs.ce020startcycle = regs.ce020endcycle = 0;
			restore_u32 ();
		}
	}
	if (flags & 0x10000000) {
		regs.chipset_latch_rw = restore_u32 ();
		regs.chipset_latch_read = restore_u32 ();
		regs.chipset_latch_write = restore_u32 ();
	}

	regs.pipeline_pos = -1;
	regs.pipeline_stop = 0;
	if ((flags & 0x4000000) && currprefs.cpu_model == 68020) {
		regs.pipeline_pos = restore_u16();
		regs.pipeline_r8[0] = restore_u16();
		regs.pipeline_r8[1] = restore_u16();
		regs.pipeline_stop = restore_u16();
	}

	if ((flags & 0x2000000) && currprefs.cpu_model <= 68010) {
		int v = restore_u32();
		regs.ird = restore_u16();
		regs.read_buffer = restore_u16();
		regs.write_buffer = restore_u16();
		if (v & 1) {
			regs.ipl[0] = restore_s8();
			regs.ipl[1] = restore_s8();
			regs.ipl_pin = restore_s8();
			regs.ipl_pin_p = restore_s8();
			regs.ipl_evt = restore_u64();
			regs.ipl_evt_pre = restore_u64();
			regs.ipl_pin_change_evt = restore_u64();
			regs.ipl_pin_change_evt_p = restore_u64();
		}
	}

	m68k_reset_sr();

	write_log (_T("CPU: %d%s%03d, PC=%08X\n"),
		model / 1000, flags & 1 ? _T("EC") : _T(""), model % 1000, regs.pc);

	return src;
}

static void fill_prefetch_quick ()
{
	if (currprefs.cpu_model >= 68020) {
		fill_prefetch ();
		return;
	}
	// old statefile compatibility, this needs to done,
	// even in 68000 cycle-exact mode
	regs.ir = get_word (m68k_getpc ());
	regs.ird = regs.ir;
	regs.irc = get_word (m68k_getpc () + 2);
}

void restore_cpu_finish (void)
{
	if (!currprefs.fpu_model)
		fpu_reset();
	init_m68k ();
	m68k_setpc_normal (regs.pc);
	doint ();
	fill_prefetch_quick ();
	set_cycles (start_cycles);
	events_schedule ();
	//activate_debugger ();
}

uae_u8 *save_cpu_trace(size_t *len, uae_u8 *dstptr)
{
	uae_u8 *dstbak, *dst;

	if (cputrace.state <= 0)
		return nullptr;

	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc (uae_u8, 10000);

	save_u32 (2 | 4 | 16 | 32 | 64 | 128);
	save_u16 (cputrace.opcode);
	for (int i = 0; i < 16; i++)
		save_u32 (cputrace.regs[i]);
	save_u32 (cputrace.pc);
	save_u16 (cputrace.irc);
	save_u16 (cputrace.ir);
	save_u32 (cputrace.usp);
	save_u32 (cputrace.isp);
	save_u16 (cputrace.sr);
	save_u16 (cputrace.intmask);
	save_u16 ((cputrace.stopped ? 1 : 0) | (regs.stopped ? 2 : 0));
	save_u16 (cputrace.state);
	save_u32 (cputrace.cyclecounter);
	save_u32 (cputrace.cyclecounter_pre);
	save_u32 (cputrace.cyclecounter_post);
	save_u32 (cputrace.readcounter);
	save_u32 (cputrace.writecounter);
	save_u32 (cputrace.memoryoffset);
	write_log (_T("CPUT SAVE: PC=%08x C=%016llX %08x %08x %08x %d %d %d\n"),
		cputrace.pc, cputrace.startcycles,
		cputrace.cyclecounter, cputrace.cyclecounter_pre, cputrace.cyclecounter_post,
		cputrace.readcounter, cputrace.writecounter, cputrace.memoryoffset);
	for (int i = 0; i < cputrace.memoryoffset; i++) {
		save_u32 (cputrace.ctm[i].addr);
		save_u32 (cputrace.ctm[i].data);
		save_u32 (cputrace.ctm[i].mode);
		write_log (_T("CPUT%d: %08x %08x %08x %08x\n"), i, cputrace.ctm[i].addr, cputrace.ctm[i].data, cputrace.ctm[i].mode, cputrace.ctm[i].flags);
	}
	save_u32 (static_cast<uae_u32>(cputrace.startcycles));

	if (currprefs.cpu_model == 68020) {
		for (auto & i : cputrace.caches020) {
			save_u32 (i.data);
			save_u32 (i.tag);
			save_u8 (i.valid ? 1 : 0);
		}
		save_u32 (cputrace.prefetch020addr);
		save_u32 (cputrace.cacheholdingaddr020);
		save_u32 (cputrace.cacheholdingdata020);
		for (int i = 0; i < CPU_PIPELINE_MAX; i++) {
			save_u16 (cputrace.prefetch020[i]);
		}
		for (int i = 0; i < CPU_PIPELINE_MAX; i++) {
			save_u32 (cputrace.prefetch020[i]);
		}
		for (int i = 0; i < CPU_PIPELINE_MAX; i++) {
			save_u8 (cputrace.prefetch020_valid[i]);
		}
		save_u16(cputrace.pipeline_pos);
		save_u16(cputrace.pipeline_r8[0]);
		save_u16(cputrace.pipeline_r8[1]);
		save_u16(cputrace.pipeline_stop);
	}

	save_u16(cputrace.ird);
	save_u16(cputrace.read_buffer);
	save_u16(cputrace.writecounter);

	save_u32(cputrace.startcycles >> 32);

	for (int i = 0; i < cputrace.memoryoffset; i++) {
		save_u32(cputrace.ctm[i].flags | 4);
	}

	*len = dst - dstbak;
	cputrace.needendcycles = 1;
	return dstbak;
}

uae_u8 *restore_cpu_trace(uae_u8 *src)
{
	cpu_tracer = 0;
	cputrace.state = 0;
	uae_u32 v = restore_u32 ();
	if (!(v & 2))
		return src;
	cputrace.opcode = restore_u16 ();
	for (int i = 0; i < 16; i++)
		cputrace.regs[i] = restore_u32 ();
	cputrace.pc = restore_u32 ();
	cputrace.irc = restore_u16 ();
	cputrace.ir = restore_u16 ();
	cputrace.usp = restore_u32 ();
	cputrace.isp = restore_u32 ();
	cputrace.sr = restore_u16 ();
	cputrace.intmask = restore_u16 ();
	cputrace.stopped = restore_u16 ();
	cputrace.state = restore_u16 ();
	cputrace.cyclecounter = restore_u32 ();
	cputrace.cyclecounter_pre = restore_u32 ();
	cputrace.cyclecounter_post = restore_u32 ();
	cputrace.readcounter = restore_u32 ();
	cputrace.writecounter = restore_u32 ();
	cputrace.memoryoffset = restore_u32 ();
	for (int i = 0; i < cputrace.memoryoffset; i++) {
		cputrace.ctm[i].addr = restore_u32 ();
		cputrace.ctm[i].data = restore_u32 ();
		cputrace.ctm[i].mode = restore_u32 ();
	}
	cputrace.startcycles = restore_u32();

	if (v & 4) {
		if (currprefs.cpu_model == 68020) {
			for (int i = 0; i < CACHELINES020; i++) {
				cputrace.caches020[i].data = restore_u32 ();
				cputrace.caches020[i].tag = restore_u32 ();
				cputrace.caches020[i].valid = restore_u8 () != 0;
			}
			cputrace.prefetch020addr = restore_u32 ();
			cputrace.cacheholdingaddr020 = restore_u32 ();
			cputrace.cacheholdingdata020 = restore_u32 ();
			for (int i = 0; i < CPU_PIPELINE_MAX; i++) {
				cputrace.prefetch020[i] = restore_u16 ();
			}
			if (v & 8) {
				// backwards compatibility
				uae_u32 v2 = restore_u32();
				cputrace.prefetch020[0] = v2 >> 16;
				cputrace.prefetch020[1] = static_cast<uae_u16>(v2);
				v2 = restore_u32();
				cputrace.prefetch020[2] = v2 >> 16;
				cputrace.prefetch020[3] = static_cast<uae_u16>(v2);
				restore_u32();
				restore_u32();
				cputrace.prefetch020_valid[0] = true;
				cputrace.prefetch020_valid[1] = true;
				cputrace.prefetch020_valid[2] = true;
				cputrace.prefetch020_valid[3] = true;

				cputrace.prefetch020[0] = cputrace.prefetch020[1];
				cputrace.prefetch020[1] = cputrace.prefetch020[2];
				cputrace.prefetch020[2] = cputrace.prefetch020[3];
				cputrace.prefetch020_valid[3] = false;
			}
			if (v & 16) {
				if ((v & 32) && !(v & 8)) {
					restore_u32();
					restore_u32();
					restore_u32();
					restore_u32();
				}
				for (int i = 0; i < CPU_PIPELINE_MAX; i++) {
					cputrace.prefetch020_valid[i] = restore_u8() != 0;
				}
			}
			if (v & 32) {
				cputrace.pipeline_pos = restore_u16();
				cputrace.pipeline_r8[0] = restore_u16();
				cputrace.pipeline_r8[1] = restore_u16();
				cputrace.pipeline_stop = restore_u16();
			}
		}
		if (v & 64) {
			cputrace.ird = restore_u16();
			cputrace.read_buffer = restore_u16();
			cputrace.write_buffer = restore_u16();
		}

		if (v & 128) {
			cputrace.startcycles |= static_cast<uae_u64>(restore_u32()) << 32;
			for (int i = 0; i < cputrace.memoryoffset; i++) {
				cputrace.ctm[i].flags = restore_u32();
			}
		}
	}

	cputrace.needendcycles = 1;
	if (v && cputrace.state) {
		if (currprefs.cpu_model > 68000) {
			if (v & 4)
				cpu_tracer = -1;
			// old format?
			if ((v & (4 | 8)) != (4 | 8) && (v & (32 | 16 | 8 | 4)) != (32 | 16 | 4))
				cpu_tracer = 0;
		} else {
			cpu_tracer = -1;
		}
	}

	return src;
}

uae_u8 *restore_cpu_extra(uae_u8 *src)
{
	restore_u32 ();
	uae_u32 flags = restore_u32 ();

	currprefs.cpu_cycle_exact = changed_prefs.cpu_cycle_exact = (flags & 1) ? true : false;
	currprefs.cpu_memory_cycle_exact = changed_prefs.cpu_memory_cycle_exact = currprefs.cpu_cycle_exact;
	if ((flags & 32) && !(flags & 1))
		currprefs.cpu_memory_cycle_exact = changed_prefs.cpu_memory_cycle_exact = true;
	currprefs.blitter_cycle_exact = changed_prefs.blitter_cycle_exact = currprefs.cpu_cycle_exact;
	currprefs.cpu_compatible = changed_prefs.cpu_compatible = (flags & 2) ? true : false;
	currprefs.cpu_frequency = changed_prefs.cpu_frequency = restore_u32 ();
	currprefs.cpu_clock_multiplier = changed_prefs.cpu_clock_multiplier = restore_u32 ();
	//currprefs.cachesize = changed_prefs.cachesize = (flags & 8) ? 8192 : 0;

	currprefs.m68k_speed = changed_prefs.m68k_speed = 0;
	if (flags & 4)
		currprefs.m68k_speed = changed_prefs.m68k_speed = -1;
	if (flags & 16)
		currprefs.m68k_speed = changed_prefs.m68k_speed = (flags >> 24) * CYCLE_UNIT;

	currprefs.cpu060_revision = changed_prefs.cpu060_revision = restore_u8 ();
	currprefs.fpu_revision = changed_prefs.fpu_revision = restore_u8 ();

	return src;
}

uae_u8 *save_cpu_extra(size_t *len, uae_u8 *dstptr)
{
	uae_u8 *dstbak, *dst;
	uae_u32 flags;

	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc (uae_u8, 1000);
	save_u32 (0); // version
	flags = 0;
	flags |= currprefs.cpu_cycle_exact ? 1 : 0;
	flags |= currprefs.cpu_compatible ? 2 : 0;
	flags |= currprefs.m68k_speed < 0 ? 4 : 0;
	flags |= currprefs.cachesize > 0 ? 8 : 0;
	flags |= currprefs.m68k_speed > 0 ? 16 : 0;
	flags |= currprefs.cpu_memory_cycle_exact ? 32 : 0;
	if (currprefs.m68k_speed > 0)
		flags |= (currprefs.m68k_speed / CYCLE_UNIT) << 24;
	save_u32 (flags);
	save_u32 (currprefs.cpu_frequency);
	save_u32 (currprefs.cpu_clock_multiplier);
	save_u8 (currprefs.cpu060_revision);
	save_u8 (currprefs.fpu_revision);
	*len = dst - dstbak;
	return dstbak;
}

uae_u8 *save_cpu(size_t *len, uae_u8 *dstptr)
{
	uae_u8 *dstbak, *dst;
	int model, khz;

	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc (uae_u8, 1000 + 30000);
	model = currprefs.cpu_model;
	save_u32 (model);					/* MODEL */
	save_u32(0x80000000 | 0x40000000 | 0x20000000 | 0x10000000 | 0x8000000 | 0x4000000 | 0x2000000 | (currprefs.address_space_24 ? 1 : 0)); /* FLAGS */
	for (int i = 0;i < 15; i++)
		save_u32 (regs.regs[i]);		/* D0-D7 A0-A6 */
	save_u32 (m68k_getpc ());			/* PC */
	save_u16 (regs.irc);				/* prefetch */
	save_u16 (regs.ir);					/* instruction prefetch */
	MakeSR ();
	save_u32 (!regs.s ? regs.regs[15] : regs.usp);	/* USP */
	save_u32 (regs.s ? regs.regs[15] : regs.isp);	/* ISP */
	save_u16 (regs.sr);								/* SR/CCR */
	save_u32 (regs.stopped == 1 ? CPUMODE_HALT : (regs.stopped == 2 ? (CPUMODE_HALT | CPUMODE_HALT2) : 0)); /* flags */
	if (model >= 68010) {
		save_u32 (regs.dfc);			/* DFC */
		save_u32 (regs.sfc);			/* SFC */
		save_u32 (regs.vbr);			/* VBR */
	}
	if (model >= 68020) {
		save_u32 (regs.caar);			/* CAAR */
		save_u32 (regs.cacr);			/* CACR */
		save_u32 (regs.msp);			/* MSP */
	}
	if (model >= 68030) {
		if (currprefs.mmu_model) {
			save_u64 (crp_030);				/* CRP */
			save_u64 (srp_030);				/* SRP */
			save_u32 (tt0_030);				/* TT0/AC0 */
			save_u32 (tt1_030);				/* TT1/AC1 */
			save_u32 (tc_030);				/* TCR */
			save_u16 (mmusr_030);			/* MMUSR/ACUSR */
		} else {
			save_u64 (fake_crp_030);		/* CRP */
			save_u64 (fake_srp_030);		/* SRP */
			save_u32 (fake_tt0_030);		/* TT0/AC0 */
			save_u32 (fake_tt1_030);		/* TT1/AC1 */
			save_u32 (fake_tc_030);			/* TCR */
			save_u16 (fake_mmusr_030);		/* MMUSR/ACUSR */
		}
	}
	if (model >= 68040) {
		save_u32 (regs.itt0);			/* ITT0 */
		save_u32 (regs.itt1);			/* ITT1 */
		save_u32 (regs.dtt0);			/* DTT0 */
		save_u32 (regs.dtt1);			/* DTT1 */
		save_u32 (regs.tcr);			/* TCR */
		save_u32 (regs.urp);			/* URP */
		save_u32 (regs.srp);			/* SRP */
	}
	if (model >= 68060) {
		save_u32 (regs.buscr);			/* BUSCR */
		save_u32 (regs.pcr);			/* PCR */
	}
	khz = -1;
	if (currprefs.m68k_speed == 0) {
		khz = currprefs.ntscmode ? 715909 : 709379;
		if (currprefs.cpu_model >= 68020)
			khz *= 2;
	}
	save_u32 (khz); // clock rate in KHz: -1 = fastest possible
	save_u32 (0); // spare
	if (model == 68020) {
		for (int i = 0; i < CACHELINES020; i++) {
			save_u32 (caches020[i].data);
			save_u32 (caches020[i].tag);
			save_u8 (caches020[i].valid ? 1 : 0);
		}
		save_u32 (regs.prefetch020addr);
		save_u32 (regs.cacheholdingaddr020);
		save_u32 (regs.cacheholdingdata020);
		for (int i = 0; i < CPU_PIPELINE_MAX; i++)
			save_u32 ((regs.prefetch020[i] << 16) | (regs.prefetch020_valid[i] ? 1 : 0));
	} else if (model == 68030) {
		for (int i = 0; i < CACHELINES030; i++) {
			for (int j = 0; j < 4; j++) {
				save_u32 (icaches030[i].data[j]);
				save_u8 (icaches030[i].valid[j] ? 1 : 0);
			}
			save_u32 (icaches030[i].tag);
		}
		for (int i = 0; i < CACHELINES030; i++) {
			for (int j = 0; j < 4; j++) {
				save_u32 (dcaches030[i].data[j]);
				save_u8 (dcaches030[i].valid[j] ? 1 : 0);
			}
			save_u32 (dcaches030[i].tag);
		}
		save_u32 (regs.prefetch020addr);
		save_u32 (regs.cacheholdingaddr020);
		save_u32 (regs.cacheholdingdata020);
		for (int i = 0; i < CPU_PIPELINE_MAX; i++)
			save_u32 (regs.prefetch020[i]);
	} else if (model >= 68040) {
		for (int i = 0; i < (model == 68060 ? CACHESETS060 : CACHESETS040); i++) {
			for (int j = 0; j < CACHELINES040; j++) {
				struct cache040 *c = &icaches040[i];
				save_u32(c->data[j][0]);
				save_u32(c->data[j][1]);
				save_u32(c->data[j][2]);
				save_u32(c->data[j][3]);
				save_u32(c->tag[j]);
				save_u16(c->valid[j] ? 1 : 0);
			}
		}
		save_u32(regs.prefetch020addr);
		save_u32(regs.cacheholdingaddr020);
		save_u32(regs.cacheholdingdata020);
		for (int i = 0; i < CPU_PIPELINE_MAX; i++) {
			save_u32(regs.prefetch040[i]);
		}
		for (int i = 0; i < (model == 68060 ? CACHESETS060 : CACHESETS040); i++) {
			for (int j = 0; j < CACHELINES040; j++) {
				struct cache040 *c = &dcaches040[i];
				save_u32(c->data[j][0]);
				save_u32(c->data[j][1]);
				save_u32(c->data[j][2]);
				save_u32(c->data[j][3]);
				save_u32(c->tag[j]);
				uae_u16 v = c->valid[j] ? 1 : 0;
				v |= c->dirty[j][0] ? 0x10 : 0;
				v |= c->dirty[j][1] ? 0x20 : 0;
				v |= c->dirty[j][2] ? 0x40 : 0;
				v |= c->dirty[j][3] ? 0x80 : 0;
				save_u16(v);
			}
		}
	}
	if (currprefs.cpu_model >= 68020) {
		save_u32 (0); //save_u32 (regs.ce020memcycles);
		save_u32 (0);
	}
	save_u32 (regs.chipset_latch_rw);
	save_u32 (regs.chipset_latch_read);
	save_u32 (regs.chipset_latch_write);
	if (currprefs.cpu_model == 68020) {
		save_u16(regs.pipeline_pos);
		save_u16(regs.pipeline_r8[0]);
		save_u16(regs.pipeline_r8[1]);
		save_u16(regs.pipeline_stop);
	}
	if (currprefs.cpu_model <= 68010) {
		save_u32(1);
		save_u16(regs.ird);
		save_u16(regs.read_buffer);
		save_u16(regs.write_buffer);
		save_s8(regs.ipl[0]);
		save_s8(regs.ipl[1]);
		save_s8(regs.ipl_pin);
		save_s8(regs.ipl_pin_p);
		save_u64(regs.ipl_evt);
		save_u64(regs.ipl_evt_pre);
		save_u64(regs.ipl_pin_change_evt);
		save_u64(regs.ipl_pin_change_evt_p);
	}
	*len = dst - dstbak;
	return dstbak;
}

uae_u8 *save_mmu(size_t *len, uae_u8 *dstptr)
{
	uae_u8 *dstbak, *dst;
	int model;

	model = currprefs.mmu_model;
	if (model != 68030 && model != 68040 && model != 68060)
		return nullptr;
	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc (uae_u8, 1000);
	save_u32 (model);	/* MODEL */
	save_u32 (0);		/* FLAGS */
	*len = dst - dstbak;
	return dstbak;
}

uae_u8 *restore_mmu(uae_u8 *src)
{
	int flags, model;

	changed_prefs.mmu_model = model = restore_u32 ();
	flags = restore_u32 ();
	write_log (_T("MMU: %d\n"), model);
	return src;
}

#endif /* SAVESTATE */

static void exception3f(uae_u32 opcode, uaecptr addr, bool writeaccess, bool instructionaccess, bool notinstruction, uaecptr pc, int size, int fc, uae_u16 secondarysr)
{
	if (currprefs.cpu_model >= 68040)
		addr &= ~1;
	if (currprefs.cpu_model >= 68020) {
		if (pc == 0xffffffff)
			last_addr_for_exception_3 = regs.instruction_pc;
		else
			last_addr_for_exception_3 = pc;
	} else if (pc == 0xffffffff) {
		last_addr_for_exception_3 = m68k_getpc();
	} else {
		last_addr_for_exception_3 = pc;
	}
	last_fault_for_exception_3 = addr;
	last_op_for_exception_3 = opcode;
	last_writeaccess_for_exception_3 = writeaccess;
	last_fc_for_exception_3 = fc >= 0 ? fc : (instructionaccess ? 2 : 1);
	last_notinstruction_for_exception_3 = notinstruction;
	last_size_for_exception_3 = size;
	last_sr_for_exception3 = secondarysr;
	Exception (3);
#if EXCEPTION3_DEBUGGER
	activate_debugger();
#endif
}

void exception3_notinstruction(uae_u32 opcode, uaecptr addr)
{
	last_di_for_exception_3 = 1;
	exception3f (opcode, addr, true, false, true, 0xffffffff, 1, -1, 0);
}
static void exception3_read_special(uae_u32 opcode, uaecptr addr, int size, int fc)
{
	exception3f(opcode, addr, false, false, false, 0xffffffff, size, fc, 0);
}

// 68010 special prefetch handling
void exception3_read_prefetch_only(uae_u32 opcode, uaecptr addr)
{
	if (currprefs.cpu_model == 68010) {
		uae_u16 prev = regs.read_buffer;
		x_get_word(addr & ~1);
		regs.irc = regs.read_buffer;
	} else {
		x_do_cycles(4 * cpucycleunit);
	}
	last_di_for_exception_3 = 0;
	exception3f(opcode, addr, false, true, false, m68k_getpc(), sz_word, -1, 0);
}

// Some hardware accepts address error aborted reads or writes as normal reads/writes.
void exception3_read_prefetch(uae_u32 opcode, uaecptr addr)
{
	x_do_cycles(4 * cpucycleunit);
	last_di_for_exception_3 = 0;
	if (currprefs.cpu_model == 68000) {
		m68k_incpci(2);
	}
	exception3f(opcode, addr, false, true, false, m68k_getpc(), sz_word, -1, 0);
}
void exception3_read_prefetch_68040bug(uae_u32 opcode, uaecptr addr, uae_u16 secondarysr)
{
	x_do_cycles(4 * cpucycleunit);
	last_di_for_exception_3 = 0;
	exception3f(opcode | 0x10000, addr, false, true, false, m68k_getpc(), sz_word, -1, secondarysr);
}

void exception3_read_access(uae_u32 opcode, uaecptr addr, int size, int fc)
{
	x_do_cycles(4 * cpucycleunit);
	exception3_read(opcode, addr, size, fc);
}
void exception3_read_access2(uae_u32 opcode, uaecptr addr, int size, int fc)
{
	// (An), -(An) and (An)+ and 68010: read happens twice!
	x_do_cycles(8 * cpucycleunit);
	exception3_read(opcode, addr, size, fc);
}
void exception3_write_access(uae_u32 opcode, uaecptr addr, int size, uae_u32 val, int fc)
{
	x_do_cycles(4 * cpucycleunit);
	exception3_write(opcode, addr, size, val, fc);
}

void exception3_read(uae_u32 opcode, uaecptr addr, int size, int fc)
{
	bool ni = false;
	bool ia = false;
	if (currprefs.cpu_model == 68000 && currprefs.cpu_compatible) {
		if (generates_group1_exception(regs.ir) && !(opcode & 0x20000)) {
			ni = true;
			fc = -1;
		}
		if (opcode & 0x10000)
			ni = true;
		if (opcode & 0x40000)
			ia = true;
		opcode = regs.ir;
	}
	last_di_for_exception_3 = 1;
	exception3f(opcode, addr, false, ia, ni, 0xffffffff, size & 15, fc, 0);
}
void exception3_write(uae_u32 opcode, uaecptr addr, int size, uae_u32 val, int fc)
{
	bool ni = false;
	bool ia = false;
	if (currprefs.cpu_model == 68000 && currprefs.cpu_compatible) {
		if (generates_group1_exception(regs.ir) && !(opcode & 0x20000)) {
			ni = true;
			fc = -1;
		}
		if (opcode & 0x10000)
			ni = true;
		if (opcode & 0x40000)
			ia = true;
		opcode = regs.ir;
	}
	last_di_for_exception_3 = 1;
	regs.write_buffer = val;
	exception3f(opcode, addr, true, ia, ni, 0xffffffff, size & 15, fc, 0);
}

void exception2_setup(uae_u32 opcode, uaecptr addr, bool read, int size, uae_u32 fc)
{
	last_addr_for_exception_3 = m68k_getpc();
	last_fault_for_exception_3 = addr;
	last_writeaccess_for_exception_3 = read == 0;
	last_op_for_exception_3 = opcode;
	last_fc_for_exception_3 = fc;
	last_notinstruction_for_exception_3 = exception_in_exception != 0;
	last_size_for_exception_3 = size & 15;
	last_di_for_exception_3 = 1;
	hardware_bus_error = 0;

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

// Common hardware bus error entry point. Both for MMU and non-MMU emulation.
void hardware_exception2(uaecptr addr, uae_u32 v, bool read, bool ins, int size)
{
	if (currprefs.cpu_model <= 68010 && currprefs.cpu_compatible && HARDWARE_BUS_ERROR_EMULATION) {
		hardware_bus_error = 1;
	} else if (currprefs.mmu_model) {
		if (currprefs.mmu_model == 68030) {
			 mmu030_hardware_bus_error(addr, v, read, ins, size);
		} else {
			mmu_hardware_bus_error(addr, v, read, ins, size);
		}
		return;
	} else {
		int fc = (regs.s ? 4 : 0) | (ins ? 2 : 1);
		if (ismoves_nommu) {
			ismoves_nommu = false;
			fc = read ? regs.sfc : regs.dfc;
		}
		// Non-MMU
		exception2_setup(regs.opcode, addr, read, size, fc);
		THROW(2);
	}
}

void exception2_read(uae_u32 opcode, uaecptr addr, int size, int fc)
{
	exception2_setup(opcode, addr, true, size & 15, fc);
	Exception(2);
}

void exception2_write(uae_u32 opcode, uaecptr addr, int size, uae_u32 val, int fc)
{
	exception2_setup(opcode, addr, false, size & 15, fc);
	if (size & 0x100) {
		regs.write_buffer = val;
	} else {
		if (size == sz_byte) {
			regs.write_buffer &= 0xff00;
			regs.write_buffer |= val & 0xff;
		} else {
			regs.write_buffer = val;
		}
	}
	Exception(2);
}

static void exception2_fetch_common(uae_u32 opcode, int offset)
{
	last_fault_for_exception_3 = m68k_getpc() + offset;
	// this is not yet fully correct
	last_addr_for_exception_3 = last_fault_for_exception_3;
	last_writeaccess_for_exception_3 = false;
	last_op_for_exception_3 = opcode;
	last_fc_for_exception_3 = 2;
	last_notinstruction_for_exception_3 = exception_in_exception != 0;
	last_size_for_exception_3 = sz_word;
	last_di_for_exception_3 = 0;
	hardware_bus_error = 0;

	if (currprefs.cpu_model == 68000 && currprefs.cpu_compatible) {
		if (generates_group1_exception(regs.ir) && !(opcode & 0x20000)) {
			last_fc_for_exception_3 |= 8;  // set N/I
		}
		if (opcode & 0x10000)
			last_fc_for_exception_3 |= 8;
	}
}

void exception2_fetch_opcode(uae_u32 opcode, int offset, int pcoffset)
{
	exception2_fetch_common(opcode, offset);
	last_addr_for_exception_3 += pcoffset;
	if (currprefs.cpu_model == 68010) {
		last_di_for_exception_3 = -1;
	}
	Exception(2);
}

void exception2_fetch(uae_u32 opcode, int offset, int pcoffset)
{
	exception2_fetch_common(opcode, offset);
	last_addr_for_exception_3 += pcoffset;
	Exception(2);
}

bool cpureset (void)
{
	/* RESET hasn't increased PC yet, 1 word offset */
	uaecptr pc;
	uaecptr ksboot = 0xf80002 - 2;
	uae_u16 ins;
	addrbank *ab;
	bool extreset = false;

	maybe_disable_fpu();
	m68k_reset_delay = currprefs.reset_delay;
	set_special(SPCFLAG_CHECK);
	unset_special(SPCFLAG_CPUINRESET);
	send_internalevent(INTERNALEVENT_CPURESET);
	warpmode_reset();
	if (cpuboard_forced_hardreset()) {
		custom_reset_cpu(false, false);
		m68k_reset();
		return true;
	}
	if (currprefs.cpu_compatible || currprefs.cpu_memory_cycle_exact) {
		custom_reset_cpu(false, false);
		return false;
	}
	pc = m68k_getpc () + 2;
	ab = &get_mem_bank (pc);
	if (ab->check (pc, 2)) {
		write_log (_T("CPU reset PC=%x (%s)..\n"), pc - 2, ab->name);
		ins = get_word (pc);
		custom_reset_cpu(false, false);
		// did memory disappear under us?
		if (ab == &get_mem_bank (pc))
			return false;
		// it did
		if ((ins & ~7) == 0x4ed0) {
			int reg = ins & 7;
			uae_u32 addr = m68k_areg (regs, reg);
			if (addr < 0x80000)
				addr += 0xf80000;
			write_log (_T("reset/jmp (ax) combination at %08x emulated -> %x\n"), pc, addr);
			m68k_setpc_normal (addr - 2);
			return false;
		}
	}
	// the best we can do, jump directly to ROM entrypoint
	// (which is probably what program wanted anyway)
	write_log (_T("CPU Reset PC=%x (%s), invalid memory -> %x.\n"), pc, ab->name, ksboot + 2);
	custom_reset_cpu(false, false);
	m68k_setpc_normal (ksboot);
	return false;
}

void do_cycles_stop(int c)
{
	c *= cpucycleunit;
	if (!currprefs.cpu_compatible) {
		do_cycles(c);
	} else {
#ifdef DEBUGGER
		if (debug_dma) {
			while (c > 0) {
				debug_cpu_stop();
				x_do_cycles(c > CYCLE_UNIT ? CYCLE_UNIT : c);
				c -= CYCLE_UNIT;
			}
		} else {
#endif
			x_do_cycles(c);
#ifdef DEBUGGER
		}
#endif
	}
}

void m68k_setstopped(int stoptype)
{
	m68k_set_stop(stoptype);
}

void m68k_resumestopped(void)
{
	if (!regs.stopped) {
		return;
	}
	if (regs.stopped == 1) {
		// STOP
		m68k_incpci(4);
	} else if (regs.stopped == 2) {
		// LPSTOP
		m68k_incpci(6);
	}
	m68k_unset_stop();
}


uae_u32 mem_access_delay_word_read (uaecptr addr)
{
	uae_u32 v;
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP16:
	case CE_MEMBANK_CHIP32:
		v = wait_cpu_cycle_read (addr, 1);
		break;
	case CE_MEMBANK_FAST16:
	case CE_MEMBANK_FAST32:
		v = get_word (addr);
		x_do_cycles_post (4 * cpucycleunit, v);
		break;
	default:
		v = get_word (addr);
		break;
	}
	regs.db = v;
	regs.read_buffer = v;
	return v;
}
uae_u32 mem_access_delay_wordi_read (uaecptr addr)
{
	uae_u32 v;
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP16:
	case CE_MEMBANK_CHIP32:
		v = wait_cpu_cycle_read (addr, 2);
		break;
	case CE_MEMBANK_FAST16:
	case CE_MEMBANK_FAST32:
		v = get_wordi (addr);
		x_do_cycles_post (4 * cpucycleunit, v);
		break;
	default:
		v = get_wordi (addr);
		break;
	}
	regs.db = v;
	regs.read_buffer = v;
	return v;
}

uae_u32 mem_access_delay_byte_read (uaecptr addr)
{
	uae_u32  v;
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP16:
	case CE_MEMBANK_CHIP32:
		v = wait_cpu_cycle_read (addr, 0);
		break;
	case CE_MEMBANK_FAST16:
	case CE_MEMBANK_FAST32:
		v = get_byte (addr);
		x_do_cycles_post (4 * cpucycleunit, v);
		break;
	default:
		v = get_byte (addr);
		break;
	}
	regs.db = (v << 8) | v;
	regs.read_buffer = v;
	return v;
}
void mem_access_delay_byte_write (uaecptr addr, uae_u32 v)
{
	regs.db = (v << 8)  | v;
	regs.write_buffer = v;
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP16:
	case CE_MEMBANK_CHIP32:
		wait_cpu_cycle_write (addr, 0, v);
		return;
	case CE_MEMBANK_FAST16:
	case CE_MEMBANK_FAST32:
		put_byte (addr, v);
		x_do_cycles_post (4 * cpucycleunit, v);
		return;
	}
	put_byte (addr, v);
}
void mem_access_delay_word_write (uaecptr addr, uae_u32 v)
{
	regs.db = v;
	regs.write_buffer = v;
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP16:
	case CE_MEMBANK_CHIP32:
		wait_cpu_cycle_write (addr, 1, v);
		return;
	case CE_MEMBANK_FAST16:
	case CE_MEMBANK_FAST32:
		put_word (addr, v);
		x_do_cycles_post (4 * cpucycleunit, v);
		return;
	}
	put_word (addr, v);
}

static void start_020_cycle()
{
	regs.ce020startcycle = get_cycles();
}

static void start_020_cycle_prefetch(bool opcode)
{
	regs.ce020startcycle = get_cycles();
	// back to back prefetch cycles require 2 extra cycles (maybe)
	if (opcode && regs.ce020startcycle == regs.ce020prefetchendcycle && currprefs.cpu_cycle_exact) {
		x_do_cycles(2 * cpucycleunit);
		regs.ce020startcycle = get_cycles();
	}
}
static void end_020_cycle()
{
	regs.ce020endcycle = get_cycles();
}
static void end_020_cycle_prefetch(bool opcode)
{
	regs.ce020endcycle = get_cycles();
	if (opcode) {
		regs.ce020prefetchendcycle = regs.ce020endcycle;
	} else {
		regs.ce020prefetchendcycle = regs.ce020startcycle;
	}
}

// this one is really simple and easy
static void fill_icache020 (uae_u32 addr, bool opcode)
{
	int index;
	uae_u32 tag;
	uae_u32 data;
	struct cache020 *c;

	regs.fc030 = (regs.s ? 4 : 0) | 2;
	addr &= ~3;
	if (regs.cacheholdingaddr020 == addr)
		return;
	index = (addr >> 2) & (CACHELINES020 - 1);
	tag = regs.s | (addr & ~((CACHELINES020 << 2) - 1));
	c = &caches020[index];
	if ((regs.cacr & 1) && c->valid && c->tag == tag) {
		// cache hit
		regs.cacheholdingaddr020 = addr;
		regs.cacheholdingdata020 = c->data;
		regs.cacheholdingdata_valid = 1;
		return;
	}

	// cache miss
#if 0
	// Prefetch apparently can be queued by bus controller
	// even if bus controller is currently processing
	// previous data access.
	// Other combinations are not possible.
	if (!regs.ce020memcycle_data) {
		if (regs.ce020memcycles > 0)
			x_do_cycles (regs.ce020memcycles);
		regs.ce020memcycles = 0;
	}
#endif

	start_020_cycle_prefetch(opcode);
	data = icache_fetch(addr);
	end_020_cycle_prefetch(opcode);

	// enabled and not frozen
	if ((regs.cacr & 1) && !(regs.cacr & 2)) {
		c->tag = tag;
		c->valid = true;
		c->data = data;
	}
	regs.cacheholdingaddr020 = addr;
	regs.cacheholdingdata020 = data;
	regs.cacheholdingdata_valid = 1;
}

#if MORE_ACCURATE_68020_PIPELINE
#define PIPELINE_DEBUG 0
#if PIPELINE_DEBUG
static uae_u16 pipeline_opcode;
#endif
static void pipeline_020(uaecptr pc)
{
	uae_u16 w = regs.prefetch020[1];

	if (regs.prefetch020_valid[1] == 0) {
		regs.pipeline_stop = -1;
		return;
	}
	if (regs.pipeline_pos < 0)
		return;
	if (regs.pipeline_pos > 0) {
		// handle annoying 68020+ addressing modes
		if (regs.pipeline_pos == regs.pipeline_r8[0]) {
			regs.pipeline_r8[0] = 0;
			if (w & 0x100) {
				int extra = 0;
				if ((w & 0x30) == 0x20)
					extra += 2;
				if ((w & 0x30) == 0x30)
					extra += 4;
				if ((w & 0x03) == 0x02)
					extra += 2;
				if ((w & 0x03) == 0x03)
					extra += 4;
				regs.pipeline_pos += extra;
			}
			return;
		}
		if (regs.pipeline_pos == regs.pipeline_r8[1]) {
			regs.pipeline_r8[1] = 0;
			if (w & 0x100) {
				int extra = 0;
				if ((w & 0x30) == 0x20)
					extra += 2;
				if ((w & 0x30) == 0x30)
					extra += 4;
				if ((w & 0x03) == 0x02)
					extra += 2;
				if ((w & 0x03) == 0x03)
					extra += 4;
				regs.pipeline_pos += extra;
			}
			return;
		}
	}
	if (regs.pipeline_pos > 2) {
		regs.pipeline_pos -= 2;
		// If stop set, prefetches stop 1 word early.
		if (regs.pipeline_stop > 0 && regs.pipeline_pos == 2)
			regs.pipeline_stop = -1;
		return;
	}
	if (regs.pipeline_stop) {
		regs.pipeline_stop = -1;
		return;
	}
#if PIPELINE_DEBUG
	pipeline_opcode = w;
#endif
	regs.pipeline_r8[0] = cpudatatbl[w].disp020[0];
	regs.pipeline_r8[1] = cpudatatbl[w].disp020[1];
	regs.pipeline_pos = cpudatatbl[w].length;
#if PIPELINE_DEBUG
	if (!regs.pipeline_pos) {
		write_log(_T("Opcode %04x has no size PC=%08x!\n"), w, pc);
	}
#endif
	// illegal instructions, TRAP, TRAPV, A-line, F-line don't stop prefetches
	int branch = cpudatatbl[w].branch;
	if (regs.pipeline_pos > 0 && branch > 0) {
		// Short branches (Bcc.s) still do one more prefetch.
#if 0
		// RTS and other unconditional single opcode instruction stop immediately.
		if (branch == 2) {
			// Immediate stop
			regs.pipeline_stop = -1;
		} else {
			// Stop 1 word early than normally
			regs.pipeline_stop = 1;
		}
#else
		regs.pipeline_stop = 1;
#endif
	}
}

#endif

static uae_u32 get_word_ce020_prefetch_2 (int o, bool opcode)
{
	uae_u32 pc = m68k_getpc () + o;
	uae_u32 v;

	v = regs.prefetch020[0];
	regs.prefetch020[0] = regs.prefetch020[1];
	regs.prefetch020[1] = regs.prefetch020[2];
#if MORE_ACCURATE_68020_PIPELINE
	pipeline_020(pc);
#endif
	if (pc & 2) {
		// branch instruction detected in pipeline: stop fetches until branch executed.
		if (!MORE_ACCURATE_68020_PIPELINE || regs.pipeline_stop >= 0) {
			fill_icache020 (pc + 2 + 4, opcode);
		}
		regs.prefetch020[2] = regs.cacheholdingdata020 >> 16;
	} else {
		regs.prefetch020[2] = static_cast<uae_u16>(regs.cacheholdingdata020);
	}
	regs.db = regs.prefetch020[0];
	do_cycles_ce020_internal(2);
	return v;
}

uae_u32 get_word_ce020_prefetch (int o)
{
	return get_word_ce020_prefetch_2(o, false);
}

uae_u32 get_word_ce020_prefetch_opcode (int o)
{
	return get_word_ce020_prefetch_2(o, true);
}

uae_u32 get_word_020_prefetch (int o)
{
	uae_u32 pc = m68k_getpc () + o;
	uae_u32 v;

	v = regs.prefetch020[0];
	regs.prefetch020[0] = regs.prefetch020[1];
	regs.prefetch020[1] = regs.prefetch020[2];
#if MORE_ACCURATE_68020_PIPELINE
	pipeline_020(pc);
#endif
	if (pc & 2) {
		// branch instruction detected in pipeline: stop fetches until branch executed.
		if (!MORE_ACCURATE_68020_PIPELINE || regs.pipeline_stop >= 0) {
			fill_icache020 (pc + 2 + 4, false);
		}
		regs.prefetch020[2] = regs.cacheholdingdata020 >> 16;
	} else {
		regs.prefetch020[2] = static_cast<uae_u16>(regs.cacheholdingdata020);
	}
	regs.db = regs.prefetch020[0];
	return v;
}

// these are also used by 68030.

#if 0
#define RESET_CE020_CYCLES \
	regs.ce020memcycles = 0; \
	regs.ce020memcycle_data = true;
#define STORE_CE020_CYCLES \
	unsigned long cycs = get_cycles ()
#define ADD_CE020_CYCLES \
	regs.ce020memcycles += get_cycles () - cycs
#endif

uae_u32 mem_access_delay_long_read_ce020 (uaecptr addr)
{
	uae_u32 v;
	start_020_cycle();
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP16:
		v  = wait_cpu_cycle_read_ce020 (addr + 0, 1) << 16;
		v |= wait_cpu_cycle_read_ce020 (addr + 2, 1) <<  0;
		break;
	case CE_MEMBANK_CHIP32:
		if ((addr & 3) != 0) {
			v  = wait_cpu_cycle_read_ce020 (addr + 0, 1) << 16;
			v |= wait_cpu_cycle_read_ce020 (addr + 2, 1) <<  0;
		} else {
			v = wait_cpu_cycle_read_ce020 (addr, -1);
		}
		break;
	case CE_MEMBANK_FAST32:
		v = get_long (addr);
		if ((addr & 3) != 0)
			do_cycles_ce020_mem (2 * CPU020_MEM_CYCLE, v);
		else
			do_cycles_ce020_mem (1 * CPU020_MEM_CYCLE, v);
		break;
	case CE_MEMBANK_FAST16:
		v = get_long (addr);
		do_cycles_ce020_mem (2 * CPU020_MEM_CYCLE, v);
		break;
	default:
		v = get_long (addr);
		break;
	}
	end_020_cycle();
	return v;
}

uae_u32 mem_access_delay_longi_read_ce020 (uaecptr addr)
{
	uae_u32 v;
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP16:
		v  = wait_cpu_cycle_read_ce020 (addr + 0, 2) << 16;
		v |= wait_cpu_cycle_read_ce020 (addr + 2, 2) <<  0;
		break;
	case CE_MEMBANK_CHIP32:
		if ((addr & 3) != 0) {
			v  = wait_cpu_cycle_read_ce020 (addr + 0, 2) << 16;
			v |= wait_cpu_cycle_read_ce020 (addr + 2, 2) <<  0;
		} else {
			v = wait_cpu_cycle_read_ce020 (addr, -2);
		}
		break;
	case CE_MEMBANK_FAST32:
		v = get_longi (addr);
		if ((addr & 3) != 0)
			do_cycles_ce020_mem (2 * CPU020_MEM_CYCLE, v);
		else
			do_cycles_ce020_mem (1 * CPU020_MEM_CYCLE, v);
		break;
	case CE_MEMBANK_FAST16:
		v = get_longi (addr);
		do_cycles_ce020_mem (2 * CPU020_MEM_CYCLE, v);
		break;
	default:
		v = get_longi (addr);
		break;
	}
	return v;
}

uae_u32 mem_access_delay_wordi_read_ce020 (uaecptr addr)
{
	uae_u32 v;
	start_020_cycle();
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP16:
	case CE_MEMBANK_CHIP32:
		if ((addr & 3) == 3) {
			v  = wait_cpu_cycle_read_ce020 (addr + 0, 0) << 8;
			v |= wait_cpu_cycle_read_ce020 (addr + 1, 0) << 0;
		} else {
			v = wait_cpu_cycle_read_ce020 (addr, 1);
		}
		break;
	case CE_MEMBANK_FAST16:
	case CE_MEMBANK_FAST32:
		v = get_wordi (addr);
		if ((addr & 3) == 3)
			do_cycles_ce020_mem (2 * CPU020_MEM_CYCLE, v);
		else
			do_cycles_ce020_mem (1 * CPU020_MEM_CYCLE, v);
		 break;
	default:
		 v = get_wordi (addr);
		break;
	}
	end_020_cycle();
	return v;
}

uae_u32 mem_access_delay_word_read_ce020 (uaecptr addr)
{
	uae_u32 v;
	start_020_cycle();
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP16:
	case CE_MEMBANK_CHIP32:
		if ((addr & 3) == 3) {
			v  = wait_cpu_cycle_read_ce020 (addr + 0, 0) << 8;
			v |= wait_cpu_cycle_read_ce020 (addr + 1, 0) << 0;
		} else {
			v = wait_cpu_cycle_read_ce020 (addr, 1);
		}
		break;
	case CE_MEMBANK_FAST16:
	case CE_MEMBANK_FAST32:
		v = get_word (addr);
		if ((addr & 3) == 3)
			do_cycles_ce020_mem (2 * CPU020_MEM_CYCLE, v);
		else
			do_cycles_ce020_mem (1 * CPU020_MEM_CYCLE, v);
		 break;
	default:
		 v = get_word (addr);
		break;
	}
	end_020_cycle();
	return v;
}

uae_u32 mem_access_delay_byte_read_ce020 (uaecptr addr)
{
	uae_u32 v;
	start_020_cycle();
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP16:
	case CE_MEMBANK_CHIP32:
		v = wait_cpu_cycle_read_ce020 (addr, 0);
		break;
	case CE_MEMBANK_FAST16:
	case CE_MEMBANK_FAST32:
		v = get_byte (addr);
		do_cycles_ce020_mem (1 * CPU020_MEM_CYCLE, v);
		break;
	default:
		v = get_byte (addr);
		break;
	}
	end_020_cycle();
	return v;
}

void mem_access_delay_byte_write_ce020 (uaecptr addr, uae_u32 v)
{
	start_020_cycle();
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP16:
	case CE_MEMBANK_CHIP32:
		wait_cpu_cycle_write_ce020 (addr, 0, v);
		break;
	case CE_MEMBANK_FAST16:
	case CE_MEMBANK_FAST32:
		put_byte (addr, v);
		do_cycles_ce020_mem (1 * CPU020_MEM_CYCLE, v);
		break;
	default:
		put_byte (addr, v);
	break;
	}
	end_020_cycle();
}

void mem_access_delay_word_write_ce020 (uaecptr addr, uae_u32 v)
{
	start_020_cycle();
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP16:
	case CE_MEMBANK_CHIP32:
		if ((addr & 3) == 3) {
			wait_cpu_cycle_write_ce020 (addr + 0, 0, (v >> 8) & 0xff);
			wait_cpu_cycle_write_ce020 (addr + 1, 0, (v >> 0) & 0xff);
		} else {
			wait_cpu_cycle_write_ce020 (addr + 0, 1, v);
		}
		break;
	case CE_MEMBANK_FAST16:
	case CE_MEMBANK_FAST32:
		put_word (addr, v);
		if ((addr & 3) == 3)
			do_cycles_ce020_mem (2 * CPU020_MEM_CYCLE, v);
		else
			do_cycles_ce020_mem (1 * CPU020_MEM_CYCLE, v);
		break;
	default:
		put_word (addr, v);
	break;
	}
	end_020_cycle();
}

void mem_access_delay_long_write_ce020 (uaecptr addr, uae_u32 v)
{
	start_020_cycle();
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP16:
		wait_cpu_cycle_write_ce020 (addr + 0, 1, (v >> 16) & 0xffff);
		wait_cpu_cycle_write_ce020 (addr + 2, 1, (v >>  0) & 0xffff);
		break;
	case CE_MEMBANK_CHIP32:
		if ((addr & 3) != 0) {
			wait_cpu_cycle_write_ce020 (addr + 0, 1, (v >> 16) & 0xffff);
			wait_cpu_cycle_write_ce020 (addr + 2, 1, (v >>  0) & 0xffff);
		} else {
			wait_cpu_cycle_write_ce020 (addr + 0, -1, v);
		}
		break;
	case CE_MEMBANK_FAST32:
		put_long (addr, v);
		if ((addr & 3) != 0)
			do_cycles_ce020_mem (2 * CPU020_MEM_CYCLE, v);
		else
			do_cycles_ce020_mem (1 * CPU020_MEM_CYCLE, v);
		break;
	case CE_MEMBANK_FAST16:
		put_long (addr, v);
		do_cycles_ce020_mem (2 * CPU020_MEM_CYCLE, v);
		break;
	default:
		put_long (addr, v);
		break;
	}
	end_020_cycle();
}


// 68030 caches aren't so simple as 68020 cache..

STATIC_INLINE struct cache030 *geticache030 (struct cache030 *cp, uaecptr addr, uae_u32 *tagp, int *lwsp)
{
	int index, lws;
	uae_u32 tag;
	struct cache030 *c;

	addr &= ~3;
	index = (addr >> 4) & (CACHELINES030 - 1);
	tag = regs.s | (addr & ~((CACHELINES030 << 4) - 1));
	lws = (addr >> 2) & 3;
	c = &cp[index];
	*tagp = tag;
	*lwsp = lws;
	return c;
}

STATIC_INLINE void update_icache030 (struct cache030 *c, uae_u32 val, uae_u32 tag, int lws)
{
	if (c->tag != tag)
		c->valid[0] = c->valid[1] = c->valid[2] = c->valid[3] = false;
	c->tag = tag;
	c->valid[lws] = true;
	c->data[lws] = val;
}

STATIC_INLINE struct cache030 *getdcache030 (struct cache030 *cp, uaecptr addr, uae_u32 *tagp, int *lwsp)
{
	int index, lws;
	uae_u32 tag;
	struct cache030 *c;

	addr &= ~3;
	index = (addr >> 4) & (CACHELINES030 - 1);
	tag = addr & ~((CACHELINES030 << 4) - 1);
	lws = (addr >> 2) & 3;
	c = &cp[index];
	*tagp = tag;
	*lwsp = lws;
	return c;
}

STATIC_INLINE void update_dcache030 (struct cache030 *c, uae_u32 val, uae_u32 tag, uae_u8 fc, int lws)
{
	if (c->tag != tag)
		c->valid[0] = c->valid[1] = c->valid[2] = c->valid[3] = false;
	c->tag = tag;
	c->fc = fc;
	c->valid[lws] = true;
	c->data[lws] = val;
}

static bool maybe_icache030(uae_u32 addr)
{
	int lws;
	uae_u32 tag;
	struct cache030 *c;

	regs.fc030 = (regs.s ? 4 : 0) | 2;
	addr &= ~3;
	if (regs.cacheholdingaddr020 == addr || regs.cacheholdingdata_valid == 0)
		return true;
	c = geticache030(icaches030, addr, &tag, &lws);
	if ((regs.cacr & 1) && c->valid[lws] && c->tag == tag) {
		// cache hit
		regs.cacheholdingaddr020 = addr;
		regs.cacheholdingdata020 = c->data[lws];
		return true;
	}
	return false;
}

static void fill_icache030(uae_u32 addr)
{
	int lws;
	uae_u32 tag;
	uae_u32 data;
	struct cache030 *c;

	regs.fc030 = (regs.s ? 4 : 0) | 2;
	addr &= ~3;
	if (regs.cacheholdingaddr020 == addr || regs.cacheholdingdata_valid == 0)
		return;
	c = geticache030 (icaches030, addr, &tag, &lws);
	if ((regs.cacr & 1) && c->valid[lws] && c->tag == tag) {
		// cache hit
		regs.cacheholdingaddr020 = addr;
		regs.cacheholdingdata020 = c->data[lws];
		return;
	}

	TRY (prb2) {
		// cache miss
		if (currprefs.cpu_cycle_exact) {
			regs.ce020memcycle_data = false;
			start_020_cycle_prefetch(false);
			data = icache_fetch(addr);
			end_020_cycle_prefetch(false);
		} else {
			data = icache_fetch(addr);
		}
	} CATCH (prb2) {
		// Bus error/MMU access fault: Delayed exception.
		regs.cacheholdingdata_valid = 0;
		regs.cacheholdingaddr020 = 0xffffffff;
		regs.cacheholdingdata020 = 0xffffffff;
		end_020_cycle_prefetch(false);
		STOPTRY;
		return;
	} ENDTRY

	if (mmu030_cache_state & CACHE_ENABLE_INS) {
		if ((regs.cacr & 0x03) == 0x01) {
			// instruction cache not frozen and enabled
			update_icache030 (c, data, tag, lws);
		}

		// Do burst fetch if enabled, cache is not frozen, all line slots invalid, and 32-bit CPU local bus (no chip ram).
		//
		// Burst cycles 2-4 are handled by the 030 bus controller rather than the sequencing unit.
		// The MMU only translates the first cache fetch (above) and the following 3 fetches increment
		// address lines A2-A3 (optionally via external hardware).  If a bus error occurs, no exception
		// is generated and the remaining cache line slots are left invalid.
		//
		if ((mmu030_cache_state & CACHE_ENABLE_INS_BURST) && (regs.cacr & 0x11) == 0x11) {
			if (c->valid[0] + c->valid[1] + c->valid[2] + c->valid[3] == 1) {
				uaecptr physaddr = addr;
				if (currprefs.mmu_model) {
					physaddr = mmu030_translate(addr, regs.s != 0, false, false);
				}

				if (ce_banktype[physaddr >> 16] == CE_MEMBANK_FAST32) {
					int i;
					for (i = 0; i < 4; i++) {
						if (c->valid[i])
							break;
					}
					uaecptr baddr = physaddr & ~15;

					if (currprefs.mmu_model) {
						TRY (prb) {
							// TODO: Need memory functions for burst row and burst column access.
							for (int j = 0; j < 3; j++) {
								i++;
								i &= 3;
								c->data[i] = get_longi(baddr + i * 4);
								c->valid[i] = true;

								if (currprefs.cpu_cycle_exact)
									do_cycles_ce020_mem(1 * (CPU020_MEM_CYCLE - 1), c->data[i]);
							}
						} CATCH (prb) {
							; // abort burst fetch if bus error, do not report it.
						} ENDTRY
					} else {
						for (int j = 0; j < 3; j++) {
							i++;
							i &= 3;
							c->data[i] = get_longi(baddr + i * 4);
							c->valid[i] = true;

							if (currprefs.cpu_cycle_exact)
								do_cycles_ce020_mem(1 * (CPU020_MEM_CYCLE - 1), c->data[i]);
						}
					}
				}
			}
		}
	}
	regs.cacheholdingaddr020 = addr;
	regs.cacheholdingdata020 = data;
}

#if VALIDATE_68030_DATACACHE
static void validate_dcache030(void)
{
	for (int i = 0; i < CACHELINES030; i++) {
		struct cache030 *c = &dcaches030[i];
		uae_u32 addr = c->tag & ~((CACHELINES030 << 4) - 1);
		addr |= i << 4;
		for (int j = 0; j < 4; j++) {
			if (c->valid[j]) {
				uae_u32 v = get_long(addr);
				if (v != c->data[j]) {
					write_log(_T("Address %08x data cache mismatch %08x != %08x\n"), addr, v, c->data[j]);
				}
			}
			addr += 4;
		}
	}
}

static void validate_dcache030_read(uae_u32 addr, uae_u32 ov, int size)
{
	uae_u32 ov2;
	if (size == 2) {
		ov2 = get_long(addr);
	} else if (size == 1) {
		ov2 = get_word(addr);
		ov &= 0xffff;
	} else {
		ov2 = get_byte(addr);
		ov &= 0xff;
	}
	if (ov2 != ov) {
		write_log(_T("Address read %08x data cache mismatch %08x != %08x\n"), addr, ov2, ov);
	}
}
#endif

// and finally the worst part, 68030 data cache..
static void write_dcache030x(uaecptr addr, uae_u32 val, uae_u32 size, uae_u32 fc)
{
	if (regs.cacr & 0x100) {
		static const uae_u32 mask[3] = { 0xff000000, 0xffff0000, 0xffffffff };
		struct cache030 *c1, *c2;
		int lws1, lws2;
		uae_u32 tag1, tag2;
		int aligned = addr & 3;
		int wa = regs.cacr & 0x2000;
		int width = 8 << size;
		int offset = 8 * aligned;
		int hit;

		c1 = getdcache030(dcaches030, addr, &tag1, &lws1);
		hit = c1->tag == tag1 && c1->fc == fc && c1->valid[lws1];

		// Write-allocate can create new valid cache entry if
		// long aligned long write and MMU CI is not active.
		// All writes ignore external CIIN signal.
		// CACHE_DISABLE_ALLOCATE = emulation only method to disable WA caching completely.

		if (width == 32 && offset == 0 && wa) {
			if (!(mmu030_cache_state & CACHE_DISABLE_MMU) && !(mmu030_cache_state & CACHE_DISABLE_ALLOCATE)) {
				update_dcache030(c1, val, tag1, fc, lws1);
#if VALIDATE_68030_DATACACHE
				validate_dcache030();
#endif
			} else if (hit) {
				// Does real 68030 do this if MMU cache inhibited?
				c1->valid[lws1] = false;
			}
			return;
		}

		if (hit || wa) {
			if (hit) {
				uae_u32 val_left_aligned = val << (32 - width);
				c1->data[lws1] &= ~(mask[size] >> offset);
				c1->data[lws1] |= val_left_aligned >> offset;
			} else {
				c1->valid[lws1] = false;
			}
		}

		// do we need to update a 2nd cache entry ?
		if (width + offset > 32) {
			c2 = getdcache030(dcaches030, addr + 4, &tag2, &lws2);
			hit = c2->tag == tag2 && c2->fc == fc && c2->valid[lws2];
			if (hit || wa) {
				if (hit) {
					c2->data[lws2] &= 0xffffffff >> (width + offset - 32);
					c2->data[lws2] |= val << (32 - (width + offset - 32));
				} else {
					c2->valid[lws2] = false;
				}
			}
		}
#if VALIDATE_68030_DATACACHE
		validate_dcache030();
#endif
	}
}

void write_dcache030_bput(uaecptr addr, uae_u32 v,uae_u32 fc)
{
	regs.fc030 = fc;
	dcache_bput(addr, v);
	write_dcache030x(addr, v, 0, fc);
}
void write_dcache030_wput(uaecptr addr, uae_u32 v,uae_u32 fc)
{
	regs.fc030 = fc;
	dcache_wput(addr, v);
	write_dcache030x(addr, v, 1, fc);
}
void write_dcache030_lput(uaecptr addr, uae_u32 v,uae_u32 fc)
{
	regs.fc030 = fc;
	dcache_lput(addr, v);
	write_dcache030x(addr, v, 2, fc);
}

// 68030 MMU bus fault retry case, direct write, store to cache if enabled
void write_dcache030_retry(uaecptr addr, uae_u32 v, uae_u32 fc, int size, int flags)
{
	regs.fc030 = fc;
	mmu030_put_generic(addr, v, fc, size, flags);
	write_dcache030x(addr, v, size, fc);
}

static void dcache030_maybe_burst(uaecptr addr, struct cache030 *c, int lws)
{
	// Do burst fetch if enabled, cache not frozen, all line slots invalid, and 32-bit CPU local bus (no chip ram).
	// (See notes about burst fetches in icache routines)
	if (c->valid[0] + c->valid[1] + c->valid[2] + c->valid[3] == 1) {
		uaecptr physaddr = addr;
		if (currprefs.mmu_model) {
			physaddr = mmu030_translate(addr, regs.s != 0, false, false);
		}

		if (ce_banktype[physaddr >> 16] == CE_MEMBANK_FAST32) {
			int i;
			for (i = 0; i < 4; i++) {
				if (c->valid[i])
					break;
			}
			uaecptr baddr = physaddr & ~15;

			if (currprefs.mmu_model) {
				TRY (prb) {
					// TODO: Need memory functions for burst row and burst column access.
					for (int j = 0; j < 3; j++) {
						i++;
						i &= 3;
						c->data[i] = get_long(baddr + i * 4);
						c->valid[i] = true;

						if (currprefs.cpu_cycle_exact)
							do_cycles_ce020_mem(1 * (CPU020_MEM_CYCLE - 1), c->data[i]);
					}
				} CATCH (prb) {
					; // abort burst fetch if bus error
				} ENDTRY
			} else {
				for (int j = 0; j < 3; j++) {
					i++;
					i &= 3;
					c->data[i] = get_long(baddr + i * 4);
					c->valid[i] = true;

					if (currprefs.cpu_cycle_exact)
						do_cycles_ce020_mem(1 * (CPU020_MEM_CYCLE - 1), c->data[i]);
				}
			}
		}
#if VALIDATE_68030_DATACACHE
		validate_dcache030();
#endif
	}
}

#ifdef DEBUGGER
static uae_u32 read_dcache030_debug(uaecptr addr, uae_u32 size, uae_u32 fc, bool *cached)
{
	static const uae_u32 mask[3] = { 0x000000ff, 0x0000ffff, 0xffffffff };
	struct cache030 *c1, *c2;
	int lws1, lws2;
	uae_u32 tag1, tag2;
	int aligned = addr & 3;
	uae_u32 v1, v2;
	int width = 8 << size;
	int offset = 8 * aligned;
	uae_u32 out;

	*cached = false;
	if (!currprefs.cpu_data_cache) {
		if (size == 0)
			return get_byte_debug(addr);
		if (size == 1)
			return get_word_debug(addr);
		return get_long_debug(addr);
	}

	c1 = getdcache030(dcaches030, addr, &tag1, &lws1);
	addr &= ~3;
	if (!c1->valid[lws1] || c1->tag != tag1 || c1->fc != fc) {
		v1 = get_long_debug(addr);
	} else {
		// Cache hit, inhibited caching do not prevent read hits.
		v1 = c1->data[lws1];
		*cached = true;
	}

	// only one long fetch needed?
	if (width + offset <= 32) {
		out = v1 >> (32 - (offset + width));
		out &= mask[size];
		return out;
	}

	// no, need another one
	addr += 4;
	c2 = getdcache030(dcaches030, addr, &tag2, &lws2);
	if (!c2->valid[lws2] || c2->tag != tag2 || c2->fc != fc) {
		v2 = get_long_debug(addr);
	} else {
		v2 = c2->data[lws2];
		*cached = true;
	}

	uae_u64 v64 = (static_cast<uae_u64>(v1) << 32) | v2;
	out = static_cast<uae_u32>(v64 >> (64 - (offset + width)));
	out &= mask[size];
	return out;
}
#endif

static bool read_dcache030_2(uaecptr addr, uae_u32 size, uae_u32 *valp)
{
	// data cache enabled?
	if (!(regs.cacr & 0x100))
		return false;

	uae_u32 addr_o = addr;
	uae_u32 fc = regs.fc030;
	static const uae_u32 mask[3] = { 0x000000ff, 0x0000ffff, 0xffffffff };
	struct cache030 *c1, *c2;
	int lws1, lws2;
	uae_u32 tag1, tag2;
	int aligned = addr & 3;
	uae_u32 v1, v2;
	int width = 8 << size;
	int offset = 8 * aligned;
	uae_u32 out;

	c1 = getdcache030(dcaches030, addr, &tag1, &lws1);
	addr &= ~3;
	if (!c1->valid[lws1] || c1->tag != tag1 || c1->fc != fc) {
		// MMU validate address, returns zero if valid but uncacheable
		// throws bus error if invalid
		uae_u8 cs = dcache_check(addr_o, false, size);
		if (!(cs & CACHE_ENABLE_DATA))
			return false;
		v1 = dcache_lget(addr);
		update_dcache030(c1, v1, tag1, fc, lws1);
		if ((cs & CACHE_ENABLE_DATA_BURST) && (regs.cacr & 0x1100) == 0x1100)
			dcache030_maybe_burst(addr, c1, lws1);
#if VALIDATE_68030_DATACACHE
		validate_dcache030();
#endif
	} else {
		// Cache hit, inhibited caching do not prevent read hits.
		v1 = c1->data[lws1];
	}

	// only one long fetch needed?
	if (width + offset <= 32) {
		out = v1 >> (32 - (offset + width));
		out &= mask[size];
#if VALIDATE_68030_DATACACHE
		validate_dcache030_read(addr_o, out, size);
#endif
		*valp = out;
		return true;
	}

	// no, need another one
	addr += 4;
	c2 = getdcache030(dcaches030, addr, &tag2, &lws2);
	if (!c2->valid[lws2] || c2->tag != tag2 || c2->fc != fc) {
		uae_u8 cs = dcache_check(addr, false, 2);
		if (!(cs & CACHE_ENABLE_DATA))
			return false;
		v2 = dcache_lget(addr);
		update_dcache030(c2, v2, tag2, fc, lws2);
		if ((cs & CACHE_ENABLE_DATA_BURST) && (regs.cacr & 0x1100) == 0x1100)
			dcache030_maybe_burst(addr, c2, lws2);
#if VALIDATE_68030_DATACACHE
		validate_dcache030();
#endif
	} else {
		v2 = c2->data[lws2];
	}

	uae_u64 v64 = (static_cast<uae_u64>(v1) << 32) | v2;
	out = static_cast<uae_u32>(v64 >> (64 - (offset + width)));
	out &= mask[size];

#if VALIDATE_68030_DATACACHE
	validate_dcache030_read(addr_o, out, size);
#endif
	*valp = out;
	return true;
}

static uae_u32 read_dcache030 (uaecptr addr, uae_u32 size, uae_u32 fc)
{
	uae_u32 val;
	regs.fc030 = fc;

	if (!read_dcache030_2(addr, size, &val)) {
		// read from memory, data cache is disabled or inhibited.
		if (size == 2)
			return dcache_lget(addr);
		else if (size == 1)
			return dcache_wget(addr);
		else
			return dcache_bget(addr);
	}
	return val;
}

// 68030 MMU bus fault retry case, either read from cache or use direct reads
uae_u32 read_dcache030_retry(uaecptr addr, uae_u32 fc, int size, int flags)
{
	uae_u32 val;
	regs.fc030 = fc;

	if (!read_dcache030_2(addr, size, &val)) {
		return mmu030_get_generic(addr, fc, size, flags);
	}
	return val;
}

uae_u32 read_dcache030_bget(uaecptr addr, uae_u32 fc)
{
	return read_dcache030(addr, 0, fc);
}
uae_u32 read_dcache030_wget(uaecptr addr, uae_u32 fc)
{
	return read_dcache030(addr, 1, fc);
}
uae_u32 read_dcache030_lget(uaecptr addr, uae_u32 fc)
{
	return read_dcache030(addr, 2, fc);
}

uae_u32 read_dcache030_mmu_bget(uaecptr addr)
{
	return read_dcache030_bget(addr, (regs.s ? 4 : 0) | 1);
}
uae_u32 read_dcache030_mmu_wget(uaecptr addr)
{
	return read_dcache030_wget(addr, (regs.s ? 4 : 0) | 1);
}
uae_u32 read_dcache030_mmu_lget(uaecptr addr)
{
	return read_dcache030_lget(addr, (regs.s ? 4 : 0) | 1);
}
void write_dcache030_mmu_bput(uaecptr addr, uae_u32 val)
{
	write_dcache030_bput(addr, val, (regs.s ? 4 : 0) | 1);
}
void write_dcache030_mmu_wput(uaecptr addr, uae_u32 val)
{
	write_dcache030_wput(addr, val, (regs.s ? 4 : 0) | 1);
}
void write_dcache030_mmu_lput(uaecptr addr, uae_u32 val)
{
	write_dcache030_lput(addr, val, (regs.s ? 4 : 0) | 1);
}

uae_u32 read_dcache030_lrmw_mmu_fcx(uaecptr addr, uae_u32 size, int fc)
{
	if (currprefs.cpu_data_cache) {
		mmu030_cache_state = CACHE_DISABLE_MMU;
		if (size == 0)
			return read_dcache030_bget(addr, fc);
		if (size == 1)
			return read_dcache030_wget(addr, fc);
		return read_dcache030_lget(addr, fc);
	} else {
		if (size == 0)
			return read_data_030_bget(addr);
		if (size == 1)
			return read_data_030_wget(addr);
		return read_data_030_lget(addr);
	}
}
uae_u32 read_dcache030_lrmw_mmu(uaecptr addr, uae_u32 size)
{
	return read_dcache030_lrmw_mmu_fcx(addr, size, (regs.s ? 4 : 0) | 1);
}
void write_dcache030_lrmw_mmu_fcx(uaecptr addr, uae_u32 val, uae_u32 size, int fc)
{
	if (currprefs.cpu_data_cache) {
		mmu030_cache_state = CACHE_DISABLE_MMU;
		if (size == 0)
			write_dcache030_bput(addr, val, fc);
		else if (size == 1)
			write_dcache030_wput(addr, val, fc);
		else
			write_dcache030_lput(addr, val, fc);
	} else {
		if (size == 0)
			write_data_030_bput(addr, val);
		else if (size == 1)
			write_data_030_wput(addr, val);
		else
			write_data_030_lput(addr, val);
	}
}
void write_dcache030_lrmw_mmu(uaecptr addr, uae_u32 val, uae_u32 size)
{
	write_dcache030_lrmw_mmu_fcx(addr, val, size, (regs.s ? 4 : 0) | 1);
}

static void do_access_or_bus_error(uaecptr pc, uaecptr pcnow)
{
	// TODO: handle external bus errors
	if (!currprefs.mmu_model)
		return;
	if (pc != 0xffffffff)
		regs.instruction_pc = pc;
	mmu030_opcode = -1;
	mmu030_page_fault(pcnow, true, -1, 0);
}

static uae_u32 get_word_ce030_prefetch_2 (int o)
{
	uae_u32 pc = m68k_getpc () + o;
	uae_u32 v;

	v = regs.prefetch020[0];
	regs.prefetch020[0] = regs.prefetch020[1];
	regs.prefetch020[1] = regs.prefetch020[2];
#if MORE_ACCURATE_68020_PIPELINE
	pipeline_020(pc);
#endif
	if (pc & 2) {
		// branch instruction detected in pipeline: stop fetches until branch executed.
		if (!MORE_ACCURATE_68020_PIPELINE || regs.pipeline_stop >= 0) {
			fill_icache030 (pc + 2 + 4);
		} else {
			if (regs.cacheholdingdata_valid > 0)
				regs.cacheholdingdata_valid++;
		}
		regs.prefetch020[2] = regs.cacheholdingdata020 >> 16;
	} else {
		pc += 4;
		// cacheholdingdata020 may be invalid if RTE from bus error
		if ((!MORE_ACCURATE_68020_PIPELINE || regs.pipeline_stop >= 0) && regs.cacheholdingaddr020 != pc) {
			fill_icache030 (pc);
		}
		regs.prefetch020[2] = static_cast<uae_u16>(regs.cacheholdingdata020);
	}
	regs.db = regs.prefetch020[0];
	do_cycles_ce020_internal(2);
	return v;
}

uae_u32 get_word_ce030_prefetch (int o)
{
	return get_word_ce030_prefetch_2(o);
}
uae_u32 get_word_ce030_prefetch_opcode (int o)
{
	return get_word_ce030_prefetch_2(o);
}

uae_u32 get_word_030_prefetch (int o)
{
	uae_u32 pc = m68k_getpc () + o;
	uae_u32 v;

	v = regs.prefetch020[0];
	regs.prefetch020[0] = regs.prefetch020[1];
	regs.prefetch020[1] = regs.prefetch020[2];
	regs.prefetch020_valid[0] = regs.prefetch020_valid[1];
	regs.prefetch020_valid[1] = regs.prefetch020_valid[2];
	regs.prefetch020_valid[2] = false;
	if (!regs.prefetch020_valid[1]) {
		if (regs.pipeline_stop) {
			regs.db = regs.prefetch020[0];
			return v;
		}
		do_access_or_bus_error(0xffffffff, pc + 4);
	}
#if MORE_ACCURATE_68020_PIPELINE
	pipeline_020(pc);
#endif
	if (pc & 2) {
		// branch instruction detected in pipeline: stop fetches until branch executed.
		if (!MORE_ACCURATE_68020_PIPELINE || regs.pipeline_stop >= 0) {
			fill_icache030 (pc + 2 + 4);
		} else {
			if (regs.cacheholdingdata_valid > 0)
				regs.cacheholdingdata_valid++;
		}
		regs.prefetch020[2] = regs.cacheholdingdata020 >> 16;
	} else {
		pc += 4;
		// cacheholdingdata020 may be invalid if RTE from bus error
		if ((!MORE_ACCURATE_68020_PIPELINE || regs.pipeline_stop >= 0) && regs.cacheholdingaddr020 != pc) {
			fill_icache030 (pc);
		}
		regs.prefetch020[2] = static_cast<uae_u16>(regs.cacheholdingdata020);
	}
	regs.prefetch020_valid[2] = regs.cacheholdingdata_valid;
	regs.db = regs.prefetch020[0];
	return v;
}

uae_u32 get_word_icache030(uaecptr addr)
{
	fill_icache030(addr);
	return regs.cacheholdingdata020 >> ((addr & 2) ? 0 : 16);
}
uae_u32 get_long_icache030(uaecptr addr)
{
	uae_u32 v;
	fill_icache030(addr);
	if ((addr & 2) == 0)
		return regs.cacheholdingdata020;
	v = regs.cacheholdingdata020 << 16;
	fill_icache030(addr + 4);
	v |= regs.cacheholdingdata020 >> 16;
	return v;
}

uae_u32 fill_icache040(uae_u32 addr)
{
	int index, lws;
	uae_u32 tag, addr2;
	struct cache040 *c;
	int line;

	addr2 = addr & ~15;
	lws = (addr >> 2) & 3;

	if (regs.prefetch020addr == addr2) {
		return regs.prefetch040[lws];
	}

	if (regs.cacr & 0x8000) {
		uae_u8 cs = mmu_cache_state;

		if (!(ce_cachable[addr >> 16] & CACHE_ENABLE_INS))
			cs = CACHE_DISABLE_MMU;

		index = (addr >> 4) & cacheisets04060mask;
		tag = addr & cacheitag04060mask;
		c = &icaches040[index];
		for (int i = 0; i < CACHELINES040; i++) {
			if (c->valid[cache_lastline] && c->tag[cache_lastline] == tag) {
				// cache hit
				if (!(cs & CACHE_ENABLE_INS) || (cs & CACHE_DISABLE_MMU)) {
					c->valid[cache_lastline] = false;
					goto end;
				}
				if ((lws & 1) != icachehalfline) {
					icachehalfline ^= 1;
					icachelinecnt++;
				}
				return c->data[cache_lastline][lws];
			}
			cache_lastline++;
			cache_lastline &= (CACHELINES040 - 1);
		}
		// cache miss
		regs.prefetch020addr = 0xffffffff;
		regs.prefetch040[0] = icache_fetch(addr2 +  0);
		regs.prefetch040[1] = icache_fetch(addr2 +  4);
		regs.prefetch040[2] = icache_fetch(addr2 +  8);
		regs.prefetch040[3] = icache_fetch(addr2 + 12);
		regs.prefetch020addr = addr2;
		if (!(cs & CACHE_ENABLE_INS) || (cs & CACHE_DISABLE_MMU))
			goto end;
		if (regs.cacr & 0x00004000) // 68060 NAI
			goto end;
		if (c->valid[0] && c->valid[1] && c->valid[2] && c->valid[3]) {
			line = icachelinecnt & (CACHELINES040 - 1);
			icachehalfline = (lws & 1) ? 0 : 1;
		} else {
			for (line = 0; line < CACHELINES040; line++) {
				if (c->valid[line] == false)
					break;
			}
		}
		c->tag[line] = tag;
		c->valid[line] = true;
		c->data[line][0] = regs.prefetch040[0];
		c->data[line][1] = regs.prefetch040[1];
		c->data[line][2] = regs.prefetch040[2];
		c->data[line][3] = regs.prefetch040[3];
		if ((lws & 1) != icachehalfline) {
			icachehalfline ^= 1;
			icachelinecnt++;
		}
		return c->data[line][lws];

	}

end:
	if (regs.prefetch020addr == addr2)
		return regs.prefetch040[lws];
	regs.prefetch020addr = addr2;
	regs.prefetch040[0] = icache_fetch(addr2 +  0);
	regs.prefetch040[1] = icache_fetch(addr2 +  4);
	regs.prefetch040[2] = icache_fetch(addr2 +  8);
	regs.prefetch040[3] = icache_fetch(addr2 + 12);
	return regs.prefetch040[lws];
}

STATIC_INLINE void do_cycles_c040_mem (int clocks, uae_u32 val)
{
	x_do_cycles_post (clocks * cpucycleunit, val);
}

uae_u32 mem_access_delay_longi_read_c040 (uaecptr addr)
{
	uae_u32 v;
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP16:
		v  = wait_cpu_cycle_read_ce020 (addr + 0, 2) << 16;
		v |= wait_cpu_cycle_read_ce020 (addr + 2, 2) <<  0;
		break;
	case CE_MEMBANK_CHIP32:
		if ((addr & 3) != 0) {
			v  = wait_cpu_cycle_read_ce020 (addr + 0, 2) << 16;
			v |= wait_cpu_cycle_read_ce020 (addr + 2, 2) <<  0;
		} else {
			v = wait_cpu_cycle_read_ce020 (addr, -2);
		}
		break;
	case CE_MEMBANK_FAST16:
		v = get_longi (addr);
		do_cycles_c040_mem(1, v);
		break;
	case CE_MEMBANK_FAST32:
		v = get_longi (addr);
		break;
	default:
		v = get_longi (addr);
		break;
	}
	return v;
}
uae_u32 mem_access_delay_long_read_c040 (uaecptr addr)
{
	uae_u32 v;
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP16:
		v  = wait_cpu_cycle_read_ce020 (addr + 0, 1) << 16;
		v |= wait_cpu_cycle_read_ce020 (addr + 2, 1) <<  0;
		break;
	case CE_MEMBANK_CHIP32:
		if ((addr & 3) != 0) {
			v  = wait_cpu_cycle_read_ce020 (addr + 0, 1) << 16;
			v |= wait_cpu_cycle_read_ce020 (addr + 2, 1) <<  0;
		} else {
			v = wait_cpu_cycle_read_ce020 (addr, -1);
		}
		break;
	case CE_MEMBANK_FAST16:
		v = get_long (addr);
		do_cycles_c040_mem(1, v);
		break;
	case CE_MEMBANK_FAST32:
		v = get_long (addr);
		break;
	default:
		v = get_long (addr);
		break;
	}
	return v;
}

uae_u32 mem_access_delay_word_read_c040 (uaecptr addr)
{
	uae_u32 v;
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP16:
	case CE_MEMBANK_CHIP32:
		if ((addr & 3) == 3) {
			v  = wait_cpu_cycle_read_ce020 (addr + 0, 0) << 8;
			v |= wait_cpu_cycle_read_ce020 (addr + 1, 0) << 0;
		} else {
			v = wait_cpu_cycle_read_ce020 (addr, 1);
		}
		break;
	case CE_MEMBANK_FAST16:
		v = get_word (addr);
		do_cycles_c040_mem (2, v);
		break;
	case CE_MEMBANK_FAST32:
		v = get_word (addr);
		 break;
	default:
		 v = get_word (addr);
		break;
	}
	return v;
}

uae_u32 mem_access_delay_byte_read_c040 (uaecptr addr)
{
	uae_u32 v;
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP16:
	case CE_MEMBANK_CHIP32:
		v = wait_cpu_cycle_read_ce020 (addr, 0);
		break;
	case CE_MEMBANK_FAST16:
		v = get_byte (addr);
		do_cycles_c040_mem (1, v);
		break;
	case CE_MEMBANK_FAST32:
		v = get_byte (addr);
		break;
	default:
		v = get_byte (addr);
		break;
	}
	return v;
}

void mem_access_delay_byte_write_c040 (uaecptr addr, uae_u32 v)
{
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP16:
	case CE_MEMBANK_CHIP32:
		wait_cpu_cycle_write_ce020 (addr, 0, v);
		break;
	case CE_MEMBANK_FAST16:
		put_byte (addr, v);
		do_cycles_c040_mem (1, v);
		break;
	case CE_MEMBANK_FAST32:
		put_byte (addr, v);
		break;
	default:
		put_byte (addr, v);
	break;
	}
}

void mem_access_delay_word_write_c040 (uaecptr addr, uae_u32 v)
{
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP16:
	case CE_MEMBANK_CHIP32:
		if ((addr & 3) == 3) {
			wait_cpu_cycle_write_ce020 (addr + 0, 0, (v >> 8) & 0xff);
			wait_cpu_cycle_write_ce020 (addr + 1, 0, (v >> 0) & 0xff);
		} else {
			wait_cpu_cycle_write_ce020 (addr + 0, 1, v);
		}
		break;
	case CE_MEMBANK_FAST16:
		put_word (addr, v);
		if ((addr & 3) == 3)
			do_cycles_c040_mem(2, v);
		else
			do_cycles_c040_mem(1, v);
		break;
	case CE_MEMBANK_FAST32:
		put_word (addr, v);
		break;
	default:
		put_word (addr, v);
	break;
	}
}

void mem_access_delay_long_write_c040 (uaecptr addr, uae_u32 v)
{
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP16:
		wait_cpu_cycle_write_ce020 (addr + 0, 1, (v >> 16) & 0xffff);
		wait_cpu_cycle_write_ce020 (addr + 2, 1, (v >>  0) & 0xffff);
		break;
	case CE_MEMBANK_CHIP32:
		if ((addr & 3) != 0) {
			wait_cpu_cycle_write_ce020 (addr + 0, 1, (v >> 16) & 0xffff);
			wait_cpu_cycle_write_ce020 (addr + 2, 1, (v >>  0) & 0xffff);
		} else {
			wait_cpu_cycle_write_ce020 (addr + 0, -1, v);
		}
		break;
	case CE_MEMBANK_FAST16:
		put_long (addr, v);
		do_cycles_c040_mem(1, v);
		break;
	case CE_MEMBANK_FAST32:
		put_long (addr, v);
		break;
	default:
		put_long (addr, v);
		break;
	}
}

static uae_u32 dcache040_get_data(uaecptr addr, struct cache040 *c, int line, int size)
{
	static const uae_u32 mask[3] = { 0x000000ff, 0x0000ffff, 0xffffffff };
	int offset = (addr & 15) * 8;
	int offset32 = offset & 31;
	int slot = offset / 32;
	int width = 8 << size;
	uae_u32 vv;

	if (offset32 + width <= 32) {
		uae_u32 v = c->data[line][slot];
		v >>= 32 - (offset32 + width);
		v &= mask[size];
		vv = v;
	} else {
#if VALIDATE_68040_DATACACHE
		if (slot >= 3) {
			write_log(_T("invalid dcache040_get_data!\n"));
			return 0;
		}
#endif
		uae_u64 v = c->data[line][slot];
		v <<= 32;
		v |= c->data[line][slot + 1];
		v >>= 64 - (offset32 + width);
		vv = v & mask[size];
	}
	return vv;
}

static void dcache040_update(uaecptr addr, struct cache040 *c, int line, uae_u32 val, int size)
{
	static const uae_u64 mask64[3] = { 0xff, 0xffff, 0xffffffff };
	static const uae_u32 mask32[3] = { 0xff, 0xffff, 0xffffffff };
	int offset = (addr & 15) * 8;
	int offset32 = offset & 31;
	int slot = offset / 32;
	int width = 8 << size;

#if VALIDATE_68040_DATACACHE > 1
	validate_dcache040();
#endif

	if (offset32 + width <= 32) {
		int shift = 32 - (offset32 + width);
		uae_u32 v = c->data[line][slot];
		v &= ~(mask32[size] << shift);
		v |= val << shift;
		c->data[line][slot] = v;
		c->dirty[line][slot] = true;
	} else {
#if VALIDATE_68040_DATACACHE
		if (slot >= 3) {
			write_log(_T("invalid dcache040_update!\n"));
			return;
		}
#endif
		int shift = 64 - (offset32 + width);
		uae_u64 v = c->data[line][slot];
		v <<= 32;
		v |= c->data[line][slot + 1];
		v &= ~(mask64[size] << shift);
		v |= static_cast<uae_u64>(val) << shift;
		c->data[line][slot] = v >> 32;
		c->dirty[line][slot] = true;
		c->data[line][slot + 1] = static_cast<uae_u32>(v);
		c->dirty[line][slot + 1] = true;
	}
	c->gdirty[line] = true;
}

static int dcache040_fill_line(int index, uae_u32 tag, uaecptr addr)
{
	// cache miss
	struct cache040 *c = &dcaches040[index];
	int line;
	if (c->valid[0] && c->valid[1] && c->valid[2] && c->valid[3]) {
		// all lines allocated, choose one, push and invalidate.
		line = dcachelinecnt & (CACHELINES040 - 1);
		dcachelinecnt++;
		dcache040_push_line(index, line, false, true);
	} else {
		// at least one invalid
		for (line = 0; line < CACHELINES040; line++) {
			if (c->valid[line] == false)
				break;
		}
	}
	c->tag[line] = tag;
	c->dirty[line][0] = false;
	c->dirty[line][1] = false;
	c->dirty[line][2] = false;
	c->dirty[line][3] = false;
	c->gdirty[line] = false;
	c->data[line][0] = dcache_lget(addr + 0);
	c->data[line][1] = dcache_lget(addr + 4);
	c->data[line][2] = dcache_lget(addr + 8);
	c->data[line][3] = dcache_lget(addr + 12);
	c->valid[line] = true;
	return line;
}

#ifdef DEBUGGER
static uae_u32 read_dcache040_debug(uae_u32 addr, int size, bool *cached)
{
	int index;
	uae_u32 tag;
	struct cache040 *c;
	int line;
	uae_u32 addr_o = addr;
	uae_u8 cs = mmu_cache_state;

	*cached = false;
	if (!currprefs.cpu_data_cache)
		goto nocache;
	if (!(regs.cacr & 0x80000000))
		goto nocache;

	addr &= ~15;
	index = (addr >> 4) & cachedsets04060mask;
	tag = addr & cachedtag04060mask;
	c = &dcaches040[index];
	for (line = 0; line < CACHELINES040; line++) {
		if (c->valid[line] && c->tag[line] == tag) {
			// cache hit
			return dcache040_get_data(addr_o, c, line, size);
		}
	}
nocache:
	if (size == 0)
		return get_byte_debug(addr);
	if (size == 1)
		return get_word_debug(addr);
	return get_long_debug(addr);
}
#endif

static uae_u32 read_dcache040(uae_u32 addr, int size, uae_u32 (*fetch)(uaecptr))
{
	int index;
	uae_u32 tag;
	struct cache040 *c;
	int line;
	uae_u32 addr_o = addr;
	uae_u8 cs = mmu_cache_state;

	if (!(regs.cacr & 0x80000000))
		goto nocache;

#if VALIDATE_68040_DATACACHE > 1
	validate_dcache040();
#endif

	// Simple because 68040+ caches physical addresses (68030 caches logical addresses)
	if (!(ce_cachable[addr >> 16] & CACHE_ENABLE_DATA))
		cs = CACHE_DISABLE_MMU;

	addr &= ~15;
	index = (addr >> 4) & cachedsets04060mask;
	tag = addr & cachedtag04060mask;
	c = &dcaches040[index];
	for (line = 0; line < CACHELINES040; line++) {
		if (c->valid[line] && c->tag[line] == tag) {
			// cache hit
			dcachelinecnt++;
			// Cache hit but MMU disabled: do not cache, push and invalidate possible existing line
			if (cs & CACHE_DISABLE_MMU) {
				dcache040_push_line(index, line, false, true);
				goto nocache;
			}
			return dcache040_get_data(addr_o, c, line, size);
		}
	}
	// Cache miss
	// 040+ always caches whole line
	if ((cs & CACHE_DISABLE_MMU) || !(cs & CACHE_ENABLE_DATA) || (cs & CACHE_DISABLE_ALLOCATE) || (regs.cacr & 0x40000000)) {
nocache:
		return fetch(addr_o);
	}
	// Allocate new cache line, return requested data.
	line = dcache040_fill_line(index, tag, addr);
	return dcache040_get_data(addr_o, c, line, size);
}

static void write_dcache040(uae_u32 addr, uae_u32 val, int size, void (*store)(uaecptr, uae_u32))
{
	static const uae_u32 mask[3] = { 0x000000ff, 0x0000ffff, 0xffffffff };
	int index;
	uae_u32 tag;
	struct cache040 *c;
	int line;
	uae_u32 addr_o = addr;
	uae_u8 cs = mmu_cache_state;

	val &= mask[size];

	if (!(regs.cacr & 0x80000000))
		goto nocache;

	if (!(ce_cachable[addr >> 16] & CACHE_ENABLE_DATA))
		cs = CACHE_DISABLE_MMU;

	addr &= ~15;
	index = (addr >> 4) & cachedsets04060mask;
	tag = addr & cachedtag04060mask;
	c = &dcaches040[index];
	for (line = 0; line < CACHELINES040; line++) {
		if (c->valid[line] && c->tag[line] == tag) {
			// cache hit
			dcachelinecnt++;
			// Cache hit but MMU disabled: do not cache, push and invalidate possible existing line
			if (cs & CACHE_DISABLE_MMU) {
				dcache040_push_line(index, line, false, true);
				goto nocache;
			}
			dcache040_update(addr_o, c, line, val, size);
			// If not copyback mode: push modifications immediately (write-through)
			if (!(cs & CACHE_ENABLE_COPYBACK) || DISABLE_68040_COPYBACK) {
				dcache040_push_line(index, line, true, false);
			}
			return;
		}
	}
	// Cache miss
	// 040+ always caches whole line
	// Writes misses in write-through mode don't allocate new cache lines
	if (!(cs & CACHE_ENABLE_DATA) || (cs & CACHE_DISABLE_MMU) || (cs & CACHE_DISABLE_ALLOCATE) || !(cs & CACHE_ENABLE_COPYBACK) || (regs.cacr & 0x40000000)) {
nocache:
		store(addr_o, val);
		return;
	}
	// Allocate new cache line and update it with new data.
	line = dcache040_fill_line(index, tag, addr);
	dcache040_update(addr_o, c, line, val, size);
	if (DISABLE_68040_COPYBACK) {
		dcache040_push_line(index, line, true, false);
	}
}

// really unoptimized
uae_u32 get_word_icache040(uaecptr addr)
{
	uae_u32 v = fill_icache040(addr);
	return v >> ((addr & 2) ? 0 : 16);
}
uae_u32 get_long_icache040(uaecptr addr)
{
	uae_u32 v1, v2;
	v1 = fill_icache040(addr);
	if ((addr & 2) == 0)
		return v1;
	v2 = fill_icache040(addr + 4);
	return (v2 >> 16) | (v1 << 16);
}
uae_u32 get_ilong_cache_040(int o)
{
	return get_long_icache040(m68k_getpci() + o);
}
uae_u32 get_iword_cache_040(int o)
{
	return get_word_icache040(m68k_getpci() + o);
}

void put_long_cache_040(uaecptr addr, uae_u32 v)
{
	int offset = addr & 15;
	// access must not cross cachelines
	if (offset < 13) {
		write_dcache040(addr, v, 2, dcache_lput);
	} else if (offset == 13 || offset == 15) {
		write_dcache040(addr + 0, v >> 24, 0, dcache_bput);
		write_dcache040(addr + 1, v >>  8, 1, dcache_wput);
		write_dcache040(addr + 3, v >>  0, 0, dcache_bput);
	} else if (offset == 14) {
		write_dcache040(addr + 0, v >> 16, 1, dcache_wput);
		write_dcache040(addr + 2, v >>  0, 1, dcache_wput);
	}
}
void put_word_cache_040(uaecptr addr, uae_u32 v)
{
	int offset = addr & 15;
	if (offset < 15) {
		write_dcache040(addr, v, 1, dcache_wput);
	} else {
		write_dcache040(addr + 0, v >> 8, 0, dcache_bput);
		write_dcache040(addr + 1, v >> 0, 0, dcache_bput);
	}
}
void put_byte_cache_040(uaecptr addr, uae_u32 v)
{
	write_dcache040(addr, v, 0, dcache_bput);
}

uae_u32 get_long_cache_040(uaecptr addr)
{
	uae_u32 v;
	int offset = addr & 15;
	if (offset < 13) {
		v = read_dcache040(addr, 2, dcache_lget);
	} else if (offset == 13 || offset == 15) {
		v =  read_dcache040(addr + 0, 0, dcache_bget) << 24;
		v |= read_dcache040(addr + 1, 1, dcache_wget) <<  8;
		v |= read_dcache040(addr + 3, 0, dcache_bget) <<  0;
	} else /* if (offset == 14) */ {
		v =  read_dcache040(addr + 0, 1, dcache_wget) << 16;
		v |= read_dcache040(addr + 2, 1, dcache_wget) <<  0;
	}
	return v;
}
uae_u32 get_word_cache_040(uaecptr addr)
{
	uae_u32 v;
	int offset = addr & 15;
	if (offset < 15) {
		v = read_dcache040(addr, 1, dcache_wget);
	} else {
		v =  read_dcache040(addr + 0, 0, dcache_bget) << 8;
		v |= read_dcache040(addr + 1, 0, dcache_bget) << 0;
	}
	return v;
}
uae_u32 get_byte_cache_040(uaecptr addr)
{
	return read_dcache040(addr, 0, dcache_bget);
}
uae_u32 next_iword_cache040()
{
	uae_u32 r = get_word_icache040(m68k_getpci());
	m68k_incpci(2);
	return r;
}
uae_u32 next_ilong_cache040()
{
	uae_u32 r = get_long_icache040(m68k_getpci());
	m68k_incpci(4);
	return r;
}

#ifdef DEBUGGER
uae_u32 get_byte_cache_debug(uaecptr addr, bool *cached)
{
	*cached = false;
	if (currprefs.cpu_model == 68030) {
		return read_dcache030_debug(addr, 0, regs.s ? 5 : 1, cached);
	} else if (currprefs.cpu_model >= 68040) {
		return read_dcache040_debug(addr, 0, cached);
	}
	return get_byte_debug(addr);
}
uae_u32 get_word_cache_debug(uaecptr addr, bool *cached)
{
	*cached = false;
	if (currprefs.cpu_model == 68030) {
		return read_dcache030_debug(addr, 1, regs.s ? 5 : 1, cached);
	} else if (currprefs.cpu_model >= 68040) {
		return read_dcache040_debug(addr, 1, cached);
	}
	return get_word_debug(addr);
}

uae_u32 get_long_cache_debug(uaecptr addr, bool *cached)
{
	*cached = false;
	if (currprefs.cpu_model == 68030) {
		return read_dcache030_debug(addr, 2, regs.s ? 5 : 1, cached);
	} else if (currprefs.cpu_model >= 68040) {
		return read_dcache040_debug(addr, 2, cached);
	}
	return get_long_debug(addr);
}
#endif

void check_t0_trace()
{
	if (regs.t0 && !regs.t1 && currprefs.cpu_model >= 68020) {
		unset_special (SPCFLAG_TRACE);
		set_special (SPCFLAG_DOTRACE);
	}
}

static void reset_pipeline_state()
{
#if MORE_ACCURATE_68020_PIPELINE
	regs.pipeline_pos = 0;
	regs.pipeline_stop = 0;
	regs.pipeline_r8[0] = regs.pipeline_r8[1] = -1;
#endif
}

static int add_prefetch_030(int idx, uae_u16 w, uaecptr pc)
{
	regs.prefetch020[0] = regs.prefetch020[1];
	regs.prefetch020_valid[0] = regs.prefetch020_valid[1];
	regs.prefetch020[1] = regs.prefetch020[2];
	regs.prefetch020_valid[1] = regs.prefetch020_valid[2];
	regs.prefetch020[2] = w;
	regs.prefetch020_valid[2] = regs.cacheholdingdata_valid;

#if MORE_ACCURATE_68020_PIPELINE
	if (idx >= 1) {
		pipeline_020(pc);
	}
#endif

	if  (!regs.prefetch020_valid[2]) {
		if (idx == 0 || !regs.pipeline_stop) {
			// Pipeline refill and first opcode word is invalid?
			// Generate previously detected bus error/MMU fault
			do_access_or_bus_error(pc, pc + idx * 2);
		}
	}
	return idx + 1;
}

void fill_prefetch_030_ntx()
{
	uaecptr pc = m68k_getpc ();
	uaecptr pc2 = pc;
	int idx = 0;

	pc &= ~3;
	mmu030_idx = mmu030_idx_done = 0;
	reset_pipeline_state();
	regs.cacheholdingdata_valid = 1;
	regs.cacheholdingaddr020 = 0xffffffff;
	regs.prefetch020_valid[0] = regs.prefetch020_valid[1] = regs.prefetch020_valid[2] = 0;

	fill_icache030(pc);
	if (pc2 & 2) {
		idx = add_prefetch_030(idx, regs.cacheholdingdata020, pc2);
	} else {
		idx = add_prefetch_030(idx, regs.cacheholdingdata020 >>	16, pc2);
		idx = add_prefetch_030(idx, regs.cacheholdingdata020, pc2);
	}

	fill_icache030(pc + 4);
	if (pc2 & 2) {
		idx = add_prefetch_030(idx, regs.cacheholdingdata020 >>	16, pc2);
		idx = add_prefetch_030(idx, regs.cacheholdingdata020, pc2);
	} else {
		idx = add_prefetch_030(idx, regs.cacheholdingdata020 >>	16, pc2);
	}

	ipl_fetch_now();
	if (currprefs.cpu_cycle_exact)
		regs.irc = get_word_ce030_prefetch_opcode (0);
	else
		regs.irc = get_word_030_prefetch (0);
}

void fill_prefetch_030_ntx_continue ()
{
	uaecptr pc = m68k_getpc ();
	uaecptr pc_orig = pc;
	int idx = 0;

	mmu030_idx = mmu030_idx_done = 0;
	reset_pipeline_state();
	regs.cacheholdingdata_valid = 1;
	regs.cacheholdingaddr020 = 0xffffffff;

	if (regs.prefetch020_valid[0] && regs.prefetch020_valid[1] && regs.prefetch020_valid[2]) {
		for (int i = 2; i >= 0; i--) {
			regs.prefetch020[i + 1] = regs.prefetch020[i];
		}
		for (int i = 1; i <= 3; i++) {
#if MORE_ACCURATE_68020_PIPELINE
			pipeline_020(pc);
#endif
			regs.prefetch020[i - 1] = regs.prefetch020[i];
			pc += 2;
			idx++;
		}
	} else if (regs.prefetch020_valid[2] && !regs.prefetch020_valid[1]) {
		regs.prefetch020_valid[1] = regs.prefetch020_valid[2];
		regs.prefetch020[1] = regs.prefetch020[2];
		regs.prefetch020_valid[2] = 0;
		pc += 2;
		idx++;
#if MORE_ACCURATE_68020_PIPELINE
		pipeline_020(pc);
#endif
		if (!regs.pipeline_stop) {
			if (maybe_icache030(pc)) {
				regs.prefetch020[2] = regs.cacheholdingdata020 >> (regs.cacheholdingaddr020 == pc ? 16 : 0);
			} else {
				regs.prefetch020[2] = icache_fetch_word(pc);
			}
			regs.prefetch020_valid[2] = 1;
			pc += 2;
			idx++;
#if MORE_ACCURATE_68020_PIPELINE
			pipeline_020(pc);
#endif
		}

	} else if (regs.prefetch020_valid[2] && regs.prefetch020_valid[1]) {
		pc += 2;
#if MORE_ACCURATE_68020_PIPELINE
		pipeline_020(pc);
#endif
		pc += 2;
#if MORE_ACCURATE_68020_PIPELINE
		pipeline_020(pc);
#endif
		idx += 2;
	}

	while (idx < 2) {
		regs.prefetch020[0] = regs.prefetch020[1];
		regs.prefetch020[1] = regs.prefetch020[2];
		regs.prefetch020_valid[0] = regs.prefetch020_valid[1];
		regs.prefetch020_valid[1] = regs.prefetch020_valid[2];
		regs.prefetch020_valid[2] = false;
		idx++;
	}

	if (idx < 3 && !regs.pipeline_stop) {
		uaecptr pc2 = pc;
		pc &= ~3;

		fill_icache030(pc);
		if (pc2 & 2) {
			idx = add_prefetch_030(idx, regs.cacheholdingdata020, pc_orig);
		} else {
			idx = add_prefetch_030(idx, regs.cacheholdingdata020 >> 16, pc_orig);
			if (idx < 3)
				idx = add_prefetch_030(idx, regs.cacheholdingdata020, pc_orig);
		}

		if (idx < 3) {
			fill_icache030(pc + 4);
			if (pc2 & 2) {
				idx = add_prefetch_030(idx, regs.cacheholdingdata020 >>	16, pc_orig);
				if (idx < 3)
					idx = add_prefetch_030(idx, regs.cacheholdingdata020, pc_orig);
			} else {
				idx = add_prefetch_030(idx, regs.cacheholdingdata020 >>	16, pc_orig);
			}
		}
	}

	ipl_fetch_now();
	if (currprefs.cpu_cycle_exact)
		regs.irc = get_word_ce030_prefetch_opcode(0);
	else
		regs.irc = get_word_030_prefetch(0);
}

void fill_prefetch_020_ntx()
{
	uaecptr pc = m68k_getpc ();
	uaecptr pc2 = pc;
	int idx = 0;

	pc &= ~3;
	reset_pipeline_state();

	fill_icache020 (pc, true);
	if (pc2 & 2) {
		idx = add_prefetch_030(idx, regs.cacheholdingdata020, pc);
	} else {
		idx = add_prefetch_030(idx, regs.cacheholdingdata020 >>	16, pc);
		idx = add_prefetch_030(idx, regs.cacheholdingdata020, pc);
	}

	fill_icache020 (pc + 4, true);
	if (pc2 & 2) {
		idx = add_prefetch_030(idx, regs.cacheholdingdata020 >>	16, pc);
		idx = add_prefetch_030(idx, regs.cacheholdingdata020, pc);
	} else {
		idx = add_prefetch_030(idx, regs.cacheholdingdata020 >>	16, pc);
	}

	ipl_fetch_now();
	if (currprefs.cpu_cycle_exact)
		regs.irc = get_word_ce020_prefetch_opcode (0);
	else
		regs.irc = get_word_020_prefetch (0);
}

// Not exactly right, requires logic analyzer checks.
void continue_ce020_prefetch()
{
	fill_prefetch_020_ntx();
}
void continue_020_prefetch()
{
	fill_prefetch_020_ntx();
}

void continue_ce030_prefetch()
{
	fill_prefetch_030_ntx();
}
void continue_030_prefetch()
{
	fill_prefetch_030_ntx();
}

void fill_prefetch_020()
{
	fill_prefetch_020_ntx();
	check_t0_trace();
}

void fill_prefetch_030()
{
	fill_prefetch_030_ntx();
	check_t0_trace();
}


void fill_prefetch ()
{
	if (currprefs.cachesize)
		return;
	if (!currprefs.cpu_compatible)
		return;
	reset_pipeline_state();
	if (currprefs.cpu_model >= 68040) {
		if (currprefs.cpu_compatible || currprefs.cpu_memory_cycle_exact) {
			fill_icache040(m68k_getpc() + 16);
			fill_icache040(m68k_getpc());
		}
	} else if (currprefs.cpu_model == 68020) {
		fill_prefetch_020 ();
	} else if (currprefs.cpu_model == 68030) {
		fill_prefetch_030 ();
	} else if (currprefs.cpu_model <= 68010) {
		uaecptr pc = m68k_getpc ();
		regs.ir = x_get_word (pc);
		regs.ird = regs.ir;
		regs.irc = x_get_word (pc + 2);
		regs.read_buffer = regs.irc;
	}
}

uae_u32 sfc_nommu_get_byte(uaecptr addr)
{
	uae_u32 v;
	ismoves_nommu = true;
	if (!cpuboard_fc_check(addr, &v, 0, false))
		v = x_get_byte(addr);
	ismoves_nommu = false;
	return v;
}
uae_u32 sfc_nommu_get_word(uaecptr addr)
{
	uae_u32 v;
	ismoves_nommu = true;
	if (!cpuboard_fc_check(addr, &v, 1, false))
		v = x_get_word(addr);
	ismoves_nommu = false;
	return v;
}
uae_u32 sfc_nommu_get_long(uaecptr addr)
{
	uae_u32 v;
	ismoves_nommu = true;
	if (!cpuboard_fc_check(addr, &v, 2, false))
		v = x_get_long(addr);
	ismoves_nommu = false;
	return v;
}
void dfc_nommu_put_byte(uaecptr addr, uae_u32 v)
{
	ismoves_nommu = true;
	if (!cpuboard_fc_check(addr, &v, 0, true))
		x_put_byte(addr, v);
	ismoves_nommu = false;
}
void dfc_nommu_put_word(uaecptr addr, uae_u32 v)
{
	ismoves_nommu = true;
	if (!cpuboard_fc_check(addr, &v, 1, true))
		x_put_word(addr, v);
	ismoves_nommu = false;
}
void dfc_nommu_put_long(uaecptr addr, uae_u32 v)
{
	ismoves_nommu = true;
	if (!cpuboard_fc_check(addr, &v, 2, true))
		x_put_long(addr, v);
	ismoves_nommu = false;
}
