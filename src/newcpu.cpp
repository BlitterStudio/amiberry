/*
* UAE - The Un*x Amiga Emulator
*
* MC68000 emulation
*
* (c) 1995 Bernd Schmidt
*/

#define MMUOP_DEBUG 2
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
//#include "disasm.h"
#include "cpummu.h"
#include "cpummu030.h"
#include "cputbl.h"
#include "cpu_prefetch.h"
#include "autoconf.h"
#include "traps.h"
//#include "debug.h"
//#include "debugmem.h"
#include "gui.h"
#include "savestate.h"
#include "blitter.h"
#include "ar.h"
#include "gayle.h"
#include "cia.h"
//#include "inputrecord.h"
#include "inputdevice.h"
#include "audio.h"
#include "fpp.h"
#include "statusline.h"
//#include "uae/ppc.h"
//#include "cpuboard.h"
#include "threaddep/thread.h"
//#include "x86.h"
#include "bsdsocket.h"
#include "devices.h"
#ifdef JIT
#include "jit/compemu.h"
#include <signal.h>
#else
/* Need to have these somewhere */
bool check_prefs_changed_comp (bool checkonly) { return false; }
#endif
/* For faster JIT cycles handling */
uae_s32 pissoff = 0;

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

int cpu_cycles;
int hardware_bus_error;
int m68k_pc_indirect;
bool m68k_interrupt_delay;
static bool m68k_reset_delay;

static volatile uae_atomic uae_interrupt;
static volatile uae_atomic uae_interrupts2[IRQ_SOURCE_MAX];
static volatile uae_atomic uae_interrupts6[IRQ_SOURCE_MAX];

static int cpu_prefs_changed_flag;

int cpu_tracer;

const int areg_byteinc[] = { 1, 1, 1, 1, 1, 1, 1, 2 };
const int imm8_table[] = { 8, 1, 2, 3, 4, 5, 6, 7 };

int movem_index1[256];
int movem_index2[256];
int movem_next[256];

cpuop_func *cpufunctbl[65536];

struct cputbl_data
{
	uae_s16 length;
	uae_s8 disp020[2];
	uae_s8 branch;
};
static struct cputbl_data cpudatatbl[65536];

struct mmufixup mmufixup[1];

static uae_u64 fake_srp_030, fake_crp_030;
static uae_u32 fake_tt0_030, fake_tt1_030, fake_tc_030;
static uae_u16 fake_mmusr_030;

int cpu_last_stop_vpos, cpu_stopped_lines;

static void exception3_notinstruction(uae_u32 opcode, uaecptr addr);

/*

 ok, all this to "record" current instruction state
 for later 100% cycle-exact restoring

 */

static uae_u32 (*x2_next_iword)(void);
static uae_u32 (*x2_next_ilong)(void);
static uae_u32 (*x2_get_iword)(int);
static uae_u32 (*x2_get_long)(uaecptr);
static uae_u32 (*x2_get_word)(uaecptr);
static uae_u32 (*x2_get_byte)(uaecptr);
static void (*x2_put_long)(uaecptr,uae_u32);
static void (*x2_put_word)(uaecptr,uae_u32);
static void (*x2_put_byte)(uaecptr,uae_u32);
static void (*x2_do_cycles)(unsigned long);
static void (*x2_do_cycles_pre)(unsigned long);
static void (*x2_do_cycles_post)(unsigned long, uae_u32);

uae_u32 (*x_next_iword)(void);
uae_u32 (*x_next_ilong)(void);
uae_u32 (*x_get_iword)(int);
uae_u32 (*x_get_long)(uaecptr);
uae_u32 (*x_get_word)(uaecptr);
uae_u32 (*x_get_byte)(uaecptr);
void (*x_put_long)(uaecptr,uae_u32);
void (*x_put_word)(uaecptr,uae_u32);
void (*x_put_byte)(uaecptr,uae_u32);

uae_u32 (*x_cp_next_iword)(void);
uae_u32 (*x_cp_next_ilong)(void);
uae_u32 (*x_cp_get_long)(uaecptr);
uae_u32 (*x_cp_get_word)(uaecptr);
uae_u32 (*x_cp_get_byte)(uaecptr);
void (*x_cp_put_long)(uaecptr,uae_u32);
void (*x_cp_put_word)(uaecptr,uae_u32);
void (*x_cp_put_byte)(uaecptr,uae_u32);

void (*x_do_cycles)(unsigned long);
void (*x_do_cycles_pre)(unsigned long);
void (*x_do_cycles_post)(unsigned long, uae_u32);

static void set_x_cp_funcs(void)
{
	x_cp_put_long = x_put_long;
	x_cp_put_word = x_put_word;
	x_cp_put_byte = x_put_byte;
	x_cp_get_long = x_get_long;
	x_cp_get_word = x_get_word;
	x_cp_get_byte = x_get_byte;
	x_cp_next_iword = x_next_iword;
	x_cp_next_ilong = x_next_ilong;
}

static struct cputracestruct cputrace;

STATIC_INLINE void clear_trace (void)
{
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
	cputrace.cyclecounter_pre = -1;
	if (accessmode == 1)
		cputrace.writecounter++;
	else
		cputrace.readcounter++;
}
static void add_trace (uaecptr addr, uae_u32 val, int accessmode, int size)
{
	if (cputrace.memoryoffset < 1) {
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
	cputrace.cyclecounter_pre = cputrace.cyclecounter_post = 0;
}

static bool check_trace (void)
{
	if (!cpu_tracer)
		return true;
	if (!cputrace.readcounter && !cputrace.writecounter && !cputrace.cyclecounter) {
		if (cpu_tracer != -2) {
			write_log (_T("CPU trace: dma_cycle() enabled. %08x %08x NOW=%08lx\n"),
				cputrace.cyclecounter_pre, cputrace.cyclecounter_post, get_cycles ());
			cpu_tracer = -2; // dma_cycle() allowed to work now
		}
	}
	if (cputrace.readcounter || cputrace.writecounter ||
		cputrace.cyclecounter || cputrace.cyclecounter_pre || cputrace.cyclecounter_post)
		return false;
	x_get_iword = x2_get_iword;
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
	write_log (_T("CPU tracer playback complete. STARTCYCLES=%08x NOWCYCLES=%08lx\n"), cputrace.startcycles, get_cycles ());
	cputrace.needendcycles = 1;
	cpu_tracer = 0;
	return true;
}

static bool get_trace (uaecptr addr, int accessmode, int size, uae_u32 *data)
{
	int mode = accessmode | (size << 4);
	for (int i = 0; i < cputrace.memoryoffset; i++) {
		struct cputracememory *ctm = &cputrace.ctm[i];
		if (ctm->addr == addr && ctm->mode == mode) {
			ctm->mode = 0;
			write_log (_T("CPU trace: GET %d: PC=%08x %08x=%08x %d %d %08x/%08x/%08x %d/%d (%08lx)\n"),
				i, cputrace.pc, addr, ctm->data, accessmode, size,
				cputrace.cyclecounter, cputrace.cyclecounter_pre, cputrace.cyclecounter_post,
				cputrace.readcounter, cputrace.writecounter, get_cycles ());
			if (accessmode == 1)
				cputrace.writecounter--;
			else
				cputrace.readcounter--;
			if (cputrace.writecounter == 0 && cputrace.readcounter == 0) {
				if (cputrace.cyclecounter_post) {
					int c = cputrace.cyclecounter_post;
					cputrace.cyclecounter_post = 0;
					x_do_cycles (c);
				} else if (cputrace.cyclecounter_pre) {
					check_trace ();
					*data = ctm->data;
					return true; // argh, need to rerun the memory access..
				}
			}
			check_trace ();
			*data = ctm->data;
			return false;
		}
	}
	if (cputrace.cyclecounter_post) {
		int c = cputrace.cyclecounter_post;
		cputrace.cyclecounter_post = 0;
		check_trace ();
		x_do_cycles (c);
		return false;
	}
	gui_message (_T("CPU trace: GET %08x %d %d NOT FOUND!\n"), addr, accessmode, size);
	check_trace ();
	*data = 0;
	return false;
}

static uae_u32 cputracefunc_x_next_iword (void)
{
	uae_u32 pc = m68k_getpc ();
	set_trace (pc, 2, 2);
	uae_u32 v = x2_next_iword ();
	add_trace (pc, v, 2, 2);
	return v;
}
static uae_u32 cputracefunc_x_next_ilong (void)
{
	uae_u32 pc = m68k_getpc ();
	set_trace (pc, 2, 4);
	uae_u32 v = x2_next_ilong ();
	add_trace (pc, v, 2, 4);
	return v;
}
static uae_u32 cputracefunc2_x_next_iword (void)
{
	uae_u32 v;
	if (get_trace (m68k_getpc (), 2, 2, &v)) {
		v = x2_next_iword ();
	}
	return v;
}
static uae_u32 cputracefunc2_x_next_ilong (void)
{
	uae_u32 v;
	if (get_trace (m68k_getpc (), 2, 4, &v)) {
		v = x2_next_ilong ();
	}
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
static uae_u32 cputracefunc2_x_get_iword (int o)
{
	uae_u32 v;
	if (get_trace (m68k_getpc () + o, 2, 2, &v)) {
		v = x2_get_iword (o);
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
	}
	return v;
}
static uae_u32 cputracefunc2_x_get_word (uaecptr o)
{
	uae_u32 v;
	if (get_trace (o, 0, 2, &v)) {
		v = x2_get_word (o);
	}
	return v;
}
static uae_u32 cputracefunc2_x_get_byte (uaecptr o)
{
	uae_u32 v;
	if (get_trace (o, 0, 1, &v)) {
		v = x2_get_byte (o);
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
	}
	if (v != val)
		write_log (_T("cputracefunc2_x_put_long %d <> %d\n"), v, val);
}
static void cputracefunc2_x_put_word (uaecptr o, uae_u32 val)
{
	uae_u32 v;
	if (get_trace (o, 1, 2, &v)) {
		x2_put_word (o, val);
	}
	if (v != val)
		write_log (_T("cputracefunc2_x_put_word %d <> %d\n"), v, val);
}
static void cputracefunc2_x_put_byte (uaecptr o, uae_u32 val)
{
	uae_u32 v;
	if (get_trace (o, 1, 1, &v)) {
		x2_put_byte (o, val);
	}
	if (v != val)
		write_log (_T("cputracefunc2_x_put_byte %d <> %d\n"), v, val);
}

static void cputracefunc_x_do_cycles (unsigned long cycles)
{
	while (cycles >= CYCLE_UNIT) {
		cputrace.cyclecounter += CYCLE_UNIT;
		cycles -= CYCLE_UNIT;
		x2_do_cycles (CYCLE_UNIT);
	}
	if (cycles > 0) {
		cputrace.cyclecounter += cycles;
		x2_do_cycles (cycles);
	}
}

static void cputracefunc2_x_do_cycles (unsigned long cycles)
{
	if (cputrace.cyclecounter > cycles) {
		cputrace.cyclecounter -= cycles;
		return;
	}
	cycles -= cputrace.cyclecounter;
	cputrace.cyclecounter = 0;
	check_trace ();
	x_do_cycles = x2_do_cycles;
	if (cycles > 0)
		x_do_cycles (cycles);
}

static void cputracefunc_x_do_cycles_pre (unsigned long cycles)
{
	cputrace.cyclecounter_post = 0;
	cputrace.cyclecounter_pre = 0;
	while (cycles >= CYCLE_UNIT) {
		cycles -= CYCLE_UNIT;
		cputrace.cyclecounter_pre += CYCLE_UNIT;
		x2_do_cycles (CYCLE_UNIT);
	}
	if (cycles > 0) {
		x2_do_cycles (cycles);
		cputrace.cyclecounter_pre += cycles;
	}
	cputrace.cyclecounter_pre = 0;
}
// cyclecounter_pre = how many cycles we need to SWALLOW
// -1 = rerun whole access
static void cputracefunc2_x_do_cycles_pre (unsigned long cycles)
{
	if (cputrace.cyclecounter_pre == -1) {
		cputrace.cyclecounter_pre = 0;
		check_trace ();
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

static void cputracefunc_x_do_cycles_post (unsigned long cycles, uae_u32 v)
{
	if (cputrace.memoryoffset < 1) {
		return;
	}
	struct cputracememory *ctm = &cputrace.ctm[cputrace.memoryoffset - 1];
	ctm->data = v;
	cputrace.cyclecounter_post = cycles;
	cputrace.cyclecounter_pre = 0;
	while (cycles >= CYCLE_UNIT) {
		cycles -= CYCLE_UNIT;
		cputrace.cyclecounter_post -= CYCLE_UNIT;
		x2_do_cycles (CYCLE_UNIT);
	}
	if (cycles > 0) {
		cputrace.cyclecounter_post -= cycles;
		x2_do_cycles (cycles);
	}
	cputrace.cyclecounter_post = 0;
}
// cyclecounter_post = how many cycles we need to WAIT
static void cputracefunc2_x_do_cycles_post (unsigned long cycles, uae_u32 v)
{
	uae_u32 c;
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

static void do_cycles_post (unsigned long cycles, uae_u32 v)
{
	do_cycles (cycles);
}
static void do_cycles_ce_post (unsigned long cycles, uae_u32 v)
{
	do_cycles_ce (cycles);
}
 
static void set_x_ifetches(void)
{
	if (m68k_pc_indirect) {
			// indirect via addrbank
			x_get_iword = get_iiword;
			x_next_iword = next_iiword;
			x_next_ilong = next_iilong;
	} else {
		// direct to memory
		x_get_iword = get_diword;
		x_next_iword = next_diword;
		x_next_ilong = next_dilong;
	}
}

// indirect memory access functions
static void set_x_funcs (void)
{
	if (currprefs.m68k_speed < 0 || currprefs.cachesize > 0)
		do_cycles = do_cycles_cpu_fastest;
	else
		do_cycles = do_cycles_cpu_norm;

	if (currprefs.cpu_model < 68020) {
		// 68000/010
		if (currprefs.cpu_cycle_exact) {
			x_get_iword = get_wordi_ce000;
			x_next_iword = NULL;
			x_next_ilong = NULL;
			x_put_long = put_long_ce000;
			x_put_word = put_word_ce000;
			x_put_byte = put_byte_ce000;
			x_get_long = get_long_ce000;
			x_get_word = get_word_ce000;
			x_get_byte = get_byte_ce000;
			x_do_cycles = do_cycles_ce;
			x_do_cycles_pre = do_cycles_ce;
			x_do_cycles_post = do_cycles_ce_post;
		} else if (currprefs.cpu_compatible) {
			// cpu_compatible only
			x_get_iword = get_iiword;
			x_next_iword = NULL;
			x_next_ilong = NULL;
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
			x_get_iword = get_diword;
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
	} else {
		// 68020+ no ce
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
	x2_get_iword = x_get_iword;
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
		x_get_iword = cputracefunc_x_get_iword;
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
			x_get_iword = cputracefunc2_x_get_iword;
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
}

bool can_cpu_tracer (void)
{
	return currprefs.cpu_model == 68000 && currprefs.cpu_memory_cycle_exact;
}

bool is_cpu_tracer (void)
{
	return cpu_tracer > 0;
}
bool set_cpu_tracer (bool state)
{
	if (cpu_tracer < 0)
		return false;
	int old = cpu_tracer;
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

static void flush_cpu_caches(bool force)
{
	bool doflush = currprefs.cpu_compatible || currprefs.cpu_memory_cycle_exact;

	if (currprefs.cpu_model == 68020) {
		regs.cacr &= ~0x08;
		regs.cacr &= ~0x04;
  } else if (currprefs.cpu_model == 68030) {
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

	for (int k = 0; k < 2; k++) {
		if (cache & (1 << k)) {
			if (scope == 3) {
				// all
        if (k) {
					// instruction
					flush_cpu_caches(true);
				}
			}
		}
	}
}

void set_cpu_caches (bool flush)
{
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

static uae_u32 REGPARAM2 op_illg_1 (uae_u32 opcode)
{
	op_illg(opcode);
	return 4;
}

// generic+direct, generic+direct+jit, more compatible, cycle-exact
static const struct cputbl *cputbls[5][4] =
{
	// 68000
	{ op_smalltbl_5, op_smalltbl_45, op_smalltbl_12, op_smalltbl_14 },
	// 68010
	{ op_smalltbl_4, op_smalltbl_44, op_smalltbl_11, op_smalltbl_13 },
	// 68020
	{ op_smalltbl_3, op_smalltbl_43, NULL, NULL },
	// 68030
	{ op_smalltbl_2, op_smalltbl_42, NULL, NULL },
	// 68040
	{ op_smalltbl_1, op_smalltbl_41, NULL, NULL },
};

static void build_cpufunctbl (void)
{
	int i, opcnt;
	unsigned long opcode;
	const struct cputbl *tbl = NULL;
	int lvl, mode;

	if (!currprefs.cachesize) {
		if (currprefs.cpu_cycle_exact) {
			mode = 3;
		} else if (currprefs.cpu_compatible && currprefs.cpu_model < 68020) {
			mode = 2;
		} else {
			mode = 0;
		}
		m68k_pc_indirect = mode != 0 ? 1 : 0;
	} else {
		mode = 1;
		m68k_pc_indirect = 0;
	}
	lvl = (currprefs.cpu_model - 68000) / 10;
	if (lvl >= 5)
		lvl = 4;
	tbl = cputbls[lvl][mode];

	if (tbl == NULL) {
		write_log (_T("no CPU emulation cores available CPU=%d!"), currprefs.cpu_model);
		abort ();
	}

	for (opcode = 0; opcode < 65536; opcode++)
		cpufunctbl[opcode] = op_illg_1;
	for (i = 0; tbl[i].handler_ff != NULL; i++) {
		opcode = tbl[i].opcode;
		cpufunctbl[opcode] = tbl[i].handler_ff;
		cpudatatbl[opcode].length = tbl[i].length;
		cpudatatbl[opcode].disp020[0] = tbl[i].disp020[0];
		cpudatatbl[opcode].disp020[1] = tbl[i].disp020[1];
		cpudatatbl[opcode].branch = tbl[i].branch;
	}

	/* hack fpu to 68000/68010 mode */
	if (currprefs.fpu_model && currprefs.cpu_model < 68020) {
		tbl = op_smalltbl_3;
		for (i = 0; tbl[i].handler_ff != NULL; i++) {
			if ((tbl[i].opcode & 0xfe00) == 0xf200) {
				cpufunctbl[tbl[i].opcode] = tbl[i].handler_ff;
				cpudatatbl[tbl[i].opcode].length = tbl[i].length;
				cpudatatbl[tbl[i].opcode].disp020[0] = tbl[i].disp020[0];
				cpudatatbl[tbl[i].opcode].disp020[1] = tbl[i].disp020[1];
				cpudatatbl[tbl[i].opcode].branch = tbl[i].branch;
			}
		}
	}

	opcnt = 0;
	for (opcode = 0; opcode < 65536; opcode++) {
		cpuop_func *f;
		struct instr *table = &table68k[opcode];

		if (table->mnemo == i_ILLG)
			continue;

		/* unimplemented opcode? */
		if (table->unimpclev > 0 && lvl >= table->unimpclev) {
			cpufunctbl[opcode] = op_illg_1;
			continue;
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
			f = cpufunctbl[idx];
			if (f == op_illg_1)
				abort ();
			cpufunctbl[opcode] = f;
			memcpy(&cpudatatbl[opcode], &cpudatatbl[idx], sizeof(struct cputbl_data));
			opcnt++;
		}
	}
	write_log (_T("Building CPU, %d opcodes (%d %d %d)\n"),
		opcnt, lvl,
		currprefs.cpu_cycle_exact ? -2 : currprefs.cpu_memory_cycle_exact ? -1 : currprefs.cpu_compatible ? 1 : 0, currprefs.address_space_24);
#ifdef JIT
	build_comp();
#endif

	write_log(_T("CPU=%d, FPU=%d%s, JIT%s=%d."),
	          currprefs.cpu_model,
	          currprefs.fpu_model, currprefs.fpu_model ? _T(" (host)") : _T(""),
	          currprefs.cachesize ? (currprefs.compfpu ? _T("=CPU/FPU") : _T("=CPU")) : _T(""),
	          currprefs.cachesize);

	regs.address_space_mask = 0xffffffff;
	if (currprefs.cpu_compatible) {
		if (currprefs.address_space_24 && currprefs.cpu_model >= 68040)
			currprefs.address_space_24 = false;
	}
	m68k_interrupt_delay = false;
	if (currprefs.cpu_cycle_exact) {
		if (tbl == op_smalltbl_14 || tbl == op_smalltbl_13)
			m68k_interrupt_delay = true;
	} else if (currprefs.cpu_compatible) {
  	if (currprefs.address_space_24 && currprefs.cpu_model >= 68040)
	    currprefs.address_space_24 = false;
		if (currprefs.m68k_speed == 0) {
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
	if (currprefs.fpu_no_unimplemented && currprefs.fpu_model) {
		write_log(_T(" no unimplemented floating point instructions"));
	}
	if (currprefs.address_space_24) {
		regs.address_space_mask = 0x00ffffff;
		write_log(_T(" 24-bit"));
	}
	write_log(_T("\n"));

	set_cpu_caches (true);
	target_cpu_speed();
}

static uae_u32 cycles_shift;
static uae_u32 cycles_shift_2;

static void update_68k_cycles (void)
{
	cycles_shift = 0;
	cycles_shift_2 = 0;
	if (currprefs.m68k_speed >= 0) {
		if (currprefs.m68k_speed == M68K_SPEED_14MHZ_CYCLES)
			cycles_shift = 1;
		else if (currprefs.m68k_speed == M68K_SPEED_25MHZ_CYCLES) {
			if (currprefs.cpu_model >= 68040) {
				cycles_shift = 4;
      } else {
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

static void prefs_changed_cpu (void)
{
	fixup_cpu (&changed_prefs);
	check_prefs_changed_comp(false);
	currprefs.cpu_model = changed_prefs.cpu_model;
	currprefs.fpu_model = changed_prefs.fpu_model;
	if (currprefs.cpu_compatible != changed_prefs.cpu_compatible) {
		currprefs.cpu_compatible = changed_prefs.cpu_compatible;
		flush_cpu_caches(true);
	}
	if (currprefs.cpu_data_cache != changed_prefs.cpu_data_cache) {
		currprefs.cpu_data_cache = changed_prefs.cpu_data_cache;
	}
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

void check_prefs_changed_cpu(void)
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

void init_m68k (void)
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

	init_table68k();

	write_log (_T("%d CPU functions\n"), nr_cpuop_funcs);
}

struct regstruct regs;

STATIC_INLINE int in_rom (uaecptr pc)
{
	return (munge24 (pc) & 0xFFF80000) == 0xF80000;
}

STATIC_INLINE int in_rtarea (uaecptr pc)
{
	return (munge24 (pc) & 0xFFFF0000) == rtarea_base && (uae_boot_rom_type || currprefs.uaeboard > 0);
}

STATIC_INLINE uae_u32 adjust_cycles (uae_u32 cycles)
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
	unset_special (SPCFLAG_TRACE);
	set_special (SPCFLAG_DOTRACE);
}

// make sure interrupt is checked immediately after current instruction
static void doint_imm(void)
{
	doint();
	if (!currprefs.cachesize && !(regs.spcflags & SPCFLAG_INT) && (regs.spcflags & SPCFLAG_DOINT))
		set_special(SPCFLAG_INT);
}

void REGPARAM2 MakeSR (void)
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

	SET_XFLG ((regs.sr >> 4) & 1);
	SET_NFLG ((regs.sr >> 3) & 1);
	SET_ZFLG ((regs.sr >> 2) & 1);
	SET_VFLG ((regs.sr >> 1) & 1);
	SET_CFLG (regs.sr & 1);
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
	regs.intmask = (regs.sr >> 8) & 7;

	if (currprefs.cpu_model >= 68020) {
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

#ifdef JIT
	// if JIT enabled and T1, T0 or M changes: end compile.
	if (currprefs.cachesize && (oldt0 != regs.t0 || oldt1 != regs.t1 || oldm != regs.m)) {
		set_special(SPCFLAG_END_COMPILE);
	}
#endif

	doint_imm();
	if (regs.t1 || regs.t0) {
		set_special (SPCFLAG_TRACE);
	} else {
		/* Keep SPCFLAG_DOTRACE, we still want a trace exception for
		SR-modifying instructions (including STOP).  */
		unset_special (SPCFLAG_TRACE);
	}
	// Stop SR-modification does not generate T0
	// If this SR modification set Tx bit, no trace until next instruction.
	if ((oldt0 && t0trace && currprefs.cpu_model >= 68020) || oldt1) {
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

static bool internalexception(int nr)
{
	return nr == 5 || nr == 6 || nr == 7 || (nr >= 32 && nr <= 47);
}

static void exception_check_trace (int nr)
{
	unset_special (SPCFLAG_TRACE | SPCFLAG_DOTRACE);
	if (regs.t1) {
		/* trace stays pending if exception is div by zero, chk,
		* trapv or trap #x. Except if 68040.
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

#ifdef CPUEMU_13

/* cycle-exact exception handler, 68000 only */

static int iack_cycle(int nr)
{
	int vector;

	// non-autovectored
	vector = x_get_byte(0x00fffff1 | ((nr - 24) << 1));
	if (currprefs.cpu_cycle_exact)
		x_do_cycles(4 * CYCLE_UNIT / 2);
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
		x_do_cycles (start * CYCLE_UNIT / 2);

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
		write_log(_T("Exception %d (%08x %x) at %x -> %x!\n"),
			nr, last_op_for_exception_3, last_addr_for_exception_3, currpc, get_long(4 * nr));
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
		if (interrupt)
			vector_nr = iack_cycle(nr);
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
		if (interrupt)
			vector_nr = iack_cycle(nr);
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
	m68k_setpc (newpc);
	regs.ir = x_get_word (m68k_getpc ()); // prefetch 1
	if (hardware_bus_error) {
		if (nr == 2 || nr == 3) {
			cpu_halt(CPU_HALT_DOUBLE_FAULT);
			return;
		}
		exception2_fetch(regs.irc, 0, 0);
		return;
	}
	x_do_cycles (2 * CYCLE_UNIT / 2);
	regs.ipl_pin = intlev();
	ipl_fetch();
	regs.irc = x_get_word (m68k_getpc () + 2); // prefetch 2
	if (hardware_bus_error) {
		if (nr == 2 || nr == 3) {
			cpu_halt(CPU_HALT_DOUBLE_FAULT);
			return;
		}
		exception2_fetch(regs.ir, 0, 2);
		return;
	}
#ifdef JIT
	set_special (SPCFLAG_END_COMPILE);
#endif
	exception_check_trace (nr);
}
#endif

static void add_approximate_exception_cycles(int nr)
{
	int cycles;

	if (currprefs.cpu_model == 68000) {
  	// 68000 exceptions
  	if (nr >= 24 && nr <= 31) {
			/* Interrupts */
  		cycles = 44 * CYCLE_UNIT / 2; 
  	} else if (nr >= 32 && nr <= 47) {
  		/* Trap (total is 34, but cpuemu_x.cpp already adds 4) */ 
  		cycles = (34 - 4) * CYCLE_UNIT / 2;
  	} else {
  		switch (nr)
		{
  			case 2: cycles = 50 * CYCLE_UNIT / 2; break;		/* Bus error */
  			case 3: cycles = 50 * CYCLE_UNIT / 2; break;		/* Address error */
  			case 4: cycles = 34 * CYCLE_UNIT / 2; break;		/* Illegal instruction */
  			case 5: cycles = 38 * CYCLE_UNIT / 2; break;		/* Division by zero */
  			case 6: cycles = 40 * CYCLE_UNIT / 2; break;		/* CHK */
  			case 7: cycles = (34 - 8) * CYCLE_UNIT / 2; break;		/* TRAPV */
  			case 8: cycles = 34 * CYCLE_UNIT / 2; break;		/* Privilege violation */
  			case 9: cycles = 34 * CYCLE_UNIT / 2; break;		/* Trace */
  			case 10: cycles = 34 * CYCLE_UNIT / 2; break;	/* Line-A */
  			case 11: cycles = 34 * CYCLE_UNIT / 2; break;	/* Line-F */
  			default:
    			cycles = 4 * CYCLE_UNIT / 2;
    			break;
  		}
		}
  } else if (currprefs.cpu_model == 68010) {
  	// 68010 exceptions
		if (nr >= 24 && nr <= 31) {
			/* Interrupts */
			cycles = 46 * CYCLE_UNIT / 2;
		} else if (nr >= 32 && nr <= 47) {
			/* Trap */
			cycles = (38 - 4) * CYCLE_UNIT / 2;
		} else {
			switch (nr)
			{
			  case 2: cycles = 126 * CYCLE_UNIT / 2; break;	/* Bus error */
			  case 3: cycles = 126 * CYCLE_UNIT / 2; break;	/* Address error */
			  case 4: cycles = 38 * CYCLE_UNIT / 2; break;		/* Illegal instruction */
			  case 5: cycles = 42 * CYCLE_UNIT / 2; break;		/* Division by zero */
			  case 6: cycles = 40 * CYCLE_UNIT / 2; break;		/* CHK */
			  case 7: cycles = (38 - 8) * CYCLE_UNIT / 2; break;		/* TRAPV */
			  case 8: cycles = 38 * CYCLE_UNIT / 2; break;		/* Privilege violation */
			  case 9: cycles = 38 * CYCLE_UNIT / 2; break;		/* Trace */
			  case 10: cycles = 38 * CYCLE_UNIT / 2; break;	/* Line-A */
			  case 11: cycles = 38 * CYCLE_UNIT / 2; break;	/* Line-F */
			  case 14: cycles = 50 * CYCLE_UNIT / 2; break;	/* RTE frame error */
			default:
				cycles = 4 * CYCLE_UNIT / 2;
				break;
			}
		}
	} else {
  	// 68020 exceptions
  	if (nr >= 24 && nr <= 31) {
			/* Interrupts */
  		cycles = 26 * CYCLE_UNIT / 2; 
  	} else if (nr >= 32 && nr <= 47) {
  		/* Trap */ 
  		cycles = 20 * CYCLE_UNIT / 2;
  	} else {
			switch (nr)
			{
  			case 2: cycles = 43 * CYCLE_UNIT / 2; break;		/* Bus error */
  			case 3: cycles = 43 * CYCLE_UNIT / 2; break;		/* Address error ??? */
  			case 4: cycles = 20 * CYCLE_UNIT / 2; break;		/* Illegal instruction */
  			case 5: cycles = 32 * CYCLE_UNIT / 2; break;		/* Division by zero */
  			case 6: cycles = 32 * CYCLE_UNIT / 2; break;		/* CHK */
  			case 7: cycles = 25 * CYCLE_UNIT / 2; break;		/* TRAPV */
  			case 8: cycles = 20 * CYCLE_UNIT / 2; break;		/* Privilege violation */
  			case 9: cycles = 25 * CYCLE_UNIT / 2; break;		/* Trace */
  			case 10: cycles = 20 * CYCLE_UNIT / 2; break;	/* Line-A */
  			case 11: cycles = 20 * CYCLE_UNIT / 2; break;	/* Line-F */
			  case 14: cycles = 21 * CYCLE_UNIT / 2; break;	/* RTE frame error */
			default:
				cycles = 4 * CYCLE_UNIT / 2;
				break;
			}
		}
	}
	cycles = adjust_cycles(cycles);
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

	interrupt = nr >= 24 && nr < 24 + 8;

	if (currprefs.cpu_model <= 68010) {
		g1 = generates_group1_exception(regs.ir);
		if (interrupt)
			vector_nr = iack_cycle(nr);
	}

	MakeSR ();

	if (!regs.s) {
		regs.usp = m68k_areg (regs, 7);
  	if (currprefs.cpu_model >= 68020) {
			m68k_areg (regs, 7) = regs.m ? regs.msp : regs.isp;
		} else {
			m68k_areg (regs, 7) = regs.isp;
		}
		regs.s = 1;
	}

	if ((m68k_areg(regs, 7) & 1) && currprefs.cpu_model < 68020) {
		if (nr == 2 || nr == 3)
			cpu_halt(CPU_HALT_DOUBLE_FAULT);
		else
			exception3_notinstruction(regs.ir, m68k_areg(regs, 7));
		return;
	}
	if ((nr == 2 || nr == 3) && exception_in_exception < 0) {
		cpu_halt(CPU_HALT_DOUBLE_FAULT);
		return;
	}

	if (!currprefs.cpu_compatible) {
		addrbank* ab = &get_mem_bank(m68k_areg(regs, 7) - 4);
		// Not plain RAM check because some CPU type tests that
		// don't need to return set stack to ROM..
		if (!ab || ab == &dummy_bank || (ab->flags & ABFLAG_IO)) {
			cpu_halt(CPU_HALT_SSP_IN_NON_EXISTING_ADDRESS);
			return;
		}
	}

	bool used_exception_build_stack_frame = false;

	add_approximate_exception_cycles(nr);
  if (currprefs.cpu_model > 68000) {
		uae_u32 oldpc = regs.instruction_pc;
		nextpc = exception_pc(nr);
  	if (nr == 2 || nr == 3) {
			int i;
	    if (currprefs.cpu_model >= 68040) {
    		if (nr == 2) {

					// 68040 bus error (not really, some garbage?)
	  	    for (i = 0 ; i < 18 ; i++) {
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

    		} else {
				// 68040/060 odd PC address error
				Exception_build_stack_frame(last_fault_for_exception_3, currpc, 0, nr, 0x02);
				used_exception_build_stack_frame = true;
			}
			} else if (currprefs.cpu_model >= 68020) {
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
				ssw |= last_di_for_exception_3 > 0 ? 0x0000 : (last_di_for_exception_3 < 0 ? (0x2000 | 0x1000) : 0x2000);
				ssw |= (!last_writeaccess_for_exception_3 && last_di_for_exception_3) ? 0x1000 : 0x000; // DF
				ssw |= (last_op_for_exception_3 & 0x10000) ? 0x0400 : 0x0000; // HB
				ssw |= last_size_for_exception_3 == 0 ? 0x0200 : 0x0000; // BY
				ssw |= last_writeaccess_for_exception_3 ? 0x0000 : 0x0100; // RW
				if (last_op_for_exception_3 & 0x20000)
					ssw &= 0x00ff;
				regs.mmu_fault_addr = last_fault_for_exception_3;
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
		} else {
			Exception_build_stack_frame_common(oldpc, currpc, nr);
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
			goto kludge_me_do;
		}
	}
	if (!used_exception_build_stack_frame) {
		m68k_areg(regs, 7) -= 4;
		x_put_long(m68k_areg(regs, 7), nextpc);
		m68k_areg(regs, 7) -= 2;
		x_put_word(m68k_areg(regs, 7), regs.sr);
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
	m68k_setpc(newpc);
#ifdef JIT
	set_special(SPCFLAG_END_COMPILE);
#endif
	regs.ipl_pin = intlev();
	ipl_fetch();
	fill_prefetch();
	exception_check_trace(nr);
}

static void ExceptionX (int nr, uaecptr oldpc)
{
	uaecptr pc = m68k_getpc();
	regs.exception = nr;
    if (cpu_tracer) {
		cputrace.state = nr;
	}
	if (oldpc != 0xffffffff) {
		regs.instruction_pc = oldpc;
	}

#ifdef CPUEMU_13
	if (currprefs.cpu_cycle_exact && currprefs.cpu_model <= 68010)
		Exception_ce000 (nr);
	else
#endif
	{
		Exception_normal(nr);
	}
	regs.exception = 0;
	if (cpu_tracer) {
		cputrace.state = 0;
	}
}

void REGPARAM2 Exception_cpu_oldpc(int nr, uaecptr oldpc)
{
	bool t0 = currprefs.cpu_model >= 68020 && regs.t0 && !regs.t1;
	ExceptionX(nr, oldpc);
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
void REGPARAM2 Exception (int nr)
{
	ExceptionX (nr, 0xffffffff);
}

static void bus_error(void)
{
	TRY (prb2) {
		Exception (2);
	} CATCH (prb2) {
		cpu_halt (CPU_HALT_BUS_ERROR_DOUBLE_FAULT);
	} ENDTRY
}

static void do_interrupt (int nr)
{
	m68k_unset_stop();

	for (;;) {
		Exception (nr + 24);
		if (!currprefs.cpu_compatible)
			break;
		if (m68k_interrupt_delay)
			nr = regs.ipl;
		else
			nr = intlev();
		if (nr <= 0 || regs.intmask >= nr)
			break;
	}

	doint ();
}

void NMI (void)
{
	do_interrupt (7);
}

static void maybe_disable_fpu(void)
{
	if (!currprefs.fpu_model) {
		regs.pcr |= 2;
	}
}

static void m68k_reset_sr(void)
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

static void m68k_reset(bool hardreset)
{
	uae_u32 v;

	pissoff = 0;

	regs.halted = 0;
	gui_data.cpu_halted = 0;
	gui_led (LED_CPU, 0, -1);

	regs.spcflags = 0;
	m68k_reset_delay = 0;
	regs.ipl = regs.ipl_pin = 0;

#ifdef SAVESTATE
	if (isrestore ()) {
		m68k_reset_sr();
		m68k_setpc_normal (regs.pc);
		return;
	} else {
		m68k_reset_delay = currprefs.reset_delay;
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
	SET_ZFLG (0);
	SET_XFLG (0);
	SET_CFLG (0);
	SET_VFLG (0);
	SET_NFLG (0);
	regs.intmask = 7;
	regs.vbr = regs.sfc = regs.dfc = 0;
	regs.irc = 0xffff;
#ifdef FPUEMU
	fpu_reset ();
#endif
	regs.caar = regs.cacr = 0;
	regs.itt0 = regs.itt1 = regs.dtt0 = regs.dtt1 = 0;
	regs.tcr = regs.mmusr = regs.urp = regs.srp = 0;
	if (currprefs.cpu_model == 68020) {
		regs.cacr |= 8;
		set_cpu_caches(false);
	}

	mmufixup[0].reg = -1;
	if (currprefs.cpu_model >= 68040) {
		set_cpu_caches(false);
	}
	/* only (E)nable bit is zeroed when CPU is reset, A3000 SuperKickstart expects this */
	fake_tc_030 &= ~0x80000000;
	fake_tt0_030 &= ~0x80000000;
	fake_tt1_030 &= ~0x80000000;
	if (hardreset || regs.halted) {
		fake_srp_030 = fake_crp_030 = 0;
		fake_tt0_030 = fake_tt1_030 = fake_tc_030 = 0;
	}
	fake_mmusr_030 = 0;

	regs.pcr = 0;

	fill_prefetch();
}

uae_u32 REGPARAM2 op_illg (uae_u32 opcode)
{
	uaecptr pc = m68k_getpc();
	int inrom = in_rom(pc);
	int inrt = in_rtarea(pc);

	if ((opcode == 0x4afc || opcode == 0xfc4a) && !valid_address(pc, 4) && valid_address(pc - 4, 4)) {
		// PC fell off the end of RAM
		bus_error();
		return 4;
	}

	if (cloanto_rom && (opcode & 0xF100) == 0x7100) {
		m68k_dreg (regs, (opcode >> 9) & 7) = (uae_s8)(opcode & 0xFF);
		m68k_incpc_normal (2);
		fill_prefetch ();
		return 4;
	}

	if (opcode == 0x4E7B && inrom) {
		if (get_long (0x10) == 0) {
			notify_user (NUMSG_KS68020);
			uae_restart (-1, NULL);
			m68k_setstopped();
			return 4;
		}
	}

#ifdef AUTOCONFIG
	if (opcode == 0xFF0D && inrt) {
		/* User-mode STOP replacement */
		m68k_setstopped();
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
		Exception(0xB);
		}
		return 4;
	}
	if ((opcode & 0xF000) == 0xA000) {
		Exception (0xA);
		return 4;
	}

	Exception (4);
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

static bool mmu_op30fake_pmove (uaecptr pc, uae_u32 opcode, uae_u16 next, uaecptr extra)
{
	int preg = (next >> 10) & 31;
	int rw = (next >> 9) & 1;
	int fd = (next >> 8) & 1;
	int unused = (next & 0xff);
	const TCHAR *reg = NULL;
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
			x_put_long (extra + 4, (uae_u32)fake_srp_030);
		} else {
			fake_srp_030 = (uae_u64)x_get_long (extra) << 32;
			fake_srp_030 |= x_get_long (extra + 4);
		}
		break;
	case 0x13: // CRP
		reg = _T("CRP");
		siz = 8;
		if (rw) {
			x_put_long (extra, fake_crp_030 >> 32);
			x_put_long (extra + 4, (uae_u32)fake_crp_030);
		} else {
			fake_crp_030 = (uae_u64)x_get_long (extra) << 32;
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
	if (fline) {
		m68k_setpc(pc);
		op_illg(opcode);
	}
	return fline;
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
void mmu_op(uae_u32 opcode, uae_u32 extra)
{
  if ((opcode & 0xFE0) == 0x0500) {
		/* PFLUSH */
		regs.mmusr = 0;
		return;
  } else if ((opcode & 0x0FD8) == 0x0548) {
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
		return;
	}
	m68k_setpc_normal(m68k_getpc() - 2);
	op_illg(opcode);
}

#endif

static void do_trace(void)
{
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

static void check_uae_int_request(void)
{
	if (uae_int_requested) {
		bool irq = false;
		if (uae_int_requested & 0x00ff) {
			INTREQ_f(0x8000 | 0x0008);
			irq = true;
		}
		if (uae_int_requested & 0xff00) {
			INTREQ_f(0x8000 | 0x2000);
			irq = true;
		}
		if (uae_int_requested & 0xff0000) {
			atomic_and(&uae_int_requested, ~0x010000);
		}
		if (irq) {
			doint();
		}
	}
}

void safe_interrupt_set(int num, int id, bool i6)
{
	//if (!is_mainthread()) {
	//	set_special_exter(SPCFLAG_UAEINT);
	//	volatile uae_atomic* p;
	//	if (i6)
	//		p = &uae_interrupts6[num];
	//	else
	//		p = &uae_interrupts2[num];
	//	atomic_or(p, 1 << id);
	//	atomic_or(&uae_interrupt, 1);
	//}
	//else {
		uae_u16 v = i6 ? 0x2000 : 0x0008;
		if (currprefs.cpu_cycle_exact || (!(intreq & v) && !currprefs.cpu_cycle_exact)) {
			INTREQ_0(0x8000 | v);
		}
	//}
}

int cpu_sleep_millis(int ms)
{
	return sleep_millis_main(ms);
}

static bool haltloop(void)
{
	while (regs.halted) {
		static int prevvpos;
		if (vpos == 0 && prevvpos) {
			prevvpos = 0;
			cpu_sleep_millis(8);
		}
		if (vpos)
			prevvpos = 1;
		x_do_cycles(8 * CYCLE_UNIT);

		if (regs.spcflags & SPCFLAG_COPPER)
			do_copper();

		if (regs.spcflags) {
			if ((regs.spcflags & (SPCFLAG_BRK | SPCFLAG_MODE_CHANGE)))
				return true;
		}
	}

	return false;
}

// handle interrupt delay (few cycles)
STATIC_INLINE bool time_for_interrupt(void)
{
	return regs.ipl > regs.intmask || regs.ipl == 7;
}

void doint(void)
{
	if (m68k_interrupt_delay) {
		regs.ipl_pin = intlev();
		if (regs.ipl_pin > regs.intmask || regs.ipl_pin == 7)
			set_special(SPCFLAG_INT);
		return;
	}
	if (currprefs.cpu_compatible && currprefs.cpu_model < 68020)
		set_special(SPCFLAG_INT);
	else
		set_special(SPCFLAG_DOINT);
}

static int do_specialties(int cycles)
{
	if (regs.spcflags & SPCFLAG_MODE_CHANGE)
		return 1;

	if (regs.spcflags & SPCFLAG_CHECK) {
		if (regs.halted) {
			unset_special(SPCFLAG_CHECK);
			if (haltloop())
				return 1;
		}
		if (m68k_reset_delay) {
			int vsynccnt = 60;
			int vsyncstate = -1;
			while (vsynccnt > 0 && !quit_program) {
				x_do_cycles(8 * CYCLE_UNIT);
				if (regs.spcflags & SPCFLAG_COPPER)
					do_copper();
				if (timeframes != vsyncstate) {
					vsyncstate = timeframes;
					vsynccnt--;
				}
			}
		}
		m68k_reset_delay = 0;
		unset_special(SPCFLAG_CHECK);
	}

#ifdef ACTION_REPLAY
#ifdef ACTION_REPLAY_HRTMON
	if ((regs.spcflags & SPCFLAG_ACTION_REPLAY) && hrtmon_flag != ACTION_REPLAY_INACTIVE) {
		int isinhrt = (m68k_getpc() >= hrtmem_start && m68k_getpc() < hrtmem_start + hrtmem_size);
		/* exit from HRTMon? */
		if (hrtmon_flag == ACTION_REPLAY_ACTIVE && !isinhrt)
			hrtmon_hide();
		/* HRTMon breakpoint? (not via IRQ7) */
		if (hrtmon_flag == ACTION_REPLAY_ACTIVATE)
			hrtmon_enter();
	}
#endif
	if ((regs.spcflags & SPCFLAG_ACTION_REPLAY) && action_replay_flag != ACTION_REPLAY_INACTIVE) {
		/*if (action_replay_flag == ACTION_REPLAY_ACTIVE && !is_ar_pc_in_rom ())*/
		/*	write_log (_T("PC:%p\n"), m68k_getpc ());*/

		if (action_replay_flag == ACTION_REPLAY_ACTIVATE || action_replay_flag == ACTION_REPLAY_DORESET)
			action_replay_enter();
		if ((action_replay_flag == ACTION_REPLAY_HIDE || action_replay_flag == ACTION_REPLAY_ACTIVE) && !is_ar_pc_in_rom ()) {
			action_replay_hide();
			unset_special(SPCFLAG_ACTION_REPLAY);
		}
		if (action_replay_flag == ACTION_REPLAY_WAIT_PC) {
			/*write_log (_T("Waiting for PC: %p, current PC= %p\n"), wait_for_pc, m68k_getpc ());*/
			if (m68k_getpc () == wait_for_pc) {
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

	while ((regs.spcflags & SPCFLAG_BLTNASTY) && dmaen (DMA_BLITTER) && cycles > 0 && ((currprefs.waiting_blits && currprefs.cpu_model >= 68020) || !currprefs.blitter_cycle_exact)) {
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
		if (regs.spcflags & SPCFLAG_COPPER)
			do_copper();
	}

	if (regs.spcflags & SPCFLAG_DOTRACE)
		Exception(9);

	if (regs.spcflags & SPCFLAG_TRAP) {
		unset_special(SPCFLAG_TRAP);
		Exception(3);
	}
	
	if ((regs.spcflags & SPCFLAG_STOP) && regs.s == 0 && currprefs.cpu_model <= 68010) {
		// 68000/68010 undocumented special case:
		// if STOP clears S-bit and T was not set:
		// cause privilege violation exception, PC pointing to following instruction.
		// If T was set before STOP: STOP works as documented.
		m68k_unset_stop();
		Exception(8);
	}

	bool first = true;
	while ((regs.spcflags & SPCFLAG_STOP) && !(regs.spcflags & SPCFLAG_BRK)) {
		check_uae_int_request();
		if (bsd_int_requested)
			bsdsock_fake_int_handler();
		
		if (cpu_tracer > 0) {
			cputrace.stopped = regs.stopped;
			cputrace.intmask = regs.intmask;
			cputrace.sr = regs.sr;
			cputrace.state = 1;
			cputrace.pc = m68k_getpc ();
			cputrace.memoryoffset = 0;
			cputrace.cyclecounter = cputrace.cyclecounter_pre = cputrace.cyclecounter_post = 0;
			cputrace.readcounter = cputrace.writecounter = 0;
		}
		if (!first)
			x_do_cycles(currprefs.cpu_cycle_exact ? 2 * CYCLE_UNIT : 4 * CYCLE_UNIT);
		first = false;
		if (regs.spcflags & SPCFLAG_COPPER)
			do_copper();

		if (m68k_interrupt_delay) {
			unset_special(SPCFLAG_INT);
			ipl_fetch ();
			if (time_for_interrupt()) {
				do_interrupt(regs.ipl);
			}
		} else {
			if (regs.spcflags & (SPCFLAG_INT | SPCFLAG_DOINT)) {
				int intr = intlev();
				unset_special(SPCFLAG_INT | SPCFLAG_DOINT);
					if (intr > 0 && intr > regs.intmask)
						do_interrupt(intr);
			}
		}

		if (regs.spcflags & SPCFLAG_MODE_CHANGE) {
			m68k_resumestopped();
			return 1;
		}
	}

	if (regs.spcflags & SPCFLAG_TRACE)
		do_trace();

	if (regs.spcflags & SPCFLAG_UAEINT) {
		check_uae_int_request();
		unset_special(SPCFLAG_UAEINT);
	}
	
	if (m68k_interrupt_delay) {
		if (time_for_interrupt()) {
			unset_special(SPCFLAG_INT);
			do_interrupt(regs.ipl);
		}
	} else {
		if (regs.spcflags & SPCFLAG_INT) {
			int intr = intlev();
			unset_special(SPCFLAG_INT | SPCFLAG_DOINT);
			if (intr > 0 && (intr > regs.intmask || intr == 7))
				do_interrupt(intr);
		}
	}

  if (regs.spcflags & SPCFLAG_DOINT) {
		unset_special(SPCFLAG_DOINT);
		set_special(SPCFLAG_INT);
	}

  if (regs.spcflags & SPCFLAG_BRK) {
		unset_special(SPCFLAG_BRK);
	}

	return 0;
}

#ifndef CPUEMU_11

static void m68k_run_1(void)
{
}

#else

/* It's really sad to have two almost identical functions for this, but we
do it all for performance... :(
This version emulates 68000's prefetch "cache" */
static void m68k_run_1(void)
{
	struct regstruct* r = &regs;
	bool exit = false;

	while (!exit) {
	  TRY (prb) {
			while (!exit) {
				r->opcode = r->ir;

				r->instruction_pc = m68k_getpc();
				cpu_cycles = (*cpufunctbl[r->opcode])(r->opcode);
				cpu_cycles = adjust_cycles(cpu_cycles);
				do_cycles(cpu_cycles);
				if (r->spcflags) {
					if (do_specialties(cpu_cycles))
						exit = true;
				}
				regs.ipl = regs.ipl_pin;
				if (!currprefs.cpu_compatible || (currprefs.cpu_cycle_exact && currprefs.cpu_model <= 68010))
					exit = true;
			}
	  } CATCH (prb) {
			bus_error();
			if (r->spcflags) {
				if (do_specialties(cpu_cycles))
					exit = true;
			}
			regs.ipl = regs.ipl_pin;
	  } ENDTRY
	}
}

#endif /* CPUEMU_11 */

#ifndef CPUEMU_13

static void m68k_run_1_ce(void)
{
}

#else

/* cycle-exact m68k_run () */

static void m68k_run_1_ce(void)
{
	struct regstruct* r = &regs;
	bool first = true;
	bool exit = false;

	while (!exit) {
		TRY(prb) {
			if (first) {
				if (cpu_tracer < 0) {
					memcpy(&r->regs, &cputrace.regs, 16 * sizeof(uae_u32));
					r->ir = cputrace.ir;
					r->irc = cputrace.irc;
					r->sr = cputrace.sr;
					r->usp = cputrace.usp;
					r->isp = cputrace.isp;
					r->intmask = cputrace.intmask;
					r->stopped = cputrace.stopped;
					r->read_buffer = cputrace.read_buffer;
					r->write_buffer = cputrace.write_buffer;
					m68k_setpc(cputrace.pc);
					if (!r->stopped) {
						if (cputrace.state > 1) {
							write_log(_T("CPU TRACE: EXCEPTION %d\n"), cputrace.state);
							Exception(cputrace.state);
						} else if (cputrace.state == 1) {
							write_log(_T("CPU TRACE: %04X\n"), cputrace.opcode);
							(*cpufunctbl[cputrace.opcode])(cputrace.opcode);
						}
					} else {
						write_log(_T("CPU TRACE: STOPPED\n"));
					}
					if (r->stopped)
						set_special(SPCFLAG_STOP);
					set_cpu_tracer(false);
					goto cont;
				}
				set_cpu_tracer(false);
				first = false;
			}

			while (!exit) {
				r->opcode = r->ir;

				if (cpu_tracer) {
					memcpy(&cputrace.regs, &r->regs, 16 * sizeof(uae_u32));
					cputrace.opcode = r->opcode;
					cputrace.ir = r->ir;
					cputrace.irc = r->irc;
					cputrace.sr = r->sr;
					cputrace.usp = r->usp;
					cputrace.isp = r->isp;
					cputrace.intmask = r->intmask;
					cputrace.stopped = r->stopped;
					cputrace.read_buffer = r->read_buffer;
					cputrace.write_buffer = r->write_buffer;
					cputrace.state = 1;
					cputrace.pc = m68k_getpc();
					cputrace.startcycles = get_cycles();
					cputrace.memoryoffset = 0;
					cputrace.cyclecounter = cputrace.cyclecounter_pre = cputrace.cyclecounter_post = 0;
					cputrace.readcounter = cputrace.writecounter = 0;
				}

				r->instruction_pc = m68k_getpc();
				(*cpufunctbl[r->opcode])(r->opcode);
				if (cpu_tracer) {
					cputrace.state = 0;
				}
			cont:
				if (cputrace.needendcycles) {
					cputrace.needendcycles = 0;
					write_log(_T("STARTCYCLES=%08x ENDCYCLES=%08lx\n"), cputrace.startcycles, get_cycles());
				}

				if (r->spcflags) {
					if (do_specialties(0))
						exit = true;
				}

				if (!currprefs.cpu_cycle_exact || currprefs.cpu_model > 68010)
					exit = true;
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

#endif

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

static uae_u32 get_jit_opcode(void)
{
	uae_u32 opcode;
	opcode = get_diword(0);
	return opcode;
}

void exec_nostats(void)
{
	struct regstruct* r = &regs;

	for (;;)
	{
		r->opcode = get_jit_opcode();
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
	for (;;) {
		/* Take note: This is the do-it-normal loop */
		r->opcode = get_jit_opcode();

		special_mem = DISTRUST_CONSISTENT_MEM;
		pc_hist[blocklen].location = (uae_u16*)r->pc_p;

		cpu_cycles = (*cpufunctbl[r->opcode])(r->opcode);
		cpu_cycles = adjust_cycles(cpu_cycles);
		do_cycles(cpu_cycles);
		total_cycles += cpu_cycles;

		pc_hist[blocklen].specmem = special_mem;
		blocklen++;
		if (end_block(r->opcode) || blocklen >= MAXRUN || r->spcflags || uae_int_requested) {
			compile_block(pc_hist, blocklen, total_cycles);
			return; /* We will deal with the spcflags in the caller */
		}
		/* No need to check regs.spcflags, because if they were set,
we'd have ended up inside that "if" */
	}
}

typedef void compiled_handler(void);

static void m68k_run_jit (void)
	{
		for (;;) {
			((compiled_handler*)(pushall_call_handler))();
			/* Whenever we return from that, we should check spcflags */
		check_uae_int_request();
  	if (regs.spcflags) {
	    if (do_specialties (0)) {
				return;
			}
		}
		// If T0, T1 or M got set: run normal emulation loop
		if (regs.t0 || regs.t1 || regs.m) {
			flush_icache(3);
			struct regstruct *r = &regs;
			bool exit = false;
			while (!exit && (regs.t0 || regs.t1 || regs.m)) {
				r->instruction_pc = m68k_getpc();
				r->opcode = x_get_iword(0);
				(*cpufunctbl[r->opcode])(r->opcode);
				do_cycles(4 * CYCLE_UNIT);
				if (r->spcflags) {
					if (do_specialties(cpu_cycles))
						exit = true;
				}
			}
			unset_special(SPCFLAG_END_COMPILE);
		}
	}
}
#endif /* JIT */

void cpu_halt(int id)
{
	// id > 0: emulation halted.
	if (!regs.halted) {
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
static void m68k_run_2_000(void)
{
	struct regstruct* r = &regs;
	bool exit = false;

	while (!exit) {
	  TRY(prb) {
			while (!exit) {
				r->instruction_pc = m68k_getpc();

				r->opcode = get_diword(0);

				cpu_cycles = (*cpufunctbl[r->opcode])(r->opcode);
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

static void m68k_run_2_020(void)
			{
	struct regstruct *r = &regs;
	bool exit = false;

	while (!exit) {
	  TRY(prb) {
			while (!exit) {
		    r->instruction_pc = m68k_getpc ();
  
				r->opcode = get_diword(0);

	      cpu_cycles = (*cpufunctbl[r->opcode])(r->opcode);
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

#ifdef WITH_THREADED_CPU
	init_cpu_thread();
#endif
	if (in_m68k_go || !may_quit) {
		write_log(_T("Bug! m68k_go is not reentrant.\n"));
		abort();
	}

#ifdef USE_JIT_FPU
#ifdef CPU_AARCH64
	save_host_fp_regs(fp_buffer);
#elif defined (CPU_arm)
	// This caused crashes in RockChip 32-bit platforms unless it was inlined like this
	__asm__ volatile ("vstmia %[fp_buffer]!, {d7-d15}"::[fp_buffer] "r" (fp_buffer));
#endif
#endif

	reset_frame_rate_hack();
	update_68k_cycles();
	start_cycles = 0;

	set_cpu_tracer (false);

	cpu_prefs_changed_flag = 0;
	in_m68k_go++;
  for (;;) {
		int restored = 0;
		void (*run_func)(void);

		cputrace.state = -1;

  	if (quit_program > 0) {
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
			set_cycles (start_cycles);
			custom_reset(cpu_hardreset != 0, cpu_keyboardreset);
			m68k_reset(cpu_hardreset != 0);
	    if (cpu_hardreset) {
				memory_clear();
				write_log(_T("hardreset, memory cleared\n"));
			}
			cpu_hardreset = false;
#ifdef SAVESTATE
			/* We may have been restoring state, but we're done now.  */
	    if (isrestore ()) {
		    restored = savestate_restore_finish ();
				startup = 1;
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

		set_cpu_tracer (false);

		if (regs.spcflags & SPCFLAG_MODE_CHANGE) {
			if (cpu_prefs_changed_flag & 1) {
				uaecptr pc = m68k_getpc();
				prefs_changed_cpu();
				custom_cpuchange();
				build_cpufunctbl();
				m68k_setpc_normal(pc);
				fill_prefetch();
				update_68k_cycles();
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
	  if (startup) {
			custom_prepare();
			protect_roms(true);
		}
		startup = 0;
		event_wait = true;
		unset_special(SPCFLAG_MODE_CHANGE);

#ifdef SAVESTATE
		if (restored) {
			restored = 0;
			savestate_restore_final();
		}
#endif

		if (!regs.halted) {
			// check that PC points to something that looks like memory.
			uaecptr pc = m68k_getpc();
			addrbank* ab = &get_mem_bank(pc);
			if (ab == NULL || ab == &dummy_bank || (!currprefs.cpu_compatible && !valid_address(pc, 2)) || (pc & 1)) {
				cpu_halt(CPU_HALT_INVALID_START_ADDRESS);
			}
		}
		if (regs.halted) {
			cpu_halt(regs.halted);
		}

		run_func = currprefs.cpu_cycle_exact && currprefs.cpu_model <= 68010 ? m68k_run_1_ce :
      currprefs.cpu_compatible && currprefs.cpu_model <= 68010 ? m68k_run_1 :
#ifdef JIT
      currprefs.cpu_model >= 68020 && currprefs.cachesize ? m68k_run_jit :
#endif
			currprefs.cpu_model < 68020 ? m68k_run_2_000 : m68k_run_2_020;
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
#elif defined (CPU_arm)
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
	uae_u32 flags, model;
	uae_u32 l;

	currprefs.cpu_model = changed_prefs.cpu_model = model = restore_u32();
	flags = restore_u32();
	changed_prefs.address_space_24 = false;
	if (flags & CPUTYPE_EC)
		changed_prefs.address_space_24 = true;
	currprefs.address_space_24 = changed_prefs.address_space_24;
	currprefs.cpu_compatible = changed_prefs.cpu_compatible;
	currprefs.cpu_cycle_exact = changed_prefs.cpu_cycle_exact;
	currprefs.cpu_memory_cycle_exact = changed_prefs.cpu_memory_cycle_exact;
	currprefs.blitter_cycle_exact = changed_prefs.blitter_cycle_exact;
	for (int i = 0; i < 15; i++)
		regs.regs[i] = restore_u32();
	regs.pc = restore_u32();
	regs.irc = restore_u16();
	regs.ir = restore_u16();
	regs.usp = restore_u32();
	regs.isp = restore_u32();
	regs.sr = restore_u16();
	l = restore_u32();
	if (l & CPUMODE_HALT) {
		regs.stopped = 1;
	}
	else {
		regs.stopped = 0;
	}
	if (model >= 68010) {
		regs.dfc = restore_u32();
		regs.sfc = restore_u32();
		regs.vbr = restore_u32();
	}
	if (model >= 68020) {
		regs.caar = restore_u32();
		regs.cacr = restore_u32();
		regs.msp = restore_u32();
	}
	if (model >= 68030) {
		fake_crp_030 = restore_u64();
		fake_srp_030 = restore_u64();
		fake_tt0_030 = restore_u32();
		fake_tt1_030 = restore_u32();
		fake_tc_030 = restore_u32();
		fake_mmusr_030 = restore_u16();
	}
	if (model >= 68040) {
		regs.itt0 = restore_u32();
		regs.itt1 = restore_u32();
		regs.dtt0 = restore_u32();
		regs.dtt1 = restore_u32();
		regs.tcr = restore_u32();
		regs.urp = restore_u32();
		regs.srp = restore_u32();
	}
	if (flags & 0x80000000) {
		int khz = restore_u32();
		if (khz < 0)
			currprefs.m68k_speed = changed_prefs.m68k_speed = -1;
		else if (khz > 0 && khz < 800000)
			currprefs.m68k_speed = changed_prefs.m68k_speed = 0;
	}
	set_cpu_caches(true);
	if (flags & 0x10000000) {
		regs.chipset_latch_rw = restore_u32();
	}

	if (flags & 0x2000000 && currprefs.cpu_model <= 68010) {
		regs.read_buffer = restore_u16();
		regs.write_buffer = restore_u16();
	}

	m68k_reset_sr();

	write_log(_T("CPU: %d%s%03d, PC=%08X\n"),
		model / 1000, flags & 1 ? _T("EC") : _T(""), model % 1000, regs.pc);

	return src;
}

static void fill_prefetch_quick(void)
{
	if (currprefs.cpu_model >= 68020) {
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
	set_cycles (start_cycles);
	events_schedule();
	if (regs.stopped)
		set_special(SPCFLAG_STOP);
}

uae_u8 *save_cpu_trace (int *len, uae_u8 *dstptr)
{
	uae_u8 *dstbak, *dst;

	if (cputrace.state <= 0)
		return NULL;

	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc (uae_u8, 10000);

	save_u32 (2 | 4 | 16 | 32 | 64);
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
	write_log (_T("CPUT SAVE: PC=%08x C=%08X %08x %08x %08x %d %d %d\n"),
		cputrace.pc, cputrace.startcycles,
		cputrace.cyclecounter, cputrace.cyclecounter_pre, cputrace.cyclecounter_post,
		cputrace.readcounter, cputrace.writecounter, cputrace.memoryoffset);
	for (int i = 0; i < cputrace.memoryoffset; i++) {
		save_u32 (cputrace.ctm[i].addr);
		save_u32 (cputrace.ctm[i].data);
		save_u32 (cputrace.ctm[i].mode);
		write_log (_T("CPUT%d: %08x %08x %08x\n"), i, cputrace.ctm[i].addr, cputrace.ctm[i].data, cputrace.ctm[i].mode);
	}
	save_u32 (cputrace.startcycles);

	save_u16(cputrace.read_buffer);
	save_u16(cputrace.writecounter);

	*len = dst - dstbak;
	cputrace.needendcycles = 1;
	return dstbak;
}

uae_u8 *restore_cpu_trace (uae_u8 *src)
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
	cputrace.startcycles = restore_u32 ();

	if (v & 4) {
		if (currprefs.cpu_model == 68020) {
			if (v & 64) {
				cputrace.read_buffer = restore_u16();
				cputrace.write_buffer = restore_u16();
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

uae_u8* restore_cpu_extra(uae_u8* src)
{
	uae_u32 flags = restore_u32();

	currprefs.cpu_cycle_exact = changed_prefs.cpu_cycle_exact = (flags & 1) ? true : false;
	currprefs.cpu_memory_cycle_exact = changed_prefs.cpu_memory_cycle_exact = currprefs.cpu_cycle_exact;
	if ((flags & 32) && !(flags & 1))
		currprefs.cpu_memory_cycle_exact = changed_prefs.cpu_memory_cycle_exact = true;
	currprefs.blitter_cycle_exact = changed_prefs.blitter_cycle_exact = currprefs.cpu_cycle_exact;
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
	flags = 0;
	flags |= currprefs.cpu_cycle_exact ? 1 : 0;
	flags |= currprefs.cpu_compatible ? 2 : 0;
	flags |= currprefs.m68k_speed < 0 ? 4 : 0;
	flags |= currprefs.cachesize > 0 ? 8 : 0;
	flags |= currprefs.m68k_speed > 0 ? 16 : 0;
	flags |= currprefs.cpu_memory_cycle_exact ? 32 : 0;
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
	uae_u32 flags;

	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc(uae_u8, 1000);
	model = currprefs.cpu_model;
	save_u32(model); /* MODEL */
	flags = 0x80000000 | 0x40000000 | 0x20000000 | 0x10000000 | 0x8000000 | 0x4000000 | (currprefs.cpu_model <= 68010 ? 0x2000000 : 0) | (currprefs.address_space_24 ? 1 : 0);
	save_u32(flags); /* FLAGS */
	for (int i = 0; i < 15; i++)
		save_u32(regs.regs[i]);		/* D0-D7 A0-A6 */
	save_u32(m68k_getpc ()); /* PC */
	save_u16(regs.irc); /* prefetch */
	save_u16(regs.ir); /* instruction prefetch */
	MakeSR();
	save_u32(!regs.s ? regs.regs[15] : regs.usp); /* USP */
	save_u32(regs.s ? regs.regs[15] : regs.isp); /* ISP */
	save_u16(regs.sr); /* SR/CCR */
	save_u32(regs.stopped ? CPUMODE_HALT : 0); /* flags */
	if (model >= 68010) {
		save_u32(regs.dfc); /* DFC */
		save_u32(regs.sfc); /* SFC */
		save_u32(regs.vbr); /* VBR */
	}
	if (model >= 68020) {
		save_u32(regs.caar); /* CAAR */
		save_u32(regs.cacr); /* CACR */
		save_u32(regs.msp); /* MSP */
	}
	if (model >= 68030) {
		save_u64(fake_crp_030); /* CRP */
		save_u64(fake_srp_030); /* SRP */
		save_u32(fake_tt0_030); /* TT0/AC0 */
		save_u32(fake_tt1_030); /* TT1/AC1 */
		save_u32(fake_tc_030); /* TCR */
		save_u16(fake_mmusr_030); /* MMUSR/ACUSR */
	}
	if (model >= 68040) {
		save_u32(regs.itt0); /* ITT0 */
		save_u32(regs.itt1); /* ITT1 */
		save_u32(regs.dtt0); /* DTT0 */
		save_u32(regs.dtt1); /* DTT1 */
		save_u32(regs.tcr); /* TCR */
		save_u32(regs.urp); /* URP */
		save_u32(regs.srp); /* SRP */
	}
	khz = -1;
	if (currprefs.m68k_speed == 0) {
		khz = currprefs.ntscmode ? 715909 : 709379;
		if (currprefs.cpu_model >= 68020)
			khz *= 2;
	}
	save_u32(khz); // clock rate in KHz: -1 = fastest possible 
	save_u32(regs.chipset_latch_rw);
	if (currprefs.cpu_model <= 68010) {
		save_u16(regs.read_buffer);
		save_u16(regs.write_buffer);
	}
	*len = dst - dstbak;
	return dstbak;
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
	Exception(3);
}

static void exception3_notinstruction(uae_u32 opcode, uaecptr addr)
{
	last_di_for_exception_3 = 1;
	exception3f (opcode, addr, true, false, true, 0xffffffff, 1, -1, 0);
}

// 68010 special prefetch handling
void exception3_read_prefetch_only(uae_u32 opcode, uae_u32 addr)
{
	if (currprefs.cpu_model == 68010) {
		uae_u16 prev = regs.read_buffer;
		x_get_word(addr & ~1);
		regs.irc = regs.read_buffer;
	} else {
		x_do_cycles(4 * CYCLE_UNIT / 2);
	}
	last_di_for_exception_3 = 0;
	exception3f(opcode, addr, false, true, false, m68k_getpc(), sz_word, -1, 0);
}

// Some hardware accepts address error aborted reads or writes as normal reads/writes.
void exception3_read_prefetch(uae_u32 opcode, uaecptr addr)
{
	x_do_cycles(4 * CYCLE_UNIT / 2);
	last_di_for_exception_3 = 0;
	if (currprefs.cpu_model == 68000) {
		m68k_incpci(2);
	}
	exception3f(opcode, addr, false, true, false, m68k_getpc(), sz_word, -1, 0);
}
void exception3_read_prefetch_68040bug(uae_u32 opcode, uaecptr addr, uae_u16 secondarysr)
{
	x_do_cycles(4 * CYCLE_UNIT / 2);
	last_di_for_exception_3 = 0;
	exception3f(opcode | 0x10000, addr, false, true, false, m68k_getpc(), sz_word, -1, secondarysr);
}

void exception3_read_access(uae_u32 opcode, uaecptr addr, int size, int fc)
{
	x_do_cycles(4 * CYCLE_UNIT / 2);
	exception3_read(opcode, addr, size, fc);
}
void exception3_read_access2(uae_u32 opcode, uaecptr addr, int size, int fc)
{
	// (An), -(An) and (An)+ and 68010: read happens twice!
	x_do_cycles(8 * CYCLE_UNIT / 2);
	exception3_read(opcode, addr, size, fc);
}
void exception3_write_access(uae_u32 opcode, uaecptr addr, int size, uae_u32 val, int fc)
{
	x_do_cycles(4 * CYCLE_UNIT / 2);
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
	if (currprefs.cpu_compatible && HARDWARE_BUS_ERROR_EMULATION) {
		hardware_bus_error = 1;
	} else {
	  int fc = (regs.s ? 4 : 0) | (ins ? 2 : 1);
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
	last_writeaccess_for_exception_3 = 0;
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


void cpureset(void)
{
	/* RESET hasn't increased PC yet, 1 word offset */
	uaecptr pc;
	uaecptr ksboot = 0xf80002 - 2;
	uae_u16 ins;
	addrbank* ab;

	maybe_disable_fpu();
	m68k_reset_delay = currprefs.reset_delay;
	set_special(SPCFLAG_CHECK);
	send_internalevent(INTERNALEVENT_CPURESET);
  if ((currprefs.cpu_compatible || currprefs.cpu_memory_cycle_exact) && currprefs.cpu_model <= 68020) {
		custom_reset(false, false);
		return;
	}
	pc = m68k_getpc() + 2;
	ab = &get_mem_bank(pc);
	if (ab->check (pc, 2)) {
		write_log(_T("CPU reset PC=%x (%s)..\n"), pc - 2, ab->name);
		ins = get_word(pc);
		custom_reset(false, false);
		// did memory disappear under us?
		if (ab == &get_mem_bank(pc))
			return;
		// it did
    if ((ins & ~7) == 0x4ed0) {
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
	if ((regs.spcflags & SPCFLAG_DOTRACE) == 0) {
		m68k_set_stop();
	} else {
		m68k_resumestopped();
	}
}

void m68k_resumestopped (void)
{
	if (!regs.stopped)
		return;
	if (currprefs.cpu_cycle_exact && currprefs.cpu_model == 68000) {
		x_do_cycles (6 * CYCLE_UNIT / 2);
	}
	fill_prefetch();
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
		  x_do_cycles_post (4 * CYCLE_UNIT / 2, v);
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
		  x_do_cycles_post (4 * CYCLE_UNIT / 2, v);
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
		  x_do_cycles_post (4 * CYCLE_UNIT / 2, v);
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
		  x_do_cycles_post (4 * CYCLE_UNIT / 2, v);
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
		  x_do_cycles_post (4 * CYCLE_UNIT / 2, v);
		  return;
	}
	put_word (addr, v);
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
	regs.read_buffer = regs.irc;
}
