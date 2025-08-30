/*
* UAE - The Un*x Amiga Emulator
*
* Debugger
*
* (c) 1995 Bernd Schmidt
* (c) 2006 Toni Wilen
*
*/

#include "sysconfig.h"
#include "sysdeps.h"

#include <ctype.h>
#include <signal.h>

#include "options.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "debug.h"
#include "disasm.h"
#include "debugmem.h"
#include "cia.h"
#include "xwin.h"
#include "identify.h"
#include "sounddep/sound.h"
#include "disk.h"
#include "savestate.h"
#include "autoconf.h"
#include "akiko.h"
#include "inputdevice.h"
#include "crc32.h"
#include "cpummu.h"
#include "rommgr.h"
#include "inputrecord.h"
#include "calc.h"
#include "cpummu.h"
#include "cpummu030.h"
#include "ar.h"
#ifdef WITH_PCI
#include "pci.h"
#endif
#ifdef WITH_PPC
#include "ppc/ppcd.h"
#include "uae/io.h"
#include "uae/ppc.h"
#endif
#include "drawing.h"
#include "devices.h"
#include "blitter.h"
#include "ini.h"
#include "readcpu.h"
#include "keybuf.h"

static int trace_mode;
static uae_u32 trace_param[3];

int debugger_active;
static int debug_rewind;
static int memwatch_triggered;
static int inside_debugger;
int debugger_used;
int memwatch_access_validator;
int memwatch_enabled;
int debugging;
int exception_debugging;
int no_trace_exceptions;
int debug_copper = 0;
int debug_dma = 0, debug_heatmap = 0;
static uae_u32 debug_sprite_mask_val = 0xff;
uae_u32 debug_sprite_mask = 0xffffffff;
int debug_illegal = 0;
uae_u64 debug_illegal_mask;
static int debug_mmu_mode;
static bool break_if_enforcer;
static uaecptr debug_pc;

static int trace_cycles;
static int last_hpos1, last_hpos2;
static int last_vpos1, last_vpos2;
static int last_frame = -1;
static evt_t last_cycles1, last_cycles2;

static uaecptr processptr;
static uae_char *processname;

static uaecptr debug_copper_pc;

extern int audio_channel_mask;
extern int inputdevice_logging;

static void debug_cycles(int mode)
{
	trace_cycles = mode;
	last_cycles2 = get_cycles();
	last_vpos2 = vpos;
	last_hpos2 = current_hpos();
}

void deactivate_debugger (void)
{
	inside_debugger = 0;
	debugger_active = 0;
	debugging = 0;
	exception_debugging = 0;
	processptr = 0;
	xfree (processname);
	processname = NULL;
	debugmem_enable();
	debug_pc = 0xffffffff;
	keybuf_ignore_next_release();
	deactivate_console();
}

void activate_debugger (void)
{
	disasm_init();
	if (isfullscreen() > 0)
		return;

	debugger_load_libraries();
	open_console();

	debugger_used = 1;
	inside_debugger = 1;
	debug_pc = 0xffffffff;
	trace_mode = 0;
	if (debugger_active) {
		// already in debugger but some break point triggered
		// during disassembly etc..
		return;
	}
	debug_cycles(1);
	debugger_active = 1;
	set_special (SPCFLAG_BRK);
	debugging = 1;
	mmu_triggered = 0;
	debugmem_disable();
}

void activate_debugger_new(void)
{
	activate_debugger();
	debug_pc = M68K_GETPC;
}

void activate_debugger_new_pc(uaecptr pc, int len)
{
	activate_debugger();
	trace_mode = TRACE_RANGE_PC;
	trace_param[0] = pc;
	trace_param[1] = pc + len;
}

static void debug_continue(void)
{
	set_special(SPCFLAG_BRK);
}

bool debug_enforcer(void)
{
	if (!break_if_enforcer)
		return false;
	activate_debugger();
	return true;
}


int firsthist = 0;
int lasthist = 0;
struct cpuhistory {
	struct regstruct regs;
	int fp, vpos, hpos;
};
static struct cpuhistory history[MAX_HIST];

static const TCHAR help[] = {
	_T("          HELP for UAE Debugger\n")
	_T("         -----------------------\n\n")
	_T("  g [<address>]         Start execution at the current address or <address>.\n")
	_T("  c                     Dump state of the CIA, disk drives and custom registers.\n")
	_T("  r                     Dump state of the CPU.\n")
	_T("  r <reg> <value>       Modify CPU registers (Dx,Ax,USP,ISP,VBR,...).\n")
	_T("  rc[d]                 Show CPU instruction or data cache contents.\n")
	_T("  m <address> [<lines>] Memory dump starting at <address>.\n")
	_T("  a <address>           Assembler.\n")
	_T("  d <address> [<lines>] Disassembly starting at <address>.\n")
	_T("  t [instructions]      Step one or more instructions.\n")
	_T("  tx                    Break when any exception.\n")
	_T("  z                     Step through one instruction - useful for JSR, DBRA etc.\n")
	_T("  f                     Step forward until PC in RAM (\"boot block finder\").\n")
	_T("  f <address> [Nx]      Add/remove breakpoint.\n")
	_T("  fa <address> [<start>] [<end>]\n")
	_T("                        Find effective address <address>.\n")
	_T("  fi                    Step forward until PC points to RTS, RTD or RTE.\n")
	_T("  fi <opcode> [<w2>] [<w3>] Step forward until PC points to <opcode>.\n")
	_T("  fp \"<name>\"/<addr>    Step forward until process <name> or <addr> is active.\n")
	_T("  fl                    List breakpoints.\n")
	_T("  fd                    Remove all breakpoints.\n")
	_T("  fs <lines to wait> | <vpos> <hpos> Wait n scanlines/position.\n")
	_T("  fc <CCKs to wait>     Wait n color clocks.\n")
	_T("  fo <num> <reg> <oper> <val> [<mask> <val2>] Conditional register breakpoint [Nx] [Hx].\n")
	_T("   reg=Dx,Ax,PC,USP,ISP,VBR,SR. oper:!=,==,<,>,>=,<=,-,!- (-=val to val2 range).\n")
	_T("  f <addr1> <addr2>     Step forward until <addr1> <= PC <= <addr2>.\n")
	_T("  e[x]                  Dump contents of all custom registers, ea = AGA colors.\n")
	_T("  i [<addr>]            Dump contents of interrupt and trap vectors.\n")
	_T("  il [<mask>]           Exception breakpoint.\n")
	_T("  o <0-2|addr> [<lines>]View memory as Copper instructions.\n")
	_T("  od                    Enable/disable Copper vpos/hpos tracing.\n")
	_T("  ot                    Copper single step trace.\n")
	_T("  ob <addr>             Copper breakpoint.\n")
	_T("  H[H] <cnt>            Show PC history (HH=full CPU info) <cnt> instructions.\n")
	_T("  C <value>             Search for values like energy or lifes in games.\n")
	_T("  Cl                    List currently found trainer addresses.\n")
	_T("  D[idxzs <[max diff]>] Deep trainer. i=new value must be larger, d=smaller,\n")
	_T("                        x = must be same, z = must be different, s = restart.\n")
	_T("  W <addr> <values[.x] separated by space> Write into Amiga memory.\n")
	_T("  W <addr> 'string'     Write into Amiga memory.\n")
	_T("  Wf <addr> <endaddr> <bytes or string like above>, fill memory.\n")
	_T("  Wc <addr> <endaddr> <destaddr>, copy memory.\n")
	_T("  w <num> <address> <length> <R/W/I> <F/C/L/N> [V<value>[.x]] (read/write/opcode) (freeze/mustchange/logonly/nobreak).\n")
	_T("                        Add/remove memory watchpoints.\n")
	_T("  wd [<0-1>]            Enable illegal access logger. 1 = enable break.\n")
	_T("  L <file> <addr> [<n>] Load a block of Amiga memory.\n")
	_T("  S <file> <addr> <n>   Save a block of Amiga memory.\n")
	_T("  s \"<string>\"/<values> [<addr>] [<length>]\n")
	_T("                        Search for string/bytes.\n")
	_T("  T or Tt               Show exec tasks and their PCs.\n")
	_T("  Td,Tl,Tr,Tp,Ts,TS,Ti,TO,TM,Tf Show devs, libs, resources, ports, semaphores,\n")
	_T("                        residents, interrupts, doslist, memorylist, fsres.\n")
	_T("  b                     Step to previous state capture position.\n")
	_T("  M<a/b/s> <val>        Enable or disable audio channels, bitplanes or sprites.\n")
	_T("  sp <addr> [<addr2][<size>] Dump sprite information.\n")
	_T("  di <mode> [<track>]   Break on disk access. R=DMA read,W=write,RW=both,P=PIO.\n")
	_T("                        Also enables level 1 disk logging.\n")
	_T("  did <log level>       Enable disk logging.\n")
	_T("  dj [<level bitmask>]  Enable joystick/mouse input debugging.\n")
	_T("  smc [<0-1>]           Enable self-modifying code detector. 1 = enable break.\n")
	_T("  dm                    Dump current address space map.\n")
	_T("  v <vpos> [<hpos>] [<lines>]\n")
	_T("                        Show DMA data (accurate only in cycle-exact mode).\n")
	_T("                        v [-1 to -4] = enable visual DMA debugger.\n")
	_T("  vh [<ratio> <lines>]  \"Heat map\"\n")
	_T("  I <custom event>      Send custom event string\n")
	_T("  ?<value>              Hex ($ and 0x)/Bin (%)/Dec (!) converter and calculator.\n")
#ifdef _WIN32
	_T("  x                     Close debugger.\n")
	_T("  xx                    Switch between console and GUI debugger.\n")
	_T("  mg <address>          Memory dump starting at <address> in GUI.\n")
	_T("  dg <address>          Disassembly starting at <address> in GUI.\n")
#endif
	_T("  q                     Quit the emulator. You don't want to use this command.\n\n")
};

void debug_help (void)
{
	console_out (help);
}


struct mw_acc {
	uae_u32 mask;
	const TCHAR *name;
};

static const struct mw_acc memwatch_access_masks[] =
{
	{ MW_MASK_ALL, _T("ALL") },
	{ MW_MASK_NONE, _T("NONE") },
	{ MW_MASK_ALL & ~(MW_MASK_CPU_I | MW_MASK_CPU_D_R | MW_MASK_CPU_D_W), _T("DMA") },

	{ MW_MASK_BLITTER_A | MW_MASK_BLITTER_B | MW_MASK_BLITTER_C | MW_MASK_BLITTER_D_N | MW_MASK_BLITTER_D_L | MW_MASK_BLITTER_D_F, _T("BLT") },
	{ MW_MASK_BLITTER_D_N | MW_MASK_BLITTER_D_L | MW_MASK_BLITTER_D_F, _T("BLTD") },

	{ MW_MASK_AUDIO_0 | MW_MASK_AUDIO_1 | MW_MASK_AUDIO_2 | MW_MASK_AUDIO_3, _T("AUD") },

	{ MW_MASK_BPL_0 | MW_MASK_BPL_1 | MW_MASK_BPL_2 | MW_MASK_BPL_3 |
		MW_MASK_BPL_4 | MW_MASK_BPL_5 | MW_MASK_BPL_6 | MW_MASK_BPL_7, _T("BPL") },

	{ MW_MASK_SPR_0 | MW_MASK_SPR_1 | MW_MASK_SPR_2 | MW_MASK_SPR_3 |
		MW_MASK_SPR_4 | MW_MASK_SPR_5 | MW_MASK_SPR_6 | MW_MASK_SPR_7, _T("SPR") },

	{ MW_MASK_CPU_I | MW_MASK_CPU_D_R | MW_MASK_CPU_D_W, _T("CPU") },
	{ MW_MASK_CPU_D_R | MW_MASK_CPU_D_W, _T("CPUD") },
	{ MW_MASK_CPU_I, _T("CPUI") },
	{ MW_MASK_CPU_D_R, _T("CPUDR") },
	{ MW_MASK_CPU_D_W, _T("CPUDW") },

	{ MW_MASK_COPPER, _T("COP") },

	{ MW_MASK_BLITTER_A, _T("BLTA") },
	{ MW_MASK_BLITTER_B, _T("BLTB") },
	{ MW_MASK_BLITTER_C, _T("BLTC") },
	{ MW_MASK_BLITTER_D_N, _T("BLTDN") },
	{ MW_MASK_BLITTER_D_L, _T("BLTDL") },
	{ MW_MASK_BLITTER_D_F, _T("BLTDF") },

	{ MW_MASK_DISK, _T("DSK") },

	{ MW_MASK_AUDIO_0, _T("AUD0") },
	{ MW_MASK_AUDIO_1, _T("AUD1") },
	{ MW_MASK_AUDIO_2, _T("AUD2") },
	{ MW_MASK_AUDIO_3, _T("AUD3") },

	{ MW_MASK_BPL_0, _T("BPL0") },
	{ MW_MASK_BPL_1, _T("BPL1") },
	{ MW_MASK_BPL_2, _T("BPL2") },
	{ MW_MASK_BPL_3, _T("BPL3") },
	{ MW_MASK_BPL_4, _T("BPL4") },
	{ MW_MASK_BPL_5, _T("BPL5") },
	{ MW_MASK_BPL_6, _T("BPL6") },
	{ MW_MASK_BPL_7, _T("BPL7") },

	{ MW_MASK_SPR_0, _T("SPR0") },
	{ MW_MASK_SPR_1, _T("SPR1") },
	{ MW_MASK_SPR_2, _T("SPR2") },
	{ MW_MASK_SPR_3, _T("SPR3") },
	{ MW_MASK_SPR_4, _T("SPR4") },
	{ MW_MASK_SPR_5, _T("SPR5") },
	{ MW_MASK_SPR_6, _T("SPR6") },
	{ MW_MASK_SPR_7, _T("SPR7") },

	{ 0, NULL },
};

static void mw_help(void)
{
	for (int i = 0; memwatch_access_masks[i].mask; i++) {
		console_out_f(_T("%s "), memwatch_access_masks[i].name);
	}
	console_out_f(_T("\n"));
}

static int debug_linecounter;
#define MAX_LINECOUNTER 1000

static int debug_out (const TCHAR *format, ...)
{
	va_list parms;
	TCHAR buffer[4000];

	va_start (parms, format);
	_vsntprintf (buffer, 4000 - 1, format, parms);
	va_end (parms);

	console_out (buffer);
	if (debug_linecounter < MAX_LINECOUNTER)
		debug_linecounter++;
	if (debug_linecounter >= MAX_LINECOUNTER)
		return 0;
	return 1;
}

uae_u32 get_byte_debug (uaecptr addr)
{
	uae_u32 v = 0xff;
	if (debug_mmu_mode) {
		flagtype olds = regs.s;
		regs.s = (debug_mmu_mode & 4) != 0;
		TRY(p) {
			if (currprefs.mmu_model == 68030) {
				v = mmu030_get_generic (addr, debug_mmu_mode, sz_byte, MMU030_SSW_SIZE_B);
			} else {
				if (debug_mmu_mode & 1) {
					bool odd = (addr & 1) != 0;
					addr &= ~1;
					v = mmu_get_iword(addr, sz_byte);
					if (!odd)
						v >>= 8;
					else
						v &= 0xff;
				} else {
					v = mmu_get_user_byte (addr, regs.s != 0, false, sz_byte, false);
				}
			}
		} CATCH(p) {
		} ENDTRY
		regs.s = olds;
	} else {
		v = get_byte (addr);
	}
	return v;
}
uae_u32 get_word_debug (uaecptr addr)
{
	uae_u32 v = 0xffff;
	if (debug_mmu_mode) {
		flagtype olds = regs.s;
		regs.s = (debug_mmu_mode & 4) != 0;
		TRY(p) {
			if (currprefs.mmu_model == 68030) {
				v = mmu030_get_generic (addr, debug_mmu_mode, sz_word, MMU030_SSW_SIZE_W);
			} else {
				if (debug_mmu_mode & 1) {
					v = mmu_get_iword(addr, sz_word);
				} else {
					v = mmu_get_user_word (addr, regs.s != 0, false, sz_word, false);
				}
			}
		} CATCH(p) {
		} ENDTRY
		regs.s = olds;
	} else {
		v = get_word (addr);
	}
	return v;
}
uae_u32 get_long_debug (uaecptr addr)
{
	uae_u32 v = 0xffffffff;
	if (debug_mmu_mode) {
		flagtype olds = regs.s;
		regs.s = (debug_mmu_mode & 4) != 0;
		TRY(p) {
			if (currprefs.mmu_model == 68030) {
				v = mmu030_get_generic (addr, debug_mmu_mode, sz_long, MMU030_SSW_SIZE_L);
			} else {
				if (debug_mmu_mode & 1) {
					v = mmu_get_ilong(addr, sz_long);
				} else {
					v = mmu_get_user_long (addr, regs.s != 0, false, sz_long, false);
				}
			}
		} CATCH(p) {
		} ENDTRY
		regs.s = olds;
	} else {
		v = get_long (addr);
	}
	return v;
}
uae_u32 get_iword_debug (uaecptr addr)
{
	if (debug_mmu_mode) {
		return get_word_debug (addr);
	} else {
		if (valid_address (addr, 2))
			return get_word (addr);
		return 0xffff;
	}
}
uae_u32 get_ilong_debug (uaecptr addr)
{
	if (debug_mmu_mode) {
		return get_long_debug (addr);
	} else {
		if (valid_address (addr, 4))
			return get_long (addr);
		return 0xffffffff;
	}
}

static uae_u8 *get_real_address_debug(uaecptr addr)
{
	if (debug_mmu_mode) {
		flagtype olds = regs.s;
		TRY(p) {
			if (currprefs.mmu_model >= 68040)
				addr = mmu_translate(addr, 0, regs.s != 0, (debug_mmu_mode & 1), false, 0);
			else
				addr = mmu030_translate(addr, regs.s != 0, (debug_mmu_mode & 1), false);
		} CATCH(p) {
		} ENDTRY
	}
	return get_real_address(addr);
}

int debug_safe_addr (uaecptr addr, int size)
{
	if (debug_mmu_mode) {
		flagtype olds = regs.s;
		regs.s = (debug_mmu_mode & 4) != 0;
		TRY(p) {
			if (currprefs.mmu_model >= 68040)
				addr = mmu_translate (addr, 0, regs.s != 0, (debug_mmu_mode & 1), false, size);
			else
				addr = mmu030_translate (addr, regs.s != 0, (debug_mmu_mode & 1), false);
		} CATCH(p) {
			STOPTRY;
			return 0;
		} ENDTRY
		regs.s = olds;
	}
	addrbank *ab = &get_mem_bank (addr);
	if (!ab)
		return 0;
	if (ab->flags & ABFLAG_SAFE)
		return 1;
	if (!ab->check (addr, size))
		return 0;
	if (ab->flags & (ABFLAG_RAM | ABFLAG_ROM | ABFLAG_ROMIN | ABFLAG_SAFE))
		return 1;
	return 0;
}

static bool iscancel (int counter)
{
	static int cnt;

	cnt++;
	if (cnt < counter)
		return false;
	cnt = 0;
	if (!console_isch ())
		return false;
	console_getch ();
	return true;
}

static bool isoperator(TCHAR **cp)
{
	TCHAR c = _totupper(**cp);
	TCHAR c1 = _totupper((*cp)[1]);
	return c == '+' || c == '-' || c == '/' || c == '*' || c == '(' || c == ')' ||
		c == '|' || c == '&' || c == '^' || c == '=' || c == '>' || c == '<' ||
		(c == 'R' && (c1 == 'L' || c1 == 'W' || c1 == 'B'));
}

static void ignore_ws (TCHAR **c)
{
	while (**c && _istspace(**c))
		(*c)++;
}
static TCHAR peekchar (TCHAR **c)
{
	return **c;
}
static TCHAR readchar (TCHAR **c)
{
	TCHAR cc = **c;
	(*c)++;
	return cc;
}
static TCHAR next_char(TCHAR **c)
{
	ignore_ws (c);
	return *(*c)++;
}
static TCHAR next_char2(TCHAR **c)
{
	return *(*c)++;
}
static TCHAR peek_next_char (TCHAR **c)
{
	TCHAR *pc = *c;
	return pc[1];
}
static int more_params(TCHAR **c)
{
	ignore_ws(c);
	return (**c) != 0;
}
static int more_params2(TCHAR **c)
{
	return (**c) != 0;
}

static uae_u32 readint(TCHAR **c, bool *err);
static uae_u32 readbin(TCHAR **c, bool *err);
static uae_u32 readhex(TCHAR **c, bool *err);

static const TCHAR *debugoper[] = {
	_T("=="),
	_T("!="),
	_T("<="),
	_T(">="),
	_T("<"),
	_T(">"),
	_T("-"),
	_T("!-"),
	NULL
};

static int getoperidx(TCHAR **c, bool *opersigned)
{
	int i;
	TCHAR *p = *c;
	TCHAR tmp[10];
	int extra = 0;

	i = 0;
	while (p[i]) {
		tmp[i] = _totupper(p[i]);
		if (i >= sizeof(tmp) / sizeof(TCHAR) - 1)
			break;
		i++;
	}
	tmp[i] = 0;
	if (!_tcsncmp(tmp, _T("!="), 2)) {
		(*c) += 2;
		return BREAKPOINT_CMP_NEQUAL;
	} else if (!_tcsncmp(tmp, _T("=="), 2)) {
		(*c) += 2;
		return BREAKPOINT_CMP_EQUAL;
	} else if (!_tcsncmp(tmp, _T(">="), 2)) {
		(*c) += 2;
		return BREAKPOINT_CMP_LARGER_EQUAL;
	} else if (!_tcsncmp(tmp, _T("<="), 2)) {
		(*c) += 2;
		return BREAKPOINT_CMP_SMALLER_EQUAL;
	} else if (!_tcsncmp(tmp, _T(">"), 1)) {
		(*c) += 1;
		return BREAKPOINT_CMP_LARGER;
	} else if (!_tcsncmp(tmp, _T("<"), 1)) {
		(*c) += 1;
		return BREAKPOINT_CMP_SMALLER;
	} else if (!_tcsncmp(tmp, _T("-"), 1)) {
		(*c) += 1;
		return BREAKPOINT_CMP_RANGE;
	} else if (!_tcsncmp(tmp, _T("!-"), 2)) {
		(*c) += 2;
		return BREAKPOINT_CMP_NRANGE;
	}
	*opersigned = false;
	if (**c == 's') {
		(*c)++;
		*opersigned = true;
	}
	return -1;
}

static const TCHAR *debugregs[] = {
	_T("D0"),
	_T("D1"),
	_T("D2"),
	_T("D3"),
	_T("D4"),
	_T("D5"),
	_T("D6"),
	_T("D7"),
	_T("A0"),
	_T("A1"),
	_T("A2"),
	_T("A3"),
	_T("A4"),
	_T("A5"),
	_T("A6"),
	_T("A7"),
	_T("PC"),
	_T("USP"),
	_T("MSP"),
	_T("ISP"),
	_T("VBR"),
	_T("SR"),
	_T("CCR"),
	_T("CACR"),
	_T("CAAR"),
	_T("SFC"),
	_T("DFC"),
	_T("TC"),
	_T("ITT0"),
	_T("ITT1"),
	_T("DTT0"),
	_T("DTT1"),
	_T("BUSC"),
	_T("PCR"),
	_T("FPIAR"),
	_T("FPCR"),
	_T("FPSR"),
	NULL
};

int getregidx(TCHAR **c)
{
	int i;
	TCHAR *p = *c;
	TCHAR tmp[10];
	int extra = 0;

	i = 0;
	while (p[i]) {
		tmp[i] = _totupper(p[i]);
		if (i >= sizeof(tmp) / sizeof(TCHAR) - 1)
			break;
		i++;
	}
	tmp[i] = 0;
	for (int i = 0; debugregs[i]; i++) {
		if (!_tcsncmp(tmp, debugregs[i], _tcslen(debugregs[i]))) {
			(*c) += _tcslen(debugregs[i]);
			return i;
		}
	}
	return -1;
}

uae_u32 returnregx(int regid)
{
	if (regid < BREAKPOINT_REG_PC)
		return regs.regs[regid];
	switch(regid)
	{
		case BREAKPOINT_REG_PC:
		return M68K_GETPC;
		case BREAKPOINT_REG_USP:
		return regs.usp;
		case BREAKPOINT_REG_MSP:
		return regs.msp;
		case BREAKPOINT_REG_ISP:
		return regs.isp;
		case BREAKPOINT_REG_VBR:
		return regs.vbr;
		case BREAKPOINT_REG_SR:
		MakeSR();
		return regs.sr;
		case BREAKPOINT_REG_CCR:
		MakeSR();
		return regs.sr & 31;
		case BREAKPOINT_REG_CACR:
		return regs.cacr;
		case BREAKPOINT_REG_CAAR:
		return regs.caar;
		case BREAKPOINT_REG_SFC:
		return regs.sfc;
		case BREAKPOINT_REG_DFC:
		return regs.dfc;
		case BREAKPOINT_REG_TC:
		if (currprefs.cpu_model == 68030)
			return tc_030;
		return regs.tcr;
		case BREAKPOINT_REG_ITT0:
		if (currprefs.cpu_model == 68030)
			return tt0_030;
		return regs.itt0;
		case BREAKPOINT_REG_ITT1:
		if (currprefs.cpu_model == 68030)
			return tt1_030;
		return regs.itt1;
		case BREAKPOINT_REG_DTT0:
		return regs.dtt0;
		case BREAKPOINT_REG_DTT1:
		return regs.dtt1;
		case BREAKPOINT_REG_BUSC:
		return regs.buscr;
		case BREAKPOINT_REG_PCR:
		return regs.pcr;
		case BREAKPOINT_REG_FPIAR:
		return regs.fpiar;
		case BREAKPOINT_REG_FPCR:
		return regs.fpcr;
		case BREAKPOINT_REG_FPSR:
		return regs.fpsr;
	}
	return 0;
}

static int readregx(TCHAR **c, uae_u32 *valp)
{
	int idx;
	uae_u32 addr;
	TCHAR *old = *c;

	addr = 0;
	if (_totupper(**c) == 'R') {
		(*c)++;
	}
	idx = getregidx(c);
	if (idx < 0) {
		*c = old;
		return 0;
	}
	addr = returnregx(idx);
	*valp = addr;
	return 1;
}

static bool checkisneg(TCHAR **c)
{
	TCHAR nc = peekchar(c);
	if (nc == '-') {
		(*c)++;
		return true;
	} else if (nc == '+') {
		(*c)++;
	}
	return false;
}

static bool readbinx (TCHAR **c, uae_u32 *valp)
{
	uae_u32 val = 0;
	bool first = true;
	bool negative = false;

	ignore_ws (c);
	negative = checkisneg(c);
	for (;;) {
		TCHAR nc = **c;
		if (nc != '1' && nc != '0' && nc != '`') {
			if (first)
				return false;
			break;
		}
		first = false;
		(*c)++;
		if (nc != '`') {
			val <<= 1;
			if (nc == '1') {
				val |= 1;
			}
		}
	}
	*valp = val * (negative ? -1 : 1);
	return true;
}

static bool readhexx (TCHAR **c, uae_u32 *valp)
{
	uae_u32 val = 0;
	TCHAR nc;
	bool negative = false;

	ignore_ws(c);
	negative = checkisneg(c);
	if (!isxdigit(peekchar(c)))
		return false;
	while (isxdigit (nc = **c)) {
		(*c)++;
		val *= 16;
		nc = _totupper (nc);
		if (isdigit (nc)) {
			val += nc - '0';
		} else {
			val += nc - 'A' + 10;
		}
	}
	*valp = val * (negative ? -1 : 1);
	return true;
}

static bool readintx (TCHAR **c, uae_u32 *valp)
{
	uae_u32 val = 0;
	TCHAR nc;
	int negative = 0;

	ignore_ws (c);
	negative = checkisneg(c);
	if (!isdigit (peekchar (c)))
		return false;
	while (isdigit (nc = **c)) {
		(*c)++;
		val *= 10;
		val += nc - '0';
	}
	*valp = val * (negative ? -1 : 1);
	return true;
}

static int checkvaltype2 (TCHAR **c, uae_u32 *val, TCHAR def)
{
	TCHAR nc;

	ignore_ws (c);
	nc = _totupper (**c);
	if (nc == '!') {
		(*c)++;
		return readintx (c, val) ? 1 : 0;
	}
	if (nc == '$') {
		(*c)++;
		return readhexx (c, val) ? 1 : 0;
	}
	if (nc == '0' && _totupper ((*c)[1]) == 'X') {
		(*c)+= 2;
		return readhexx (c, val) ? 1 : 0;
	}
	if (nc == '%') {
		(*c)++;
		return readbinx (c, val) ? 1: 0;
	}
	if (nc >= 'A' && nc <= 'Z' && nc != 'A' && nc != 'D') {
		if (readregx (c, val))
			return 1;
	}
	TCHAR name[256];
	name[0] = 0;
	for (int i = 0; i < sizeof name / sizeof(TCHAR) - 1; i++) {
		nc = (*c)[i];
		if (nc == 0 || nc == ' ')
			break;
		name[i] = nc;
		name[i + 1] = 0;
	}
	if (name[0]) {
		TCHAR *np = name;
		if (*np == '#')
			np++;
		if (debugmem_get_symbol_value(np, val)) {
			(*c) += _tcslen(name);
			return 1;
		}
	}
	if (def == '!') {
		return readintx (c, val) ? -1 : 0;
	} else if (def == '$') {
		return readhexx (c, val) ? -1 : 0;
	} else if (def == '%') {
		return readbinx (c, val) ? -1 : 0;
	}
	return 0;
}

static int readsize (int val, TCHAR **c)
{
	TCHAR cc = _totupper (readchar(c));
	if (cc == 'B')
		return 1;
	if (cc == 'W')
		return 2;
	if (cc == '3')
		return 3;
	if (cc == 'L')
		return 4;
	return 0;
}

static int checkvaltype(TCHAR **cp, uae_u32 *val, int *size, TCHAR def)
{
	TCHAR form[256], *p;
	bool gotop = false;
	bool copyrest = false;
	double out;

	form[0] = 0;
	if (size)
		*size = 0;
	p = form;
	for (;;) {
		uae_u32 v;
		if (!checkvaltype2(cp, &v, def)) {
			if (isoperator(cp) || gotop || **cp == '\"' || **cp == '\'') {
				goto docalc;
			}
			return 0;
		}
		*val = v;
		// stupid but works!
		_sntprintf(p, sizeof p, _T("%u"), v);
		p += _tcslen (p);
		*p = 0;
		if (peekchar(cp) == '.') {
			readchar(cp);
			if (size) {
				*size = readsize(v, cp);
			}
		}
		TCHAR *cpb = *cp;
		ignore_ws(cp);
		if (!isoperator(cp)) {
			*cp = cpb;
			break;
		}
		gotop = true;
		*p++= readchar(cp);
		*p = 0;
	}
	if (!gotop) {
		if (size && *size == 0) {
			uae_s32 v = (uae_s32)(*val);
			if (v > 65535 || v < -32767) {
				*size = 4;
			} else if (v > 255 || v < -127) {
				*size = 2;
			} else {
				*size = 1;
			}
		}
		return 1;
	}
docalc:
	while (more_params2(cp)) {
		TCHAR c = readchar(cp);
		if (c == ' ') {
			break;
		}
		*p++ = c;
	}
	*p = 0;
	TCHAR tmp[MAX_DPATH];
	int v = calc(form, &out, tmp, sizeof(tmp) / sizeof(TCHAR));
	if (v > 0) {
		*val = (uae_u32)out;
		if (size && *size == 0) {
			uae_s32 v = (uae_s32)(*val);
			if (v > 255 || v < -127) {
				*size = 2;
			} else if (v > 65535 || v < -32767) {
				*size = 4;
			} else {
				*size = 1;
			}
		}
		return 1;
	} else if (v < 0) {
		console_out_f(_T("String returned: '%s'\n"), tmp);
	}
	return 0;
}


static uae_u32 readnum(TCHAR **c, int *size, TCHAR def, bool *err)
{
	uae_u32 val;
	if (err) {
		*err = 0;
	}
	if (checkvaltype(c, &val, size, def)) {
		return val;
	}
	if (err) {
		*err = 1;
	}
	return 0;
}

static uae_u32 readint(TCHAR **c, bool *err)
{
	int size;
	return readnum(c, &size, '!', err);
}
static uae_u32 readhex(TCHAR **c, bool *err)
{
	int size;
	return readnum(c, &size, '$', err);
}
static uae_u32 readbin(TCHAR **c, bool *err)
{
	int size;
	return readnum(c, &size, '%', err);
}
static uae_u32 readint(TCHAR **c, int *size, bool *err)
{
	return readnum(c, size, '!', err);
}
static uae_u32 readhex(TCHAR **c, int *size, bool *err)
{
	return readnum(c, size, '$', err);
}

static size_t next_string (TCHAR **c, TCHAR *out, int max, int forceupper)
{
	TCHAR *p = out;
	int startmarker = 0;

	if (**c == '\"') {
		startmarker = 1;
		(*c)++;
	}
	*p = 0;
	while (**c != 0) {
		if (**c == '\"' && startmarker) {
			(*c)++;
			ignore_ws(c);
			break;
		}
		if (**c == 32 && !startmarker) {
			ignore_ws (c);
			break;
		}
		*p = next_char2(c);
		if (forceupper) {
			*p = _totupper(*p);
		}
		*++p = 0;
		max--;
		if (max <= 1) {
			break;
		}
	}
	return _tcslen (out);
}

static void converter(TCHAR **c)
{
	bool err;
	uae_u32 v = readint(c, &err);
	TCHAR s[100];
	int i, j;

	if (err) {
		return;
	}
	for (i = 0, j = 0; i < 32; i++) {
		s[j++] = (v & (1 << (31 - i))) ? '1' : '0';
		if (i < 31 && (i & 7) == 7) {
			s[j++] = '`';
		}
	}
	s[j] = 0;
	console_out_f (_T("$%08X = %%%s = %u = %d\n"), v, s, v, (uae_s32)v);
}

static bool isrom(uaecptr addr)
{
	addrbank *ab = &get_mem_bank(addr);
	if (ab->flags & ABFLAG_ROM) {
		return true;
	}
	return false;
}

static uae_u32 lastaddr(uae_u32 start)
{
	int lastbank = currprefs.address_space_24 ? 255 : 65535;

	addrbank *ab2 = get_mem_bank_real(start + 1);
	uae_u32 flags = ab2->flags & (ABFLAG_RAM | ABFLAG_ROM);
	if (start == 0xffffffff) {
		flags = ABFLAG_RAM;
	}
	for (int i = lastbank; i >= 0; i--) {
		addrbank *ab = get_mem_bank_real(i << 16);
		if (ab->baseaddr && (ab->flags & (ABFLAG_RAM | ABFLAG_ROM)) == flags) {
			return (i + 1) << 16;
		}
	}
	return 0;
}

static uae_u32 nextaddr_ab_flags = ABFLAG_RAM;
static uae_u32 nextaddr_ab_flags_mask = ABFLAG_RAM;

static void nextaddr_init(uaecptr addr)
{
	addrbank *ab = get_mem_bank_real(addr + 1);
	if (addr != 0xffffffff && (ab->flags & ABFLAG_ROM)) {
		nextaddr_ab_flags = ABFLAG_ROM;
		nextaddr_ab_flags_mask = ABFLAG_ROM;
	} else {
		nextaddr_ab_flags = ABFLAG_RAM;
		nextaddr_ab_flags_mask = ABFLAG_RAM;
	}
}

static uaecptr nextaddr(uaecptr addr, uaecptr last, uaecptr *endp, bool verbose, bool *lfp)
{
	addrbank *ab;
	int lastbank = currprefs.address_space_24 ? 255 : 65535;

	if (addr != 0xffffffff) {
		addrbank *ab2 = get_mem_bank_real(addr);
		addr++;
		ab = get_mem_bank_real(addr);
		if (ab->baseaddr && (ab->flags & nextaddr_ab_flags_mask) == nextaddr_ab_flags && ab == ab2) {
			return addr;
		}
	} else {
		addr = 0;
	}

	while (addr < (lastbank << 16)) {
		ab = get_mem_bank_real(addr);
		if (ab->baseaddr && ((ab->flags & nextaddr_ab_flags_mask) == nextaddr_ab_flags))
			break;
		addr += 65536;
	}
	if (addr >= (lastbank << 16)) {
		if (endp)
			*endp = 0xffffffff;
		return 0xffffffff;
	}

	uaecptr start = addr;

	while (addr <= (lastbank << 16)) {
		addrbank *ab2 = get_mem_bank_real(addr);
		if ((last && last != 0xffffffff && addr >= last) || !ab2->baseaddr || ((ab2->flags & nextaddr_ab_flags_mask) != nextaddr_ab_flags) || ab != ab2) {
			if (endp)
				*endp = addr;
			break;
		}
		addr += 65536;
	}

	if (verbose) {
		if (lfp && *lfp) {
			console_out_f(_T("\n"));
			*lfp = false;
		}
		console_out_f(_T("Scanning.. %08x - %08x (%s)\n"), start, addr, get_mem_bank(start).name);
	}

	return start;
}

uaecptr dumpmem2 (uaecptr addr, TCHAR *out, int osize)
{
	int i, cols = 8;
	int nonsafe = 0;

	if (osize <= (9 + cols * 5 + 1 + 2 * cols))
		return addr;
	_sntprintf (out, sizeof out, _T("%08X "), addr);
	for (i = 0; i < cols; i++) {
		uae_u8 b1, b2;
		b1 = b2 = 0;
		if (debug_safe_addr (addr, 1)) {
			b1 = get_byte_debug (addr + 0);
			b2 = get_byte_debug (addr + 1);
			_sntprintf (out + 9 + i * 5, sizeof out + 9 + i * 5, _T("%02X%02X "), b1, b2);
			out[9 + cols * 5 + 1 + i * 2 + 0] = b1 >= 32 && b1 < 127 ? b1 : '.';
			out[9 + cols * 5 + 1 + i * 2 + 1] = b2 >= 32 && b2 < 127 ? b2 : '.';
		} else {
			nonsafe++;
			_tcscpy (out + 9 + i * 5, _T("**** "));
			out[9 + cols * 5 + 1 + i * 2 + 0] = '*';
			out[9 + cols * 5 + 1 + i * 2 + 1] = '*';
		}
		addr += 2;
	}
	out[9 + cols * 5] = ' ';
	out[9 + cols * 5 + 1 + 2 * cols] = 0;
	if (nonsafe == cols) {
		addrbank *ab = &get_mem_bank (addr);
		if (ab->name)
			memcpy (out + (9 + 4 + 1) * sizeof (TCHAR), ab->name, _tcslen (ab->name) * sizeof (TCHAR));
	}
	return addr;
}

static TCHAR dumpmemline[MAX_LINEWIDTH + 1];

static void dumpmem (uaecptr addr, uaecptr *nxmem, int lines)
{
	for (;lines--;) {
		addr = dumpmem2 (addr, dumpmemline, sizeof(dumpmemline) / sizeof(TCHAR));
		debug_out (_T("%s"), dumpmemline);
		if (!debug_out (_T("\n")))
			break;
	}
	*nxmem = addr;
}

static void dump_custom_regs(bool aga, bool ext)
{
	size_t len;
	uae_u8 *p1, *p2, *p3, *p4;
	TCHAR extra1[256], extra2[256];

	extra1[0] = 0;
	extra2[0] = 0;
	if (aga) {
		dump_aga_custom();
		return;
	}

	p1 = p2 = save_custom (&len, 0, 1);
	p1 += 4; // skip chipset type
	for (int i = 0; i < 4; i++) {
		p4 = p1 + 0xa0 + i * 16;
		p3 = save_audio (i, &len, 0);
		p4[0] = p3[12];
		p4[1] = p3[13];
		p4[2] = p3[14];
		p4[3] = p3[15];
		p4[4] = p3[4];
		p4[5] = p3[5];
		p4[6] = p3[8];
		p4[7] = p3[9];
		p4[8] = 0;
		p4[9] = p3[1];
		p4[10] = p3[10];
		p4[11] = p3[11];
		free (p3);
	}
	int total = 0;
	int i = 0;
	while (custd[i].name) {
		if (!(custd[i].special & CD_NONE))
			total++;
		i++;
	}
	int cnt1 = 0;
	int cnt2 = 0;
	i = 0;
	while (i < total / 2 + 1) {
		for (;;) {
			cnt2++;
			if (!(custd[cnt2].special & CD_NONE))
				break;
		}
		i++;
	}
	for (int i = 0; i < total / 2 + 1; i++) {
		uae_u16 v1, v2;
		int addr1, addr2;
		addr1 = custd[cnt1].adr & 0x1ff;
		addr2 = custd[cnt2].adr & 0x1ff;
		v1 = (p1[addr1 + 0] << 8) | p1[addr1 + 1];
		v2 = (p1[addr2 + 0] << 8) | p1[addr2 + 1];
		if (ext) {
			struct custom_store *cs;
			cs = &custom_storage[addr1 >> 1];
			_sntprintf(extra1, sizeof extra1, _T("\t%04X %08X %s"), cs->value, cs->pc & ~1, (cs->pc & 1) ? _T("COP") : _T("CPU"));
			cs = &custom_storage[addr2 >> 1];
			_sntprintf(extra2, sizeof extra2, _T("\t%04X %08X %s"), cs->value, cs->pc & ~1, (cs->pc & 1) ? _T("COP") : _T("CPU"));
		}
		console_out_f (_T("%03X %s\t%04X%s\t%03X %s\t%04X%s\n"),
			addr1, custd[cnt1].name, v1, extra1,
			addr2, custd[cnt2].name, v2, extra2);
		for (;;) {
			cnt1++;
			if (!(custd[cnt1].special & CD_NONE))
				break;
		}
		for (;;) {
			cnt2++;
			if (!(custd[cnt2].special & CD_NONE))
				break;
		}
	}
	xfree(p2);
}

static void dump_vectors (uaecptr addr)
{
	int i = 0, j = 0;

	if (addr == 0xffffffff)
		addr = regs.vbr;

	while (int_labels[i].name || trap_labels[j].name) {
		if (int_labels[i].name) {
			console_out_f (_T("$%08X %02d: %12s $%08X  "), int_labels[i].adr + addr, int_labels[i].adr / 4,
				int_labels[i].name, get_long_debug (int_labels[i].adr + addr));
			i++;
		}
		if (trap_labels[j].name) {
			console_out_f (_T("$%08X %02d: %12s $%08X"), trap_labels[j].adr + addr, trap_labels[j].adr / 4,
				trap_labels[j].name, get_long_debug (trap_labels[j].adr + addr));
			j++;
		}
		console_out (_T("\n"));
	}
}

static void disassemble_wait (FILE *file, unsigned long insn)
{
	int vp, hp, ve, he, bfd, v_mask, h_mask;
	int doout = 0;

	vp = (insn & 0xff000000) >> 24;
	hp = (insn & 0x00fe0000) >> 16;
	ve = (insn & 0x00007f00) >> 8;
	he = (insn & 0x000000fe);
	bfd = (insn & 0x00008000) >> 15;

	/* bit15 can never be masked out*/
	v_mask = vp & (ve | 0x80);
	h_mask = hp & he;
	if (v_mask > 0) {
		doout = 1;
		console_out (_T("vpos "));
		if (ve != 0x7f) {
			console_out_f (_T("& 0x%02x "), ve);
		}
		console_out_f (_T(">= 0x%02x"), v_mask);
	}
	if (he > 0) {
		if (v_mask > 0) {
			console_out (_T(" and"));
		}
		console_out (_T(" hpos "));
		if (he != 0xfe) {
			console_out_f (_T("& 0x%02x "), he);
		}
		console_out_f (_T(">= 0x%02x"), h_mask);
	} else {
		if (doout)
			console_out (_T(", "));
		console_out (_T(", ignore horizontal"));
	}

	console_out_f (_T("\n                        \t;  VP %02x, VE %02x; HP %02x, HE %02x; BFD %d\n"),
		vp, ve, hp, he, bfd);
}

#define NR_COPPER_RECORDS 100000
/* Record copper activity for the debugger.  */
struct cop_rec
{
	uae_u16 w1, w2;
	int hpos, vpos;
	int bhpos, bvpos;
	uaecptr addr, nextaddr;
};
static struct cop_rec *cop_record[2];
static int nr_cop_records[2], curr_cop_set, selected_cop_set;

#define NR_DMA_REC_LINES_MAX 1000
#define NR_DMA_REC_COLS_MAX 300
#define NR_DMA_REC_MAX (NR_DMA_REC_LINES_MAX * NR_DMA_REC_COLS_MAX)
static struct dma_rec *dma_record_data;
static int dma_record_cycle;
static int dma_record_vpos_type;
static struct dma_rec **dma_record_lines;
struct dma_rec *last_dma_rec;

struct dma_rec *record_dma_next_cycle(int hpos, int vpos, int vvpos)
{
	if (!dma_record_data) {
		return NULL;
	}

	struct dma_rec *dr = &dma_record_data[dma_record_cycle];
	struct dma_rec *dro = dr;
	dr->hpos = hpos;
	dr->vpos[0] = vpos;
	dr->vpos[1] = vvpos;
	dr->frame = vsync_counter;
	dr->tick = currcycle_cck;
	dma_record_cycle++;
	if (dma_record_cycle >= NR_DMA_REC_MAX) {
		dma_record_cycle = 0;
	}
	if (hpos == 0 && vvpos < NR_DMA_REC_LINES_MAX) {
		dma_record_lines[vvpos] = dr;
	}
	dr = &dma_record_data[dma_record_cycle];
	memset(dr, 0, sizeof(struct dma_rec));
	dr->reg = 0xffff;
	dr->cf_reg = 0xffff;
	dr->denise_evt[0] = DENISE_EVENT_UNKNOWN;
	dr->denise_evt[1] = DENISE_EVENT_UNKNOWN;
	dr->agnus_evt = dro->agnus_evt;
	dr->hpos = -1;
	return dro;
}

static void dma_record_init(void)
{
	if (!dma_record_data) {
		dma_record_data = xcalloc(struct dma_rec, NR_DMA_REC_MAX + 2);
		dma_record_lines = xcalloc(struct dma_rec*, NR_DMA_REC_LINES_MAX);
		for (int i = 0;i < NR_DMA_REC_MAX; i++) {
			struct dma_rec *dr = &dma_record_data[i];
			dr->reg = 0xffff;
			dr->cf_reg = 0xffff;
			dr->hpos = -1;
		}
	}
}

void record_dma_reset(int start)
{
	if (start && !dma_record_data) {
		dma_record_init();
	}
	if (!dma_record_data) {
		return;
	}
	if (start && !debug_dma) {
		debug_dma = start;
	}
}

void record_copper_reset (void)
{
	/* Start a new set of copper records.  */
	curr_cop_set ^= 1;
	nr_cop_records[curr_cop_set] = 0;
}

static uae_u32 ledcolor (uae_u32 c, uae_u32 *rc, uae_u32 *gc, uae_u32 *bc, uae_u32 *a)
{
	uae_u32 v = rc[(c >> 16) & 0xff] | gc[(c >> 8) & 0xff] | bc[(c >> 0) & 0xff];
	if (a)
		v |= a[255 - ((c >> 24) & 0xff)];
	return v;
}

#define lc(x) ledcolor (x, xredcolors, xgreencolors, xbluecolors, NULL)
#define DMARECORD_SUBITEMS 8
struct dmadebug
{
	uae_u32 l[DMARECORD_SUBITEMS];
	uae_u8 r, g, b;
	bool enabled;
	int max;
	const TCHAR *name;
};

static uae_u32 intlevc[] = { 0x000000, 0x444444, 0x008800, 0xffff00, 0x000088, 0x880000, 0xff0000, 0xffffff };
static struct dmadebug debug_colors[DMARECORD_MAX];
static bool debug_colors_set;

static void set_dbg_color(int index, int extra, uae_u8 r, uae_u8 g, uae_u8 b, int max, const TCHAR *name)
{
	if (extra <= 0) {
		debug_colors[index].r = r;
		debug_colors[index].g = g;
		debug_colors[index].b = b;
		debug_colors[index].enabled = true;
	}
	if (name != NULL)
		debug_colors[index].name = name;
	if (max > 0)
		debug_colors[index].max = max;
	if (extra >= 0) {
		debug_colors[index].l[extra] = lc((r << 16) | (g << 8) | (b << 0));
	} else {
		for (int i = 0; i < DMARECORD_SUBITEMS; i++) {
			debug_colors[index].l[i] = lc((r << 16) | (g << 8) | (b << 0));
		}
	}
}

static void set_debug_colors(void)
{
	if (debug_colors_set)
		return;
	debug_colors_set = true;
	set_dbg_color(0,						0, 0x22, 0x22, 0x22, 1, _T("-"));
	set_dbg_color(DMARECORD_REFRESH,		0, 0x44, 0x44, 0x44, 4, _T("Refresh"));
	set_dbg_color(DMARECORD_CPU,			0, 0xa2, 0x53, 0x42, 2, _T("CPU")); // code
	set_dbg_color(DMARECORD_COPPER,		0, 0xee, 0xee, 0x00, 3, _T("Copper"));
	set_dbg_color(DMARECORD_AUDIO,			0, 0xff, 0x00, 0x00, 4, _T("Audio"));
	set_dbg_color(DMARECORD_BLITTER,		0, 0x00, 0x88, 0x88, 2, _T("Blitter")); // blit A
	set_dbg_color(DMARECORD_BITPLANE,		0, 0x00, 0x00, 0xff, 8, _T("Bitplane"));
	set_dbg_color(DMARECORD_SPRITE,		0, 0xff, 0x00, 0xff, 8, _T("Sprite"));
	set_dbg_color(DMARECORD_DISK,			0, 0xff, 0xff, 0xff, 3, _T("Disk"));
	set_dbg_color(DMARECORD_CONFLICT,		0, 0xff, 0xb8, 0x40, 0, _T("Conflict"));

	for (int i = 0; i < DMARECORD_MAX; i++) {
		for (int j = 1; j < DMARECORD_SUBITEMS; j++) {
			debug_colors[i].l[j] = debug_colors[i].l[0];
		}
	}

	set_dbg_color(DMARECORD_CPU,		1, 0xad, 0x98, 0xd6, 0, NULL); // data
	set_dbg_color(DMARECORD_COPPER,	1, 0xaa, 0xaa, 0x22, 0, NULL); // wait
	set_dbg_color(DMARECORD_COPPER,	2, 0x66, 0x66, 0x44, 0, NULL); // special
	set_dbg_color(DMARECORD_BLITTER,	1, 0x00, 0x88, 0x88, 0, NULL); // blit B
	set_dbg_color(DMARECORD_BLITTER,	2, 0x00, 0x88, 0x88, 0, NULL); // blit C
	set_dbg_color(DMARECORD_BLITTER,	3, 0x00, 0xaa, 0x88, 0, NULL); // blit D (write)
	set_dbg_color(DMARECORD_BLITTER,	4, 0x00, 0x88, 0xff, 0, NULL); // fill A-D
	set_dbg_color(DMARECORD_BLITTER,	6, 0x00, 0xff, 0x00, 0, NULL); // line A-D
}

static int cycles_toggle;

static void debug_draw_cycles(uae_u8 *buf, uae_u8 *genlock, int line, int width, int height, uae_u32 *xredcolors, uae_u32 *xgreencolors, uae_u32 *xbluescolors)
{
	int y, x, xx, dx, xplus, yplus;
	struct dma_rec *dr;

	if (debug_dma >= 4)
		yplus = 2;
	else
		yplus = 1;
	if (debug_dma >= 5)
		xplus = 3;
	else if (debug_dma >= 3)
		xplus = 2;
	else
		xplus = 1;

	y = line / yplus;
	if (yplus < 2)
		y -= 8;

	if (y < 0)
		return;
	if (y >= NR_DMA_REC_LINES_MAX)
		return;
	if (y >= height)
		return;

	dr = dma_record_lines[y];
	if (!dr)
		return;
	dx = width - xplus * ((maxhpos + 1) & ~1) - 16;

	bool ended = false;
	uae_s8 intlev = 0;
	for (x = 0; x < NR_DMA_REC_COLS_MAX; x++) {
		uae_u32 c = debug_colors[0].l[0];
		xx = x * xplus + dx;

		if (dr->end) {
			ended = true;
		}
		if (ended) {
			c = 0;
		} else {
			if (dr->reg != 0xffff && debug_colors[dr->type].enabled) {
				// General DMA slots
				c = debug_colors[dr->type].l[dr->extra & 7];

				// Special cases
				if (dr->cf_reg != 0xffff && ((cycles_toggle ^ line) & 1)) {
					c = debug_colors[DMARECORD_CONFLICT].l[0];
				} else if (dr->extra > 0xF) {
					// High bits of "extra" contain additional blitter state.
					if (dr->extra & 0x10)
						c = debug_colors[dr->type].l[4]; // blit fill, channels A-D
					else if (dr->extra & 0x20)
						c = debug_colors[dr->type].l[6]; // blit line, channels A-D
				}
			}
		}
		if (dr->intlev > intlev)
			intlev = dr->intlev;
		putpixel(buf, genlock, xx + 4, c);
		if (xplus > 1)
			putpixel(buf, genlock, xx + 4 + 1, c);
		if (xplus > 2)
			putpixel(buf, genlock, xx + 4 + 2, c);

		dr++;
		if (dr->hpos == 0) {
			break;
		}
	}
	putpixel(buf, genlock, dx + 0, 0);
	putpixel(buf, genlock, dx + 1, lc(intlevc[intlev]));
	putpixel(buf, genlock, dx + 2, lc(intlevc[intlev]));
	putpixel(buf, genlock, dx + 3, 0);
}

#define HEATMAP_WIDTH 256
#define HEATMAP_HEIGHT 256
#define HEATMAP_COUNT 32
#define HEATMAP_DIV 8
static const int max_heatmap = 16 * 1048576; // 16M
static uae_u32 *heatmap_debug_colors;

static struct memory_heatmap *heatmap;
struct memory_heatmap
{
	uae_u32 mask;
	uae_u32 cpucnt;
	uae_u16 cnt;
	uae_u16 type, extra;
};

static void debug_draw_heatmap(uae_u8 *buf, uae_u8 *genlock, int line, int width, int height, uae_u32 *xredcolors, uae_u32 *xgreencolors, uae_u32 *xbluescolors)
{
	struct memory_heatmap *mht = heatmap;
	int dx = 16;
	int y = line;

	if (y < 0 || y >= HEATMAP_HEIGHT)
		return;

	mht += y * HEATMAP_WIDTH;

	for (int x = 0; x < HEATMAP_WIDTH; x++) {
		uae_u32 c = heatmap_debug_colors[mht->cnt * DMARECORD_MAX + mht->type];
		//c = heatmap_debug_colors[(HEATMAP_COUNT - 1) * DMARECORD_MAX + DMARECORD_CPU_I];
		int xx = x + dx;
		putpixel(buf, genlock, xx, c);
		if (mht->cnt > 0)
			mht->cnt--;
		mht++;
	}
}

void debug_draw(uae_u8 *buf, uae_u8 *genlock, int line, int width, int height, uae_u32 *xredcolors, uae_u32 *xgreencolors, uae_u32 *xbluescolors)
{
	if (!heatmap_debug_colors) {
		heatmap_debug_colors = xcalloc(uae_u32, DMARECORD_MAX * HEATMAP_COUNT);
		set_debug_colors();
		for (int i = 0; i < HEATMAP_COUNT; i++) {
			uae_u32 *cp = heatmap_debug_colors + i * DMARECORD_MAX;
			for (int j = 0; j < DMARECORD_MAX; j++) {
				uae_u8 r = debug_colors[j].r;
				uae_u8 g = debug_colors[j].g;
				uae_u8 b = debug_colors[j].b;
				r = r * i / HEATMAP_COUNT;
				g = g * i / HEATMAP_COUNT;
				b = b * i / HEATMAP_COUNT;
				cp[j] = lc((r << 16) | (g << 8) | (b << 0));
			}
		}
	}

	if (heatmap) {
		debug_draw_heatmap(buf, genlock, line, width, height, xredcolors, xgreencolors, xbluecolors);
	} else if (dma_record_data) {
		debug_draw_cycles(buf, genlock, line, width, height, xredcolors, xgreencolors, xbluecolors);
	}
}

struct heatmapstore
{
	TCHAR *s;
	double v;
};

static void heatmap_stats(TCHAR **c)
{
	int range = 95;
	int maxlines = 30;
	double max;
	int maxcnt;
	uae_u32 mask = MW_MASK_CPU_I;
	const TCHAR *maskname = NULL;

	if (more_params(c)) {
		if (**c == 'c' && peek_next_char(c) == 0) {
			for (int i = 0; i < max_heatmap / HEATMAP_DIV; i++) {
				struct memory_heatmap *hm = &heatmap[i];
				memset(hm, 0, sizeof(struct memory_heatmap));
			}
			console_out(_T("heatmap data cleared\n"));
			return;
		}
		if (!isdigit(peek_next_char(c))) {
			TCHAR str[100];
			if (next_string(c, str, sizeof str / sizeof (TCHAR), true)) {
				for (int j = 0; memwatch_access_masks[j].mask; j++) {
					if (!_tcsicmp(str, memwatch_access_masks[j].name)) {
						mask = memwatch_access_masks[j].mask;
						maskname = memwatch_access_masks[j].name;
						console_out_f(_T("Mask %08x Name %s\n"), mask, maskname);
						break;
					}
				}
			}
			if (more_params(c)) {
				maxlines = readint(c, NULL);
			}
		} else {
			range = readint(c, NULL);
			if (more_params(c)) {
				maxlines = readint(c, NULL);
			}
		}
	}
	if (maxlines <= 0)
		maxlines = 10000;

	if (mask != MW_MASK_CPU_I) {

		int found = -1;
		int firstaddress = 0;
		for (int lines = 0; lines < maxlines; lines++) {

			for (; firstaddress < max_heatmap / HEATMAP_DIV; firstaddress++) {
				struct memory_heatmap *hm = &heatmap[firstaddress];
				if (hm->mask & mask)
					break;
			}

			if (firstaddress == max_heatmap / HEATMAP_DIV)
				return;

			int lastaddress;
			for (lastaddress = firstaddress; lastaddress < max_heatmap / HEATMAP_DIV; lastaddress++) {
				struct memory_heatmap *hm = &heatmap[lastaddress];
				if (!(hm->mask & mask))
					break;
			}
			lastaddress--;

			console_out_f(_T("%03d: %08x - %08x %08x (%d) %s\n"), 
				lines,
				firstaddress * HEATMAP_DIV, lastaddress * HEATMAP_DIV + HEATMAP_DIV - 1,
				lastaddress * HEATMAP_DIV - firstaddress * HEATMAP_DIV + HEATMAP_DIV - 1,
				lastaddress * HEATMAP_DIV - firstaddress * HEATMAP_DIV + HEATMAP_DIV - 1, 
				maskname);

			firstaddress = lastaddress + 1;
		}

	} else {
#define MAX_HEATMAP_LINES 1000
		struct heatmapstore linestore[MAX_HEATMAP_LINES] = { 0 };
		int storecnt = 0;
		uae_u32 maxlimit = 0xffffffff;

		max = 0;
		maxcnt = 0;
		for (int i = 0; i < max_heatmap / HEATMAP_DIV; i++) {
			struct memory_heatmap *hm = &heatmap[i];
			if (hm->cpucnt > 0) {
				max += hm->cpucnt;
				maxcnt++;
			}
		}

		if (!maxcnt) {
			console_out(_T("No CPU accesses found\n"));
			return;
		}

		for (int lines = 0; lines < maxlines; lines++) {

			int found = -1;
			int foundcnt = 0;

			for (int i = 0; i < max_heatmap / HEATMAP_DIV; i++) {
				struct memory_heatmap *hm = &heatmap[i];
				if (hm->cpucnt > 0 && hm->cpucnt > foundcnt && hm->cpucnt < maxlimit) {
					foundcnt = hm->cpucnt;
					found = i;
				}
			}
			if (found < 0)
				break;

			int totalcnt = 0;
			int cntrange = foundcnt * range / 100;
			if (cntrange <= 0)
				cntrange = 1;

			int lastaddress;
			for (lastaddress = found; lastaddress < max_heatmap / HEATMAP_DIV; lastaddress++) {
				struct memory_heatmap *hm = &heatmap[lastaddress];
				if (hm->cpucnt == 0 || hm->cpucnt < cntrange || hm->cpucnt >= maxlimit)
					break;
				totalcnt += hm->cpucnt;
			}
			lastaddress--;

			int firstaddress;
			for (firstaddress = found - 1; firstaddress >= 0; firstaddress--) {
				struct memory_heatmap *hm = &heatmap[firstaddress];
				if (hm->cpucnt == 0 || hm->cpucnt < cntrange || hm->cpucnt >= maxlimit)
					break;
				totalcnt += hm->cpucnt;
			}
			firstaddress--;

			firstaddress *= HEATMAP_DIV;
			lastaddress *= HEATMAP_DIV;

			TCHAR tmp[100];
			double pct = totalcnt / max * 100.0;
			_sntprintf(tmp, sizeof tmp, _T("%03d: %08x - %08x %08x (%d) %.5f%%\n"), lines + 1,
				firstaddress, lastaddress + HEATMAP_DIV - 1,
				lastaddress - firstaddress + HEATMAP_DIV - 1,
				lastaddress - firstaddress + HEATMAP_DIV - 1,
				pct);
			linestore[storecnt].s = my_strdup(tmp);
			linestore[storecnt].v = pct;

			storecnt++;
			if (storecnt >= MAX_HEATMAP_LINES)
				break;

			maxlimit = foundcnt;
		}

		for (int lines1 = 0; lines1 < storecnt; lines1++) {
			for (int lines2 = lines1 + 1; lines2 < storecnt; lines2++) {
				if (linestore[lines1].v < linestore[lines2].v) {
					struct heatmapstore hms;
					memcpy(&hms, &linestore[lines1], sizeof(struct heatmapstore));
					memcpy(&linestore[lines1], &linestore[lines2], sizeof(struct heatmapstore));
					memcpy(&linestore[lines2], &hms, sizeof(struct heatmapstore));
				}
			}
		}
		for (int lines1 = 0; lines1 < storecnt; lines1++) {
			console_out(linestore[lines1].s);
			xfree(linestore[lines1].s);
		}

	}

}

static void free_heatmap(void)
{
	xfree(heatmap);
	heatmap = NULL;
	debug_heatmap = 0;
}

static void init_heatmap(void)
{
	if (!heatmap)
		heatmap = xcalloc(struct memory_heatmap, max_heatmap / HEATMAP_DIV);
}

static void memwatch_heatmap(uaecptr addr, int rwi, int size, uae_u32 accessmask)
{
	if (addr >= max_heatmap || !heatmap)
		return;
	struct memory_heatmap *hm = &heatmap[addr / HEATMAP_DIV];
	if (accessmask & MW_MASK_CPU_I) {
		hm->cpucnt++;
	}
	hm->cnt = HEATMAP_COUNT - 1;
	int type = 0;
	int extra = 0;
	for (int i = 0; i < 32; i++) {
		if (accessmask & (1 << i)) {
			switch (1 << i)
			{
				case MW_MASK_BPL_0:
				case MW_MASK_BPL_1:
				case MW_MASK_BPL_2:
				case MW_MASK_BPL_3:
				case MW_MASK_BPL_4:
				case MW_MASK_BPL_5:
				case MW_MASK_BPL_6:
				case MW_MASK_BPL_7:
				type = DMARECORD_BITPLANE;
				break;
				case MW_MASK_AUDIO_0:
				case MW_MASK_AUDIO_1:
				case MW_MASK_AUDIO_2:
				case MW_MASK_AUDIO_3:
				type = DMARECORD_AUDIO;
				break;
				case MW_MASK_BLITTER_A:
				case MW_MASK_BLITTER_B:
				case MW_MASK_BLITTER_C:
				case MW_MASK_BLITTER_D_N:
				case MW_MASK_BLITTER_D_F:
				case MW_MASK_BLITTER_D_L:
				type = DMARECORD_BLITTER;
				break;
				case MW_MASK_COPPER:
				type = DMARECORD_COPPER;
				break;
				case MW_MASK_DISK:
				type = DMARECORD_DISK;
				break;
				case MW_MASK_CPU_I:
				type = DMARECORD_CPU;
				break;
				case MW_MASK_CPU_D_R:
				case MW_MASK_CPU_D_W:
				type = DMARECORD_CPU;
				extra = 1;
				break;
			}
		}
	}
	hm->type = type;
	hm->extra = extra;
	hm->mask |= accessmask;
}

struct refdata
{
	evt_t c;
	uae_u32 cnt;
};
static struct refdata refreshtable[1024];
static int refcheck_count;
#define REFRESH_LINES 64

static void check_refreshed(void)
{
	int max = ecs_agnus ? 512 : 256;
	int reffail = 0;
	evt_t c = get_cycles();
	for (int i = 0; i < max; i++) {
		struct refdata *rd = &refreshtable[i];
		if (rd->cnt < 10) {
			rd->cnt++;
		}
		if (rd->cnt == 10) {
			reffail++;
			rd->cnt = 0;
		}
		if (rd->c && (int)c - (int)rd->c >= CYCLE_UNIT * maxhpos * REFRESH_LINES) {
			reffail++;
			rd->c = 0;
			rd->cnt = 0;
		}
		if (reffail) {
			write_log("%03u ", i);
		}
	}
	if (reffail) {
		write_log("%d memory rows not refreshed fast enough!\n", reffail);
	}
}

void debug_mark_refreshed(uaecptr rp)
{
	int ras;
	if (ecs_agnus && currprefs.chipmem.size > 0x100000) {
		ras = (rp >> 9) & 0x3ff;
	} else if (ecs_agnus) {
		ras = (rp >> 9) & 0x1ff;
	} else {
		ras = (rp >> 1) & 0xff;
	}
	struct refdata *rd = &refreshtable[ras];
	evt_t c = get_cycles();
	rd->c = c;
	rd->cnt = 0;
}

void record_dma_ipl(void)
{
	struct dma_rec *dr;

	if (!dma_record_data)
		return;
	dr = &dma_record_data[dma_record_cycle];
	dr->intlev = regs.intmask;
	dr->ipl = regs.ipl_pin;
	dr->evt |= DMA_EVENT_IPL;
}

void record_dma_ipl_sample(void)
{
	struct dma_rec *dr;

	if (!dma_record_data)
		return;
	dr = &dma_record_data[dma_record_cycle];
	dr->intlev = regs.intmask;
	dr->ipl2 = regs.ipl_pin;
	dr->evt |= DMA_EVENT_IPLSAMPLE;
}

void record_dma_event_denise(struct dma_rec *dr, int h, uae_u32 evt, bool onoff)
{
	if (!dma_record_data)
		return;
	if (h && !(dr->denise_evt[1] & DENISE_EVENT_COPIED)) {
		dr->denise_evt[1] = dr->denise_evt[0] | DENISE_EVENT_COPIED;
	}
	if (onoff) {
		dr->denise_evt[h] |= evt;
		dr->denise_evt_changed[h] |= evt;
	} else {
		dr->denise_evt[h] &= ~evt;
		dr->denise_evt_changed[h] |= evt;
	}
}

void record_dma_event_agnus(uae_u32 evt, bool onoff)
{
	struct dma_rec *dr;

	if (!dma_record_data)
		return;
	dr = &dma_record_data[dma_record_cycle];
	if (onoff) {
		dr->agnus_evt |= evt;
		dr->agnus_evt_changed |= evt;
	} else {
		dr->agnus_evt &= ~evt;
		dr->agnus_evt_changed |= evt;
	}
}

void record_dma_event(uae_u32 evt)
{
	struct dma_rec *dr;

	if (!dma_record_data)
		return;
	dr = &dma_record_data[dma_record_cycle];
	dr->evt |= evt;
	dr->ipl = regs.ipl_pin;
}

void record_dma_event_data(uae_u32 evt, uae_u32 data)
{
	struct dma_rec *dr;

	if (!dma_record_data)
		return;
	dr = &dma_record_data[dma_record_cycle];
	dr->evt |= evt;
	dr->evtdata = data;
	dr->evtdataset = true;
	dr->ipl = regs.ipl_pin;
}

void record_dma_replace(int type, int extra)
{
	struct dma_rec *dr;

	if (!dma_record_data)
		return;
	dr = &dma_record_data[dma_record_cycle];
	if (dr->reg == 0xffff) {
		write_log(_T("DMA record replace without old data!\n"));
		return;
	}
	if (dr->type != type) {
		write_log(_T("DMA record replace type change %d -> %d!\n"), dr->type, type);
		return;
	}
	dr->extra = extra;
}

static void dma_conflict(int vpos, int hpos, struct dma_rec *dr, int reg, bool write)
{
	write_log(_T("DMA conflict %c: v=%d h=%d OREG=%04X NREG=%04X\n"), write ? 'W' : 'R', vpos, hpos, dr->reg, reg);
	//activate_debugger();
}

void record_dma_write(uae_u16 reg, uae_u32 dat, uae_u32 addr, int type, int extra)
{
	struct dma_rec *dr;

	if (!dma_record_data) {
		dma_record_init();
		if (!dma_record_data)
			return;
	}

	dr = &dma_record_data[dma_record_cycle];
	if (dr->reg != 0xffff) {
		dr->cf_reg = reg;
		dr->cf_dat = dat;
		dr->cf_addr = addr;
		dma_conflict(dr->vpos[0], dr->hpos, dr, reg, false);
		return;
	}
	dr->reg = reg;
	dr->dat = dat;
	dr->addr = addr;
	dr->type = type;
	dr->extra = extra;
	dr->intlev = regs.intmask;
	dr->ipl = regs.ipl_pin;
	dr->size = 2;
	dr->end = false;
	last_dma_rec = dr;
	debug_mark_refreshed(dr->addr);
}

void record_dma_read_value_pos(uae_u32 v)
{
	if (!dma_record_data)
		return;
	struct dma_rec *dr = &dma_record_data[dma_record_cycle];
	last_dma_rec = dr;
	record_dma_read_value(v);
}

void record_dma_read_value(uae_u32 v)
{
	if (last_dma_rec) {
		if (last_dma_rec->cf_reg != 0xffff) {
			last_dma_rec->cf_dat = v;
		} else {
			last_dma_rec->dat = v;
		}
		last_dma_rec->size = 2;
	}
}

void record_dma_read_value_wide(uae_u64 v, bool quad)
{
	if (last_dma_rec) {
		if (last_dma_rec->cf_reg != 0xffff) {
			last_dma_rec->cf_dat = (uae_u16)v;
		} else {
			last_dma_rec->dat = v;
		}
		last_dma_rec->size = quad ? 8 : 4;
	}
}

bool record_dma_check(void)
{
	if (!dma_record_data)
		return false;
	struct dma_rec *dr = &dma_record_data[dma_record_cycle];
	return dr->reg != 0xffff;
}

void record_dma_clear(void)
{
	if (!dma_record_data)
		return;
	struct dma_rec *dr = &dma_record_data[dma_record_cycle];
	dr->reg = 0xffff;
	dr->cf_reg = 0xffff;
}

void record_rom_access(uaecptr ptr, uae_u32 v, int size, bool rw)
{
	dma_record_init();
	if (!dma_record_data)
		return;
	struct dma_rec *dr = &dma_record_data[dma_record_cycle];
	dr->miscaddr = ptr;
	dr->miscval = v;
	dr->ciarw = rw;
	dr->miscsize = size;
}

void record_cia_access(int r, int mask, uae_u16 value, bool rw, int phase)
{
	dma_record_init();
	if (!dma_record_data)
		return;

	struct dma_rec *dr = &dma_record_data[dma_record_cycle];
	if (dr->ciaphase < 0) {
		return;
	}

	dr->ciamask = mask;
	dr->ciareg = r;
	dr->ciavalue = value;
	dr->ciarw = rw;
	dr->ciaphase = phase;
}

void record_dma_read(uae_u16 reg, uae_u32 addr, int type, int extra)
{
	dma_record_init();
	if (!dma_record_data)
		return;

	struct dma_rec *dr = &dma_record_data[dma_record_cycle];

	if (dr->reg != 0xffff) {
		if (dr->reg != reg) {
			dma_conflict(dr->vpos[0], dr->hpos, dr, reg, false);
			dr->cf_reg = reg;
			dr->cf_addr = addr;
		}
		return;
	}
	dr->reg = reg;
	dr->dat = 0;
	dr->addr = addr;
	dr->type = type;
	dr->extra = extra;
	dr->intlev = regs.intmask;
	dr->ipl = regs.ipl_pin;
	dr->end = false;

	last_dma_rec = dr;
	debug_mark_refreshed(dr->addr);
}

static bool get_record_dma_info(struct dma_rec *drs, struct dma_rec *dr, TCHAR *l1, TCHAR *l1b, TCHAR *l1c, TCHAR *l2, TCHAR *l3, TCHAR *l4, TCHAR *l5, TCHAR *l6, uae_u32 *split, int *iplp)
{
	int longsize = dr->size;
	bool got = false;
	int r = dr->reg;
	int regsize = 3;
	const TCHAR *sr;
	int br = dr->extra & 7;
	int chcnt = -1;
	TCHAR srtext[10];
	bool extra64 = false;
	uae_u32 extraval;
	bool noval = false;

	if (l1)
		l1[0] = 0;
	if (l1b)
		l1b[0] = 0;
	if (l1c)
		l1c[0] = 0;
	if (l2)
		l2[0] = 0;
	if (l3)
		l3[0] = 0;
	if (l4)
		l4[0] = 0;
	if (l5)
		l5[0] = 0;
	if (l6)
		l6[0] = 0;

	int hpos = dr->hpos;
	int dhpos0 = dr->dhpos[0];
	int dhpos1 = dr->dhpos[1];
	if (hpos < 0) {
		struct dma_rec *dr2 = dr;
		int cnt = 0;
		while (dr2->vpos[dma_record_vpos_type] == dr->vpos[dma_record_vpos_type]) {
			if (dr2 == drs) {
				hpos = addrdiff(dr, drs);
				break;
			}
			if (dr2->hpos >= 0) {
				hpos = dr2->hpos + cnt;
				break;
			}
			cnt++;
			dr2--;
		}
	}
	if (hpos < 0) {
		hpos = 0;
	}

	if (split) {
		if ((dr->evt & DMA_EVENT_CPUINS) && dr->evtdataset) {
			*split = dr->evtdata;
		}
	}

	if (dr->type != 0 || dr->reg != 0xffff || dr->evt)
		got = true;

	sr = _T("    ");
	if (dr->type == DMARECORD_COPPER) {
		if (br == 3)
			sr = _T("COP-S");
		else if (br == 2)
			sr = _T("COP-W");
		else if (br == 1)
			sr = _T("COP-M");
		else if (br == 4)
			sr = _T("COP-X");
		else if (br == 5)
			sr = _T("COP-1");
		else if (br == 6)
			sr = _T("COP-J");
		else if (br == 7)
			sr = _T("COP-D");
		else
			sr = _T("COP  ");
	} else if (dr->type == DMARECORD_BLITTER) {
		if (dr->extra & 0x20) {
			if (br == 0)
				sr = _T("BLL-A");
			if (br == 1)
				sr = _T("BLL-B");
			if (br == 2)
				sr = _T("BLL-C");
			if (br == 3)
				sr = _T("BLL-D");
		} else if (dr->extra & 0x10) {
			if (br == 0)
				sr = _T("BLF-A");
			if (br == 1)
				sr = _T("BLF-B");
			if (br == 2)
				sr = _T("BLF-C");
			if (br == 3)
				sr = _T("BLF-D");
		} else {
			if (br == 0)
				sr = _T("BLT-A");
			if (br == 1)
				sr = _T("BLT-B");
			if (br == 2)
				sr = _T("BLT-C");
			if (br == 3)
				sr = _T("BLT-D");
		}
		regsize = 2;
	} else if (dr->type == DMARECORD_REFRESH) {
		sr = _T("RFS");
		chcnt = br;
		noval = true;
	} else if (dr->type == DMARECORD_AUDIO) {
		sr = _T("AUD");
		chcnt = br;
	} else if (dr->type == DMARECORD_DISK) {
		sr = _T("DSK");
		chcnt = br;
	} else if (dr->type == DMARECORD_SPRITE) {
		sr = _T("SPR");
		chcnt = br;
	} else if (dr->type == DMARECORD_BITPLANE) {
		sr = _T("BPL");
		chcnt = br + 1;
	} else if (dr->type == DMARECORD_UHRESBPL) {
		sr = _T("UHB");
		chcnt = 0;
	} else if (dr->type == DMARECORD_UHRESSPR) {
		sr = _T("UHS");
		chcnt = 0;
	}
	if (dr->cf_reg != 0xffff) {
		_stprintf(srtext, _T("!%03x"), dr->cf_reg);
		chcnt = -1;
		regsize--;
	} else {
		_tcscpy(srtext, sr);
	}
	int ipl = 0;
	if (iplp) {
		ipl = *iplp;
		if (dr->ipl > 0) {
			ipl = dr->ipl;
		} else if (dr->ipl < 0) {
			ipl = 0;
		}
		*iplp = ipl;
		if (dr->ipl2 > 0) {
		}
	}
	if (ipl >= 0) {
		_sntprintf(l1, sizeof l1, _T("[%02X %03X/%03X %d]"), hpos, dhpos0, dhpos1, ipl);
	} else if (ipl == -2) {
		_sntprintf(l1, sizeof l1, _T("[%02X %03X/%03X -]"), hpos, dhpos0, dhpos1);
	} else {
		_sntprintf(l1, sizeof l1, _T("[%02X %03X/%03X  ]"), hpos, dhpos0, dhpos1);
	}
	if (l1c) {
		TCHAR *p = l1c;
		uae_u32 v = dr->agnus_evt;
		uae_u32 c = dr->agnus_evt_changed;
		if (c & AGNUS_EVENT_VDIW) {
			*p++ = 'W';
		} else if (v & AGNUS_EVENT_VDIW) {
			*p++ = 'w';
		} else {
			*p++ = '-';
		}
		if (c & AGNUS_EVENT_BPRUN2) {
			*p++ = 'D';
		} else if (v & AGNUS_EVENT_BPRUN2) {
			*p++ = 'd';
		} else {
			if (c & AGNUS_EVENT_BPRUN) {
				*p++ = 'B';
			} else if (v & AGNUS_EVENT_BPRUN) {
				*p++ = 'b';
			} else {
				*p++ = '-';
			}
		}
		if (c & AGNUS_EVENT_VE) {
			*p++ = 'E';
		} else if (v & AGNUS_EVENT_VE) {
			*p++ = 'e';
		} else {
			*p++ = '-';
		}
		if (c & AGNUS_EVENT_P_VE) {
			*p++ = 'E';
		} else if (v & AGNUS_EVENT_P_VE) {
			*p++ = 'e';
		} else {
			*p++ = '-';
		}
		*p++ = ' ';
		if (c & AGNUS_EVENT_HW_HS) {
			*p++ = 'H';
		} else if (v & AGNUS_EVENT_HW_HS) {
			*p++ = 'h';
		} else {
			*p++ = '-';
		}
		if (c & AGNUS_EVENT_HW_VS) {
			*p++ = 'V';
		} else if (v & AGNUS_EVENT_HW_VS) {
			*p++ = 'v';
		} else {
			*p++ = '-';
		}
		if (c & AGNUS_EVENT_HW_CS) {
			*p++ = 'C';
		} else if (v & AGNUS_EVENT_HW_CS) {
			*p++ = 'c';
		} else {
			*p++ = '-';
		}
		if (c & AGNUS_EVENT_PRG_HS) {
			*p++ = 'H';
		} else if (v & AGNUS_EVENT_PRG_HS) {
			*p++ = 'h';
		} else {
			*p++ = '-';
		}
		if (c & AGNUS_EVENT_PRG_VS) {
			*p++ = 'V';
		} else if (v & AGNUS_EVENT_PRG_VS) {
			*p++ = 'v';
		} else {
			*p++ = '-';
		}
		if (c & AGNUS_EVENT_PRG_CS) {
			*p++ = 'C';
		} else if (v & AGNUS_EVENT_PRG_CS) {
			*p++ = 'c';
		} else {
			*p++ = '-';
		}
		if (c & AGNUS_EVENT_HB) {
			*p++ = 'B';
		} else if (v & AGNUS_EVENT_HB) {
			*p++ = 'b';
		} else {
			*p++ = '-';
		}
		*p = 0;
	}
	if (l1b) {
		TCHAR *p = l1b;
		for (int h = 0; h < 2; h++) {
			uae_u32 v = dr->denise_evt[h];
			uae_u32 c = dr->denise_evt_changed[h];
			if (v & DENISE_EVENT_UNKNOWN) {
				*p++ = '?';
				*p++ = '?';
				*p++ = '?';
				*p++ = '?';
				*p++ = '?';
				*p++ = '?';
			} else {
				if (c & DENISE_EVENT_HB) {
					*p++ = 'H';
				} else if (v & DENISE_EVENT_HB) {
					*p++ = 'h';
				} else {
					*p++ = '-';
				}
				if (c & DENISE_EVENT_VB) {
					*p++ = 'V';
				} else if (v & DENISE_EVENT_VB) {
					*p++ = 'v';
				} else {
					*p++ = '-';
				}
				if (c & DENISE_EVENT_BURST) {
					*p++ = 'U';
				} else if (v & DENISE_EVENT_BURST) {
					*p++ = 'u';
				} else {
					*p++ = '-';
				}
				if (c & DENISE_EVENT_HDIW) {
					*p++ = 'W';
				} else if (v & DENISE_EVENT_HDIW) {
					*p++ = 'w';
				} else {
					*p++ = '-';
				}
				if (c & DENISE_EVENT_BPL1DAT_HDIW) {
					*p++ = 'B';
				} else if (v & DENISE_EVENT_BPL1DAT_HDIW) {
					*p++ = 'b';
				} else {
					*p++ = '-';
				}
			}
			*p++ = ' ';
		}
		*p = 0;
	}
	if (l4) {
		_tcscpy(l4, _T("              "));
	}
	if (l2) {
		_tcscpy(l2, _T("              "));
	}
	if (l3) {
		_tcscpy(l3, _T("              "));
	}
	if (r != 0xffff) {
		if (r & 0x1000) {
			if ((r & 0x0100) == 0x0000)
				_tcscpy(l2, _T("CPU-R  "));
			else if ((r & 0x0100) == 0x0100)
				_tcscpy(l2, _T("CPU-W  "));
			if ((r & 0xff) == 4) {
				l2[5] = 'L';
				longsize = 4;
			}
			if ((r & 0xff) == 2) {
				l2[5] = 'W';
			}
			if ((r & 0xff) == 1) {
				l2[5] = 'B';
			}
			if (br) {
				l2[6] = 'D';
			} else {
				l2[6] = 'I';
			}
		} else {
			if (chcnt >= 0) {
				if (regsize == 3)
					_sntprintf(l2, sizeof l2, _T("%3s%d   %03X"), srtext, chcnt, r);
				else if (regsize == 2)
					_sntprintf(l2, sizeof l2, _T("%4s%d   %02X"), srtext, chcnt, r);
				else
					_sntprintf(l2, sizeof l2, _T("%5s%d   %02X"), srtext, chcnt, r);
			} else {
				if (regsize == 3)
					_sntprintf(l2, sizeof l2, _T("%4s   %03X"), srtext, r);
				else if (regsize == 2)
					_sntprintf(l2, sizeof l2, _T("%5s   %02X"), srtext, r);
				else
					_sntprintf(l2, sizeof l2, _T("%6s   %02X"), srtext, r);
			}
		}
		if (l3 && !noval) {
			uae_u64 v = dr->dat;
			if (longsize == 4) {
				_sntprintf(l3, sizeof l3, _T("%08X"), (uae_u32)v);
			} else if (longsize == 8) {
				_sntprintf(l3, sizeof l3, _T("%08X"), (uae_u32)(v >> 32));
				extra64 = true;
				extraval = (uae_u32)v;
			} else {
				_sntprintf(l3, sizeof l3, _T("     %04X"), (uae_u32)(v & 0xffff));
			}
		}
		if (l4 && dr->addr != 0xffffffff)
			_sntprintf (l4, sizeof l4, _T("%08X"), dr->addr & 0x00ffffff);
	}
	if (l3) {
		int cl2 = 0;
		if (dr->evt & DMA_EVENT_BLITFINALD)
			l3[cl2++] = 'D';
		if (dr->evt & DMA_EVENT_BLITSTARTFINISH)
			l3[cl2++] = 'B';
		if (dr->evt & DMA_EVENT_CPUBLITTERSTEAL)
			l3[cl2++] = 's';
		if (dr->evt & DMA_EVENT_CPUBLITTERSTOLEN)
			l3[cl2++] = 'S';
		if (dr->evt & DMA_EVENT_BLITIRQ)
			l3[cl2++] = 'b';
		if (dr->evt & DMA_EVENT_BPLFETCHUPDATE)
			l3[cl2++] = 'p';
		if (dr->evt & (DMA_EVENT_COPPERWAKE | DMA_EVENT_COPPERSKIP))
			l3[cl2++] = 'W';
		if (dr->evt & DMA_EVENT_COPPERWAKE2) {
			l3[cl2++] = '#';
		} else if (dr->evt & DMA_EVENT_COPPERWANTED) {
			l3[cl2++] = 'c';
		}
		if (dr->evt & DMA_EVENT_CPUIRQ)
			l3[cl2++] = 'I';
		if (dr->evt & DMA_EVENT_CPUSTOP)
			l3[cl2++] = '|';
		if (dr->evt & DMA_EVENT_CPUSTOPIPL)
			l3[cl2++] = '+';
		if (dr->evt & DMA_EVENT_INTREQ)
			l3[cl2++] = 'i';
		if (dr->evt & DMA_EVENT_SPECIAL)
			l3[cl2++] = 'X';
		if (dr->evt & DMA_EVENT_DDFSTRT)
			l3[cl2++] = '0';
		if (dr->evt & DMA_EVENT_DDFSTOP)
			l3[cl2++] = '1';
		if (dr->evt & DMA_EVENT_DDFSTOP2)
			l3[cl2++] = '2';

		if (dr->evt & (DMA_EVENT_LOL | DMA_EVENT_LOF)) {
			l3[cl2++] = '*';
		}
		if (dr->evt & DMA_EVENT_LOL) {
			l3[cl2++] = 'L';
		}
		if (dr->evt & DMA_EVENT_LOF) {
			l3[cl2++] = 'F';
		}
		if (dr->evt & (DMA_EVENT_LOL | DMA_EVENT_LOF)) {
			l3[cl2++] = 0;
		}

		if (dr->evt & (DMA_EVENT_CIAA_IRQ | DMA_EVENT_CIAB_IRQ)) {
			l3[cl2++] = '#';
		}
		if (dr->evt & DMA_EVENT_CIAA_IRQ) {
			l3[cl2++] = 'A';
		}
		if (dr->evt & DMA_EVENT_CIAB_IRQ) {
			l3[cl2++] = 'B';
		}

		if (dr->evt & DMA_EVENT_IPLSAMPLE) {
			l3[cl2++] = '^';
		}
		if (dr->evt & DMA_EVENT_COPPERUSE) {
			l3[cl2++] = 'C';
		}
		if (dr->evt & DMA_EVENT_MODADD) {
			l3[cl2++] = 'M';
		}

	}
	if (l5) {
		if (dr->ciaphase) {
			if (dr->ciamask) {
				_sntprintf(l5, sizeof l5, _T("%c%s%X   %04X"), dr->ciarw ? 'W' : 'R',
					dr->ciamask == 1 ? _T("A") : (dr->ciamask == 2 ? _T("B") : _T("X")),
					dr->ciareg, dr->ciavalue);
			} else {
				int ph = dr->ciaphase;
				if (ph >= 100) {
					_tcscpy(l5, _T(" - "));
				} else {
					_sntprintf(l5, sizeof l5, _T(" %u "), ph - 1);
				}
			}
		} else if (dr->miscsize) {
			_stprintf(l5, _T("ROM%c%c %08X"), dr->ciarw ? 'W' : 'R', dr->miscsize == 1 ? 'B' : (dr->miscsize == 2 ? 'W' : 'L'), dr->miscaddr);
		}
	}
	if (l6) {
		TCHAR sync1 = ' ', sync2 = ' ';
		if (dr->hs && dr->vs) {
			sync1 = 'X';
		} else if (dr->hs) {
			sync1 = 'H';
		} else if (dr->vs) {
			sync1 = 'V';
		}
		if (dr->cs) {
			sync2 = 'C';
		}
		if (dr->addr != 0xffffffff) {
			int ras, cas;
			TCHAR xtra = ' ';
			bool ret = get_ras_cas(dr->addr, &ras, &cas);
			if (ret) {
				xtra = '+';
			}
			_sntprintf(l6, sizeof l6, _T("%c%c%c%03X %03X"), sync1, sync2, xtra, ras, cas);
		} else {
			_sntprintf(l6, sizeof l6, _T("%c%c        "), sync1, sync2);
		}
	}
	if (extra64) {
		_tcscpy(l6, l4);
		_sntprintf(l4, sizeof l4, _T("%08X"), extraval);
	}
	return got;
}


static struct dma_rec *find_dma_record(int hpos, int vpos, int toggle)
{
	int frame = vsync_counter - toggle;
	int found = -1;
	struct dma_rec *dr = NULL;

	if (!dma_record_data) {
		return NULL;
	}
	for (int i = 0; i < NR_DMA_REC_MAX; i++) {
		int idx = dma_record_cycle - i;
		if (idx < 0) {
			idx += NR_DMA_REC_MAX;
		}
		dr = &dma_record_data[idx];
		if (found < 0) {
			if (dr->frame == frame) {
				if ((dr->hpos == 2 || dr->hpos == hpos && hpos >= 2) && dr->vpos[dma_record_vpos_type] == vpos) {
					for (;;) {
						dr = &dma_record_data[idx];
						int tick = dr->tick;
						if (dr->vpos[dma_record_vpos_type] == vpos && dr->frame == frame) {
							if (dr->hpos == hpos) {
								break;
							}
						} else {
							idx++;
							break;
						}
						idx--;
						if (idx < 0) {
							idx += NR_DMA_REC_MAX;
						}
						dr = &dma_record_data[idx];
						if (dr->tick != tick - 1) {
							idx++;
							break;
						}
					}
					found = idx;
					break;
				}
			}
		}
	}
	if (found >= 0) {
		return dr;
	}
#if 0
	for (int i = 0; i < NR_DMA_REC_MAX; i++) {
		int idx = dma_record_cycle - i;
		if (idx < 0) {
			idx += NR_DMA_REC_MAX;
		}
		dr = &dma_record[idx];
		if (found < 0 && dr->hpos >= 0 && dr->vpos[dma_record_vpos_type] == vpos && dr->frame == frame) {
			found = idx;
			break;
		}
	}
	if (found >= 0) {
		int max = maxhpos;
		int idx = found;
		while (max-- > 0) {
			idx--;
			if (idx < 0) {
				idx += NR_DMA_REC_MAX;
			}
			dr = &dma_record[idx];
			if (dr->hpos == 1 || dr->hpos <= hpos) {
				return dr;
			}
		}
	}
#endif
	return NULL;
}

static void decode_dma_record(int hpos, int vpos, int count, int toggle, bool logfile)
{
	struct dma_rec *dr, *dr_start;
	int h, i, maxh = 0;
	int zerohpos = 0;
	int cols = logfile ? 16 : 8;

	if (!dma_record_data || hpos < 0 || vpos < 0)
		return;
	if (hpos == 0) {
		zerohpos = 1;
	}
	dr_start = find_dma_record(hpos + zerohpos, vpos, toggle);
	if (!dr_start) {
		return;
	}
	dr = dr_start;
	dr_start -= zerohpos;
	if (logfile)
		write_log (_T("Line: %03X/%03X (%3d/%3d) HPOS %02X (%3d):\n"), dr->vpos[0], dr->vpos[1], dr->vpos[0], dr->vpos[1], hpos, hpos);
	else
		console_out_f (_T("Line: %03X/%03X (%3d/%3d) HPOS %02X (%3d): **********************************************************************************************\n"),
			dr->vpos[0], dr->vpos[1], dr->vpos[0], dr->vpos[1], hpos, hpos);
	h = 0;
	dr = dr_start;
	while (maxh < 300) {
		if (dr - dma_record_data == dma_record_cycle) {
			break;
		}
		if (dr->hpos == 1 && maxh >= 4) {
			maxh++;
		}
		dr++;
		if (dr == dma_record_data + NR_DMA_REC_MAX) {
			dr = dma_record_data;
		}
		maxh++;
	}
	dr = dr_start;
	if (!logfile && maxh - h > 48) {
		int maxh2 = maxh;
		maxh = h + 48;
		if (maxh > maxh2) {
			maxh = maxh2;
		}
	}
	int ipl = -2;
	zerohpos = 0;
	bool quit = false;
	while (h < maxh && !quit) {
		TCHAR l1[400];
		TCHAR l1b[400];
		TCHAR l1c[400];
		TCHAR l2[400];
		TCHAR l3[400];
		TCHAR l4[400];
		TCHAR l5[400];
		TCHAR l6[400];
		l1[0] = 0;
		l1b[0] = 0;
		l1c[0] = 0;
		l2[0] = 0;
		l3[0] = 0;
		l4[0] = 0;
		l5[0] = 0;
		l6[0] = 0;

		for (i = 0; i < cols; i++, h++, dr++) {
			TCHAR l1l[30], l1bl[30], l1cl[30], l2l[30], l3l[30], l4l[30], l5l[30], l6l[30];
			uae_u32 split = 0xffffffff;

			get_record_dma_info(dr_start, dr, l1l, l1bl, l1cl, l2l, l3l, l4l, l5l, l6l, &split, &ipl);

			TCHAR *p = l1 + _tcslen(l1);
			_sntprintf(p, sizeof p,_T("%15s  "), l1l);
			p = l1b + _tcslen(l1b);
			_sntprintf(p, sizeof p,_T("%15s  "), l1bl);
			p = l1c + _tcslen(l1c);
			_sntprintf(p, sizeof p,_T("%15s  "), l1cl);
			p = l2 + _tcslen(l2);
			_sntprintf(p, sizeof p, _T("%15s  "), l2l);
			p = l3 + _tcslen(l3);
			_sntprintf(p, sizeof p, _T("%15s  "), l3l);
			p = l4 + _tcslen(l4);
			_sntprintf(p, sizeof p, _T("%15s  "), l4l);
			p = l5 + _tcslen(l5);
			_sntprintf(p, sizeof p, _T("%15s  "), l5l);
			p = l6 + _tcslen(l6);
			_sntprintf(p, sizeof p, _T("%15s  "), l6l);

			if (split != 0xffffffff) {
				if (split < 0x10000) {
					struct instr *dp = table68k + split;
					if (dp->mnemo == i_ILLG) {
						split = 0x4AFC;
						dp = table68k + split;
					}
					struct mnemolookup *lookup;
					for (lookup = lookuptab; lookup->mnemo != dp->mnemo; lookup++)
						;
					const TCHAR *opcodename = lookup->friendlyname;
					if (!opcodename) {
						opcodename = lookup->name;
					}
					TCHAR *ptrs[10];
					ptrs[0] = &l1[_tcslen(l1)];
					ptrs[1] = &l1b[_tcslen(l1b)];
					ptrs[2] = &l1c[_tcslen(l1c)];
					ptrs[3] = &l2[_tcslen(l2)];
					ptrs[4] = &l3[_tcslen(l3)];
					ptrs[5] = &l4[_tcslen(l4)];
					ptrs[6] = &l5[_tcslen(l5)];
					ptrs[7] = &l6[_tcslen(l6)];
					for (int i = 0; i < 8; i++) {
						if (!opcodename[i]) {
							break;
						}
						TCHAR *p = ptrs[i];
						p[-1] = opcodename[i];
					}
				} else {
					l1[_tcslen(l1) - 1] = '*';
				}
			}
			if (dr - dma_record_data == dma_record_cycle) {
				quit = true;
				break;
			}
			if (h > 4 && dr->hpos == 1) {
				zerohpos = 1;
			}
		}
		if (logfile) {
			write_log(_T("%s\n"), l1);
			write_log(_T("%s\n"), l1b);
			write_log(_T("%s\n"), l1c);
			write_log(_T("%s\n"), l2);
			write_log(_T("%s\n"), l3);
			write_log(_T("%s\n"), l4);
			write_log(_T("%s\n"), l5);
			write_log(_T("%s\n"), l6);
			write_log(_T("\n"));
		} else {
			console_out_f(_T("%s\n"), l1);
			console_out_f(_T("%s\n"), l1b);
			console_out_f(_T("%s\n"), l1c);
			console_out_f(_T("%s\n"), l2);
			console_out_f(_T("%s\n"), l3);
			console_out_f(_T("%s\n"), l4);
			console_out_f(_T("%s\n"), l5);
			console_out_f(_T("%s\n"), l6);
			console_out_f(_T("\n"));
		}
		if (zerohpos) {
			break;
		}
		if (count > 0) {
			count--;
			if (!count) {
				break;
			}
		}
	}
	if (logfile)
		flush_log();
}

void log_dma_record (void)
{
	if (!input_record && !input_play)
		return;
	if (!debug_dma)
		return;
	decode_dma_record (0, 0, 0, 0, true);
}

static void init_record_copper(void)
{
	if (!cop_record[0]) {
		cop_record[0] = xmalloc(struct cop_rec, NR_COPPER_RECORDS);
		cop_record[1] = xmalloc(struct cop_rec, NR_COPPER_RECORDS);
	}
}

void record_copper_blitwait (uaecptr addr, int hpos, int vpos)
{
	int t = nr_cop_records[curr_cop_set];
	init_record_copper();
	cop_record[curr_cop_set][t].bhpos = hpos;
	cop_record[curr_cop_set][t].bvpos = vpos;
}

void record_copper (uaecptr addr, uaecptr nextaddr, uae_u16 word1, uae_u16 word2, int hpos, int vpos)
{
	int t = nr_cop_records[curr_cop_set];
	init_record_copper();
	if (t < NR_COPPER_RECORDS) {
		cop_record[curr_cop_set][t].addr = addr;
		cop_record[curr_cop_set][t].nextaddr = nextaddr;
		cop_record[curr_cop_set][t].w1 = word1;
		cop_record[curr_cop_set][t].w2 = word2;
		cop_record[curr_cop_set][t].hpos = hpos;
		cop_record[curr_cop_set][t].vpos = vpos;
		cop_record[curr_cop_set][t].bvpos = -1;
		nr_cop_records[curr_cop_set] = t + 1;
	}
	if (debug_copper & 2) { /* trace */
		debug_copper &= ~2;
		activate_debugger_new();
	}
	if ((debug_copper & 4) && addr >= debug_copper_pc && addr <= debug_copper_pc + 3) {
		debug_copper &= ~4;
		activate_debugger_new();
	}
}

static struct cop_rec *find_copper_records(uaecptr addr)
{
	int s = selected_cop_set;
	int t = nr_cop_records[s];
	int i;
	for (i = 0; i < t; i++) {
		if (cop_record[s][i].addr == addr)
			return &cop_record[s][i];
	}
	return 0;
}

/* simple decode copper by Mark Cox */
static uaecptr decode_copper_insn(FILE *file, uae_u16 mword1, uae_u16 mword2, uaecptr addr)
{
	struct cop_rec *cr = NULL;
	uae_u32 insn_type, insn;
	TCHAR here = ' ';
	TCHAR record[] = _T("          ");

	if ((cr = find_copper_records(addr))) {
		_sntprintf(record, sizeof record, _T(" [%03x %03x]"), cr->vpos, cr->hpos);
		insn = (cr->w1 << 16) | cr->w2;
	} else {
		insn = (mword1 << 16) | mword2;
	}

	insn_type = insn & 0x00010001;

	if (get_copper_address(-1) >= addr && get_copper_address(-1) <= addr + 3)
		here = '*';

	console_out_f (_T("%c%08x: %04x %04x%s\t;%c "), here, addr, insn >> 16, insn & 0xFFFF, record, insn != ((mword1 << 16) | mword2) ? '!' : ' ');

	switch (insn_type) {
	case 0x00010000: /* WAIT insn */
		console_out(_T("Wait for "));
		disassemble_wait(file, insn);

		if (insn == 0xfffffffe)
			console_out(_T("                           \t;  End of Copperlist\n"));

		break;

	case 0x00010001: /* SKIP insn */
		console_out(_T("Skip if "));
		disassemble_wait(file, insn);
		break;

	case 0x00000000:
	case 0x00000001: /* MOVE insn */
		{
			int addr = (insn >> 16) & 0x1fe;
			int i = 0;
			while (custd[i].name) {
				if (custd[i].adr == addr + 0xdff000)
					break;
				i++;
			}
			if (custd[i].name && custd[i].name[0] && _tcscmp(custd[i].name, _T("-"))) {
				console_out_f(_T("%s := 0x%04x\n"), custd[i].name, insn & 0xffff);
			} else {
				console_out_f(_T("%04x := 0x%04x\n"), addr, insn & 0xffff);
			}
		}
		break;

	default:
		abort ();
	}

	if (cr && cr->bvpos >= 0) {
		console_out_f(_T("                 BLT [%03x %03x]\n"), cr->bvpos, cr->bhpos);
	}
	if (cr && cr->nextaddr != 0xffffffff && cr->nextaddr != addr + 4) {
		console_out_f(_T(" %08x: Copper jump\n"), cr->nextaddr);
		return cr->nextaddr;
	}
	return addr + 4;
}

static uaecptr decode_copperlist(FILE *file, uaecptr address, int nolines)
{
	uaecptr next;
	while (nolines-- > 0) {
		next = decode_copper_insn(file, chipmem_wget_indirect(address), chipmem_wget_indirect(address + 2), address);
		address = next;
	}
	return address;
	/* You may wonder why I don't stop this at the end of the copperlist?
	* Well, often nice things are hidden at the end and it is debatable the actual
	* values that mean the end of the copperlist */
}

static int copper_debugger (TCHAR **c)
{
	static uaecptr nxcopper;
	uae_u32 maddr;
	int lines;

	if (**c == 'd') {
		next_char (c);
		if (debug_copper)
			debug_copper = 0;
		else
			debug_copper = 1;
		console_out_f (_T("Copper debugger %s.\n"), debug_copper ? _T("enabled") : _T("disabled"));
	} else if (**c == 't') {
		debug_copper = 1|2;
		return 1;
	} else if (**c == 'b') {
		(*c)++;
		debug_copper = 1|4;
		if (more_params (c)) {
			debug_copper_pc = readhex(c, NULL);
			console_out_f (_T("Copper breakpoint @0x%08x\n"), debug_copper_pc);
		} else {
			debug_copper &= ~4;
		}
	} else {
		if (more_params(c)) {
			maddr = readhex(c, NULL);
			if (maddr == 1 || maddr == 2 || maddr == 3)
				maddr = get_copper_address(maddr);
			else if (maddr == 0)
				maddr = get_copper_address(-1);
		} else {
			maddr = nxcopper;
		}
		selected_cop_set = curr_cop_set;
		if (!find_copper_records(maddr)) {
			selected_cop_set = curr_cop_set ^ 1;
		}

		if (more_params(c))
			lines = readhex(c, NULL);
		else
			lines = 20;

		nxcopper = decode_copperlist (stdout, maddr, lines);
	}
	return 0;
}

#define MAX_CHEAT_VIEW 100
struct trainerstruct {
	uaecptr addr;
	int size;
};

static struct trainerstruct *trainerdata;
static int totaltrainers;

static void clearcheater(void)
{
	if (!trainerdata)
		trainerdata = xmalloc(struct trainerstruct, MAX_CHEAT_VIEW);
	memset(trainerdata, 0, sizeof (struct trainerstruct) * MAX_CHEAT_VIEW);
	totaltrainers = 0;
}
static int addcheater(uaecptr addr, int size)
{
	if (totaltrainers >= MAX_CHEAT_VIEW)
		return 0;
	trainerdata[totaltrainers].addr = addr;
	trainerdata[totaltrainers].size = size;
	totaltrainers++;
	return 1;
}
static void listcheater(int mode, int size)
{
	int i, skip;

	if (!trainerdata)
		return;
	if (mode)
		skip = 4;
	else
		skip = 8;
	for(i = 0; i < totaltrainers; i++) {
		struct trainerstruct *ts = &trainerdata[i];
		uae_u16 b;

		if (size) {
			b = get_byte_debug(ts->addr);
		} else {
			b = get_word_debug(ts->addr);
		}
		if (mode)
			console_out_f(_T("%08X=%04X "), ts->addr, b);
		else
			console_out_f(_T("%08X "), ts->addr);
		if ((i % skip) == skip)
			console_out(_T("\n"));
	}
}

static void deepcheatsearch(TCHAR **c)
{
	static int first = 1;
	static uae_u8 *memtmp;
	static int memsize, memsize2;
	uae_u8 *p1, *p2;
	uaecptr addr, end;
	int i, wasmodified, nonmodified;
	static int size;
	static int inconly, deconly, maxdiff;
	int addrcnt, cnt;
	TCHAR v;

	v = _totupper(**c);

	if(!memtmp || v == 'S') {
		maxdiff = 0x10000;
		inconly = 0;
		deconly = 0;
		size = 1;
	}

	if (**c)
		(*c)++;
	ignore_ws(c);
	if ((**c) == '1' || (**c) == '2') {
		size = **c - '0';
		(*c)++;
	}
	if (more_params(c))
		maxdiff = readint(c, NULL);

	if (!memtmp || v == 'S') {
		first = 1;
		xfree (memtmp);
		memsize = 0;
		addr = 0xffffffff;
		nextaddr_init(addr);
		while ((addr = nextaddr(addr, 0, &end, false, NULL)) != 0xffffffff)  {
			memsize += end - addr;
			addr = end - 1;
		}
		memsize2 = (memsize + 7) / 8;
		memtmp = xmalloc(uae_u8, memsize + memsize2);
		if (!memtmp)
			return;
		memset(memtmp + memsize, 0xff, memsize2);
		p1 = memtmp;
		addr = 0xffffffff;
		nextaddr_init(addr);
		while ((addr = nextaddr(addr, 0, &end, true, NULL)) != 0xffffffff) {
			for (i = addr; i < end; i++)
				*p1++ = get_byte_debug(i);
			addr = end - 1;
		}
		console_out(_T("Deep trainer first pass complete.\n"));
		return;
	}
	inconly = deconly = 0;
	wasmodified = v == 'X' ? 0 : 1;
	nonmodified = v == 'Z' ? 1 : 0;
	if (v == 'I')
		inconly = 1;
	if (v == 'D')
		deconly = 1;
	p1 = memtmp;
	p2 = memtmp + memsize;
	addrcnt = 0;
	cnt = 0;
	addr = 0xffffffff;
	nextaddr_init(addr);
	while ((addr = nextaddr(addr, 0, NULL, true, NULL)) != 0xffffffff) {
		uae_s32 b, b2;
		int doremove = 0;
		int addroff;
		int addrmask ;

		if (size == 1) {
			b = (uae_s8)get_byte_debug(addr);
			b2 = (uae_s8)p1[addrcnt];
			addroff = addrcnt >> 3;
			addrmask = 1 << (addrcnt & 7);
		} else {
			b = (uae_s16)get_word_debug(addr);
			b2 = (uae_s16)((p1[addrcnt] << 8) | p1[addrcnt + 1]);
			addroff = addrcnt >> 2;
			addrmask = 3 << (addrcnt & 3);
		}

		if (p2[addroff] & addrmask) {
			if (wasmodified && !nonmodified) {
				int diff = b - b2;
				if (b == b2)
					doremove = 1;
				if (abs(diff) > maxdiff)
					doremove = 1;
				if (inconly && diff < 0)
					doremove = 1;
				if (deconly && diff > 0)
					doremove = 1;
			} else if (nonmodified && b == b2) {
				doremove = 1;
			} else if (!wasmodified && b != b2) {
				doremove = 1;
			}
			if (doremove)
				p2[addroff] &= ~addrmask;
			else
				cnt++;
		}
		if (size == 1) {
			p1[addrcnt] = b;
			addrcnt++;
		} else {
			p1[addrcnt] = b >> 8;
			p1[addrcnt + 1] = b >> 0;
			addr = nextaddr(addr, 0, NULL, true, NULL);
			if (addr == 0xffffffff)
				break;
			addrcnt++;
		}
		if  (iscancel(65536)) {
			console_out_f(_T("Aborted at %08X\n"), addr);
			break;
		}
	}

	console_out_f(_T("%d addresses found\n"), cnt);
	if (cnt <= MAX_CHEAT_VIEW) {
		clearcheater();
		cnt = 0;
		addrcnt = 0;
		addr = 0xffffffff;
		while ((addr = nextaddr(addr, 0, NULL, true, NULL)) != 0xffffffff) {
			int addroff = addrcnt >> (size == 1 ? 3 : 2);
			int addrmask = (size == 1 ? 1 : 3) << (addrcnt & (size == 1 ? 7 : 3));
			if (p2[addroff] & addrmask)
				addcheater(addr, size);
			if (size == 2) {
				addr = nextaddr(addr, 0, NULL, true, NULL);
				if (addr == 0xffffffff) {
					break;
				}
			}
			addrcnt++;
			cnt++;
		}
		if (cnt > 0)
			console_out(_T("\n"));
		listcheater(1, size);
	} else {
		console_out(_T("Now continue with 'g' and use 'D' again after you have lost another life\n"));
	}
}

/* cheat-search by Toni Wilen (originally by Holger Jakob) */
static void cheatsearch (TCHAR **c)
{
	static uae_u8 *vlist;
	static int listsize;
	static int first = 1;
	static int size = 1;
	uae_u32 val, memcnt, prevmemcnt;
	int i, count, vcnt, memsize;
	uaecptr addr, end;
	bool err;

	memsize = 0;
	addr = 0xffffffff;
	nextaddr_init(addr);
	while ((addr = nextaddr(addr, 0, &end, false, NULL)) != 0xffffffff)  {
		memsize += end - addr;
		addr = end - 1;
	}

	if (_totupper (**c) == 'L') {
		listcheater (1, size);
		return;
	}
	ignore_ws (c);
	if (!more_params (c)) {
		first = 1;
		console_out (_T("Search reset\n"));
		xfree (vlist);
		listsize = memsize;
		vlist = xcalloc (uae_u8, listsize >> 3);
		return;
	}
	if (first)
		val = readint(c, &size, &err);
	else
		val = readint(c, &err);
	if (err) {
		return;
	}

	if (vlist == NULL) {
		listsize = memsize;
		vlist = xcalloc (uae_u8, listsize >> 3);
	}

	count = 0;
	vcnt = 0;

	clearcheater ();
	addr = 0xffffffff;
	nextaddr_init(addr);
	prevmemcnt = memcnt = 0;
	while ((addr = nextaddr(addr, 0, &end, true, NULL)) != 0xffffffff) {
		if (addr + size < end) {
			for (i = 0; i < size; i++) {
				int shift = (size - i - 1) * 8;
				if (get_byte_debug (addr + i) != ((val >> shift) & 0xff))
					break;
			}
			if (i == size) {
				int voffset = memcnt >> 3;
				int vmask = 1 << (memcnt & 7);
				if (!first) {
					while (prevmemcnt < memcnt) {
						vlist[prevmemcnt >> 3] &= ~(1 << (prevmemcnt & 7));
						prevmemcnt++;
					}
					if (vlist[voffset] & vmask) {
						count++;
						addcheater(addr, size);
					} else {
						vlist[voffset] &= ~vmask;
					}
					prevmemcnt = memcnt + 1;
				} else {
					vlist[voffset] |= vmask;
					count++;
				}
			}
		}
		memcnt++;
		if  (iscancel (65536)) {
			console_out_f (_T("Aborted at %08X\n"), addr);
			break;
		}
	}
	if (!first) {
		while (prevmemcnt < memcnt) {
			vlist[prevmemcnt >> 3] &= ~(1 << (prevmemcnt & 7));
			prevmemcnt++;
		}
		listcheater (0, size);
	}
	console_out_f (_T("Found %d possible addresses with 0x%X (%u) (%d bytes)\n"), count, val, val, size);
	if (count > 0)
		console_out (_T("Now continue with 'g' and use 'C' with a different value\n"));
	first = 0;
}

struct breakpoint_node bpnodes[BREAKPOINT_TOTAL];
static addrbank **debug_mem_banks;
static addrbank *debug_mem_area;
struct memwatch_node mwnodes[MEMWATCH_TOTAL];
static int mwnodes_start, mwnodes_end;
static struct memwatch_node mwhit;

#define MUNGWALL_SLOTS 16
struct mungwall_data
{
	int slots;
	uae_u32 start[MUNGWALL_SLOTS], end[MUNGWALL_SLOTS];
};
static struct mungwall_data **mungwall;

static uae_u8 *illgdebug, *illghdebug;
static int illgdebug_break;

static void illg_free (void)
{
	xfree (illgdebug);
	illgdebug = NULL;
	xfree (illghdebug);
	illghdebug = NULL;
}

static void illg_init (void)
{
	int i;
	uae_u8 c = 3;
	uaecptr addr, end;

	illgdebug = xcalloc (uae_u8, 0x01000000);
	illghdebug = xcalloc (uae_u8, 65536);
	if (!illgdebug || !illghdebug) {
		illg_free();
		return;
	}
	addr = 0xffffffff;
	nextaddr_init(addr);
	while ((addr = nextaddr(addr, 0, &end, false, NULL)) != 0xffffffff)  {
		if (end < 0x01000000) {
			memset (illgdebug + addr, c, end - addr);
		} else {
			uae_u32 s = addr >> 16;
			uae_u32 e = end >> 16;
			memset (illghdebug + s, c, e - s);
		}
		addr = end - 1;
	}
	for (int i = 0; i < MAX_RTG_BOARDS; i++) {
		if (currprefs.rtgboards[i].rtgmem_size)
			memset (illghdebug + (gfxmem_banks[i]->start >> 16), 3, currprefs.rtgboards[i].rtgmem_size >> 16);
	}

	i = 0;
	while (custd[i].name) {
		int rw = (custd[i].special & CD_WO) ? 2 : 1;
		illgdebug[custd[i].adr] = rw;
		illgdebug[custd[i].adr + 1] = rw;
		i++;
	}
	for (i = 0; i < 16; i++) { /* CIAs */
		if (i == 11)
			continue;
		illgdebug[0xbfe001 + i * 0x100] = c;
		illgdebug[0xbfd000 + i * 0x100] = c;
	}
	memset (illgdebug + 0xf80000, 1, 512 * 1024); /* KS ROM */
	memset (illgdebug + 0xdc0000, c, 0x3f); /* clock */
#ifdef CDTV
	if (currprefs.cs_cdtvram) {
		memset (illgdebug + 0xdc8000, c, 4096); /* CDTV batt RAM */
		memset (illgdebug + 0xf00000, 1, 256 * 1024); /* CDTV ext ROM */
	}
#endif
#ifdef CD32
	if (currprefs.cs_cd32cd) {
		memset (illgdebug + AKIKO_BASE, c, AKIKO_BASE_END - AKIKO_BASE);
		memset (illgdebug + 0xe00000, 1, 512 * 1024); /* CD32 ext ROM */
	}
#endif
	if (currprefs.cs_ksmirror_e0)
		memset (illgdebug + 0xe00000, 1, 512 * 1024);
	if (currprefs.cs_ksmirror_a8)
		memset (illgdebug + 0xa80000, 1, 2 * 512 * 1024);
#ifdef FILESYS
	if (uae_boot_rom_type) /* filesys "rom" */
		memset (illgdebug + rtarea_base, 1, 0x10000);
#endif
	if (currprefs.cs_ide > 0)
		memset (illgdebug + 0xdd0000, 3, 65536);
}

/* add special custom register check here */
static void illg_debug_check (uaecptr addr, int rwi, int size, uae_u32 val)
{
	return;
}

static void illg_debug_do (uaecptr addr, int rwi, int size, uae_u32 val)
{
	uae_u8 mask;
	uae_u32 pc = m68k_getpc ();
	int i;

	for (i = size - 1; i >= 0; i--) {
		uae_u8 v = val >> (i * 8);
		uae_u32 ad = addr + i;
		if (ad >= 0x01000000)
			mask = illghdebug[ad >> 16];
		else
			mask = illgdebug[ad];
		if ((mask & 3) == 3)
			return;
		if (mask & 0x80) {
			illg_debug_check (ad, rwi, size, val);
		} else if ((mask & 3) == 0) {
			if (rwi & 2)
				console_out_f (_T("W: %08X=%02X PC=%08X\n"), ad, v, pc);
			else if (rwi & 1)
				console_out_f (_T("R: %08X    PC=%08X\n"), ad, pc);
			if (illgdebug_break)
				activate_debugger_new();
		} else if (!(mask & 1) && (rwi & 1)) {
			console_out_f (_T("RO: %08X=%02X PC=%08X\n"), ad, v, pc);
			if (illgdebug_break)
				activate_debugger_new();
		} else if (!(mask & 2) && (rwi & 2)) {
			console_out_f (_T("WO: %08X    PC=%08X\n"), ad, pc);
			if (illgdebug_break)
				activate_debugger_new();
		}
	}
}

static int debug_mem_off (uaecptr *addrp)
{
	uaecptr addr = *addrp;
	addrbank *ba;
	int offset = munge24 (addr) >> 16;
	if (!debug_mem_banks)
		return offset;
	ba = debug_mem_banks[offset];
	if (!ba)
		return offset;
	if (ba->mask || ba->startmask) {
		uae_u32 start = ba->startmask ? ba->startmask : ba->start;
		addr -= start;
		if (ba->mask) {
			addr &= ba->mask;
		}
		addr += start;
	}
	*addrp = addr;
	return offset;
}

struct smc_item {
	uae_u32 addr;
	uae_u16 version;
	uae_u8 cnt;
};

static uae_u32 smc_size, smc_mode;
static struct smc_item *smc_table;
static uae_u16 smc_version;

static void smc_free (void)
{
	if (smc_table)
		console_out (_T("SMCD disabled\n"));
	xfree(smc_table);
	smc_mode = 0;
	smc_table = NULL;
}

static void initialize_memwatch(int mode);
static void smc_detect_init(TCHAR **c)
{
	int v;

	ignore_ws(c);
	v = readint(c, NULL);
	smc_free();
	smc_size = 1 << 24;
	if (highest_ram > smc_size) {
		smc_size = highest_ram;
	}
	smc_table = xcalloc(struct smc_item, smc_size + 4);
	if (!smc_table) {
		console_out_f(_T("Failed to allocated SMCD buffers, %d bytes needed\n."), sizeof(struct smc_item) * smc_size + 4);
		return;
	}
	smc_version = 0xffff;
	debug_smc_clear(-1, 0);
	if (!memwatch_enabled)
		initialize_memwatch(0);
	if (v)
		smc_mode = 1;
	console_out_f(_T("SMCD enabled. Break=%d. Last address=%08x\n"), smc_mode, smc_size);
}

void debug_smc_clear(uaecptr addr, int size)
{
	if (!smc_table)
		return;
	smc_version++;
	if (smc_version == 0) {
		for (uae_u32 i = 0; i < smc_size; i += 65536) {
			addrbank *ab = &get_mem_bank(i);
			if (ab->flags & (ABFLAG_RAM | ABFLAG_ROM)) {
				for (uae_u32 j = 0; j < 65536; j++) {
					struct smc_item *si = &smc_table[i + j];
					if (size < 0 || (si->addr >= addr && si->addr < addr + size)) {
						si->addr = 0xffffffff;
						si->cnt = 0;
						si->version = smc_version;
					}
				}
			}
		}
	}
}

#define SMC_MAXHITS 8
static void smc_detector(uaecptr addr, int rwi, int size, uae_u32 *valp)
{
	int hitcnt;
	uaecptr hitaddr, hitpc;

	if (!smc_table)
		return;
	if (addr + size > smc_size)
		return;

	if (rwi == 2) {
		for (int i = 0; i < size; i++) {
			struct smc_item *si = &smc_table[addr + i];
			if (si->version != smc_version) {
				si->version = smc_version;
				si->addr = 0xffffffff;
				si->cnt = 0;
			}	
			if (si->cnt < SMC_MAXHITS) {
				si->addr = m68k_getpc();
			}
		}
		return;
	}
	hitpc = 0xffffffff;
	for (int i = 0; i < size && hitpc == 0xffffffff && addr + i < smc_size; i += 2) {
		struct smc_item *si = &smc_table[addr + i];
		if (si->version == smc_version) {
			hitpc = si->addr;
		}
	}
	if (hitpc == 0xffffffff) {
		return;
	}
	if ((hitpc & 0xFFF80000) == 0xF80000) {
		return;
	}
	hitaddr = addr;
	hitcnt = 0;
	while (addr < smc_size) {
		struct smc_item *si = &smc_table[addr];
		if (si->addr == 0xffffffff || si->version != smc_version) {
			break;
		}
		si->addr = 0xffffffff;
		hitcnt++;
		addr++;
	}
	if (currprefs.cpu_model <= 68010 && currprefs.cpu_compatible) {
		/* ignore single-word unconditional jump instructions
		* (instruction prefetch from PC+2 can cause false positives) */
		if (regs.irc == 0x4e75 || regs.irc == 0x4e74 || regs.irc == 0x4e73 || regs.irc == 0x4e77)
			return; /* RTS, RTD, RTE, RTR */
		if ((regs.irc & 0xff00) == 0x6000 && (regs.irc & 0x00ff) != 0 && (regs.irc & 0x00ff) != 0xff)
			return; /* BRA.B */
	}
	if (hitcnt < 100) {
		struct smc_item *si = &smc_table[hitaddr];
		si->cnt++;
		console_out_f(_T("SMC at %08X - %08X (%d) from %08X\n"), hitaddr, hitaddr + hitcnt, hitcnt, hitpc);
		if (smc_mode) {
			activate_debugger_new();
		}
		if (si->cnt >= SMC_MAXHITS) {
			console_out_f(_T("* hit count >= %d, future hits ignored\n"), SMC_MAXHITS);
		}
	}
}

uae_u8 *save_debug_memwatch (size_t *len, uae_u8 *dstptr)
{
	uae_u8 *dstbak, *dst;
	int total;

	total = 0;
	for (int i = 0; i < MEMWATCH_TOTAL; i++) {
		if (mwnodes[i].size > 0)
			total++;
	}
	if (!total)
		return NULL;

	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc (uae_u8, 1000);
	save_u32 (1);
	save_u8 (total);
	for (int i = 0; i < MEMWATCH_TOTAL; i++) {
		struct memwatch_node *m = &mwnodes[i];
		if (m->size <= 0)
			continue;
		save_store_pos ();
		save_u8 (i);
		save_u8 (m->modval_written);
		save_u8 (m->mustchange);
		save_u8 (m->frozen);
		save_u8 (m->val_enabled);
		save_u8 (m->rwi);
		save_u32 (m->addr);
		save_u32 (m->size);
		save_u32 (m->modval);
		save_u32 (m->val_mask);
		save_u32 (m->val_size);
		save_u32 (m->val);
		save_u32 (m->pc);
		save_u32 (m->access_mask);
		save_u32 (m->reg);
		save_u8(m->nobreak);
		save_u8(m->reportonly);
		save_store_size ();
	}
	*len = dst - dstbak;
	return dstbak;
}

uae_u8 *restore_debug_memwatch (uae_u8 *src)
{
	if (restore_u32 () != 1)
		return src;
	int total = restore_u8 ();
	for (int i = 0; i < total; i++) {
		restore_store_pos ();
		int idx = restore_u8 ();
		struct memwatch_node *m = &mwnodes[idx];
		m->modval_written = restore_u8 ();
		m->mustchange = restore_u8 ();
		m->frozen = restore_u8 ();
		m->val_enabled = restore_u8 ();
		m->rwi = restore_u8 ();
		m->addr = restore_u32 ();
		m->size = restore_u32 ();
		m->modval = restore_u32 ();
		m->val_mask = restore_u32 ();
		m->val_size = restore_u32 ();
		m->val = restore_u32 ();
		m->pc = restore_u32 ();
		m->access_mask = restore_u32();
		m->reg = restore_u32();
		m->nobreak = restore_u8();
		m->reportonly = restore_u8();
		restore_store_size ();
	}
	return src;
}

void restore_debug_memwatch_finish (void)
{
	for (int i = 0; i < MEMWATCH_TOTAL; i++) {
		struct memwatch_node *m = &mwnodes[i];
		if (m->size) {
			if (!memwatch_enabled)
				initialize_memwatch (0);
			return;
		}
	}
}

void debug_check_reg(uae_u32 addr, int write, uae_u16 v)
{
	if (!memwatch_access_validator)
		return;
	int reg = addr & 0x1ff;
	const struct customData *cd = &custd[reg >> 1];

	if (((addr & 0xfe00) != 0xf000 && (addr & 0xffff0000) != 0) || ((addr & 0xffff0000) != 0 && (addr & 0xffff0000) != 0x00df0000) || (addr & 0x0600)) {
		write_log(_T("Mirror custom register %08x (%s) %s access. PC=%08x\n"), addr, cd->name, write ? _T("write") : _T("read"), M68K_GETPC);
	}

	int spc = cd->special;
	if ((spc & CD_AGA) && !(currprefs.chipset_mask & CSMASK_AGA))
		spc |= CD_NONE;
	if ((spc & CD_ECS_DENISE) && !(currprefs.chipset_mask & CSMASK_ECS_DENISE))
		spc |= CD_NONE;
	if ((spc & CD_ECS_AGNUS) && !(currprefs.chipset_mask & CSMASK_ECS_AGNUS))
		spc |= CD_NONE;
	if (spc & CD_NONE) {
		write_log(_T("Non-existing custom register %04x (%s) %s access. PC=%08x\n"), reg, cd->name, write ? _T("write") : _T("read"), M68K_GETPC);
		return;
	}

	if (spc & CD_COLOR) {
		if (currprefs.chipset_mask & CSMASK_AGA)
			return;
	}

	if (write & !(spc & CD_WO)) {
		write_log(_T("Write access to read-only custom register %04x (%s). PC=%08x\n"), reg, cd->name, M68K_GETPC);
		return;
	} else if (!write && (spc & CD_WO)) {
		write_log(_T("Read access from write-only custom register %04x (%s). PC=%08x\n"), reg, cd->name, M68K_GETPC);
		return;
	}

	if (write && cd->mask[2]) {
		int idx = (currprefs.chipset_mask & CSMASK_AGA) ? 2 : (currprefs.chipset_mask & CSMASK_ECS_AGNUS) ? 1 : 0;
		uae_u16 mask = cd->mask[idx];
		if (v & ~mask) {
			write_log(_T("Unuset bits set %04x when writing custom register %04x (%s) PC=%08x\n"), v & ~mask, reg, cd->name, M68K_GETPC);
		}
	}

	if (spc & CD_DMA_PTR) {
		uae_u32 addr = (custom_storage[((reg & ~2) >> 1)].value << 16) | custom_storage[((reg | 2) >> 1)].value;
		if (currprefs.z3chipmem.size) {
			if (addr >= currprefs.z3chipmem.start_address && addr < currprefs.z3chipmem.start_address + currprefs.z3chipmem.size)
				return;
		}
		if(addr >= currprefs.chipmem.size)
			write_log(_T("DMA pointer %04x (%s) set to invalid value %08x %s=%08x\n"), reg, cd->name, addr,
				custom_storage[reg >> 1].pc & 1 ? _T("COP") : _T("PC"), custom_storage[reg >> 1].pc);
	}
}

void debug_invalid_reg(int reg, int size, uae_u16 v)
{
	if (!memwatch_access_validator)
		return;
	reg &= 0x1ff;
	if (size == 1) {
		if (reg == 2) // DMACONR low byte
			return;
		if (reg == 6) // VPOS
			return;
	}
	const struct customData *cd = &custd[reg >> 1];
	if (size == -2 && (reg & 1)) {
		write_log(_T("Unaligned word write to register %04x (%s) val %04x PC=%08x\n"), reg, cd->name, v, M68K_GETPC);
	} else if (size == -1) {
		write_log(_T("Byte write to register %04x (%s) val %02x PC=%08x\n"), reg, cd->name, v & 0xff, M68K_GETPC);
	} else if (size == 2 && (reg & 1)) {
		write_log(_T("Unaligned word read from register %04x (%s) PC=%08x\n"), reg, cd->name, M68K_GETPC);
	} else if (size == 1) {
		write_log(_T("Byte read from register %04x (%s) PC=%08x\n"), reg, cd->name, M68K_GETPC);
	}
}

static void is_valid_dma(int reg, int ptrreg, uaecptr addr)
{
	if (!memwatch_access_validator)
		return;
	if (reg == 0x1fe) // refresh
		return;
	if (currprefs.z3chipmem.size) {
		if (addr >= currprefs.z3chipmem.start_address && addr < currprefs.z3chipmem.start_address + currprefs.z3chipmem.size)
			return;
	}
	if (!(addr & ~(currprefs.chipmem.size - 1)))
		return;
	const struct customData *cdreg = &custd[reg >> 1];
	const struct customData *cdptr = &custd[ptrreg >> 1];
	write_log(_T("DMA DAT %04x (%s), PT %04x (%s) accessed invalid memory %08x. Init: %08x, PC/COP=%08x\n"),
		reg, cdreg->name, ptrreg, cdptr->name, addr,
		(custom_storage[ptrreg >> 1].value << 16) | (custom_storage[(ptrreg >> 1) + 1].value),
		custom_storage[ptrreg >> 1].pc);
}

static void mungwall_memwatch(uaecptr addr, int rwi, int size, uae_u32 valp)
{
	struct mungwall_data *mwd = mungwall[addr >> 16];
	if (!mwd)
		return;
	for (int i = 0; i < mwd->slots; i++) {
		if (!mwd->end[i])
			continue;
		if (addr + size > mwd->start[i] && addr < mwd->end[i]) {

		}
	}
}

static void memwatch_hit_msg(int mw)
{
	console_out_f(_T("Memwatch %d: break at %08X.%c %c%c%c %08X PC=%08X "), mw, mwhit.addr,
		mwhit.size == 1 ? 'B' : (mwhit.size == 2 ? 'W' : 'L'),
		(mwhit.rwi & 1) ? 'R' : ' ', (mwhit.rwi & 2) ? 'W' : ' ', (mwhit.rwi & 4) ? 'I' : ' ',
		mwhit.val, mwhit.pc);
	for (int i = 0; memwatch_access_masks[i].mask; i++) {
		if (mwhit.access_mask == memwatch_access_masks[i].mask)
			console_out_f(_T("%s (%03x)\n"), memwatch_access_masks[i].name, mwhit.reg);
	}
	if (mwhit.access_mask & (MW_MASK_BLITTER_A | MW_MASK_BLITTER_B | MW_MASK_BLITTER_C | MW_MASK_BLITTER_D_N | MW_MASK_BLITTER_D_L | MW_MASK_BLITTER_D_F)) {
		blitter_debugdump();
	}
}

static int memwatch_func (uaecptr addr, int rwi, int size, uae_u32 *valp, uae_u32 accessmask, uae_u32 reg)
{
	uae_u32 val = *valp;

	if (inside_debugger)
		return 1;

	if (mungwall)
		mungwall_memwatch(addr, rwi, size, val);

	if (illgdebug)
		illg_debug_do (addr, rwi, size, val);

	if (heatmap)
		memwatch_heatmap (addr, rwi, size, accessmask);

	addr = munge24 (addr);

	if (smc_table && (rwi >= 2))
		smc_detector (addr, rwi, size, valp);

	for (int i = mwnodes_start; i <= mwnodes_end; i++) {
		struct memwatch_node *m = &mwnodes[i];
		uaecptr addr2 = m->addr;
		uaecptr addr3 = addr2 + m->size;
		int rwi2 = m->rwi;
		uae_u32 oldval = 0;
		int isoldval = 0;
		int brk = 0;
		uae_u32 newval = 0;

		if (m->size == 0)
			continue;
		if (!(rwi & rwi2))
			continue;
		if (!(m->access_mask & accessmask))
			continue;

		if (addr >= addr2 && addr < addr3)
			brk = 1;
		if (!brk && size == 2 && (addr + 1 >= addr2 && addr + 1 < addr3))
			brk = 1;
		if (!brk && size == 4 && ((addr + 2 >= addr2 && addr + 2 < addr3) || (addr + 3 >= addr2 && addr + 3 < addr3)))
			brk = 1;

		if (!brk)
			continue;

		if (m->bus_error) {
			if (((m->bus_error & 1) && (rwi & 1)) || ((m->bus_error & 4) && (rwi & 4)) || ((m->bus_error & 2) && (rwi & 2))) {
				hardware_exception2(addr, val, (rwi & 2) != 0, (rwi & 4) != 0, size == 4 ? sz_long : (size == 2 ? sz_word : sz_byte));
			}
			continue;
		}

		if (mem_banks[addr >> 16]->check (addr, size)) {
			uae_u8 *p = mem_banks[addr >> 16]->xlateaddr (addr);
			if (size == 1) {
				oldval = p[0];
				newval = (*valp) & 0xff;
			} else if (size == 2) {
				oldval = (p[0] << 8) | p[1];
				newval = (*valp) & 0xffff;
			} else {
				oldval = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | (p[3] << 0);
				newval = *valp;
			}
			isoldval = 1;
		}

		if (m->pc != 0xffffffff) {
			if (m->pc != regs.instruction_pc)
				continue;
		}

		if (!m->frozen && m->val_enabled) {
			int trigger = 0;
			uae_u32 mask = m->size == 4 ? 0xffffffff : (1 << (m->size * 8)) - 1;
			uae_u32 mval = m->val;
			int scnt = size;
			for (;;) {
				if (((mval & mask) & m->val_mask) == ((val & mask) & m->val_mask))
					trigger = 1;
				if (mask & 0x80000000)
					break;
				if (m->size == 1) {
					mask <<= 8;
					mval <<= 8;
					scnt--;
				} else if (m->size == 2) {
					mask <<= 16;
					scnt -= 2;
					mval <<= 16;
				} else {
					scnt -= 4;
				}
				if (scnt <= 0)
					break;
			}
			if (!trigger)
				continue;
		}

		if (m->mustchange && rwi == 2 && isoldval) {
			if (oldval == newval)
				continue;
		}

		if (m->modval_written) {
			if (!rwi) {
				brk = 0;
			} else if (m->modval_written == 1) {
				m->modval_written = 2;
				m->modval = val;
				brk = 0;
			} else if (m->modval == val) {
				brk = 0;
			}
		}
		if (m->frozen) {
			if (m->val_enabled) {
				int shift = (addr + size - 1) - (m->addr + m->val_size - 1);
				uae_u32 sval;
				uae_u32 mask;

				if (m->val_size == 4)
					mask = 0xffffffff;
				else if (m->val_size == 2)
					mask = 0x0000ffff;
				else
					mask = 0x000000ff;

				sval = m->val;
				if (shift < 0) {
					shift = -8 * shift;
					sval >>= shift;
					mask >>= shift;
				} else {
					shift = 8 * shift;
					sval <<= shift;
					mask <<= shift;
				}
				*valp = (sval & mask) | ((*valp) & ~mask);
				//write_log (_T("%08x %08x %08x %08x %d\n"), addr, m->addr, *valp, mask, shift);
				return 1;
			}
			return 0;
		}
		mwhit.addr = addr;
		mwhit.rwi = rwi;
		mwhit.size = size;
		mwhit.val = 0;
		mwhit.access_mask = accessmask;
		mwhit.reg = reg;
		if (mwhit.rwi & 2)
			mwhit.val = val;
		mwhit.pc = M68K_GETPC;
		memwatch_triggered = i + 1;
		if (m->reportonly) {
			memwatch_hit_msg(memwatch_triggered - 1);
		}
		if (!m->nobreak && !m->reportonly) {
			debugging = 1;
			debug_pc = M68K_GETPC;
			debug_cycles(1);
			set_special(SPCFLAG_BRK);
		}
		return 1;
	}
	return 1;
}

static int mmu_hit (uaecptr addr, int size, int rwi, uae_u32 *v);

static uae_u32 REGPARAM2 mmu_lget (uaecptr addr)
{
	int off = debug_mem_off (&addr);
	uae_u32 v = 0;
	if (!mmu_hit (addr, 4, 0, &v))
		v = debug_mem_banks[off]->lget (addr);
	return v;
}
static uae_u32 REGPARAM2 mmu_wget (uaecptr addr)
{
	int off = debug_mem_off (&addr);
	uae_u32 v = 0;
	if (!mmu_hit (addr, 2, 0, &v))
		v = debug_mem_banks[off]->wget (addr);
	return v;
}
static uae_u32 REGPARAM2 mmu_bget (uaecptr addr)
{
	int off = debug_mem_off (&addr);
	uae_u32 v = 0;
	if (!mmu_hit(addr, 1, 0, &v))
		v = debug_mem_banks[off]->bget (addr);
	return v;
}
static void REGPARAM2 mmu_lput (uaecptr addr, uae_u32 v)
{
	int off = debug_mem_off (&addr);
	if (!mmu_hit (addr, 4, 1, &v))
		debug_mem_banks[off]->lput (addr, v);
}
static void REGPARAM2 mmu_wput (uaecptr addr, uae_u32 v)
{
	int off = debug_mem_off (&addr);
	if (!mmu_hit (addr, 2, 1, &v))
		debug_mem_banks[off]->wput (addr, v);
}
static void REGPARAM2 mmu_bput (uaecptr addr, uae_u32 v)
{
	int off = debug_mem_off (&addr);
	if (!mmu_hit (addr, 1, 1, &v))
		debug_mem_banks[off]->bput (addr, v);
}
static uae_u32 REGPARAM2 mmu_lgeti (uaecptr addr)
{
	int off = debug_mem_off (&addr);
	uae_u32 v = 0;
	if (!mmu_hit (addr, 4, 4, &v))
		v = debug_mem_banks[off]->lgeti (addr);
	return v;
}
static uae_u32 REGPARAM2 mmu_wgeti (uaecptr addr)
{
	int off = debug_mem_off (&addr);
	uae_u32 v = 0;
	if (!mmu_hit (addr, 2, 4, &v))
		v = debug_mem_banks[off]->wgeti (addr);
	return v;
}

static uae_u32 REGPARAM2 debug_lget(uaecptr addr)
{
	uae_u32 off = debug_mem_off(&addr);
	uae_u32 v;
	v = debug_mem_banks[off]->lget(addr);
	memwatch_func(addr, 1, 4, &v, MW_MASK_CPU_D_R, 0);
	return v;
}
static uae_u32 REGPARAM2 debug_wget (uaecptr addr)
{
	int off = debug_mem_off (&addr);
	uae_u32 v;
	v = debug_mem_banks[off]->wget (addr);
	memwatch_func (addr, 1, 2, &v, MW_MASK_CPU_D_R, 0);
	return v;
}
static uae_u32 REGPARAM2 debug_bget (uaecptr addr)
{
	int off = debug_mem_off (&addr);
	uae_u32 v;
	v = debug_mem_banks[off]->bget (addr);
	memwatch_func (addr, 1, 1, &v, MW_MASK_CPU_D_R, 0);
	return v;
}
static uae_u32 REGPARAM2 debug_lgeti (uaecptr addr)
{
	int off = debug_mem_off (&addr);
	uae_u32 v;
	v = debug_mem_banks[off]->lgeti (addr);
	memwatch_func (addr, 4, 4, &v, MW_MASK_CPU_I, 0);
	return v;
}
static uae_u32 REGPARAM2 debug_wgeti (uaecptr addr)
{
	int off = debug_mem_off (&addr);
	uae_u32 v;
	v = debug_mem_banks[off]->wgeti (addr);
	memwatch_func (addr, 4, 2, &v, MW_MASK_CPU_I, 0);
	return v;
}
static void REGPARAM2 debug_lput (uaecptr addr, uae_u32 v)
{
	int off = debug_mem_off (&addr);
	if (memwatch_func (addr, 2, 4, &v, MW_MASK_CPU_D_W, 0))
		debug_mem_banks[off]->lput (addr, v);
}
static void REGPARAM2 debug_wput (uaecptr addr, uae_u32 v)
{
	int off = debug_mem_off (&addr);
	if (memwatch_func (addr, 2, 2, &v, MW_MASK_CPU_D_W, 0))
		debug_mem_banks[off]->wput (addr, v);
}
static void REGPARAM2 debug_bput (uaecptr addr, uae_u32 v)
{
	int off = debug_mem_off (&addr);
	if (memwatch_func (addr, 2, 1, &v, MW_MASK_CPU_D_W, 0))
		debug_mem_banks[off]->bput (addr, v);
}
static int REGPARAM2 debug_check (uaecptr addr, uae_u32 size)
{
	return debug_mem_banks[munge24 (addr) >> 16]->check (addr, size);
}
static uae_u8 *REGPARAM2 debug_xlate (uaecptr addr)
{
	return debug_mem_banks[munge24 (addr) >> 16]->xlateaddr (addr);
}

struct peekdma peekdma_data;

static void peekdma_save(int type, uaecptr addr, uae_u32 mask, int reg, int ptrreg)
{
	peekdma_data.type = type;
	peekdma_data.addr = addr;
	peekdma_data.mask = mask;
	peekdma_data.reg = reg;
	peekdma_data.ptrreg = ptrreg;
}

void debug_getpeekdma_value(uae_u32 v)
{
	uae_u32 vv = v;
	if (!memwatch_enabled) {
		return;
	}
	is_valid_dma(peekdma_data.reg, peekdma_data.ptrreg, peekdma_data.addr);
	if (debug_mem_banks[peekdma_data.addr >> 16] == NULL) {
		return;
	}
	if (!currprefs.z3chipmem.size) {
		peekdma_data.addr &= chipmem_bank.mask;
	}
	memwatch_func(peekdma_data.addr, 1, 2, &vv, peekdma_data.mask, peekdma_data.reg);
}
void debug_getpeekdma_value_long(uae_u32 v, int offset)
{
	uae_u32 vv = v;
	uae_u32 mask = 0xffffffff;
	if (!memwatch_enabled) {
		return;
	}
	is_valid_dma(peekdma_data.reg, peekdma_data.ptrreg, peekdma_data.addr + offset);
	if (debug_mem_banks[(peekdma_data.addr + offset) >> 16] == NULL) {
		return;
	}
	if (!currprefs.z3chipmem.size) {
		mask = chipmem_bank.mask;
	}
	memwatch_func((peekdma_data.addr + offset) & mask, 1, 4, &vv, peekdma_data.mask, peekdma_data.reg);
}


uae_u32 debug_putpeekdma_chipset(uaecptr addr, uae_u32 v, uae_u32 mask, int reg)
{
	peekdma_save(0, addr, mask, reg, 0);
	if (!memwatch_enabled)
		return v;
	peekdma_data.addr &= 0x1fe;
	peekdma_data.addr += 0xdff000;
	memwatch_func(peekdma_data.addr, 2, 2, &v, peekdma_data.mask, peekdma_data.reg);
	return v;
}

uae_u32 debug_putpeekdma_chipram(uaecptr addr, uae_u32 v, uae_u32 mask, int reg)
{
	peekdma_save(1, addr, mask, reg, reg);
	if (!memwatch_enabled)
		return v;
	is_valid_dma(peekdma_data.reg, peekdma_data.ptrreg, peekdma_data.addr);
	if (debug_mem_banks[peekdma_data.addr >> 16] == NULL)
		return v;
	if (!currprefs.z3chipmem.size)
		peekdma_data.addr &= chipmem_bank.mask;
	memwatch_func(peekdma_data.addr & chipmem_bank.mask, 2, 2, &v, peekdma_data.mask, peekdma_data.reg);
	return v;
}

void debug_getpeekdma_chipram(uaecptr addr, uae_u32 mask, int reg)
{
	peekdma_save(2, addr, mask, reg, reg);
}

static void debug_putlpeek (uaecptr addr, uae_u32 v)
{
	if (!memwatch_enabled)
		return;
	memwatch_func (addr, 2, 4, &v, MW_MASK_CPU_D_W, 0);
}
void debug_wputpeek (uaecptr addr, uae_u32 v)
{
	if (!memwatch_enabled)
		return;
	memwatch_func (addr, 2, 2, &v, MW_MASK_CPU_D_W, 0);
}
void debug_bputpeek (uaecptr addr, uae_u32 v)
{
	if (!memwatch_enabled)
		return;
	memwatch_func (addr, 2, 1, &v, MW_MASK_CPU_D_W, 0);
}
void debug_bgetpeek (uaecptr addr, uae_u32 v)
{
	uae_u32 vv = v;
	if (!memwatch_enabled)
		return;
	memwatch_func (addr, 1, 1, &vv, MW_MASK_CPU_D_R, 0);
}
void debug_wgetpeek (uaecptr addr, uae_u32 v)
{
	uae_u32 vv = v;
	if (!memwatch_enabled)
		return;
	memwatch_func (addr, 1, 2, &vv, MW_MASK_CPU_D_R, 0);
}
void debug_lgetpeek (uaecptr addr, uae_u32 v)
{
	uae_u32 vv = v;
	if (!memwatch_enabled)
		return;
	memwatch_func (addr, 1, 4, &vv, MW_MASK_CPU_D_R, 0);
}

struct membank_store
{
	addrbank *addr;
	addrbank newbank;
	int banknr;
};

static struct membank_store *membank_stores;
static int membank_total;
#define MEMWATCH_STORE_SLOTS 32

static void memwatch_reset (void)
{
	for (int i = 0; i < membank_total; i++) {
		addrbank *ab = debug_mem_banks[i];
		if (!ab)
			continue;
		map_banks_quick (ab, i, 1, 1);
	}
	for (int i = 0; membank_stores[i].addr; i++) {
		struct membank_store *ms = &membank_stores[i];
		/* name was allocated in memwatch_remap */
		xfree ((char*)ms->newbank.name);
		memset (ms, 0, sizeof (struct membank_store));
		ms->addr = NULL;
	}
	memset (debug_mem_banks, 0, membank_total * sizeof (addrbank*));
}

static void memwatch_remap (uaecptr addr)
{
	int mode = 0;
	int i;
	int banknr;
	struct membank_store *ms;
	addrbank *bank;
	addrbank *newbank = NULL;

	addr &= ~65535;
	banknr = addr >> 16;
	if (debug_mem_banks[banknr])
		return;
	bank = mem_banks[banknr];
	for (i = 0 ; i < MEMWATCH_STORE_SLOTS; i++) {
		ms = &membank_stores[i];
		if (ms->addr == NULL)
			break;
		if (ms->addr == bank) {
			newbank = &ms->newbank;
			break;
		}
	}
	if (i >= MEMWATCH_STORE_SLOTS)
		return;
	if (!newbank) {
		TCHAR tmp[200];
		_sntprintf (tmp, sizeof tmp, _T("%s [D]"), bank->name);
		ms->addr = bank;
		ms->banknr = banknr;
		newbank = &ms->newbank;
		memcpy (newbank, bank, sizeof(addrbank));
		newbank->bget = mode ? mmu_bget : debug_bget;
		newbank->wget = mode ? mmu_wget : debug_wget;
		newbank->lget = mode ? mmu_lget : debug_lget;
		newbank->bput = mode ? mmu_bput : debug_bput;
		newbank->wput = mode ? mmu_wput : debug_wput;
		newbank->lput = mode ? mmu_lput : debug_lput;
		newbank->check = debug_check;
		newbank->xlateaddr = debug_xlate;
		newbank->wgeti = mode ? mmu_wgeti : debug_wgeti;
		newbank->lgeti = mode ? mmu_lgeti : debug_lgeti;
		/* name will be freed by memwatch_reset */
		newbank->name = my_strdup (tmp);
		if (!newbank->mask)
			newbank->mask = -1;
		newbank->baseaddr_direct_r = 0;
		newbank->baseaddr_direct_w = 0;
	}
	debug_mem_banks[banknr] = bank;
	map_banks_quick (newbank, banknr, 1, 1);
	// map aliases
	for (i = 0; i < membank_total; i++) {
		uaecptr addr2 = i << 16;
		addrbank *ab = &get_mem_bank(addr2);
		if (ab != ms->addr)
			continue;
		if ((addr2 & ab->mask) == (addr & bank->mask)) {
			debug_mem_banks[i] = ms->addr;
			map_banks_quick (newbank, i, 1, 1);
		}
	}
}

static void memwatch_setup(void)
{
	memwatch_reset();
	mwnodes_start = MEMWATCH_TOTAL - 1;
	mwnodes_end = 0;
	for (int i = 0; i < MEMWATCH_TOTAL; i++) {
		struct memwatch_node *m = &mwnodes[i];
		if (!m->size)
			continue;
		if (mwnodes_start > i)
			mwnodes_start = i;
		if (mwnodes_end < i)
			mwnodes_end = i;
		int addr = m->addr & ~65535;
		int eaddr = (m->addr + m->size + 65535) & ~65535;
		while (addr < eaddr) {
			memwatch_remap(addr);
			addr += 65536;
		}
	}
}

static int deinitialize_memwatch (void)
{
	int oldmode;

	if (!memwatch_enabled && !mmu_enabled)
		return -1;
	memwatch_reset ();
	oldmode = mmu_enabled ? 1 : 0;
	xfree (debug_mem_banks);
	debug_mem_banks = NULL;
	xfree (debug_mem_area);
	debug_mem_area = NULL;
	xfree (membank_stores);
	membank_stores = NULL;
	memwatch_enabled = 0;
	mmu_enabled = 0;
	xfree (illgdebug);
	illgdebug = 0;
	return oldmode;
}

static void initialize_memwatch (int mode)
{
	membank_total = currprefs.address_space_24 ? 256 : 65536;
	deinitialize_memwatch ();
	debug_mem_banks = xcalloc (addrbank*, 65536);
	if (!debug_mem_banks)
		return;
	if (membank_total < 65536) {
		for (int i = 256; i < 65536; i++) {
			debug_mem_banks[i] = &dummy_bank;
		}
	}
	debug_mem_area = xcalloc (addrbank, membank_total);
	membank_stores = xcalloc (struct membank_store, MEMWATCH_STORE_SLOTS);
	for (int i = 0; i < MEMWATCH_TOTAL; i++) {
		struct memwatch_node *m = &mwnodes[i];
		m->pc = 0xffffffff;
	}
#if 0
	int i, j, as;
	addrbank *a1, *a2, *oa;
	oa = NULL;
	for (i = 0; i < as; i++) {
		a1 = debug_mem_banks[i] = debug_mem_area + i;
		a2 = mem_banks[i];
		if (a2 != oa) {
			for (j = 0; membank_stores[j].addr; j++) {
				if (membank_stores[j].addr == a2)
					break;
			}
			if (membank_stores[j].addr == NULL) {
				membank_stores[j].addr = a2;
				memcpy (&membank_stores[j].store, a2, sizeof (addrbank));
			}
		}
		memcpy (a1, a2, sizeof (addrbank));
	}
	for (i = 0; i < as; i++) {
		a2 = mem_banks[i];
		a2->bget = mode ? mmu_bget : debug_bget;
		a2->wget = mode ? mmu_wget : debug_wget;
		a2->lget = mode ? mmu_lget : debug_lget;
		a2->bput = mode ? mmu_bput : debug_bput;
		a2->wput = mode ? mmu_wput : debug_wput;
		a2->lput = mode ? mmu_lput : debug_lput;
		a2->check = debug_check;
		a2->xlateaddr = debug_xlate;
		a2->wgeti = mode ? mmu_wgeti : debug_wgeti;
		a2->lgeti = mode ? mmu_lgeti : debug_lgeti;
	}
#endif
	if (mode)
		mmu_enabled = 1;
	else
		memwatch_enabled = 1;
}

int debug_bankchange (int mode)
{
	if (mode == -1) {
		int v = deinitialize_memwatch ();
		if (v < 0)
			return -2;
		return v;
	}
	if (mode >= 0) {
		initialize_memwatch (mode);
		memwatch_setup ();
	}
	return -1;
}

addrbank *get_mem_bank_real(uaecptr addr)
{
	addrbank *ab = &get_mem_bank(addr);
	if (!memwatch_enabled)
		return ab;
	addrbank *ab2 = debug_mem_banks[addr >> 16];
	if (ab2)
		return ab2;
	return ab;
}

static const TCHAR *getsizechar (int size)
{
	if (size == 4)
		return _T(".l");
	if (size == 3)
		return _T(".3");
	if (size == 2)
		return _T(".w");
	if (size == 1)
		return _T(".b");
	return _T("");
}

void memwatch_dump2 (TCHAR *buf, int bufsize, int num)
{
	int i;
	struct memwatch_node *mwn;

	if (buf)
		memset (buf, 0, bufsize * sizeof (TCHAR));
	for (i = 0; i < MEMWATCH_TOTAL; i++) {
		if ((num >= 0 && num == i) || (num < 0)) {
			uae_u32 usedmask = 0;
			mwn = &mwnodes[i];
			if (mwn->size == 0)
				continue;
			buf = buf_out (buf, &bufsize, _T("%2d: %08X - %08X (%d) %c%c%c"),
				i, mwn->addr, mwn->addr + (mwn->size - 1), mwn->size,
				(mwn->rwi & 1) ? 'R' : ' ', (mwn->rwi & 2) ? 'W' : ' ', (mwn->rwi & 4) ? 'I' : ' ');
			if (mwn->frozen)
				buf = buf_out (buf, &bufsize, _T(" F"));
			if (mwn->val_enabled)
				buf = buf_out (buf, &bufsize, _T(" =%X%s"), mwn->val, getsizechar (mwn->val_size));
			if (mwn->modval_written)
				buf = buf_out (buf, &bufsize, _T(" =M"));
			if (mwn->mustchange)
				buf = buf_out(buf, &bufsize, _T(" C"));
			if (mwn->pc != 0xffffffff)
				buf = buf_out(buf, &bufsize, _T(" PC=%08x"), mwn->pc);
			if (mwn->reportonly)
				buf = buf_out(buf, &bufsize, _T(" L"));
			if (mwn->nobreak)
				buf = buf_out(buf, &bufsize, _T(" N"));
			if (mwn->bus_error) {
				buf = buf_out(buf, &bufsize, _T(" BER%s%s%s"),
					(mwn->bus_error & 1) ? _T("R") : _T(""),
					(mwn->bus_error & 2) ? _T("W") : _T(""),
					(mwn->bus_error & 4) ? _T("P") : _T(""));
			}
			for (int j = 0; memwatch_access_masks[j].mask; j++) {
				uae_u32 mask = memwatch_access_masks[j].mask;
				if ((mwn->access_mask & mask) == mask && (usedmask & mask) == 0) {
					buf = buf_out(buf, &bufsize, _T(" "));
					buf = buf_out(buf, &bufsize, memwatch_access_masks[j].name);
					usedmask |= mask;
				}
			}
			buf = buf_out (buf, &bufsize, _T("\n"));
		}
	}
}

static void memwatch_dump (int num)
{
	TCHAR *buf;
	int multiplier = num < 0 ? MEMWATCH_TOTAL : 1;

	buf = xmalloc (TCHAR, 50 * multiplier);
	if (!buf)
		return;
	memwatch_dump2 (buf, 50 * multiplier, num);
	f_out (stdout, _T("%s"), buf);
	xfree (buf);
}

static void memwatch (TCHAR **c)
{
	int num;
	struct memwatch_node *mwn;
	TCHAR nc, *cp;
	bool err;

	if (!memwatch_enabled) {
		initialize_memwatch (0);
		console_out (_T("Memwatch breakpoints enabled\n"));
		memwatch_access_validator = 0;
	}

	cp = *c;
	ignore_ws (c);
	if (!more_params (c)) {
		memwatch_dump (-1);
		return;
	}
	nc = next_char (c);
	if (nc == '-') {
		deinitialize_memwatch ();
		console_out (_T("Memwatch breakpoints disabled\n"));
		return;
	}
	if (nc == 'l') {
		memwatch_access_validator = !memwatch_access_validator;
		console_out_f(_T("Memwatch DMA validator %s\n"), memwatch_access_validator ? _T("enabled") : _T("disabled"));
		return;
	}

	if (nc == 'd') {
		if (illgdebug) {
			ignore_ws (c);
			if (more_params (c)) {
				uae_u32 addr = readhex(c, NULL);
				uae_u32 len = 1;
				if (more_params (c))
					len = readhex(c, NULL);
				console_out_f (_T("Cleared logging addresses %08X - %08X\n"), addr, addr + len);
				while (len > 0) {
					addr &= 0xffffff;
					illgdebug[addr] = 7;
					addr++;
					len--;
				}
			} else {
				illg_free();
				console_out (_T("Illegal memory access logging disabled\n"));
			}
		} else {
			illg_init ();
			ignore_ws (c);
			illgdebug_break = 0;
			if (more_params (c))
				illgdebug_break = 1;
			console_out_f (_T("Illegal memory access logging enabled. Break=%d\n"), illgdebug_break);
		}
		return;
	}
	*c = cp;
	num = readint(c, NULL);
	if (num < 0 || num >= MEMWATCH_TOTAL)
		return;
	mwn = &mwnodes[num];
	mwn->size = 0;
	ignore_ws (c);
	if (!more_params (c)) {
		console_out_f (_T("Memwatch %d removed\n"), num);
		memwatch_setup ();
		return;
	}
	mwn->addr = readhex(c, NULL);
	mwn->size = 1;
	mwn->rwi = 7;
	mwn->val_enabled = 0;
	mwn->val_mask = 0xffffffff;
	mwn->val = 0;
	mwn->access_mask = 0;
	mwn->reg = 0xffffffff;
	mwn->frozen = 0;
	mwn->modval_written = 0;
	mwn->mustchange = 0;
	mwn->bus_error = 0;
	mwn->reportonly = false;
	mwn->nobreak = false;
	ignore_ws (c);
	if (more_params (c)) {
		mwn->size = readhex(c, NULL);
		ignore_ws (c);
		if (more_params (c)) {
			TCHAR *cs = *c;
			while (*cs) {
				for (int i = 0; memwatch_access_masks[i].mask; i++) {
					const TCHAR *n = memwatch_access_masks[i].name;
					int len = uaetcslen(n);
					if (!_tcsnicmp(cs, n, len)) {
						if (cs[len] == 0 || cs[len] == 10 || cs[len] == 13 || cs[len] == ' ') {
							mwn->access_mask |= memwatch_access_masks[i].mask;
							while (len > 0) {
								len--;
								cs[len] = ' ';
							}
						}
					}
				}
				cs++;
			}
			ignore_ws (c);
			if (more_params(c)) {
				for (;;) {
					TCHAR ncc = _totupper(peek_next_char(c));
					TCHAR nc = _totupper(next_char(c));
					if (mwn->rwi == 7 && (nc == 'W' || nc == 'R' || nc == 'I'))
						mwn->rwi = 0;
					if (nc == 'F')
						mwn->frozen = 1;
					if (nc == 'W')
						mwn->rwi |= 2;
					if (nc == 'I')
						mwn->rwi |= 4;
					if (nc == 'R')
						mwn->rwi |= 1;
					if (nc == 'B') {
						mwn->bus_error = 0;
						for (;;) {
							ncc = next_char2(c);
							if (ncc == ' ' || ncc == 0)
								break;
							if (ncc == 'R') {
								mwn->bus_error |= 1;
								mwn->rwi |= 1;
							} else if (ncc == 'W') {
								mwn->bus_error |= 2;
								mwn->rwi |= 2;
							} else if (ncc == 'P') {
								mwn->bus_error |= 4;
								mwn->rwi |= 4;
							} else {
								break;
							}
						}
						if (!mwn->rwi)
							mwn->rwi = 7;
						if (!mwn->bus_error)
							mwn->bus_error = 7;
					}
					if (nc == 'L')
						mwn->reportonly = true;
					if (nc == 'N')
						mwn->nobreak = true;
					if (nc == 'P' && ncc == 'C') {
						next_char(c);
						mwn->pc = readhex(c, NULL);
					}
					if (nc == 'M') {
						mwn->modval_written = 1;
					}
					if (nc == 'C') {
						mwn->mustchange = 1;
					}
					if (nc == 'V') {
						mwn->val = readhex(c, &mwn->val_size, &err);
						mwn->val_enabled = 1;
					}
					if (!more_params(c))
						break;
				}
				ignore_ws (c);
			}
		}
	}
	if (!mwn->access_mask)
		mwn->access_mask = MW_MASK_CPU_D_R | MW_MASK_CPU_D_W | MW_MASK_CPU_I;
	if (mwn->frozen && mwn->rwi == 0)
		mwn->rwi = 3;
	memwatch_setup ();
	memwatch_dump (num);
}

static void copymem(TCHAR **c)
{
	uae_u32 addr = 0, eaddr = 0, dst = 0;
	bool err;

	ignore_ws(c);
	if (!more_params(c))
		return;
	addr = readhex(c, &err);
	if (err) {
		return;
	}
	ignore_ws (c);
	if (!more_params(c))
		return;
	eaddr = readhex(c, &err);
	if (err) {
		return;
	}
	ignore_ws (c);
	if (!more_params(c))
		return;
	dst = readhex(c, &err);
	if (err) {
		return;
	}

	if (addr >= eaddr)
		return;
	uae_u32 addrb = addr;
	uae_u32 dstb = dst;
	uae_u32 len = eaddr - addr;
	if (dst <= addr) {
		while (addr < eaddr) {
			put_byte(dst, get_byte(addr));
			addr++;
			dst++;
		}
	} else {
		dst += eaddr - addr;
		while (addr < eaddr) {
			dst--;
			eaddr--;
			put_byte(dst, get_byte(eaddr));
		}
	}
	console_out_f(_T("Copied from %08x - %08x to %08x - %08x\n"), addrb, addrb + len - 1, dstb, dstb + len - 1);
}

static void writeintomem (TCHAR **c)
{
	uae_u32 addr = 0;
	uae_u32 eaddr = 0xffffffff;
	uae_u32 val = 0;
	TCHAR cc;
	int len = 1;
	bool fillmode = false;
	bool err;

	if (**c == 'f') {
		fillmode = true;
		(*c)++;
	} else if (**c == 'c') {
		(*c)++;
		copymem(c);
		return;
	}

	ignore_ws(c);
	addr = readhex(c, &err);
	if (err) {
		return;
	}
	ignore_ws (c);

	if (fillmode) {
		if (!more_params(c))
			return;
		eaddr = readhex(c, &err);
		if (err) {
			return;
		}
		ignore_ws(c);
	}

	if (!more_params (c))
		return;
	TCHAR *cb = *c;
	uae_u32 addrc = addr;
	bool retry = false;
	for(;;) {
		cc = peekchar(c);
		retry = false;
		uae_u32 addrb = addr;
		if (cc == '\'' || cc == '\"') {
			TCHAR quoted = cc;
			next_char2(c);
			while (more_params2(c)) {
				TCHAR str[2];
				char *astr;
				cc = next_char2(c);
				if (quoted == cc) {
					ignore_ws(c);
					retry = true;
					break;
				}
				if (addr >= eaddr) {
					break;
				}
				str[0] = cc;
				str[1] = 0;
				astr = ua(str);
				put_byte(addr, astr[0]);
				if (!fillmode) {
					char c = astr[0];
					if (c < 32) {
						c = '.';
					}
					console_out_f(_T("Wrote '%c' (%02X, %02u) at %08X.B\n"), c, c, c, addr);
				}
				xfree(astr);
				addr++;
			}
			if (fillmode && peekchar(c) == 0) {
				*c = cb;
			}
		} else {
			for (;;) {
				bool err;
				ignore_ws(c);
				if (!more_params(c))
					break;
				val = readhex(c, &len, &err);
				if (err) {
					goto end;
				}
				if (len == 4) {
					put_long(addr, val);
					cc = 'L';
				} else if (len == 2) {
					put_word(addr, val);
					cc = 'W';
				} else if (len == 1) {
					put_byte(addr, val);
					cc = 'B';
				} else {
					cc = peekchar(c);
					if (cc == '\'' || cc == '\"') {
						retry = true;
					} else {
						next_char(c);
						retry = true;
					}
					break;
				}
				if (!fillmode) {
					console_out_f(_T("Wrote %X (%u) at %08X.%c\n"), val, val, addr, cc);
				}
				addr += len;
				if (addr >= eaddr)
					break;
			}
			if (fillmode && peekchar(c) == 0) {
				*c = cb;
			}
		}
		if (retry) {
			continue;
		}
		if (addr >= eaddr) {
			break;
		}
		if (eaddr == 0xffffffff || addr <= addrb) {
			break;
		}
	}
end:
	if (eaddr != 0xffffffff)
		console_out_f(_T("Wrote data to %08x - %08x\n"), addrc, addr);
}

static uae_u8 *dump_xlate (uae_u32 addr)
{
	if (!mem_banks[addr >> 16]->check (addr, 1))
		return NULL;
	return mem_banks[addr >> 16]->xlateaddr (addr);
}

#if 0
#define UAE_MEMORY_REGIONS_MAX 64
#define UAE_MEMORY_REGION_NAME_LENGTH 64

#define UAE_MEMORY_REGION_RAM (1 << 0)
#define UAE_MEMORY_REGION_ALIAS (1 << 1)
#define UAE_MEMORY_REGION_MIRROR (1 << 2)

typedef struct UaeMemoryRegion {
	uaecptr start;
	int size;
	TCHAR name[UAE_MEMORY_REGION_NAME_LENGTH];
	TCHAR rom_name[UAE_MEMORY_REGION_NAME_LENGTH];
	uaecptr alias;
	int flags;
} UaeMemoryRegion;

typedef struct UaeMemoryMap {
	UaeMemoryRegion regions[UAE_MEMORY_REGIONS_MAX];
	int num_regions;
} UaeMemoryMap;
#endif

static const TCHAR *bankmodes[] = { _T("F32"), _T("C16"), _T("C32"), _T("CIA"), _T("F16"), _T("F16X") };

static void memory_map_dump_3(UaeMemoryMap *map, int log)
{
	bool imold;
	int i, j, max;
	addrbank *a1 = mem_banks[0];
	TCHAR txt[256];

	map->num_regions = 0;
	imold = currprefs.illegal_mem;
	currprefs.illegal_mem = false;
	max = currprefs.address_space_24 ? 256 : 65536;
	j = 0;
	for (i = 0; i < max + 1; i++) {
		addrbank *a2 = NULL;
		if (i < max)
			a2 = mem_banks[i];
		if (a1 != a2) {
			int k, mirrored, mirrored2, size, size_out;
			TCHAR size_ext;
			uae_u8 *caddr;
			TCHAR tmp[MAX_DPATH];
			const TCHAR *name = a1->name;
			struct addrbank_sub *sb = a1->sub_banks;
			int bankoffset = 0;
			int region_size;

			k = j;
			caddr = dump_xlate (k << 16);
			mirrored = caddr ? 1 : 0;
			k++;
			while (k < i && caddr) {
				if (dump_xlate (k << 16) == caddr) {
					mirrored++;
				}
				k++;
			}
			mirrored2 = mirrored;
			if (mirrored2 == 0)
				mirrored2 = 1;

			while (bankoffset < 65536) {
				int bankoffset2 = bankoffset;
				if (sb) {
					uaecptr daddr;
					if (!sb->bank)
						break;
					daddr = (j << 16) | bankoffset;
					a1 = get_sub_bank(&daddr);
					name = a1->name;
					for (;;) {
						bankoffset2 += MEMORY_MIN_SUBBANK;
						if (bankoffset2 >= 65536)
							break;
						daddr = (j << 16) | bankoffset2;
						addrbank *dab = get_sub_bank(&daddr);
						if (dab != a1)
							break;
					}
					sb++;
					size = (bankoffset2 - bankoffset) / 1024;
					region_size = size * 1024;
				} else {
					size = (i - j) << (16 - 10);
					region_size = ((i - j) << 16) / mirrored2;
				}

				if (name == NULL)
					name = _T("<none>");

				size_out = size;
				size_ext = 'K';
				if (j >= 256 && (size_out / mirrored2 >= 1024) && !((size_out / mirrored2) & 1023)) {
					size_out /= 1024;
					size_ext = 'M';
				}
				_sntprintf (txt, sizeof txt, _T("%08X %7d%c/%d = %7d%c %s%s%c %s %s"), (j << 16) | bankoffset, size_out, size_ext,
					mirrored, mirrored ? size_out / mirrored : size_out, size_ext,
					(a1->flags & ABFLAG_CACHE_ENABLE_INS) ? _T("I") : _T("-"),
					(a1->flags & ABFLAG_CACHE_ENABLE_DATA) ? _T("D") : _T("-"),
					a1->baseaddr == NULL ? ' ' : '*',
					bankmodes[ce_banktype[j]],
					name);
				tmp[0] = 0;
				if ((a1->flags & ABFLAG_ROM) && mirrored) {
					TCHAR *p = txt + _tcslen (txt);
					uae_u32 crc = 0xffffffff;
					if (a1->check(((j << 16) | bankoffset), (size * 1024) / mirrored))
						crc = get_crc32 (a1->xlateaddr((j << 16) | bankoffset), (size * 1024) / mirrored);
					struct romdata *rd = getromdatabycrc (crc);
					_sntprintf (p, sizeof p, _T(" (%08X)"), crc);
					if (rd) {
						tmp[0] = '=';
						getromname (rd, tmp + 1);
						_tcscat (tmp, _T("\n"));
					}
				}

				if (a1 != &dummy_bank) {
					for (int m = 0; m < mirrored2; m++) {
						if (map->num_regions >= UAE_MEMORY_REGIONS_MAX)
							break;
						UaeMemoryRegion *r = &map->regions[map->num_regions];
						r->start = (j << 16) + bankoffset + region_size * m;
						r->size = region_size;
						r->flags = 0;
						r->memory = NULL;
						if (!(a1->flags & ABFLAG_PPCIOSPACE)) {
							r->memory = dump_xlate((j << 16) | bankoffset);
							if (r->memory)
								r->flags |= UAE_MEMORY_REGION_RAM;
						}
						/* just to make it easier to spot in debugger */
						r->alias = 0xffffffff;
						if (m >= 0) {
							r->alias = j << 16;
							r->flags |= UAE_MEMORY_REGION_ALIAS | UAE_MEMORY_REGION_MIRROR;
						}
						_sntprintf(r->name, sizeof r->name, _T("%s"), name);
						_sntprintf(r->rom_name, sizeof r->rom_name, _T("%s"), tmp);
						map->num_regions += 1;
					}
				}
				_tcscat (txt, _T("\n"));
				if (log > 0)
					write_log (_T("%s"), txt);
				else if (log == 0)
					console_out (txt);
				if (tmp[0]) {
					if (log > 0)
						write_log (_T("%s"), tmp);
					else if (log == 0)
						console_out (tmp);
				}
				if (!sb)
					break;
				bankoffset = bankoffset2;
			}
			j = i;
			a1 = a2;
		}
	}
#ifdef WITH_PCI
	pci_dump(log);
#endif
	currprefs.illegal_mem = imold;
}

void uae_memory_map(UaeMemoryMap *map)
{
	memory_map_dump_3(map, -1);
}

static void memory_map_dump_2 (int log)
{
	UaeMemoryMap map;
	memory_map_dump_3(&map, log);
#if 0
	for (int i = 0; i < map.num_regions; i++) {
		TCHAR txt[256];
		UaeMemoryRegion *r = &map.regions[i];
		int size = r->size / 1024;
		TCHAR size_ext = 'K';
		int mirrored = 1;
		int size_out = 0;
		_stprintf (txt, _T("%08X %7u%c/%d = %7u%c %s\n"), r->start, size, size_ext,
			r->flags & UAE_MEMORY_REGION_RAM, size, size_ext, r->name);
		if (log)
			write_log (_T("%s"), txt);
		else
			console_out (txt);
		if (r->rom_name[0]) {
			if (log)
				write_log (_T("%s"), r->rom_name);
			else
				console_out (r->rom_name);
		}
	}
#endif
}

void memory_map_dump (void)
{
	memory_map_dump_2(1);
}

STATIC_INLINE uaecptr BPTR2APTR (uaecptr addr)
{
	return addr << 2;
}
static TCHAR *BSTR2CSTR (uae_u8 *bstr)
{
	TCHAR *s;
	char *cstr = xmalloc (char, bstr[0] + 1);
	if (cstr) {
		memcpy (cstr, bstr + 1, bstr[0]);
		cstr[bstr[0]] = 0;
	}
	s = au (cstr);
	xfree (cstr);
	return s;
}

static void print_task_info (uaecptr node, bool nonactive)
{
	TCHAR *s;
	int process = get_byte_debug (node + 8) == 13 ? 1 : 0;

	console_out_f (_T("%08X: "), node);
	s = au ((char*)get_real_address_debug(get_long_debug (node + 10)));
	console_out_f (process ? _T("PROCESS '%s'\n") : _T("TASK    '%s'\n"), s);
	xfree (s);
	if (process) {
		uaecptr cli = BPTR2APTR (get_long_debug (node + 172));
		int tasknum = get_long_debug (node + 140);
		if (cli && tasknum) {
			uae_u8 *command_bstr = get_real_address_debug(BPTR2APTR (get_long_debug (cli + 16)));
			TCHAR *command = BSTR2CSTR (command_bstr);
			console_out_f (_T(" [%d, '%s']\n"), tasknum, command);
			xfree (command);
		} else {
			console_out (_T("\n"));
		}
	}
	if (nonactive) {
		uae_u32 sigwait = get_long_debug(node + 22);
		if (sigwait)
			console_out_f(_T("          Waiting signals: %08x\n"), sigwait);
		int offset = kickstart_version >= 37 ? 74 : 70;
		uae_u32 sp = get_long_debug(node + 54) + offset;
		uae_u32 pc = get_long_debug(sp);
		console_out_f(_T("          SP: %08x PC: %08x\n"), sp, pc);
	}
}

static void show_exec_tasks (void)
{
	uaecptr execbase = get_long_debug (4);
	uaecptr taskready = execbase + 406;
	uaecptr taskwait = execbase + 420;
	uaecptr node;
	console_out_f (_T("Execbase at 0x%08X\n"), execbase);
	console_out (_T("Current:\n"));
	node = get_long_debug (execbase + 276);
	print_task_info (node, false);
	console_out_f (_T("Ready:\n"));
	node = get_long_debug (taskready);
	while (node && get_long_debug(node)) {
		print_task_info (node, true);
		node = get_long_debug (node);
	}
	console_out (_T("Waiting:\n"));
	node = get_long_debug (taskwait);
	while (node && get_long_debug(node)) {
		print_task_info (node, true);
		node = get_long_debug (node);
	}
}

static uaecptr get_base (const uae_char *name, int offset)
{
	uaecptr v = get_long_debug (4);
	addrbank *b = &get_mem_bank(v);

	if (!b || !b->check (v, 400) || !(b->flags & ABFLAG_RAM))
		return 0;
	v += offset;
	while ((v = get_long_debug (v))) {
		uae_u32 v2;
		uae_u8 *p;
		b = &get_mem_bank (v);
		if (!b || !b->check (v, 32) || (!(b->flags & ABFLAG_RAM) && !(b->flags & ABFLAG_ROMIN)))
			goto fail;
		v2 = get_long_debug (v + 10); // name
		b = &get_mem_bank (v2);
		if (!b || !b->check (v2, 20))
			goto fail;
		if ((b->flags & ABFLAG_ROM) || (b->flags & ABFLAG_RAM) || (b->flags & ABFLAG_ROMIN)) {
			p = get_real_address_debug(v2);
			if (!memcmp (p, name, strlen (name) + 1))
				return v;
		}
	}
	return 0;
fail:
	return 0xffffffff;
}

static TCHAR *getfrombstr(uaecptr pp)
{
	uae_u8 len = get_byte(pp << 2);
	TCHAR *s = xcalloc (TCHAR, len + 1);
	char data[256];
	for (int i = 0; i < len; i++) {
		data[i] = get_byte((pp << 2) + 1 + i);
		data[i + 1] = 0;
	}
	return au_copy (s, len + 1, data);
}

// read one byte from expansion autoconfig ROM
static void copyromdata(uae_u8 bustype, uaecptr rom, int offset, uae_u8 *out, int size)
{
	switch (bustype & 0xc0)
	{
	case 0x00: // nibble
		while (size-- > 0) {
			*out++ = (get_byte_debug(rom + offset * 4 + 0) & 0xf0) | ((get_byte_debug(rom + offset * 4 + 2) & 0xf0) >> 4);
			offset++;
		}
		break;
	case 0x40: // byte
		while (size-- > 0) {
			*out++ = get_byte_debug(rom + offset * 2);
			offset++;
		}
		break;
	case 0x80: // word
	default:
		while (size-- > 0) {
			*out++ = get_byte_debug(rom + offset);
			offset++;
		}
		break;
	}
}

static void show_exec_lists (TCHAR *t)
{
	uaecptr execbase = get_long_debug (4);
	uaecptr list = 0, node;
	TCHAR c = t[0];

	if (c == 'o' || c == 'O') { // doslist
		uaecptr dosbase = get_base ("dos.library", 378);
		if (dosbase) {
			uaecptr rootnode = get_long_debug (dosbase + 34);
			uaecptr dosinfo = get_long_debug (rootnode + 24) << 2;
			console_out_f (_T("ROOTNODE: %08x DOSINFO: %08x\n"), rootnode, dosinfo);
			uaecptr doslist = get_long_debug (dosinfo + 4) << 2;
			while (doslist) {
				int type = get_long_debug (doslist + 4);
				uaecptr msgport = get_long_debug (doslist + 8);
				uaecptr lock = get_long_debug(doslist + 12);
				TCHAR *name = getfrombstr(get_long_debug(doslist + 40));
				console_out_f(_T("%08x: Type=%d Port=%08x Lock=%08x '%s'\n"), doslist, type, msgport, lock, name);
				if (type == 0) {
					uaecptr fssm = get_long_debug(doslist + 28) << 2;
					console_out_f (_T(" - H=%08x Stack=%5d Pri=%2d Start=%08x Seg=%08x GV=%08x\n"),
						get_long_debug (doslist + 16) << 2, get_long_debug (doslist + 20),
						get_long_debug (doslist + 24), fssm,
						get_long_debug (doslist + 32) << 2, get_long_debug (doslist + 36));
					if (fssm >= 0x100 && (fssm & 3) == 0) {
						TCHAR *unitname = getfrombstr(get_long_debug(fssm + 4));
						console_out_f (_T("   %s:%d %08x\n"), unitname, get_long_debug(fssm), get_long_debug(fssm + 8));
						uaecptr de = get_long_debug(fssm + 8) << 2;
						if (de) {
							console_out_f (_T("    TableSize       %u\n"), get_long_debug(de + 0));
							console_out_f (_T("    SizeBlock       %u\n"), get_long_debug(de + 4));
							console_out_f (_T("    SecOrg          %u\n"), get_long_debug(de + 8));
							console_out_f (_T("    Surfaces        %u\n"), get_long_debug(de + 12));
							console_out_f (_T("    SectorPerBlock  %u\n"), get_long_debug(de + 16));
							console_out_f (_T("    BlocksPerTrack  %u\n"), get_long_debug(de + 20));
							console_out_f (_T("    Reserved        %u\n"), get_long_debug(de + 24));
							console_out_f (_T("    PreAlloc        %u\n"), get_long_debug(de + 28));
							console_out_f (_T("    Interleave      %u\n"), get_long_debug(de + 32));
							console_out_f (_T("    LowCyl          %u\n"), get_long_debug(de + 36));
							console_out_f (_T("    HighCyl         %u (Total %u)\n"), get_long_debug(de + 40), get_long_debug(de + 40) - get_long_debug(de + 36) + 1);
							console_out_f (_T("    NumBuffers      %u\n"), get_long_debug(de + 44));
							console_out_f (_T("    BufMemType      0x%08x\n"), get_long_debug(de + 48));
							console_out_f (_T("    MaxTransfer     0x%08x\n"), get_long_debug(de + 52));
							console_out_f (_T("    Mask            0x%08x\n"), get_long_debug(de + 56));
							console_out_f (_T("    BootPri         %d\n"), get_long_debug(de + 60));
							console_out_f (_T("    DosType         0x%08x\n"), get_long_debug(de + 64));
						}
						xfree(unitname);
					}
				} else if (type == 2) {
					console_out_f(_T(" - VolumeDate=%08x %08x %08x LockList=%08x DiskType=%08x\n"),
						get_long_debug(doslist + 16), get_long_debug(doslist + 20), get_long_debug(doslist + 24),
						get_long_debug(doslist + 28),
						get_long_debug(doslist + 32));
				}
				xfree (name);
				doslist = get_long_debug (doslist) << 2;
			}
		} else {
			console_out_f (_T("can't find dos.library\n"));
		}
		return;
	} else if (c == 'i' || c == 'I') { // interrupts
		static const int it[] = {  1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0 };
		static const int it2[] = { 1, 1, 1, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 6, 6, 7 };
		list = execbase + 84;
		for (int i = 0; i < 16; i++) {
			console_out_f (_T("%2d %d: %08X\n"), i + 1, it2[i], list);
			if (it[i]) {
				console_out_f (_T("  [H] %08X\n"), get_long_debug (list));
				node = get_long_debug (list + 8);
				if (node) {
					uae_u8 *addr = get_real_address_debug(get_long_debug (node + 10));
					TCHAR *name = addr ? au ((char*)addr) : au("<null>");
					console_out_f (_T("      %08X (C=%08X D=%08X) '%s'\n"), node, get_long_debug (list + 4), get_long_debug (list), name);
					xfree (name);
				}
			} else {
				int cnt = 0;
				node = get_long_debug (list);
				node = get_long_debug (node);
				while (get_long_debug (node)) {
					uae_u8 *addr = get_real_address_debug(get_long_debug (node + 10));
					TCHAR *name = addr ? au ((char*)addr) : au("<null>");
					uae_s8 pri = get_byte_debug(node + 9);
					console_out_f (_T("  [S] %08x %+03d (C=%08x D=%08X) '%s'\n"), node, pri, get_long_debug (node + 18), get_long_debug (node + 14), name);
					if (i == 4 - 1 || i == 14 - 1) {
						if (!_tcsicmp (name, _T("cia-a")) || !_tcsicmp (name, _T("cia-b"))) {
							static const TCHAR *ciai[] = { _T("A"), _T("B"), _T("ALRM"), _T("SP"), _T("FLG") };
							uaecptr cia = node + 22;
							for (int j = 0; j < 5; j++) {
								uaecptr ciap = get_long_debug (cia);
								console_out_f (_T("        %5s: %08x"), ciai[j], ciap);
								if (ciap) {
									uae_u8 *addr2 = get_real_address_debug(get_long_debug (ciap + 10));
									TCHAR *name2 = addr ? au ((char*)addr2) : au("<null>");
									console_out_f (_T(" (C=%08x D=%08X) '%s'"), get_long_debug (ciap + 18), get_long_debug (ciap + 14), name2);
									xfree (name2);
								}
								console_out_f (_T("\n"));
								cia += 4;
							}
						}
					}
					xfree (name);
					node = get_long_debug (node);
					cnt++;
				}
				if (!cnt)
					console_out_f (_T("  [S] <none>\n"));
			}
			list += 12;
		}
		return;
	} else if (c == 'e') { // expansion
		uaecptr expbase = get_base("expansion.library", 378);
		if (expbase) {
			if (t[1] == 'm') {
				uaecptr list = get_long_debug(expbase + 74);
				while (list && get_long_debug(list)) {
					uaecptr name = get_long(list + 10);
					uae_s8 pri = get_byte(list + 9);
					uae_u16 flags = get_word_debug(list + 14);
					uae_u32 dn = get_long_debug(list + 16);
					uae_u8 *addr = get_real_address_debug(name);
					TCHAR *name1 = addr ? au((char*)addr) : au("<null>");
					my_trim(name1);
					console_out_f(_T("%08x %04x %08x %d %s\n"), list, flags, dn, pri, name1);
					xfree(name1);
					list = get_long_debug(list);
				}
			} else {
				list = get_long_debug(expbase + 60);
				while (list && get_long_debug(list)) {
					uae_u32 addr = get_long_debug(list + 32);
					uae_u16 rom_vector = get_word_debug(list + 16 + 10);
					uae_u8 type = get_byte_debug(list + 16 + 0);
					console_out_f(_T("%02x %02x %08x %08x %04x %02x %08x %04x (%u/%u)\n"),
						type, get_byte_debug(list + 16 + 2),
						addr, get_long_debug(list + 36),
						get_word_debug(list + 16 + 4), get_byte_debug(list + 16 + 1),
						get_long_debug(list + 16 + 6), rom_vector,
						get_word_debug(list + 16 + 4), get_byte_debug(list + 16 + 1));
					for (int i = 0; i < 16; i++) {
						console_out_f(_T("%02x"), get_byte_debug(list + 16 + i));
						if (i < 15)
							console_out_f(_T("."));
					}
					console_out_f(_T("\n"));
					if ((type & 0x10)) {
						uae_u8 diagarea[256];
						uae_u16 nameoffset;
						uaecptr rom = addr + rom_vector;
						uae_u8 config = get_byte_debug(rom);
						copyromdata(config, rom, 0, diagarea, 16);
						nameoffset = (diagarea[8] << 8) | diagarea[9];
						console_out_f(_T(" %02x %02x Size %04x Diag %04x Boot %04x Name %04x %04x %04x\n"),
							diagarea[0], diagarea[1],
							(diagarea[2] << 8) | diagarea[3],
							(diagarea[4] << 8) | diagarea[5],
							(diagarea[6] << 8) | diagarea[7],
							nameoffset,
							(diagarea[10] << 8) | diagarea[11],
							(diagarea[12] << 8) | diagarea[13]);
						if (nameoffset != 0 && nameoffset != 0xffff) {
							copyromdata(config, rom, nameoffset, diagarea, 256);
							diagarea[sizeof diagarea - 1] = 0;
							TCHAR *str = au((char*)diagarea);
							console_out_f(_T(" '%s'\n"), str);
							xfree(str);
						}
					}
					list = get_long_debug(list);
				}
			}
		}
		return;
	} else if (c == 'R') { // residents
		list = get_long_debug(execbase + 300);
		while (list) {
			uaecptr resident = get_long_debug (list);
			if (!resident)
				break;
			if (resident & 0x80000000) {
				console_out_f (_T("-> %08X\n"), resident & 0x7fffffff);
				list = resident & 0x7fffffff;
				continue;
			}
			uae_u8 *addr;
			addr = get_real_address_debug(get_long_debug (resident + 14));
			TCHAR *name1 = addr ? au ((char*)addr) : au("<null>");
			my_trim (name1);
			addr = get_real_address_debug(get_long_debug (resident + 18));
			TCHAR *name2 = addr ? au ((char*)addr) : au("<null>");
			my_trim (name2);
			console_out_f (_T("%08X %08X: %02X %3d %02X %+3.3d '%s' ('%s')\n"),
				list, resident,
				get_byte_debug (resident + 10), get_byte_debug (resident + 11),
				get_byte_debug (resident + 12), (uae_s8)get_byte_debug (resident + 13),
				name1, name2);
			xfree (name2);
			xfree (name1);
			list += 4;
		}
		return;
	} else if (c == 'f' || c== 'F') { // filesystem.resource
		uaecptr fs = get_base ("FileSystem.resource", 336);
		if (fs) {
			static const TCHAR *fsnames[] = {
				_T("DosType"),
				_T("Version"),
				_T("PatchFlags"),
				_T("Type"),
				_T("Task"),
				_T("Lock"),
				_T("Handler"),
				_T("StackSize"),
				_T("Priority"),
				_T("Startup"),
				_T("SegList"),
				_T("GlobalVec"),
				NULL
			};
			uae_u8 *addr = get_real_address_debug(get_long_debug (fs + 14));
			TCHAR *name = addr ? au ((char*)addr) : au ("<null>");
			my_trim (name);
			console_out_f (_T("%08x: '%s'\n"), fs, name);
			xfree (name);
			node = get_long_debug (fs + 18);
			while (get_long_debug (node)) {
				TCHAR *name = au ((char*)get_real_address_debug(get_long_debug (node + 10)));
				my_trim (name);
				console_out_f (_T("%08x: '%s'\n"), node, name);
				xfree (name);
				for (int i = 0; fsnames[i]; i++) {
					uae_u32 v = get_long_debug (node + 14 + i * 4);
					console_out_f (_T("%16s = %08x %d\n"), fsnames[i], v, v);
				}
				console_out_f (_T("\n"));
				node = get_long_debug (node);
			}

		} else {
			console_out_f (_T("FileSystem.resource not found.\n"));
		}
		return;
	} else if (c == 'm' || c == 'M') { // memory
		list = execbase + 322;
		node = get_long_debug (list);
		while (get_long_debug (node)) {
			TCHAR *name = au ((char*)get_real_address_debug(get_long_debug (node + 10)));
			uae_u16 v = get_word_debug (node + 8);
			console_out_f (_T("%08x %d %d %s\n"), node, (int)((v >> 8) & 0xff), (uae_s8)(v & 0xff), name);
			xfree (name);
			console_out_f (_T("Attributes %04x First %08x Lower %08x Upper %08x Free %d\n"),
				get_word_debug (node + 14), get_long_debug (node + 16), get_long_debug (node + 20), 
				get_long_debug (node + 24), get_long_debug (node + 28));
			uaecptr mc = get_long_debug (node + 16);
			while (mc) {
				uae_u32 mc1 = get_long_debug (mc);
				uae_u32 mc2 = get_long_debug (mc + 4);
				console_out_f (_T(" %08x: %08x-%08x,%08x,%08x (%d)\n"), mc, mc, mc + mc2, mc1, mc2, mc2);
				mc = mc1;
			}
			console_out_f (_T("\n"));
			node = get_long_debug (node);
		}
		return;
	}

	bool full = false;
	switch (c)
	{
	case 'r': // resources
		list = execbase + 336;
		break;
	case 'd': // devices
		list = execbase + 350;
		full = true;
		break;
	case 'l': // libraries
		list = execbase + 378;
		full = true;
		break;
	case 'p': // ports
		list = execbase + 392;
		break;
	case 's': // semaphores
		list = execbase + 532;
		break;
	}
	if (list == 0)
		return;
	node = get_long_debug (list);
	while (get_long_debug (node)) {
		TCHAR *name = au ((char*)get_real_address_debug(get_long_debug (node + 10)));
		uae_u16 v = get_word_debug (node + 8);
		console_out_f (_T("%08x %d %d"), node, (int)((v >> 8) & 0xff), (uae_s8)(v & 0xff));
		if (full) {
			uae_u16 ver = get_word_debug(node + 20);
			uae_u16 rev = get_word_debug(node + 22);
			uae_u32 op = get_word_debug(node + 32);
			console_out_f(_T(" %d.%d %d"), ver, rev, op);
		}
		console_out_f(_T(" %s"), name);
		xfree (name);
		if (full) {
			uaecptr idstring = get_long_debug(node + 24);
			if (idstring) {
				name = au((char*)get_real_address_debug(idstring));
				console_out_f(_T(" (%s)"), name);
				xfree(name);
			}
		}
		console_out_f(_T("\n"));
		node = get_long_debug (node);
	}
}

static int debug_vpos = -1;
static int debug_hpos = -1;

static void breakfunc(uae_u32 v)
{
	write_log(_T("Cycle breakpoint hit\n"));
	debugging = 1;
	debug_vpos = -1;
	debug_hpos = -1;
	debug_cycles(2);
	set_special(SPCFLAG_BRK);
}

void debug_hsync(void)
{
	if (debug_vpos < 0) {
		return;
	}
	if (debug_vpos != vpos) {
		return;
	}
	if (debug_hpos <= 0) {
		breakfunc(0);
	} else {
		if (current_hpos() < debug_hpos) {
			event2_newevent_x(-1, debug_hpos - current_hpos(), 0, breakfunc);
		} else {
			breakfunc(0);
		}
	}
}

static int cycle_breakpoint(TCHAR **c)
{
	TCHAR nc = (*c)[0];
	next_char(c);
	if (more_params(c)) {
		int count = readint(c, NULL);
		if (nc == 's') {
			if (more_params(c)) {
				debug_vpos = count;
				debug_hpos = readint(c, NULL);
				if (debug_vpos == vpos && debug_hpos > current_hpos()) {
					debug_vpos = -1;
					count = debug_hpos - current_hpos();
					debug_hpos = -1;
				} else {
					return 1;
				}
			} else {
				count *= maxhpos;
			}
		}
		if (count > 0) {
			event2_newevent_x(-1, count, 0, breakfunc);
		}
		return 1;
	}
	return 0;
}

#if 0
static int trace_same_insn_count;
static uae_u8 trace_insn_copy[10];
static struct regstruct trace_prev_regs;
#endif
static uaecptr nextpc;

static void check_breakpoint_extra(TCHAR **c, struct breakpoint_node *bpn)
{
	bpn->cnt = 0;
	bpn->chain = -1;
	if (more_params(c)) {
		TCHAR nc = _totupper((*c)[0]);
		if (nc == 'N') {
			next_char(c);
			bpn->cnt = readint(c, NULL);
		}
	}
	if (more_params(c)) {
		TCHAR nc = _totupper((*c)[0]);
		if (nc == 'H') {
			next_char(c);
			bpn->chain = readint(c, NULL);
			if (bpn->chain < 0 || bpn->chain >= BREAKPOINT_TOTAL) {
				bpn->chain = -1;
			}
		}
	}
}

int instruction_breakpoint(TCHAR **c)
{
	struct breakpoint_node *bpn;
	int i;
	TCHAR next = 0;
	bool err;

	if (more_params (c)) {
		TCHAR nc = _totupper ((*c)[0]);
		if (nc == 'O') {
			// bpnum register operation value1 [mask value2]
			next_char(c);
			if (more_params(c)) {
				int bpidx = readint(c, NULL);
				if (more_params(c) && bpidx >= 0 && bpidx < BREAKPOINT_TOTAL) {
					bpn = &bpnodes[bpidx];
					int regid = getregidx(c);
					if (regid >= 0) {
						bpn->type = regid;
						bpn->mask = 0xffffffff;
						if (more_params(c)) {
							int operid = getoperidx(c, &bpn->opersigned);
							if (more_params(c) && operid >= 0) {
								bpn->oper = operid;
								bpn->value1 = readhex(c, NULL);
								bpn->enabled = 1;
								if (more_params(c)) {
									bpn->mask = readhex(c, NULL);
									if (more_params(c))  {
										bpn->value2 = readhex(c, NULL);
									}
								}
								check_breakpoint_extra(c, bpn);
								console_out(_T("Breakpoint added.\n"));
							}
						}
					}
				}
			}
			return 0;
		} else if (nc == 'I') {
			uae_u16 opcodes[32];
			next_char(c);
			ignore_ws(c);
			trace_param[1] = 0x10000;
			trace_param[2] = 0x10000;

			int w = m68k_asm(*c, opcodes, 0);
			if (w > 0) {
				trace_param[0] = opcodes[0];
				if (w > 1) {
					trace_param[1] = opcodes[1];
					if (w > 2) {
						trace_param[2] = opcodes[2];
					}
				}
			} else {
				if (more_params(c)) {
					trace_param[0] = readhex(c, NULL);
					if (more_params(c)) {
						trace_param[1] = readhex(c, NULL);
					}
					if (more_params(c)) {
						trace_param[2] = readhex(c, NULL);
					}
				} else {
					trace_param[0] = 0x10000;
				}
			}
			trace_mode = TRACE_MATCH_INS;
			return 1;
		} else if (nc == 'D' && (*c)[1] == 0) {
			for (i = 0; i < BREAKPOINT_TOTAL; i++) {
				bpnodes[i].enabled = 0;
			}
			console_out(_T("All breakpoints removed.\n"));
			return 0;
		} else if (nc == 'R' && (*c)[1] == 0) {
			if (more_params(c)) {
				int bpnum = readint(c, NULL);
				if (bpnum >= 0 && bpnum < BREAKPOINT_TOTAL) {
					bpnodes[bpnum].enabled = 0;
					console_out_f(_T("Breakpoint %d removed.\n"), bpnum);
				}
			}
			return 0;
		} else if (nc == 'L') {
			int got = 0;
			for (i = 0; i < BREAKPOINT_TOTAL; i++) {
				bpn = &bpnodes[i];
				if (!bpn->enabled)
					continue;
				console_out_f (_T("%d: %s %s %08x [%08x %08x]"), i, debugregs[bpn->type], debugoper[bpn->oper], bpn->value1, bpn->mask, bpn->value2);
				if (bpn->cnt > 0) {
					console_out_f(_T(" N=%d"), bpn->cnt);
				}
				if (bpn->chain > 0) {
					console_out_f(_T(" H=%d"), bpn->chain);
				}
				console_out_f(_T("\n"));
				got = 1;
			}
			if (!got)
				console_out (_T("No breakpoints.\n"));
			else
				console_out (_T("\n"));
			return 0;
		}
		trace_mode = TRACE_RANGE_PC;
		trace_param[0] = readhex(c, &err);
		if (err) {
			trace_mode = 0;
			return 0;
		}
		if (more_params (c)) {
			trace_param[1] = readhex(c, &err);
			if (!err) {
				return 1;
			}
		}
		for (i = 0; i < BREAKPOINT_TOTAL; i++) {
			bpn = &bpnodes[i];
			if (bpn->enabled && bpn->value1 == trace_param[0]) {
				bpn->enabled = 0;
				console_out (_T("Breakpoint removed.\n"));
				trace_mode = 0;
				return 0;
			}
		}
		for (i = 0; i < BREAKPOINT_TOTAL; i++) {
			bpn = &bpnodes[i];
			if (bpn->enabled)
				continue;
			bpn->value1 = trace_param[0];
			bpn->type = BREAKPOINT_REG_PC;
			bpn->oper = BREAKPOINT_CMP_EQUAL;
			bpn->enabled = 1;
			check_breakpoint_extra(c, bpn);
			console_out (_T("Breakpoint added.\n"));
			trace_mode = 0;
			break;
		}
		return 0;
	}
	trace_mode = TRACE_RAM_PC;
	return 1;
}

static int process_breakpoint(TCHAR **c)
{
	bool err;
	processptr = 0;
	xfree(processname);
	processname = NULL;
	if (!more_params(c))
		return 0;
	if (**c == '\"') {
		TCHAR pn[200];
		next_string(c, pn, sizeof(pn) / sizeof(TCHAR), 0);
		processname = ua(pn);
	} else {
		processptr = readhex(c, &err);
		if (err) {
			return 0;
		}
	}
	trace_mode = TRACE_CHECKONLY;
	return 1;
}

static void saveloadmem (TCHAR **cc, bool save)
{
	uae_u8 b;
	uae_u32 src, src2;
	int len, len2;
	TCHAR name[MAX_DPATH];
	FILE *fp;

	if (!more_params (cc))
		goto S_argh;
	next_string(cc, name, sizeof(name) / sizeof(TCHAR), 0);
	if (!more_params (cc))
		goto S_argh;
	src2 = src = readhex(cc, NULL);
	if (save) {
		if (!more_params(cc))
			goto S_argh;
	}
	len2 = len = -1;
	if (more_params(cc)) {
		len2 = len = readhex(cc, NULL);
	}
	fp = uae_tfopen (name, save ? _T("wb") : _T("rb"));
	if (fp == NULL) {
		console_out_f (_T("Couldn't open file '%s'.\n"), name);
		return;
	}
	if (save) {
		while (len > 0) {
			b = get_byte_debug (src);
			src++;
			len--;
			if (fwrite (&b, 1, 1, fp) != 1) {
				console_out (_T("Error writing file.\n"));
				break;
			}
		}
		if (len == 0)
			console_out_f (_T("Wrote %08X - %08X (%d bytes) to '%s'.\n"),
				src2, src2 + len2, len2, name);
	} else {
		len2 = 0;
		while (len != 0) {
			if (fread(&b, 1, 1, fp) != 1) {
				if (len > 0)
					console_out (_T("Unexpected end of file.\n"));
				len = 0;
				break;
			}
			put_byte (src, b);
			src++;
			if (len > 0)
				len--;
			len2++;
		}
		if (len == 0)
			console_out_f (_T("Read %08X - %08X (%d bytes) to '%s'.\n"),
				src2, src2 + len2, len2, name);
	}
	fclose (fp);
	return;
S_argh:
	console_out (_T("Command needs more arguments!\n"));
}

static void searchmem (TCHAR **cc)
{
	int i, sslen, got, val, stringmode;
	uae_u8 ss[256];
	uae_u32 addr, endaddr;
	TCHAR nc;

	got = 0;
	sslen = 0;
	stringmode = 0;
	ignore_ws (cc);
	while(more_params(cc)) {
		if (**cc == '"' || **cc == '\'') {
			TCHAR quoted = **cc;
			stringmode = 1;
			(*cc)++;
			while (**cc != quoted && **cc != 0) {
				ss[sslen++] = tolower(**cc);
				(*cc)++;
			}
			if (**cc != 0) {
				(*cc)++;
			}
		} else {
			for (;;) {
				if (**cc == 32 || **cc == 0) {
					break;
				}
				if (**cc == '"' || **cc == '\'') {
					break;
				}
				nc = _totupper(next_char(cc));
				if (isspace(nc))
					break;
				if (isdigit(nc))
					val = nc - '0';
				else
					val = nc - 'A' + 10;
				if (val < 0 || val > 15)
					return;
				val *= 16;
				if (**cc == 32 || **cc == 0)
					break;
				nc = _totupper(next_char(cc));
				if (isspace(nc))
					break;
				if (isdigit(nc))
					val += nc - '0';
				else
					val += nc - 'A' + 10;
				if (val < 0 || val > 255)
					return;
				ss[sslen++] = (uae_u8)val;
			}
		}
		if (**cc == 0 || **cc == 32) {
			break;
		}
		if (**cc != '"' && **cc != '\'') {
			(*cc)++;
		}
	}
	if (sslen == 0)
		return;
	ignore_ws (cc);
	addr = 0xffffffff;
	endaddr = lastaddr(addr);
	if (more_params (cc)) {
		addr = readhex(cc, NULL);
		addr--;
		endaddr = lastaddr(addr);
		if (more_params(cc)) {
			endaddr = readhex(cc, NULL);
		}
	}
	console_out_f (_T("Searching from %08X to %08X..\n"), addr + 1, endaddr);
	nextaddr_init(addr);
	bool out = false;
	while ((addr = nextaddr(addr, endaddr, NULL, true, &out)) != 0xffffffff) {
		if (addr == endaddr)
			break;
		for (i = 0; i < sslen; i++) {
			uae_u8 b = get_byte_debug (addr + i);
			if (stringmode) {
				if (tolower (b) != ss[i])
					break;
			} else {
				if (b != ss[i])
					break;
			}
		}
		if (i == sslen) {
			got++;
			console_out_f (_T(" %08X"), addr);
			out = true;
			if (got > 100) {
				console_out (_T("\nMore than 100 results, aborting.."));
				break;
			}
		}
		if  (iscancel (65536)) {
			console_out_f (_T("Aborted at %08X\n"), addr);
			break;
		}
	}
	if (!got)
		console_out (_T("nothing found"));
	console_out (_T("\n"));
}

static int staterecorder (TCHAR **cc)
{
#if 0
	TCHAR nc;

	if (!more_params (cc)) {
		if (savestate_dorewind (1)) {
			debug_rewind = 1;
			return 1;
		}
		return 0;
	}
	nc = next_char (cc);
	if (nc == 'l') {
		savestate_listrewind ();
		return 0;
	}
#endif
	return 0;
}

static int debugtest_modes[DEBUGTEST_MAX];
static const TCHAR *debugtest_names[] = {
	_T("Blitter"), _T("Keyboard"), _T("Floppy")
};

void debugtest (enum debugtest_item di, const TCHAR *format, ...)
{
	va_list parms;
	TCHAR buffer[1000];

	if (!debugtest_modes[di])
		return;
	va_start (parms, format);
	_vsntprintf (buffer, 1000 - 1, format, parms);
	va_end (parms);
	write_log (_T("%s PC=%08X: %s\n"), debugtest_names[di], M68K_GETPC, buffer);
	if (debugtest_modes[di] == 2)
		activate_debugger_new();
}

static void debugtest_set (TCHAR **inptr)
{
	int i, val, val2;
	ignore_ws (inptr);

	val2 = 1;
	if (!more_params (inptr)) {
		for (i = 0; i < DEBUGTEST_MAX; i++)
			debugtest_modes[i] = 0;
		console_out (_T("All debugtests disabled\n"));
		return;
	}
	val = readint(inptr, NULL);
	if (more_params (inptr)) {
		val2 = readint(inptr, NULL);
		if (val2 > 0)
			val2 = 2;
	}
	if (val < 0) {
		for (i = 0; i < DEBUGTEST_MAX; i++)
			debugtest_modes[i] = val2;
		console_out (_T("All debugtests enabled\n"));
		return;
	}
	if (val >= 0 && val < DEBUGTEST_MAX) {
		if (debugtest_modes[val])
			debugtest_modes[val] = 0;
		else
			debugtest_modes[val] = val2;
		console_out_f (_T("Debugtest '%s': %s. break = %s\n"),
			debugtest_names[val], debugtest_modes[val] ? _T("on") :_T("off"), val2 == 2 ? _T("on") : _T("off"));
	}
}

static void debug_sprite (TCHAR **inptr)
{
	uaecptr saddr, addr, addr2;
	int xpos, xpos_ecs;
	int ypos, ypos_ecs;
	int ypose, ypose_ecs;
	int attach;
	uae_u64 w1, w2, ww1, ww2;
	int size = 1, width;
	int ecs, sh10;
	int y, i;
	TCHAR tmp[80];
	int max = 14;

	addr2 = 0;
	ignore_ws(inptr);
	addr = readhex(inptr, NULL);
	ignore_ws(inptr);
	if (more_params (inptr))
		size = readhex(inptr, NULL);
	if (size != 1 && size != 2 && size != 4) {
		addr2 = size;
		ignore_ws(inptr);
		if (more_params(inptr))
			size = readint(inptr, NULL);
		if (size != 1 && size != 2 && size != 4)
			size = 1;
	}
	for (;;) {
		ecs = 0;
		sh10 = 0;
		saddr = addr;
		width = size * 16;
		w1 = get_word_debug (addr);
		w2 = get_word_debug (addr + size * 2);
		console_out_f (_T("    %06X "), addr);
		for (i = 0; i < size * 2; i++)
			console_out_f (_T("%04X "), get_word_debug (addr + i * 2));
		console_out_f (_T("\n"));

		ypos = (int)(w1 >> 8);
		xpos = w1 & 255;
		ypose = (int)(w2 >> 8);
		attach = (w2 & 0x80) ? 1 : 0;
		if (w2 & 4)
			ypos |= 256;
		if (w2 & 2)
			ypose |= 256;
		ypos_ecs = ypos;
		ypose_ecs = ypose;
		if (w2 & 0x40)
			ypos_ecs |= 512;
		if (w2 & 0x20)
			ypose_ecs |= 512;
		xpos <<= 1;
		if (w2 & 0x01)
			xpos |= 1;
		xpos_ecs = xpos << 2;
		if (w2 & 0x10)
			xpos_ecs |= 2;
		if (w2 & 0x08)
			xpos_ecs |= 1;
		if (w2 & (0x40 | 0x20 | 0x10 | 0x08))
			ecs = 1;
		if (w1 & 0x80)
			sh10 = 1;
		if (ypose < ypos)
			ypose += 256;

		if (ecs_agnus) {
			ypos = ypos_ecs;
			ypose = ypose_ecs;
		}

		for (y = ypos; y < ypose; y++) {
			int x;
			addr += size * 4;
			if (addr2)
				addr2 += size * 4;
			if (size == 1) {
				w1 = get_word_debug (addr);
				w2 = get_word_debug (addr + 2);
				if (addr2) {
					ww1 = get_word_debug (addr2);
					ww2 = get_word_debug (addr2 + 2);
				}
			} else if (size == 2) {
				w1 = get_long_debug (addr);
				w2 = get_long_debug (addr + 4);
				if (addr2) {
					ww1 = get_long_debug (addr2);
					ww2 = get_long_debug (addr2 + 4);
				}
			} else if (size == 4) {
				w1 = get_long_debug (addr + 0);
				w2 = get_long_debug (addr + 8);
				w1 <<= 32;
				w2 <<= 32;
				w1 |= get_long_debug (addr + 4);
				w2 |= get_long_debug (addr + 12);
				if (addr2) {
					ww1 = get_long_debug (addr2 + 0);
					ww2 = get_long_debug (addr2 + 8);
					ww1 <<= 32;
					ww2 <<= 32;
					ww1 |= get_long_debug (addr2 + 4);
					ww2 |= get_long_debug (addr2 + 12);
				}
			}
			width = size * 16;
			for (x = 0; x < width; x++) {
				int v1 = w1 & 1;
				int v2 = w2 & 1;
				int v = v2 * 2 + v1;
				w1 >>= 1;
				w2 >>= 1;
				if (addr2) {
					int vv1 = ww1 & 1;
					int vv2 = ww2 & 1;
					int vv = vv2 * 2 + vv1;
					ww1 >>= 1;
					ww2 >>= 1;
					v *= 4;
					v += vv;
					tmp[width - (x + 1)] = v >= 10 ? 'A' + v - 10 : v + '0';
				} else {
					tmp[width - (x + 1)] = v + '0';
				}
			}
			tmp[width] = 0;
			console_out_f (_T("%3d %06X %s\n"), y, addr, tmp);
		}

		console_out_f (_T("Sprite address %08X, width = %d\n"), saddr, size * 16);
		console_out_f (_T("OCS: StartX=%d StartY=%d EndY=%d\n"), xpos, ypos, ypose);
		console_out_f (_T("ECS: StartX=%d (%d.%d) StartY=%d EndY=%d%s\n"), xpos_ecs, xpos_ecs / 4, xpos_ecs & 3, ypos_ecs, ypose_ecs, ecs ? _T(" (*)") : _T(""));
		console_out_f (_T("Attach: %d. AGA SSCAN/SH10 bit: %d\n"), attach, sh10);

		addr += size * 4;
		if (get_word_debug (addr) == 0 && get_word_debug (addr + size * 4) == 0)
			break;
		max--;
		if (max <= 0) {
			console_out_f(_T("Max sprite count reached.\n"));
			break;
		}
	}

}

int debug_write_memory_16 (uaecptr addr, uae_u16 v)
{
	addrbank *ad;
	
	ad = &get_mem_bank (addr);
	if (ad) {
		ad->wput (addr, v);
		return 1;
	}
	return -1;
}
int debug_write_memory_8 (uaecptr addr, uae_u8 v)
{
	addrbank *ad;
	
	ad = &get_mem_bank (addr);
	if (ad) {
		ad->bput (addr, v);
		return 1;
	}
	return -1;
}
int debug_peek_memory_16 (uaecptr addr)
{
	addrbank *ad;
	
	ad = &get_mem_bank (addr);
	if (ad->flags & (ABFLAG_RAM | ABFLAG_ROM | ABFLAG_ROMIN | ABFLAG_SAFE))
		return ad->wget (addr);
	if (ad == &custom_bank) {
		addr &= 0x1fe;
		return (ar_custom[addr + 0] << 8) | ar_custom[addr + 1];
	}
	return -1;
}
int debug_read_memory_16 (uaecptr addr)
{
	addrbank *ad;
	
	ad = &get_mem_bank (addr);
	if (ad)
		return ad->wget (addr);
	return -1;
}
int debug_read_memory_8 (uaecptr addr)
{
	addrbank *ad;
	
	ad = &get_mem_bank (addr);
	if (ad)
		return ad->bget (addr);
	return -1;
}

static void disk_debug (TCHAR **inptr)
{
	TCHAR parm[10];
	int i;

	if (**inptr == 'd') {
		(*inptr)++;
		ignore_ws (inptr);
		disk_debug_logging = readint(inptr, NULL);
		console_out_f (_T("Disk logging level %d\n"), disk_debug_logging);
		return;
	}
	disk_debug_mode = 0;
	disk_debug_track = -1;
	ignore_ws (inptr);
	if (!next_string (inptr, parm, sizeof (parm) / sizeof (TCHAR), 1))
		goto end;
	for (i = 0; i < _tcslen(parm); i++) {
		if (parm[i] == 'R')
			disk_debug_mode |= DISK_DEBUG_DMA_READ;
		if (parm[i] == 'W')
			disk_debug_mode |= DISK_DEBUG_DMA_WRITE;
		if (parm[i] == 'P')
			disk_debug_mode |= DISK_DEBUG_PIO;
	}
	if (more_params(inptr))
		disk_debug_track = readint(inptr, NULL);
	if (disk_debug_track < 0 || disk_debug_track > 2 * 83)
		disk_debug_track = -1;
	if (disk_debug_logging == 0)
		disk_debug_logging = 1;
end:
	console_out_f (_T("Disk breakpoint mode %c%c%c track %d\n"),
		disk_debug_mode & DISK_DEBUG_DMA_READ ? 'R' : '-',
		disk_debug_mode & DISK_DEBUG_DMA_WRITE ? 'W' : '-',
		disk_debug_mode & DISK_DEBUG_PIO ? 'P' : '-',
		disk_debug_track);
}

static void find_ea (TCHAR **inptr)
{
	uae_u32 ea, sea, dea;
	uaecptr addr, end, end2;
	int hits = 0;
	bool err;

	addr = 0xffffffff;
	end = lastaddr(addr);
	ea = readhex(inptr, &err);
	if (err) {
		return;
	}
	if (more_params(inptr)) {
		addr = readhex(inptr, &err);
		if (err) {
			return;
		}
		addr--;
		end = lastaddr(addr);
		if (more_params(inptr)) {
			end = readhex(inptr, &err);
			if (err) {
				return;
			}
		}
	}
	console_out_f (_T("Searching from %08X to %08X\n"), addr + 1, end);
	end2 = 0;
	nextaddr_init(addr);
	bool out = false;
	while((addr = nextaddr(addr, end, &end2, true, &out)) != 0xffffffff) {
		if ((addr & 1) == 0 && addr + 6 <= end2) {
			sea = 0xffffffff;
			dea = 0xffffffff;
			m68k_disasm_ea (addr, NULL, 1, &sea, &dea, 0xffffffff);
			if (ea == sea || ea == dea) {
				m68k_disasm (addr, NULL, 0xffffffff, 1);
				out = true;
				hits++;
				if (hits > 100) {
					console_out_f (_T("Too many hits. End addr = %08X\n"), addr);
					break;
				}
			}
			if  (iscancel (65536)) {
				console_out_f (_T("Aborted at %08X\n"), addr);
				break;
			}
		}
	}
}

static void m68k_modify (TCHAR **inptr)
{
	uae_u32 v;
	TCHAR parm[10];
	TCHAR c1, c2;
	int i;

	if (!next_string (inptr, parm, sizeof (parm) / sizeof (TCHAR), 1))
		return;
	c1 = _totupper (parm[0]);
	c2 = 99;
	if (c1 == 'A' || c1 == 'D' || c1 == 'P') {
		c2 = _totupper (parm[1]);
		if (isdigit (c2))
			c2 -= '0';
		else
			c2 = 99;
	}
	v = readhex(inptr, NULL);
	if (c1 == 'A' && c2 < 8)
		regs.regs[8 + c2] = v;
	else if (c1 == 'D' && c2 < 8)
		regs.regs[c2] = v;
	else if (c1 == 'P' && c2 == 0)
		regs.irc = v;
	else if (c1 == 'P' && c2 == 1)
		regs.ir = v;
	else if (!_tcscmp (parm, _T("SR"))) {
		regs.sr = v;
		MakeFromSR ();
	} else if (!_tcscmp (parm, _T("CCR"))) {
		regs.sr = (regs.sr & ~31) | (v & 31);
		MakeFromSR ();
	} else if (!_tcscmp (parm, _T("USP"))) {
		regs.usp = v;
	} else if (!_tcscmp (parm, _T("ISP"))) {
		regs.isp = v;
	} else if (!_tcscmp (parm, _T("PC"))) {
		m68k_setpc (v);
		fill_prefetch ();
	} else {
		for (i = 0; m2cregs[i].regname; i++) {
			if (!_tcscmp (parm, m2cregs[i].regname))
				val_move2c2 (m2cregs[i].regno, v);
		}
	}
}

static void ppc_disasm(uaecptr addr, uaecptr *nextpc, int cnt)
{
#ifdef WITH_PPC
	PPCD_CB disa;

	while(cnt-- > 0) {
		uae_u32 instr = get_long_debug(addr);
		disa.pc = addr;
		disa.instr = instr;
		PPCDisasm(&disa);
		TCHAR *mnemo = au(disa.mnemonic);
		TCHAR *ops = au(disa.operands);
		console_out_f(_T("%08X  %08X  %-12s%-30s\n"), addr, instr, mnemo, ops);
		xfree(ops);
		xfree(mnemo);
		addr += 4;
	}
	if (nextpc)
		*nextpc = addr;
#endif
}

static void dma_disasm(int frames, int vp, int hp, int frames_end, int vp_end, int hp_end)
{
	if (!dma_record_data || frames < 0 || vp < 0 || hp < 0) {
		return;
	}
	struct dma_rec *drs = find_dma_record(hp, vp, 0);
	if (!drs) {
		return;
	}
	struct dma_rec *dr = drs;
	for (;;) {
		TCHAR l1[30], l1b[30], l1c[30], l2[30], l3[30], l4[30], l5[30], l6[30];
		if (get_record_dma_info(drs, dr, l1, l1b, l1c, l2, l3, l4, l5, l6, NULL, NULL)) {
			TCHAR tmp[256];
			_sntprintf(tmp, sizeof tmp, _T(" - %03d %s"), dr->hpos, l2);
			while (_tcslen(tmp) < 20) {
				_tcscat(tmp, _T(" "));
			}
			console_out_f(_T("%s %11s %11s\n"), tmp, l3, l4);
		}
		dr++;
		if (dr == dma_record_data + NR_DMA_REC_MAX) {
			dr = dma_record_data;
		}
		if (vp_end < 0 || hp_end < 0 || frames_end < 0) {
			break;
		}
		int vpc = dr->vpos[dma_record_vpos_type];
		if (vpc > vp_end || vpc < vp) {
			break;
		}
		if (vpc == vp_end && dr->hpos >= hp_end) {
			break;
		}
	}
}

static uaecptr nxdis, nxmem, asmaddr;
static bool ppcmode, asmmode;

static bool parsecmd(TCHAR *cmd, bool *out)
{
	if (!_tcsicmp(cmd, _T("reset"))) {
		deactivate_debugger();
		debug_continue();
		uae_reset(0, 0);
		return true;
	}
	if (!_tcsicmp(cmd, _T("reseth"))) {
		deactivate_debugger();
		debug_continue();
		uae_reset(1, 0);
		return true;
	}
	if (!_tcsicmp(cmd, _T("resetk"))) {
		deactivate_debugger();
		debug_continue();
		uae_reset(0, 1);
		return true;
	}
	return false;
}

static bool debug_line (TCHAR *input)
{
	TCHAR cmd, *inptr;
	uaecptr addr;
	bool err;

	inptr = input;

	if (asmmode) {
		if (more_params(&inptr)) {
			if (!_tcsicmp(inptr, _T("x"))) {
				asmmode = false;
				return false;
			}
			uae_u16 asmout[16];
			int inss = m68k_asm(inptr, asmout, asmaddr);
			if (inss > 0) {
				for (int i = 0; i < inss; i++) {
					put_word(asmaddr + i * 2, asmout[i]);
				}
				m68k_disasm(asmaddr, &nxdis, 0xffffffff, 1);
				asmaddr = nxdis;
			}
			console_out_f(_T("%08X "), asmaddr);
			return false;
		} else {
			asmmode = false;
			return false;
		}
	}

	ignore_ws(&inptr);
	if (parsecmd(inptr, &err)) {
		return err;
	}
	cmd = next_char (&inptr);

	switch (cmd)
	{
		case 'I':
		if (more_params (&inptr)) {
			static int recursive;
			if (!recursive) {
				recursive++;
				handle_custom_event(inptr, 0);
				device_check_config();
				recursive--;
			}
		}
		break;
		case 'c': dumpcia (); dumpdisk (_T("DEBUG")); dumpcustom (); break;
		case 'i':
		{
			if (*inptr == 'l') {
				next_char (&inptr);
				if (more_params (&inptr)) {
					debug_illegal_mask = readhex(&inptr, NULL);
					if (more_params(&inptr))
						debug_illegal_mask |= ((uae_u64)readhex(&inptr, NULL)) << 32;
				} else {
					debug_illegal_mask = debug_illegal ? 0 : -1;
					debug_illegal_mask &= ~((uae_u64)255 << 24); // mask interrupts
				}
				console_out_f (_T("Exception breakpoint mask: %08X %08X\n"), (uae_u32)(debug_illegal_mask >> 32), (uae_u32)debug_illegal_mask);
				debug_illegal = debug_illegal_mask ? 1 : 0;
			} else {
				addr = 0xffffffff;
				if (more_params (&inptr))
					addr = readhex (&inptr, NULL);
				dump_vectors (addr);
			}
			break;
		}
		case 'e':
		{
			bool aga = tolower(*inptr) == 'a';
			if (aga)
				next_char(&inptr);
			bool ext = tolower(*inptr) == 'x';
			dump_custom_regs(aga, ext);
		}
		break;
		case 'r':
		{
			if (*inptr == 'c') {
				next_char(&inptr);
				m68k_dumpcache(*inptr == 'd');
			} else if (*inptr == 's') {
				if (*(inptr + 1) == 's')
					debugmem_list_stackframe(true);
				else
					debugmem_list_stackframe(false);
			} else if (more_params(&inptr)) {
				m68k_modify(&inptr);
			} else {
				custom_dumpstate(0);
				m68k_dumpstate(&nextpc, 0xffffffff);
			}
		}
		break;
		case 'D': deepcheatsearch (&inptr); break;
		case 'C': cheatsearch (&inptr); break;
		case 'W': writeintomem (&inptr); break;
		case 'w': memwatch (&inptr); break;
		case 'S': saveloadmem (&inptr, true); break;
		case 'L': saveloadmem (&inptr, false); break;
		case 's':
			if (*inptr == 'e' && *(inptr + 1) == 'g') {
				next_char(&inptr);
				next_char(&inptr);
				addr = 0xffffffff;
				if (*inptr == 's') {
					debugmem_list_segment(1, addr);
				} else {
					if (more_params(&inptr)) {
						addr = readhex(&inptr, NULL);
					}
					debugmem_list_segment(0, addr);
				}
			} else if (*inptr == 'c') {
				screenshot(-1, 1, 1);
			} else if (*inptr == 'p') {
				inptr++;
				debug_sprite (&inptr);
			} else if (*inptr == 'm') {
				if (*(inptr + 1) == 'c') {
					next_char (&inptr);
					next_char (&inptr);
					if (!smc_table)
						smc_detect_init (&inptr);
					else
						smc_free ();
				}
			} else {
				searchmem (&inptr);
			}
			break;
		case 'a':
			asmaddr = nxdis;
			if (more_params(&inptr)) {
				asmaddr = readhex(&inptr, &err);
				if (err) {
					break;
				}
				if (more_params(&inptr)) {
					uae_u16 asmout[16];
					int inss = m68k_asm(inptr, asmout, asmaddr);
					if (inss > 0) {
						for (int i = 0; i < inss; i++) {
							put_word(asmaddr + i * 2, asmout[i]);
						}
						m68k_disasm(asmaddr, &nxdis, 1, 0xffffffff);
						asmaddr = nxdis;
						return false;
					}
				}
			}
			asmmode = true;
			console_out_f(_T("%08X "), asmaddr);
		break;
		case 'd':
			{
				if (*inptr == 'i') {
					next_char (&inptr);
					disk_debug (&inptr);
				} else if (*inptr == 'j') {
					inptr++;
					inputdevice_logging = 1 | 2;
					if (more_params (&inptr))
						inputdevice_logging = readint(&inptr, NULL);
					console_out_f (_T("Input logging level %d\n"), inputdevice_logging);
				} else if (*inptr == 'm') {
					memory_map_dump_2 (0);
				} else if (*inptr == 't') {
					next_char (&inptr);
					debugtest_set (&inptr);
#ifdef _WIN32
				} else if (*inptr == 'g') {
					extern void update_disassembly (uae_u32);
					next_char (&inptr);
					if (more_params (&inptr))
						update_disassembly(readhex (&inptr, NULL));
#endif
				} else {
					uae_u32 daddr;
					int count;
					if (*inptr == 'p' && inptr[1] == 'p' && inptr[2] == 'c') {
						ppcmode = true;
						next_char(&inptr);
					} else if(*inptr == 'o') {
						ppcmode = false;
						next_char(&inptr);
					}
					if (more_params (&inptr))
						daddr = readhex(&inptr, NULL);
					else
						daddr = nxdis;
					if (more_params (&inptr))
						count = readhex(&inptr, NULL);
					else
						count = 10;
					if (ppcmode) {
						ppc_disasm(daddr, &nxdis, count);
					} else {
						m68k_disasm (daddr, &nxdis, 0xffffffff, count);
					}
				}
			}
			break;
		case 'T':
			if (inptr[0] == 'L')
				debugger_scan_libraries();
			else if (inptr[0] == 't' || inptr[0] == 0)
				show_exec_tasks ();
			else
				show_exec_lists (&inptr[0]);
			break;
		case 't':
			no_trace_exceptions = 0;
			debug_cycles(2);
			trace_param[0] = trace_param[1] = 0;
			if (*inptr == 't') {
				no_trace_exceptions = 1;
				inptr++;
			}
			if (*inptr == 'r') {
				// break when PC in debugmem
				if (debugmem_get_range(&trace_param[0], &trace_param[1])) {
					trace_mode = TRACE_RANGE_PC;
					return true;
				}
			} else if (*inptr == 's') {
				if (*(inptr + 1) == 'e') {
					debugmem_enable_stackframe(true);
				} else if (*(inptr + 1) == 'd') {
					debugmem_enable_stackframe(false);
				} else if (*(inptr + 1) == 'p') {
					if (debugmem_break_stack_pop()) {
						debugger_active = 0;
						return true;
					}
				} else {
					if (debugmem_break_stack_pop()) {
						debugger_active = 0;
						return true;
					}
				}
			} else if (*inptr == 'l') {
				// skip next source line
				if (debugmem_isactive()) {
					trace_mode = TRACE_SKIP_LINE;
					trace_param[0] = 1;
					trace_param[1] = debugmem_get_sourceline(M68K_GETPC, NULL, 0);
					return true;
				}
			} else if (*inptr == 'x') {
				trace_mode = TRACE_SKIP_INS;
				trace_param[0] = 0xffffffff;
				exception_debugging = 1;
				return true;
			} else {
				if (more_params(&inptr))
					trace_param[0] = readint(&inptr, NULL);
				if (trace_param[0] <= 0 || trace_param[0] > 10000)
					trace_param[0] = 1;
				trace_mode = TRACE_SKIP_INS;
				exception_debugging = 1;
				return true;
			}
			break;
		case 'z':
			trace_mode = TRACE_MATCH_PC;
			trace_param[0] = nextpc;
			exception_debugging = 1;
			debug_cycles(2);
			return true;

		case 'f':
			if (inptr[0] == 'a') {
				next_char (&inptr);
				find_ea (&inptr);
			} else if (inptr[0] == 'p') {
				inptr++;
				if (process_breakpoint (&inptr))
					return true;
			} else if (inptr[0] == 'c' || inptr[0] == 's') {
				if (cycle_breakpoint(&inptr)) {
					return true;
				}
			} else if (inptr[0] == 'e' && inptr[1] == 'n') {
				break_if_enforcer = break_if_enforcer ? false : true;
				console_out_f(_T("Break when enforcer hit: %s\n"), break_if_enforcer ? _T("enabled") : _T("disabled"));
			} else {
				if (instruction_breakpoint(&inptr)) {
					debug_cycles(1);
					return true;
				}
			}
			break;

		case 'q':
			uae_quit();
			deactivate_debugger();
			return true;

		case 'g':
			if (more_params (&inptr)) {
				uaecptr addr = readhex(&inptr, &err);
				if (err) {
					break;
				}
				m68k_setpc(addr);
				fill_prefetch();
			}
			deactivate_debugger();
			return true;

		case 'x':
			if (_totupper(inptr[0]) == 'X') {
				debugger_change(-1);
			} else {
				deactivate_debugger();
				close_console();
				return true;
			}
			break;

		case 'H':
			{
				int count, temp, badly, skip, dmadbg;
				uae_u32 addr = 0;
				uae_u32 oldpc = m68k_getpc ();
				int lastframes, lastvpos, lasthpos;
				struct regstruct save_regs = regs;

				badly = 0;
				if (inptr[0] == 'H') {
					badly = 1;
					inptr++;
				}
				dmadbg = 0;
				if (inptr[0] == 'D') {
					dmadbg = 1;
					inptr++;
				}

				if (more_params(&inptr))
					count = readint(&inptr, NULL);
				else
					count = 10;
				if (count > 1000) {
					addr = count;
					count = MAX_HIST;
				}
				if (count < 0)
					break;
				skip = count;
				if (more_params (&inptr))
					skip = count - readint(&inptr, NULL);

				temp = lasthist;
				while (count-- > 0 && temp != firsthist) {
					if (temp == 0)
						temp = MAX_HIST - 1;
					else
						temp--;
				}
				lastframes = lastvpos = lasthpos = -1;
				while (temp != lasthist) {
					regs = history[temp].regs;
					if (regs.pc == addr || addr == 0) {
						m68k_setpc (regs.pc);
						if (badly) {
							m68k_dumpstate(NULL, 0xffffffff);
						} else {
							if (dmadbg && lastvpos >= 0) {
								dma_disasm(lastframes, lastvpos, lasthpos, history[temp].fp, history[temp].vpos, history[temp].hpos);
							}
							lastframes = history[temp].fp;
							lastvpos = history[temp].vpos;
							lasthpos = history[temp].hpos;
							console_out_f(_T("%2d %03d/%03d "), regs.intmask ? regs.intmask : (regs.s ? -1 : 0), lasthpos, lastvpos);
							m68k_disasm (regs.pc, NULL, 0xffffffff, 1);
						}
						if (addr && regs.pc == addr)
							break;
					}
					if (skip-- < 0)
						break;
					if (++temp == MAX_HIST)
						temp = 0;
				}
				regs = save_regs;
				m68k_setpc (oldpc);
			}
			break;
		case 'M':
			if (more_params (&inptr)) {
				switch (next_char (&inptr))
				{
				case 'a':
					if (more_params (&inptr))
						audio_channel_mask = readhex(&inptr, NULL);
					console_out_f (_T("Audio mask = %02X\n"), audio_channel_mask);
					break;
				case 's':
					if (more_params (&inptr)) {
						debug_sprite_mask_val = readhex(&inptr, NULL);
						debug_sprite_mask = 0;
						for (int i = 0; i < 8; i++) {
							if (debug_sprite_mask_val & (1 << i)) {
								debug_sprite_mask |= (3 << (i * 2)) | (1 << (i + 16));
							}
						}
					}
					console_out_f (_T("Sprite mask: %02X\n"), debug_sprite_mask_val);
					break;
				case 'b':
					if (more_params(&inptr)) {
						debug_bpl_mask = readhex(&inptr, NULL) & 0xff;
						if (more_params(&inptr))
							debug_bpl_mask_one = readhex(&inptr, NULL) & 0xff;
						notice_screen_contents_lost(0);
					}
					console_out_f (_T("Bitplane mask: %02X (%02X)\n"), debug_bpl_mask, debug_bpl_mask_one);
					break;
				}
			}
			break;
		case 'm':
			{
				uae_u32 maddr;
				int lines;
#ifdef _WIN32
				if (*inptr == 'g') {
					extern void update_memdump (uae_u32);
					next_char (&inptr);
					if (more_params(&inptr)) {
						uaecptr addr = readhex(&inptr, &err);
						if (!err) {
							update_memdump(addr);
						}
					}
					break;
				}
#endif
				if (*inptr == 'm' && inptr[1] == 'u') {
					inptr += 2;
					if (inptr[0] == 'd') {
						if (currprefs.mmu_model >= 68040)
							mmu_dump_tables();
					} else {
						if (currprefs.mmu_model) {
							if (more_params (&inptr))
								debug_mmu_mode = readint(&inptr, NULL);
							else
								debug_mmu_mode = 0;
							console_out_f (_T("MMU translation function code = %d\n"), debug_mmu_mode);
						}
					}
					break;
				}
				err = false;
				if (more_params (&inptr)) {
					maddr = readhex(&inptr, &err);
				} else {
					maddr = nxmem;
				}
				if (more_params (&inptr))
					lines = readhex(&inptr, &err);
				else
					lines = 20;
				if (!err) {
					dumpmem(maddr, &nxmem, lines);
				}
			}
			break;
		case 'v':
		case 'V':
			{
				static int v1 = 0, v2 = 0, v3 = 0;
				if (*inptr == 'h') {
					inptr++;
					if (more_params(&inptr) && *inptr == '?') {
						mw_help();
					} else if (!heatmap) {
						debug_heatmap = 1;
						init_heatmap();
						if (more_params(&inptr)) {
							v1 = readint(&inptr, NULL);
							if (v1 < 0) {
								debug_heatmap = 2;
							}
						}
						TCHAR buf[200];
						TCHAR *pbuf;
						_sntprintf(buf, sizeof buf, _T("0 dff000 200 NONE"));
						pbuf = buf;
						memwatch(&pbuf);
						_sntprintf(buf, sizeof buf, _T("1 0 %08x NONE"), currprefs.chipmem.size);
						pbuf = buf;
						memwatch(&pbuf);
						if (currprefs.bogomem.size) {
							_sntprintf(buf, sizeof buf, _T("2 c00000 %08x NONE"), currprefs.bogomem.size);
							pbuf = buf;
							memwatch(&pbuf);
						}
						console_out_f(_T("Heatmap enabled\n"));
					} else {
						if (*inptr == 'd') {
							console_out_f(_T("Heatmap disabled\n"));
							free_heatmap();
						} else {
							heatmap_stats(&inptr);
						}
					}
				} else if (*inptr == 'o') {
					if (debug_dma) {
						console_out_f (_T("DMA debugger disabled\n"), debug_dma);
						record_dma_reset(0);
						reset_drawing();
						debug_dma = 0;
					}
				} else if (*inptr == 'm') {
					set_debug_colors();
					inptr++;
					if (more_params(&inptr)) {
						v1 = readint(&inptr, &err);
						if (!err && v1 >= 0 && v1 < DMARECORD_MAX) {
							v2 = readint(&inptr, &err);
							if (!err && v2 >= 0 && v2 <= DMARECORD_SUBITEMS) {
								if (more_params(&inptr)) {
									uae_u32 rgb = readhex(&inptr, NULL);
									if (v2 == 0) {
										for (int i = 0; i < DMARECORD_SUBITEMS; i++) {
											debug_colors[v1].l[i] = rgb;
										}
									} else {
										v2--;
										debug_colors[v1].l[v2] = rgb;
									}
									debug_colors[v1].enabled = true;
								} else {
									debug_colors[v1].enabled = !debug_colors[v1].enabled;
								}
								console_out_f(_T("%d,%d: %08x %s %s\n"), v1, v2, debug_colors[v1].l[v2], debug_colors[v1].enabled ? _T("*") : _T(" "), debug_colors[v1].name);
							}
						}
					} else {
						for (int i = 0; i < DMARECORD_MAX; i++) {
							for (int j = 0; j < DMARECORD_SUBITEMS; j++) {
								if (j < debug_colors[i].max) {
									console_out_f(_T("%d,%d: %08x %s %s\n"), i, j, debug_colors[i].l[j], debug_colors[i].enabled ? _T("*") : _T(" "), debug_colors[i].name);
								}
							}
						}
					}
				} else {
					if (more_params(&inptr) && *inptr == '?') {
						mw_help();
					} else {
						dma_record_vpos_type = 0;
						free_heatmap();
						int nextcmd = peekchar(&inptr);
						if (nextcmd == 'l') {
							next_char(&inptr);
						} else if (nextcmd == 'v') {
							next_char(&inptr);
							dma_record_vpos_type = 1;
						}
						if (more_params(&inptr))
							v1 = readint(&inptr, NULL);
						if (more_params(&inptr))
							v2 = readint(&inptr, NULL);
						if (more_params(&inptr))
							v3 = readint(&inptr, NULL);
						if (debug_dma && v1 >= 0 && v2 >= 0) {
							decode_dma_record(v2, v1, v3, cmd == 'v', nextcmd == 'l');
						} else {
							if (debug_dma) {
								record_dma_reset(0);
								reset_drawing();
							}
							debug_dma = v1 < 0 ? -v1 : 1;
							console_out_f(_T("DMA debugger enabled, mode=%d.\n"), debug_dma);
						}
					}
				}
			}
			break;
		case 'o':
			{
				if (copper_debugger (&inptr)) {
					debugger_active = 0;
					debugging = 0;
					return true;
				}
				break;
			}
		case 'O':
			break;
		case 'b':
			if (staterecorder (&inptr))
				return true;
			break;
		case 'u':
			{
				if (more_params(&inptr)) {
					if (*inptr == 'a') {
						debugmem_inhibit_break(1);
						console_out(_T("All break to debugger methods inhibited.\n"));
					} else if (*inptr == 'c') {
						debugmem_inhibit_break(-1);
						console_out(_T("All break to debugger methods allowed.\n"));
					}
				} else {
					if (debugmem_inhibit_break(0)) {
						console_out(_T("Current break to debugger method inhibited.\n"));
					} else {
						console_out(_T("Current break to debugger method allowed.\n"));
					}
				}
			}
			break;
		case 'U':
			if (currprefs.mmu_model && more_params (&inptr)) {
				int i;
				uaecptr addrl = readhex(&inptr, NULL);
				uaecptr addrp;
				console_out_f (_T("%08X translates to:\n"), addrl);
				for (i = 0; i < 4; i++) {
					bool super = (i & 2) != 0;
					bool data = (i & 1) != 0;
					console_out_f (_T("S%dD%d="), super, data);
					TRY(prb) {
						if (currprefs.mmu_model >= 68040)
							addrp = mmu_translate (addrl, 0, super, data, false, sz_long);
						else
							addrp = mmu030_translate (addrl, super, data, false);
						console_out_f (_T("%08X"), addrp);
						TRY(prb2) {
							if (currprefs.mmu_model >= 68040)
								addrp = mmu_translate (addrl, 0, super, data, true, sz_long);
							else
								addrp = mmu030_translate (addrl, super, data, true);
							console_out_f (_T(" RW"));
						} CATCH(prb2) {
							console_out_f (_T(" RO"));
						} ENDTRY
					} CATCH(prb) {
						console_out_f (_T("***********"));
					} ENDTRY
					console_out_f (_T(" "));
				}
				console_out_f (_T("\n"));
			}
			break;
		case 'h':
		case '?':
			if (more_params (&inptr))
				converter (&inptr);
			else
				debug_help ();
			break;
	}
	return false;
}

static TCHAR input[MAX_LINEWIDTH];

static void debug_1 (void)
{
	open_console();
	custom_dumpstate(0);
	m68k_dumpstate(&nextpc, debug_pc);
	debug_pc = 0xffffffff;
	nxdis = nextpc; nxmem = 0;
	debugger_active = 1;

	for (;;) {
		int v;

		if (!debugger_active)
			return;
		update_debug_info ();
		console_out (_T(">"));
		console_flush ();
		debug_linecounter = 0;
		v = console_get (input, MAX_LINEWIDTH);
		if (v < 0)
			return;
		if (v == 0)
			continue;
		if (debug_line (input))
			return;
	}
}

static void addhistory(void)
{
	uae_u32 pc = currprefs.cpu_model >= 68020 && currprefs.cpu_compatible ? regs.instruction_pc : m68k_getpc();
	int prevhist = lasthist == 0 ? MAX_HIST - 1 : lasthist - 1;
	if (history[prevhist].regs.pc == pc) {
		return;
	}
	history[lasthist].regs = regs;
	history[lasthist].regs.pc = pc;
	history[lasthist].vpos = vpos;
	history[lasthist].hpos = current_hpos();
	history[lasthist].fp = vsync_counter;

	if (++lasthist == MAX_HIST) {
		lasthist = 0;
	}
	if (lasthist == firsthist) {
		if (++firsthist == MAX_HIST) {
			firsthist = 0;
		}
	}
}

void debug_exception(int nr)
{
	if (debug_illegal) {
		if (nr <= 63 && (debug_illegal_mask & ((uae_u64)1 << nr))) {
			write_log(_T("Exception %d breakpoint\n"), nr);
			activate_debugger();
		}
	}
	if (trace_param[0] == 0xffffffff && trace_mode == TRACE_SKIP_INS) {
		activate_debugger();
	}
}

static bool check_breakpoint(struct breakpoint_node *bpn, uaecptr pc)
{
	int bpnum = -1;

	if (!bpn->enabled) {
		return false;
	}
	if (bpn->type == BREAKPOINT_REG_PC) {
		if (bpn->value1 == pc) {
			return true;
		}
	} else if (bpn->type >= 0 && bpn->type < BREAKPOINT_REG_END) {
		uae_u32 value1 = bpn->value1 & bpn->mask;
		uae_u32 value2 = bpn->value2 & bpn->mask;
		uae_u32 cval = returnregx(bpn->type) & bpn->mask;
		int opersize = bpn->mask == 0xff ? 1 : (bpn->mask == 0xffff) ? 2 : 4;
		bool opersigned = bpn->opersigned;
		uae_s32 value1s = (uae_s32)value1;
		uae_s32 value2s = (uae_s32)value2;
		uae_s32 cvals = (uae_s32)cval;
		if (opersize == 2) {
			cvals = (uae_s32)(uae_s16)cvals;
			value1s = (uae_s32)(uae_s16)value1s;
			value2s = (uae_s32)(uae_s16)value2s;
		} else if (opersize == 1) {
			cvals = (uae_s32)(uae_s8)cvals;
			value1s = (uae_s32)(uae_s8)value1s;
			value2s = (uae_s32)(uae_s8)value2s;
		}
		switch (bpn->oper)
		{
			case BREAKPOINT_CMP_EQUAL:
				if (cval == value1)
					return true;
				break;
			case BREAKPOINT_CMP_NEQUAL:
				if (cval != value1)
					return true;
				break;
			case BREAKPOINT_CMP_SMALLER:
				if (opersigned) {
					if (cvals <= value1s)
						return true;
				} else {
					if (cval <= value1)
						return true;
				}
				break;
			case BREAKPOINT_CMP_LARGER:
				if (opersigned) {
					if (cvals >= value1s)
						return true;
				} else {
					if (cval >= value1)
						return true;
				}
				break;
			case BREAKPOINT_CMP_RANGE:
				if (opersigned) {
					if (cvals >= value1s && cvals <= value2s)
						return true;
				} else {
					if (cval >= value1 && cval <= value2)
						return true;
				}
				break;
			case BREAKPOINT_CMP_NRANGE:
				if (opersigned) {
					if (cvals <= value1s || cvals >= value2s)
						return true;
				} else {
					if (cval <= value1 || cval >= value2)
					return true;
				}
				break;
		}
	}
	return false;
}

static bool check_breakpoint_count(struct breakpoint_node *bpn, uaecptr pc)
{
	if (bpn->cnt <= 0) {
		return true;
	}
	console_out_f(_T("Breakpoint %d hit: PC=%08x, count=%d.\n"), bpn - bpnodes, pc, bpn->cnt);
	bpn->cnt--;
	return false;
}

void debug (void)
{
	int wasactive;

	if (savestate_state || quit_program)
		return;

	bogusframe = 1;
	disasm_init();
	addhistory ();

#if 0
	if (do_skip && skipaddr_start == 0xC0DEDBAD) {
		if (trace_same_insn_count > 0) {
			if (memcmp (trace_insn_copy, regs.pc_p, 10) == 0
				&& memcmp (trace_prev_regs.regs, regs.regs, sizeof regs.regs) == 0)
			{
				trace_same_insn_count++;
				return;
			}
		}
		if (trace_same_insn_count > 1)
			fprintf (logfile, "[ repeated %d times ]\n", trace_same_insn_count);
		m68k_dumpstate (logfile, &nextpc);
		trace_same_insn_count = 1;
		memcpy (trace_insn_copy, regs.pc_p, 10);
		memcpy (&trace_prev_regs, &regs, sizeof regs);
	}
#endif

	if (!memwatch_triggered) {
		if (trace_mode) {
			uae_u32 pc;
			uae_u16 opcode;
			int bpnum = -1;
			int bp = 0;

			pc = munge24 (m68k_getpc ());
			opcode = currprefs.cpu_model < 68020 && (currprefs.cpu_compatible || currprefs.cpu_cycle_exact) ? regs.ir : get_word_debug (pc);

			for (int i = 0; i < BREAKPOINT_TOTAL; i++) {
				struct breakpoint_node *bpn = &bpnodes[i];
				if (check_breakpoint(bpn, pc)) {
					int j;
					// if this breakpoint is chained, ignore it
					for (j = 0; j < BREAKPOINT_TOTAL; j++) {
						struct breakpoint_node *bpn2 = &bpnodes[j];
						if (bpn2->enabled && bpn2->chain == i) {
							break;
						}
					}
					if (j >= BREAKPOINT_TOTAL) {
						if (!check_breakpoint_count(bpn, pc)) {
							break;
						}
						int max = BREAKPOINT_TOTAL;
						bpnum = i;
						// check breakpoint chain
						while (bpnum >= 0 && bpnodes[bpnum].chain >= 0 && bpnodes[bpnum].chain != bpnum && max > 0) {
							bpnum = bpnodes[bpnum].chain;
							struct breakpoint_node *bpn = &bpnodes[bpnum];
							if (!check_breakpoint(bpn, pc)) {
								bpnum = -1;
								break;
							}
							if (!check_breakpoint_count(bpn, pc)) {
								bpnum = -1;
								break;
							}
							max--;
						}
						if (bpnum >= 0) {
							break;
						}
					}
				}
			}

			if (trace_mode) {
				if (trace_mode == TRACE_MATCH_PC && trace_param[0] == pc)
					bp = -1;
				if (trace_mode == TRACE_RAM_PC) {
					addrbank *ab = &get_mem_bank(pc);
					if (ab->flags & ABFLAG_RAM) {
						uae_u16 ins = get_word_debug(pc);
						// skip JMP xxxxxx (LVOs)
						if (ins != 0x4ef9) {
							bp = -1;
						}
					}
				}
				if ((processptr || processname) && !isrom(m68k_getpc())) {
					uaecptr execbase = get_long_debug (4);
					uaecptr activetask = get_long_debug (execbase + 276);
					int process = get_byte_debug (activetask + 8) == 13 ? 1 : 0;
					char *name = (char*)get_real_address_debug(get_long_debug (activetask + 10));
					if (process) {
						uaecptr cli = BPTR2APTR(get_long_debug (activetask + 172));
						uaecptr seglist = 0;

						uae_char *command = NULL;
						if (cli) {
							if (processname)
								command = (char*)get_real_address_debug(BPTR2APTR(get_long_debug (cli + 16)));
							seglist = BPTR2APTR(get_long_debug (cli + 60));
						} else {
							seglist = BPTR2APTR(get_long_debug (activetask + 128));
							seglist = BPTR2APTR(get_long_debug (seglist + 12));
						}
						if (activetask == processptr || (processname && (!stricmp (name, processname) || (command && command[0] && !_tcsnicmp (command + 1, processname, command[0]) && processname[command[0]] == 0)))) {
							while (seglist) {
								uae_u32 size = get_long_debug (seglist - 4) - 4;
								if (pc >= (seglist + 4) && pc < (seglist + size)) {
									bp = -1;
									break;
								}
								seglist = BPTR2APTR(get_long_debug (seglist));
							}
						}
					}
				} else if (trace_mode == TRACE_MATCH_INS) {
					if (trace_param[0] == 0x10000) {
						if (opcode == 0x4e75 || opcode == 0x4e73 || opcode == 0x4e77)
							bp = -1;
					} else if (opcode == trace_param[0]) {
						bp = -1;
						for (int op = 1; op < 3; op++) {
							if (trace_param[op] != 0x10000) {
								uae_u16 w = 0xffff;
								debug_get_prefetch(op, &w);
								if (w != trace_param[op])
									bp = 0;
							}
						}
					}
				} else if (trace_mode == TRACE_SKIP_INS) {
					if (trace_param[0] != 0 && trace_param[0] != 0xffffffff)
						trace_param[0]--;
					if (trace_param[0] == 0) {
						bp = -1;
					}
#if 0
				} else if (skipaddr_start == 0xffffffff && skipaddr_doskip > 0) {
					bp = -1;
#endif
				} else if (trace_mode == TRACE_RANGE_PC) {
					if (pc >= trace_param[0] && pc < trace_param[1])
						bp = -1;
				} else if (trace_mode == TRACE_SKIP_LINE) {
					if (trace_param[0] != 0)
						trace_param[0]--;
					if (trace_param[0] == 0) {
						int line = debugmem_get_sourceline(pc, NULL, 0);
						if (line > 0 && line != trace_param[1])
							bp = -1;
					}
				}
			}
			if (!bp && bpnum < 0) {
				debug_continue();
				return;
			}
			if (bpnum >= 0) {
				console_out_f(_T("Breakpoint %d triggered.\n"), bpnum);
			}
			debug_cycles(1);
		}
	} else {
		memwatch_hit_msg(memwatch_triggered - 1);
		memwatch_triggered = 0;
	}

	wasactive = ismouseactive ();
#ifdef WITH_PPC
	uae_ppc_pause(1);
#endif
	inputdevice_unacquire();
	pause_sound ();
	setmouseactive(0, 0);
	target_inputdevice_unacquire(true);
	activate_console ();
	trace_mode = 0;
	exception_debugging = 0;
	debug_rewind = 0;
	processptr = 0;
#if 0
	if (!currprefs.statecapture) {
		changed_prefs.statecapture = currprefs.statecapture = 1;
		savestate_init ();
	}
#endif
	debugmem_disable();

	if (trace_cycles && last_frame >= 0) {
		if (last_frame + 2 >= vsync_counter || trace_cycles > 1) {
			evt_t c = last_cycles2 - last_cycles1;
			uae_u32 cc;
			if (c >= 0x7fffffff) {
				cc = 0x7fffffff;
			} else {
				cc = (uae_u32)c;
			}
			console_out_f(_T("Cycles: %d Chip, %d CPU. (V=%d H=%d -> V=%d H=%d)\n"),
				cc / CYCLE_UNIT,
				cc / cpucycleunit,
				last_vpos1, last_hpos1,
				last_vpos2, last_hpos2);
		}
	}
	trace_cycles = 0;

	debug_1 ();
	debugmem_enable();
	if (!debug_rewind && !currprefs.cachesize
#ifdef FILESYS
		&& nr_units () == 0
#endif
		) {
			savestate_capture (1);
	}
	if (!trace_mode) {
		for (int i = 0; i < BREAKPOINT_TOTAL; i++) {
			if (bpnodes[i].enabled)
				trace_mode = TRACE_CHECKONLY;
		}
	}
	if (trace_mode) {
		set_special (SPCFLAG_BRK);
		debugging = -1;
	}
	resume_sound ();
	inputdevice_acquire (TRUE);
#ifdef WITH_PPC
	uae_ppc_pause(0);
#endif
	setmouseactive(0, wasactive ? 2 : 0);
	target_inputdevice_acquire();

	last_cycles1 = get_cycles();
	last_vpos1 = vpos;
	last_hpos1 = current_hpos();
	last_frame = timeframes;
}

const TCHAR *debuginfo (int mode)
{
	static TCHAR txt[100];
	uae_u32 pc = M68K_GETPC;
	_sntprintf (txt, sizeof txt, _T("PC=%08X INS=%04X %04X %04X"),
		pc, get_word_debug (pc), get_word_debug (pc + 2), get_word_debug (pc + 4));
	return txt;
}

void mmu_disasm (uaecptr pc, int lines)
{
	debug_mmu_mode = regs.s ? 6 : 2;
	m68k_dumpstate(NULL, 0xffffffff);
	m68k_disasm (pc, NULL, 0xffffffff, lines);
}

static int mmu_logging;

#define MMU_PAGE_SHIFT 16

struct mmudata {
	uae_u32 flags;
	uae_u32 addr;
	uae_u32 len;
	uae_u32 remap;
	uae_u32 p_addr;
};

static struct mmudata *mmubanks;
static uae_u32 mmu_struct, mmu_callback, mmu_regs;
static uae_u32 mmu_fault_bank_addr, mmu_fault_addr;
static int mmu_fault_size, mmu_fault_rw;
static int mmu_slots;
static struct regstruct mmur;

struct mmunode {
	struct mmudata *mmubank;
	struct mmunode *next;
};
static struct mmunode **mmunl;
extern regstruct mmu_backup_regs;

#define MMU_READ_U (1 << 0)
#define MMU_WRITE_U (1 << 1)
#define MMU_READ_S (1 << 2)
#define MMU_WRITE_S (1 << 3)
#define MMU_READI_U (1 << 4)
#define MMU_READI_S (1 << 5)

#define MMU_MAP_READ_U (1 << 8)
#define MMU_MAP_WRITE_U (1 << 9)
#define MMU_MAP_READ_S (1 << 10)
#define MMU_MAP_WRITE_S (1 << 11)
#define MMU_MAP_READI_U (1 << 12)
#define MMU_MAP_READI_S (1 << 13)

void mmu_do_hit (void)
{
	int i;
	uaecptr p;
	uae_u32 pc;

	mmu_triggered = 0;
	pc = m68k_getpc ();
	p = mmu_regs + 18 * 4;
	put_long (p, pc);
	regs = mmu_backup_regs;
	regs.intmask = 7;
	regs.t0 = regs.t1 = 0;
	if (!regs.s) {
		regs.usp = m68k_areg (regs, 7);
		if (currprefs.cpu_model >= 68020)
			m68k_areg (regs, 7) = regs.m ? regs.msp : regs.isp;
		else
			m68k_areg (regs, 7) = regs.isp;
		regs.s = 1;
	}
	MakeSR ();
	m68k_setpc (mmu_callback);
	fill_prefetch ();

	if (currprefs.cpu_model > 68000) {
		for (i = 0 ; i < 9; i++) {
			m68k_areg (regs, 7) -= 4;
			put_long (m68k_areg (regs, 7), 0);
		}
		m68k_areg (regs, 7) -= 4;
		put_long (m68k_areg (regs, 7), mmu_fault_addr);
		m68k_areg (regs, 7) -= 2;
		put_word (m68k_areg (regs, 7), 0); /* WB1S */
		m68k_areg (regs, 7) -= 2;
		put_word (m68k_areg (regs, 7), 0); /* WB2S */
		m68k_areg (regs, 7) -= 2;
		put_word (m68k_areg (regs, 7), 0); /* WB3S */
		m68k_areg (regs, 7) -= 2;
		put_word (m68k_areg (regs, 7),
			(mmu_fault_rw ? 0 : 0x100) | (mmu_fault_size << 5)); /* SSW */
		m68k_areg (regs, 7) -= 4;
		put_long (m68k_areg (regs, 7), mmu_fault_bank_addr);
		m68k_areg (regs, 7) -= 2;
		put_word (m68k_areg (regs, 7), 0x7002);
	}
	m68k_areg (regs, 7) -= 4;
	put_long (m68k_areg (regs, 7), get_long_debug (p - 4));
	m68k_areg (regs, 7) -= 2;
	put_word (m68k_areg (regs, 7), mmur.sr);
#ifdef JIT
	if (currprefs.cachesize) {
		set_special(SPCFLAG_END_COMPILE);
	}
#endif
}

static void mmu_do_hit_pre (struct mmudata *md, uaecptr addr, int size, int rwi, uae_u32 v)
{
	uae_u32 p, pc;
	int i;

	mmur = regs;
	pc = m68k_getpc ();
	if (mmu_logging)
		console_out_f (_T("MMU: hit %08X SZ=%d RW=%d V=%08X PC=%08X\n"), addr, size, rwi, v, pc);

	p = mmu_regs;
	put_long (p, 0); p += 4;
	for (i = 0; i < 16; i++) {
		put_long (p, regs.regs[i]);
		p += 4;
	}
	put_long (p, pc); p += 4;
	put_long (p, 0); p += 4;
	put_long (p, regs.usp); p += 4;
	put_long (p, regs.isp); p += 4;
	put_long (p, regs.msp); p += 4;
	put_word (p, regs.sr); p += 2;
	put_word (p, (size << 1) | ((rwi & 2) ? 1 : 0)); /* size and rw */ p += 2;
	put_long (p, addr); /* fault address */ p += 4;
	put_long (p, md->p_addr); /* bank address */ p += 4;
	put_long (p, v); p += 4;
	mmu_fault_addr = addr;
	mmu_fault_bank_addr = md->p_addr;
	mmu_fault_size = size;
	mmu_fault_rw = rwi;
	mmu_triggered = 1;
}

static int mmu_hit (uaecptr addr, int size, int rwi, uae_u32 *v)
{
	int s, trig;
	uae_u32 flags;
	struct mmudata *md;
	struct mmunode *mn;

	if (mmu_triggered)
		return 1;

	mn = mmunl[addr >> MMU_PAGE_SHIFT];
	if (mn == NULL)
		return 0;

	s = regs.s;
	while (mn) {
		md = mn->mmubank;
		if (addr >= md->addr && addr < md->addr + md->len) {
			flags = md->flags;
			if (flags & (MMU_MAP_READ_U | MMU_MAP_WRITE_U | MMU_MAP_READ_S | MMU_MAP_WRITE_S | MMU_MAP_READI_U | MMU_MAP_READI_S)) {
				trig = 0;
				if (!s && (flags & MMU_MAP_READ_U) && (rwi & 1))
					trig = 1;
				if (!s && (flags & MMU_MAP_WRITE_U) && (rwi & 2))
					trig = 1;
				if (s && (flags & MMU_MAP_READ_S) && (rwi & 1))
					trig = 1;
				if (s && (flags & MMU_MAP_WRITE_S) && (rwi & 2))
					trig = 1;
				if (!s && (flags & MMU_MAP_READI_U) && (rwi & 4))
					trig = 1;
				if (s && (flags & MMU_MAP_READI_S) && (rwi & 4))
					trig = 1;
				if (trig) {
					uaecptr maddr = md->remap + (addr - md->addr);
					if (maddr == addr) /* infinite mmu hit loop? no thanks.. */
						return 1;
					if (mmu_logging)
						console_out_f (_T("MMU: remap %08X -> %08X SZ=%d RW=%d\n"), addr, maddr, size, rwi);
					if ((rwi & 2)) {
						switch (size)
						{
						case 4:
							put_long (maddr, *v);
							break;
						case 2:
							put_word (maddr, *v);
							break;
						case 1:
							put_byte (maddr, *v);
							break;
						}
					} else {
						switch (size)
						{
						case 4:
							*v = get_long_debug (maddr);
							break;
						case 2:
							*v = get_word_debug (maddr);
							break;
						case 1:
							*v = get_byte_debug (maddr);
							break;
						}
					}
					return 1;
				}
			}
			if (flags & (MMU_READ_U | MMU_WRITE_U | MMU_READ_S | MMU_WRITE_S | MMU_READI_U | MMU_READI_S)) {
				trig = 0;
				if (!s && (flags & MMU_READ_U) && (rwi & 1))
					trig = 1;
				if (!s && (flags & MMU_WRITE_U) && (rwi & 2))
					trig = 1;
				if (s && (flags & MMU_READ_S) && (rwi & 1))
					trig = 1;
				if (s && (flags & MMU_WRITE_S) && (rwi & 2))
					trig = 1;
				if (!s && (flags & MMU_READI_U) && (rwi & 4))
					trig = 1;
				if (s && (flags & MMU_READI_S) && (rwi & 4))
					trig = 1;
				if (trig) {
					mmu_do_hit_pre (md, addr, size, rwi, *v);
					return 1;
				}
			}
		}
		mn = mn->next;
	}
	return 0;
}

#ifdef JIT

static void mmu_free_node(struct mmunode *mn)
{
	if (!mn)
		return;
	mmu_free_node (mn->next);
	xfree (mn);
}

static void mmu_free(void)
{
	struct mmunode *mn;
	int i;

	for (i = 0; i < mmu_slots; i++) {
		mn = mmunl[i];
		mmu_free_node (mn);
	}
	xfree (mmunl);
	mmunl = NULL;
	xfree (mmubanks);
	mmubanks = NULL;
}

#endif

static int getmmubank(struct mmudata *snptr, uaecptr p)
{
	snptr->flags = get_long_debug (p);
	if (snptr->flags == 0xffffffff)
		return 1;
	snptr->addr = get_long_debug (p + 4);
	snptr->len = get_long_debug (p + 8);
	snptr->remap = get_long_debug (p + 12);
	snptr->p_addr = p;
	return 0;
}

int mmu_init(int mode, uaecptr parm, uaecptr parm2)
{
	uaecptr p, p2, banks;
	int size;
	struct mmudata *snptr;
	struct mmunode *mn;
#ifdef JIT
	static int wasjit;
	if (currprefs.cachesize) {
		wasjit = currprefs.cachesize;
		changed_prefs.cachesize = 0;
		console_out (_T("MMU: JIT disabled\n"));
		check_prefs_changed_comp(false);
	}
	if (mode == 0) {
		if (mmu_enabled) {
			mmu_free ();
			deinitialize_memwatch ();
			console_out (_T("MMU: disabled\n"));
			changed_prefs.cachesize = wasjit;
		}
		mmu_logging = 0;
		return 1;
	}
#endif

	if (mode == 1) {
		if (!mmu_enabled)
			return 0xffffffff;
		return mmu_struct;
	}

	p = parm;
	mmu_struct = p;
	if (get_long_debug (p) != 1) {
		console_out_f (_T("MMU: version mismatch %d <> %d\n"), get_long_debug (p), 1);
		return 0;
	}
	p += 4;
	mmu_logging = get_long_debug (p) & 1;
	p += 4;
	mmu_callback = get_long_debug (p);
	p += 4;
	mmu_regs = get_long_debug (p);
	p += 4;

	if (mode == 3) {
		int off;
		uaecptr addr = get_long_debug (parm2 + 4);
		if (!mmu_enabled)
			return 0;
		off = addr >> MMU_PAGE_SHIFT;
		mn = mmunl[off];
		while (mn) {
			if (mn->mmubank->p_addr == parm2) {
				getmmubank(mn->mmubank, parm2);
				if (mmu_logging)
					console_out_f (_T("MMU: bank update %08X: %08X - %08X %08X\n"),
					mn->mmubank->flags, mn->mmubank->addr, mn->mmubank->len + mn->mmubank->addr,
					mn->mmubank->remap);
			}
			mn = mn->next;
		}
		return 1;
	}

	mmu_slots = 1 << ((currprefs.address_space_24 ? 24 : 32) - MMU_PAGE_SHIFT);
	mmunl = xcalloc (struct mmunode*, mmu_slots);
	size = 1;
	p2 = get_long_debug (p);
	while (get_long_debug (p2) != 0xffffffff) {
		p2 += 16;
		size++;
	}
	p = banks = get_long_debug (p);
	snptr = mmubanks = xmalloc (struct mmudata, size);
	for (;;) {
		int off;
		if (getmmubank(snptr, p))
			break;
		p += 16;
		off = snptr->addr >> MMU_PAGE_SHIFT;
		if (mmunl[off] == NULL) {
			mn = mmunl[off] = xcalloc (struct mmunode, 1);
		} else {
			mn = mmunl[off];
			while (mn->next)
				mn = mn->next;
			mn = mn->next = xcalloc (struct mmunode, 1);
		}
		mn->mmubank = snptr;
		snptr++;
	}

	initialize_memwatch (1);
	console_out_f (_T("MMU: enabled, %d banks, CB=%08X S=%08X BNK=%08X SF=%08X, %d*%d\n"),
		size - 1, mmu_callback, parm, banks, mmu_regs, mmu_slots, 1 << MMU_PAGE_SHIFT);
	set_special (SPCFLAG_BRK);
	return 1;
}

void debug_parser (const TCHAR *cmd, TCHAR *out, uae_u32 outsize)
{
	TCHAR empty[2] = { 0 };
	TCHAR *input = my_strdup (cmd);
	if (out == NULL && outsize == 0) {
		setconsolemode (empty, 1);
	} else if (out != NULL && outsize > 0) {
		out[0] = 0;
		setconsolemode (out, outsize);
	}
	debug_line (input);
	setconsolemode (NULL, 0);
	xfree (input);
}

/*

trainer file is .ini file with following one or more [patch] sections.
Each [patch] section describes single trainer option.

After [patch] section must come at least one patch descriptor.

[patch]
name=name
enable=true/false
event=KEY_F1

; patch descriptor
data=200e46802d400026200cxx02 ; this is comment
offset=2
access=write
setvalue=<value>
type=nop/freeze/set/setonce

; patch descriptor
data=11223344556677889900
offset=10
replacedata=4e71
replaceoffset=4

; next patch section
[patch]


name: name of the option (appears in GUI in the future)
enable: true = automatically enabled at startup. (false=manually activated using key shortcut etc.., will be implemented later)
event: inputevents.def event name

data: match data, when emulated CPU executes first opcode of this data and following words also match: match is detected. x = anything.
offset: word offset from beginning of "data" that points to memory read/write instruction that you want to "patch". Default=0.
access: read=read access, write=write access. Default: write if instruction does both memory read and write, read if read-only.

setvalue: value to write if type is set or setonce.
type=nop: found instruction's write does nothing. This instruction only. Other instruction(s) modifying same memory location are not skipped.
type=freeze: found instruction's memory read always returns value in memory. Write does nothing.
type=set: found instruction's memory read always returns "setvalue" contents. Write works normally.
type=setonce: "setvalue" contents are written to memory when patch is detected.

replacedata: data to be copied over data + replaceoffset. x masking is also supported. Memory is modified.
replaceoffset: word offset from data.

---

Internally it uses debugger memory watch points to modify/freeze memory contents. No memory or code is modified.
Only type=setonce and replacedata modifies memory.

When CPU emulator current to be executed instruction's matches contents of data[offset], other words of data are also checked.
If all words match: instruction's effective address(es) are calculated and matching (read/write) EA is stored. Matching part
of patch is now done.

Reason for this complexity is to enable single patch to work even if game is relocatable or it uses different memory
locations depending on hardware config.

If type=nop/freeze/set: debugger memwatch point is set that handles faking of read/write access.
If type=setonce: "setvalue" contents gets written to detected effective address.
If replacedata is set: copy code.

Detection phase may cause increased CPU load, this may get optimized more but it shouldn't be (too) noticeable in basic
A500 or A1200 modes.

*/

#define TRAINER_NOP 0
#define TRAINER_FREEZE 1
#define TRAINER_SET 2
#define TRAINER_SETONCE 3

struct trainerpatch
{
	TCHAR *name;
	uae_u16 *data;
	uae_u16 *maskdata;
	uae_u16 *replacedata;
	uae_u16 *replacemaskdata;
	uae_u16 *replacedata_original;
	uae_u16 first;
	int length;
	int offset;
	int access;
	int replacelength;
	int replaceoffset;
	uaecptr addr;
	uaecptr varaddr;
	int varsize;
	uae_u32 oldval;
	int patchtype;
	int setvalue;
	int *events;
	int eventcount;
	int memwatchindex;
	bool enabledatstart;
	bool enabled;
};

static struct trainerpatch **tpptr;
static int tpptrcnt;
bool debug_opcode_watch;

static int debug_trainer_get_ea(struct trainerpatch *tp, uaecptr pc, uae_u16 opcode, uaecptr *addr)
{
	struct instr *dp = table68k + opcode;
	uae_u32 sea = 0, dea = 0;
	uaecptr spc = 0, dpc = 0;
	uaecptr pc2 = pc + 2;
	if (dp->suse) {
		spc = pc2;
		pc2 = ShowEA(NULL, pc2, opcode, dp->sreg, dp->smode, dp->size, NULL, &sea, NULL, 1);
		if (sea == spc)
			spc = 0xffffffff;
	}
	if (dp->duse) {
		dpc = pc2;
		pc2 = ShowEA(NULL, pc2, opcode, dp->dreg, dp->dmode, dp->size, NULL, &dea, NULL, 1);
		if (dea == dpc)
			dpc = 0xffffffff;
	}
	if (dea && dpc != 0xffffffff && tp->access == 1) {
		*addr = dea;
		return 1 << dp->size;
	}
	if (sea && spc != 0xffffffff && tp->access == 0) {
		*addr = sea;
		return 1 << dp->size;
	}
	if (dea && tp->access > 1) {
		*addr = dea;
		return 1 << dp->size;
	}
	if (sea && tp->access > 1) {
		*addr = sea;
		return 1 << dp->size;
	}
	return 0;
}

static void debug_trainer_enable(struct trainerpatch *tp, bool enable)
{
	if (tp->enabled == enable)
		return;

	if (tp->replacedata) {
		if (enable) {
			bool first = false;
			if (!tp->replacedata_original) {
				tp->replacedata_original = xcalloc(uae_u16, tp->replacelength);
				first = true;
			}
			for (int j = 0; j < tp->replacelength; j++) {
				uae_u16 v = tp->replacedata[j];
				uae_u16 m = tp->replacemaskdata[j];
				uaecptr addr = (tp->addr - tp->offset * 2) + j * 2 + tp->replaceoffset * 2;
				if (m == 0xffff) {
					x_put_word(addr, v);
				} else {
					uae_u16 vo = x_get_word(addr);
					x_put_word(addr, (vo & ~m) | (v & m));
					if (first)
						tp->replacedata_original[j] = vo;
				}
			}
		} else if (tp->replacedata_original) {
			for (int j = 0; j < tp->replacelength; j++) {
				uae_u16 m = tp->replacemaskdata[j];
				uaecptr addr = (tp->addr - tp->offset * 2) + j * 2 + tp->replaceoffset * 2;
				if (m != 0xffff) {
					x_put_word(addr, tp->replacedata_original[j]);
				}
			}
		}
	}

	if (tp->patchtype == TRAINER_SETONCE && tp->varaddr != 0xffffffff) {
		uae_u32 v = enable ? tp->setvalue : tp->oldval;
		switch (tp->varsize)
		{
		case 1:
			x_put_byte(tp->varaddr, tp->setvalue);
			break;
		case 2:
			x_put_word(tp->varaddr, tp->setvalue);
			break;
		case 4:
			x_put_long(tp->varaddr, tp->setvalue);
			break;
		}
	}

	if ((tp->patchtype == TRAINER_NOP || tp->patchtype == TRAINER_FREEZE || tp->patchtype == TRAINER_SET) && tp->varaddr != 0xffffffff) {
		struct memwatch_node *mwn;
		if (!memwatch_enabled)
			initialize_memwatch(0);
		if (enable) {
			int i;
			for (i = MEMWATCH_TOTAL - 1; i >= 0; i--) {
				mwn = &mwnodes[i];
				if (!mwn->size)
					break;
			}
			if (i < 0) {
				write_log(_T("Trainer out of free memwatchpoints ('%s' %08x\n).\n"), tp->name, tp->addr);
			} else {
				mwn->addr = tp->varaddr;
				mwn->size = tp->varsize;
				mwn->rwi = 1 | 2;
				mwn->access_mask = MW_MASK_CPU_D_R | MW_MASK_CPU_D_W;
				mwn->reg = 0xffffffff;
				mwn->pc = tp->patchtype == TRAINER_NOP ? tp->addr : 0xffffffff;
				mwn->frozen = tp->patchtype == TRAINER_FREEZE || tp->patchtype == TRAINER_NOP;
				mwn->modval_written = 0;
				mwn->val_enabled = 0;
				mwn->val_mask = 0xffffffff;
				mwn->val = 0;
				mwn->reportonly = false;
				if (tp->patchtype == TRAINER_SET) {
					mwn->val_enabled = 1;
					mwn->val = tp->setvalue;
				}
				mwn->nobreak = true;
				memwatch_setup();
				TCHAR buf[256];
				memwatch_dump2(buf, sizeof(buf) / sizeof(TCHAR), i);
				write_log(_T("%s"), buf);
			}
		} else {
			mwn = &mwnodes[tp->memwatchindex];
			mwn->size = 0;
			memwatch_setup();
		}
	}

	write_log(_T("Trainer '%s' %s (addr=%08x)\n"), tp->name, enable ? _T("enabled") : _T("disabled"), tp->addr);
	tp->enabled = enable;
}

void debug_trainer_match(void)
{
	uaecptr pc = m68k_getpc();
	uae_u16 opcode = x_get_word(pc);
	for (int i = 0; i < tpptrcnt; i++) {
		struct trainerpatch *tp = tpptr[i];
		if (tp->first != opcode)
			continue;
		if (tp->addr)
			continue;
		int j;
		for (j = 0; j < tp->length; j++) {
			uae_u16 d = x_get_word(pc + (j - tp->offset) * 2);
			if ((d & tp->maskdata[j]) != tp->data[j])
				break;
		}
		if (j < tp->length)
			continue;
		tp->first = 0xffff;
		tp->addr = pc;
		tp->varsize = -1;
		tp->varaddr = 0xffffffff;
		tp->oldval = 0xffffffff;
		if (tp->access >= 0) {
			tp->varsize = debug_trainer_get_ea(tp, pc, opcode, &tp->varaddr);
			switch (tp->varsize)
			{
			case 1:
				tp->oldval = x_get_byte(tp->varaddr);
				break;
			case 2:
				tp->oldval = x_get_word(tp->varaddr);
				break;
			case 4:
				tp->oldval = x_get_long(tp->varaddr);
				break;
			}
		}
		write_log(_T("Patch %d match at %08x. Addr %08x, size %d, val %08x\n"), i, pc, tp->varaddr, tp->varsize, tp->oldval);

		if (tp->enabledatstart)
			debug_trainer_enable(tp, true);

		// all detected?
		for (j = 0; j < tpptrcnt; j++) {
			struct trainerpatch *tp = tpptr[j];
			if (!tp->addr)
				break;
		}
		if (j == tpptrcnt)
			debug_opcode_watch = false;
	}
}

static int parsetrainerdata(const TCHAR *data, uae_u16 *outdata, uae_u16 *outmask)
{
	int len = uaetcslen(data);
	uae_u16 v = 0, vm = 0;
	int j = 0;
	for (int i = 0; i < len; ) {
		TCHAR c1 = _totupper(data[i + 0]);
		TCHAR c2 = _totupper(data[i + 1]);
		if (c1 > 0 && c1 <= ' ') {
			i++;
			continue;
		}
		if (i + 1 >= len)
			return 0;

		vm <<= 8;
		vm |= 0xff;
		if (c1 == 'X' || c1 == '?')
			vm &= 0x0f;
		if (c2 == 'X' || c2 == '?')
			vm &= 0xf0;

		if (c1 >= 'A')
			c1 -= 'A' - 10;
		else if (c1 >= '0')
			c1 -= '0';
		if (c2 >= 'A')
			c2 -= 'A' - 10;
		else if (c2 >= '0')
			c2 -= '0';

		v <<= 8;
		if (c1 >= 0 && c1 < 16)
			v |= c1 << 4;
		if (c2 >= 0 && c2 < 16)
			v |= c2;

		if (i & 2) {
			outdata[j] = v;
			outmask[j] = vm;
			j++;
		}

		i += 2;
	}
	return j;
}

void debug_init_trainer(const TCHAR *file)
{
	TCHAR section[256];
	int cnt = 1;

	struct ini_data *ini = ini_load(file, false);
	if (!ini)
		return;

	write_log(_T("Loaded '%s'\n"), file);

	_tcscpy(section, _T("patch"));

	for (;;) {
		struct ini_context ictx;
		ini_initcontext(ini, &ictx);

		for (;;) {
			TCHAR *name = NULL;
			TCHAR *data;

			ini_getstring_multi(ini, section, _T("name"), &name, &ictx);

			if (!ini_getstring_multi(ini, section, _T("data"), &data, &ictx))
				break;
			ini_setcurrentasstart(ini, &ictx);
			ini_setlast(ini, section, _T("data"), &ictx);

			TCHAR *p = _tcschr(data, ';');
			if (p)
				*p = 0;
			my_trim(data);

			struct trainerpatch *tp = xcalloc(struct trainerpatch, 1);

			int datalen = (uaetcslen(data) + 3) / 4;
			tp->data = xcalloc(uae_u16, datalen);
			tp->maskdata = xcalloc(uae_u16, datalen);
			tp->length = parsetrainerdata(data, tp->data, tp->maskdata);
			xfree(data);

			ini_getval_multi(ini, section, _T("offset"), &tp->offset, &ictx);
			if (tp->offset < 0 || tp->offset >= tp->length)
				tp->offset = 0;

			if (ini_getstring_multi(ini, section, _T("replacedata"), &data, &ictx)) {
				int replacedatalen = (uaetcslen(data) + 3) / 4;
				tp->replacedata = xcalloc(uae_u16, replacedatalen);
				tp->replacemaskdata = xcalloc(uae_u16, replacedatalen);
				tp->replacelength = parsetrainerdata(data, tp->replacedata, tp->replacemaskdata);
				ini_getval_multi(ini, section, _T("replaceoffset"), &tp->offset, &ictx);
				if (tp->replaceoffset < 0 || tp->replaceoffset >= tp->length)
					tp->replaceoffset = 0;
				tp->access = -1;
				xfree(data);
			}

			tp->access = 2;
			if (ini_getstring_multi(ini, section, _T("access"), &data, &ictx)) {
				if (!_tcsicmp(data, _T("read")))
					tp->access = 0;
				else if (!_tcsicmp(data, _T("write")))
					tp->access = 1;
				xfree(data);
			}

			if (ini_getstring_multi(ini, section, _T("type"), &data, &ictx)) {
				if (!_tcsicmp(data, _T("freeze")))
					tp->patchtype = TRAINER_FREEZE;
				else if (!_tcsicmp(data, _T("nop")))
					tp->patchtype = TRAINER_NOP;
				else if (!_tcsicmp(data, _T("set")))
					tp->patchtype = TRAINER_SET;
				else if (!_tcsicmp(data, _T("setonce")))
					tp->patchtype = TRAINER_SETONCE;
				xfree(data);
			}

			if (ini_getstring_multi(ini, section, _T("setvalue"), &data, &ictx)) {
				TCHAR *endptr;
				if (data[0] == '$') {
					tp->setvalue = _tcstol(data + 1, &endptr, 16);
				} else if (_tcslen(data) > 2 && data[0] == '0' && _totupper(data[1]) == 'x') {
					tp->setvalue = _tcstol(data + 2, &endptr, 16);
				} else {
					tp->setvalue = _tcstol(data, &endptr, 10);
				}
				xfree(data);
			}

			if (ini_getstring(ini, section, _T("enable"), &data)) {
				if (!_tcsicmp(data, _T("true")))
					tp->enabledatstart = true;
				xfree(data);
			}

			if (ini_getstring(ini, section, _T("event"), &data)) {
				TCHAR *s = data;
				_tcscat(s, _T(","));
				while (*s) {
					bool end = false;
					while (*s == ' ')
						s++;
					TCHAR *se = _tcschr(s, ',');
					if (se) {
						*se = 0;
					} else {
						end = true;
					}
					TCHAR *se2 = se - 1;
					while (se2 > s) {
						if (*se2 != ' ')
							break;
						*se2 = 0;
						se2--;
					}
					int evt = inputdevice_geteventid(s);
					if (evt > 0) {
						if (tp->events) {
							tp->events = xrealloc(int, tp->events, tp->eventcount + 1);
						} else {
							tp->events = xmalloc(int, 1);
						}
						tp->events[tp->eventcount++] = evt;
					} else {
						write_log(_T("Unknown event '%s'\n"), s);
					}
					if (end)
						break;
					s = se + 1;
				}
				xfree(data);
			}

			tp->first = tp->data[tp->offset];
			tp->name = name;

			if (tpptrcnt)
				tpptr = xrealloc(struct trainerpatch*, tpptr, tpptrcnt + 1);
			else
				tpptr = xcalloc(struct trainerpatch*, tpptrcnt + 1);
			tpptr[tpptrcnt++] = tp;

			write_log(_T("%d: '%s' parsed and enabled\n"), cnt, tp->name ? tp->name : _T("<no name>"));
			cnt++;

			ini_setlastasstart(ini, &ictx);
		}

		if (!ini_nextsection(ini, section))
			break;

	}

	if (tpptrcnt > 0)
		debug_opcode_watch = true;

	ini_free(ini);
}

bool debug_trainer_event(int evt, int state)
{
	for (int i = 0; i < tpptrcnt; i++) {
		struct trainerpatch *tp = tpptr[i];
		for (int j = 0; j < tp->eventcount; j++) {
			if (tp->events[j] <= 0)
				continue;
			if (tp->events[j] == evt) {
				if (!state)
					return true;
				write_log(_T("Trainer %d ('%s') -> %s\n"), i, tp->name, tp->enabled ? _T("off") : _T("on"));
				debug_trainer_enable(tp, !tp->enabled);
				return true;
			}
		}
	}
	return false;
}

bool debug_get_prefetch(int idx, uae_u16 *opword)
{
	if (currprefs.cpu_compatible) {
		if (currprefs.cpu_model < 68020) {
			if (idx == 0) {
				*opword = regs.ir;
				return true;
			}
			if (idx == 1) {
				*opword = regs.irc;
				return true;
			}
			*opword = get_word_debug(m68k_getpc() + idx * 2);
			return false;
		} else {
			if (regs.prefetch020_valid[idx]) {
				*opword = regs.prefetch020[idx];
				return true;
			}
			*opword = get_word_debug(m68k_getpc() + idx * 2);
			return false;
		}

	} else {
		*opword = get_word_debug(m68k_getpc() + idx * 2);
		return false;
	}
}

#define DEBUGSPRINTF_SIZE 32
static int debugsprintf_cnt;
struct dsprintfstack
{
	uae_u32 val;
	int size;
};
static dsprintfstack debugsprintf_stack[DEBUGSPRINTF_SIZE];
static uae_u16 debugsprintf_latch, debugsprintf_latched;
static evt_t debugsprintf_cycles, debugsprintf_cycles_set;
static uaecptr debugsprintf_va;
static int debugsprintf_mode;

static void read_bstring(char *out, int max, uae_u32 addr)
{
	out[0] = 0;
	if (!valid_address(addr, 1))
		return;
	uae_u8 l = get_byte(addr);
	if (l > max)
		l = max;
	addr++;
	for (int i = 0; i < l && i < max; i++) {
		uae_u8 c = 0;
		if (valid_address(addr, 1)) {
			c = get_byte(addr);
		}
		if (c == 0) {
			c = '.';
		}
		addr++;
		out[i] = c;
		out[i + 1] = 0;
	}
}

static void read_string(char *out, int max, uae_u32 addr)
{
	out[0] = 0;
	for (int i = 0; i < max; i++) {
		uae_u8 c = 0;
		if (valid_address(addr, 1)) {
			c = get_byte(addr);
		}
		addr++;
		out[i] = c;
		out[i + 1] = 0;
		if (!c)
			break;
	}
}

static void parse_custom(char *out, int buffersize, char *format, char *p, char c)
{
	bool gotv = false;
	bool gots = false;
	out[0] = 0;
	uae_u32 v = 0;
	char s[256];
	if (!strcmp(p, "CYCLES")) {
		if (debugsprintf_cycles_set) {
			v = (uae_u32)((get_cycles() - debugsprintf_cycles) / CYCLE_UNIT);
		} else {
			v = 0xffffffff;
		}
		gotv = true;
	}
	if (gotv) {
		if (c == 'x' || c == 'X' || c == 'd' || c == 'i' || c == 'u' || c == 'o') {
			char *fs = format + strlen(format);
			*fs++ = c;
			*fs = 0;
			snprintf(out, buffersize, format, v);
		} else {
			strcpy(s, "****");
			gots = true;
		}
	}
	if (gots) {
		char *fs = format + strlen(format);
		*fs++ = 's';
		*fs = 0;
		snprintf(out, buffersize, format, s);
	}
}

static uae_u32 get_value(struct dsprintfstack **stackp, uae_u32 *sizep, uaecptr *ptrp, uae_u32 size)
{
	if (debugsprintf_mode) {
		uae_u32 v;
		uaecptr ptr = *ptrp;
		if (size == sz_long) {
			v = get_long_debug(ptr);
			ptr += 4;
		} else if (size == sz_word) {
			v = get_word_debug(ptr);
			ptr += 2;
		} else {
			v = get_byte_debug(ptr);
			ptr++;
		}
		*ptrp = ptr;
		*sizep = size;
		return v;
	} else {
		struct dsprintfstack *stack = *stackp;
		uae_u32 v = stack->val;
		if (stack->size == 0)
			v &= 0xff;
		else if (stack->size == 1)
			v &= 0xffff;
		if (size == 1)
			v &= 0xffff;
		*sizep = size;
		stack++;
		*stackp = stack;
		return v;
	}
}

static void debug_sprintf_do(uae_u32 s)
{
	int cnt = 0;
	char format[MAX_DPATH];
	char out[MAX_DPATH];
	read_string(format, MAX_DPATH - 1, s);
	char *p = format;
	char *d = out;
	bool gotm = false;
	bool l = false;
	uaecptr ptr = debugsprintf_va;
	struct dsprintfstack *stack = debugsprintf_stack;
	char fstr[100], *fstrp;
	int buffersize = MAX_DPATH - 1;
	fstrp = fstr;
	*d = 0;
	for (;;) {
		char c = *p++;
		if (c == 0)
			break;
		if (gotm) {
			bool got = false;
			buffersize = MAX_DPATH - uaestrlen(out);
			if (buffersize <= 1)
				break;
			if (c == '%') {
				*d++ = '%';
				gotm = false;
			} else if (c == 'l') {
				l = true;
			} else if (c == 'c') {
				uae_u32 size;
				uae_u32 val = get_value(&stack, &size, &ptr, l ? sz_long : sz_word);
				*fstrp++ = c;
				*fstrp = 0;
				snprintf(d, buffersize, fstr, val);
				got = true;
			} else if (c == 'b') {
				uae_u32 size;
				uae_u32 val = get_value(&stack, &size, &ptr, sz_long);
				char tmp[MAX_DPATH];
				read_bstring(tmp, MAX_DPATH - 1, val);
				*fstrp++ = 's';
				*fstrp = 0;
				snprintf(d, buffersize, fstr, tmp);
				got = true;
			} else if (c == 's') {
				uae_u32 size;
				uae_u32 val = get_value(&stack, &size, &ptr, sz_long);
				char tmp[MAX_DPATH];
				read_string(tmp, MAX_DPATH - 1, val);
				*fstrp++ = c;
				*fstrp = 0;
				snprintf(d, buffersize, fstr, tmp);
				got = true;
			} else if (c == 'p') {
				uae_u32 size;
				uae_u32 val = get_value(&stack, &size, &ptr, sz_long);
				snprintf(d, buffersize, "$%08x", val);
				got = true;
			} else if (c == 'x' || c == 'X' || c == 'd' || c == 'i' || c == 'u' || c == 'o') {
				uae_u32 size;
				uae_u32 val = get_value(&stack, &size, &ptr, l ? sz_long : sz_word);
				if (c == 'd' || c == 'i') {
					if (size == sz_word && (val & 0x8000)) {
						val = (uae_s32)(uae_s16)val;
					}
				}
				*fstrp++ = c;
				*fstrp = 0;
				snprintf(d, buffersize, fstr, val);
				got = true;
			} else if (c == '[') {
				char *next = strchr(p, ']');
				if (next && next[1]) {
					char customout[MAX_DPATH];
					customout[0] = 0;
					*next = 0;
					parse_custom(d, buffersize, fstr, p, next[1]);
					p = next + 2;
					got = true;
				} else {
					gotm = false;
				}
			} else {
				if (fstrp - fstr < sizeof(fstr) - 1) {
					*fstrp++ = c;
					*fstrp = 0;
				}
			}
			if (got) {
				d += strlen(d);
				gotm = false;
			}
		} else if (c == '%') {
			l = false;
			fstrp = fstr;
			*fstrp++ = c;
			*fstrp = 0;
			gotm = true;
		} else {
			*d++ = c;
		}
		*d = 0;
	}
	write_log("%s", out);
}

bool debug_sprintf(uaecptr addr, uae_u32 val, int size)
{
	if (!currprefs.debug_mem)
		return false;

	uae_u32 v = val;
	if (size == sz_word && currprefs.cpu_model < 68020) {
		v &= 0xffff;
		if (!(addr & 2)) {
			debugsprintf_latch = v;
			debugsprintf_latched = 1;
		} else if (debugsprintf_latched) {
			v |= debugsprintf_latch << 16;
			size = sz_long;
			if (!(addr & 4) && debugsprintf_cnt > 0) {
				debugsprintf_cnt--;
			}
		}
	}
	if (size != sz_word) {
		debugsprintf_latched = 0;
	}
	if ((addr & (8 | 4)) == 4) {
		if (size != sz_long)
			return true;
		debug_sprintf_do(v);
		debugsprintf_cnt = 0;
		debugsprintf_latched = 0;
		debugsprintf_cycles = get_cycles();
		debugsprintf_cycles_set = 1;
	} else if ((addr & (8 | 4)) == 8) {
		if (size != sz_long)
			return true;
		debugsprintf_va = val;
		debugsprintf_mode = 1;
	} else {
		if (debugsprintf_cnt < DEBUGSPRINTF_SIZE) {
			debugsprintf_stack[debugsprintf_cnt].val = v;
			debugsprintf_stack[debugsprintf_cnt].size = size;
			debugsprintf_cnt++;
		}
		debugsprintf_mode = 0;
	}
	return true;
}
