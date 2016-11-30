/*
* UAE - The Un*x Amiga Emulator
*
* MC68000 emulation generator
*
* This is a fairly stupid program that generates a lot of case labels that
* can be #included in a switch statement.
* As an alternative, it can generate functions that handle specific
* MC68000 instructions, plus a prototype header file and a function pointer
* array to look up the function for an opcode.
* Error checking is bad, an illegal table68k file will cause the program to
* call abort().
* The generated code is sometimes sub-optimal, an optimizing compiler should
* take care of this.
*
* The source for the insn timings is Markt & Technik's Amiga Magazin 8/1992.
*
* Copyright 1995, 1996, 1997, 1998, 1999, 2000 Bernd Schmidt
*/

#include "sysconfig.h"
#include "sysdeps.h"
#include <ctype.h>

#include "readcpu.h"

#define BOOL_TYPE "int"
/* Define the minimal 680x0 where NV flags are not affected by xBCD instructions.  */
#define xBCD_KEEPS_NV_FLAGS 4

static FILE *headerfile;
static FILE *stblfile;

static int using_prefetch, using_indirect, using_mmu;
static int using_prefetch_020, using_ce020;
static int using_exception_3;
static int using_ce;
static int using_tracer;
static int using_waitstates;
static int using_simple_cycles;
static int cpu_level;
static int count_read, count_write, count_cycles, count_ncycles;
static int count_cycles_ce020;
static int count_read_ea, count_write_ea, count_cycles_ea;
static const char *mmu_postfix;
static int memory_cycle_cnt;
static int did_prefetch;
static int ipl_fetched;

static int optimized_flags;

#define GF_APDI 1
#define GF_AD8R 2
#define GF_PC8R 4
#define GF_AA 7
#define GF_NOREFILL 8
#define GF_PREFETCH 16
#define GF_FC 32
#define GF_MOVE 64
#define GF_IR2IRC 128
#define GF_LRMW 256
#define GF_NOFAULTPC 512
#define GF_RMW 1024
#define GF_OPCE020 2048

/* For the current opcode, the next lower level that will have different code.
* Initialized to -1 for each opcode. If it remains unchanged, indicates we
* are done with that opcode.  */
static int next_cpu_level;

static int *opcode_map;
static int *opcode_next_clev;
static int *opcode_last_postfix;
static unsigned long *counts;
static int generate_stbl;
static int mmufixupcnt;
static int mmufixupstate;
static int mmudisp020cnt;
static bool candormw;
static bool genastore_done;
static char rmw_varname[100];

#define GENA_GETV_NO_FETCH	0
#define GENA_GETV_FETCH		1
#define GENA_GETV_FETCH_ALIGN	2
#define GENA_MOVEM_DO_INC	0
#define GENA_MOVEM_NO_INC	1
#define GENA_MOVEM_MOVE16	2

static char *srcl, *dstl;
static char *srcw, *dstw;
static char *srcb, *dstb;
static char *srcblrmw, *srcwlrmw, *srcllrmw;
static char *dstblrmw, *dstwlrmw, *dstllrmw;
static char *srcbrmw, *srcwrmw, *srclrmw;
static char *dstbrmw, *dstwrmw, *dstlrmw;
static char *prefetch_long, *prefetch_word;
static char *srcli, *srcwi, *srcbi, *nextl, *nextw, *nextb;
static char *srcld, *dstld;
static char *srcwd, *dstwd;
static char *do_cycles, *disp000, *disp020, *getpc;

#define fetchmode_fea 1
#define fetchmode_cea 2
#define fetchmode_fiea 3
#define fetchmode_ciea 4
#define fetchmode_jea 5

static void term (void)
{
	printf("Abort!\n");
	abort ();
}
static void term (const char *err)
{
	printf ("%s\n", err);
	term ();
}

static void read_counts (void)
{
	FILE *file;
	unsigned long opcode, count, total;
	char name[20];
	int nr = 0;
	memset (counts, 0, 65536 * sizeof *counts);

	count = 0;
	file = fopen ("frequent.68k", "r");
	if (file) {
		fscanf (file, "Total: %lu\n", &total);
		while (fscanf (file, "%lx: %lu %s\n", &opcode, &count, name) == 3) {
			opcode_next_clev[nr] = 5;
			opcode_last_postfix[nr] = -1;
			opcode_map[nr++] = opcode;
			counts[opcode] = count;
		}
		fclose (file);
	}
	if (nr == nr_cpuop_funcs)
		return;
	for (opcode = 0; opcode < 0x10000; opcode++) {
		if (table68k[opcode].handler == -1 && table68k[opcode].mnemo != i_ILLG
			&& counts[opcode] == 0)
		{
			opcode_next_clev[nr] = 5;
			opcode_last_postfix[nr] = -1;
			opcode_map[nr++] = opcode;
			counts[opcode] = count;
		}
	}
	if (nr != nr_cpuop_funcs)
		term ();
}

static char endlabelstr[80];
static int endlabelno = 0;
static int need_endlabel;

static int n_braces, limit_braces;
static int m68k_pc_offset, m68k_pc_offset_old;
static int insn_n_cycles, insn_n_cycles020;
static int ir2irc;

static int tail_ce020, total_ce020, head_in_ea_ce020;
static bool head_ce020_cycs_done, tail_ce020_done;
static int subhead_ce020;
static instr *curi_ce020;
static bool no_prefetch_ce020;
static bool got_ea_ce020;

static void fpulimit (void)
{
	if (limit_braces)
		return;
	printf ("\n#ifdef FPUEMU\n");
	limit_braces = n_braces;
	n_braces = 0;
}
static void cpulimit (void)
{
	printf ("#ifndef CPUEMU_68000_ONLY\n");
}


static void addcycles_ce020 (int cycles, char *s)
{
	if (!using_ce020)
		return;
	if (cycles > 0) {
		if (s == NULL)
			printf ("\t%s (%d);\n", do_cycles, cycles);
		else
			printf ("\t%s (%d); /* %d */\n", do_cycles, cycles, s);
	}
	count_cycles += cycles;
	count_cycles_ce020 += cycles;
}
static void addcycles_ce020 (int cycles)
{
	addcycles_ce020 (cycles, NULL);
}

static void get_prefetch_020 (void)
{
	if (!using_prefetch_020 || no_prefetch_ce020)
		return;
	printf ("\tregs.irc = %s (%d);\n", prefetch_word, m68k_pc_offset);
}
static void get_prefetch_020_0 (void)
{
	if (!using_prefetch_020 || no_prefetch_ce020)
		return;
	printf ("\tregs.irc = %s (0);\n", prefetch_word);
}

static void returntail (bool iswrite)
{
	if (!using_ce020) {
		if (using_prefetch_020) {
			if (!tail_ce020_done) {
				if (!did_prefetch)
					get_prefetch_020 ();
				did_prefetch = 1;
				tail_ce020_done = true;
			}
		}
		return;
	}
	if (!tail_ce020_done) {
		total_ce020 -= 2;
#if 0
		if (iswrite) {
			printf("\t/* C - %d = %d */\n", memory_cycle_cnt, total_ce020 - memory_cycle_cnt);
			total_ce020 -= memory_cycle_cnt;
		} else {
			printf("\t/* C = %d */\n", total_ce020);
		}
#endif
		if (0 && total_ce020 <= 0) {
			printf ("\t/* C was zero */\n");
			total_ce020 = 1;
		}
		if (!did_prefetch)
			get_prefetch_020 ();
		if (total_ce020 > 0)
			addcycles_ce020 (total_ce020);

		//printf ("\tregs.irc = %s;\n", prefetch_word);
		if (0 && total_ce020 >= 2) {
			printf ("\top_cycles = get_cycles () - op_cycles;\n");
			printf ("\top_cycles /= cpucycleunit;\n");
			printf ("\tif (op_cycles < %d) {\n", total_ce020);
			printf ("\t\tdo_cycles_ce020 ((%d) - op_cycles);\n", total_ce020);
			printf ("\t}\n");
		}
#if 0
		if (tail_ce020 > 0) {
			printf ("\tregs.ce020_tail = %d * cpucycleunit;\n", tail_ce020);
			printf ("\tregs.ce020_tail_cycles = get_cycles () + regs.ce020_tail;\n");
		} else {
			printf ("\tregs.ce020_tail = 0;\n");
		}
#endif
		tail_ce020_done = true;
	}
}


static void returncycles (char *s, int cycles)
{
	if (using_ce || using_ce020) {
#if 0
		if (tail_ce020 == 0)
			printf ("\tregs.ce020memcycles -= 2 * cpucycleunit; /* T=0 */ \n");
		else if (tail_ce020 == 1)
			printf ("\tregs.ce020memcycles -= 1 * cpucycleunit; /* T=1 */ \n");
		else if (tail_ce020 == 2)
			printf ("\tregs.ce020memcycles -= 0 * cpucycleunit; /* T=2 */\n");
#endif
		printf ("%sreturn;\n", s);
		return;
	}
  if (using_simple_cycles)
		printf ("%sreturn %d * CYCLE_UNIT / 2 + count_cycles;\n", s, cycles);
	else
		printf ("%sreturn %d * CYCLE_UNIT / 2;\n", s, cycles);
}

static void addcycles_ce020 (char *name, int head, int tail, int cycles)
{
	if (!using_ce020)
		return;
	if (!head && !tail && !cycles)
		return;
	printf ("\t/* %s H:%d,T:%d,C:%d */\n", name, head, tail, cycles);
}
static void addcycles_ce020 (char *name, int head, int tail, int cycles, int ophead)
{
	if (!using_ce020)
		return;
	if (!head && !tail && !cycles && !ophead) {
		printf ("\t/* OP zero */\n");
		return;
	}
	count_cycles += cycles;
	if (!ophead) {
		addcycles_ce020 (name, head, tail, cycles);
	} else if (ophead > 0) {
		printf ("\t/* %s H:%d-,T:%d,C:%d */\n", name, head, tail, cycles);
	} else {
		printf ("\t/* %s H:%d+,T:%d,C:%d */\n", name, head, tail, cycles);
	}
}

static void addcycles000 (int cycles)
{
	if (using_ce)
		printf ("\t%s (%d);\n", do_cycles, cycles);
	count_cycles += cycles;
}

static void returncycles_exception (char *s, int cycles)
{
	if (using_ce || using_ce020) {
		printf ("%sreturn;\n", s);
		return;
	}
	printf ("\t\t%sreturn %d * CYCLE_UNIT / 2;\n", s, cycles);
}

static void addcycles000_nonces(char *s, char *sc)
{
	if (using_simple_cycles) {
		printf("%scount_cycles += (%s) * CYCLE_UNIT / 2;\n", s, sc);
		count_ncycles++;
	}
}
static void addcycles000_nonce(char *s, int c)
{
	if (using_simple_cycles) {
		printf("%scount_cycles += %d * CYCLE_UNIT / 2;\n", s, c);
		count_ncycles++;
	}
}

static void addcycles000_onlyce (int cycles)
{
	if (using_ce) {
		printf ("\t%s (%d);\n", do_cycles, cycles);
	}
}

static void addcycles000_2 (char *s, int cycles)
{
	if (using_ce)
		printf ("%s%s (%d);\n", s, do_cycles, cycles);
	count_cycles += cycles;
}

static void addcycles000_3 (char *s)
{
	if (using_ce)
		printf ("%sif (cycles > 0) %s (cycles);\n", s, do_cycles);
	count_ncycles++;
}

static int isreg (amodes mode)
{
	if (mode == Dreg || mode == Areg)
		return 1;
	return 0;
}

static void start_brace (void)
{
	n_braces++;
	printf ("{");
}

static void close_brace (void)
{
	assert (n_braces > 0);
	n_braces--;
	printf ("}");
}

static void finish_braces (void)
{
	while (n_braces > 0)
		close_brace ();
}

static void pop_braces (int to)
{
	while (n_braces > to)
		close_brace ();
}

static int bit_size (int size)
{
	switch (size) {
	case sz_byte: return 8;
	case sz_word: return 16;
	case sz_long: return 32;
	default: term ();
	}
	return 0;
}

static const char *bit_mask (int size)
{
	switch (size) {
	case sz_byte: return "0xff";
	case sz_word: return "0xffff";
	case sz_long: return "0xffffffff";
	default: term ();
	}
	return 0;
}

static void add_mmu040_movem (int movem)
{
	if (movem != 3)
		return;
	printf ("\tif (mmu040_movem) {\n");
	printf ("\t\tsrca = mmu040_movem_ea;\n");
	printf ("\t} else\n");
	start_brace ();
}

static void gen_nextilong2 (char *type, char *name, int flags, int movem)
{
	int r = m68k_pc_offset;
	m68k_pc_offset += 4;

	printf ("\t%s %s;\n", type, name);
	add_mmu040_movem (movem);
	if (using_ce020) {
		printf("\t%s = %s (%d);\n", name, prefetch_long, r);
		count_read += 2;
	} else if (using_ce) {
		/* we must do this because execution order of (something | something2) is not defined */
		if (flags & GF_NOREFILL) {
			printf ("\t%s = %s (%d) << 16;\n", name, prefetch_word, r + 2);
			count_read++;
			printf ("\t%s |= regs.irc;\n", name);
		} else {
			printf ("\t%s = %s (%d) << 16;\n", name, prefetch_word, r + 2);
			count_read++;
			printf ("\t%s |= %s (%d);\n", name, prefetch_word, r + 4);
			count_read++;
		}
	} else {
		if (using_prefetch) {
			if (flags & GF_NOREFILL) {
				printf ("\t%s = %s (%d) << 16;\n", name, prefetch_word, r + 2);
				count_read++;
				printf ("\t%s |= regs.irc;\n", name);
				insn_n_cycles += 4;
			} else {
				printf ("\t%s = %s (%d) << 16;\n", name, prefetch_word, r + 2);
				count_read += 2;
				printf ("\t%s |= %s (%d);\n", name, prefetch_word, r + 4);
				insn_n_cycles += 8;
			}
		} else {
			count_read += 2;
			insn_n_cycles += 8;
			printf ("\t%s = %s (%d);\n", name, prefetch_long, r);
		}
	}
}
static void gen_nextilong (char *type, char *name, int flags)
{
	gen_nextilong2 (type, name, flags, 0);
}

static const char *gen_nextiword (int flags)
{
	static char buffer[80];
	int r = m68k_pc_offset;
	m68k_pc_offset += 2;

	if (using_ce020) {
		sprintf(buffer, "%s (%d)", prefetch_word, r);
		count_read++;
	} else if (using_ce) {
		if (flags & GF_NOREFILL) {
			strcpy (buffer, "regs.irc");
		} else {
			sprintf (buffer, "%s (%d)", prefetch_word, r + 2);
			count_read++;
		}
	} else {
		if (using_prefetch) {
			if (flags & GF_NOREFILL) {
				strcpy (buffer, "regs.irc");
			} else {
				sprintf (buffer, "%s (%d)", prefetch_word, r + 2);
				count_read++;
				insn_n_cycles += 4;
			}
		} else {
			sprintf (buffer, "%s (%d)", prefetch_word, r);
			count_read++;
			insn_n_cycles += 4;
		}
	}
	return buffer;
}

static const char *gen_nextibyte (int flags)
{
	static char buffer[80];
	int r = m68k_pc_offset;
	m68k_pc_offset += 2;

	if (using_ce020 || using_prefetch_020) {
		sprintf (buffer, "(uae_u8)%s (%d)", prefetch_word, r);
		count_read++;
	} else if (using_ce) {
		if (flags & GF_NOREFILL) {
			strcpy (buffer, "(uae_u8)regs.irc");
		} else {
			sprintf (buffer, "(uae_u8)%s (%d)", prefetch_word, r + 2);
			count_read++;
		}
	} else {
		insn_n_cycles += 4;
		if (using_prefetch) {
			if (flags & GF_NOREFILL) {
				strcpy (buffer, "(uae_u8)regs.irc");
			} else {
				sprintf (buffer, "(uae_u8)%s (%d)", prefetch_word, r + 2);
				insn_n_cycles += 4;
				count_read++;
			}
		} else {
			sprintf (buffer, "%s (%d)", srcbi, r);
			insn_n_cycles += 4;
			count_read++;
		}
	}
	return buffer;
}

static void makefromsr (void)
{
	printf ("\tMakeFromSR();\n");
	if (using_ce || using_ce020)
		printf ("\tregs.ipl_pin = intlev ();\n");
}

static void check_ipl (void)
{
	if (ipl_fetched)
		return;
	if (using_ce || using_ce020)
		printf ("\tipl_fetch ();\n");
	ipl_fetched = true;
}

static void irc2ir (bool dozero)
{
	if (!using_prefetch)
		return;
	if (ir2irc)
		return;
	ir2irc = 1;
	printf ("\tregs.ir = regs.irc;\n");
	if (dozero)
		printf ("\tregs.irc = 0;\n");
	check_ipl ();
}
static void irc2ir (void)
{
	irc2ir (false);
}

static void fill_prefetch_2 (void)
{
	if (!using_prefetch)
		return;
	printf ("\t%s (%d);\n", prefetch_word, m68k_pc_offset + 2);
	did_prefetch = 1;
	ir2irc = 0;
	count_read++;
	insn_n_cycles += 4;
}

static void fill_prefetch_1 (int o)
{
	if (using_prefetch) {
		printf ("\t%s (%d);\n", prefetch_word, o);
		did_prefetch = 1;
		ir2irc = 0;
		count_read++;
		insn_n_cycles += 4;
	}
}

static void fill_prefetch_full (void)
{
	if (using_prefetch) {
		fill_prefetch_1 (0);
		irc2ir ();
		fill_prefetch_1 (2);
	} else if (using_prefetch_020) {
		did_prefetch = 2;
		total_ce020 -= 4;
		returntail (false);
		if (cpu_level >= 3)
			printf ("\tfill_prefetch_030 ();\n");
		else if (cpu_level == 2)
			printf ("\tfill_prefetch_020 ();\n");
	} else {
    // Count cycles for 2nd prefetch
		count_read++;
		insn_n_cycles += 4;
	}
}

// 68000 and 68010 only
static void fill_prefetch_full_000 (void)
{
	if (!using_prefetch)
		return;
	fill_prefetch_full ();
}

// 68020+
static void fill_prefetch_full_020 (void)
{
	if (!using_prefetch_020)
		return;
	fill_prefetch_full ();
}

static void fill_prefetch_0 (void)
{
	if (!using_prefetch)
		return;
	printf ("\t%s (0);\n", prefetch_word);
	did_prefetch = 1;
	ir2irc = 0;
	count_read++;
	insn_n_cycles += 4;
}

static void dummy_prefetch (void)
{
	int o = m68k_pc_offset + 2;
	if (!using_prefetch)
		return;
	//printf ("\t%s (%d);\n", srcwi, o);
	count_read++;
	insn_n_cycles += 4;
}

static void fill_prefetch_next_1 (void)
{
	irc2ir ();
	fill_prefetch_1 (m68k_pc_offset + 2);
}

static void fill_prefetch_next (void)
{
	if (using_prefetch) {
		fill_prefetch_next_1 ();
	}
//	if (using_prefetch_020) {
//		printf ("\t%s (%d);\n", prefetch_word, m68k_pc_offset);
//		did_prefetch = 1;
//	}
}

static void fill_prefetch_finish (void)
{
	if (did_prefetch)
		return;
	if (using_prefetch) {
		fill_prefetch_1 (m68k_pc_offset);
	}
	if (using_prefetch_020) {
		did_prefetch = 1;
	}
}

static void setpc (const char *format, ...)
{
	va_list parms;
	char buffer[1000];

	va_start (parms, format);
	_vsnprintf (buffer, 1000 - 1, format, parms);
	va_end (parms);

	if (using_mmu || using_prefetch || using_prefetch_020)
		printf ("\tm68k_setpci (%s);\n", buffer);
	else
		printf ("\tm68k_setpc (%s);\n", buffer);
}

static void incpc (const char *format, ...)
{
	va_list parms;
	char buffer[1000];

	va_start (parms, format);
	_vsnprintf (buffer, 1000 - 1, format, parms);
	va_end (parms);

	if (using_mmu || using_prefetch || using_prefetch_020)
		printf ("\tm68k_incpci (%s);\n", buffer);
	else
		printf ("\tm68k_incpc (%s);\n", buffer);
}

static void sync_m68k_pc (void)
{
	m68k_pc_offset_old = m68k_pc_offset;
	if (m68k_pc_offset == 0)
		return;
	incpc ("%d", m68k_pc_offset);
	m68k_pc_offset = 0;
}

static void sync_m68k_pc_noreset (void)
{
	sync_m68k_pc ();
	m68k_pc_offset = m68k_pc_offset_old;
}

static void addmmufixup (char *reg)
{
	if (!using_mmu)
		return;
	if (using_mmu == 68040 && (mmufixupstate || mmufixupcnt > 0))
		return;
	printf ("\tmmufixup[%d].reg = %s;\n", mmufixupcnt, reg);
	printf ("\tmmufixup[%d].value = m68k_areg (regs, %s);\n", mmufixupcnt, reg);
	mmufixupstate |= 1 << mmufixupcnt;
	mmufixupcnt++;
}

static void clearmmufixup (int cnt)
{
	if (mmufixupstate & (1 << cnt)) {
		printf ("\tmmufixup[%d].reg = -1;\n", cnt);
		mmufixupstate &= ~(1 << cnt);
	}
}

static void gen_set_fault_pc (void)
{
	if (using_mmu != 68040)
		return;
	sync_m68k_pc ();
	printf ("\tregs.instruction_pc = %s;\n", getpc);
	printf ("\tmmu_restart = false;\n");
	m68k_pc_offset = 0;
	clearmmufixup (0);
}

static void syncmovepc (int getv, int flags)
{
#if 0
	if (!(flags & GF_MOVE))
		return;
	if (getv == 1) {
		sync_m68k_pc ();
		//fill_prefetch_next ();
	}
#endif
}

static void head_cycs (int h)
{
	if (head_ce020_cycs_done)
		return;
	if (h < 0)
		return;
	//printf ("\tdo_sync_tail (%d);\n", h);
	//printf ("\tdo_head_cycles_ce020 (%d);\n", h);
	head_ce020_cycs_done = true;
	tail_ce020 = -1;
}

static void add_head_cycs (int h)
{
	if (!using_ce020)
		return;
	head_ce020_cycs_done = false;
	head_cycs (h);
}

static void addopcycles_ce20 (int h, int t, int c, int subhead)
{
	head_cycs (h);

	//c = 0;
#if 0
	if (tail_ce020 == 1)
		printf ("\tregs.ce020memcycles -= 1 * cpucycleunit; /* T=1 */ \n");
	else if (tail_ce020 == 2)
		printf ("\tregs.ce020memcycles -= 2 * cpucycleunit; /* T=2 */\n");
#endif
	if (1 && !subhead && (h > 0 || t > 0 || c > 0) && got_ea_ce020) {
		if (!did_prefetch) {
			get_prefetch_020 ();
			did_prefetch = 1;
		}
		if (1) {
			if (h > 0) {
				printf ("\tif (regs.ce020memcycles > %d * cpucycleunit)\n", h);
				printf ("\t\tregs.ce020memcycles = %d * cpucycleunit;\n", h);
			} else {
				printf ("\tregs.ce020memcycles = 0;\n", h);
			}
		}
	}

#if 0
	if (tail_ce020 >= 0 && h >= 0 && head_in_ea_ce020 == 0) {
		int largest = tail_ce020 > h ? tail_ce020: h;
		if (tail_ce020 != h) {
			//printf ("\tdo_cycles_ce020 (%d - %d);\n", tail_ce020 > h ? tail_ce020 : h, tail_ce020 > h ? h : tail_ce020);
			//printf ("\tdo_cycles_ce020 (%d - %d);\n", tail_ce020 > h ? tail_ce020 : h, tail_ce020 > h ? h : tail_ce020);
			if (h) {
				printf ("\tregs.ce020memcycles -= %d * cpucycleunit;\n", h);
				printf ("\tif (regs.ce020memcycles < 0) {\n");
				//printf ("\t\tx_do_cycles (-regs.ce020memcycles);\n");
				printf ("\t\tregs.ce020memcycles = 0;\n");
				printf ("\t}\n");
			} else {
				printf ("\tregs.ce020memcycles = 0;\n");
			}
			//printf ("\tregs.ce020memcycles = 0;\n");

#if 0
		if (tail_ce020)
			printf ("\tregs.ce020_tail = get_cycles () - regs.ce020_tail;\n");
		else
			printf ("\tregs.ce020_tail = 0;\n");
		printf ("\tif (regs.ce020_tail < %d * cpucycleunit)\n", largest);
		printf ("\t\tx_do_cycles (%d * cpucycleunit - regs.ce020_tail);\n", largest);
#endif
		} else if (h) {
			printf ("\t/* ea tail == op head (%d) */\n", h);

			printf ("\tregs.ce020memcycles -= %d * cpucycleunit;\n", h);
			printf ("\tif (regs.ce020memcycles < 0) {\n");
			//printf ("\t\tx_do_cycles (-regs.ce020memcycles);\n");
			printf ("\t\tregs.ce020memcycles = 0;\n");
			printf ("\t}\n");
		}
	}
#endif

	if (h < 0)
		h = 0;
	
	//c = 0;
	
	// c = internal cycles needed after head cycles and before tail cycles. Not total cycles.
	addcycles_ce020 ("op", h, t, c - h - t, -subhead);
	//printf ("\tregs.irc = get_word_ce020_prefetch (%d);\n", m68k_pc_offset);
#if 0
	if (c - h - t > 0) {
		printf ("\t%s (%d);\n", do_cycles, c - h - t);
		count_cycles_ce020 += c;
		count_cycles += c;
	}
#endif
	//printf ("\tregs.ce020_tail = 0;\n");
	total_ce020 = c;
	tail_ce020 = t;
//	if (total_ce020 >= 2)
//		printf ("\tint op_cycles = get_cycles ();\n");
}

static void addop_ce020 (instr *curi, int subhead)
{
	if (!using_ce020)
		return;
	int h = curi->head;
	int t = curi->tail;
	int c = curi->clocks;
#if 0
	if ((((curi->sduse & 2) && !isreg (curi->smode)) || (((curi->sduse >> 4) & 2) && !isreg (curi->dmode))) && using_waitstates) {
		t += using_waitstates;
		c += using_waitstates;
	}
#endif
	addopcycles_ce20 (h, t, c, -subhead);
}

static void addcycles_ea_ce020 (char *ea, int h, int t, int c, int oph)
{
	head_cycs (h + oph);

//	if (!h && !h && !c && !oph)
//		return;

	c = c - h - t;

	//c = 0;

	if (!oph) {
		printf ("\t/* ea H:%d,T:%d,C:%d %s */\n", h, t, c, ea);
	} else {
		if (oph && t)
			term ("Both op head and tail can't be non-zero");
		if (oph > 0) {
			printf ("\t/* ea H:%d+%d=%d,T:%d,C:%d %s */\n", h, oph, h + oph, t, c, ea);
			h += oph;
		} else {
			printf ("\t/* ea H:%d-%d=%d,T:%d,C:%d %s */\n", h, -oph, h + oph, t, c, ea);
			h += oph;
		}
	}

	if (h) {
		printf ("\tif (regs.ce020memcycles > %d * cpucycleunit)\n", h);
		printf ("\t\tregs.ce020memcycles = %d * cpucycleunit;\n", h);
	} else {
		printf ("\tregs.ce020memcycles = 0;\n", h);
	}

	if (1 && c > 0) {
		printf ("\t%s (%d);\n", do_cycles, c);
		count_cycles += c;
	}
	tail_ce020 = t;
	head_in_ea_ce020 = oph;
	got_ea_ce020 = true;
//	if (t > 0)
//		printf ("\tregs.ce020_tail = get_cycles () + %d * cpucycleunit;\n", t);
}
static void addcycles_ea_ce020 (char *ea, int h, int t, int c)
{
	addcycles_ea_ce020 (ea, h, t, c, 0);
}

#define SETCE020(h2,t2,c2) { h = h2; t = t2; c = c2; }
#define SETCE020H(h2,t2,c2) { h = h2; oph = curi ? curi->head : 0; t = t2; c = c2; }

static int gence020cycles_fiea (instr *curi, wordsizes ssize, amodes dmode)
{
	bool l = ssize == sz_long;
	int h = 0, t = 0, c = 0, oph = 0;
	switch (dmode)
	{
	case Dreg:
	case Areg:
		if (!l)
			SETCE020H(2, 0, 2)
		else
			SETCE020H(4, 0, 4)
		break;
	case Aind: // (An)
		if (!l)
			SETCE020(1, 1, 3)
		else
			SETCE020(1, 0, 4)
		break;
	case Aipi: // (An)+
		if (!l)
			SETCE020(2, 1, 5)
		else
			SETCE020(4, 1, 7)
		break;
	case Apdi: // -(An)
		if (!l)
			SETCE020(2, 2, 4)
		else
			SETCE020(2, 0, 4)
		break;
	case Ad8r: // (d8,An,Xn)
	case PC8r: // (d8,PC,Xn)
		if (!l)
			SETCE020(6, 2, 8)
		else
			SETCE020(8, 2, 10)
		break;
	case Ad16: // (d16,An)
	case PC16: // (d16,PC)
		if (!l)
			SETCE020(2, 0, 4)
		else
			SETCE020(4, 0, 6)
		break;
	case absw:
		if (!l)
			SETCE020(4, 2, 6)
		else
			SETCE020(6, 2, 8)
		break;
	case absl:
		if (!l)
			SETCE020(3, 0, 6)
		else
			SETCE020(5, 0, 8)
		break;
	}
	addcycles_ea_ce020 ("fiea", h, t, c, oph);
	return oph;
}


static int gence020cycles_ciea (instr *curi, wordsizes ssize, amodes dmode)
{
	int h = 0, t = 0, c = 0, oph = 0;
	bool l = ssize == sz_long;
	switch (dmode)
	{
	case Dreg:
	case Areg:
		if (!l)
			SETCE020H(2, 0, 2)
		else
			SETCE020H(4, 0, 4)
		break;
	case Aind: // (An)
		if (!l)
			SETCE020H(2, 0, 2)
		else
			SETCE020H(4, 0, 4)
		break;
	case Aipi: // (An)+
		if (!l)
			SETCE020(2, 0, 4)
		else
			SETCE020(4, 0, 6)
		break;
	case Apdi: // -(An)
		if (!l)
			SETCE020H(2, 0, 2)
		else
			SETCE020H(4, 0, 4)
		break;
	case Ad8r: // (d8,An,Xn)
	case PC8r: // (d8,PC,Xn)
		if (!l)
			SETCE020H(6, 0, 6)
		else
			SETCE020H(8, 0, 8)
		break;
	case Ad16: // (d16,An)
	case PC16: // (d16,PC)
		if (!l)
			SETCE020H(4, 0, 4)
		else
			SETCE020H(6, 0, 6)
		break;
	case absw:
		if (!l)
			SETCE020H(4, 0, 4)
		else
			SETCE020H(6, 0, 6)
		break;
	case absl:
		if (!l)
			SETCE020H(6, 0, 6)
		else
			SETCE020H(8, 0, 8)
		break;
	}
	addcycles_ea_ce020 ("ciea", h, t, c, oph);
	return oph;
}


static int gence020cycles_fea (amodes mode)
{
	int h = 0, t = 0, c = 0, ws = 0;
	switch (mode)
	{
	case Dreg:
	case Areg:
		SETCE020(0, 0, 0)
		break;
	case Aind: // (An)
		ws++;
		SETCE020(1, 1, 3)
		break;
	case Aipi: // (An)+
		ws++;
		SETCE020(0, 1, 3)
		break;
	case Apdi: // -(An)
		ws++;
		SETCE020(2, 2, 4)
		break;
	case Ad8r: // (d8,An,Xn)
	case PC8r: // (d8,PC,Xn)
		ws++;
		SETCE020(4, 2, 6)
		break;
	case Ad16: // (d16,An)
	case PC16: // (d16,PC)
		ws++;
		SETCE020(2, 2, 4)
		break;
	case absw:
		ws++;
		SETCE020(2, 2, 4)
		break;
	case absl:
		ws++;
		SETCE020(1, 0, 4)
		break;
	}
#if 0
	if (using_waitstates) {
		t += ws * using_waitstates;
		c += ws * using_waitstates;
	}
#endif
	addcycles_ea_ce020 ("fea", h, t, c);
	return 0;
}

static int gence020cycles_cea (instr *curi, amodes mode)
{
	int h = 0, t = 0, c = 0, oph = 0;
	switch (mode)
	{
	case Dreg:
	case Areg:
		SETCE020(0, 0, 0);
		break;
	case Aind: // (An)
		SETCE020H(2 + h, 0, 2);
		break;
	case Aipi: // (An)+
		SETCE020(0, 0, 2);
		break;
	case Apdi: // -(An)
		SETCE020H(2, 0, 2)
		break;
	case Ad8r: // (d8,An,Xn)
	case PC8r: // (d8,PC,Xn)
		SETCE020H(4, 0, 4)
		break;
	case Ad16: // (d16,An)
	case PC16: // (d16,PC)
		SETCE020H(2, 0, 2)
		break;
	case absw:
		SETCE020H(2, 0, 2)
		break;
	case absl:
		SETCE020H(4, 0, 4)
		break;
	}
	addcycles_ea_ce020 ("cea", h, t, c, oph);
	return oph;
}

static int gence020cycles_jea (instr *curi, amodes mode)
{
	int h = 0, t = 0, c = 0, oph = 0;
	switch (mode)
	{
	case Aind: // (An)
		SETCE020H(2, 0, 2)
		break;
	case Ad16: // (d16,An)
	case PC16: // (d16,PC)
		SETCE020H(4, 0, 4)
		break;
	case absw:
		SETCE020H(2, 0, 2)
		break;
	case absl:
		SETCE020H(2, 0, 2)
		break;
	}
	addcycles_ea_ce020 ("jea", h, t, c, oph);
	return oph;
}

static void next_level_000 (void)
{
	if (next_cpu_level < 0)
		next_cpu_level = 0;
}

static void maybeaddop_ce020 (int flags)
{
	if (flags & GF_OPCE020)
		addop_ce020 (curi_ce020, subhead_ce020);
}


/* getv == 1: fetch data; getv != 0: check for odd address. If movem != 0,
* the calling routine handles Apdi and Aipi modes.
* gb-- movem == 2 means the same thing but for a MOVE16 instruction */

/* fixup indicates if we want to fix up adress registers in pre decrement
* or post increment mode now (0) or later (1). A value of 2 will then be
* used to do the actual fix up. This allows to do all memory readings
* before any register is modified, and so to rerun operation without
* side effect in case a bus fault is generated by any memory access.
* XJ - 2006/11/13 */

static void genamode2x (amodes mode, char *reg, wordsizes size, char *name, int getv, int movem, int flags, int fetchmode)
{
	char namea[100];
	int pc_68000_offset = m68k_pc_offset;
	bool rmw = false;
	int pc_68000_offset_fetch = 0;
	int pc_68000_offset_store = 0;

	sprintf (namea, "%sa", name);
	if ((flags & GF_RMW) && using_mmu == 68060) {
		strcpy (rmw_varname, name);
		candormw = true;
		rmw = true;
	}

	start_brace ();

	switch (mode) {
	case Dreg:
		if (movem)
			term ();
		if (getv == 1)
			switch (size) {
			case sz_byte:
#ifdef USE_DUBIOUS_BIGENDIAN_OPTIMIZATION
				/* This causes the target compiler to generate better code on few systems */
				printf ("\tuae_s8 %s = ((uae_u8*)&m68k_dreg (regs, %s))[3];\n", name, reg);
#else
				printf ("\tuae_s8 %s = m68k_dreg (regs, %s);\n", name, reg);
#endif
				break;
			case sz_word:
#ifdef USE_DUBIOUS_BIGENDIAN_OPTIMIZATION
				printf ("\tuae_s16 %s = ((uae_s16*)&m68k_dreg (regs, %s))[1];\n", name, reg);
#else
				printf ("\tuae_s16 %s = m68k_dreg (regs, %s);\n", name, reg);
#endif
				break;
			case sz_long:
				printf ("\tuae_s32 %s = m68k_dreg (regs, %s);\n", name, reg);
				break;
			default:
				term ();
		}
		maybeaddop_ce020 (flags);
		syncmovepc (getv, flags);
		return;
	case Areg:
		if (movem)
			term ();
		if (getv == 1)
			switch (size) {
			case sz_word:
				printf ("\tuae_s16 %s = m68k_areg (regs, %s);\n", name, reg);
				break;
			case sz_long:
				printf ("\tuae_s32 %s = m68k_areg (regs, %s);\n", name, reg);
				break;
			default:
				term ();
		}
		maybeaddop_ce020 (flags);
		syncmovepc (getv, flags);
		return;
	case Aind: // (An)
		switch (fetchmode)
		{
			case fetchmode_fea:
			addcycles_ce020 (1);
			break;
			case fetchmode_cea:
			addcycles_ce020 (2);
			break;
			case fetchmode_jea:
			addcycles_ce020 (2);
			break;
		}
		printf ("\tuaecptr %sa;\n", name);
		add_mmu040_movem (movem);
		printf ("\t%sa = m68k_areg (regs, %s);\n", name, reg);
		break;
	case Aipi: // (An)+
		switch (fetchmode)
		{
			case fetchmode_fea:
			addcycles_ce020 (1);
			break;
			case fetchmode_cea:
			break;
		}
		printf ("\tuaecptr %sa;\n", name);
		add_mmu040_movem (movem);
		printf ("\t%sa = m68k_areg (regs, %s);\n", name, reg);
		break;
	case Apdi: // -(An)
		switch (fetchmode)
		{
			case fetchmode_fea:
			case fetchmode_cea:
			addcycles_ce020 (2);
			break;
		}
		printf ("\tuaecptr %sa;\n", name);
		add_mmu040_movem (movem);
		switch (size) {
		case sz_byte:
			if (movem)
				printf ("\t%sa = m68k_areg (regs, %s);\n", name, reg);
			else
				printf ("\t%sa = m68k_areg (regs, %s) - areg_byteinc[%s];\n", name, reg, reg);
			break;
		case sz_word:
			printf ("\t%sa = m68k_areg (regs, %s) - %d;\n", name, reg, movem ? 0 : 2);
			break;
		case sz_long:
			printf ("\t%sa = m68k_areg (regs, %s) - %d;\n", name, reg, movem ? 0 : 4);
			break;
		default:
			term ();
		}
		if (!(flags & GF_APDI)) {
			addcycles000 (2);
			insn_n_cycles += 2;
			count_cycles_ea += 2;
			pc_68000_offset_fetch += 2;
		}
		break;
	case Ad16: // (d16,An)
		printf ("\tuaecptr %sa;\n", name);
		add_mmu040_movem (movem);
		printf ("\t%sa = m68k_areg (regs, %s) + (uae_s32)(uae_s16)%s;\n", name, reg, gen_nextiword (flags));
		count_read_ea++; 
		break;
	case PC16: // (d16,PC,Xn)
		printf ("\tuaecptr %sa;\n", name);
		add_mmu040_movem (movem);
		printf ("\t%sa = %s + %d;\n", name, getpc, m68k_pc_offset);
		printf ("\t%sa += (uae_s32)(uae_s16)%s;\n", name, gen_nextiword (flags));
		break;
	case Ad8r: // (d8,An,Xn)
		switch (fetchmode)
		{
			case fetchmode_fea:
			addcycles_ce020 (4);
			break;
			case fetchmode_cea:
			case fetchmode_jea:
			break;
		}
		printf ("\tuaecptr %sa;\n", name);
		if (cpu_level > 1) {
			if (next_cpu_level < 1)
				next_cpu_level = 1;
			sync_m68k_pc ();
			add_mmu040_movem (movem);
			start_brace ();
			/* This would ordinarily be done in gen_nextiword, which we bypass.  */
			insn_n_cycles += 4;
			printf ("\t%sa = %s (m68k_areg (regs, %s), %d);\n", name, disp020, reg, mmudisp020cnt++);
		} else {
			if (!(flags & GF_AD8R)) {
				addcycles000 (2);
				insn_n_cycles += 2;
				count_cycles_ea += 2;
			}
			if ((flags & GF_NOREFILL) && using_prefetch) {
				printf ("\t%sa = %s (m68k_areg (regs, %s), regs.irc);\n", name, disp000, reg);
			} else {
				printf ("\t%sa = %s (m68k_areg (regs, %s), %s);\n", name, disp000, reg, gen_nextiword (flags));
			}
			count_read_ea++; 
		}
		break;
	case PC8r: // (d8,PC,Xn)
		switch (fetchmode)
		{
			case fetchmode_fea:
			addcycles_ce020 (4);
			break;
			case fetchmode_cea:
			case fetchmode_jea:
			break;
		}
		printf ("\tuaecptr tmppc;\n");
		printf ("\tuaecptr %sa;\n", name);
		if (cpu_level > 1) {
			if (next_cpu_level < 1)
				next_cpu_level = 1;
			sync_m68k_pc ();
			add_mmu040_movem (movem);
			start_brace ();
			/* This would ordinarily be done in gen_nextiword, which we bypass.  */
			insn_n_cycles += 4;
			printf ("\ttmppc = %s;\n", getpc);
			printf ("\t%sa = %s (tmppc, %d);\n", name, disp020, mmudisp020cnt++);
		} else {
			printf ("\ttmppc = %s + %d;\n", getpc, m68k_pc_offset);
			if (!(flags & GF_PC8R)) {
				addcycles000 (2);
				insn_n_cycles += 2;
				count_cycles_ea += 2;
			}
			if ((flags & GF_NOREFILL) && using_prefetch) {
				printf ("\t%sa = %s (tmppc, regs.irc);\n", name, disp000);
			} else {
				printf ("\t%sa = %s (tmppc, %s);\n", name, disp000, gen_nextiword (flags));
			}
		}

		break;
	case absw:
		printf ("\tuaecptr %sa;\n", name);
		add_mmu040_movem (movem);
		printf ("\t%sa = (uae_s32)(uae_s16)%s;\n", name, gen_nextiword (flags));
		pc_68000_offset_fetch += 2;
		break;
	case absl:
		gen_nextilong2 ("uaecptr", namea, flags, movem);
		count_read_ea += 2;
		pc_68000_offset_fetch += 4;
		pc_68000_offset_store += 2;
		break;
	case imm:
		// fetch immediate address
		if (getv != 1)
			term ();
		insn_n_cycles020++;
		switch (size) {
		case sz_byte:
			printf ("\tuae_s8 %s = %s;\n", name, gen_nextibyte (flags));
			count_read_ea++;
			break;
		case sz_word:
			printf ("\tuae_s16 %s = %s;\n", name, gen_nextiword (flags));
			count_read_ea++;
			break;
		case sz_long:
			gen_nextilong ("uae_s32", name, flags);
			count_read_ea += 2;
			break;
		default:
			term ();
		}
		maybeaddop_ce020 (flags);
		syncmovepc (getv, flags);
		return;
	case imm0:
		if (getv != 1)
			term ();
		printf ("\tuae_s8 %s = %s;\n", name, gen_nextibyte (flags));
		count_read_ea++;
		maybeaddop_ce020 (flags);
		syncmovepc (getv, flags);
		return;
	case imm1:
		if (getv != 1)
			term ();
		printf ("\tuae_s16 %s = %s;\n", name, gen_nextiword (flags));
		count_read_ea++;
		maybeaddop_ce020 (flags);
		syncmovepc (getv, flags);
		return;
	case imm2:
		if (getv != 1)
			term ();
		gen_nextilong ("uae_s32", name, flags);
		count_read_ea += 2;
		maybeaddop_ce020 (flags);
		syncmovepc (getv, flags);
		return;
	case immi:
		if (getv != 1)
			term ();
		printf ("\tuae_u32 %s = %s;\n", name, reg);
		maybeaddop_ce020 (flags);
		syncmovepc (getv, flags);
		return;
	default:
		term ();
	}

	syncmovepc (getv, flags);
	maybeaddop_ce020 (flags);

	/* We get here for all non-reg non-immediate addressing modes to
	* actually fetch the value. */

	if ((using_prefetch || using_ce) && using_exception_3 && getv != 0 && size != sz_byte) {
		int offset = 0;
		if (getv == 2) {
			// store
			if (pc_68000_offset) {
				offset = pc_68000_offset + pc_68000_offset_store + 2;
			}
		} else {
			// fetch
			pc_68000_offset_fetch += 2;
			offset = pc_68000_offset_fetch;
		}
		printf ("\tif (%sa & 1) {\n", name);
		if (offset)
			incpc("%d", offset);
		printf ("\t\texception3 (opcode, %sa);\n", name);
		returncycles_exception ("", (count_read + 1 + count_write) * 4 + count_cycles);
		printf ("\t}\n");
		start_brace ();
	}

	if (flags & GF_PREFETCH)
		fill_prefetch_next ();
	else if (flags & GF_IR2IRC)
		irc2ir (true);

	if (getv == 1) {
		start_brace ();
		if (using_ce020 || using_prefetch_020) {
			switch (size) {
			case sz_byte: insn_n_cycles += 4; printf ("\tuae_s8 %s = %s (%sa);\n", name, srcb, name); count_read++; break;
			case sz_word: insn_n_cycles += 4; printf ("\tuae_s16 %s = %s (%sa);\n", name, srcw, name); count_read++; break;
			case sz_long: insn_n_cycles += 8; printf ("\tuae_s32 %s = %s (%sa);\n", name, srcl, name); count_read += 2; break;
			default: term ();
			}
		} else if (using_ce || using_prefetch) {
			switch (size) {
			case sz_byte: insn_n_cycles += 4; printf ("\tuae_s8 %s = %s (%sa);\n", name, srcb, name); count_read++; break;
			case sz_word: insn_n_cycles += 4; printf ("\tuae_s16 %s = %s (%sa);\n", name, srcw, name); count_read++; break;
			case sz_long: insn_n_cycles += 8; printf ("\tuae_s32 %s = %s (%sa) << 16; %s |= %s (%sa + 2);\n", name, srcw, name, name, srcw, name); count_read += 2; break;
			default: term ();
			}
		} else if (using_mmu) {
			if (flags & GF_FC) {
				switch (size) {
				case sz_byte: insn_n_cycles += 4; printf ("\tuae_s8 %s = sfc%s_get_byte (%sa);\n", name, mmu_postfix, name); break;
				case sz_word: insn_n_cycles += 4; printf ("\tuae_s16 %s = sfc%s_get_word (%sa);\n", name, mmu_postfix, name); break;
				case sz_long: insn_n_cycles += 8; printf ("\tuae_s32 %s = sfc%s_get_long (%sa);\n", name, mmu_postfix, name); break;
				default: term ();
				}
			} else {
				switch (size) {
				case sz_byte: insn_n_cycles += 4; printf ("\tuae_s8 %s = %s (%sa);\n", name, (flags & GF_LRMW) ? srcblrmw : (rmw ? srcbrmw : srcb), name); break;
				case sz_word: insn_n_cycles += 4; printf ("\tuae_s16 %s = %s (%sa);\n", name, (flags & GF_LRMW) ? srcwlrmw : (rmw ? srcwrmw : srcw), name); break;
				case sz_long: insn_n_cycles += 8; printf ("\tuae_s32 %s = %s (%sa);\n", name, (flags & GF_LRMW) ? srcllrmw : (rmw ? srclrmw : srcl), name); break;
				default: term ();
				}
			}
		} else {
			switch (size) {
			case sz_byte: insn_n_cycles += 4; printf ("\tuae_s8 %s = %s (%sa);\n", name, srcb, name); count_read++; break;
			case sz_word: insn_n_cycles += 4; printf ("\tuae_s16 %s = %s (%sa);\n", name, srcw, name); count_read++; break;
			case sz_long: insn_n_cycles += 8; printf ("\tuae_s32 %s = %s (%sa);\n", name, srcl, name); count_read += 2; break;
			default: term ();
			}
		}
	}

	/* We now might have to fix up the register for pre-dec or post-inc
	* addressing modes. */
	if (!movem)
		switch (mode) {
		case Aipi:
			addmmufixup (reg);
			switch (size) {
			case sz_byte:
				printf ("\tm68k_areg (regs, %s) += areg_byteinc[%s];\n", reg, reg);
				break;
			case sz_word:
				printf ("\tm68k_areg (regs, %s) += 2;\n", reg);
				break;
			case sz_long:
				printf ("\tm68k_areg (regs, %s) += 4;\n", reg);
				break;
			default:
				term ();
			}
			break;
		case Apdi:
			addmmufixup (reg);
			printf ("\tm68k_areg (regs, %s) = %sa;\n", reg, name);
			break;
		default:
			break;
	}

	if (movem == 3) {
		close_brace ();
	}
}

static void genamode2 (amodes mode, char *reg, wordsizes size, char *name, int getv, int movem, int flags)
{
	genamode2x (mode, reg, size, name, getv, movem, flags, -1);
}

static void genamode (instr *curi, amodes mode, char *reg, wordsizes size, char *name, int getv, int movem, int flags)
{
	int oldfixup = mmufixupstate;
	int subhead = 0;
	if (using_ce020 && curi) {
		switch (curi->fetchmode)
		{
		case fetchmode_fea:
			subhead = gence020cycles_fea (mode);
		break;
		case fetchmode_cea:
			subhead = gence020cycles_cea (curi, mode);
		break;
		case fetchmode_jea:
			subhead = gence020cycles_jea (curi, mode);
		break;
		}
		genamode2x (mode, reg, size, name, getv, movem, flags, curi->fetchmode);
	} else {
		genamode2 (mode, reg, size, name, getv, movem, flags);
	}
	if (using_mmu == 68040 && (oldfixup & 1)) {
		// we have fixup already active = this genamode call is destination mode and we can now clear previous source fixup.
		clearmmufixup (0);
	}
	if (using_ce020 && curi)
		addop_ce020 (curi, subhead);
}

static void genamode3 (instr *curi, amodes mode, char *reg, wordsizes size, char *name, int getv, int movem, int flags)
{
	int oldfixup = mmufixupstate;
	genamode2x (mode, reg, size, name, getv, movem, flags, curi ? curi->fetchmode : -1);
	if (using_mmu == 68040 && (oldfixup & 1)) {
		// we have fixup already active = this genamode call is destination mode and we can now clear previous source fixup.
		clearmmufixup (0);
	}
}

static void genamodedual (instr *curi, amodes smode, char *sreg, wordsizes ssize, char *sname, int sgetv, int sflags,
					   amodes dmode, char *dreg, wordsizes dsize, char *dname, int dgetv, int dflags)
{
	int subhead = 0;
	bool eadmode = false;

	if (using_ce020) {
		switch (curi->fetchmode)
		{
		case fetchmode_fea:
			if (smode >= imm || isreg (smode)) {
				subhead = gence020cycles_fea (dmode);
				eadmode = true;
			} else {
				subhead = gence020cycles_fea (smode);
			}
		break;
		case fetchmode_cea:
			subhead = gence020cycles_cea (curi, smode);
		break;
		case fetchmode_fiea:
			subhead = gence020cycles_fiea (curi, ssize, dmode);
		break;
		case fetchmode_ciea:
			subhead = gence020cycles_ciea (curi, ssize, dmode);
		break;
		case fetchmode_jea:
			subhead = gence020cycles_jea (curi, smode);
		break;
		default:
			printf ("\t/* No EA */\n");
		break;
		}
	}
	subhead_ce020 = subhead;
	curi_ce020 = curi;
	genamode3 (curi, smode, sreg, ssize, sname, sgetv, 0, sflags);
	genamode3 (NULL, dmode, dreg, dsize, dname, dgetv, 0, dflags | (eadmode == true ? GF_OPCE020 : 0));
	if (eadmode == false)
		maybeaddop_ce020 (GF_OPCE020);
}

static void genastore_2 (char *from, amodes mode, char *reg, wordsizes size, char *to, int store_dir, int flags)
{
	if (candormw) {
		if (strcmp (rmw_varname, to) != 0)
			candormw = false;
	}
	genastore_done = true;
	returntail (mode != Dreg && mode != Areg);

	switch (mode) {
	case Dreg:
		switch (size) {
		case sz_byte:
			printf ("\tm68k_dreg (regs, %s) = (m68k_dreg (regs, %s) & ~0xff) | ((%s) & 0xff);\n", reg, reg, from);
			break;
		case sz_word:
			printf ("\tm68k_dreg (regs, %s) = (m68k_dreg (regs, %s) & ~0xffff) | ((%s) & 0xffff);\n", reg, reg, from);
			break;
		case sz_long:
			printf ("\tm68k_dreg (regs, %s) = (%s);\n", reg, from);
			break;
		default:
			term ();
		}
		break;
	case Areg:
		switch (size) {
		case sz_word:
			printf ("\tm68k_areg (regs, %s) = (uae_s32)(uae_s16)(%s);\n", reg, from);
			break;
		case sz_long:
			printf ("\tm68k_areg (regs, %s) = (%s);\n", reg, from);
			break;
		default:
			term ();
		}
		break;
	case Aind:
	case Aipi:
	case Apdi:
	case Ad16:
	case Ad8r:
	case absw:
	case absl:
	case PC16:
	case PC8r:
		if (!(flags & GF_NOFAULTPC))
			gen_set_fault_pc ();
		if (using_ce020 || using_prefetch_020) {
			switch (size) {
			case sz_byte:
				printf ("\t%s (%sa, %s);\n", dstb, to, from);
				count_write++;
				break;
			case sz_word:
				if (cpu_level < 2 && (mode == PC16 || mode == PC8r))
					term ();
				printf ("\t%s (%sa, %s);\n", dstw, to, from);
				count_write++;
				break;
			case sz_long:
				if (cpu_level < 2 && (mode == PC16 || mode == PC8r))
					term ();
				printf ("\t%s (%sa, %s);\n", dstl, to, from);
				count_write += 2;
				break;
			default:
				term ();
			}
		} else if (using_ce) {
			switch (size) {
			case sz_byte:
				printf ("\tx_put_byte (%sa, %s);\n", to, from);
				count_write++;
				break;
			case sz_word:
				if (cpu_level < 2 && (mode == PC16 || mode == PC8r))
					term ();
				printf ("\tx_put_word (%sa, %s);\n", to, from);
				count_write++;
				break;
			case sz_long:
				if (cpu_level < 2 && (mode == PC16 || mode == PC8r))
					term ();
				if (store_dir) {
					printf ("\t%s (%sa + 2, %s);\n", dstw, to, from);
					printf ("\t%s (%sa, %s >> 16);\n", dstw, to, from);
				} else {
					printf ("\t%s (%sa, %s >> 16);\n", dstw, to, from);
					printf ("\t%s (%sa + 2, %s);\n", dstw, to, from);
				}
				count_write += 2;
				break;
			default:
				term ();
			}
		} else if (using_mmu) {
			switch (size) {
			case sz_byte:
				insn_n_cycles += 4;
				if (flags & GF_FC)
					printf ("\tdfc%s_put_byte (%sa, %s);\n", mmu_postfix, to, from);
				else
					printf ("\t%s (%sa, %s);\n", (flags & GF_LRMW) ? dstblrmw : (candormw ? dstbrmw : dstb), to, from);
				break;
			case sz_word:
				insn_n_cycles += 4;
				if (cpu_level < 2 && (mode == PC16 || mode == PC8r))
					term ();
				if (flags & GF_FC)
					printf ("\tdfc%s_put_word (%sa, %s);\n", mmu_postfix, to, from);
				else
					printf ("\t%s (%sa, %s);\n", (flags & GF_LRMW) ? dstwlrmw : (candormw ? dstwrmw : dstw), to, from);
				break;
			case sz_long:
				insn_n_cycles += 8;
				if (cpu_level < 2 && (mode == PC16 || mode == PC8r))
					term ();
				if (flags & GF_FC)
					printf ("\tdfc%s_put_long (%sa, %s);\n", mmu_postfix, to, from);
				else
					printf ("\t%s (%sa, %s);\n", (flags & GF_LRMW) ? dstllrmw : (candormw ? dstlrmw : dstl), to, from);
				break;
			default:
				term ();
			}
		} else if (using_prefetch) {
			switch (size) {
			case sz_byte:
				insn_n_cycles += 4;
				printf ("\t%s (%sa, %s);\n", dstb, to, from);
				count_write++;
				break;
			case sz_word:
				insn_n_cycles += 4;
				if (cpu_level < 2 && (mode == PC16 || mode == PC8r))
					term ();
				printf ("\t%s (%sa, %s);\n", dstw, to, from);
				count_write++;
				break;
			case sz_long:
				insn_n_cycles += 8;
				if (cpu_level < 2 && (mode == PC16 || mode == PC8r))
					term ();
				if (store_dir)
					printf ("\t%s (%sa + 2, %s); %s (%sa, %s >> 16);\n", dstw, to, from, dstw, to, from);
				else
					printf ("\t%s (%sa, %s >> 16); %s (%sa + 2, %s);\n", dstw, to, from, dstw, to, from);
				count_write += 2;
				break;
			default:
				term ();
			}
		} else {
			switch (size) {
			case sz_byte:
				insn_n_cycles += 4;
				printf ("\t%s (%sa, %s);\n", dstb, to, from);
				count_write++;
				break;
			case sz_word:
				insn_n_cycles += 4;
				if (cpu_level < 2 && (mode == PC16 || mode == PC8r))
					term ();
				printf ("\t%s (%sa, %s);\n", dstw, to, from);
				count_write++;
				break;
			case sz_long:
				insn_n_cycles += 8;
				if (cpu_level < 2 && (mode == PC16 || mode == PC8r))
					term ();
				printf ("\t%s (%sa, %s);\n", dstl, to, from);
				count_write += 2;
				break;
			default:
				term ();
			}
		}
		break;
	case imm:
	case imm0:
	case imm1:
	case imm2:
	case immi:
		term ();
		break;
	default:
		term ();
	}
}

static void genastore (char *from, amodes mode, char *reg, wordsizes size, char *to)
{
	genastore_2 (from, mode, reg, size, to, 0, 0);
}
static void genastore_tas (char *from, amodes mode, char *reg, wordsizes size, char *to)
{
	genastore_2 (from, mode, reg, size, to, 0, GF_LRMW);
}
static void genastore_cas (char *from, amodes mode, char *reg, wordsizes size, char *to)
{
	genastore_2 (from, mode, reg, size, to, 0, GF_LRMW | GF_NOFAULTPC);
}
static void genastore_rev (char *from, amodes mode, char *reg, wordsizes size, char *to)
{
	genastore_2 (from, mode, reg, size, to, 1, 0);
}
static void genastore_fc (char *from, amodes mode, char *reg, wordsizes size, char *to)
{
	genastore_2 (from, mode, reg, size, to, 1, GF_FC);
}

static void movem_mmu060 (const char *code, int size, bool put, bool aipi, bool apdi)
{
	char *index;
	int dphase, aphase;
	if (apdi) {
		dphase = 1; aphase = 0;
		index = "movem_index2";
	} else {
		dphase = 0; aphase = 1;
		index = "movem_index1";
	}

	if (!put) {
		printf("\tuae_u32 tmp[16];\n");
		printf("\tint tmpreg[16];\n");
		printf("\tint idx = 0;\n");
		for (int i = 0; i < 2; i++) {
			char reg;
			if (i == dphase)
				reg = 'd';
			else
				reg = 'a';
			printf ("\twhile (%cmask) {\n", reg);
			if (apdi)
				printf ("\t\tsrca -= %d;\n", size);
			printf ("\t\ttmpreg[idx] = %s[%cmask] + %d;\n", index, reg, i == dphase ? 0 : 8);
			printf ("\t\ttmp[idx++] = %s;\n", code);
			if (!apdi)
				printf ("\t\tsrca += %d;\n", size);
			printf ("\t\t%cmask = movem_next[%cmask];\n", reg, reg);
			printf ("\t}\n");
		}
		if (aipi || apdi)
			printf ("\tm68k_areg (regs, dstreg) = srca;\n");
		printf ("\twhile (--idx >= 0) {\n");
		printf ("\t\tregs.regs[tmpreg[idx]] = tmp[idx];\n");
		printf ("\t}\n");
	} else {
		for (int i = 0; i < 2; i++) {
			char reg;
			if (i == dphase)
				reg = 'd';
			else
				reg = 'a';
			printf ("\twhile (%cmask) {\n", reg);
			if (apdi)
				printf ("\t\tsrca -= %d;\n", size);
			if (put) {
				printf ("\t\t%s, m68k_%creg (regs, %s[%cmask]));\n", code, reg, index, reg);
			} else {
				printf ("\t\tm68k_%creg (regs, %s[%cmask]) = %s;\n", reg, index, reg, code);
			}
			if (!apdi)
				printf ("\t\tsrca += %d;\n", size);
			printf ("\t\t%cmask = movem_next[%cmask];\n", reg, reg);
			printf ("\t}\n");
		}
		if (aipi || apdi)
			printf ("\tm68k_areg (regs, dstreg) = srca;\n");
	}
}

static bool mmu040_special_movem (uae_u16 opcode)
{
	if (using_mmu != 68040)
		return false;
	return true;
//	return (((((opcode >> 3) & 7) == 7) && ((opcode & 7) == 2 || (opcode & 7) == 3)) || ((opcode >> 3) & 7) == 6);
}

static void movem_mmu040 (const char *code, int size, bool put, bool aipi, bool apdi, uae_u16 opcode)
{
	char *index;
	int dphase, aphase;
	bool mvm = false;

	if (apdi) {
		dphase = 1; aphase = 0;
		index = "movem_index2";
	} else {
		dphase = 0; aphase = 1;
		index = "movem_index1";
	}

	printf ("\tmmu040_movem = 1;\n");
	printf ("\tmmu040_movem_ea = srca;\n");
	mvm = true;

	for (int i = 0; i < 2; i++) {
		char reg;
		if (i == dphase)
			reg = 'd';
		else
			reg = 'a';
		printf ("\twhile (%cmask) {\n", reg);
		if (apdi)
			printf ("\t\tsrca -= %d;\n", size);
		if (put) {
			printf ("\t\t%s, m68k_%creg (regs, %s[%cmask]));\n", code, reg, index, reg);
		} else {
			printf ("\t\tm68k_%creg (regs, %s[%cmask]) = %s;\n", reg, index, reg, code);
		}
		if (!apdi)
			printf ("\t\tsrca += %d;\n", size);
		printf ("\t\t%cmask = movem_next[%cmask];\n", reg, reg);
		printf ("\t}\n");
	}
	if (aipi || apdi)
		printf ("\tm68k_areg (regs, dstreg) = srca;\n");
	printf ("\tmmu040_movem = 0;\n");
}

/* 68030 MMU does not restore register state if it bus faults.
 * (also there wouldn't be enough space in stack frame to store all registers)
 */
static void movem_mmu030 (const char *code, int size, bool put, bool aipi, bool apdi)
{
	char *index;
	int dphase, aphase;
	if (apdi) {
		dphase = 1; aphase = 0;
		index = "movem_index2";
	} else {
		dphase = 0; aphase = 1;
		index = "movem_index1";
	}
	printf ("\tmmu030_state[1] |= MMU030_STATEFLAG1_MOVEM1;\n");
	printf ("\tint movem_cnt = 0;\n");
	if (!put) {
		printf ("\tuae_u32 val;\n");
		printf ("\tif (mmu030_state[1] & MMU030_STATEFLAG1_MOVEM2)\n");
		printf ("\t\tsrca = mmu030_ad[mmu030_idx].val;\n");
		printf ("\telse\n");
		printf ("\t\tmmu030_ad[mmu030_idx].val = srca;\n");
	}
	for (int i = 0; i < 2; i++) {
		char reg;
		if (i == dphase)
			reg = 'd';
		else
			reg = 'a';
		printf ("\twhile (%cmask) {\n", reg);
		if (apdi)
			printf ("\t\tsrca -= %d;\n", size);
		printf ("\t\tif (mmu030_state[0] == movem_cnt) {\n");
		printf ("\t\t\tif (mmu030_state[1] & MMU030_STATEFLAG1_MOVEM2) {\n");
		printf ("\t\t\t\tmmu030_state[1] &= ~MMU030_STATEFLAG1_MOVEM2;\n");
		if (!put)
			printf ("\t\t\t\tval = %smmu030_data_buffer;\n", size == 2 ? "(uae_s32)(uae_s16)" : "");
		printf ("\t\t\t} else {\n");
		if (put)
			printf ("\t\t\t\t%s, (mmu030_data_buffer = m68k_%creg (regs, %s[%cmask])));\n", code, reg, index, reg);
		else
			printf ("\t\t\t\tval = %s;\n", code);
		printf ("\t\t\t}\n");
		if (!put) {
			printf ("\t\t\tm68k_%creg (regs, %s[%cmask]) = val;\n", reg, index, reg);
		}
		printf ("\t\t\tmmu030_state[0]++;\n");
		printf ("\t\t}\n");
		if (!apdi)
			printf ("\t\tsrca += %d;\n", size);
		printf ("\t\tmovem_cnt++;\n");
		printf ("\t\t%cmask = movem_next[%cmask];\n", reg, reg);
		printf ("\t}\n");
	}
	if (aipi || apdi)
		printf ("\tm68k_areg (regs, dstreg) = srca;\n");
}

static void genmovemel (uae_u16 opcode)
{
	char getcode[100];
	int size = table68k[opcode].size == sz_long ? 4 : 2;

	if (table68k[opcode].size == sz_long) {
		sprintf (getcode, "%s (srca)", srcld);
	} else {
		sprintf (getcode, "(uae_s32)(uae_s16)%s (srca)", srcwd);
	}
	count_read += table68k[opcode].size == sz_long ? 2 : 1;
	printf ("\tuae_u16 mask = %s;\n", gen_nextiword (0));
	printf ("\tuae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;\n");
	genamode (NULL, table68k[opcode].dmode, "dstreg", table68k[opcode].size, "src", 2, mmu040_special_movem (opcode) ? 3 : 1, 0);
	addcycles_ce020 (8 - 2);
	start_brace ();
	if (using_mmu == 68030) {
		movem_mmu030 (getcode, size, false, table68k[opcode].dmode == Aipi, false);
	} else if (using_mmu == 68060) {
		movem_mmu060 (getcode, size, false, table68k[opcode].dmode == Aipi, false);
	} else if (using_mmu == 68040) {
		movem_mmu040 (getcode, size, false, table68k[opcode].dmode == Aipi, false, opcode);
	} else {
		printf ("\twhile (dmask) {\n");
		printf ("\t\tm68k_dreg (regs, movem_index1[dmask]) = %s; srca += %d; dmask = movem_next[dmask];\n", getcode, size);
		//addcycles_ce020 (1);
		printf ("\t}\n");
		printf ("\twhile (amask) {\n");
		printf ("\t\tm68k_areg (regs, movem_index1[amask]) = %s; srca += %d; amask = movem_next[amask];\n", getcode, size);
		//addcycles_ce020 (1);
		printf ("\t}\n");
		if (table68k[opcode].dmode == Aipi) {
			printf ("\tm68k_areg (regs, dstreg) = srca;\n");
			count_read++;
		}
	}
	count_ncycles++;
	fill_prefetch_next ();
	get_prefetch_020 ();
}

static void genmovemel_ce (uae_u16 opcode)
{
	int size = table68k[opcode].size == sz_long ? 4 : 2;
	printf ("\tuae_u16 mask = %s;\n", gen_nextiword (0));
	printf ("\tuae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;\n");
	if (using_prefetch && table68k[opcode].size == sz_long)
		printf ("\tuae_u32 v;\n");
	genamode (NULL, table68k[opcode].dmode, "dstreg", table68k[opcode].size, "src", 2, 1, GF_AA);
	if (table68k[opcode].dmode == Ad8r || table68k[opcode].dmode == PC8r)
		addcycles000 (2);
	start_brace ();
	if (table68k[opcode].size == sz_long) {
		printf ("\twhile (dmask) {\n");
		if (using_prefetch) {
			printf ("\t\tv = %s (srca) << 16; v |= %s (srca + 2);\n", srcw, srcw);
  	  printf ("\t\tm68k_dreg (regs, movem_index1[dmask]) = v;\n");
		}	else
	    printf ("\t\tm68k_dreg (regs, movem_index1[dmask]) = %s (srca);\n", srcl);
		printf ("\t\tsrca += %d;\n", size);
		printf ("\t\tdmask = movem_next[dmask];\n");
		addcycles000_nonce("\t\t", cpu_level > 1 ? 4 : 8);
		printf ("\t}\n");
		printf ("\twhile (amask) {\n");
		if (using_prefetch) {
			printf ("\t\tv = %s (srca) << 16; v |= %s (srca + 2);\n", srcw, srcw);
		  printf ("\t\tm68k_areg (regs, movem_index1[amask]) = v;\n");
		} else 
	    printf ("\t\tm68k_areg (regs, movem_index1[amask]) = %s (srca);\n", srcl);
		printf ("\t\tsrca += %d;\n", size);
		printf ("\t\tamask = movem_next[amask];\n");
		addcycles000_nonce("\t\t", cpu_level > 1 ? 4 : 8);
		printf ("\t}\n");
	} else {
		printf ("\twhile (dmask) {\n");
		printf ("\t\tm68k_dreg (regs, movem_index1[dmask]) = (uae_s32)(uae_s16)%s (srca);\n", srcw);
		printf ("\t\tsrca += %d;\n", size);
		printf ("\t\tdmask = movem_next[dmask];\n");
		addcycles000_nonce("\t\t", cpu_level > 1 ? 4 : 4);
		printf ("\t}\n");
		printf ("\twhile (amask) {\n");
		printf ("\t\tm68k_areg (regs, movem_index1[amask]) = (uae_s32)(uae_s16)%s (srca);\n", srcw);
		printf ("\t\tsrca += %d;\n", size);
		printf ("\t\tamask = movem_next[amask];\n");
		addcycles000_nonce("\t\t", cpu_level > 1 ? 4 : 4);
		printf ("\t}\n");
	}
	//printf ("\t%s (srca);\n", srcw); // and final extra word fetch that goes nowhere..
	count_read++;
	if (table68k[opcode].dmode == Aipi)
		printf ("\tm68k_areg (regs, dstreg) = srca;\n");
	count_ncycles++;
	fill_prefetch_next ();
}

static void genmovemle (uae_u16 opcode)
{
	char putcode[100];
	int size = table68k[opcode].size == sz_long ? 4 : 2;

	if (table68k[opcode].size == sz_long) {
		sprintf (putcode, "%s (srca", dstld);
	} else {
		sprintf (putcode, "%s (srca", dstwd);
	}
	count_write += table68k[opcode].size == sz_long ? 2 : 1;

	printf ("\tuae_u16 mask = %s;\n", gen_nextiword (0));
	genamode (NULL, table68k[opcode].dmode, "dstreg", table68k[opcode].size, "src", 2, mmu040_special_movem (opcode) ? 3 : 1, 0);
	addcycles_ce020 (4 - 2);
	start_brace ();
	if (using_mmu >= 68030) {
		if (table68k[opcode].dmode == Apdi)
			printf ("\tuae_u16 amask = mask & 0xff, dmask = (mask >> 8) & 0xff;\n");
		else
			printf ("\tuae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;\n");
		if (using_mmu == 68030)
			movem_mmu030 (putcode, size, true, false, table68k[opcode].dmode == Apdi);
		else if (using_mmu == 68060)
			movem_mmu060 (putcode, size, true, false, table68k[opcode].dmode == Apdi);
		else if (using_mmu == 68040)
			movem_mmu040 (putcode, size, true, false, table68k[opcode].dmode == Apdi, opcode);
	} else {
		if (table68k[opcode].dmode == Apdi) {
			printf ("\tuae_u16 amask = mask & 0xff, dmask = (mask >> 8) & 0xff;\n");
			if (!using_mmu)
				printf ("\tint type = get_cpu_model () >= 68020;\n");
			printf ("\twhile (amask) {\n");
			printf ("\t\tsrca -= %d;\n", size);

			printf ("\t\tif (!type || movem_index2[amask] != dstreg)\n");
			printf ("\t\t\t%s, m68k_areg (regs, movem_index2[amask]));\n", putcode);
			printf ("\t\telse\n");
			printf ("\t\t\t%s, m68k_areg (regs, movem_index2[amask]) - %d);\n", putcode, size);

			printf ("\t\tamask = movem_next[amask];\n");
			printf ("\t}\n");
			printf ("\twhile (dmask) { srca -= %d; %s, m68k_dreg (regs, movem_index2[dmask])); dmask = movem_next[dmask]; }\n",
				size, putcode);
			printf ("\tm68k_areg (regs, dstreg) = srca;\n");
		} else {
			printf ("\tuae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;\n");
			printf ("\twhile (dmask) { %s, m68k_dreg (regs, movem_index1[dmask])); srca += %d; dmask = movem_next[dmask]; }\n",
				putcode, size);
			printf ("\twhile (amask) { %s, m68k_areg (regs, movem_index1[amask])); srca += %d; amask = movem_next[amask]; }\n",
				putcode, size);
		}
	}
	count_ncycles++;
	fill_prefetch_next ();
	get_prefetch_020 ();
}

static void genmovemle_ce (uae_u16 opcode)
{
	int size = table68k[opcode].size == sz_long ? 4 : 2;

	printf ("\tuae_u16 mask = %s;\n", gen_nextiword (0));
	genamode (NULL, table68k[opcode].dmode, "dstreg", table68k[opcode].size, "src", 2, 1, GF_AA);
	if (table68k[opcode].dmode == Ad8r || table68k[opcode].dmode == PC8r) {
		addcycles000 (2);
	}
	start_brace ();
	if (table68k[opcode].size == sz_long) {
		if (table68k[opcode].dmode == Apdi) {
			printf ("\tuae_u16 amask = mask & 0xff, dmask = (mask >> 8) & 0xff;\n");
      if(cpu_level >= 2)
        printf ("\tint type = get_cpu_model() >= 68020;\n");
			printf ("\twhile (amask) {\n");
			printf ("\t\tsrca -= %d;\n", size);
      if(cpu_level >= 2)
        printf ("\t\tif (type) m68k_areg (regs, dstreg) = srca;\n");
			if (using_prefetch) {
		    printf ("\t\t%s (srca, m68k_areg (regs, movem_index2[amask]) >> 16);\n", dstw);
		    printf ("\t\t%s (srca + 2, m68k_areg (regs, movem_index2[amask]));\n", dstw);
			} else
	      printf ("\t\t%s (srca, m68k_areg (regs, movem_index2[amask]));\n", dstl);
			printf ("\t\tamask = movem_next[amask];\n");
			addcycles000_nonce("\t\t", cpu_level > 1 ? 3 : 8);
			printf ("\t}\n");
			printf ("\twhile (dmask) {\n");
			printf ("\t\tsrca -= %d;\n", size);
			if (using_prefetch) {
		    printf ("\t\t%s (srca, m68k_dreg (regs, movem_index2[dmask]) >> 16);\n", dstw);
		    printf ("\t\t%s (srca + 2, m68k_dreg (regs, movem_index2[dmask]));\n", dstw);
			} else
	      printf ("\t\t%s (srca, m68k_dreg (regs, movem_index2[dmask]));\n", dstl);
			printf ("\t\tdmask = movem_next[dmask];\n");
			addcycles000_nonce("\t\t", cpu_level > 1 ? 3 : 8);
			printf ("\t}\n");
			printf ("\tm68k_areg (regs, dstreg) = srca;\n");
		} else {
			printf ("\tuae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;\n");
			printf ("\twhile (dmask) {\n");
			if (using_prefetch) {
		    printf ("\t\t%s (srca, m68k_dreg (regs, movem_index1[dmask]) >> 16);\n", dstw);
		    printf ("\t\t%s (srca + 2, m68k_dreg (regs, movem_index1[dmask]));\n", dstw);
			} else
	      printf ("\t\t%s (srca, m68k_dreg (regs, movem_index1[dmask]));\n", dstl);
			printf ("\t\tsrca += %d;\n", size);
			printf ("\t\tdmask = movem_next[dmask];\n");
			addcycles000_nonce("\t\t", cpu_level > 1 ? 3 : 8);
			printf ("\t}\n");
			printf ("\twhile (amask) {\n");
			if (using_prefetch) {
		    printf ("\t\t%s (srca, m68k_areg (regs, movem_index1[amask]) >> 16);\n", dstw);
		    printf ("\t\t%s (srca + 2, m68k_areg (regs, movem_index1[amask]));\n", dstw);
			} else
	      printf ("\t\t%s (srca, m68k_areg (regs, movem_index1[amask]));\n", dstl);
			printf ("\t\tsrca += %d;\n", size);
			printf ("\t\tamask = movem_next[amask];\n");
			addcycles000_nonce("\t\t", cpu_level > 1 ? 3 : 8);
			printf ("\t}\n");
		}
	} else {
		if (table68k[opcode].dmode == Apdi) {
			printf ("\tuae_u16 amask = mask & 0xff, dmask = (mask >> 8) & 0xff;\n");
      if(cpu_level >= 2)
  			printf ("\tint type = get_cpu_model() >= 68020;\n");
      printf ("\twhile (amask) {\n");
			printf ("\t\tsrca -= %d;\n", size);
      if(cpu_level >= 2)
  			printf ("\t\tif (type) m68k_areg (regs, dstreg) = srca;\n");
      printf ("\t\t%s (srca, m68k_areg (regs, movem_index2[amask]));\n", dstw);
			printf ("\tamask = movem_next[amask];\n");
			addcycles000_nonce("\t\t", cpu_level > 1 ? 3 : 4);
			printf ("\t}\n");
			printf ("\twhile (dmask) {\n");
			printf ("\t\tsrca -= %d;\n", size);
			printf ("\t\t%s (srca, m68k_dreg (regs, movem_index2[dmask]));\n", dstw);
			printf ("\t\tdmask = movem_next[dmask];\n");
			addcycles000_nonce("\t\t", cpu_level > 1 ? 3 : 4);
			printf ("\t}\n");
			printf ("\tm68k_areg (regs, dstreg) = srca;\n");
		} else {
			printf ("\tuae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;\n");
			printf ("\twhile (dmask) {\n");
			printf ("\t\t%s (srca, m68k_dreg (regs, movem_index1[dmask]));\n", dstw);
			printf ("\t\tsrca += %d;\n", size);
			printf ("\t\tdmask = movem_next[dmask];\n");
			addcycles000_nonce("\t\t", cpu_level > 1 ? 3 : 4);
			printf ("\t}\n");
			printf ("\twhile (amask) {\n");
			printf ("\t\t%s (srca, m68k_areg (regs, movem_index1[amask]));\n", dstw);
			printf ("\t\tsrca += %d;\n", size);
			printf ("\t\tamask = movem_next[amask];\n");
			addcycles000_nonce("\t\t", cpu_level > 1 ? 3 : 4);
			printf ("\t}\n");
		}
	}
	count_ncycles++;
	fill_prefetch_next ();
}

static void duplicate_carry (int n)
{
	int i;
	for (i = 0; i <= n; i++)
		printf ("\t");
	printf ("COPY_CARRY ();\n");
}

typedef enum
{
	flag_logical_noclobber, flag_logical, flag_add, flag_sub, flag_cmp, flag_addx, flag_subx, flag_z, flag_zn,
	flag_av, flag_sv
}
flagtypes;

static void genflags_normal (flagtypes type, wordsizes size, char *value, char *src, char *dst)
{
	char vstr[100], sstr[100], dstr[100];
	char usstr[100], udstr[100];
	char unsstr[100], undstr[100];

	switch (size) {
	case sz_byte:
		strcpy (vstr, "((uae_s8)(");
		strcpy (usstr, "((uae_u8)(");
		break;
	case sz_word:
		strcpy (vstr, "((uae_s16)(");
		strcpy (usstr, "((uae_u16)(");
		break;
	case sz_long:
		strcpy (vstr, "((uae_s32)(");
		strcpy (usstr, "((uae_u32)(");
		break;
	default:
		term ();
	}
	strcpy (unsstr, usstr);

	strcpy (sstr, vstr);
	strcpy (dstr, vstr);
	strcat (vstr, value);
	strcat (vstr, "))");
	strcat (dstr, dst);
	strcat (dstr, "))");
	strcat (sstr, src);
	strcat (sstr, "))");

	strcpy (udstr, usstr);
	strcat (udstr, dst);
	strcat (udstr, "))");
	strcat (usstr, src);
	strcat (usstr, "))");

	strcpy (undstr, unsstr);
	strcat (unsstr, "-");
	strcat (undstr, "~");
	strcat (undstr, dst);
	strcat (undstr, "))");
	strcat (unsstr, src);
	strcat (unsstr, "))");

	switch (type) {
	case flag_logical_noclobber:
	case flag_logical:
	case flag_z:
	case flag_zn:
	case flag_av:
	case flag_sv:
	case flag_addx:
	case flag_subx:
		break;

	case flag_add:
		start_brace ();
		printf ("uae_u32 %s = %s + %s;\n", value, dstr, sstr);
		break;
	case flag_sub:
	case flag_cmp:
		start_brace ();
		printf ("uae_u32 %s = %s - %s;\n", value, dstr, sstr);
		break;
	}

	switch (type) {
	case flag_logical_noclobber:
	case flag_logical:
	case flag_zn:
		break;

	case flag_add:
	case flag_sub:
	case flag_addx:
	case flag_subx:
	case flag_cmp:
	case flag_av:
	case flag_sv:
		start_brace ();
		printf ("\t" BOOL_TYPE " flgs = %s < 0;\n", sstr);
		printf ("\t" BOOL_TYPE " flgo = %s < 0;\n", dstr);
		printf ("\t" BOOL_TYPE " flgn = %s < 0;\n", vstr);
		break;
	}

	switch (type) {
	case flag_logical:
		printf ("\tCLEAR_CZNV ();\n");
		printf ("\tSET_ZFLG   (%s == 0);\n", vstr);
		printf ("\tSET_NFLG   (%s < 0);\n", vstr);
		break;
	case flag_logical_noclobber:
		printf ("\tSET_ZFLG (%s == 0);\n", vstr);
		printf ("\tSET_NFLG (%s < 0);\n", vstr);
		break;
	case flag_av:
		printf ("\tSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));\n");
		break;
	case flag_sv:
		printf ("\tSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));\n");
		break;
	case flag_z:
		printf ("\tSET_ZFLG (GET_ZFLG () & (%s == 0));\n", vstr);
		break;
	case flag_zn:
		printf ("\tSET_ZFLG (GET_ZFLG () & (%s == 0));\n", vstr);
		printf ("\tSET_NFLG (%s < 0);\n", vstr);
		break;
	case flag_add:
		printf ("\tSET_ZFLG (%s == 0);\n", vstr);
		printf ("\tSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));\n");
		printf ("\tSET_CFLG (%s < %s);\n", undstr, usstr);
		duplicate_carry (0);
		printf ("\tSET_NFLG (flgn != 0);\n");
		break;
	case flag_sub:
		printf ("\tSET_ZFLG (%s == 0);\n", vstr);
		printf ("\tSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));\n");
		printf ("\tSET_CFLG (%s > %s);\n", usstr, udstr);
		duplicate_carry (0);
		printf ("\tSET_NFLG (flgn != 0);\n");
		break;
	case flag_addx:
		printf ("\tSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));\n"); /* minterm SON: 0x42 */
		printf ("\tSET_CFLG (flgs ^ ((flgs ^ flgo) & (flgo ^ flgn)));\n"); /* minterm SON: 0xD4 */
		duplicate_carry (0);
		break;
	case flag_subx:
		printf ("\tSET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));\n"); /* minterm SON: 0x24 */
		printf ("\tSET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));\n"); /* minterm SON: 0xB2 */
		duplicate_carry (0);
		break;
	case flag_cmp:
		printf ("\tSET_ZFLG (%s == 0);\n", vstr);
		printf ("\tSET_VFLG ((flgs != flgo) && (flgn != flgo));\n");
		printf ("\tSET_CFLG (%s > %s);\n", usstr, udstr);
		printf ("\tSET_NFLG (flgn != 0);\n");
		break;
	}
}

static void genflags (flagtypes type, wordsizes size, char *value, char *src, char *dst)
{
	/* Temporarily deleted 68k/ARM flag optimizations.  I'd prefer to have
	them in the appropriate m68k.h files and use just one copy of this
	code here.  The API can be changed if necessary.  */
	if (optimized_flags) {
		switch (type) {
		case flag_add:
		case flag_sub:
			start_brace ();
			printf ("\tuae_u32 %s;\n", value);
			break;

		default:
			break;
		}

		/* At least some of those casts are fairly important! */
		switch (type) {
		case flag_logical_noclobber:
			printf ("\t{uae_u32 oldcznv = GET_CZNV & ~(FLAGVAL_Z | FLAGVAL_N);\n");
			if (strcmp (value, "0") == 0) {
				printf ("\tSET_CZNV (olcznv | FLAGVAL_Z);\n");
			} else {
				switch (size) {
				case sz_byte: printf ("\toptflag_testb (regs, (uae_s8)(%s));\n", value); break;
				case sz_word: printf ("\toptflag_testw (regs, (uae_s16)(%s));\n", value); break;
				case sz_long: printf ("\toptflag_testl (regs, (uae_s32)(%s));\n", value); break;
				}
				printf ("\tIOR_CZNV (oldcznv);\n");
			}
			printf ("\t}\n");
			return;
		case flag_logical:
			if (strcmp (value, "0") == 0) {
				printf ("\tSET_CZNV (FLAGVAL_Z);\n");
			} else {
				switch (size) {
				case sz_byte: printf ("\toptflag_testb (regs, (uae_s8)(%s));\n", value); break;
				case sz_word: printf ("\toptflag_testw (regs, (uae_s16)(%s));\n", value); break;
				case sz_long: printf ("\toptflag_testl (regs, (uae_s32)(%s));\n", value); break;
				}
			}
			return;

		case flag_add:
			switch (size) {
			case sz_byte: printf ("\toptflag_addb (regs, %s, (uae_s8)(%s), (uae_s8)(%s));\n", value, src, dst); break;
			case sz_word: printf ("\toptflag_addw (regs, %s, (uae_s16)(%s), (uae_s16)(%s));\n", value, src, dst); break;
			case sz_long: printf ("\toptflag_addl (regs, %s, (uae_s32)(%s), (uae_s32)(%s));\n", value, src, dst); break;
			}
			return;

		case flag_sub:
			switch (size) {
			case sz_byte: printf ("\toptflag_subb (regs, %s, (uae_s8)(%s), (uae_s8)(%s));\n", value, src, dst); break;
			case sz_word: printf ("\toptflag_subw (regs, %s, (uae_s16)(%s), (uae_s16)(%s));\n", value, src, dst); break;
			case sz_long: printf ("\toptflag_subl (regs, %s, (uae_s32)(%s), (uae_s32)(%s));\n", value, src, dst); break;
			}
			return;

		case flag_cmp:
			switch (size) {
			case sz_byte: printf ("\toptflag_cmpb (regs, (uae_s8)(%s), (uae_s8)(%s));\n", src, dst); break;
			case sz_word: printf ("\toptflag_cmpw (regs, (uae_s16)(%s), (uae_s16)(%s));\n", src, dst); break;
			case sz_long: printf ("\toptflag_cmpl (regs, (uae_s32)(%s), (uae_s32)(%s));\n", src, dst); break;
			}
			return;

		default:
			break;
		}
	}

	genflags_normal (type, size, value, src, dst);
}

static void force_range_for_rox (const char *var, wordsizes size)
{
	/* Could do a modulo operation here... which one is faster? */
	switch (size) {
	case sz_long:
		printf ("\tif (%s >= 33) %s -= 33;\n", var, var);
		break;
	case sz_word:
		printf ("\tif (%s >= 34) %s -= 34;\n", var, var);
		printf ("\tif (%s >= 17) %s -= 17;\n", var, var);
		break;
	case sz_byte:
		printf ("\tif (%s >= 36) %s -= 36;\n", var, var);
		printf ("\tif (%s >= 18) %s -= 18;\n", var, var);
		printf ("\tif (%s >= 9) %s -= 9;\n", var, var);
		break;
	}
}

static const char *cmask (wordsizes size)
{
	switch (size) {
	case sz_byte: return "0x80";
	case sz_word: return "0x8000";
	case sz_long: return "0x80000000";
	default: term ();
	}
}

static int source_is_imm1_8 (struct instr *i)
{
	return i->stype == 3;
}

static void shift_ce (amodes dmode, int size)
{
	if (isreg (dmode)) {
		int c = size == sz_long ? 4 : 2;
		if (using_ce) {
			printf ("\t{\n");
			printf ("\t\tint cycles = %d;\n", c);
			printf ("\t\tcycles += 2 * cnt;\n");
			addcycles000_3 ("\t\t");
			printf ("\t}\n");
		}
		addcycles000_nonces("\t", "2 * cnt");
		count_cycles += c;
		count_ncycles++;
	}
}

// BCHG/BSET/BCLR Dx,Dx or #xx,Dx adds 2 cycles if bit number > 15 
static void bsetcycles (struct instr *curi)
{
	if (curi->size == sz_byte) {
		printf ("\tsrc &= 7;\n");
	} else {
		printf ("\tsrc &= 31;\n");
		if (isreg (curi->dmode)) {
			addcycles000 (2);
			if (curi->mnemo != i_BTST) {
				if (using_ce)
					printf ("\tif (src > 15) %s (2);\n", do_cycles);
				addcycles000_nonce("\tif (src > 15) ", 2);
				count_ncycles++;
			}
		}
	}
}

static int islongimm (struct instr *curi)
{
	return (curi->size == sz_long && (curi->smode == Dreg || curi->smode == imm || curi->smode == Areg));
}


static void resetvars (void)
{
	insn_n_cycles = using_prefetch ? 0 : 4;
	insn_n_cycles020 = 0;
	ir2irc = 0;
	mmufixupcnt = 0;
	mmufixupstate = 0;
	mmudisp020cnt = 0;
	candormw = false;
	genastore_done = false;
	rmw_varname[0] = 0;
	tail_ce020 = 0;
	total_ce020 = 0;
	tail_ce020_done = false;
	head_in_ea_ce020 = 0;
	head_ce020_cycs_done = false;
	no_prefetch_ce020 = false;
	got_ea_ce020 = false;
	
	prefetch_long = NULL;
	srcli = NULL;
	srcbi = NULL;
	disp000 = "get_disp_ea_000";
	disp020 = "get_disp_ea_020";
	nextw = NULL;
	nextl = NULL;
	do_cycles = "do_cycles";
	srcwd = srcld = NULL;
	dstwd = dstld = NULL;
	srcblrmw = NULL;
	srcwlrmw = NULL;
	srcllrmw = NULL;
	dstblrmw = NULL;
	dstwlrmw = NULL;
	dstllrmw = NULL;
	getpc = "m68k_getpc ()";

	if (using_indirect) {
		// tracer
		getpc = "m68k_getpci ()";
		if (!using_ce020 && !using_prefetch_020) {
			prefetch_word = "get_word_ce000_prefetch";
			srcli = "x_get_ilong";
			srcwi = "x_get_iword";
			srcbi = "x_get_ibyte";
			srcl = "x_get_long";
			dstl = "x_put_long";
			srcw = "x_get_word";
			dstw = "x_put_word";
			srcb = "x_get_byte";
			dstb = "x_put_byte";
			do_cycles = "do_cycles_ce000";
		} else if (using_ce020 == 1) {
			/* x_ not used if it redirects to
			 * get_word_ce020_prefetch()
			 */
			disp020 = "x_get_disp_ea_ce020";
			prefetch_word = "get_word_ce020_prefetch";
			prefetch_long = "get_long_ce020_prefetch";
			srcli = "x_get_ilong";
			srcwi = "x_get_iword";
			srcbi = "x_get_ibyte";
			srcl = "x_get_long";
			dstl = "x_put_long";
			srcw = "x_get_word";
			dstw = "x_put_word";
			srcb = "x_get_byte";
			dstb = "x_put_byte";
			do_cycles = "do_cycles_ce020";
			nextw = "next_iword_020ce";
			nextl = "next_ilong_020ce";
		} else if (using_ce020 == 2) {
			// 68030 CE
			disp020 = "x_get_disp_ea_ce030";
			prefetch_long = "get_long_ce030_prefetch";
			prefetch_word = "get_word_ce030_prefetch";
			srcli = "x_get_ilong";
			srcwi = "x_get_iword";
			srcbi = "x_get_ibyte";
			srcl = "x_get_long";
			dstl = "x_put_long";
			srcw = "x_get_word";
			dstw = "x_put_word";
			srcb = "x_get_byte";
			dstb = "x_put_byte";
			do_cycles = "do_cycles_ce020";
			nextw = "next_iword_030ce";
			nextl = "next_ilong_030ce";
		} else if (using_prefetch_020) {
			disp020 = "x_get_disp_ea_020";
			prefetch_word = "get_word_020_prefetch";
			prefetch_long = "get_long_020_prefetch";
			srcli = "x_get_ilong";
			srcwi = "x_get_iword";
			srcbi = "x_get_ibyte";
			srcl = "x_get_long";
			dstl = "x_put_long";
			srcw = "x_get_word";
			dstw = "x_put_word";
			srcb = "x_get_byte";
			dstb = "x_put_byte";
			nextw = "next_iword_020_prefetch";
			nextl = "next_ilong_020_prefetch";
		}
#if 0
	} else if (using_ce020) {
		disp020 = "x_get_disp_ea_020";
		do_cycles = "do_cycles_ce020";
		if (using_ce020 == 2) {
			// 68030/40/60 CE
			prefetch_long = "get_long_ce030_prefetch";
			prefetch_word = "get_word_ce030_prefetch";
			nextw = "next_iword_030ce";
			nextl = "next_ilong_030ce";
			srcli = "get_word_ce030_prefetch";
			srcwi = "get_long_ce030_prefetch";
			srcl = "get_long_ce030";
			dstl = "put_long_ce030";
			srcw = "get_word_ce030";
			dstw = "put_word_ce030";
			srcb = "get_byte_ce030";
			dstb = "put_byte_ce030";
		} else {
			// 68020 CE
			prefetch_long = "get_long_ce020_prefetch";
			prefetch_word = "get_word_ce020_prefetch";
			nextw = "next_iword_020ce";
			nextl = "next_ilong_020ce";
			srcli = "get_word_ce020_prefetch";
			srcwi = "get_long_ce020_prefetch";
			srcl = "get_long_ce020";
			dstl = "put_long_ce020";
			srcw = "get_word_ce020";
			dstw = "put_word_ce020";
			srcb = "get_byte_ce020";
			dstb = "put_byte_ce020";
		}
#endif
	} else if (using_mmu == 68030) {
		// 68030 MMU
		disp020 = "get_disp_ea_020_mmu030";
		prefetch_long = "get_ilong_mmu030_state";
		prefetch_word = "get_iword_mmu030_state";
		nextw = "next_iword_mmu030_state";
		nextl = "next_ilong_mmu030_state";
		srcli = "get_ilong_mmu030_state";
		srcwi = "get_iword_mmu030_state";
		srcbi = "get_ibyte_mmu030_state";
		srcl = "get_long_mmu030_state";
		dstl = "put_long_mmu030_state";
		srcw = "get_word_mmu030_state";
		dstw = "put_word_mmu030_state";
		srcb = "get_byte_mmu030_state";
		dstb = "put_byte_mmu030_state";
		srcblrmw = "get_lrmw_byte_mmu030_state";
		srcwlrmw = "get_lrmw_word_mmu030_state";
		srcllrmw = "get_lrmw_long_mmu030_state";
		dstblrmw = "put_lrmw_byte_mmu030_state";
		dstwlrmw = "put_lrmw_word_mmu030_state";
		dstllrmw = "put_lrmw_long_mmu030_state";
		srcld = "get_long_mmu030";
		srcwd = "get_word_mmu030";
		dstld = "put_long_mmu030";
		dstwd = "put_word_mmu030";
		getpc = "m68k_getpci ()";
	} else if (using_mmu == 68040) {
		// 68040 MMU
		disp020 = "x_get_disp_ea_020";
		prefetch_long = "get_ilong_mmu040";
		prefetch_word = "get_iword_mmu040";
		nextw = "next_iword_mmu040";
		nextl = "next_ilong_mmu040";
		srcli = "get_ilong_mmu040";
		srcwi = "get_iword_mmu040";
		srcbi = "get_ibyte_mmu040";
		srcl = "get_long_mmu040";
		dstl = "put_long_mmu040";
		srcw = "get_word_mmu040";
		dstw = "put_word_mmu040";
		srcb = "get_byte_mmu040";
		dstb = "put_byte_mmu040";
		srcblrmw = "get_lrmw_byte_mmu040";
		srcwlrmw = "get_lrmw_word_mmu040";
		srcllrmw = "get_lrmw_long_mmu040";
		dstblrmw = "put_lrmw_byte_mmu040";
		dstwlrmw = "put_lrmw_word_mmu040";
		dstllrmw = "put_lrmw_long_mmu040";
		getpc = "m68k_getpci ()";
	} else if (using_mmu) {
		// 68060 MMU
		disp020 = "x_get_disp_ea_020";
		prefetch_long = "get_ilong_mmu060";
		prefetch_word = "get_iword_mmu060";
		nextw = "next_iword_mmu060";
		nextl = "next_ilong_mmu060";
		srcli = "get_ilong_mmu060";
		srcwi = "get_iword_mmu060";
		srcbi = "get_ibyte_mmu060";
		srcl = "get_long_mmu060";
		dstl = "put_long_mmu060";
		srcw = "get_word_mmu060";
		dstw = "put_word_mmu060";
		srcb = "get_byte_mmu060";
		dstb = "put_byte_mmu060";
		srcblrmw = "get_lrmw_byte_mmu060";
		srcwlrmw = "get_lrmw_word_mmu060";
		srcllrmw = "get_lrmw_long_mmu060";
		dstblrmw = "put_lrmw_byte_mmu060";
		dstwlrmw = "put_lrmw_word_mmu060";
		dstllrmw = "put_lrmw_long_mmu060";
		// 68060 only: also non-locked read-modify-write accesses are reported
		srcbrmw = "get_rmw_byte_mmu060";
		srcwrmw = "get_rmw_word_mmu060";
		srclrmw = "get_rmw_long_mmu060";
		dstbrmw = "put_rmw_byte_mmu060";
		dstwrmw = "put_rmw_word_mmu060";
		dstlrmw = "put_rmw_long_mmu060";
		getpc = "m68k_getpci ()";
	} else if (using_ce) {
		// 68000 ce
		prefetch_word = "get_word_ce000_prefetch";
		srcwi = "get_wordi_ce000";
		srcl = "get_long_ce000";
		dstl = "put_long_ce000";
		srcw = "get_word_ce000";
		dstw = "put_word_ce000";
		srcb = "get_byte_ce000";
		dstb = "put_byte_ce000";
		do_cycles = "do_cycles_ce000";
		getpc = "m68k_getpci ()";
	} else if (using_prefetch) {
		// 68000 prefetch
		prefetch_word = "get_word_prefetch";
		prefetch_long = "get_long_prefetch";
		srcwi = "get_wordi_prefetch";
		srcl = "get_long_prefetch";
		dstl = "put_long_prefetch";
		srcw = "get_word_prefetch";
		dstw = "put_word_prefetch";
		srcb = "get_byte_prefetch";
		dstb = "put_byte_prefetch";
		getpc = "m68k_getpci ()";
	} else {
		// generic
		prefetch_long = "get_dilong";
		prefetch_word = "get_diword";
		nextw = "next_diword";
		nextl = "next_dilong";
		srcli = "get_dilong";
		srcwi = "get_diword";
		srcbi = "get_dibyte";
		srcl = "get_long";
		dstl = "put_long";
		srcw = "get_word";
		dstw = "put_word";
		srcb = "get_byte";
		dstb = "put_byte";
	}
	if (!dstld)
		dstld = dstl;
	if (!dstwd)
		dstwd = dstw;
	if (!srcld)
		srcld = srcl;
	if (!srcwd)
		srcwd = srcw;
	if (!srcblrmw) {
		srcblrmw = srcb;
		srcwlrmw = srcw;
		srcllrmw = srcl;
		dstblrmw = dstb;
		dstwlrmw = dstw;
		dstllrmw = dstl;
	}

}

static void gen_opcode (unsigned long int opcode)
{
	struct instr *curi = table68k + opcode;

	resetvars ();
	
  if(curi->mnemo == i_DIVL)
     printf ("\tuae_u32 cyc = 0;\n");

	start_brace ();

	m68k_pc_offset = 2;
	switch (curi->plev) {
	case 0: /* not privileged */
		break;
	case 1: /* unprivileged only on 68000 */
		if (cpu_level == 0)
			break;
		if (next_cpu_level < 0)
			next_cpu_level = 0;

		/* fall through */
	case 2: /* priviledged */
		printf ("if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }\n");
		start_brace ();
		break;
	case 3: /* privileged if size == word */
		if (curi->size == sz_byte)
			break;
		printf ("if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }\n");
		start_brace ();
		break;
	}
	switch (curi->mnemo) {
	case i_OR:
	case i_AND:
	case i_EOR:
	{
		// documentaion error: and.l #imm,dn = 2 idle, not 1 idle (same as OR and EOR)
		int c = 0;
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "src", 1, 0,
			curi->dmode, "dstreg", curi->size, "dst", 1, GF_RMW);
//		genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, 0);
//		genamode (curi, curi->dmode, "dstreg", curi->size, "dst", 1, 0, GF_RMW);
		printf ("\tsrc %c= dst;\n", curi->mnemo == i_OR ? '|' : curi->mnemo == i_AND ? '&' : '^');
		genflags (flag_logical, curi->size, "src", "", "");
		if (curi->dmode == Dreg && curi->size == sz_long) {
			c += 2;
			if (curi->smode == imm || curi->smode == Dreg)
				c += 2;
		}
		fill_prefetch_next ();
		if (c > 0)
			addcycles000 (c);
		genastore_rev ("src", curi->dmode, "dstreg", curi->size, "dst");
		break;
	}
	// all SR/CCR modifications does full prefetch
	case i_ORSR:
	case i_EORSR:
		printf ("\tMakeSR ();\n");
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, 0);
		if (curi->size == sz_byte) {
			printf ("\tsrc &= 0xFF;\n");
		}
		addcycles000 (8);
		if (cpu_level <= 1) {
			sync_m68k_pc ();
			fill_prefetch_full ();
		} else {
			fill_prefetch_next ();
		}
		printf ("\tregs.sr %c= src;\n", curi->mnemo == i_EORSR ? '^' : '|');
		makefromsr ();
		break;
	case i_ANDSR:
		printf ("\tMakeSR ();\n");
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, 0);
		if (curi->size == sz_byte) {
			printf ("\tsrc |= 0xFF00;\n");
		}
		addcycles000 (8);
		if (cpu_level <= 1) {
			sync_m68k_pc ();
			fill_prefetch_full ();
		} else {
			fill_prefetch_next ();
		}
		printf ("\tregs.sr &= src;\n");
		makefromsr ();
		break;
	case i_SUB:
	{
		int c = 0;
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "src", 1, 0,
			curi->dmode, "dstreg", curi->size, "dst", 1, GF_RMW);
		//genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, 0);
		//genamode (curi, curi->dmode, "dstreg", curi->size, "dst", 1, 0, GF_RMW);
		if (curi->dmode == Dreg) {
			if (curi->size == sz_long) {
				c += 2;
				if (curi->smode == imm || curi->smode == immi || curi->smode == Dreg || curi->smode == Areg)
					c += 2;
			}
		}
		fill_prefetch_next ();
		if (c > 0)
			addcycles000 (c);
		start_brace ();
		genflags (flag_sub, curi->size, "newv", "src", "dst");
		genastore_rev ("newv", curi->dmode, "dstreg", curi->size, "dst");
		break;
	}
	case i_SUBA:
	{
		int c = 0;
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "src", 1, 0,
			curi->dmode, "dstreg", sz_long, "dst", 1, GF_RMW);
		//genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, 0);
		//genamode (curi, curi->dmode, "dstreg", sz_long, "dst", 1, 0, GF_RMW);
		if (curi->smode == immi) {
			// SUBAQ.x is always 8 cycles
			c += 4;
		} else {
			c = curi->size == sz_long ? 2 : 4;
			if (islongimm (curi))
				c += 2;
		}
		fill_prefetch_next ();
		if (c > 0)
			addcycles000 (c);
		start_brace ();
		printf ("\tuae_u32 newv = dst - src;\n");
		genastore ("newv", curi->dmode, "dstreg", sz_long, "dst");
		break;
	}
	case i_SUBX:
		if (!isreg (curi->smode))
			addcycles000 (2);
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, GF_AA);
		genamode (curi, curi->dmode, "dstreg", curi->size, "dst", 1, 0, GF_AA | GF_RMW);
		fill_prefetch_next ();
		if (curi->size == sz_long && isreg (curi->smode))
			addcycles000 (4);
		start_brace ();
		printf ("\tuae_u32 newv = dst - src - (GET_XFLG () ? 1 : 0);\n");
		genflags (flag_subx, curi->size, "newv", "src", "dst");
		genflags (flag_zn, curi->size, "newv", "", "");
		genastore ("newv", curi->dmode, "dstreg", curi->size, "dst");
		break;
	case i_SBCD:
		if (!isreg (curi->smode))
			addcycles000 (2);
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, GF_AA);
		genamode (curi, curi->dmode, "dstreg", curi->size, "dst", 1, 0, GF_AA | GF_RMW);
		fill_prefetch_next ();
		start_brace ();
		printf ("\tuae_u16 newv_lo = (dst & 0xF) - (src & 0xF) - (GET_XFLG () ? 1 : 0);\n");
		printf ("\tuae_u16 newv_hi = (dst & 0xF0) - (src & 0xF0);\n");
		printf ("\tuae_u16 newv, tmp_newv;\n");
		printf ("\tint bcd = 0;\n");
		printf ("\tnewv = tmp_newv = newv_hi + newv_lo;\n");
		printf ("\tif (newv_lo & 0xF0) { newv -= 6; bcd = 6; };\n");
		printf ("\tif ((((dst & 0xFF) - (src & 0xFF) - (GET_XFLG () ? 1 : 0)) & 0x100) > 0xFF) { newv -= 0x60; }\n");
		printf ("\tSET_CFLG ((((dst & 0xFF) - (src & 0xFF) - bcd - (GET_XFLG () ? 1 : 0)) & 0x300) > 0xFF);\n");
		duplicate_carry (0);
		/* Manual says bits NV are undefined though a real 68040/060 don't change them */
		if (cpu_level >= xBCD_KEEPS_NV_FLAGS) {
			if (next_cpu_level < xBCD_KEEPS_NV_FLAGS)
				next_cpu_level = xBCD_KEEPS_NV_FLAGS - 1;
			genflags (flag_z, curi->size, "newv", "", "");
		} else {
			genflags (flag_zn, curi->size, "newv", "", "");
			printf ("\tSET_VFLG ((tmp_newv & 0x80) != 0 && (newv & 0x80) == 0);\n");
		}
		if (isreg (curi->smode)) {
			addcycles000 (2);
		}
		genastore ("newv", curi->dmode, "dstreg", curi->size, "dst");
		break;
	case i_ADD:
	{
		int c = 0;
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "src", 1, 0,
			curi->dmode, "dstreg", curi->size, "dst", 1, GF_RMW);
		//genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, 0);
		//genamode (curi, curi->dmode, "dstreg", curi->size, "dst", 1, 0, GF_RMW);
		if (curi->dmode == Dreg) {
			if (curi->size == sz_long) {
				c += 2;
				if (curi->smode == imm || curi->smode == immi || curi->smode == Dreg || curi->smode == Areg)
					c += 2;
			}
		}
		fill_prefetch_next ();
		if (c > 0)
			addcycles000 (c);
		start_brace ();
		genflags (flag_add, curi->size, "newv", "src", "dst");
		genastore_rev ("newv", curi->dmode, "dstreg", curi->size, "dst");
		break;
	}
	case i_ADDA:
	{
		int c = 0;
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "src", 1, 0,
			curi->dmode, "dstreg", sz_long, "dst", 1, GF_RMW);
		//genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, 0);
		//genamode (curi, curi->dmode, "dstreg", sz_long, "dst", 1, 0, GF_RMW);
		if (curi->smode == immi) {
			// ADDAQ.x is always 8 cycles
			c += 4;
		} else {
			c = curi->size == sz_long ? 2 : 4;
			if (islongimm (curi))
				c += 2;
		}
		fill_prefetch_next ();
		if (c > 0)
			addcycles000 (c);
		start_brace ();
		printf ("\tuae_u32 newv = dst + src;\n");
		genastore ("newv", curi->dmode, "dstreg", sz_long, "dst");
		break;
	}
	case i_ADDX:
		if (!isreg (curi->smode))
			addcycles000 (2);
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, GF_AA);
		genamode (curi, curi->dmode, "dstreg", curi->size, "dst", 1, 0, GF_AA | GF_RMW);
		fill_prefetch_next ();
		if (curi->size == sz_long && isreg (curi->smode))
			addcycles000 (4);
		start_brace ();
		printf ("\tuae_u32 newv = dst + src + (GET_XFLG () ? 1 : 0);\n");
		genflags (flag_addx, curi->size, "newv", "src", "dst");
		genflags (flag_zn, curi->size, "newv", "", "");
		genastore ("newv", curi->dmode, "dstreg", curi->size, "dst");
		break;
	case i_ABCD:
		if (!isreg (curi->smode))
			addcycles000 (2);
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, GF_AA);
		genamode (curi, curi->dmode, "dstreg", curi->size, "dst", 1, 0, GF_AA | GF_RMW);
		fill_prefetch_next ();
		start_brace ();
		printf ("\tuae_u16 newv_lo = (src & 0xF) + (dst & 0xF) + (GET_XFLG () ? 1 : 0);\n");
		printf ("\tuae_u16 newv_hi = (src & 0xF0) + (dst & 0xF0);\n");
		printf ("\tuae_u16 newv, tmp_newv;\n");
		printf ("\tint cflg;\n");
		printf ("\tnewv = tmp_newv = newv_hi + newv_lo;\n");
		printf ("\tif (newv_lo > 9) { newv += 6; }\n");
		printf ("\tcflg = (newv & 0x3F0) > 0x90;\n");
		printf ("\tif (cflg) newv += 0x60;\n");
		printf ("\tSET_CFLG (cflg);\n");
		duplicate_carry (0);
		/* Manual says bits NV are undefined though a real 68040 don't change them */
		if (cpu_level >= xBCD_KEEPS_NV_FLAGS) {
			if (next_cpu_level < xBCD_KEEPS_NV_FLAGS)
				next_cpu_level = xBCD_KEEPS_NV_FLAGS - 1;
			genflags (flag_z, curi->size, "newv", "", "");
		}
		else {
			genflags (flag_zn, curi->size, "newv", "", "");
			printf ("\tSET_VFLG ((tmp_newv & 0x80) == 0 && (newv & 0x80) != 0);\n");
		}
		if (isreg (curi->smode)) {
			addcycles000 (2);
		}
		genastore ("newv", curi->dmode, "dstreg", curi->size, "dst");
		break;
	case i_NEG:
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, GF_RMW);
		fill_prefetch_next ();
		if (isreg (curi->smode) && curi->size == sz_long)
			addcycles000 (2);
		start_brace ();
		genflags (flag_sub, curi->size, "dst", "src", "0");
		genastore_rev ("dst", curi->smode, "srcreg", curi->size, "src");
		break;
	case i_NEGX:
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, GF_RMW);
		fill_prefetch_next ();
		if (isreg (curi->smode) && curi->size == sz_long)
			addcycles000 (2);
		start_brace ();
		printf ("\tuae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);\n");
		genflags (flag_subx, curi->size, "newv", "src", "0");
		genflags (flag_zn, curi->size, "newv", "", "");
		genastore_rev ("newv", curi->smode, "srcreg", curi->size, "src");
		break;
	case i_NBCD:
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, GF_RMW);
		if (isreg (curi->smode))
			addcycles000 (2);
		fill_prefetch_next ();
		start_brace ();
		printf ("\tuae_u16 newv_lo = - (src & 0xF) - (GET_XFLG () ? 1 : 0);\n");
		printf ("\tuae_u16 newv_hi = - (src & 0xF0);\n");
		printf ("\tuae_u16 newv;\n");
		printf ("\tint cflg, tmp_newv;\n");
		printf ("\tif (newv_lo > 9) { newv_lo -= 6; }\n");
		printf ("\ttmp_newv = newv = newv_hi + newv_lo;\n");
		printf ("\tcflg = (newv & 0x1F0) > 0x90;\n");
		printf ("\tif (cflg) newv -= 0x60;\n");
		printf ("\tSET_CFLG (cflg);\n");
		duplicate_carry(0);
		/* Manual says bits NV are undefined though a real 68040 don't change them */
		if (cpu_level >= xBCD_KEEPS_NV_FLAGS) {
			if (next_cpu_level < xBCD_KEEPS_NV_FLAGS)
				next_cpu_level = xBCD_KEEPS_NV_FLAGS - 1;
			genflags (flag_z, curi->size, "newv", "", "");
		}
		else {
			genflags (flag_zn, curi->size, "newv", "", "");
			printf ("\tSET_VFLG ((tmp_newv & 0x80) != 0 && (newv & 0x80) == 0);\n");
		}
		genastore ("newv", curi->smode, "srcreg", curi->size, "src");
		break;
	case i_CLR:
		next_level_000 ();
		genamode (curi, curi->smode, "srcreg", curi->size, "src", cpu_level == 0 ? 1 : 2, 0, 0);
		fill_prefetch_next ();
    if(!using_prefetch && curi->smode != Dreg) {
      if(curi->size != sz_long)
        addcycles000(4);
      else
        addcycles000(8);
    }
		if (isreg (curi->smode) && curi->size == sz_long)
			addcycles000 (2);
		genflags (flag_logical, curi->size, "0", "", "");
		genastore_rev ("0", curi->smode, "srcreg", curi->size, "src");
		break;
	case i_NOT:
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, GF_RMW);
		fill_prefetch_next ();
		if (isreg (curi->smode) && curi->size == sz_long)
			addcycles000 (2);
		start_brace ();
		printf ("\tuae_u32 dst = ~src;\n");
		genflags (flag_logical, curi->size, "dst", "", "");
		genastore_rev ("dst", curi->smode, "srcreg", curi->size, "src");
		break;
	case i_TST:
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, 0);
		fill_prefetch_next ();
		genflags (flag_logical, curi->size, "src", "", "");
		break;
	case i_BTST:
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "src", 1, 0,
			curi->dmode, "dstreg", curi->size, "dst", 1, 0);
		//genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, 0);
		//genamode (curi, curi->dmode, "dstreg", curi->size, "dst", 1, 0, GF_IR2IRC);
		fill_prefetch_next ();
		bsetcycles (curi);
		printf ("\tSET_ZFLG (1 ^ ((dst >> src) & 1));\n");
		break;
	case i_BCHG:
	case i_BCLR:
	case i_BSET:
		// on 68000 these have weird side-effect, if EA points to write-only custom register
		//during instruction's read access CPU data lines appear as zero to outside world,
		// (normally previously fetched data appears in data lines if reading write-only register)
		// this allows stupid things like bset #2,$dff096 to work "correctly"
		// NOTE: above can't be right.
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "src", 1, 0,
			curi->dmode, "dstreg", curi->size, "dst", 1, GF_RMW);
		//genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, 0);
		//genamode (curi, curi->dmode, "dstreg", curi->size, "dst", 1, 0, GF_IR2IRC | GF_RMW);
		fill_prefetch_next ();
		bsetcycles (curi);
		// bclr needs 1 extra cycle
		if (curi->mnemo == i_BCLR && curi->dmode == Dreg)
			addcycles000 (2);
		if (curi->mnemo == i_BCHG) {
			printf ("\tdst ^= (1 << src);\n");
			printf ("\tSET_ZFLG (((uae_u32)dst & (1 << src)) >> src);\n");
		} else if (curi->mnemo == i_BCLR) {
			printf ("\tSET_ZFLG (1 ^ ((dst >> src) & 1));\n");
			printf ("\tdst &= ~(1 << src);\n");
		} else if (curi->mnemo == i_BSET) {
			printf ("\tSET_ZFLG (1 ^ ((dst >> src) & 1));\n");
			printf ("\tdst |= (1 << src);\n");
		}
		genastore ("dst", curi->dmode, "dstreg", curi->size, "dst");
		break;
	case i_CMPM:
		// confirmed
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "src", 1, GF_AA,
			curi->dmode, "dstreg", curi->size, "dst", 1, GF_AA);
		//genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, GF_AA);
		//genamode (curi, curi->dmode, "dstreg", curi->size, "dst", 1, 0, GF_AA);
		fill_prefetch_next ();
		start_brace ();
		genflags (flag_cmp, curi->size, "newv", "src", "dst");
		break;
	case i_CMP:
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "src", 1, 0,
			curi->dmode, "dstreg", curi->size, "dst", 1, 0);
		//genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, 0);
		//genamode (curi, curi->dmode, "dstreg", curi->size, "dst", 1, 0, 0);
		fill_prefetch_next ();
		if (curi->dmode == Dreg && curi->size == sz_long)
			addcycles000 (2);
		start_brace ();
		genflags (flag_cmp, curi->size, "newv", "src", "dst");
		break;
	case i_CMPA:
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "src", 1, 0,
			curi->dmode, "dstreg", sz_long, "dst", 1, 0);
		//genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, 0);
		//genamode (curi, curi->dmode, "dstreg", sz_long, "dst", 1, 0, 0);
		fill_prefetch_next ();
		addcycles000 (2);
		start_brace ();
		genflags (flag_cmp, sz_long, "newv", "src", "dst");
		break;
		/* The next two are coded a little unconventional, but they are doing
		* weird things... */
	case i_MVPRM: // MOVEP R->M
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, 0);
		printf ("\tuaecptr memp = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)%s;\n", gen_nextiword (0));
		if (curi->size == sz_word) {
			printf ("\t%s (memp, src >> 8);\n\t%s (memp + 2, src);\n", dstb, dstb);
			count_write += 2;
		} else {
			printf ("\t%s (memp, src >> 24);\n\t%s (memp + 2, src >> 16);\n", dstb, dstb);
			printf ("\t%s (memp + 4, src >> 8);\n\t%s (memp + 6, src);\n", dstb, dstb);
			count_write += 4;
		}
		fill_prefetch_next ();
		break;
	case i_MVPMR: // MOVEP M->R
		printf ("\tuaecptr memp = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)%s;\n", gen_nextiword (0));
		genamode (curi, curi->dmode, "dstreg", curi->size, "dst", 2, 0, 0);
		if (curi->size == sz_word) {
			printf ("\tuae_u16 val = ((%s (memp) & 0xff) << 8) + (%s (memp + 2) & 0xff);\n", srcb, srcb);
			count_read += 2;
		} else {
			printf ("\tuae_u32 val = ((%s (memp) & 0xff) << 24) + ((%s (memp + 2) & 0xff) << 16)\n", srcb, srcb);
			printf ("              + ((%s (memp + 4) & 0xff) << 8) + (%s (memp + 6) & 0xff);\n", srcb, srcb);
			count_read += 4;
		}
		fill_prefetch_next ();
		genastore ("val", curi->dmode, "dstreg", curi->size, "dst");
		break;
	case i_MOVE:
	case i_MOVEA:
		{
			/* 2 MOVE instruction variants have special prefetch sequence:
			* - MOVE <ea>,-(An) = prefetch is before writes (Apdi)
			* - MOVE memory,(xxx).L = 2 prefetches after write
			* - move.x #imm = prefetch is done before write
			* - all others = prefetch is done after writes
			*
			* - move.x xxx,[at least 1 extension word here] = fetch 1 extension word before (xxx)
			*
			*/
			if (using_ce020) {
				// MOVE is too complex to handle in table68k
				int h = 0, t = 0, c = 0, subhead = 0;
				bool fea = false;
				if (curi->smode == immi && isreg (curi->dmode)) {
					// MOVEQ
					h = 2; t = 0; c = 0;
				} else if (isreg (curi->smode) && isreg (curi->dmode)) {
					// MOVE Rn,Rn
					h = 2; t = 0; c = 2;
				} else if (isreg (curi->dmode)) {
					// MOVE EA,Rn
					h = 0; t = 0; c = 2;
					fea = true;
				} else if (curi->dmode == Aind) {
					if (isreg (curi->smode)) {
						// MOVE Rn,(An)
						h = 0; t = 1; c = 3;
					} else {
						// MOVE SOURCE,(An)
						h = 2; t = 0; c = 4;
						fea = true;
					}
				} else if (curi->dmode == Aipi) {
					if (isreg (curi->smode)) {
						// MOVE Rn,(An)+
						h = 0; t = 1; c = 3;
					} else {
						// MOVE SOURCE,(An)+
						h = 2; t = 0; c = 4;
						fea = true;
					}
				} else if (curi->dmode == Apdi) {
					if (isreg (curi->smode)) {
						// MOVE Rn,-(An)
						h = 0; t = 2; c = 4;
					} else {
						// MOVE SOURCE,-(An)
						h = 2; t = 0; c = 4;
						fea = true;
					}
				} else if (curi->dmode == Ad16) {
					// MOVE EA,(d16,An)
					h = 2; t = 0; c = 4;
					fea = true;
				} else if (curi->dmode == Ad8r) {
					h = 4; t = 0; c = 6;
					fea = true;
				} else if (curi->dmode == absw) {
					// MOVE EA,xxx.W
					h = 2; t = 0; c = 4;
					fea = true;
				} else if (curi->dmode == absl) {
					// MOVE EA,xxx.L
					h = 0; t = 0; c = 6;
					fea = true;
				} else {
					h = 4; t = 0; c = 6;
					fea = true;
				}
				if (fea) {
					if (curi->smode == imm)
						subhead = gence020cycles_fiea (curi, curi->size, Dreg);
					else
						subhead = gence020cycles_fea (curi->smode);
				}
				genamode2x (curi->smode, "srcreg", curi->size, "src", 1, 0, GF_MOVE, fea ? fetchmode_fea : -1);
				genamode2 (curi->dmode, "dstreg", curi->size, "dst", 2, 0, GF_MOVE);
				addopcycles_ce20 (h, t, c, -subhead);
				if (curi->mnemo == i_MOVEA && curi->size == sz_word)
					printf ("\tsrc = (uae_s32)(uae_s16)src;\n");
				if (curi->mnemo == i_MOVE)
					genflags (flag_logical, curi->size, "src", "", "");
				genastore ("src", curi->dmode, "dstreg", curi->size, "dst");
				sync_m68k_pc ();

			} else {
				int prefetch_done = 0, flags;
				int dualprefetch = curi->dmode == absl && (curi->smode != Dreg && curi->smode != Areg && curi->smode != imm);
				genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, GF_MOVE);
				flags = GF_MOVE | GF_APDI;
				//if (curi->size == sz_long && (curi->smode == Dreg || curi->smode == Areg))
				//	flags &= ~GF_APDI;
				flags |= dualprefetch ? GF_NOREFILL : 0;
				genamode (curi, curi->dmode, "dstreg", curi->size, "dst", 2, 0, flags);
				if (curi->mnemo == i_MOVEA && curi->size == sz_word)
					printf ("\tsrc = (uae_s32)(uae_s16)src;\n");
				if (curi->dmode == Apdi) {
					fill_prefetch_next ();
					prefetch_done = 1;
				}
				if (curi->mnemo == i_MOVE)
					genflags (flag_logical, curi->size, "src", "", "");
				genastore ("src", curi->dmode, "dstreg", curi->size, "dst");
				sync_m68k_pc ();
				if (dualprefetch) {
					fill_prefetch_full_000 ();
					prefetch_done = 1;
				}
				if (!prefetch_done)
					fill_prefetch_next ();
			}
		}
		break;
	case i_MVSR2: // MOVE FROM SR
		genamode (curi, curi->smode, "srcreg", sz_word, "src", 2, 0, 0);
		if (isreg (curi->smode)) {
			fill_prefetch_next ();
			addcycles000 (2);
		} else {
			// write to memory, dummy write to same address, X-flag seems to be always set
			if (cpu_level <= 1 && curi->size == sz_word) {
				printf ("\t%s (srca, regs.sr | 0x0010);\n", dstw);
				count_write++;
			}
			fill_prefetch_next ();
		}
		printf ("\tMakeSR ();\n");
		// real write
		if (curi->size == sz_byte)
			genastore ("regs.sr & 0xff", curi->smode, "srcreg", sz_word, "src");
		else
			genastore ("regs.sr", curi->smode, "srcreg", sz_word, "src");
		break;
	case i_MV2SR: // MOVE TO SR
		genamode (curi, curi->smode, "srcreg", sz_word, "src", 1, 0, 0);
		if (curi->size == sz_byte) {
			// MOVE TO CCR
			addcycles000 (4);
			printf ("\tMakeSR ();\n\tregs.sr &= 0xFF00;\n\tregs.sr |= src & 0xFF;\n");
		} else {
			// MOVE TO SR
			addcycles000 (4);
			printf ("\tregs.sr = src;\n");
		}
		makefromsr ();
		if (cpu_level <= 1) {
			// 68000 does 2xprefetch
			sync_m68k_pc ();
			fill_prefetch_full ();
		} else {
			fill_prefetch_next ();
		}
		break;
	case i_SWAP:
		genamode (curi, curi->smode, "srcreg", sz_long, "src", 1, 0, 0);
		fill_prefetch_next ();
		start_brace ();
		printf ("\tuae_u32 dst = ((src >> 16)&0xFFFF) | ((src&0xFFFF)<<16);\n");
		genflags (flag_logical, sz_long, "dst", "", "");
		genastore ("dst", curi->smode, "srcreg", sz_long, "src");
		break;
	case i_EXG:
		// confirmed
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "src", 1, 0,
			curi->dmode, "dstreg", curi->size, "dst", 1, 0);
		//genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, 0);
		//genamode (curi, curi->dmode, "dstreg", curi->size, "dst", 1, 0, 0);
		fill_prefetch_next ();
		addcycles000 (2);
		genastore ("dst", curi->smode, "srcreg", curi->size, "src");
		genastore ("src", curi->dmode, "dstreg", curi->size, "dst");
		break;
	case i_EXT:
		// confirmed
		genamode (curi, curi->smode, "srcreg", sz_long, "src", 1, 0, 0);
		fill_prefetch_next ();
		start_brace ();
		switch (curi->size) {
		case sz_byte: printf ("\tuae_u32 dst = (uae_s32)(uae_s8)src;\n"); break;
		case sz_word: printf ("\tuae_u16 dst = (uae_s16)(uae_s8)src;\n"); break;
		case sz_long: printf ("\tuae_u32 dst = (uae_s32)(uae_s16)src;\n"); break;
		default: term ();
		}
		genflags (flag_logical,
			curi->size == sz_word ? sz_word : sz_long, "dst", "", "");
		genastore ("dst", curi->smode, "srcreg",
			curi->size == sz_word ? sz_word : sz_long, "src");
		break;
	case i_MVMEL:
		// confirmed
		//if (using_ce)
			genmovemel_ce (opcode);
		//else
		//	genmovemel (opcode);
		break;
	case i_MVMLE:
		// confirmed
		//if (using_ce || using_prefetch)
			genmovemle_ce (opcode);
		//else
		//	genmovemle (opcode);
		break;
	case i_TRAP:
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, 0);
		gen_set_fault_pc ();
		sync_m68k_pc ();
		printf ("\tException (src + 32);\n");
		did_prefetch = 1;
		m68k_pc_offset = 0;
		break;
	case i_MVR2USP:
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, 0);
		fill_prefetch_next ();
		printf ("\tregs.usp = src;\n");
		break;
	case i_MVUSP2R:
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 2, 0, 0);
		fill_prefetch_next ();
		genastore ("regs.usp", curi->smode, "srcreg", curi->size, "src");
		break;
	case i_RESET:
		fill_prefetch_next ();
		printf ("\tcpureset ();\n");
		sync_m68k_pc ();
		addcycles000 (128);
		if (using_prefetch) {
			printf ("\t%s (2);\n", prefetch_word);
			m68k_pc_offset = 0;
		}
		break;
	case i_NOP:
		fill_prefetch_next ();
		if (using_ce020)
			printf ("\t%s (6);\n", do_cycles);
		break;
	case i_STOP:
		if (using_prefetch) {
			printf ("\tregs.sr = regs.irc;\n");
			m68k_pc_offset += 2;
		} else {
			genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, 0);
			printf ("\tregs.sr = src;\n");
		}
		makefromsr ();
		printf ("\tm68k_setstopped ();\n");
		sync_m68k_pc ();
		// STOP does not prefetch anything
		did_prefetch = -1;
		break;
	case i_LPSTOP: /* 68060 */
		printf ("\tuae_u16 sw = %s (2);\n", srcwi);
		printf ("\tuae_u16 sr;\n");
		printf ("\tif (sw != (0x100|0x80|0x40)) { Exception (4); return 4 * CYCLE_UNIT / 2; }\n");
		printf ("\tsr = %s (4);\n", srcwi);
		printf ("\tif (!(sr & 0x8000)) { Exception (8); return 4 * CYCLE_UNIT / 2; }\n");
		printf ("\tregs.sr = sr;\n");
		makefromsr ();
		printf ("\tm68k_setstopped();\n");
		m68k_pc_offset += 4;
		sync_m68k_pc ();
		fill_prefetch_full ();
		break;
	case i_RTE:
		next_level_000 ();
		if (cpu_level == 0) {
			genamode (NULL, Aipi, "7", sz_word, "sr", 1, 0, GF_NOREFILL);
			genamode (NULL, Aipi, "7", sz_long, "pc", 1, 0, GF_NOREFILL);
			printf ("\tregs.sr = sr;\n");
			printf ("\tif (pc & 1) {\n");
			printf ("\t\texception3i (0x%04X, pc);\n", opcode);
			returncycles_exception ("", (count_read + 1 + count_write) * 4 + count_cycles);
			printf ("\t}\n");
			setpc ("pc");
			makefromsr ();
		} else if (cpu_level == 1 && using_prefetch) {
	    int old_brace_level = n_braces;
	    printf ("\tuae_u16 newsr; uae_u32 newpc;\n");
			printf ("\tfor (;;) {\n");
			printf ("\t\tuaecptr a = m68k_areg (regs, 7);\n");
			printf ("\t\tuae_u16 sr = %s (a);\n", srcw);
			count_read++;
			printf ("\t\tuae_u32 pc = %s (a + 2) << 16; pc |= %s (a + 4);\n", srcw, srcw);
			count_read += 2;
			printf ("\t\tuae_u16 format = %s (a + 2 + 4);\n", srcw);
			count_read++;
			printf ("\t\tint frame = format >> 12;\n");
			printf ("\t\tint offset = 8;\n");
			printf ("\t\tnewsr = sr; newpc = pc;\n");
			addcycles_ce020 (6);
	    printf ("\t\tif (frame == 0x0) { m68k_areg (regs, 7) += offset; break; }\n");
    	printf ("\t\telse if (frame == 0x8) { m68k_areg (regs, 7) += offset + 50; break; }\n");
	    printf ("\t\telse { m68k_areg (regs, 7) += offset; Exception (14); return 4 * CYCLE_UNIT / 2; }\n");
	    printf ("\t\tregs.sr = newsr;\n");
			makefromsr ();
			printf ("}\n");
	    pop_braces (old_brace_level);
	    printf ("\tregs.sr = newsr;\n");
			addcycles_ce020 (4);
			makefromsr ();
	    printf ("\tif (newpc & 1) {\n");
	    printf ("\t\texception3i (0x%04X, newpc);\n", opcode);
			returncycles_exception ("", (count_read + 1 + count_write) * 4 + count_cycles);
			printf ("\t}\n");
	    setpc ("newpc");
			check_ipl ();
		} else {
		    int old_brace_level = n_braces;
		    printf ("\tuae_u16 newsr; uae_u32 newpc;\n");
			printf ("\tfor (;;) {\n");
			printf ("\t\tuaecptr a = m68k_areg (regs, 7);\n");
			printf ("\t\tuae_u16 sr = %s (a);\n", srcw);
			count_read++;
			printf ("\t\tuae_u32 pc = %s (a + 2);\n", srcl);
			count_read += 2;
			printf ("\t\tuae_u16 format = %s (a + 2 + 4);\n", srcw);
			count_read++;
			printf ("\t\tint frame = format >> 12;\n");
			printf ("\t\tint offset = 8;\n");
			printf ("\t\tnewsr = sr; newpc = pc;\n");
			addcycles_ce020 (6);
		    printf ("\t\tif (frame == 0x0) { m68k_areg (regs, 7) += offset; break; }\n");
		    printf ("\t\telse if (frame == 0x1) { m68k_areg (regs, 7) += offset; }\n");
		    printf ("\t\telse if (frame == 0x2) { m68k_areg (regs, 7) += offset + 4; break; }\n");
		    if (using_mmu == 68060) {
				printf ("\t\telse if (frame == 0x4) { m68k_do_rte_mmu060 (a); m68k_areg (regs, 7) += offset + 8; break; }\n");
			} else if (cpu_level >= 4) {
				printf ("\t\telse if (frame == 0x4) { m68k_areg (regs, 7) += offset + 8; break; }\n");
			}
			if (cpu_level == 1) // 68010 only
		    printf ("\t\telse if (frame == 0x8) { m68k_areg (regs, 7) += offset + 50; break; }\n");
			if (using_mmu == 68040) {
		    	printf ("\t\telse if (frame == 0x7) { m68k_do_rte_mmu040 (a); m68k_areg (regs, 7) += offset + 52; break; }\n");
			} else if (cpu_level >= 4) {
		    	printf ("\t\telse if (frame == 0x7) { m68k_areg (regs, 7) += offset + 52; break; }\n");
			}
			printf ("\t\telse if (frame == 0x9) { m68k_areg (regs, 7) += offset + 12; break; }\n");
			if (using_mmu == 68030) {
				printf ("\t\telse if (frame == 0xa) { m68k_do_rte_mmu030 (a); break; }\n");
			    printf ("\t\telse if (frame == 0xb) { m68k_do_rte_mmu030 (a); break; }\n");
			} else {
				printf ("\t\telse if (frame == 0xa) { m68k_areg (regs, 7) += offset + 24; break; }\n");
			    printf ("\t\telse if (frame == 0xb) { m68k_areg (regs, 7) += offset + 84; break; }\n");
			}
	    printf ("\t\telse { m68k_areg (regs, 7) += offset; Exception (14); return 4 * CYCLE_UNIT / 2; }\n");
		    printf ("\t\tregs.sr = newsr;\n");
			makefromsr ();
			printf ("}\n");
		    pop_braces (old_brace_level);
		    printf ("\tregs.sr = newsr;\n");
			addcycles_ce020 (4);
			makefromsr ();
		    printf ("\tif (newpc & 1) {\n");
		    printf ("\t\texception3i (0x%04X, newpc);\n", opcode);
			printf ("\t\treturn 4 * CYCLE_UNIT / 2;\n");
			printf ("\t}\n");
		    setpc ("newpc");
			check_ipl ();
		}
		/* PC is set and prefetch filled. */
		m68k_pc_offset = 0;
		tail_ce020_done = true;
		fill_prefetch_full ();
		break;
	case i_RTD:
		addop_ce020 (curi, 0);
		if (using_mmu) {
			genamode (curi, curi->smode, "srcreg", curi->size, "offs", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
			genamode (NULL, Aipi, "7", sz_long, "pc", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
			printf ("\tm68k_areg(regs, 7) += offs;\n");
		} else {
			genamode (NULL, Aipi, "7", sz_long, "pc", 1, 0, 0);
			genamode (curi, curi->smode, "srcreg", curi->size, "offs", 1, 0, 0);
			printf ("\tm68k_areg (regs, 7) += offs;\n");
		}
		printf ("\tif (pc & 1) {\n");
		printf ("\t\texception3i (0x%04X, pc);\n", opcode);
		returncycles_exception ("", (count_read + 1 + count_write) * 4 + count_cycles);
		printf ("\t}\n");
		setpc ("pc");
		/* PC is set and prefetch filled. */
		m68k_pc_offset = 0;
		tail_ce020_done = true;
		fill_prefetch_full ();
		break;
	case i_LINK:
		// ce confirmed
		if (using_mmu) {
			addmmufixup ("srcreg");
			genamode (NULL, curi->dmode, "dstreg", curi->size, "offs", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
			genamode (NULL, Apdi, "7", sz_long, "old", GENA_GETV_FETCH_ALIGN, GENA_MOVEM_DO_INC, 0);
			genamode (NULL, curi->smode, "srcreg", sz_long, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
			genastore ("m68k_areg(regs, 7)", curi->smode, "srcreg", sz_long, "src");
			printf ("\tm68k_areg(regs, 7) += offs;\n");
			genastore ("src", Apdi, "7", sz_long, "old");
		} else {
			addop_ce020 (curi, 0);
			genamode (NULL, Apdi, "7", sz_long, "old", 2, 0, GF_AA);
			genamode (NULL, curi->smode, "srcreg", sz_long, "src", 1, 0, GF_AA);
			genamode (NULL, curi->dmode, "dstreg", curi->size, "offs", 1, 0, 0);
			genastore ("src", Apdi, "7", sz_long, "old");
			genastore ("m68k_areg (regs, 7)", curi->smode, "srcreg", sz_long, "src");
			printf ("\tm68k_areg (regs, 7) += offs;\n");
			fill_prefetch_next ();
		}
		break;
	case i_UNLK:
		// ce confirmed
		if (using_mmu) {
			genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, 0);
			printf ("\tuae_s32 old = %s (src);\n", srcl);
			printf ("\tm68k_areg (regs, 7) = src + 4;\n");
			printf ("\tm68k_areg (regs, srcreg) = old;\n");
		} else {
			genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, 0);
			printf ("\tm68k_areg (regs, 7) = src;\n");
			genamode (NULL, Aipi, "7", sz_long, "old", 1, 0, 0);
			fill_prefetch_next ();
			genastore ("old", curi->smode, "srcreg", curi->size, "src");
		}
		break;
	case i_RTS:
		addop_ce020 (curi, 0);
		printf ("\tuaecptr pc = %s;\n", getpc);
		if (using_ce020 == 1) {
			add_head_cycs (1);
			printf ("\tm68k_do_rts_ce020 ();\n");
		} else if (using_ce020 == 2) {
			add_head_cycs (1);
			printf ("\tm68k_do_rts_ce030 ();\n");
		} else if (using_ce) {
			printf ("\tm68k_do_rts_ce ();\n");
		} else if (using_mmu) {
			printf ("\tm68k_do_rts_mmu%s ();\n", mmu_postfix);
		} else if (using_prefetch || using_prefetch_020) {
			printf ("\tm68k_do_rtsi ();\n");
		} else {
			printf ("\tm68k_do_rts ();\n");
		}
	  printf ("\tif (%s & 1) {\n", getpc);
		printf ("\t\tuaecptr faultpc = %s;\n", getpc);
		setpc ("pc");
		printf ("\t\texception3i (0x%04X, faultpc);\n", opcode);
		printf ("\t\treturn 8 * CYCLE_UNIT / 2;\n");
		printf ("\t}\n");
		count_read += 2;
		m68k_pc_offset = 0;
		fill_prefetch_full ();
		break;
	case i_TRAPV:
		sync_m68k_pc ();
		fill_prefetch_next ();
		printf ("\tif (GET_VFLG ()) {\n");
		printf ("\t\tException (7);\n");
		printf ("\t\treturn 4 * CYCLE_UNIT / 2;\n");
		printf ("\t}\n");
		break;
	case i_RTR:
		printf ("\tuaecptr oldpc = %s;\n", getpc);
		printf ("\tMakeSR ();\n");
		genamode (NULL, Aipi, "7", sz_word, "sr", 1, 0, 0);
		genamode (NULL, Aipi, "7", sz_long, "pc", 1, 0, 0);
		printf ("\tregs.sr &= 0xFF00; sr &= 0xFF;\n");
		printf ("\tregs.sr |= sr;\n");
		setpc ("pc");
		makefromsr ();
		printf ("\tif (%s & 1) {\n", getpc);
		printf ("\t\tuaecptr faultpc = %s;\n", getpc);
		setpc ("oldpc");
		printf ("\t\texception3i (0x%04X, faultpc);\n", opcode);
		printf ("\t\treturn 8 * CYCLE_UNIT / 2;\n");
		printf ("\t}\n");
		m68k_pc_offset = 0;
		fill_prefetch_full ();
		tail_ce020_done = true;
		break;
	case i_JSR:
		// possible idle cycle, prefetch from new address, stack high return addr, stack low, prefetch
		no_prefetch_ce020 = true;
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 0, 0, GF_AA|GF_NOREFILL);
		start_brace ();
		printf ("\tuaecptr oldpc = %s + %d;\n", getpc, m68k_pc_offset);
		if (using_exception_3) {
			printf ("\tif (srca & 1) {\n");
			printf ("\t\texception3i (opcode, srca);\n");
			returncycles_exception ("", (count_read + 1 + count_write) * 4 + count_cycles);
			printf ("\t}\n");
		}
		if (using_mmu) {
			printf ("\t%s (m68k_areg (regs, 7) - 4, oldpc);\n", dstl);
			printf ("\tm68k_areg (regs, 7) -= 4;\n");
			setpc ("srca");
			m68k_pc_offset = 0;
		} else {
			if (curi->smode == Ad16 || curi->smode == absw || curi->smode == PC16)
				addcycles000 (2);
			if (curi->smode == Ad8r || curi->smode == PC8r) {
				addcycles000 (6);
				if (cpu_level <= 1 && using_prefetch)
					printf ("\toldpc += 2;\n");
			}
			setpc ("srca");
			m68k_pc_offset = 0;
			fill_prefetch_1 (0);
			printf ("\tm68k_areg (regs, 7) -= 4;\n");
			if (using_ce || using_prefetch) {
				printf ("\t%s (m68k_areg (regs, 7), oldpc >> 16);\n", dstw);
				printf ("\t%s (m68k_areg (regs, 7) + 2, oldpc);\n", dstw);
			} else {
				printf ("\t%s (m68k_areg (regs, 7), oldpc);\n", dstl);
			}
		}
		count_write += 2;
    if(!using_prefetch && curi->smode == Aind)
      addcycles000 (4);
		fill_prefetch_full_020 ();
		fill_prefetch_next ();
		break;
	case i_JMP:
		no_prefetch_ce020 = true;
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 0, 0, GF_AA|GF_NOREFILL);
		if (using_prefetch && (curi->smode == Ad8r || curi->smode == PC8r))
			addcycles000 (4);
		if (using_exception_3) {
			printf ("\tif (srca & 1) {\n");
			printf ("\t\texception3i (opcode, srca);\n");
			returncycles_exception ("", (count_read + 1 + count_write) * 4 + count_cycles);
			printf ("\t}\n");
		}
    if(!(using_ce || using_prefetch)) {
		  if (curi->smode != Aind && curi->smode != Ad8r && curi->smode != PC8r)
			  count_read--;
    }
		if (curi->smode == Ad16 || curi->smode == Ad8r || curi->smode == absw || curi->smode == PC16 || curi->smode == PC8r)
			addcycles000 (2);
		setpc ("srca");
		m68k_pc_offset = 0;
		fill_prefetch_full ();
		break;
	case i_BSR:
		// .b/.w = idle cycle, store high, store low, 2xprefetch
		if (using_ce020)
			no_prefetch_ce020 = true;
		printf ("\tuae_s32 s;\n");
		if (curi->size == sz_long) {
			if (next_cpu_level < 1)
				next_cpu_level = 1;
		}
		if (curi->size == sz_long && cpu_level < 2) {
			printf ("\tuae_u32 src = 0xffffffff;\n");
		} else {
			genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, GF_AA|GF_NOREFILL);
		}
		printf ("\ts = (uae_s32)src + 2;\n");
		if (using_exception_3) {
			printf ("\tif (src & 1) {\n");
			printf ("\t\texception3b (opcode, %s + s, 0, 1, %s + s);\n", getpc, getpc);
			returncycles_exception ("", (count_read + 1 + count_write) * 4 + count_cycles);
			printf ("\t}\n");
		}
		addcycles000 (2);
		if (using_ce020 == 1) {
			printf ("\tm68k_do_bsr_ce020 (%s + %d, s);\n", getpc, m68k_pc_offset);
		} else if (using_ce020 == 2) {
			printf ("\tm68k_do_bsr_ce030 (%s + %d, s);\n", getpc, m68k_pc_offset);
		} else if (using_ce) {
			printf ("\tm68k_do_bsr_ce (%s + %d, s);\n", getpc, m68k_pc_offset);
		} else if (using_mmu) {
			printf ("\tm68k_do_bsr_mmu%s (%s + %d, s);\n", mmu_postfix, getpc, m68k_pc_offset);
		} else if (using_prefetch || using_prefetch_020) {
			printf ("\tm68k_do_bsri (%s + %d, s);\n", getpc, m68k_pc_offset);
		} else {
			printf ("\tm68k_do_bsr (%s + %d, s);\n", getpc, m68k_pc_offset);
		}
		count_write += 2;
    if(!using_prefetch && curi->smode == imm1)
      count_write--;
		m68k_pc_offset = 0;
		fill_prefetch_full ();
		break;
	case i_Bcc:
		// bcc.b branch: idle cycle, prefetch, prefetch
		// bcc.b not branch: 2 idle cycles, prefetch
		tail_ce020_done = true;
		if (curi->size == sz_long) {
			if (cpu_level < 2) {
				addcycles000 (2);
				printf ("\tif (cctrue (regs.ccrflags, %d)) {\n", curi->cc);
				printf ("\t\texception3i (opcode, %s + 1);\n", getpc);
				returncycles_exception ("", (count_read + 1 + count_write) * 4 + count_cycles);
				printf ("\t}\n");
				sync_m68k_pc ();
				addcycles000 (2);
				irc2ir ();
				fill_prefetch_2 ();
				goto bccl_not68020;
			} else {
				if (next_cpu_level < 1)
					next_cpu_level = 1;
			}
		}
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, GF_AA | (cpu_level < 2 ? GF_NOREFILL : 0));
		addcycles000 (2);
		printf ("\tif (!cctrue (regs.ccrflags, %d)) goto didnt_jump;\n", curi->cc);
		if (using_exception_3) {
			printf ("\tif (src & 1) {\n");
			printf ("\t\texception3i (opcode, %s + 2 + (uae_s32)src);\n", getpc);
			returncycles_exception ("", (count_read + 1 + count_write) * 4 + count_cycles);
			printf ("\t}\n");
		}
		if (using_prefetch) {
			incpc ("(uae_s32)src + 2");
			fill_prefetch_full_000 ();
			if (using_ce)
				printf ("\treturn;\n");
			else
				printf ("\treturn 10 * CYCLE_UNIT / 2;\n");
		} else {
			incpc ("(uae_s32)src + 2");
			add_head_cycs (6);
			fill_prefetch_full_020 ();
			returncycles ("\t", 10);
		}
		printf ("didnt_jump:;\n");
		sync_m68k_pc ();
		get_prefetch_020_0 ();
		if (curi->size == sz_byte) {
			addcycles000 (2);
			irc2ir ();
			add_head_cycs (4);
			fill_prefetch_2 ();
		} else if (curi->size == sz_word) {
			addcycles000 (2);
			add_head_cycs (6);
			fill_prefetch_full_000 ();
		} else {
			add_head_cycs (6);
			fill_prefetch_full_000 ();
		}
		if (using_prefetch) {
			count_read = curi->size == sz_byte ? 1 : 2;
		} else {
			count_read = 0;
			count_cycles = 0;
 		}
		insn_n_cycles = curi->size == sz_byte ? 8 : 12;
bccl_not68020:
		break;
	case i_LEA:
		if (curi->smode == Ad8r || curi->smode == PC8r)
			addcycles000 (2);
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "src", 0, GF_AA,
			curi->dmode, "dstreg", curi->size, "dst", 2, GF_AA);
		//genamode (curi, curi->smode, "srcreg", curi->size, "src", 0, 0, GF_AA);
		//genamode (curi, curi->dmode, "dstreg", curi->size, "dst", 2, 0, GF_AA);
		if (curi->smode == Ad8r || curi->smode == PC8r)
			addcycles000 (2);
		fill_prefetch_next ();
		genastore ("srca", curi->dmode, "dstreg", curi->size, "dst");
		break;
	case i_PEA:
		if (curi->smode == Ad8r || curi->smode == PC8r)
			addcycles000 (2);
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 0, 0, GF_AA);
		genamode (NULL, Apdi, "7", sz_long, "dst", 2, 0, GF_AA);
		if (!(curi->smode == absw || curi->smode == absl))
			fill_prefetch_next ();
		if (curi->smode == Ad8r || curi->smode == PC8r)
			addcycles000 (2);
		genastore ("srca", Apdi, "7", sz_long, "dst");
		if ((curi->smode == absw || curi->smode == absl))
			fill_prefetch_next ();
		break;
	case i_DBcc:
		// cc true: idle cycle, prefetch
		// cc false, counter expired: idle cycle, prefetch (from branch address), 2xprefetch (from next address)
		// cc false, counter not expired: idle cycle, prefetch
		tail_ce020_done = true;
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "src", 1, GF_AA | (cpu_level < 2 ? GF_NOREFILL : 0),
			curi->dmode, "dstreg", curi->size, "offs", 1, GF_AA | (cpu_level < 2 ? GF_NOREFILL : 0));
		//genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, GF_AA | (cpu_level < 2 ? GF_NOREFILL : 0));
		//genamode (curi, curi->dmode, "dstreg", curi->size, "offs", 1, 0, GF_AA | (cpu_level < 2 ? GF_NOREFILL : 0));
		printf ("\tuaecptr oldpc = %s;\n", getpc);
		addcycles000 (2);
		printf ("\tif (!cctrue (regs.ccrflags, %d)) {\n", curi->cc);
		incpc ("(uae_s32)offs + 2");
		printf ("\t");
		fill_prefetch_1 (0);
		printf ("\t");
		genastore ("(src - 1)", curi->smode, "srcreg", curi->size, "src");

		printf ("\t\tif (src) {\n");
		if (using_exception_3) {
			printf ("\t\t\tif (offs & 1) {\n");
			printf ("\t\t\t\texception3i (opcode, %s + 2 + (uae_s32)offs + 2);\n", getpc);
			returncycles_exception ("", (count_read + 1 + count_write) * 4 + count_cycles);
			printf ("\t\t\t}\n");
		}
		irc2ir ();
		add_head_cycs (6);
		fill_prefetch_1 (2);
		fill_prefetch_full_020 ();
		returncycles ("\t\t\t", 10);
		printf ("\t\t}\n");
		add_head_cycs (10);
		addcycles000_nonce("\t\t", 4);
		printf ("\t} else {\n");
		// cc == true
		addcycles000_nonce("\t\t", 2);
		addcycles_ce020 (4);
		printf ("\t}\n");
		setpc ("oldpc + %d", m68k_pc_offset);
		m68k_pc_offset = 0;
		get_prefetch_020_0 ();
		fill_prefetch_full_000 ();
		insn_n_cycles = 12;
		if (using_ce || using_prefetch)
			count_read = 2;
		else
			count_read = 1;
		count_write = 0;
		break;
	case i_Scc:
		// confirmed
		next_level_000 ();
		genamode (curi, curi->smode, "srcreg", curi->size, "src", cpu_level == 0 ? 1 : 2, 0, 0);
		start_brace ();
		fill_prefetch_next();
		start_brace ();
		printf ("\tint val = cctrue (regs.ccrflags, %d) ? 0xff : 0;\n", curi->cc);
		if (isreg (curi->smode)) {
			if (using_ce)
				printf ("\tint cycles = val ? 2 : 0;\n");
			addcycles000_3 ("\t");
			addcycles000_nonces("\t", "(val ? 2 : 0)");
		}
		genastore ("val", curi->smode, "srcreg", curi->size, "src");
    if(!using_prefetch && curi->smode != Dreg && cpu_level == 1)
      addcycles000 (2);
		break;
	case i_DIVU:
		tail_ce020_done	= true;
		genamodedual (curi,
			curi->smode, "srcreg", sz_word, "src", 1, 0,
			curi->dmode, "dstreg", sz_long, "dst", 1, 0);
		printf ("\tCLEAR_CZNV ();\n");
		printf ("\tif (src == 0) {\n");
		if (cpu_level > 1) {
			/* 68020 sets V when dividing by zero and N if dst is negative (0 for DIVU)
			* 68000 clears both
			*/
			printf("\t\tSET_VFLG (1);\n");
			printf("\t\tif (dst < 0) SET_NFLG (1);\n");
		}
		incpc ("%d", m68k_pc_offset);
		printf ("\t\tException (5);\n");
		returncycles_exception ("", (count_read + 1 + count_write) * 4 + count_cycles);
		printf ("\t} else {\n");
		printf ("\t\tuae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;\n");
		printf ("\t\tuae_u32 rem = (uae_u32)dst %% (uae_u32)(uae_u16)src;\n");
		if (using_ce) {
			start_brace ();
			printf ("\t\tint cycles = (getDivu68kCycles((uae_u32)dst, (uae_u16)src)) - 4;\n");
			addcycles000_3 ("\t\t");
		}
		addcycles000_nonces("\t\t", "(getDivu68kCycles((uae_u32)dst, (uae_u16)src)) - 4");
		fill_prefetch_next ();
		/* The N flag appears to be set each time there is an overflow.
		* Weird. but 68020 only sets N when dst is negative.. */
		printf ("\t\tif (newv > 0xffff) {\n");
		printf ("\t\t\tSET_VFLG (1);\n");
#ifdef UNDEF68020
		if (cpu_level >= 2)
			printf ("\t\t\tif (currprefs.cpu_level == 0 || dst < 0) SET_NFLG (&regs, 1);\n");
		else /* ??? some 68000 revisions may not set NFLG when overflow happens.. */
#endif
			printf ("\t\t\tSET_NFLG (1);\n");
		printf ("\t\t} else {\n");
		printf ("\t\t"); genflags (flag_logical, sz_word, "newv", "", "");
		printf ("\t\t\tnewv = (newv & 0xffff) | ((uae_u32)rem << 16);\n");
		printf ("\t\t"); genastore ("newv", curi->dmode, "dstreg", sz_long, "dst");
		printf ("\t\t}\n");
		sync_m68k_pc ();
		printf ("\t}\n");
		count_ncycles++;
		//insn_n_cycles += 136 - (136 - 76) / 2; /* average */
		tail_ce020_done	= false;
		returntail (false);
		break;
	case i_DIVS:
		tail_ce020_done	= true;
		genamodedual (curi,
			curi->smode, "srcreg", sz_word, "src", 1, 0,
			curi->dmode, "dstreg", sz_long, "dst", 1, 0);
		printf ("\tif (src == 0) {\n");
		if (cpu_level > 1) {
			/* 68020 sets V when dividing by zero. Z is also set.
			* 68000 clears both
			*/
			printf("\t\tSET_VFLG (1);\n");
			printf("\t\tSET_ZFLG (1);\n");
		}
		incpc ("%d", m68k_pc_offset);
		printf ("\t\tException (5);\n");
		returncycles_exception ("", (count_read + 1 + count_write) * 4 + count_cycles);
		printf ("\t}\n");
		printf ("\tCLEAR_CZNV ();\n");
		if (using_ce) {
			start_brace ();
			printf ("\t\tint cycles = (getDivs68kCycles((uae_s32)dst, (uae_s16)src)) - 4;\n");
			addcycles000_3 ("\t\t");
		}
		addcycles000_nonces("\t\t", "(getDivs68kCycles((uae_s32)dst, (uae_s16)src)) - 4");
		fill_prefetch_next ();
		printf ("\tif (dst == 0x80000000 && src == -1) {\n");
		printf ("\t\tSET_VFLG (1);\n");
		printf ("\t\tSET_NFLG (1);\n");
		printf ("\t} else {\n");
		printf ("\t\tuae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;\n");
		printf ("\t\tuae_u16 rem = (uae_s32)dst %% (uae_s32)(uae_s16)src;\n");
		printf ("\t\tif ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) {\n");
		printf ("\t\t\tSET_VFLG (1);\n");
#ifdef UNDEF68020
		if (cpu_level > 0)
			printf ("\t\t\tif (currprefs.cpu_level == 0) SET_NFLG (&regs, 1);\n");
		else
#endif
			printf ("\t\t\tSET_NFLG (1);\n");
		printf ("\t\t} else {\n");
		printf ("\t\t\tif (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;\n");
		genflags (flag_logical, sz_word, "newv", "", "");
		printf ("\t\t\tnewv = (newv & 0xffff) | ((uae_u32)rem << 16);\n");
		printf ("\t\t"); genastore ("newv", curi->dmode, "dstreg", sz_long, "dst");
		printf ("\t\t}\n");
		printf ("\t}\n");
		sync_m68k_pc ();
		count_ncycles++;
		//insn_n_cycles += 156 - (156 - 120) / 2; /* average */
		tail_ce020_done	= false;
		returntail (false);
		break;
	case i_MULU:
		genamodedual (curi,
			curi->smode, "srcreg", sz_word, "src", 1, 0,
			curi->dmode, "dstreg", sz_word, "dst", 1, 0);
		fill_prefetch_next();
		start_brace ();
		printf ("\tuae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;\n");
		if (using_ce)
			printf ("\tint cycles = 38 - 4, bits;\n");
		genflags (flag_logical, sz_long, "newv", "", "");
		if (using_ce) {
			printf ("\tfor(bits = 0; bits < 16 && src; bits++, src >>= 1)\n");
			printf ("\t\tif (src & 1) cycles += 2;\n");
			addcycles000_3 ("\t");
		}
    addcycles000_nonces("\t", "bitset_count16(src) * 2");
		genastore ("newv", curi->dmode, "dstreg", sz_long, "dst");
		sync_m68k_pc ();
		count_cycles += 38 - 4;
		count_ncycles++;
		insn_n_cycles += (70 - 38) / 2 + 38; /* average */
		break;
	case i_MULS:
		genamodedual (curi,
			curi->smode, "srcreg", sz_word, "src", 1, 0,
			curi->dmode, "dstreg", sz_word, "dst", 1, 0);
		fill_prefetch_next();
		start_brace ();
		printf ("\tuae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;\n");
		if (using_ce) {
			printf ("\tint cycles = 38 - 4, bits;\n");
			printf ("\tuae_u32 usrc;\n");
		}
		genflags (flag_logical, sz_long, "newv", "", "");
		if (using_ce) {
			printf ("\tusrc = ((uae_u32)src) << 1;\n");
			printf ("\tfor(bits = 0; bits < 16 && usrc; bits++, usrc >>= 1)\n");
			printf ("\t\tif ((usrc & 3) == 1 || (usrc & 3) == 2) cycles += 2;\n");
			addcycles000_3 ("\t");
		}
		addcycles000_nonces("\t", "bitset_count16(src ^ (src << 1)) * 2");
		genastore ("newv", curi->dmode, "dstreg", sz_long, "dst");
		count_cycles += 38 - 4;
		count_ncycles++;
		insn_n_cycles += (70 - 38) / 2 + 38; /* average */
		break;
	case i_CHK:
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, 0);
		genamode (curi, curi->dmode, "dstreg", curi->size, "dst", 1, 0, 0);
		sync_m68k_pc ();
		addcycles000 (4);
		printf ("\tif (dst > src) {\n");
		printf ("\t\tSET_NFLG (0);\n");
		printf ("\t\tException (6);\n");
		returncycles_exception ("", (count_read + 1 + count_write) * 4 + count_cycles);
		printf ("\t}\n");
		addcycles000 (2);
		printf ("\tif ((uae_s32)dst < 0) {\n");
		printf ("\t\tSET_NFLG (1);\n");
		printf ("\t\tException (6);\n");
		returncycles_exception ("", (count_read + 1 + count_write) * 4 + count_cycles);
		printf ("\t}\n");
		fill_prefetch_next ();
		break;
	case i_CHK2:
		genamode (curi, curi->smode, "srcreg", curi->size, "extra", 1, 0, 0);
		genamode (curi, curi->dmode, "dstreg", curi->size, "dst", 2, 0, 0);
		fill_prefetch_0 ();
		printf ("\t{uae_s32 upper,lower,reg = regs.regs[(extra >> 12) & 15];\n");
		switch (curi->size) {
		case sz_byte:
			printf ("\tlower = (uae_s32)(uae_s8)%s (dsta); upper = (uae_s32)(uae_s8)%s (dsta + 1);\n", srcb, srcb);
			printf ("\tif ((extra & 0x8000) == 0) reg = (uae_s32)(uae_s8)reg;\n");
			break;
		case sz_word:
			printf ("\tlower = (uae_s32)(uae_s16)%s (dsta); upper = (uae_s32)(uae_s16)%s (dsta + 2);\n", srcw, srcw);
			printf ("\tif ((extra & 0x8000) == 0) reg = (uae_s32)(uae_s16)reg;\n");
			break;
		case sz_long:
			printf ("\tlower = %s (dsta); upper = %s (dsta + 4);\n", srcl, srcl);
			break;
		default:
			term ();
		}
		printf ("\tSET_ZFLG (upper == reg || lower == reg);\n");
		printf ("\tSET_CFLG_ALWAYS (lower <= upper ? reg < lower || reg > upper : reg > upper || reg < lower);\n");
		printf ("\tif ((extra & 0x800) && GET_CFLG ()) { Exception (6);\n");
		returncycles_exception ("", (count_read + 1 + count_write) * 4 + count_cycles);
		printf("\t}\n}\n");
		break;

	case i_ASR:
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "cnt", 1, 0,
			curi->dmode, "dstreg", curi->size, "data", 1, GF_RMW);
		//genamode (curi, curi->smode, "srcreg", curi->size, "cnt", 1, 0, 0);
		//genamode (curi, curi->dmode, "dstreg", curi->size, "data", 1, 0, GF_RMW);
		fill_prefetch_next();
		start_brace ();
		switch (curi->size) {
		case sz_byte: printf ("\tuae_u32 val = (uae_u8)data;\n"); break;
		case sz_word: printf ("\tuae_u32 val = (uae_u16)data;\n"); break;
		case sz_long: printf ("\tuae_u32 val = data;\n"); break;
		default: term ();
		}
		printf ("\tuae_u32 sign = (%s & val) >> %d;\n", cmask (curi->size), bit_size (curi->size) - 1);
		//printf ("\tint ccnt = cnt & 63;\n");
		printf ("\tcnt &= 63;\n");
		printf ("\tCLEAR_CZNV ();\n");
		printf ("\tif (cnt >= %d) {\n", bit_size (curi->size));
		printf ("\t\tval = %s & (uae_u32)-sign;\n", bit_mask (curi->size));
		printf ("\t\tSET_CFLG (sign);\n");
		duplicate_carry (1);
		if (source_is_imm1_8 (curi))
			printf ("\t} else {\n");
		else
			printf ("\t} else if (cnt > 0) {\n");
		printf ("\t\tval >>= cnt - 1;\n");
		printf ("\t\tSET_CFLG (val & 1);\n");
		duplicate_carry (1);
		printf ("\t\tval >>= 1;\n");
		printf ("\t\tval |= (%s << (%d - cnt)) & (uae_u32)-sign;\n",
			bit_mask (curi->size),
			bit_size (curi->size));
		printf ("\t\tval &= %s;\n", bit_mask (curi->size));
		printf ("\t}\n");
		genflags (flag_logical_noclobber, curi->size, "val", "", "");
		shift_ce (curi->dmode, curi->size);
		genastore ("val", curi->dmode, "dstreg", curi->size, "data");
		break;
	case i_ASL:
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "cnt", 1, 0,
			curi->dmode, "dstreg", curi->size, "data", 1, GF_RMW);
		//genamode (curi, curi->smode, "srcreg", curi->size, "cnt", 1, 0, 0);
		//genamode (curi, curi->dmode, "dstreg", curi->size, "data", 1, 0, GF_RMW);
		fill_prefetch_next();
		start_brace ();
		switch (curi->size) {
		case sz_byte: printf ("\tuae_u32 val = (uae_u8)data;\n"); break;
		case sz_word: printf ("\tuae_u32 val = (uae_u16)data;\n"); break;
		case sz_long: printf ("\tuae_u32 val = data;\n"); break;
		default: term ();
		}
		//printf ("\tint ccnt = cnt & 63;\n");
		printf ("\tcnt &= 63;\n");
		printf ("\tCLEAR_CZNV ();\n");
		printf ("\tif (cnt >= %d) {\n", bit_size (curi->size));
		printf ("\t\tSET_VFLG (val != 0);\n");
		printf ("\t\tSET_CFLG (cnt == %d ? val & 1 : 0);\n",
			bit_size (curi->size));
		duplicate_carry (1);
		printf ("\t\tval = 0;\n");
		if (source_is_imm1_8 (curi))
			printf ("\t} else {\n");
		else
			printf ("\t} else if (cnt > 0) {\n");
		printf ("\t\tuae_u32 mask = (%s << (%d - cnt)) & %s;\n",
			bit_mask (curi->size),
			bit_size (curi->size) - 1,
			bit_mask (curi->size));
		printf ("\t\tSET_VFLG ((val & mask) != mask && (val & mask) != 0);\n");
		printf ("\t\tval <<= cnt - 1;\n");
		printf ("\t\tSET_CFLG ((val & %s) >> %d);\n", cmask (curi->size), bit_size (curi->size) - 1);
		duplicate_carry (1);
		printf ("\t\tval <<= 1;\n");
		printf ("\t\tval &= %s;\n", bit_mask (curi->size));
		printf ("\t}\n");
		genflags (flag_logical_noclobber, curi->size, "val", "", "");
		shift_ce (curi->dmode, curi->size);
		genastore ("val", curi->dmode, "dstreg", curi->size, "data");
		break;
	case i_LSR:
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "cnt", 1, 0,
			curi->dmode, "dstreg", curi->size, "data", 1, GF_RMW);
		//genamode (curi, curi->smode, "srcreg", curi->size, "cnt", 1, 0, 0);
		//genamode (curi, curi->dmode, "dstreg", curi->size, "data", 1, 0, GF_RMW);
		fill_prefetch_next();
		start_brace ();
		switch (curi->size) {
		case sz_byte: printf ("\tuae_u32 val = (uae_u8)data;\n"); break;
		case sz_word: printf ("\tuae_u32 val = (uae_u16)data;\n"); break;
		case sz_long: printf ("\tuae_u32 val = data;\n"); break;
		default: term ();
		}
		//printf ("\tint ccnt = cnt & 63;\n");
		printf ("\tcnt &= 63;\n");
		printf ("\tCLEAR_CZNV ();\n");
		printf ("\tif (cnt >= %d) {\n", bit_size (curi->size));
		printf ("\t\tSET_CFLG ((cnt == %d) & (val >> %d));\n",
			bit_size (curi->size), bit_size (curi->size) - 1);
		duplicate_carry (1);
		printf ("\t\tval = 0;\n");
		if (source_is_imm1_8 (curi))
			printf ("\t} else {\n");
		else
			printf ("\t} else if (cnt > 0) {\n");
		printf ("\t\tval >>= cnt - 1;\n");
		printf ("\t\tSET_CFLG (val & 1);\n");
		duplicate_carry (1);
		printf ("\t\tval >>= 1;\n");
		printf ("\t}\n");
		genflags (flag_logical_noclobber, curi->size, "val", "", "");
		shift_ce (curi->dmode, curi->size);
		genastore ("val", curi->dmode, "dstreg", curi->size, "data");
		break;
	case i_LSL:
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "cnt", 1, 0,
			curi->dmode, "dstreg", curi->size, "data", 1, GF_RMW);
		//genamode (curi, curi->smode, "srcreg", curi->size, "cnt", 1, 0, 0);
		//genamode (curi, curi->dmode, "dstreg", curi->size, "data", 1, 0, GF_RMW);
		fill_prefetch_next();
		start_brace ();
		switch (curi->size) {
		case sz_byte: printf ("\tuae_u32 val = (uae_u8)data;\n"); break;
		case sz_word: printf ("\tuae_u32 val = (uae_u16)data;\n"); break;
		case sz_long: printf ("\tuae_u32 val = data;\n"); break;
		default: term ();
		}
		//printf ("\tint ccnt = cnt & 63;\n");
		printf ("\tcnt &= 63;\n");
		printf ("\tCLEAR_CZNV ();\n");
		printf ("\tif (cnt >= %d) {\n", bit_size (curi->size));
		printf ("\t\tSET_CFLG (cnt == %d ? val & 1 : 0);\n", bit_size (curi->size));
		duplicate_carry (1);
		printf ("\t\tval = 0;\n");
		if (source_is_imm1_8 (curi))
			printf ("\t} else {\n");
		else
			printf ("\t} else if (cnt > 0) {\n");
		printf ("\t\tval <<= (cnt - 1);\n");
		printf ("\t\tSET_CFLG ((val & %s) >> %d);\n", cmask (curi->size), bit_size (curi->size) - 1);
		duplicate_carry (1);
		printf ("\t\tval <<= 1;\n");
		printf ("\tval &= %s;\n", bit_mask (curi->size));
		printf ("\t}\n");
		genflags (flag_logical_noclobber, curi->size, "val", "", "");
		shift_ce (curi->dmode, curi->size);
		genastore ("val", curi->dmode, "dstreg", curi->size, "data");
		break;
	case i_ROL:
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "cnt", 1, 0,
			curi->dmode, "dstreg", curi->size, "data", 1, GF_RMW);
		//genamode (curi, curi->smode, "srcreg", curi->size, "cnt", 1, 0, 0);
		//genamode (curi, curi->dmode, "dstreg", curi->size, "data", 1, 0, GF_RMW);
		fill_prefetch_next ();
		start_brace ();
		switch (curi->size) {
		case sz_byte: printf ("\tuae_u32 val = (uae_u8)data;\n"); break;
		case sz_word: printf ("\tuae_u32 val = (uae_u16)data;\n"); break;
		case sz_long: printf ("\tuae_u32 val = data;\n"); break;
		default: term ();
		}
		//printf ("\tint ccnt = cnt & 63;\n");
		printf ("\tcnt &= 63;\n");
		printf ("\tCLEAR_CZNV ();\n");
		if (source_is_imm1_8 (curi))
			printf ("{");
		else
			printf ("\tif (cnt > 0) {\n");
		printf ("\tuae_u32 loval;\n");
		printf ("\tcnt &= %d;\n", bit_size (curi->size) - 1);
		printf ("\tloval = val >> (%d - cnt);\n", bit_size (curi->size));
		printf ("\tval <<= cnt;\n");
		printf ("\tval |= loval;\n");
		printf ("\tval &= %s;\n", bit_mask (curi->size));
		printf ("\tSET_CFLG (val & 1);\n");
		printf ("}\n");
		genflags (flag_logical_noclobber, curi->size, "val", "", "");
		shift_ce (curi->dmode, curi->size);
		genastore ("val", curi->dmode, "dstreg", curi->size, "data");
		break;
	case i_ROR:
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "cnt", 1, 0,
			curi->dmode, "dstreg", curi->size, "data", 1, GF_RMW);
		//genamode (curi, curi->smode, "srcreg", curi->size, "cnt", 1, 0, 0);
		//genamode (curi, curi->dmode, "dstreg", curi->size, "data", 1, 0, GF_RMW);
		fill_prefetch_next ();
		start_brace ();
		switch (curi->size) {
		case sz_byte: printf ("\tuae_u32 val = (uae_u8)data;\n"); break;
		case sz_word: printf ("\tuae_u32 val = (uae_u16)data;\n"); break;
		case sz_long: printf ("\tuae_u32 val = data;\n"); break;
		default: term ();
		}
		//printf ("\tint ccnt = cnt & 63;\n");
		printf ("\tcnt &= 63;\n");
		printf ("\tCLEAR_CZNV ();\n");
		if (source_is_imm1_8 (curi))
			printf ("{");
		else
			printf ("\tif (cnt > 0) {");
		printf ("\tuae_u32 hival;\n");
		printf ("\tcnt &= %d;\n", bit_size (curi->size) - 1);
		printf ("\thival = val << (%d - cnt);\n", bit_size (curi->size));
		printf ("\tval >>= cnt;\n");
		printf ("\tval |= hival;\n");
		printf ("\tval &= %s;\n", bit_mask (curi->size));
		printf ("\tSET_CFLG ((val & %s) >> %d);\n", cmask (curi->size), bit_size (curi->size) - 1);
		printf ("\t}\n");
		genflags (flag_logical_noclobber, curi->size, "val", "", "");
		shift_ce (curi->dmode, curi->size);
		genastore ("val", curi->dmode, "dstreg", curi->size, "data");
		break;
	case i_ROXL:
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "cnt", 1, 0,
			curi->dmode, "dstreg", curi->size, "data", 1, GF_RMW);
		//genamode (curi, curi->smode, "srcreg", curi->size, "cnt", 1, 0, 0);
		//genamode (curi, curi->dmode, "dstreg", curi->size, "data", 1, 0, GF_RMW);
		fill_prefetch_next ();
		start_brace ();
		switch (curi->size) {
		case sz_byte: printf ("\tuae_u32 val = (uae_u8)data;\n"); break;
		case sz_word: printf ("\tuae_u32 val = (uae_u16)data;\n"); break;
		case sz_long: printf ("\tuae_u32 val = data;\n"); break;
		default: term ();
		}
		//printf ("\tint ccnt = cnt & 63;\n");
		printf ("\tcnt &= 63;\n");
		printf ("\tCLEAR_CZNV ();\n");
		if (source_is_imm1_8 (curi))
			printf ("{");
		else {
			force_range_for_rox ("cnt", curi->size);
			printf ("\tif (cnt > 0) {\n");
		}
		printf ("\tcnt--;\n");
		printf ("\t{\n\tuae_u32 carry;\n");
		printf ("\tuae_u32 loval = val >> (%d - cnt);\n", bit_size (curi->size) - 1);
		printf ("\tcarry = loval & 1;\n");
		printf ("\tval = (((val << 1) | GET_XFLG ()) << cnt) | (loval >> 1);\n");
		printf ("\tSET_XFLG (carry);\n");
		printf ("\tval &= %s;\n", bit_mask (curi->size));
		printf ("\t} }\n");
		printf ("\tSET_CFLG (GET_XFLG ());\n");
		genflags (flag_logical_noclobber, curi->size, "val", "", "");
		shift_ce (curi->dmode, curi->size);
		genastore ("val", curi->dmode, "dstreg", curi->size, "data");
		break;
	case i_ROXR:
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "cnt", 1, 0,
			curi->dmode, "dstreg", curi->size, "data", 1, GF_RMW);
		//genamode (curi, curi->smode, "srcreg", curi->size, "cnt", 1, 0, 0);
		//genamode (curi, curi->dmode, "dstreg", curi->size, "data", 1, 0, GF_RMW);
		fill_prefetch_next ();
		start_brace ();
		switch (curi->size) {
		case sz_byte: printf ("\tuae_u32 val = (uae_u8)data;\n"); break;
		case sz_word: printf ("\tuae_u32 val = (uae_u16)data;\n"); break;
		case sz_long: printf ("\tuae_u32 val = data;\n"); break;
		default: term ();
		}
		//printf ("\tint ccnt = cnt & 63;\n");
		printf ("\tcnt &= 63;\n");
		printf ("\tCLEAR_CZNV ();\n");
		if (source_is_imm1_8 (curi))
			printf ("{");
		else {
			force_range_for_rox ("cnt", curi->size);
			printf ("\tif (cnt > 0) {\n");
		}
		printf ("\tcnt--;\n");
		printf ("\t{\n\tuae_u32 carry;\n");
		printf ("\tuae_u32 hival = (val << 1) | GET_XFLG ();\n");
		printf ("\thival <<= (%d - cnt);\n", bit_size (curi->size) - 1);
		printf ("\tval >>= cnt;\n");
		printf ("\tcarry = val & 1;\n");
		printf ("\tval >>= 1;\n");
		printf ("\tval |= hival;\n");
		printf ("\tSET_XFLG (carry);\n");
		printf ("\tval &= %s;\n", bit_mask (curi->size));
		printf ("\t} }\n");
		printf ("\tSET_CFLG (GET_XFLG ());\n");
		genflags (flag_logical_noclobber, curi->size, "val", "", "");
		shift_ce (curi->dmode, curi->size);
		genastore ("val", curi->dmode, "dstreg", curi->size, "data");
		break;
	case i_ASRW:
		genamode (curi, curi->smode, "srcreg", curi->size, "data", 1, 0, GF_RMW);
		fill_prefetch_next ();
		start_brace ();
		switch (curi->size) {
		case sz_byte: printf ("\tuae_u32 val = (uae_u8)data;\n"); break;
		case sz_word: printf ("\tuae_u32 val = (uae_u16)data;\n"); break;
		case sz_long: printf ("\tuae_u32 val = data;\n"); break;
		default: term ();
		}
		printf ("\tuae_u32 sign = %s & val;\n", cmask (curi->size));
		printf ("\tuae_u32 cflg = val & 1;\n");
		printf ("\tval = (val >> 1) | sign;\n");
		genflags (flag_logical, curi->size, "val", "", "");
		printf ("\tSET_CFLG (cflg);\n");
		duplicate_carry (0);
		genastore ("val", curi->smode, "srcreg", curi->size, "data");
		break;
	case i_ASLW:
		genamode (curi, curi->smode, "srcreg", curi->size, "data", 1, 0, GF_RMW);
		fill_prefetch_next ();
		start_brace ();
		switch (curi->size) {
		case sz_byte: printf ("\tuae_u32 val = (uae_u8)data;\n"); break;
		case sz_word: printf ("\tuae_u32 val = (uae_u16)data;\n"); break;
		case sz_long: printf ("\tuae_u32 val = data;\n"); break;
		default: term ();
		}
		printf ("\tuae_u32 sign = %s & val;\n", cmask (curi->size));
		printf ("\tuae_u32 sign2;\n");
		printf ("\tval <<= 1;\n");
		genflags (flag_logical, curi->size, "val", "", "");
		printf ("\tsign2 = %s & val;\n", cmask (curi->size));
		printf ("\tSET_CFLG (sign != 0);\n");
		duplicate_carry (0);
		printf ("\tSET_VFLG (GET_VFLG () | (sign2 != sign));\n");
		genastore ("val", curi->smode, "srcreg", curi->size, "data");
		break;
	case i_LSRW:
		genamode (curi, curi->smode, "srcreg", curi->size, "data", 1, 0, GF_RMW);
		fill_prefetch_next ();
		start_brace ();
		switch (curi->size) {
		case sz_byte: printf ("\tuae_u32 val = (uae_u8)data;\n"); break;
		case sz_word: printf ("\tuae_u32 val = (uae_u16)data;\n"); break;
		case sz_long: printf ("\tuae_u32 val = data;\n"); break;
		default: term ();
		}
		printf ("\tuae_u32 carry = val & 1;\n");
		printf ("\tval >>= 1;\n");
		genflags (flag_logical, curi->size, "val", "", "");
		printf ("\tSET_CFLG (carry);\n");
		duplicate_carry (0);
		genastore ("val", curi->smode, "srcreg", curi->size, "data");
		break;
	case i_LSLW:
		genamode (curi, curi->smode, "srcreg", curi->size, "data", 1, 0, GF_RMW);
		fill_prefetch_next ();
		start_brace ();
		switch (curi->size) {
		case sz_byte: printf ("\tuae_u8 val = data;\n"); break;
		case sz_word: printf ("\tuae_u16 val = data;\n"); break;
		case sz_long: printf ("\tuae_u32 val = data;\n"); break;
		default: term ();
		}
		printf ("\tuae_u32 carry = val & %s;\n", cmask (curi->size));
		printf ("\tval <<= 1;\n");
		genflags (flag_logical, curi->size, "val", "", "");
		printf ("\tSET_CFLG (carry >> %d);\n", bit_size (curi->size) - 1);
		duplicate_carry (0);
		genastore ("val", curi->smode, "srcreg", curi->size, "data");
		break;
	case i_ROLW:
		genamode (curi, curi->smode, "srcreg", curi->size, "data", 1, 0, GF_RMW);
		fill_prefetch_next ();
		start_brace ();
		switch (curi->size) {
		case sz_byte: printf ("\tuae_u8 val = data;\n"); break;
		case sz_word: printf ("\tuae_u16 val = data;\n"); break;
		case sz_long: printf ("\tuae_u32 val = data;\n"); break;
		default: term ();
		}
		printf ("\tuae_u32 carry = val & %s;\n", cmask (curi->size));
		printf ("\tval <<= 1;\n");
		printf ("\tif (carry)  val |= 1;\n");
		genflags (flag_logical, curi->size, "val", "", "");
		printf ("\tSET_CFLG (carry >> %d);\n", bit_size (curi->size) - 1);
		genastore ("val", curi->smode, "srcreg", curi->size, "data");
		break;
	case i_RORW:
		genamode (curi, curi->smode, "srcreg", curi->size, "data", 1, 0, GF_RMW);
		fill_prefetch_next ();
		start_brace ();
		switch (curi->size) {
		case sz_byte: printf ("\tuae_u8 val = data;\n"); break;
		case sz_word: printf ("\tuae_u16 val = data;\n"); break;
		case sz_long: printf ("\tuae_u32 val = data;\n"); break;
		default: term ();
		}
		printf ("\tuae_u32 carry = val & 1;\n");
		printf ("\tval >>= 1;\n");
		printf ("\tif (carry) val |= %s;\n", cmask (curi->size));
		genflags (flag_logical, curi->size, "val", "", "");
		printf ("\tSET_CFLG (carry);\n");
		genastore ("val", curi->smode, "srcreg", curi->size, "data");
		break;
	case i_ROXLW:
		genamode (curi, curi->smode, "srcreg", curi->size, "data", 1, 0, GF_RMW);
		fill_prefetch_next ();
		start_brace ();
		switch (curi->size) {
		case sz_byte: printf ("\tuae_u8 val = data;\n"); break;
		case sz_word: printf ("\tuae_u16 val = data;\n"); break;
		case sz_long: printf ("\tuae_u32 val = data;\n"); break;
		default: term ();
		}
		printf ("\tuae_u32 carry = val & %s;\n", cmask (curi->size));
		printf ("\tval <<= 1;\n");
		printf ("\tif (GET_XFLG ()) val |= 1;\n");
		genflags (flag_logical, curi->size, "val", "", "");
		printf ("\tSET_CFLG (carry >> %d);\n", bit_size (curi->size) - 1);
		duplicate_carry (0);
		genastore ("val", curi->smode, "srcreg", curi->size, "data");
		break;
	case i_ROXRW:
		genamode (curi, curi->smode, "srcreg", curi->size, "data", 1, 0, GF_RMW);
		fill_prefetch_next ();
		start_brace ();
		switch (curi->size) {
		case sz_byte: printf ("\tuae_u8 val = data;\n"); break;
		case sz_word: printf ("\tuae_u16 val = data;\n"); break;
		case sz_long: printf ("\tuae_u32 val = data;\n"); break;
		default: term ();
		}
		printf ("\tuae_u32 carry = val & 1;\n");
		printf ("\tval >>= 1;\n");
		printf ("\tif (GET_XFLG ()) val |= %s;\n", cmask (curi->size));
		genflags (flag_logical, curi->size, "val", "", "");
		printf ("\tSET_CFLG (carry);\n");
		duplicate_carry (0);
		genastore ("val", curi->smode, "srcreg", curi->size, "data");
		break;
	case i_MOVEC2:
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, 0);
		fill_prefetch_next ();
		start_brace ();
		printf ("\tint regno = (src >> 12) & 15;\n");
		printf ("\tuae_u32 *regp = regs.regs + regno;\n");
		printf ("\tif (! m68k_movec2(src & 0xFFF, regp)) goto %s;\n", endlabelstr);
		need_endlabel = 1;
		break;
	case i_MOVE2C:
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, 0);
		fill_prefetch_next ();
		start_brace ();
		printf ("\tint regno = (src >> 12) & 15;\n");
		printf ("\tuae_u32 *regp = regs.regs + regno;\n");
		printf ("\tif (! m68k_move2c(src & 0xFFF, regp)) goto %s;\n", endlabelstr);
		need_endlabel = 1;
		break;
	case i_CAS:
		{
			int old_brace_level;
			genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, GF_LRMW);
			genamode (curi, curi->dmode, "dstreg", curi->size, "dst", 1, 0, GF_LRMW);
			//if (cpu_level == 5 && curi->size > 0) {
			//	printf ("\tif ((dsta & %d) && currprefs.cpu_compatible && get_cpu_model () == 68060) {\n", curi->size == 1 ? 1 : 3);
			//	if (curi->dmode == Aipi || curi->dmode == Apdi)
			//		printf ("\t\tm68k_areg (regs, dstreg) %c= %d;\n", curi->dmode == Aipi ? '-' : '+', 1 << curi->size);
			//	sync_m68k_pc_noreset ();
			//	printf ("\t\top_unimpl (opcode);\n");
			//	printf ("\t\tgoto %s;\n", endlabelstr);
			//	printf ("\t}\n");
			//	need_endlabel = 1;
			//}
			fill_prefetch_0 ();
			start_brace ();
			printf ("\tint ru = (src >> 6) & 7;\n");
			printf ("\tint rc = src & 7;\n");
			genflags (flag_cmp, curi->size, "newv", "m68k_dreg (regs, rc)", "dst");
			sync_m68k_pc ();
			printf ("\tif (GET_ZFLG ())");
			old_brace_level = n_braces;
			start_brace ();
			printf ("\n\t");
			genastore_cas ("(m68k_dreg (regs, ru))", curi->dmode, "dstreg", curi->size, "dst");
			printf ("\t");
			pop_braces (old_brace_level);
			printf ("else");
			start_brace ();
			printf ("\n");
			get_prefetch_020 ();
			if (cpu_level >= 4) {
				// apparently 68040/060 needs to always write at the end of RMW cycle
				printf ("\t");
				genastore_cas ("dst", curi->dmode, "dstreg", curi->size, "dst");
			}
  			switch (curi->size) {
			    case sz_byte:
				printf ("\t\tm68k_dreg(regs, rc) = (m68k_dreg(regs, rc) & ~0xff) | (dst & 0xff);\n");
				break;
			    case sz_word:
				printf ("\t\tm68k_dreg(regs, rc) = (m68k_dreg(regs, rc) & ~0xffff) | (dst & 0xffff);\n");
				break;
	    		default:
				printf ("\t\tm68k_dreg(regs, rc) = dst;\n");
				break;
			}	
			pop_braces (old_brace_level);
		}
		break;
	case i_CAS2:
		genamode (curi, curi->smode, "srcreg", curi->size, "extra", 1, 0, GF_LRMW);
		printf ("\tuae_u32 rn1 = regs.regs[(extra >> 28) & 15];\n");
		printf ("\tuae_u32 rn2 = regs.regs[(extra >> 12) & 15];\n");
		if (curi->size == sz_word) {
			int old_brace_level = n_braces;
			printf ("\tuae_u16 dst1 = %s (rn1), dst2 = %s (rn2);\n", srcwlrmw, srcwlrmw);
			genflags (flag_cmp, curi->size, "newv", "m68k_dreg (regs, (extra >> 16) & 7)", "dst1");
			printf ("\tif (GET_ZFLG ()) {\n");
			genflags (flag_cmp, curi->size, "newv", "m68k_dreg (regs, extra & 7)", "dst2");
			printf ("\tif (GET_ZFLG ()) {\n");
			printf ("\t%s (rn1, m68k_dreg (regs, (extra >> 22) & 7));\n", dstwlrmw);
			printf ("\t%s (rn2, m68k_dreg (regs, (extra >> 6) & 7));\n", dstwlrmw);
			printf ("\t}}\n");
			pop_braces (old_brace_level);
			printf ("\tif (! GET_ZFLG ()) {\n");
			printf ("\tm68k_dreg (regs, (extra >> 0) & 7) = (m68k_dreg (regs, (extra >> 6) & 7) & ~0xffff) | (dst2 & 0xffff);\n");
			printf ("\tm68k_dreg (regs, (extra >> 16) & 7) = (m68k_dreg (regs, (extra >> 22) & 7) & ~0xffff) | (dst1 & 0xffff);\n");
			printf ("\t}\n");
		} else {
			int old_brace_level = n_braces;
			printf ("\tuae_u32 dst1 = %s (rn1), dst2 = %s (rn2);\n", srcllrmw, srcllrmw);
			genflags (flag_cmp, curi->size, "newv", "m68k_dreg (regs, (extra >> 16) & 7)", "dst1");
			printf ("\tif (GET_ZFLG ()) {\n");
			genflags (flag_cmp, curi->size, "newv", "m68k_dreg (regs, extra & 7)", "dst2");
			printf ("\tif (GET_ZFLG ()) {\n");
			printf ("\t%s (rn1, m68k_dreg (regs, (extra >> 22) & 7));\n", dstllrmw);
			printf ("\t%s (rn2, m68k_dreg (regs, (extra >> 6) & 7));\n", dstllrmw);
			printf ("\t}}\n");
			pop_braces (old_brace_level);
			printf ("\tif (! GET_ZFLG ()) {\n");
			printf ("\tm68k_dreg (regs, (extra >> 0) & 7) = dst2;\n");
			printf ("\tm68k_dreg (regs, (extra >> 16) & 7) = dst1;\n");
			printf ("\t}\n");
		}
		break;
	case i_MOVES: /* ignore DFC and SFC when using_mmu == false */
		{
			int old_brace_level;
			tail_ce020_done	= true;
			genamode (curi, curi->smode, "srcreg", curi->size, "extra", 1, 0, 0);
			printf ("\tif (extra & 0x800)\n");
			{
				int old_m68k_pc_offset = m68k_pc_offset;
				old_brace_level = n_braces;
				start_brace ();
				printf ("\tuae_u32 src = regs.regs[(extra >> 12) & 15];\n");
				genamode (curi, curi->dmode, "dstreg", curi->size, "dst", 2, 0, 0);
				genastore_fc ("src", curi->dmode, "dstreg", curi->size, "dst");
				pop_braces (old_brace_level);
				m68k_pc_offset = old_m68k_pc_offset;
			}
			printf ("else");
			{
				start_brace ();
				genamode (curi, curi->dmode, "dstreg", curi->size, "src", 1, 0, GF_FC);
				printf ("\tif (extra & 0x8000) {\n");
				switch (curi->size) {
				case sz_byte: printf ("\tm68k_areg (regs, (extra >> 12) & 7) = (uae_s32)(uae_s8)src;\n"); break;
				case sz_word: printf ("\tm68k_areg (regs, (extra >> 12) & 7) = (uae_s32)(uae_s16)src;\n"); break;
				case sz_long: printf ("\tm68k_areg (regs, (extra >> 12) & 7) = src;\n"); break;
				default: term ();
				}
				printf ("\t} else {\n");
				genastore ("src", Dreg, "(extra >> 12) & 7", curi->size, "");
				printf ("\t}\n");
				if (using_mmu == 68040)
					sync_m68k_pc ();
				pop_braces (old_brace_level);
			}
			tail_ce020_done	= false;
			returntail (false);
		}
		break;
	case i_BKPT:		/* only needed for hardware emulators */
		sync_m68k_pc ();
		printf ("\top_illg (opcode);\n");
		break;
	case i_CALLM:		/* not present in 68030 */
		sync_m68k_pc ();
		printf ("\top_illg (opcode);\n");
		break;
	case i_RTM:		/* not present in 68030 */
		sync_m68k_pc ();
		printf ("\top_illg (opcode);\n");
		break;
	case i_TRAPcc:
		if (curi->smode != am_unknown && curi->smode != am_illg)
			genamode (curi, curi->smode, "srcreg", curi->size, "dummy", 1, 0, 0);
		fill_prefetch_0 ();
		printf ("\tif (cctrue (regs.ccrflags, %d)) { Exception (7); goto %s; }\n", curi->cc, endlabelstr);
		need_endlabel = 1;
		break;
	case i_DIVL:
    genamode (curi, curi->smode, "srcreg", curi->size, "extra", 1, 0, 0);
    printf ("\tif (extra & 0x800) cyc = 12 * CYCLE_UNIT / 2;\n");
		genamode (curi, curi->dmode, "dstreg", curi->size, "dst", 1, 0, 0);
		sync_m68k_pc ();
    printf ("\tif (dst == 0) {\n");
    printf ("\t\tException (5);\n");
	  printf ("\t\treturn 4 * CYCLE_UNIT / 2;\n");
    printf ("\t}\n");
		printf ("\tm68k_divl(opcode, dst, extra);\n");
		break;
	case i_MULL:
		genamode (curi, curi->smode, "srcreg", curi->size, "extra", 1, 0, 0);
		genamode (curi, curi->dmode, "dstreg", curi->size, "dst", 1, 0, 0);
		sync_m68k_pc ();
		printf ("\tm68k_mull(opcode, dst, extra);\n");
		break;
	case i_BFTST:
	case i_BFEXTU:
	case i_BFCHG:
	case i_BFEXTS:
	case i_BFCLR:
	case i_BFFFO:
	case i_BFSET:
	case i_BFINS:
		{
			char *getb, *putb;

			if (using_mmu == 68060 && (curi->mnemo == i_BFCHG || curi->mnemo == i_BFCLR ||  curi->mnemo == i_BFSET ||  curi->mnemo == i_BFINS)) {
				getb = "mmu060_get_rmw_bitfield";
				putb = "mmu060_put_rmw_bitfield";
			} else if (using_mmu || using_ce020) {
				getb = "x_get_bitfield";
				putb = "x_put_bitfield";
			} else {
				getb = "get_bitfield";
				putb = "put_bitfield";
			}

			genamode (curi, curi->smode, "srcreg", curi->size, "extra", 1, 0, 0);
			genamode (curi, curi->dmode, "dstreg", sz_long, "dst", 2, 0, 0);
			start_brace ();
			printf ("\tuae_u32 bdata[2];\n");
			printf ("\tuae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;\n");
			printf ("\tint width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;\n");
			if (curi->mnemo == i_BFFFO)
				printf("\tuae_u32 offset2 = offset;\n");
			if (curi->dmode == Dreg) {
				printf ("\tuae_u32 tmp = m68k_dreg(regs, dstreg);\n");
				printf ("\toffset &= 0x1f;\n");
				printf ("\ttmp = (tmp << offset) | (tmp >> (32 - offset));\n");
				printf ("\tbdata[0] = tmp & ((1 << (32 - width)) - 1);\n");
			} else {
				printf ("\tuae_u32 tmp;\n");
				printf ("\tdsta += offset >> 3;\n");
				printf ("\ttmp = %s (dsta, bdata, offset, width);\n", getb);
			}
			printf ("\tSET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);\n");
			if (curi->mnemo == i_BFEXTS)
				printf ("\ttmp = (uae_s32)tmp >> (32 - width);\n");
			else
				printf ("\ttmp >>= (32 - width);\n");
			printf ("\tSET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);\n");
			switch (curi->mnemo) {
			case i_BFTST:
				break;
			case i_BFEXTU:
			case i_BFEXTS:
				printf ("\tm68k_dreg (regs, (extra >> 12) & 7) = tmp;\n");
				break;
			case i_BFCHG:
				printf ("\ttmp = tmp ^ (0xffffffffu >> (32 - width));\n");
				break;
			case i_BFCLR:
				printf ("\ttmp = 0;\n");
				break;
			case i_BFFFO:
				printf ("\t{ uae_u32 mask = 1 << (width - 1);\n");
				printf ("\twhile (mask) { if (tmp & mask) break; mask >>= 1; offset2++; }}\n");
				printf ("\tm68k_dreg (regs, (extra >> 12) & 7) = offset2;\n");
				break;
			case i_BFSET:
				printf ("\ttmp = 0xffffffffu >> (32 - width);\n");
				break;
			case i_BFINS:
				printf ("\ttmp = m68k_dreg (regs, (extra >> 12) & 7);\n");
				printf ("\ttmp = tmp & (0xffffffffu >> (32 - width));\n");
				printf ("\tSET_NFLG (tmp & (1 << (width - 1)) ? 1 : 0);\n");
				printf ("\tSET_ZFLG (tmp == 0);\n");
				break;
			default:
				break;
			}
			if (curi->mnemo == i_BFCHG
				|| curi->mnemo == i_BFCLR
				|| curi->mnemo == i_BFSET
				|| curi->mnemo == i_BFINS) {
					if (curi->dmode == Dreg) {
						printf ("\ttmp = bdata[0] | (tmp << (32 - width));\n");
						printf ("\tm68k_dreg(regs, dstreg) = (tmp >> offset) | (tmp << (32 - offset));\n");
					} else {
						printf ("\t%s(dsta, bdata, tmp, offset, width);\n", putb);
					}
			}
		}
		break;
	case i_PACK:
		if (curi->smode == Dreg) {
			printf ("\tuae_u16 val = m68k_dreg (regs, srcreg) + %s;\n", gen_nextiword (0));
			printf ("\tm68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & 0xffffff00) | ((val >> 4) & 0xf0) | (val & 0xf);\n");
		} else {
			printf ("\tuae_u16 val;\n");
			addmmufixup ("srcreg");
			printf ("\tm68k_areg (regs, srcreg) -= areg_byteinc[srcreg];\n");
			printf ("\tval = (uae_u16)(%s (m68k_areg (regs, srcreg)) & 0xff);\n", srcb);
			printf ("\tm68k_areg (regs, srcreg) -= areg_byteinc[srcreg];\n");
			printf ("\tval = (val | ((uae_u16)(%s (m68k_areg (regs, srcreg)) & 0xff) << 8)) + %s;\n", srcb, gen_nextiword (0));
			addmmufixup ("dstreg");
			printf ("\tm68k_areg (regs, dstreg) -= areg_byteinc[dstreg];\n");
			gen_set_fault_pc ();
			printf ("\t%s (m68k_areg (regs, dstreg),((val >> 4) & 0xf0) | (val & 0xf));\n", dstb);
		}
		break;
	case i_UNPK:
		if (curi->smode == Dreg) {
			printf ("\tuae_u16 val = m68k_dreg (regs, srcreg);\n");
			printf ("\tval = (((val << 4) & 0xf00) | (val & 0xf)) + %s;\n", gen_nextiword (0));
			printf ("\tm68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & 0xffff0000) | (val & 0xffff);\n");
		} else {
			printf ("\tuae_u16 val;\n");
			addmmufixup ("srcreg");
			printf ("\tm68k_areg (regs, srcreg) -= areg_byteinc[srcreg];\n");
			printf ("\tval = (uae_u16)(%s (m68k_areg (regs, srcreg)) & 0xff);\n", srcb);
			printf ("\tval = (((val << 4) & 0xf00) | (val & 0xf)) + %s;\n", gen_nextiword (0));
			addmmufixup ("dstreg");
			if (cpu_level >= 2) {
				printf ("\tm68k_areg (regs, dstreg) -= 2 * areg_byteinc[dstreg];\n");
				printf ("\t%s (m68k_areg (regs, dstreg) + areg_byteinc[dstreg], val);\n", dstb);
				printf ("\t%s (m68k_areg (regs, dstreg), val >> 8);\n", dstb);
			} else {
				printf ("\tm68k_areg (regs, dstreg) -= areg_byteinc[dstreg];\n");
				printf ("\t%s (m68k_areg (regs, dstreg),val);\n", dstb);
				printf ("\tm68k_areg (regs, dstreg) -= areg_byteinc[dstreg];\n");
				gen_set_fault_pc ();
				printf ("\t%s (m68k_areg (regs, dstreg),val >> 8);\n", dstb);
			}
		}
		break;
	case i_TAS:
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, GF_LRMW);
		genflags (flag_logical, curi->size, "src", "", "");
		if (!isreg (curi->smode))
			addcycles000 (6);
		fill_prefetch_next ();
		printf ("\tsrc |= 0x80;\n");
		if (cpu_level >= 2 || curi->smode == Dreg || !using_ce) {
			if (next_cpu_level < 2)
				next_cpu_level = 2 - 1;
			genastore_tas ("src", curi->smode, "srcreg", curi->size, "src");
		} else {
			printf ("\tif (!is_cycle_ce ()) {\n");
			genastore ("src", curi->smode, "srcreg", curi->size, "src");
			printf ("\t} else {\n");
			printf ("\t\t%s (4);\n", do_cycles);
			addcycles000_nonce("\t\t", 4);
			printf ("\t}\n");
		}
		break;
	case i_FPP:
		fpulimit();
		genamode (curi, curi->smode, "srcreg", curi->size, "extra", 1, 0, 0);
		sync_m68k_pc ();
		printf ("\tfpuop_arithmetic(opcode, extra);\n");
		break;
	case i_FDBcc:
		fpulimit();
		genamode (curi, curi->smode, "srcreg", curi->size, "extra", 1, 0, 0);
		sync_m68k_pc ();
		printf ("\tfpuop_dbcc (opcode, extra);\n");
		break;
	case i_FScc:
		fpulimit();
		genamode (curi, curi->smode, "srcreg", curi->size, "extra", 1, 0, 0);
		sync_m68k_pc ();
		printf ("\tfpuop_scc (opcode, extra);\n");
		break;
	case i_FTRAPcc:
		fpulimit();
		printf ("\tuaecptr oldpc = %s;\n", getpc);
		printf ("\tuae_u16 extra = %s;\n", gen_nextiword (0));
		if (curi->smode != am_unknown && curi->smode != am_illg)
			genamode (curi, curi->smode, "srcreg", curi->size, "dummy", 1, 0, 0);
		sync_m68k_pc ();
		printf ("\tfpuop_trapcc (opcode, oldpc, extra);\n");
		break;
	case i_FBcc:
		fpulimit();
		sync_m68k_pc ();
		start_brace ();
		printf ("\tuaecptr pc = %s;\n", getpc);
		genamode (curi, curi->dmode, "srcreg", curi->size, "extra", 1, 0, 0);
		sync_m68k_pc ();
		printf ("\tfpuop_bcc (opcode, pc,extra);\n");
		break;
	case i_FSAVE:
		fpulimit();
		sync_m68k_pc ();
		printf ("\tfpuop_save (opcode);\n");
		break;
	case i_FRESTORE:
		fpulimit();
		sync_m68k_pc ();
		printf ("\tfpuop_restore (opcode);\n");
		break;

	case i_CINVL:
	case i_CINVP:
	case i_CINVA:
	case i_CPUSHL:
	case i_CPUSHP:
	case i_CPUSHA:
		if (using_mmu)
			printf ("\tflush_mmu%s(m68k_areg (regs, opcode & 3), (opcode >> 6) & 3);\n", mmu_postfix);
		printf ("\tif (opcode & 0x80)\n");
		printf ("\t\tflush_icache(m68k_areg (regs, opcode & 3), (opcode >> 6) & 3);\n");
		break;

	case i_MOVE16:
		{
			if ((opcode & 0xfff8) == 0xf620) {
				/* MOVE16 (Ax)+,(Ay)+ */
				printf ("\tuae_u32 v[4];\n");
				printf ("\tuaecptr mems = m68k_areg (regs, srcreg) & ~15, memd;\n");
				printf ("\tdstreg = (%s >> 12) & 7;\n", gen_nextiword (0));
				printf ("\tmemd = m68k_areg (regs, dstreg) & ~15;\n");
				if (using_mmu >= 68040) {
					printf ("\tget_move16_mmu (mems, v);\n");
					printf ("\tput_move16_mmu (memd, v);\n");
				} else {
					printf ("\tv[0] = %s (mems);\n", srcl);
					printf ("\tv[1] = %s (mems + 4);\n", srcl);
					printf ("\tv[2] = %s (mems + 8);\n", srcl);
					printf ("\tv[3] = %s (mems + 12);\n", srcl);
					printf ("\t%s (memd , v[0]);\n", dstl);
					printf ("\t%s (memd + 4, v[1]);\n", dstl);
					printf ("\t%s (memd + 8, v[2]);\n", dstl);
					printf ("\t%s (memd + 12, v[3]);\n", dstl);
				}
				printf ("\tif (srcreg != dstreg)\n");
				printf ("\t\tm68k_areg (regs, srcreg) += 16;\n");
				printf ("\tm68k_areg (regs, dstreg) += 16;\n");
			} else {
				/* Other variants */
				printf ("\tuae_u32 v[4];\n");
				genamode (curi, curi->smode, "srcreg", curi->size, "mems", 0, 2, 0);
				genamode (curi, curi->dmode, "dstreg", curi->size, "memd", 0, 2, 0);
				if (using_mmu == 68040) {
					printf ("\tget_move16_mmu (memsa, mmu040_move16);\n");
					printf ("\tput_move16_mmu (memda, mmu040_move16);\n");
				} else if (using_mmu == 68060) {
					printf ("\tget_move16_mmu (memsa, v);\n");
					printf ("\tput_move16_mmu (memda, v);\n");
				} else {
					printf ("\tmemsa &= ~15;\n");
					printf ("\tmemda &= ~15;\n");
					printf ("\tv[0] = %s (memsa);\n", srcl);
					printf ("\tv[1] = %s (memsa + 4);\n", srcl);
					printf ("\tv[2] = %s (memsa + 8);\n", srcl);
					printf ("\tv[3] = %s (memsa + 12);\n", srcl);
					printf ("\t%s (memda , v[0]);\n", dstl);
					printf ("\t%s (memda + 4, v[1]);\n", dstl);
					printf ("\t%s (memda + 8, v[2]);\n", dstl);
					printf ("\t%s (memda + 12, v[3]);\n", dstl);
				}
				if ((opcode & 0xfff8) == 0xf600)
					printf ("\tm68k_areg (regs, srcreg) += 16;\n");
				else if ((opcode & 0xfff8) == 0xf608)
					printf ("\tm68k_areg (regs, dstreg) += 16;\n");
			}
		}
		break;

	case i_PFLUSHN:
	case i_PFLUSH:
	case i_PFLUSHAN:
	case i_PFLUSHA:
	case i_PLPAR:
	case i_PLPAW:
	case i_PTESTR:
	case i_PTESTW:
		sync_m68k_pc ();
		printf ("\tmmu_op (opcode, 0);\n");
		break;
	case i_MMUOP030:
		printf ("\tuaecptr pc = %s;\n", getpc);
		printf ("\tuae_u16 extra = %s (2);\n", prefetch_word);
		m68k_pc_offset += 2;
		sync_m68k_pc ();
		if (curi->smode == Areg || curi->smode == Dreg)
			printf("\tuae_u16 extraa = 0;\n");
		else
			genamode (curi, curi->smode, "srcreg", curi->size, "extra", 0, 0, 0);
		sync_m68k_pc ();
		printf ("\tmmu_op30 (pc, opcode, extra, extraa);\n");
		break;
	default:
		term ();
		break;
	}
	if (!genastore_done)
		returntail (0);
	finish_braces ();
	if (limit_braces) {
		printf ("\n#endif\n");
		n_braces = limit_braces;
		limit_braces = 0;
		finish_braces ();
	}
	if (did_prefetch >= 0)
		fill_prefetch_finish ();
	sync_m68k_pc ();
	did_prefetch = 0;
	ipl_fetched = 0;
}

static void generate_includes (FILE * f, int id)
{
	fprintf (f, "#include \"sysconfig.h\"\n");
	fprintf (f, "#include \"sysdeps.h\"\n");
	fprintf (f, "#include \"options.h\"\n");
	fprintf (f, "#include \"memory.h\"\n");
	fprintf (f, "#include \"custom.h\"\n");
	fprintf (f, "#include \"newcpu.h\"\n");
	fprintf (f, "#include \"cpu_prefetch.h\"\n");
	fprintf (f, "#include \"cputbl.h\"\n");
	if (id == 31 || id == 33)
		fprintf (f, "#include \"cpummu.h\"\n");
	else if (id == 32)
		fprintf (f, "#include \"cpummu030.h\"\n");

	fprintf (f, "#define CPUFUNC(x) x##_ff\n"
		"#define SET_CFLG_ALWAYS(x) SET_CFLG(x)\n"
		"#define SET_NFLG_ALWAYS(x) SET_NFLG(x)\n"
		"#ifdef NOFLAGS\n"
		"#include \"noflags.h\"\n"
		"#endif\n");
}

static int postfix;


static char *decodeEA (amodes mode, wordsizes size)
{
	static char buffer[80];

	buffer[0] = 0;
	switch (mode){
	case Dreg:
		strcpy (buffer,"Dn");
		break;
	case Areg:
		strcpy (buffer,"An");
		break;
	case Aind:
		strcpy (buffer,"(An)");
		break;
	case Aipi:
		strcpy (buffer,"(An)+");
		break;
	case Apdi:
		strcpy (buffer,"-(An)");
		break;
	case Ad16:
		strcpy (buffer,"(d16,An)");
		break;
	case Ad8r:
		strcpy (buffer,"(d8,An,Xn)");
		break;
	case PC16:
		strcpy (buffer,"(d16,PC)");
		break;
	case PC8r:
		strcpy (buffer,"(d8,PC,Xn)");
		break;
	case absw:
		strcpy (buffer,"(xxx).W");
		break;
	case absl:
		strcpy (buffer,"(xxx).L");
		break;
	case imm:
		switch (size){
		case sz_byte:
			strcpy (buffer,"#<data>.B");
			break;
		case sz_word:
			strcpy (buffer,"#<data>.W");
			break;
		case sz_long:
			strcpy (buffer,"#<data>.L");
			break;
		default:
			break;
		}
		break;
	case imm0:
		strcpy (buffer,"#<data>.B");
		break;
	case imm1:
		strcpy (buffer,"#<data>.W");
		break;
	case imm2:
		strcpy (buffer,"#<data>.L");
		break;
	case immi:
		strcpy (buffer,"#<data>");
		break;

	default:
		break;
	}
	return buffer;
}

static const char *m68k_cc[] = {
	"T",
	"F",
	"HI",
	"LS",
	"CC",
	"CS",
	"NE",
	"EQ",
	"VC",
	"VS",
	"PL",
	"MI",
	"GE",
	"LT",
	"GT",
	"LE"
};

static char *outopcode (int opcode)
{
	static char out[100];
	struct instr *ins;
	int i;

	ins = &table68k[opcode];
	for (i = 0; lookuptab[i].name[0]; i++) {
		if (ins->mnemo == lookuptab[i].mnemo)
			break;
	}
	{
		char *s = ua (lookuptab[i].name);
		strcpy (out, s);
		xfree (s);
	}
	if (ins->smode == immi)
		strcat (out, "Q");
	if (ins->size == sz_byte)
		strcat (out,".B");
	if (ins->size == sz_word)
		strcat (out,".W");
	if (ins->size == sz_long)
		strcat (out,".L");
	strcat (out," ");
	if (ins->suse)
		strcat (out, decodeEA (ins->smode, ins->size));
	if (ins->duse) {
		if (ins->suse) strcat (out,",");
		strcat (out, decodeEA (ins->dmode, ins->size));
	}
	if (ins->mnemo == i_DBcc || ins->mnemo == i_Scc || ins->mnemo == i_Bcc || ins->mnemo == i_TRAPcc) {
		strcat (out, " (");
		strcat (out, m68k_cc[table68k[opcode].cc]);
		strcat (out, ")");
	}

	return out;
}

struct cputbl_tmp
{
	uae_s16 length;
	uae_s8 disp020[2];
	uae_u8 branch;
};
static struct cputbl_tmp cputbltmp[65536];

static int count_required(int opcode)
{
  struct instr *curi = table68k + opcode;

  switch(curi->mnemo) {
    case i_MVMLE:
    case i_MVMEL:
      return 1;
  	case i_ASR:
  	case i_ASL:
  	case i_LSR:
  	case i_LSL:
    case i_ROXR:
    case i_ROR:
    case i_ROXL:
    case i_ROL:
    case i_MULU:
    case i_MULS:
    case i_DIVU:
    case i_DIVS:
    case i_DBcc:
      if(cpu_level <= 1)
      	return 1;
      break;
    case i_Scc:
      if(curi->smode == Dreg && cpu_level <= 1)
        return 1;
      break;
  }
  return 0;
}

static void generate_one_opcode (int rp, char *extra)
{
	int idx;
	uae_u16 smsk, dmsk;
	long int opcode = opcode_map[rp];
	int i68000 = table68k[opcode].clev > 0;

	if (table68k[opcode].mnemo == i_ILLG
		|| table68k[opcode].clev > cpu_level)
		return;

	for (idx = 0; lookuptab[idx].name[0]; idx++) {
		if (table68k[opcode].mnemo == lookuptab[idx].mnemo)
			break;
	}

	if (table68k[opcode].handler != -1)
		return;

	if (opcode_next_clev[rp] != cpu_level) {
		char *name = ua (lookuptab[idx].name);
		if (generate_stbl)
			fprintf (stblfile, "{ %sCPUFUNC(op_%04x_%d%s), %d }, /* %s */\n",
			(using_ce || using_ce020) ? "(cpuop_func*)" : "",
			opcode, opcode_last_postfix[rp],
			extra, opcode, name);
		xfree (name);
		return;
	}
	fprintf (headerfile, "extern %s op_%04lx_%d%s_nf;\n",
		(using_ce || using_ce020) ? "cpuop_func_ce" : "cpuop_func", opcode, postfix, extra);
	fprintf (headerfile, "extern %s op_%04lx_%d%s_ff;\n",
		(using_ce || using_ce020) ? "cpuop_func_ce" : "cpuop_func", opcode, postfix, extra);
	printf ("/* %s */\n", outopcode (opcode));
//	if (i68000)
//		printf("#ifndef CPUEMU_68000_ONLY\n");
	printf ("%s REGPARAM2 CPUFUNC(op_%04lx_%d%s)(uae_u32 opcode)\n{\n", (using_ce || using_ce020) ? "void" : "uae_u32", opcode, postfix, extra);
  int org_using_simple_cycles = using_simple_cycles;
  if(count_required(opcode))
    using_simple_cycles = 1;
	if (using_simple_cycles)
		printf("\tint count_cycles = 0;\n");

	switch (table68k[opcode].stype) {
	case 0: smsk = 7; break;
	case 1: smsk = 255; break;
	case 2: smsk = 15; break;
	case 3: smsk = 7; break;
	case 4: smsk = 7; break;
	case 5: smsk = 63; break;
  case 6: smsk = 255; break;
	case 7: smsk = 3; break;
	default: term ();
	}
	dmsk = 7;

	next_cpu_level = -1;
	if (table68k[opcode].suse
		&& table68k[opcode].smode != imm && table68k[opcode].smode != imm0
		&& table68k[opcode].smode != imm1 && table68k[opcode].smode != imm2
		&& table68k[opcode].smode != absw && table68k[opcode].smode != absl
		&& table68k[opcode].smode != PC8r && table68k[opcode].smode != PC16
	/* gb-- We don't want to fetch the EmulOp code since the EmulOp()
	   routine uses the whole opcode value. Maybe all the EmulOps
	   could be expanded out but I don't think it is an improvement */
	&& table68k[opcode].stype != 6
	)
	{
		if (table68k[opcode].spos == -1) {
			if (((int) table68k[opcode].sreg) >= 128)
				printf ("\tuae_u32 srcreg = (uae_s32)(uae_s8)%d;\n", (int) table68k[opcode].sreg);
			else
				printf ("\tuae_u32 srcreg = %d;\n", (int) table68k[opcode].sreg);
		} else {
			char source[100];
			int pos = table68k[opcode].spos;

			if (pos)
				sprintf (source, "((opcode >> %d) & %d)", pos, smsk);
			else
				sprintf (source, "(opcode & %d)", smsk);

			if (table68k[opcode].stype == 3)
				printf ("\tuae_u32 srcreg = imm8_table[%s];\n", source);
			else if (table68k[opcode].stype == 1)
				printf ("\tuae_u32 srcreg = (uae_s32)(uae_s8)%s;\n", source);
			else
				printf ("\tuae_u32 srcreg = %s;\n", source);
		}
	}
	if (table68k[opcode].duse
		/* Yes, the dmode can be imm, in case of LINK or DBcc */
		&& table68k[opcode].dmode != imm && table68k[opcode].dmode != imm0
		&& table68k[opcode].dmode != imm1 && table68k[opcode].dmode != imm2
		&& table68k[opcode].dmode != absw && table68k[opcode].dmode != absl)
	{
		if (table68k[opcode].dpos == -1) {
			if (((int) table68k[opcode].dreg) >= 128)
				printf ("\tuae_u32 dstreg = (uae_s32)(uae_s8)%d;\n", (int) table68k[opcode].dreg);
			else
				printf ("\tuae_u32 dstreg = %d;\n", (int) table68k[opcode].dreg);
		} else {
			int pos = table68k[opcode].dpos;
			if (pos)
				printf ("\tuae_u32 dstreg = (opcode >> %d) & %d;\n",
				pos, dmsk);
			else
				printf ("\tuae_u32 dstreg = opcode & %d;\n", dmsk);
		}
	}
	need_endlabel = 0;
	endlabelno++;
	sprintf (endlabelstr, "l_%d", endlabelno);
	count_read = count_write = count_ncycles = count_cycles = 0;
	count_cycles_ce020 = 0;
	count_read_ea = count_write_ea = count_cycles_ea = 0;
	gen_opcode (opcode);
	if (need_endlabel)
		printf ("%s: ;\n", endlabelstr);
	clearmmufixup (0);
	clearmmufixup (1);
	if (using_ce || using_prefetch) {
		if (count_read + count_write + count_cycles == 0)
			count_cycles = 4;
		returncycles ("", (count_read + count_write) * 4 + count_cycles);
		printf ("}");
		printf (" /* %d%s (%d/%d)",
			(count_read + count_write) * 4 + count_cycles, count_ncycles ? "+" : "", count_read, count_write);
		printf (" */\n");
	} else if (count_read + count_write + count_cycles) {
		returncycles ("", (count_read + count_write) * 4 + count_cycles + 4);
		printf ("}");
		printf("\n");
	} else {
		returncycles ("", insn_n_cycles);
		printf ("}");
		printf("\n");
	}
	printf ("\n");
//	if (i68000)
//		printf("#endif\n");
	using_simple_cycles = org_using_simple_cycles;

	opcode_next_clev[rp] = next_cpu_level;
	opcode_last_postfix[rp] = postfix;

	if (generate_stbl) {
		char *name = ua (lookuptab[idx].name);
//		if (i68000)
//			fprintf (stblfile, "#ifndef CPUEMU_68000_ONLY\n");
		fprintf (stblfile, "{ %sCPUFUNC(op_%04x_%d%s), %d }, /* %s */\n",
			(using_ce || using_ce020) ? "(cpuop_func*)" : "",
			opcode, postfix, extra, opcode, name);
//		if (i68000)
//			fprintf (stblfile, "#endif\n");
		xfree (name);
	}
}

static void generate_func (char *extra)
{
	int j, rp;

	/* sam: this is for people with low memory (eg. me :)) */
	printf ("\n"
		"#if !defined(PART_1) && !defined(PART_2) && "
		"!defined(PART_3) && !defined(PART_4) && "
		"!defined(PART_5) && !defined(PART_6) && "
		"!defined(PART_7) && !defined(PART_8)"
		"\n"
		"#define PART_1 1\n"
		"#define PART_2 1\n"
		"#define PART_3 1\n"
		"#define PART_4 1\n"
		"#define PART_5 1\n"
		"#define PART_6 1\n"
		"#define PART_7 1\n"
		"#define PART_8 1\n"
		"#endif\n\n");

	rp = 0;
	for(j = 1; j <= 8; ++j) {
		int k = (j * nr_cpuop_funcs) / 8;
		printf ("#ifdef PART_%d\n",j);
		for (; rp < k; rp++)
			generate_one_opcode (rp, extra);
		printf ("#endif\n\n");
	}

	if (generate_stbl)
		fprintf (stblfile, "{ 0, 0 }};\n");
}

static void generate_cpu (int id, int mode)
{
	char fname[100];
	char *extra, *extraup;
	static int postfix2 = -1;
	int rp;

	using_tracer = mode;
	extra = "";
	extraup = "";
	if (using_tracer) {
		extra = "_t";
		extraup = "_T";
	}

	postfix = id;
	if (id == 0 || id == 4 || id == 11 || id == 13 || id == 20 || id == 21 || id == 22 || id == 31 || id == 32 || id == 33) {
		if (generate_stbl && id != 4)
			fprintf (stblfile, "#ifdef CPUEMU_%d%s\n", postfix, extraup);
		postfix2 = postfix;
		sprintf (fname, "cpuemu_%d%s.cpp", postfix, extra);
		freopen (fname, "wb", stdout);
		generate_includes (stdout, id);
	}

	using_exception_3 = 1;
	using_prefetch = 0;
	using_prefetch_020 = 0;
	using_ce = 0;
	using_ce020 = 0;
	using_mmu = 0;
	using_waitstates = 0;
	memory_cycle_cnt = 4;
	mmu_postfix = "";
	using_simple_cycles = 0;
	using_indirect = 0;

	if (id == 4) {
		cpu_level = 1;
		for (rp = 0; rp < nr_cpuop_funcs; rp++) {
      opcode_next_clev[rp] = cpu_level;
		}
	} else if (id == 11 || id == 12) { // 11 = 68010 prefetch, 12 = 68000 prefetch
		cpu_level = id == 11 ? 1 : 0;
		using_prefetch = 1;
		using_exception_3 = 1;
		using_simple_cycles = 1;
		if (id == 11) {
			read_counts ();
			for (rp = 0; rp < nr_cpuop_funcs; rp++)
				opcode_next_clev[rp] = cpu_level;
		}
	} else if (id == 13 || id == 14) { // 13 = 68010 cycle-exact, 14 = 68000 cycle-exact
		cpu_level = id == 13 ? 1 : 0;
		using_prefetch = 1;
		using_exception_3 = 1;
		using_ce = 1;
		if (id == 13) {
			read_counts ();
			for (rp = 0; rp < nr_cpuop_funcs; rp++)
				opcode_next_clev[rp] = cpu_level;
		}
	} else if (id == 20) { // 68020 prefetch
		cpu_level = 2;
		using_prefetch_020 = 2;
		read_counts ();
		for (rp = 0; rp < nr_cpuop_funcs; rp++)
			opcode_next_clev[rp] = cpu_level;
	} else if (id == 21) { // 68020 cycle-exact
		cpu_level = 2;
		using_ce020 = 1;
		using_prefetch_020 = 2;
		// timing tables are from 030 which has 2
		// clock memory accesses, 68020 has 3 clock
		// memory accesses
		using_waitstates = 1;
		memory_cycle_cnt = 3;
		read_counts ();
		for (rp = 0; rp < nr_cpuop_funcs; rp++)
			opcode_next_clev[rp] = cpu_level;
	} else if (id == 22 || id == 23 || id == 24) { // 68030/040/60 "cycle-exact"
		cpu_level = 3 + (24 - id);
		using_ce020 = 2;
		using_prefetch_020 = 2;
		memory_cycle_cnt = 2;
		if (id == 22) {
			read_counts ();
			for (rp = 0; rp < nr_cpuop_funcs; rp++)
				opcode_next_clev[rp] = cpu_level;
		}
	} else if (id == 31) { // 31 = 68040 MMU
		mmu_postfix = "040";
		cpu_level = 4;
		using_mmu = 68040;
		read_counts ();
		for (rp = 0; rp < nr_cpuop_funcs; rp++)
			opcode_next_clev[rp] = cpu_level;
	} else if (id == 32) { // 32 = 68030 MMU
		mmu_postfix = "030";
		cpu_level = 3;
		using_mmu = 68030;
		read_counts ();
		for (rp = 0; rp < nr_cpuop_funcs; rp++)
			opcode_next_clev[rp] = cpu_level;
	} else if (id == 33) { // 33 = 68060 MMU
		mmu_postfix = "060";
		cpu_level = 5;
		using_mmu = 68060;
			read_counts ();
		for (rp = 0; rp < nr_cpuop_funcs; rp++)
			opcode_next_clev[rp] = cpu_level;
	} else {
		cpu_level = 5 - id; // "generic"
	}
 
	if (!using_indirect)
		using_indirect = using_ce || using_ce020 || using_prefetch_020;

	if (generate_stbl) {
//		if ((id > 0 && id < 10) || (id >= 20))
//			fprintf (stblfile, "#ifndef CPUEMU_68000_ONLY\n");
		fprintf (stblfile, "const struct cputbl CPUFUNC(op_smalltbl_%d%s)[] = {\n", postfix, extra);
	}
	endlabelno = id * 10000;
	generate_func (extra);
	if (generate_stbl) {
//		if ((id > 0 && id < 10) || (id >= 20))
//			fprintf (stblfile, "#endif /* CPUEMU_68000_ONLY */\n");
		if (postfix2 >= 0 && postfix2 != 4)
			fprintf (stblfile, "#endif /* CPUEMU_%d%s */\n", postfix2, extraup);
	}
	postfix2 = -1;
}

int main (int argc, char **argv)
{
	int i;

	read_table68k ();
	do_merges ();

	opcode_map =  xmalloc (int, nr_cpuop_funcs);
	opcode_last_postfix = xmalloc (int, nr_cpuop_funcs);
	opcode_next_clev = xmalloc (int, nr_cpuop_funcs);
	counts = xmalloc (unsigned long, 65536);
	read_counts ();

	/* It would be a lot nicer to put all in one file (we'd also get rid of
	* cputbl.h that way), but cpuopti can't cope.  That could be fixed, but
	* I don't dare to touch the 68k version.  */

	headerfile = fopen ("cputbl.h", "wb");

	stblfile = fopen ("cpustbl.cpp", "wb");
	generate_includes (stblfile, 0);

	using_prefetch = 0;
	using_indirect = 0;
	using_exception_3 = 1;
	using_ce = 0;

	for (i = 0; i < 13; i++) {
		if ((i >= 6 && i < 11) || (i > 14 && i < 20) || (i > 24 && i < 31))
			continue;
		generate_stbl = 1;
		generate_cpu (i, 0);
	}

	free (table68k);
  fclose(headerfile);
  fclose(stblfile);
	return 0;
}

void write_log (const TCHAR *format,...)
{
}
