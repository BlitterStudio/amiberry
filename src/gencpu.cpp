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
#define xBCD_KEEPS_N_FLAG 4
#define xBCD_KEEPS_V_FLAG 2

static FILE *headerfile;
static FILE *stblfile;

// optional settings
static int using_bus_error = 1;

static int using_prefetch, using_indirect;
static int using_exception_3;
static int using_ce;
static int using_simple_cycles;
static int need_exception_oldpc;
static int cpu_level, cpu_generic;
static int count_readw, count_readl;
static int count_writew, count_writel;
static int count_cycles, count_ncycles;
static int did_prefetch;
static int ipl_fetched;

#define GF_APDI		0x00001
#define GF_AD8R		0x00002
#define GF_PC8R		0x00004
#define GF_AA		0x00007
#define GF_NOREFILL	0x00008
#define GF_PREFETCH	0x00010
#define GF_FC		0x00020
#define GF_MOVE		0x00040
#define GF_IR2IRC	0x00080
#define GF_LRMW		0x00100
#define GF_NOFAULTPC	0x00200
#define GF_RMW		0x00400
#define GF_OPCE020	0x00800
#define GF_REVERSE	0x01000
#define GF_REVERSE2	0x02000
#define GF_SECONDWORDSETFLAGS	0x04000
#define GF_SECONDEA	0x08000
#define GF_NOFETCH	0x10000 // 68010
#define GF_CLR68010	0x20000 // 68010
#define GF_NOEXC3	0x40000
#define GF_EXC3		0x80000
// internal PC is 2 less than address being prefetched.
#define GF_PCM2		0x100000
// internal PC is 2 more than address being prefetched.
#define GF_PCP2		0x200000

typedef enum
{
	flag_logical_noclobber, flag_logical, flag_add, flag_sub, flag_cmp, flag_addx, flag_subx, flag_z, flag_zn,
	flag_av, flag_sv
}
flagtypes;

/* For the current opcode, the next lower level that will have different code.
* Initialized to -1 for each opcode. If it remains unchanged, indicates we
* are done with that opcode.  */
static int next_cpu_level;

static int *opcode_map;
static int *opcode_next_clev;
static int *opcode_last_postfix;
static unsigned long *counts;
static int generate_stbl;
static struct instr *g_instr;
static char g_srcname[100];
static int postfix;

#define GENA_GETV_NO_FETCH	0
#define GENA_GETV_FETCH		1
#define GENA_GETV_FETCH_ALIGN	2
#define GENA_MOVEM_DO_INC	0
#define GENA_MOVEM_NO_INC	1
#define GENA_MOVEM_MOVE16	2

static const char *srcl, *dstl;
static const char *srcw, *dstw;
static const char *srcb, *dstb;
static const char *srcblrmw, *srcwlrmw, *srcllrmw;
static const char *dstblrmw, *dstwlrmw, *dstllrmw;
static const char *prefetch_long, *prefetch_word, *prefetch_opcode;
static const char *srcli, *srcwi, *srcbi, *nextl, *nextw;
static const char *srcld, *dstld;
static const char *srcwd, *dstwd;
static const char *do_cycles, *disp000, *disp020, *getpc;

#define fetchmode_fea 1
#define fetchmode_cea 2
#define fetchmode_fiea 3
#define fetchmode_ciea 4
#define fetchmode_jea 5

static int brace_level;

static char outbuffer[30000];
static int last_access_offset_ipl;

static void out(const char *format, ...)
{
	char outbuf[1000];
	va_list parms;
	va_start(parms, format);
	_vsnprintf(outbuf, sizeof(outbuf) - 1, format, parms);
	outbuf[sizeof(outbuf) - 1] = 0;
	va_end(parms);

	char *p = outbuf;
	while (*p) {
		char v = *p;
		if (v == '\t') {
			memmove(p, p + 1, strlen(p + 1) + 1);
		} else {
			p++;
		}
	}

	p = outbuf;
	for (;;) {
		char *pe = p;
		int islf = 0;
		while (*pe != 0 && *pe != '\n') {
			pe++;
		}
		if (*pe == '\n') {
			islf = 1;
			*pe = 0;
		}

		char outbuf2[1000];
		strcpy(outbuf2, p);
		outbuf2[pe - p] = 0;

		if (outbuf2[0]) {
			char *ps = outbuf2;
			while (*ps) {
				char v = *ps;
				if (v == '}') {
					brace_level--;
				}
				ps++;
			}
			for (int i = 0; i < brace_level; i++) {
				strcat(outbuffer, "\t");
			}
			strcat(outbuffer, outbuf2);

			ps = outbuf2;
			while (*ps) {
				char v = *ps;
				if (v == '{') {
					brace_level++;
				}
				ps++;
			}
		}

		if (islf) {
			strcat(outbuffer, "\n");
			pe++;
		}
		if (*pe == 0)
			break;
		p = pe;
	}
}

static void insertstring(const char *s, int offset)
{
	int len = strlen(s);
	memmove(outbuffer + offset + len + brace_level, outbuffer + offset, strlen(outbuffer + offset) + 1);
	for (int i = 0; i < brace_level; i++) {
		outbuffer[offset + i] = '\t';
	}
	memcpy(outbuffer + offset + brace_level, s, len);
}

static void set_last_access_ipl(void)
{
  if (ipl_fetched)
    return; 
  last_access_offset_ipl = strlen(outbuffer);
}

NORETURN static void term (void)
{
	out("Abort!\n");
	abort();
}
NORETURN static void term (const char *err)
{
	out("%s\n", err);
	term();
}

static void read_counts (void)
{
	FILE *file;
	unsigned int opcode, count, total;
	char name[20];
	int nr = 0;
	memset (counts, 0, 65536 * sizeof *counts);

	count = 0;
	file = fopen ("frequent.68k", "r");
	if (file) {
		if (fscanf (file, "Total: %u\n", &total) == 0) {
			abort();
		}
		while (fscanf (file, "%x: %u %s\n", &opcode, &count, name) == 3) {
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
		term();
}

static int genamode_cnt, genamode8r_offset[2];
static int set_fpulimit;

static int m68k_pc_offset, m68k_pc_offset_old;
static int m68k_pc_total;
static int exception_pc_offset, exception_pc_offset_extra_000;
static int branch_inst;
static int ir2irc;
static int insn_n_cycles;

static bool needbuserror(void)
{
	if (!using_bus_error)
		return false;
	// only 68000/010 need cpuemu internal bus error handling
	// 68020+ use CATCH/TRY method
	if (postfix >= 10 && postfix < 20)
		return true;
	return false;
}

// 68010-40 needs different implementation than 68060
static bool next_level_060_to_040(void)
{
	if (cpu_level >= 5) {
		if (next_cpu_level < 5) {
			next_cpu_level = 5 - 1;
			return true;
		}
	}
	return false;
}
// 68010-30 needs different implementation than 68040/060
static bool next_level_040_to_030(void)
{
	if (cpu_level >= 4) {
		if (next_cpu_level < 4) {
			next_cpu_level = 4 - 1;
			return true;
		}
	}
	return false;
}
// 68000-010 needs different implementation than 68020+
static bool next_level_020_to_010(void)
{
	if (cpu_level >= 2) {
		if (next_cpu_level < 2) {
			next_cpu_level = 2 - 1;
			return true;
		}
	}
	return false;
}
// 68000 <> 68010
static void next_level_000(void)
{
	if (next_cpu_level < 0) {
		next_cpu_level = 0;
  }
}

static void fpulimit (void)
{
	out("\n#ifdef FPUEMU\n");
	set_fpulimit = 1;
}

static int s_count_readw, s_count_readl;
static int s_count_writew, s_count_writel;
static int s_count_cycles, s_count_ncycles, s_insn_cycles;

static void push_ins_cnt(void)
{
	s_count_readw = count_readw;
	s_count_readl = count_readl;
	s_count_writew = count_writew;
	s_count_writel = count_writel;
	s_count_cycles = count_cycles;
	s_count_ncycles = count_ncycles;
	s_insn_cycles = insn_n_cycles;
}
static void pop_ins_cnt(void)
{
	count_readw = s_count_readw;
	count_readl = s_count_readl;
	count_writew = s_count_writew;
	count_writel = s_count_writel;
	count_cycles = s_count_cycles;
	count_ncycles = s_count_ncycles;
	insn_n_cycles = s_insn_cycles;
}

static void check_ipl(void)
{
	if (ipl_fetched)
		return;
	if (using_ce)
		out("ipl_fetch();\n");
	ipl_fetched = 1;
}

static void check_ipl_always(void)
{
	if (using_ce)
		out("ipl_fetch();\n");
}

static void returncycles (int cycles)
{
	if (using_ce) {
		out("return;\n");
		return;
	}
  if (using_simple_cycles) {
		out("return %d * CYCLE_UNIT / 2 + count_cycles;\n", cycles);
	} else {
		out("return %d * CYCLE_UNIT / 2;\n", cycles);
  }
}

static void returncycles_exception(void)
{
	if (using_ce) {
		out("return;\n");
	} else {
    if (using_simple_cycles) {
	    out("return %d * CYCLE_UNIT / 2 + count_cycles;\n", (count_readw + count_readl * 2 + 1 + count_writew + count_writel * 2) * 4 + count_cycles);
    } else {
      out("return %d * CYCLE_UNIT / 2;\n", (count_readw + count_readl * 2 + 1 + count_writew + count_writel * 2) * 4 + count_cycles);
    }
  }
}

static void write_return_cycles2(int end, int no4)
{
	if (using_ce || using_prefetch) {
		int cc = count_cycles;
		if (count_readw + count_writew + count_readl + count_writel + cc == 0 && !no4)
			cc = 4;
		returncycles((count_readw + count_writew) * 4 + (count_readl + count_writel) * 8 + cc);
		if (end) {
		  out("}\n");
		  out("/* %d%s (%d/%d)",
			  (count_readw + count_writew) * 4 + (count_readl + count_writel) * 8 + cc, count_ncycles ? "+" : "", 
        count_readw + count_readl * 2, count_writew + count_writel * 2);
		  out(" */\n");
    }
	} else {
		returncycles ((count_readw + count_writew) * 4 + (count_readl + count_writel) * 8 + insn_n_cycles);
		out("}\n");
	}
}

static void write_return_cycles(int end)
{
	write_return_cycles2(end, 0);
}

static void addcycles000_nonces(const char *sc)
{
	if (using_simple_cycles) {
		out("count_cycles += (%s) * CYCLE_UNIT / 2;\n", sc);
		count_ncycles++;
	}
}
static void addcycles000_nonce(int c)
{
	if (using_simple_cycles) {
		out("count_cycles += %d * CYCLE_UNIT / 2;\n", c);
		count_ncycles++;
	}
}

static void addcycles000_onlyce (int cycles)
{
	if (using_ce) {
		out("%s(%d);\n", do_cycles, cycles);
	}
}

static void addcycles000(int cycles)
{
	if (using_ce) {
		out("%s(%d);\n", do_cycles, cycles);
	}
	count_cycles += cycles;
	insn_n_cycles += cycles;
}
static void addcycles000_3(void)
{
	if (using_ce) {
		out("if (cycles > 0) %s(cycles);\n", do_cycles);
	}
	count_ncycles++;
}

static int isreg (amodes mode)
{
	if (mode == Dreg || mode == Areg)
		return 1;
	return 0;
}

static int bit_size (int size)
{
	switch (size) {
	case sz_byte: return 8;
	case sz_word: return 16;
	case sz_long: return 32;
	default: term();
	}
	return 0;
}

static const char *bit_mask (int size)
{
	switch (size) {
	case sz_byte: return "0xff";
	case sz_word: return "0xffff";
	case sz_long: return "0xffffffff";
	default: term();
	}
	return 0;
}

static void setpcstring(char *dst, const char *format, ...)
{
	va_list parms;
	char buffer[1000];

	va_start(parms, format);
	_vsnprintf(buffer, 1000 - 1, format, parms);
	va_end(parms);

	if (using_prefetch)
		sprintf(dst, "m68k_setpci_j(%s);\n", buffer);
	else
		sprintf(dst, "m68k_setpc_j(%s);\n", buffer);
}

static void setpc(const char *format, ...)
{
	va_list parms;
	char buffer[1000];

	va_start(parms, format);
	_vsnprintf(buffer, 1000 - 1, format, parms);
	va_end(parms);

	if (using_prefetch)
		out("m68k_setpci_j(%s);\n", buffer);
	else
		out("m68k_setpc_j(%s);\n", buffer);
}

static void incpc(const char *format, ...)
{
	va_list parms;
	char buffer[1000];

	va_start(parms, format);
	_vsnprintf(buffer, 1000 - 1, format, parms);
	va_end(parms);

	if (using_prefetch)
		out("m68k_incpci(%s);\n", buffer);
	else
		out("m68k_incpc(%s);\n", buffer);
}

static void sync_m68k_pc(void)
{
	m68k_pc_offset_old = m68k_pc_offset;
	if (m68k_pc_offset == 0)
		return;
	incpc("%d", m68k_pc_offset);
	m68k_pc_total += m68k_pc_offset;
	m68k_pc_offset = 0;
}

static void clear_m68k_offset(void)
{
	m68k_pc_total += m68k_pc_offset;
	m68k_pc_offset = 0;
}

static void sync_m68k_pc_noreset(void)
{
	sync_m68k_pc();
	m68k_pc_offset = m68k_pc_offset_old;
}

static char bus_error_text[200];
static int bus_error_specials;
static int bus_error_cycles;
// 1 = first access, 2 = long second access only
static char bus_error_code[1000], bus_error_code2[1000];

static void do_instruction_buserror(void)
{
	if (!needbuserror())
		return;

	if (bus_error_text[0]) {
		out("if(hardware_bus_error) {\n");
		out("int pcoffset = 0;\n");
		if (bus_error_code[0])
			out("%s", bus_error_code);
		out("%s", bus_error_text);
		write_return_cycles(0);
		out("}\n");
	}
	bus_error_code[0] = 0;
	bus_error_specials = 0;
}

static void check_bus_error_ins(int offset, int pcoffset)
{
	const char *opcode;
	if (bus_error_specials) {
		opcode = "0";
	} else {
		opcode = "opcode";
	}
	if (pcoffset == -1) {
		sprintf(bus_error_text, "exception2_fetch(%s, %d, pcoffset);\n", opcode, offset);
	} else {
		sprintf(bus_error_text, "exception2_fetch(%s, %d, %d);\n", opcode, offset, pcoffset);
	}
}

static void check_bus_error_ins_opcode(int offset, int pcoffset)
{
	const char *opcode;
	if (bus_error_specials) {
		opcode = "0";
	} else {
		opcode = "opcode";
	}
	if (pcoffset == -1) {
		sprintf(bus_error_text, "exception2_fetch_opcode(%s, %d, pcoffset);\n", opcode, offset);
	} else {
		sprintf(bus_error_text, "exception2_fetch_opcode(%s, %d, %d);\n", opcode, offset, pcoffset);
	}
}

static void check_prefetch_bus_error(int offset, int pcoffset, int secondprefetchmode)
{
	if (!needbuserror())
		return;

	if (offset < 0) {
		if (offset == -1)
			offset = 0;
		else
			offset = 2;
		// full prefetch: opcode field is zero
		if ((offset == 2 && !secondprefetchmode) || secondprefetchmode > 0) {
			bus_error_specials = 1;
		}
	}
	check_bus_error_ins_opcode(offset, pcoffset);
	do_instruction_buserror();
}

static void check_prefetch_buserror(int offset, int pcoffset)
{
	check_bus_error_ins(offset, pcoffset);
	do_instruction_buserror();
}


static void gen_nextilong2(const char *type, const char *name, int flags, int movem)
{
	int r = m68k_pc_offset;
	int pcoffset = ((flags & (GF_PCM2 | GF_PCP2)) == (GF_PCM2 | GF_PCP2) ? 0 : (((flags & GF_PCM2) ? -2 : (flags & GF_PCP2) ? 2 : 0)));
	m68k_pc_offset += 4;

	out("%s %s;\n", type, name);
	if (using_ce) {
		/* we must do this because execution order of (something | something2) is not defined */
		if (flags & GF_NOREFILL) {
			set_last_access_ipl();
			out("%s = %s(%d) << 16;\n", name, prefetch_word, r + 2);
			count_readw++;
			out("%s |= regs.irc;\n", name);
			check_bus_error_ins(r + 2, pcoffset);
			do_instruction_buserror();
			strcpy(bus_error_code, bus_error_code2);
			bus_error_code2[0] = 0;
			bus_error_text[0] = 0;
		} else {
			out("%s = %s(%d) << 16;\n", name, prefetch_word, r + 2);
			count_readw++;
			check_bus_error_ins(r + 2, pcoffset);
			do_instruction_buserror();
			strcpy(bus_error_code, bus_error_code2);
			bus_error_code2[0] = 0;
      set_last_access_ipl();
			out("%s |= %s(%d);\n", name, prefetch_word, r + 4);
			count_readw++;
			check_bus_error_ins(r + 4, -1);
		}
	} else if (using_prefetch) {
		if (flags & GF_NOREFILL) {
			set_last_access_ipl();
			out("%s = %s(%d) << 16;\n", name, prefetch_word, r + 2);
			count_readw++;
			out("%s |= regs.irc;\n", name);
			check_bus_error_ins(r + 2, pcoffset);
			do_instruction_buserror();
			strcpy(bus_error_code, bus_error_code2);
			bus_error_code2[0] = 0;
			bus_error_text[0] = 0;
		} else {
			out("%s = %s(%d) << 16;\n", name, prefetch_word, r + 2);
			count_readw++;
			check_bus_error_ins(r + 2, pcoffset);
			do_instruction_buserror();
			strcpy(bus_error_code, bus_error_code2);
			bus_error_code2[0] = 0;
			set_last_access_ipl();
			out("%s |= %s(%d);\n", name, prefetch_word, r + 4);
			count_readw++;
			check_bus_error_ins(r + 4, -1);
		}
	} else {
		if (flags & GF_NOREFILL) {
			count_readw++;
		} else {
			count_readl++;
		}
		out("%s = %s(%d);\n", name, prefetch_long, r);
	}
}
static void gen_nextilong (const char *type, const char *name, int flags)
{
	bus_error_text[0] = 0;
	bus_error_specials = 0;
	gen_nextilong2 (type, name, flags, 0);
}

static const char *gen_nextiword(int flags)
{
	static char buffer[80];
	int r = m68k_pc_offset;
	int pcoffset = ((flags & (GF_PCM2 | GF_PCP2)) == (GF_PCM2 | GF_PCP2) ? 0 : (((flags & GF_PCM2) ? -2 : (flags & GF_PCP2) ? 2 : 0)));
	m68k_pc_offset += 2;

	bus_error_text[0] = 0;
	bus_error_specials = 0;
	if (using_ce) {
		if (flags & GF_NOREFILL) {
			strcpy(buffer, "regs.irc");
		} else {
			set_last_access_ipl();
			sprintf(buffer, "%s(%d)", prefetch_word, r + 2);
			count_readw++;
			check_bus_error_ins(r + 2, pcoffset);
		}
	} else if (using_prefetch) {
		if (flags & GF_NOREFILL) {
			strcpy(buffer, "regs.irc");
		} else {
			set_last_access_ipl();
			sprintf(buffer, "%s(%d)", prefetch_word, r + 2);
			count_readw++;
			check_bus_error_ins(r + 2, pcoffset);
		}
	} else {
		sprintf(buffer, "%s(%d)", prefetch_word, r);
		if (!(flags & GF_NOREFILL)) {
			count_readw++;
			check_bus_error_ins(r, pcoffset);
		}
	}
	return buffer;
}

static const char *gen_nextibyte(int flags)
{
	static char buffer[80];
	int r = m68k_pc_offset;
	int pcoffset = ((flags & (GF_PCM2 | GF_PCP2)) == (GF_PCM2 | GF_PCP2) ? 0 : (((flags & GF_PCM2) ? -2 : (flags & GF_PCP2) ? 2 : 0)));
	m68k_pc_offset += 2;

	bus_error_text[0] = 0;
	bus_error_specials = 0;
	if (using_ce) {
		if (flags & GF_NOREFILL) {
			strcpy(buffer, "(uae_u8)regs.irc");
		} else {
			set_last_access_ipl();
			sprintf(buffer, "(uae_u8)%s(%d)", prefetch_word, r + 2);
			count_readw++;
			check_bus_error_ins(r + 2, pcoffset);
		}
	} else if (using_prefetch) {
		if (flags & GF_NOREFILL) {
			strcpy(buffer, "(uae_u8)regs.irc");
		} else {
			set_last_access_ipl();
			sprintf(buffer, "(uae_u8)%s(%d)", prefetch_word, r + 2);
			count_readw++;
			check_bus_error_ins(r + 2, pcoffset);
		}
	} else {
		sprintf(buffer, "%s(%d)", srcbi, r);
		if (!(flags & GF_NOREFILL)) {
			count_readw++;
			check_bus_error_ins(r, pcoffset);
		}
	}
	return buffer;
}

static void makefromsr(void)
{
	out("MakeFromSR();\n");
	if (using_ce)
		out("regs.ipl_pin = intlev();\n");
}

static void makefromsr_t0(void)
{
	if (using_prefetch || using_ce) {
		out("MakeFromSR();\n");
	} else {
	  out("MakeFromSR_T0();\n");
  }
	if (using_ce)
		out("regs.ipl_pin = intlev();\n");
}

static void irc2ir (bool dozero)
{
	if (!using_prefetch)
		return;
	if (ir2irc)
		return;
	ir2irc = 1;
	out("regs.ir = regs.irc;\n");
	if (dozero)
		out("regs.irc = 0;\n");
}
static void irc2ir (void)
{
	irc2ir (false);
}

static void copy_opcode(void)
{
	out("opcode = regs.ir;\n");
}

static void fill_prefetch_bcc(void)
{
	if (!using_prefetch)
		return;

	if (using_bus_error) {
		copy_opcode();
		if (cpu_level == 0) {
			out("if(regs.t1) opcode |= 0x10000;\n");
		}
		next_level_000();
	}
	out("%s(%d);\n", prefetch_word, m68k_pc_offset + 2);
	count_readw++;
	check_prefetch_bus_error(m68k_pc_offset + 2, -1, 0);
	did_prefetch = 1;
	ir2irc = 0;
}

static void fill_prefetch_1(int o)
{
	if (using_prefetch) {
		set_last_access_ipl();
		out("%s(%d);\n", prefetch_word, o);
    count_readw++;
		check_prefetch_bus_error(o, -1, 0);
		did_prefetch = 1;
		ir2irc = 0;
	} else {
		insn_n_cycles += 4;
	}
}

// complete prefetch
static void fill_prefetch_1_empty(int o)
{
	if (using_prefetch) {
		set_last_access_ipl();
		out("%s(%d);\n", prefetch_word, o);
		count_readw++;
		check_prefetch_bus_error(o ? -2 : -1, 0, 0);
		did_prefetch = 1;
		ir2irc = 0;
	} else {
		insn_n_cycles += 4;
	}
}

// don't check trace bits
static void fill_prefetch_full_ntx(int beopcode)
{
	if (using_prefetch) {
		fill_prefetch_1_empty(0);
		irc2ir();
		if (beopcode) {
			copy_opcode();
			if (cpu_level == 0) {
				if (beopcode == 2)
					out("if(regs.t1) opcode |= 0x10000;\n");
				if (beopcode == 3)
					out("if(t1) opcode |= 0x10000;\n");
			}
			next_level_000();
			fill_prefetch_1(2);
		} else {
			fill_prefetch_1_empty(2);
		}
	} else {
		insn_n_cycles += 2 * 4;
	}
}
static void check_trace(void)
{
	if (!using_prefetch && !using_ce && cpu_level >= 2)
		out("if(regs.t0) check_t0_trace();\n");
}

static void trace_t0_68040_only(void)
{
	if (cpu_level == 4)
		check_trace();
	if (cpu_level == 5) {
		if (next_cpu_level < 5)
			next_cpu_level = 5 - 1;
	} else if (cpu_level == 4) {
		if (next_cpu_level < 4)
			next_cpu_level = 4 - 1;
	}
}

// check trace bits
static void fill_prefetch_full(int beopcode)
{
	if (using_prefetch) {
		fill_prefetch_1_empty(0);
		irc2ir();
		if (beopcode) {
			copy_opcode();
			if (beopcode > 1 && cpu_level == 0) {
				out("if(regs.t1) opcode |= 0x10000;\n");
			}
			next_level_000();
			fill_prefetch_1(2);
		} else {
			fill_prefetch_1_empty(2);
		}
	} else if (cpu_level >= 2) {
		insn_n_cycles += 2 * 4;
		check_trace();
	} else {
		insn_n_cycles += 2 * 4;
	}
}

// 0 = pc not changed
// 1 = pc == oldpc + 2, both address and PC
// 2 = pc not changed, stacked PC is oldpc + 2
// -1 = first fetch: pcoffset - adjustment second: pcoffset = 0 (dbcc)
static void fill_prefetch_full_000_special(int pctype, const char *format, ...)
{
	char tmp[100];
	if (!using_prefetch) {
		if (format && cpu_level <= 1) {
			char outbuf[256];
			va_list parms;
			va_start(parms, format);
			_vsnprintf(outbuf, sizeof(outbuf) - 1, format, parms);
			va_end(parms);
			out(outbuf);
		}
		insn_n_cycles += 2 * 4;
		return;
	}

	out("%s(%d);\n", prefetch_word, 0);
	count_readw++;
	if (pctype == 1) {
		setpcstring(tmp, "oldpc + 2");
		strcpy(bus_error_code, tmp);
	} else if (pctype == 2) {
		sprintf(tmp, "pcoffset = oldpc - %s + 2;\n", getpc);
		strcpy(bus_error_code, tmp);
	} else if (pctype == -1) {
		strcpy(bus_error_code, "pcoffset += pcadjust;\n");
	}
	check_prefetch_bus_error(-1, -1, -1);
	irc2ir();
	if (using_bus_error) {
		copy_opcode();
		if (cpu_level == 0) {
			if (g_instr->mnemo == i_RTE) {
				out("if(oldt1) opcode |= 0x10000;\n");
			} else {
				out("if(regs.t1) opcode |= 0x10000;\n");
			}
		}
		next_level_000();
	}
	if (format) {
		char outbuf[256];
		va_list parms;
		va_start(parms, format);
		_vsnprintf(outbuf, sizeof(outbuf) - 1, format, parms);
		va_end(parms);
		out(outbuf);
	}
  set_last_access_ipl();
	out("%s(%d);\n", prefetch_word, 2);
	count_readw++;
	if (pctype > 0) {
		strcpy(bus_error_code, tmp);
	}
	check_prefetch_bus_error(-2, -1, -1);
	did_prefetch = 1;
	ir2irc = 0;
}

// 68000 and 68010 only
static void fill_prefetch_full_000(int beopcode)
{
	if (using_prefetch) {
  	fill_prefetch_full(beopcode);
	} else {
		insn_n_cycles += 2 * 4;
  }
}

// 68020+
static void fill_prefetch_full_020 (void)
{
	if (!using_prefetch && !using_ce && cpu_level >= 2)
		out("if(regs.t0) check_t0_trace();\n");
}

static void fill_prefetch_0 (void)
{
	if (using_prefetch) {
	  out("%s(0);\n", prefetch_word);
		count_readw++;
		check_prefetch_bus_error(0, 0, 0);
	  did_prefetch = 1;
	  ir2irc = 0;
	} else {
		insn_n_cycles += 4;
  }
}

static void fill_prefetch_next_noopcodecopy(const char *format, ...)
{
	if (using_prefetch) {
		irc2ir();
		if (using_bus_error) {
			bus_error_code[0] = 0;
			if (format) {
				va_list parms;
				va_start(parms, format);
				_vsnprintf(bus_error_code, sizeof(bus_error_code) - 1, format, parms);
				va_end(parms);
			}
			if (cpu_level == 0) {
				out("opcode |= 0x20000;\n");
			}
			next_level_000();
		}
		fill_prefetch_1(m68k_pc_offset + 2);
		if (using_bus_error) {
			copy_opcode();
		}
	} else {
		insn_n_cycles += 4;
	}
}

static void fill_prefetch_next(void)
{
	if (using_prefetch) {
	  irc2ir ();
		if (using_bus_error) {
			copy_opcode();
		}
	  fill_prefetch_1 (m68k_pc_offset + 2);
	} else {
		insn_n_cycles += 4;
	}
}

static void fill_prefetch_next_t(void)
{
	if (using_prefetch) {
		irc2ir();
		if (using_bus_error) {
			copy_opcode();
			if (cpu_level == 0) {
				strcat(bus_error_code, "if (regs.t1) opcode |= 0x10000;\n");
			}
			next_level_000();
		}
		fill_prefetch_1(m68k_pc_offset + 2);
	} else {
		insn_n_cycles += 4;
	}
}


static void fill_prefetch_next_extra(const char *cond, const char *format, ...)
{
	if (using_prefetch) {
		irc2ir();
		if (using_bus_error) {
			if (cond)
				out("%s\n", cond);
			copy_opcode();
			bus_error_code[0] = 0;
			if (format) {
				va_list parms;
				va_start(parms, format);
				_vsnprintf(bus_error_code, sizeof(bus_error_code) - 1, format, parms);
				va_end(parms);
			}
		}
		fill_prefetch_1(m68k_pc_offset + 2);
	} else {
		insn_n_cycles += 4;
	}
}

static void fill_prefetch_next_after(int copy, const char *format, ...)
{
	if (using_prefetch) {
		irc2ir();
		if (cpu_level == 0) {
			out("opcode |= 0x20000;\n");
		}
		next_level_000();
		bus_error_code[0] = 0;
		if (format) {
			va_list parms;
			va_start(parms, format);
			_vsnprintf(bus_error_code, sizeof(bus_error_code) - 1, format, parms);
			va_end(parms);
		}
		fill_prefetch_1(m68k_pc_offset + 2);
		if (using_bus_error) {
			if (copy) {
				copy_opcode();
			}
		}
	} else {
		insn_n_cycles += 4;
	}
}

static void fill_prefetch_next_empty(void)
{
	if (using_prefetch) {
		irc2ir();
		fill_prefetch_1_empty(m68k_pc_offset + 2);
	} else {
		insn_n_cycles += 4;
	}
}

static void fill_prefetch_finish (void)
{
	if (did_prefetch)
		return;
	if (using_prefetch) {
		fill_prefetch_1 (m68k_pc_offset);
	}
}

static void dummy_prefetch(const char *newpc, const char *oldpc)
{
	if (using_prefetch && cpu_level == 1) {
		if (!newpc && !oldpc) {
			out("%s((m68k_getpci() & 1) ? -1 : 0);\n", prefetch_word);
		} else {
			if (newpc) {
				if (!oldpc)
					out("uaecptr d_oldpc = m68k_getpci();\n");
				setpc("%s & ~1", newpc);
			}
			out("%s(0);\n", prefetch_word);
			if (oldpc) {
				setpc(oldpc);
			} else if (newpc) {
				setpc("d_oldpc");
			}
		}
	}
}

static void duplicate_carry(void)
{
	out("COPY_CARRY();\n");
}

static void genflags_normal(flagtypes type, wordsizes size, const char *value, const char *src, const char *dst)
{
	char vstr[100], sstr[100], dstr[100];
	char usstr[100], udstr[100];
	char unsstr[100], undstr[100];

	switch (size) {
	case sz_byte:
		strcpy(vstr, "((uae_s8)(");
		strcpy(usstr, "((uae_u8)(");
		break;
	case sz_word:
		strcpy(vstr, "((uae_s16)(");
		strcpy(usstr, "((uae_u16)(");
		break;
	case sz_long:
		strcpy(vstr, "((uae_s32)(");
		strcpy(usstr, "((uae_u32)(");
		break;
	default:
		term();
	}
	strcpy(unsstr, usstr);

	strcpy(sstr, vstr);
	strcpy(dstr, vstr);
	strcat(vstr, value);
	strcat(vstr, "))");
	strcat(dstr, dst);
	strcat(dstr, "))");
	strcat(sstr, src);
	strcat(sstr, "))");

	strcpy(udstr, usstr);
	strcat(udstr, dst);
	strcat(udstr, "))");
	strcat(usstr, src);
	strcat(usstr, "))");

	strcpy(undstr, unsstr);
	strcat(unsstr, "-");
	strcat(undstr, "~");
	strcat(undstr, dst);
	strcat(undstr, "))");
	strcat(unsstr, src);
	strcat(unsstr, "))");

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
		out("uae_u32 %s = %s + %s;\n", value, udstr, usstr);
		break;
	case flag_sub:
	case flag_cmp:
		out("uae_u32 %s = %s - %s;\n", value, udstr, usstr);
		break;
	}

	switch (type) {
	case flag_logical_noclobber:
	case flag_logical:
	case flag_z:
	case flag_zn:
		break;

	case flag_add:
	case flag_sub:
	case flag_addx:
	case flag_subx:
	case flag_cmp:
	case flag_av:
	case flag_sv:
		out("" BOOL_TYPE " flgs = %s < 0;\n", sstr);
		out("" BOOL_TYPE " flgo = %s < 0;\n", dstr);
		out("" BOOL_TYPE " flgn = %s < 0;\n", vstr);
		break;
	}

	switch (type) {
	case flag_logical:
		out("CLEAR_CZNV();\n");
		out("SET_ZFLG(%s == 0);\n", vstr);
		out("SET_NFLG(%s < 0);\n", vstr);
		break;
	case flag_logical_noclobber:
		out("SET_ZFLG(%s == 0);\n", vstr);
		out("SET_NFLG(%s < 0);\n", vstr);
		break;
	case flag_av:
		out("SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));\n");
		break;
	case flag_sv:
		out("SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));\n");
		break;
	case flag_z:
		out("SET_ZFLG(GET_ZFLG() & (%s == 0));\n", vstr);
		break;
	case flag_zn:
		out("SET_ZFLG(GET_ZFLG() & (%s == 0));\n", vstr);
		out("SET_NFLG(%s < 0);\n", vstr);
		break;
	case flag_add:
		out("SET_ZFLG(%s == 0);\n", vstr);
		out("SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));\n");
		out("SET_CFLG(%s < %s);\n", undstr, usstr);
		duplicate_carry();
		out("SET_NFLG(flgn != 0);\n");
		break;
	case flag_sub:
		out("SET_ZFLG(%s == 0);\n", vstr);
		out("SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));\n");
		out("SET_CFLG(%s > %s);\n", usstr, udstr);
		duplicate_carry();
		out("SET_NFLG(flgn != 0);\n");
		break;
	case flag_addx:
		out("SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));\n"); /* minterm SON: 0x42 */
		out("SET_CFLG(flgs ^ ((flgs ^ flgo) & (flgo ^ flgn)));\n"); /* minterm SON: 0xD4 */
		duplicate_carry();
		break;
	case flag_subx:
		out("SET_VFLG((flgs ^ flgo) & (flgo ^ flgn));\n"); /* minterm SON: 0x24 */
		out("SET_CFLG(flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));\n"); /* minterm SON: 0xB2 */
		duplicate_carry();
		break;
	case flag_cmp:
		out("SET_ZFLG(%s == 0);\n", vstr);
		out("SET_VFLG((flgs != flgo) && (flgn != flgo));\n");
		out("SET_CFLG(%s > %s);\n", usstr, udstr);
		out("SET_NFLG(flgn != 0);\n");
		break;
	}
}

static void genflags(flagtypes type, wordsizes size, const char *value, const char *src, const char *dst)
{
	genflags_normal(type, size, value, src, dst);
}

// Handle special MOVE.W/.L condition codes when destination write causes bus error.
static void move_68000_bus_error(int offset, int size, int *setapdi, int *fcmodeflags)
{

	int smode = g_instr->smode;
	int dmode = g_instr->dmode;

	if (size == sz_byte) {

		if (dmode == Apdi) {

			if (cpu_level == 0) {
			  out("if (regs.t1) opcode |= 0x10000;\n"); // I/N set
			}

		} else if (dmode == Aipi) {

			// move.b x,(an)+: an is not increased
			out("m68k_areg(regs, dstreg) -= areg_byteinc[dstreg];\n");
		}

	} else if (size == sz_word) {

		if (dmode == Apdi) {

			if (cpu_level == 0) {
				out("if (regs.t1) opcode |= 0x10000;\n"); // I/N set
			}

		} else if (dmode == Aipi) {

			// move.w x,(an)+: an is not increased
			out("m68k_areg(regs, dstreg) -= 2;\n");
		}

	} else if (size == sz_long && ((offset == 0 && dmode != Apdi) || (offset == 2 && dmode == Apdi))) {

		// Long MOVE is more complex but not as complex as address error..
		// First word.

		int set_ccr = 0;
		int set_high_word = 0;
		int set_low_word = 0;
		switch (smode)
		{
		case Dreg:
		case Areg:
			if (dmode == Ad16 || dmode == Ad8r) {
				set_high_word = 3;
			} else if (dmode == Apdi || dmode == absw || dmode == absl) {
				set_ccr = 1;
			}
			break;
		case Aind:
		case Aipi:
		case Apdi:
		case Ad16:
		case Ad8r:
		case PC16:
		case PC8r:
		case absl:
		case absw:
			if (dmode == Aind || dmode == Aipi || dmode == absw || dmode == absl) {
				set_low_word = 1;
			} else if (dmode == Apdi || dmode == Ad16 || dmode == Ad8r) {
				set_ccr = 1;
			}
			break;
		case imm:
			if (dmode == Ad16 || dmode == Ad8r) {
				set_high_word = 1;
			} else if (dmode == Apdi || dmode == absw || dmode == absl) {
				set_ccr = 1;
			}
			break;
		}

		if (set_low_word == 1) {
			// Low word: Z and N
			out("ccr_68000_long_move_ae_LZN(src);\n");
		} else if (set_high_word == 3) {
			// High word: N only, clear Z if non-zero
			out("SET_NFLG(src < 0);\n");
			out("if((src & 0xffff0000)) SET_ZFLG(0);\n");
		} else if (set_high_word) {
			// High word: N set/reset and Z clear.
			out("ccr_68000_long_move_ae_HNZ(src);\n");
		} else if (set_ccr) {
			// Set normally.
			out("ccr_68000_long_move_ae_normal(src);\n");
		}

		if (dmode == Aipi) {
			out("m68k_areg(regs, dstreg) -= 4;\n");
		} else if (dmode == Apdi) {
			out("m68k_areg(regs, dstreg) += 4;\n");
		}

	} else if (size == sz_long) {

		// Second word (much simpler)

		int set_ccr = 0;
		int set_low_word = 0;
		switch (smode)
		{
		case Dreg:
		case Areg:
			if (dmode == Apdi || dmode == absw || dmode == absl) {
				set_ccr = 1;
			} else {
				set_low_word = 1;
			}
			break;
		case Aind:
		case Aipi:
		case Apdi:
		case Ad16:
		case Ad8r:
		case PC16:
		case PC8r:
		case absl:
		case absw:
			set_ccr = 1;
			break;
		case imm:
			if (dmode == Apdi || dmode == absw || dmode == absl) {
				set_ccr = 1;
			} else {
				set_low_word = 1;
			}
			break;
		}

		if (set_low_word == 1) {
			// Low word: Z and N
			out("ccr_68000_long_move_ae_LZN(src);\n");
		} else if (set_ccr) {
			// Set normally.
			out("ccr_68000_long_move_ae_normal(src);\n");
		}

		if (dmode == Aipi) {
			out("m68k_areg(regs, dstreg) -= 4;\n");
		} else if (dmode == Apdi) {
			out("m68k_areg(regs, dstreg) += 4;\n");
		}

	}

}

static char const *bus_error_reg;
static int bus_error_reg_add;

static int do_bus_error_fixes(const char *name, int offset, int write)
{
	switch (bus_error_reg_add)
	{
	case 1:
	case -1:
		if (g_instr->mnemo == i_CMPM && bus_error_reg_add > 0) {
			// CMPM.B (an)+,(an)+: first increased normally, second not increased
			out("m68k_areg(regs, %s) += areg_byteinc[%s] + %d;\n", bus_error_reg, bus_error_reg, offset);
		}
		break;
	case 2:
	case -2:
		if (g_instr->mnemo == i_RTR) {
			;
		} else if (g_instr->mnemo == i_RTE) {
			// stack is decreased first
			out("m68k_areg(regs, %s) += %d;\n", bus_error_reg, cpu_level == 0 ? 14 : 58);
		} else {
			out("m68k_areg(regs, %s) += 2 + %d;\n", bus_error_reg, offset);
		}
		break;
	case 3:
	case -3:
		if (g_instr->mnemo == i_CMPM && bus_error_reg_add > 0) {
			// CMPM.L (an)+,(an)+: first increased normally, second not increased
			out("m68k_areg(regs, %s) += 2 + %d;\n", bus_error_reg, offset);
		}
		break;
	case 4:
	case -4:
		if ((g_instr->mnemo == i_ADDX || g_instr->mnemo == i_SUBX) && g_instr->size == sz_long) {
			// ADDX.L/SUBX.L -(an),-(an) source: stack frame decreased by 2, not 4.
			offset += 2;
		} else if (g_instr->mnemo == i_RTR) {
			if (offset) {
				out("m68k_areg(regs, %s) += 4;\n", bus_error_reg);
				out("regs.sr &= 0xFF00; sr &= 0xFF;\n");
				out("regs.sr |= sr;\n");
				out("MakeFromSR();\n");
			} else {
				out("m68k_areg(regs, %s) -= 2;\n", bus_error_reg);
			}
		} else if (g_instr->mnemo == i_RTE) {
			// stack is decreased first
			out("m68k_areg(regs, %s) += %d - 2;\n", bus_error_reg, cpu_level == 0 ? 14 : 58);
			if (offset) {
				out("regs.sr = sr;\n");
				out("MakeFromSR();\n");
			}
		} else if (cpu_level == 1) {
			// -(an).l where first word causes bus error: An is not modified
			// -(an).l where second word causes bus error: An is modified
			if (g_instr->size != sz_long || (g_instr->size == sz_long && offset)) {
				out("m68k_areg(regs, %s) = %sa;\n", bus_error_reg, name);
			}
		} else {
			out("m68k_areg(regs, %s) = %sa;\n", bus_error_reg, name);
		}
		break;
	}
	return offset;
}

static void check_bus_error(const char *name, int offset, int write, int size, const char *writevar, int fc, int pcoffset)
{
	int mnemo = g_instr->mnemo;

	if (!needbuserror())
		return;

	// basic support
	if ((!using_prefetch && !using_ce) || cpu_level >= 2) {

		fc &= 7;
		out("if(hardware_bus_error) {\n");
		if (write) {
			out("exception2_write(opcode, %sa + %d, %d, %s, %d);\n",
				name, offset, size, writevar,
				(!write && (g_instr->smode == PC16 || g_instr->smode == PC8r)) ||
				(write && (g_instr->dmode == PC16 || g_instr->dmode == PC8r)) ? 2 : fc);
		} else {
			out("exception2_read(opcode, %sa + %d, %d, %d);\n",
				name, offset, size,
				(!write && (g_instr->smode == PC16 || g_instr->smode == PC8r)) ||
				(write && (g_instr->dmode == PC16 || g_instr->dmode == PC8r)) ? 2 : fc);
		}
		write_return_cycles(0);
		out("}\n");
		return;
	}

	// 68000/68010 bus error
	if (cpu_level >= 2)
		return;
	if (!using_prefetch && !using_ce)
		return;

	next_level_000();

	uae_u32 extra = fc & 0xffff0000;
	fc &= 0xffff;

	out("if(hardware_bus_error) {\n");

	int setapdiback = 0;

	if (fc == 2) {

		out("exception2_fetch(opcode, %d);\n", offset);

	} else {

		if (bus_error_cycles > 0) {
			if (using_prefetch) {
				out("%s(%d);\n", do_cycles, bus_error_cycles);
			} else {
				out("count_cycles += % d * CYCLE_UNIT / 2;\n", bus_error_cycles);
			}
			bus_error_cycles = 0;
		}

		int pc_offset_extra = cpu_level == 0 ? exception_pc_offset_extra_000 : 0;

		if (pcoffset == -1) {
			incpc("%d", m68k_pc_offset + 2);
		} else if (exception_pc_offset + pc_offset_extra + pcoffset) {
			incpc("%d", exception_pc_offset + pc_offset_extra + pcoffset);
		}

		if (g_instr->mnemo == i_MOVE && write) {
			move_68000_bus_error(offset, g_instr->size, &setapdiback, &fc);
		}

		offset = do_bus_error_fixes(name, offset, write);

		if (mnemo == i_BTST && (g_instr->dmode == PC16 || g_instr->dmode == PC8r)) {
			// BTST special case where destination is read access
			fc = 2;
		}
		if (mnemo == i_MVMEL && (g_instr->dmode == PC16 || g_instr->dmode == PC8r)) {
			// MOVEM to registers
			fc = 2;
		}

		if ((mnemo == i_ADDX || mnemo == i_SUBX) && g_instr->size == sz_long && g_instr->dmode == Apdi && write && offset) {
			out(
				"int bflgs = ((uae_s16)(src)) < 0;\n"
				"int bflgo = ((uae_s16)(dst)) < 0;\n"
				"int bflgn = ((uae_s16)(newv)) < 0;\n");
			if (mnemo == i_ADDX) {
				out(
					"SET_VFLG((bflgs ^ bflgn) & (bflgo ^ bflgn));\n"
					"SET_CFLG(bflgs ^ ((bflgs ^ bflgo) & (bflgo ^ bflgn)));\n");
			} else {
				out(
					"SET_VFLG((bflgs ^ bflgo) & (bflgo ^ bflgn));\n"
					"SET_CFLG(bflgs ^ ((bflgs ^ bflgn) & (bflgo ^ bflgn)));\n");
			}
			out(
				"COPY_CARRY();\n"
				"SET_ZFLG(GET_ZFLG() & (((uae_s16)(newv)) == 0));\n"
				"SET_NFLG(((uae_s16)(newv)) < 0);\n");
		}

		if (mnemo == i_LINK) {
			// a7 -> a0 copy done before A7 address error check
			if (write) {
				out("m68k_areg(regs, 7) += 4;\n");
			}
			out("m68k_areg(regs, srcreg) = olda;\n");
		}
		if (mnemo == i_PEA && write && offset && g_instr->smode != absw && g_instr->smode != absl) {
			if (cpu_level == 0) {
				out("if (regs.t1) opcode |= 0x10000;\n"); // I/N set
			}
		}

		if (cpu_level == 1) {
			// 68010 bus/address error HB bit
			if (extra) {
				out("opcode |= 0x%x;\n", extra);
			}
			// upper byte of SSW is zero -flag.
			if (g_instr->mnemo == i_MVSR2 && !write) {
				out("opcode |= 0x20000;\n");
			}
			if (g_instr->mnemo == i_MOVES) {
				out("regs.irc = extra;\n");
				if (!write) {
					out("regs.write_buffer = extra;\n");
				}
				if (g_instr->size == sz_long) {
					if (g_instr->dmode == Apdi) {
						if (!write) {
							out("m68k_areg(regs, dstreg) = srca;\n");
						}
					} else if (g_instr->dmode == Aipi) {
						out("m68k_areg(regs, dstreg) += 4;\n");
					}
				}
				if (!write) {
					if (g_instr->dmode == Apdi || g_instr->dmode == absl) {
						incpc("2");
					} else if (g_instr->dmode >= Ad16) {
						incpc("4");
					}
				}
			} else if (g_instr->mnemo == i_TAS) {
				if (!write) {
					out("regs.read_buffer = regs.irc & 0xff00;\n");
					out("regs.read_buffer |= 0x80;\n");
					if (cpu_level == 1 && g_instr->smode >= Ad16) {
						incpc("2");
					}
				}
				out("opcode |= 0x80000;\n");
			} else if (g_instr->mnemo == i_CLR) {
				if (g_instr->smode < Ad16) {
					out("#if defined(CPU_i386) || defined(CPU_x86_64)\n");
					out("regs.ccrflags.cznv = oldflags;\n");
					out("#else\n");
					out("regs.ccrflags.nzcv = oldflags;\n");
					out("#endif\n");
				}
				// (an)+ and -(an) is done later
				if (g_instr->smode == Aipi || g_instr->smode == Apdi) {
					if (g_instr->size == sz_byte) {
						out("m68k_areg(regs, srcreg) %c= areg_byteinc[srcreg];\n", g_instr->smode == Aipi ? '-' : '+');
					} else {
						out("m68k_areg(regs, srcreg) %c= %d;\n", g_instr->smode == Aipi ? '-' : '+', 1 << g_instr->size);
					}
				}
			} else if (g_instr->mnemo == i_MOVE) {
				if (write) {
					if (g_instr->smode >= Aind && g_instr->smode < imm && g_instr->dmode == absl) {
						out("regs.irc = dsta >> 16;\n");
					}
				}
			} else if (g_instr->mnemo == i_MVPRM) {
				if (write) {
					if (g_instr->size == sz_word) {
						out("uae_u16 val = src;\n");
					} else {
						out("uae_u16 val = src >> %d;\n", offset <= 2 ? 16 : 0);
					}
					size |= 0x100;
					writevar = "val";
				}
			}
		}

		// write causing bus error and trace: set I/N
		if (write && g_instr->size <= sz_word && cpu_level == 0 &&
			mnemo != i_MOVE &&
			mnemo != i_BSR &&
			mnemo != i_LINK &&
			mnemo != i_MVMEL && mnemo != i_MVMLE &&
			mnemo != i_MVPRM && mnemo != i_MVPMR) {
			out("if (regs.t1) opcode |= 0x10000;\n"); // I/N set
		}

		if (write) {
			out("exception2_write(opcode, %sa + %d, 0x%x, %s, %d);\n",
				name, offset, size, writevar,
				(!write && (g_instr->smode == PC16 || g_instr->smode == PC8r)) ||
				(write && (g_instr->dmode == PC16 || g_instr->dmode == PC8r)) ? 2 : fc);
		} else {
			out("exception2_read(opcode, %sa + %d, 0x%x, %d);\n",
				name, offset, size,
				(!write && (g_instr->smode == PC16 || g_instr->smode == PC8r)) ||
				(write && (g_instr->dmode == PC16 || g_instr->dmode == PC8r)) ? 2 : fc);
		}
	}

	write_return_cycles(0);
	out("}\n");
}

// Handle special MOVE.W/.L condition codes when destination write causes address error.
static void move_68000_address_error(int size, int *setapdi, int *fcmodeflags)
{
	int smode = g_instr->smode;
	int dmode = g_instr->dmode;

	if (dmode == Apdi) {
		addcycles000_onlyce(2);
		addcycles000_nonce(2);
	} else if (dmode == Aipi) {
		// move.x x,(an)+: an is not increased
		out("m68k_areg(regs, dstreg) -= %d;\n", 1 << size);
	}

	if (size == sz_word) {
		// Word MOVE is relatively simple
		int set_ccr = 0;
		switch(smode)
		{
			case Dreg:
			case Areg:
				set_ccr = 1;
			break;
			case Aind:
			case Aipi:
			case Apdi:
			case Ad16:
			case PC16:
			case Ad8r:
			case PC8r:
			case absw:
			case absl:
			case imm:
				if (dmode == Aind || dmode == Aipi || dmode == Apdi || dmode == Ad16 || dmode == Ad8r || dmode == absw || dmode == absl)
					set_ccr = 1;
			break;
		}
		if (set_ccr) {
			out("ccr_68000_word_move_ae_normal((uae_s16)(src));\n");
		}
	} else {
		// Long MOVE is much more complex..
		int set_ccr = 0;
		int set_high_word = 0;
		int set_low_word = 0;
		switch (smode)
		{
		case Dreg:
		case Areg:
		case imm:
			if (dmode == Aind || dmode == Aipi) {
				set_ccr = 0;
			} else if (dmode == Ad16 || dmode == Ad8r) {
				set_high_word = 1;
			} else if (dmode == Apdi || dmode == absw || dmode == absl) {
				set_ccr = 1;
			}
			break;
		case Ad16:
		case PC16:
		case Ad8r:
		case PC8r:
		case absw:
		case absl:
			if (dmode == Apdi || dmode == Ad16 || dmode == Ad8r || dmode == absw) {
				set_ccr = 1;
			} else if (dmode == Aind || dmode == Aipi || dmode == absl) {
				set_low_word = 1;
			} else {
				set_low_word = 2;
			}
			break;
		case Aind:
		case Aipi:
		case Apdi:
			if (dmode == Aind || dmode == Aipi || dmode == absl) {
				set_low_word = 1;
			} else {
				set_ccr = 1;
			}
			break;
		}

		if (dmode == Apdi) {
			*setapdi = 0;
			out("m68k_areg(regs, dstreg) += 4;\n");
			out("regs.ir = opcode;\n");
		}

		if (set_low_word == 1) {
			// Low word: Z and N
			out("ccr_68000_long_move_ae_LZN(src);\n");
		} else if (set_high_word) {
			// High word: N and Z clear.
			out("ccr_68000_long_move_ae_HNZ(src);\n");
		} else if (set_ccr) {
			// Set normally.
			out("ccr_68000_long_move_ae_normal(src);\n");
		}
	}
}

// Handle special MOVE.W/.L condition codes when destination write causes address error.
static void move_68010_address_error(int size, int *setapdi, int *fcmodeflags)
{
	int smode = g_instr->smode;
	int dmode = g_instr->dmode;

	if (size == sz_word) {
		// Word MOVE is relatively simple
		int set_ccr = 0;
		int reset_ccr = 0;
		*setapdi = 1;
		switch (smode)
		{
		case Dreg:
		case Areg:
		case imm:
			if (dmode == Apdi || dmode == Ad16 || dmode == Ad8r || dmode == absw || dmode == absl)
				set_ccr = 1;
			else
				reset_ccr = 1;
			break;
		case Aind:
		case Aipi:
		case Apdi:
		case Ad16:
		case PC16:
		case Ad8r:
		case PC8r:
		case absw:
		case absl:
			if (dmode == Aind || dmode == Aipi || dmode == Apdi || dmode == Ad16 || dmode == Ad8r || dmode == absw || dmode == absl)
				set_ccr = 1;
			break;
		}
		if (dmode == Apdi) {
			dummy_prefetch(NULL, NULL);
		} else if (dmode == absl && smode >= Aind && smode < imm) {
			out("regs.irc = dsta >> 16;\n");
		}
		if (reset_ccr) {
			out("#if defined(CPU_i386) || defined(CPU_x86_64)\n");
			out("regs.ccrflags.cznv = oldflags;\n");
			out("#else\n");
			out("regs.ccrflags.nzcv = oldflags;\n");
			out("#endif\n");
		}
		if (set_ccr) {
			out("ccr_68000_word_move_ae_normal((uae_s16)(src));\n");
		}
	} else {
		// Long MOVE is much more complex..
		int set_ccr = 0;
		int set_high_word = 0;
		int set_low_word = 0;
		if (dmode == Apdi) {
			*setapdi = -4;
		} else {
			*setapdi = 1;
		}
		switch (smode)
		{
		case Dreg:
		case Areg:
		case imm:
			if (dmode == Aind || dmode == Aipi) {
				set_ccr = 0;
			} else if (dmode == Ad16 || dmode == Ad8r) {
				set_high_word = 1;
			} else if (dmode == Apdi || dmode == absw || dmode == absl) {
				set_ccr = 1;
			}
			break;
		case Ad16:
		case PC16:
		case Ad8r:
		case PC8r:
		case absw:
		case absl:
			if (dmode == Apdi || dmode == Ad16 || dmode == Ad8r || dmode == absw) {
				set_ccr = 1;
			} else if (dmode == Aind || dmode == Aipi || dmode == absl) {
				set_low_word = 1;
			} else {
				set_low_word = 2;
			}
			break;
		case Aind:
		case Aipi:
		case Apdi:
			if (dmode == Aind || dmode == Aipi || dmode == absl) {
				set_low_word = 1;
			} else {
				set_ccr = 1;
			}
			break;
		}

		if (dmode == absl && smode >= Aind && smode < imm) {
			out("regs.irc = dsta >> 16;\n");
		}

		if (set_low_word == 1) {
			// Low word: Z and N
			out("ccr_68000_long_move_ae_LZN(src);\n");
		} else if (set_high_word) {
			// High word: N and Z clear.
			out("ccr_68000_long_move_ae_HNZ(src);\n");
		} else if (set_ccr) {
			// Set normally.
			out("ccr_68000_long_move_ae_normal(src);\n");
		}
	}
}

static void check_address_error(const  char *name, int mode, const char *reg, int size, int getv, int movem, int flags)
{
	// check possible address error (if 68000/010 and enabled)
	if ((using_prefetch || using_ce) && using_exception_3 && getv != 0 && getv != 3 && size != sz_byte && !movem && !(flags & GF_NOEXC3)) {
		int setapdiback = 0;
		int fcmodeflags = 0;
		int exp3rw = getv == 2;
		int pcextra = 0;

		next_level_000();

		out("if (%sa & 1) {\n", name);

		if (cpu_level == 1) {
			int bus_error_reg_add_old = bus_error_reg_add;
			if (abs(bus_error_reg_add) == 4)
				bus_error_reg_add = 0;
			// 68010 CLR <memory>: pre and post are not added yet
			if (g_instr->mnemo == i_CLR) {
				if (mode == Aipi)
					bus_error_reg_add = 0;
				if (mode == Apdi)
					bus_error_reg_add = 0;
			}
			if (g_instr->mnemo == i_MOVE || g_instr->mnemo == i_MOVEA) {
				if (getv != 2) {
					do_bus_error_fixes(name, 0, getv == 2);
				}
			} else {
				do_bus_error_fixes(name, 0, getv == 2);
			}
			if (g_instr->mnemo == i_MOVES) {
				// MOVES has strange behavior
				out("regs.irc = extra;\n");
				if (!exp3rw) {
					out("regs.write_buffer = extra;\n");
					if (mode == Ad8r || mode == PC8r) {
						pcextra = 4;
					} else {
						pcextra = 2;
					}
				} else {
					// moves.w an,-(an)/(an)+ (same registers): write buffer contains modified value.
					if (mode == Aipi || mode == Apdi) {
						out("if (dstreg + 8 == ((extra >> 12) & 15)) {\n");
						out("src += %d;\n", mode == Aipi ? 2 : -2);
						out("}\n");
					}
					if (mode >= Ad16) {
						pcextra = 2;
					}
				}
				if (size == sz_long) {
					if (mode == Aipi) {
						out("m68k_areg(regs, dstreg) += 4;\n");
					} else if (mode == Apdi) {
						setapdiback = 1;
					}
				}
			}

			// x,-(an): an is modified (MOVE to CCR counts as word sized)
			if (mode == Apdi && g_instr->mnemo != i_CLR && size == sz_word) {
				out("m68k_areg(regs, %s) = %sa;\n", reg, name);
			}
			bus_error_reg_add = bus_error_reg_add_old;
		}

		if (g_instr->mnemo == i_ADDX || g_instr->mnemo == i_SUBX) {
			// ADDX/SUBX special case
			if (g_instr->size == sz_word) {
				out("m68k_areg(regs, %s) = %sa;\n", reg, name);
			}
		} else if (mode == Apdi && g_instr->mnemo != i_LINK) {
			// 68000 decrements register first, then checks for address error
			// 68010 does not
			if (cpu_level == 0) {
				setapdiback = 1;
			}
		}

		// adjust MOVE write address error stacked PC
		if (g_instr->mnemo == i_MOVE && getv == 2) {
			if (g_instr->smode >= Aind && g_instr->smode != imm && g_instr->dmode == absl) {
				pcextra = -2;
			} else if (g_instr->dmode >= Ad16) {
				pcextra = 0;
			} else {
				pcextra = 2;
			}
		}

		int pc_offset_extra = cpu_level == 0 ? exception_pc_offset_extra_000 : 0;

		if (exception_pc_offset + pc_offset_extra + pcextra) {
			incpc("%d", exception_pc_offset + pc_offset_extra + pcextra);
		}

		if (g_instr->mnemo == i_MOVE) {
			if (getv == 2) {
				if (cpu_level == 0) {
					move_68000_address_error(size, &setapdiback, &fcmodeflags);
				} else {
					move_68010_address_error(size, &setapdiback, &fcmodeflags);
				}
				if (mode != Apdi && mode != Aipi) {
					setapdiback = 0;
				}
			}
		} else if (g_instr->mnemo == i_MVSR2) {
			// If MOVE from SR generates address error exception,
			// Change it to read because it does dummy read first.
			exp3rw = 0;
			if (cpu_level == 1) {
				out("opcode |= 0x20000;\n"); // upper byte of SSW is zero -flag.
			}
		} else if (g_instr->mnemo == i_LINK) {
			// a7 -> a0 copy done before A7 address error check
			out("m68k_areg(regs, srcreg) = olda;\n");
			setapdiback = 0;
		}

		// can be used for both Apdi and Aipi
		if (setapdiback) {
			if (setapdiback > 0) {
				out("m68k_areg(regs, %s) = %sa;\n", reg, name);
			} else {
				out("m68k_areg(regs, %s) = %sa + %d;\n", reg, name, -setapdiback);
			}
		}

		// MOVE.L EA,-(An) causing address error: stacked value is original An - 2, not An - 4.
		if ((flags & (GF_REVERSE | GF_REVERSE2)) && size == sz_long && mode == Apdi)
			out("%sa += %d;\n", name, flags & GF_REVERSE2 ? -2 : 2);

		if (exp3rw) {
			const char *shift = (size == sz_long && !(flags & GF_REVERSE)) ? " >> 16" : "";
			out("exception3_write_access(opcode, %sa, %d, %s%s, %d);\n",
				name, size, g_srcname, shift,
				// PC-relative: FC=2
				(getv == 1 && (g_instr->smode == PC16 || g_instr->smode == PC8r) ? 2 : 1) | fcmodeflags);

		} else {
			// 68010 address error: if addressing mode is (An), (An)+ or -(An) and byte or word: CPU does extra read access!
			if (cpu_level == 1 && (g_instr->smode == Aind || g_instr->smode == Aipi || g_instr->smode == Apdi) && g_instr->size < sz_long) {
				out("exception3_read_access2(opcode, %sa, %d, %d);\n",
					name, size,
					// PC-relative: FC=2
					(getv == 1 && (g_instr->smode == PC16 || g_instr->smode == PC8r) ? 2 : 1) | fcmodeflags);
			} else {
				out("exception3_read_access(opcode, %sa, %d, %d);\n",
					name, size,
					// PC-relative: FC=2
					(getv == 1 && (g_instr->smode == PC16 || g_instr->smode == PC8r) ? 2 : 1) | fcmodeflags);
			}
		}

		returncycles_exception();
		out("}\n");
	}
}

/* getv == 1: fetch data; getv != 0: check for odd address. If movem != 0,
* the calling routine handles Apdi and Aipi modes.
* gb-- movem == 2 means the same thing but for a MOVE16 instruction */

/* fixup indicates if we want to fix up address registers in pre decrement
* or post increment mode now (0) or later (1). A value of 2 will then be
* used to do the actual fix up. This allows to do all memory readings
* before any register is modified, and so to rerun operation without
* side effect in case a bus fault is generated by any memory access.
* XJ - 2006/11/13 */

static void genamode2x (amodes mode, const char *reg, wordsizes size, const char *name, int getv, int movem, int flags, int fetchmode)
{
	char namea[100];
	bool rmw = false;
	int pc_68000_offset = m68k_pc_offset;
	int pc_68000_offset_fetch = 0;
	int pc_68000_offset_store = 0;
	bool addr = false;
	// common EA prefetch bus error PC behavior
	int pcflag = !(flags & (GF_PCM2 | GF_PCP2)) ? GF_PCM2 : 0;

	sprintf (namea, "%sa", name);

	if (mode == Ad8r || mode == PC8r) {
		genamode8r_offset[genamode_cnt] = m68k_pc_total + m68k_pc_offset;
		genamode_cnt++;
	}

	switch (mode) {
	case Dreg:
		if (movem)
			term();
		if (getv == 1)
			switch (size) {
			case sz_byte:
#ifdef USE_DUBIOUS_BIGENDIAN_OPTIMIZATION
				/* This causes the target compiler to generate better code on few systems */
				out("uae_s8 %s = ((uae_u8*)&m68k_dreg(regs, %s))[3];\n", name, reg);
#else
				out("uae_s8 %s = m68k_dreg(regs, %s);\n", name, reg);
#endif
				break;
			case sz_word:
#ifdef USE_DUBIOUS_BIGENDIAN_OPTIMIZATION
				out("uae_s16 %s = ((uae_s16*)&m68k_dreg(regs, %s))[1];\n", name, reg);
#else
				out("uae_s16 %s = m68k_dreg(regs, %s);\n", name, reg);
#endif
				break;
			case sz_long:
				out("uae_s32 %s = m68k_dreg(regs, %s);\n", name, reg);
				break;
			default:
				term();
		}
		strcpy(g_srcname, name);
		return;
	case Areg:
		if (movem)
			term();
		if (getv == 1)
			switch (size) {
			case sz_word:
				out("uae_s16 %s = m68k_areg(regs, %s);\n", name, reg);
				break;
			case sz_long:
				out("uae_s32 %s = m68k_areg(regs, %s);\n", name, reg);
				break;
			default:
				term();
		}
		strcpy(g_srcname, name);
		return;
	case Aind: // (An)
		out("uaecptr %sa;\n", name);
		out("%sa = m68k_areg(regs, %s);\n", name, reg);
		if (flags & GF_NOFETCH) {
			addcycles000(2);
		}
		break;
	case Aipi: // (An)+
		out("uaecptr %sa;\n", name);
		out("%sa = m68k_areg(regs, %s);\n", name, reg);
		if (flags & GF_NOFETCH) {
			addcycles000(4);
		}
		break;
	case Apdi: // -(An)
		out("uaecptr %sa;\n", name);
		switch (size) {
		case sz_byte:
			if (movem)
				out("%sa = m68k_areg(regs, %s);\n", name, reg);
			else
				out("%sa = m68k_areg(regs, %s) - areg_byteinc[%s];\n", name, reg, reg);
			break;
		case sz_word:
			out("%sa = m68k_areg(regs, %s) - %d;\n", name, reg, movem ? 0 : 2);
			break;
		case sz_long:
			out("%sa = m68k_areg(regs, %s) - %d;\n", name, reg, movem ? 0 : 4);
			break;
		default:
			term();
		}
		if (flags & GF_NOFETCH) {
			addcycles000(4);
		} else if (!(flags & GF_APDI)) {
			addcycles000(2);
			if (cpu_level == 1) {
				;
			} else {
				pc_68000_offset_fetch += 2;
				if (size == sz_long)
					pc_68000_offset_fetch -= 2;
			}
		}
		break;
	case Ad16: // (d16,An)
		out("uaecptr %sa;\n", name);
		out("%sa = m68k_areg(regs, %s) + (uae_s32)(uae_s16)%s;\n", name, reg, gen_nextiword(flags | pcflag));
		addr = true;
		break;
	case PC16: // (d16,PC)
		out("uaecptr %sa;\n", name);
		out("%sa = %s + %d;\n", name, getpc, m68k_pc_offset);
		out("%sa += (uae_s32)(uae_s16)%s;\n", name, gen_nextiword(flags | pcflag));
		addr = true;
		break;
	case Ad8r: // (d8,An,Xn)
		out("uaecptr %sa;\n", name);
		if (cpu_level > 1) {
			if (next_cpu_level < 1)
				next_cpu_level = 1;
			sync_m68k_pc();
			/* This would ordinarily be done in gen_nextiword, which we bypass.  */
			insn_n_cycles += 4;
			out("%sa = %s(m68k_areg(regs, %s));\n", name, disp020, reg);
		} else {
			if (!(flags & GF_AD8R) && !(flags & GF_NOFETCH) && !(flags & GF_CLR68010)) {
				addcycles000(2);
			}
			if ((flags & GF_NOREFILL) && using_prefetch) {
				out("%sa = %s(m68k_areg(regs, %s), regs.irc);\n", name, disp000, reg);
			} else {
				out("%sa = %s(m68k_areg(regs, %s), %s);\n", name, disp000, reg, gen_nextiword(flags | pcflag));
			}
		}
		if (flags & GF_NOFETCH) {
			addcycles000(4);
		}
		if ((flags & GF_CLR68010) && using_prefetch) {
			addcycles000(4);
		}
		addr = true;
		break;
	case PC8r: // (d8,PC,Xn)
		out("uaecptr %sa;\n", name);
		if (cpu_level > 1) {
			if (next_cpu_level < 1)
				next_cpu_level = 1;
			sync_m68k_pc();
			/* This would ordinarily be done in gen_nextiword, which we bypass.  */
			insn_n_cycles += 4;
			out("uaecptr tmppc = %s;\n", getpc);
			out("%sa = %s(tmppc);\n", name, disp020);
		} else {
			out("uaecptr tmppc = %s + %d;\n", getpc, m68k_pc_offset);
			if (!(flags & GF_PC8R)) {
				addcycles000(2);
			}
			if ((flags & GF_NOREFILL) && using_prefetch) {
				out("%sa = %s(tmppc, regs.irc);\n", name, disp000);
			} else {
				out("%sa = %s(tmppc, %s);\n", name, disp000, gen_nextiword(flags | pcflag));
			}
		}
		if (flags & GF_NOFETCH) {
			addcycles000(4);
		}
		addr = true;
		break;
	case absw:
		out("uaecptr %sa;\n", name);
		out("%sa = (uae_s32)(uae_s16)%s;\n", name, gen_nextiword(flags));
		pc_68000_offset_fetch += 2;
		addr = true;
		break;
	case absl:
		gen_nextilong2 ("uaecptr", namea, flags | pcflag, movem);
		pc_68000_offset_fetch += 4;
		pc_68000_offset_store += 2;
		addr = true;
		break;
	case imm:
		// fetch immediate address
		if (getv != 1)
			term();
		switch (size) {
		case sz_byte:
			out("uae_s8 %s = %s;\n", name, gen_nextibyte(flags));
			break;
		case sz_word:
			out("uae_s16 %s = %s;\n", name, gen_nextiword(flags));
			break;
		case sz_long:
			gen_nextilong("uae_s32", name, flags | pcflag);
			break;
		default:
			term ();
		}
		do_instruction_buserror();
		strcpy(g_srcname, name);
		return;
	case imm0:
		if (getv != 1)
			term();
		out("uae_s8 %s = %s;\n", name, gen_nextibyte(flags));
		do_instruction_buserror();
		strcpy(g_srcname, name);
		return;
	case imm1:
		if (getv != 1)
			term();
		out("uae_s16 %s = %s;\n", name, gen_nextiword(flags));
		do_instruction_buserror();
		strcpy(g_srcname, name);
		return;
	case imm2:
		if (getv != 1)
			term();
		gen_nextilong("uae_s32", name, flags);
		strcpy(g_srcname, name);
		return;
	case immi:
		if (getv != 1)
			term();
		out("uae_u32 %s = %s;\n", name, reg);
		strcpy(g_srcname, name);
		return;
	case am_unknown:
		// reg = internal variable
		out("uae_u32 %sa = %s;\n", name, reg);
		addr = true;
		break;
	default:
		term();
	}

	if (getv == 2 && g_srcname[0] && g_srcname[strlen(g_srcname) - 1] == 'a' && g_instr->mnemo != i_PEA) {
		g_srcname[strlen(g_srcname) - 1] = 0;
	}
	if (g_srcname[0] == 0) {
		if (addr || g_instr->mnemo == i_PEA)
			strcpy(g_srcname, namea);
		else
			strcpy(g_srcname, name);	
	}

	if (mode >= Ad16 && mode < am_unknown) {
		do_instruction_buserror();
	}

	/* We get here for all non-reg non-immediate addressing modes to
	* actually fetch the value. */

	bus_error_reg_add = 0;
	bus_error_reg = reg;
	if (!movem) {
		if (mode == Aipi) {
			switch (size)
			{
			case sz_byte:
				bus_error_reg_add = 1;
				break;
			case sz_word:
				bus_error_reg_add = 2;
				break;
			case sz_long:
				bus_error_reg_add = 3;
				break;
			default: term();
			}
		} else if (mode == Apdi) {
			bus_error_reg_add = 4;
		}
		if (flags & GF_SECONDEA) {
			bus_error_reg_add = -bus_error_reg_add;
		}
	}

	exception_pc_offset = pc_68000_offset;
	if (getv == 2) {
		// store
		if (pc_68000_offset) {
			exception_pc_offset += pc_68000_offset_store + 2;
		}
	} else {
		// fetch
		exception_pc_offset += pc_68000_offset_fetch;
	}

	if (!(flags & GF_NOEXC3)) {
		check_address_error(name, mode, reg, size, getv, movem, flags);
	}

	if (flags & GF_PREFETCH)
		fill_prefetch_next();
	else if (flags & GF_IR2IRC)
		irc2ir (true);

	if (getv == 1) {
		if (using_ce || using_prefetch) {
			switch (size) {
			case sz_byte:
      {
        out("uae_s8 %s = %s(%sa);\n", name, srcb, name); 
        count_readw++; 
				check_bus_error(name, 0, 0, 0, NULL, 1, 0);
        break;
			}
			case sz_word:
      {
        out("uae_s16 %s = %s(%sa);\n", name, srcw, name); 
        count_readw++; 
				check_bus_error(name, 0, 0, 1, NULL, 1, 0);
        break;
			}
			case sz_long: 
      {
				if ((flags & GF_REVERSE) && mode == Apdi) {
					out("uae_s32 %s = %s(%sa + 2);\n", name, srcw, name);
					count_readw++;
					check_bus_error(name, 0, 0, 1, NULL, 1, 0);
					out("%s |= %s(%sa) << 16; \n", name, srcw, name);
					count_readw++;
					check_bus_error(name, -2, 0, 1, NULL, 1, 0);
				} else {
					out("uae_s32 %s = %s(%sa) << 16;\n", name, srcw, name);
					count_readw++;
					check_bus_error(name, 0, 0, 1, NULL, 1, 0);
					out("%s |= %s(%sa + 2); \n", name, srcw, name);
					count_readw++;
					check_bus_error(name, 2, 0, 1, NULL, 1, 0);
				}
        break;
      }
			default: term ();
			}
		} else {
			switch (size) {
			case sz_byte: 
        out("uae_s8 %s = %s(%sa);\n", name, srcb, name); 
        count_readw++; 
				check_bus_error(name, 0, 0, 0, NULL, 1, 0);
        break;
			case sz_word: 
        out("uae_s16 %s = %s(%sa);\n", name, srcw, name); 
        count_readw++; 
				check_bus_error(name, 0, 0, 1, NULL, 1, 0);
        break;
			case sz_long: 
        out("uae_s32 %s = %s(%sa);\n", name, srcl, name); 
				count_readl++;
				check_bus_error(name, 0, 0, 2, NULL, 1, 0);
        break;
			default: term ();
			}
		}
	}

	bus_error_reg_add = 0;

	/* We now might have to fix up the register for pre-dec or post-inc
	* addressing modes. */
	if (!movem) {
		switch (mode) {
		  case Aipi:
			  switch (size) {
			  case sz_byte:
				  out("m68k_areg(regs, %s) += areg_byteinc[%s];\n", reg, reg);
				  break;
			  case sz_word:
				  out("m68k_areg(regs, %s) += 2;\n", reg);
				  break;
			  case sz_long:
				  out("m68k_areg(regs, %s) += 4;\n", reg);
				  break;
			  default:
				  term ();
			  }
			  break;
		  case Apdi:
			  out("m68k_areg(regs, %s) = %sa;\n", reg, name);
			  break;
		  default:
			  break;
	  }
  }
}

static void genamode2 (amodes mode, const char *reg, wordsizes size, const char *name, int getv, int movem, int flags)
{
	genamode2x (mode, reg, size, name, getv, movem, flags, -1);
}

static void genamode(struct instr *curi, amodes mode, const char *reg, wordsizes size, const char *name, int getv, int movem, int flags)
{
	genamode2 (mode, reg, size, name, getv, movem, flags);
}

static void genamode3 (struct instr *curi, amodes mode, const char *reg, wordsizes size, const char *name, int getv, int movem, int flags)
{
	genamode2x (mode, reg, size, name, getv, movem, flags, curi ? curi->fetchmode : -1);
}

static void genamodedual(struct instr *curi, amodes smode, const char *sreg, wordsizes ssize, const char *sname, int sgetv, int sflags,
					   amodes dmode, const char *dreg, wordsizes dsize, const char *dname, int dgetv, int dflags)
{
	int subhead = 0;
	bool eadmode = false;

	genamode3 (curi, smode, sreg, ssize, sname, sgetv, 0, sflags);
	genamode3 (NULL, dmode, dreg, dsize, dname, dgetv, 0, dflags | (eadmode == true ? GF_OPCE020 : 0) | GF_SECONDEA);
}

static void genastore_2 (const char *from, amodes mode, const char *reg, wordsizes size, const char *to, int store_dir, int flags)
{
	char tmp[100];
	int pcoffset = (flags & GF_MOVE) ? 0 : 2;

	if (flags & GF_PCM2) {
		pcoffset -= 2;
	} else if (flags & GF_PCP2) {
		pcoffset += 2;
	}

	exception_pc_offset = m68k_pc_offset;

	if ((flags & GF_EXC3) && !isreg(mode)) {
		check_address_error(to, mode, reg, size, 2, 0, flags);
	}

	switch (mode) {
	case Dreg:
		switch (size) {
		case sz_byte:
			out("m68k_dreg(regs, %s) = (m68k_dreg(regs, %s) & ~0xff) | ((%s) & 0xff);\n", reg, reg, from);
			break;
		case sz_word:
			out("m68k_dreg(regs, %s) = (m68k_dreg(regs, %s) & ~0xffff) | ((%s) & 0xffff);\n", reg, reg, from);
			break;
		case sz_long:
			out("m68k_dreg(regs, %s) = (%s);\n", reg, from);
			break;
		default:
			term ();
		}
		break;
	case Areg:
		switch (size) {
		case sz_word:
			out("m68k_areg(regs, %s) = (uae_s32)(uae_s16)(%s);\n", reg, from);
			break;
		case sz_long:
			out("m68k_areg(regs, %s) = (%s);\n", reg, from);
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
  {
		if (using_ce) {
			switch (size) {
			case sz_byte:
				out("%s(%sa, %s);\n", dstb, to, from);
				count_writew++;
				check_bus_error(to, 0, 1, 0, from, 1, pcoffset);
				break;
			case sz_word:
				if (cpu_level < 2 && (mode == PC16 || mode == PC8r))
					term();
				out("%s(%sa, %s);\n", dstw, to, from);
				count_writew++;
				check_bus_error(to, 0, 1, 1, from, 1, pcoffset);
				break;
			case sz_long:
				if (cpu_level < 2 && (mode == PC16 || mode == PC8r))
					term();
				if (store_dir) {
					out("%s(%sa + 2, %s);\n", dstw, to, from);
					count_writew++;
					check_bus_error(to, 2, 1, 1, from, 1, pcoffset);
					if (flags & GF_SECONDWORDSETFLAGS) {
						genflags(flag_logical, g_instr->size, "src", "", "");
					}
					// ADDX.L/SUBX.L -(an),-(an) only
					if (store_dir > 1) {
						fill_prefetch_next_after(0, NULL);
					}
          check_ipl(); 
          out("%s(%sa, %s >> 16);\n", dstw, to, from);
					sprintf(tmp, "%s >> 16", from);
					count_writew++;
					check_bus_error(to, 0, 1, 1, tmp, 1, pcoffset);
				} else {
					out("%s(%sa, %s >> 16);\n", dstw, to, from);
					sprintf(tmp, "%s >> 16", from);
					count_writew++;
					check_bus_error(to, 0, 1, 1, tmp, 1, pcoffset);
					if (flags & GF_SECONDWORDSETFLAGS) {
						genflags(flag_logical, g_instr->size, "src", "", "");
					}
          check_ipl();
					out("%s(%sa + 2, %s);\n", dstw, to, from);
					count_writew++;
					check_bus_error(to, 2, 1, 1, from, 1, pcoffset);
				}
				break;
			default:
				term();
			}
		} else if (using_prefetch) {
			switch (size) {
			case sz_byte:
				out("%s(%sa, %s);\n", dstb, to, from);
				count_writew++;
				check_bus_error(to, 0, 1, 0, from, 1, pcoffset);
				break;
			case sz_word:
				if (cpu_level < 2 && (mode == PC16 || mode == PC8r))
					term ();
				out("%s(%sa, %s);\n", dstw, to, from);
				count_writew++;
				check_bus_error(to, 0, 1, 1, from, 1, pcoffset);
				break;
			case sz_long:
				if (cpu_level < 2 && (mode == PC16 || mode == PC8r))
					term ();
				if (store_dir) {
					out("%s(%sa + 2, %s);\n", dstw, to, from);
					count_writew++;
					check_bus_error(to, 2, 1, 1, from, 1, pcoffset);
					if (flags & GF_SECONDWORDSETFLAGS) {
						genflags(flag_logical, g_instr->size, "src", "", "");
					}
					// ADDX.L/SUBX.L -(an),-(an) only
					if (store_dir > 1) {
						fill_prefetch_next_after(0, NULL);
					}
          check_ipl();
					out("%s(%sa, %s >> 16); \n", dstw, to, from);
					sprintf(tmp, "%s >> 16", from);
					count_writew++;
					check_bus_error(to, 0, 1, 1, tmp, 1, pcoffset);
				} else {
					out("%s(%sa, %s >> 16);\n", dstw, to, from);
					sprintf(tmp, "%s >> 16", from);
					count_writew++;
					check_bus_error(to, 0, 1, 1, tmp, 1, pcoffset);
					if (flags & GF_SECONDWORDSETFLAGS) {
						genflags(flag_logical, g_instr->size, "src", "", "");
					}
          check_ipl();
					out("%s(%sa + 2, %s); \n", dstw, to, from);
					count_writew++;
					check_bus_error(to, 2, 1, 1, from, 1, pcoffset);
				}
				break;
			default:
				term ();
			}
		} else {
			switch (size) {
			case sz_byte:
				out("%s(%sa, %s);\n", dstb, to, from);
				count_writew++;
				check_bus_error(to, 0, 1, 0, from, 1, pcoffset);
				break;
			case sz_word:
				if (cpu_level < 2 && (mode == PC16 || mode == PC8r))
					term ();
				out("%s(%sa, %s);\n", dstw, to, from);
				count_writew++;
				check_bus_error(to, 0, 1, 1, from, 1, pcoffset);
				break;
			case sz_long:
				if (cpu_level < 2 && (mode == PC16 || mode == PC8r))
					term ();
				// ADDX.L/SUBX.L -(an),-(an) only
				if (store_dir > 1) {
					insn_n_cycles += 4;
				}
				out("%s(%sa, %s);\n", dstl, to, from);
				count_writel++;
				check_bus_error(to, 0, 1, 2, from, 1, pcoffset);
				break;
			default:
				term ();
			}
		}
		break;
	}
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

static void genastore (const char *from, amodes mode, const char *reg, wordsizes size, const char *to)
{
	genastore_2 (from, mode, reg, size, to, 0, 0);
}
static void genastore_tas (const char *from, amodes mode, const char *reg, wordsizes size, const char *to)
{
	genastore_2 (from, mode, reg, size, to, 0, GF_LRMW);
}
static void genastore_cas (const char *from, amodes mode, const char *reg, wordsizes size, const char *to)
{
	genastore_2 (from, mode, reg, size, to, 0, GF_LRMW | GF_NOFAULTPC);
}
// write to addr + 2, write to addr + 0
static void genastore_rev (const char *from, amodes mode, const char *reg, wordsizes size, const char *to)
{
	genastore_2 (from, mode, reg, size, to, 1, 0);
}
// write to addr + 2, prefetch, write to addr + 0
static void genastore_rev_prefetch(const char *from, amodes mode, const char *reg, wordsizes size, const char *to)
{
	genastore_2(from, mode, reg, size, to, 2, 0);
}
static void genastore_fc (const char *from, amodes mode, const char *reg, wordsizes size, const char *to)
{
	genastore_2(from, mode, reg, size, to, 0, GF_FC);
}

static void movem_ex3(int write)
{
	if ((using_prefetch || using_ce) && using_exception_3) {
		if (write) {
			// MOVEM write to memory won't generate address error
			// exception if mask is zero and EA is odd.
			out("if ((amask || dmask) && (srca & 1)) {\n");
			// MOVE.L EA,-(An) causing address error: stacked value is original An - 2, not An - 4.
			if (g_instr->dmode == Apdi) {
				out("srca -= 2;\n");
      }
			out("uaecptr srcav = srca;\n");
			if (cpu_level == 1) {
				if (g_instr->dmode == Apdi) {
					out("if(amask) {\n");
					out("srcav = m68k_areg(regs, movem_index2[amask]);\n");
					out("} else if (dmask) {\n");
					out("srcav = m68k_dreg(regs, movem_index2[dmask]);\n");
					out("}\n");
				} else {
					int shift = g_instr->size == sz_long ? 16 : 0;
					out("if(dmask) {\n");
					out("srcav = m68k_dreg(regs, movem_index1[dmask]) >> %d;\n", shift);
					out("} else if (amask) {\n");
					out("srcav = m68k_areg(regs, movem_index1[amask]) >> %d;\n", shift);
					out("}\n");
				}
				if (g_instr->dmode == Aind || g_instr->dmode == Apdi || g_instr->dmode == Aipi) {
					out("regs.read_buffer = mask;\n");
				}
			}
		} else {
			// MOVEM from memory will generate address error
			// exception if mask is zero and EA is odd.
			out("if (srca & 1) {\n");
			if ((g_instr->dmode == PC16 || g_instr->dmode == PC8r) && cpu_level == 0) {
				out("opcode |= 0x00020000;\n");
			}
		}
		if (write) {
			incpc("%d", m68k_pc_offset + 2);
			out("exception3_write_access(opcode, srca, %d, srcav, %d);\n",
				g_instr->size,
				(g_instr->dmode == PC16 || g_instr->dmode == PC8r) ? 2 : 1);
		} else {
			int pcoff = 2;
			if (cpu_level == 0 && (g_instr->dmode == Ad8r || g_instr->dmode == PC8r)) {
				pcoff = -2;
			}
			incpc("%d", m68k_pc_offset + pcoff);
			out("exception3_read_access(opcode, srca, %d, %d);\n",
				g_instr->size,
				(g_instr->dmode == PC16 || g_instr->dmode == PC8r) ? 2 : 1);
		}
		returncycles_exception();
		out("}\n");
	}
}

static void genmovemel(uae_u16 opcode)
{
	char getcode[100];
	int size = table68k[opcode].size == sz_long ? 4 : 2;

	if (table68k[opcode].size == sz_long) {
		sprintf (getcode, "%s(srca)", srcld);
	} else {
		sprintf (getcode, "(uae_s32)(uae_s16)%s(srca)", srcwd);
	}
	out("uae_u16 mask = %s;\n", gen_nextiword(0));
	out("uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;\n");
  genamode(NULL, table68k[opcode].dmode, "dstreg", table68k[opcode].size, "src", 2, -1, GF_MOVE);
	movem_ex3(0);
	out("while (dmask) {\n");
  out("m68k_dreg(regs, movem_index1[dmask]) = %s;\n", getcode);
	if (cpu_level <= 3) {
    addcycles000_nonce(cpu_level <= 1 ? size * 2 : 4);
	}
	out("srca += %d;\n", size);
	out("dmask = movem_next[dmask];\n");
	out("}\n");
	out("while (amask) {\n");
  out("m68k_areg(regs, movem_index1[amask]) = %s;\n", getcode);
	if (cpu_level <= 3) {
    addcycles000_nonce(cpu_level <= 1 ? size * 2 : 4);
  }
	out("srca += %d;\n", size);
	out("amask = movem_next[amask];\n");
	out("}\n");
	if (table68k[opcode].dmode == Aipi) {
		out("m68k_areg(regs, dstreg) = srca;\n");
	}
	if (cpu_level <= 1) {
  	count_readw++;
  }
	if (!next_level_040_to_030())
		next_level_020_to_010();
	count_ncycles++;
	fill_prefetch_next_t();
}

static void genmovemel_ce(uae_u16 opcode)
{
	int size = table68k[opcode].size == sz_long ? 4 : 2;
	amodes mode = table68k[opcode].dmode;
	out("uae_u16 mask = %s;\n", gen_nextiword(mode < Ad16 ? GF_PCM2 : 0));
	do_instruction_buserror();
	out("uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;\n");
	if (mode == Ad8r || mode == PC8r) {
		addcycles000 (2);
  }
  genamode(NULL, mode, "dstreg", table68k[opcode].size, "src", 2, -1, GF_AA | GF_MOVE | GF_PCM2);
	movem_ex3(0);
	if (table68k[opcode].size == sz_long) {
		out("while (dmask) {\n");
	  out("uae_u32 v = (%s(srca) << 16) | (m68k_dreg(regs, movem_index1[dmask]) & 0xffff);\n", srcw);
    addcycles000_nonce(4);
		check_bus_error("src", 0, 0, 1, NULL, 1, -1);
		if (cpu_level == 0) {
			// 68010 does not do partial updates
      out("m68k_dreg(regs, movem_index1[dmask]) = v;\n");
		}
	  out("v &= 0xffff0000;\n");
	  out("v |= %s(srca + 2);\n", srcw);
 		addcycles000_nonce(4);
		check_bus_error("src", 2, 0, 1, NULL, 1, -1);
	  out("m68k_dreg(regs, movem_index1[dmask]) = v;\n");
		out("srca += %d;\n", size);
		out("dmask = movem_next[dmask];\n");
		out("}\n");
		out("while (amask) {\n");
	  out("uae_u32 v = (%s(srca) << 16) | (m68k_areg(regs, movem_index1[amask]) & 0xffff);\n", srcw);
    addcycles000_nonce(4);
		check_bus_error("src", 0, 0, 1, NULL, 1, -1);
		if (cpu_level == 0) {
      out("m68k_areg(regs, movem_index1[amask]) = v;\n");
		}
	  out("v &= 0xffff0000;\n");
	  out("v |= %s(srca + 2);\n", srcw);
    addcycles000_nonce(4);
		check_bus_error("src", 2, 0, 1, NULL, 1, -1);
	  out("m68k_areg(regs, movem_index1[amask]) = v;\n");
		out("srca += %d;\n", size);
		out("amask = movem_next[amask];\n");
		out("}\n");
	} else {
		out("while (dmask) {\n");
		out("uae_u32 v = (uae_s32)(uae_s16)%s(srca);\n", srcw);
    addcycles000_nonce(4);
		check_bus_error("src", 0, 0, 1, NULL, 1, -1);
		out("m68k_dreg(regs, movem_index1[dmask]) = v;\n");
		out("srca += %d;\n", size);
		out("dmask = movem_next[dmask];\n");
		out("}\n");
		out("while (amask) {\n");
		out("uae_u32 v = (uae_s32)(uae_s16)%s(srca);\n", srcw);
    addcycles000_nonce(4);
		check_bus_error("src", 0, 0, 1, NULL, 1, -1);
		out("m68k_areg(regs, movem_index1[amask]) = v;\n");
		out("srca += %d;\n", size);
		out("amask = movem_next[amask];\n");
		out("}\n");
	}
  out("%s(srca);\n", srcw); // and final extra word fetch that goes nowhere..
	count_readw++;
	check_bus_error("src", 0, 0, 1, NULL, 1, -1);
	if (mode == Aipi) {
		out("m68k_areg(regs, dstreg) = srca;\n");
	}
  next_level_000();
	count_ncycles++;
	fill_prefetch_next_t();
}

static void genmovemle(uae_u16 opcode)
{
	char putcode[100];
	int size = table68k[opcode].size == sz_long ? 4 : 2;

	if (size == 4) {
		sprintf(putcode, "%s(srca", dstld);
	} else {
		sprintf(putcode, "%s(srca", dstwd);
	}
	out("uae_u16 mask = %s;\n", gen_nextiword(0));
	genamode(NULL, table68k[opcode].dmode, "dstreg", table68k[opcode].size, "src", 2, 1, GF_MOVE | GF_APDI);
	if (table68k[opcode].size == sz_long) {
		if (table68k[opcode].dmode == Apdi) {
			out("uae_u16 amask = mask & 0xff, dmask = (mask >> 8) & 0xff;\n");
			movem_ex3(1);
			out("while (amask) {\n");
      if(cpu_level >= 2)
        out("m68k_areg(regs, dstreg) = srca;\n");
      out("%s - 4, m68k_areg(regs, movem_index2[amask]));\n", putcode);
		  if (cpu_level <= 3) {
		    addcycles000_nonce(cpu_level <= 1 ? size * 2 : 3);
      }
			out("srca -= %d;\n", size);
			out("amask = movem_next[amask];\n");
			out("}\n");
			out("while (dmask) {\n");
      out("%s - 4, m68k_dreg(regs, movem_index2[dmask]));\n", putcode);
		  if (cpu_level <= 3) {
		    addcycles000_nonce(cpu_level <= 1 ? size * 2 : 3);
      }
			out("srca -= %d;\n", size);
			out("dmask = movem_next[dmask];\n");
			out("}\n");
			out("m68k_areg(regs, dstreg) = srca;\n");
		} else {
			out("uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;\n");
			movem_ex3(1);
			out("while (dmask) {\n");
      out("%s, m68k_dreg(regs, movem_index1[dmask]));\n", putcode);
		  if (cpu_level <= 3) {
        addcycles000_nonce(cpu_level <= 1 ? size * 2 : 3);
      }
			out("srca += %d;\n", size);
			out("dmask = movem_next[dmask];\n");
			out("}\n");
			out("while (amask) {\n");
      out("%s, m68k_areg(regs, movem_index1[amask]));\n", putcode);
		  if (cpu_level <= 3) {
        addcycles000_nonce(cpu_level <= 1 ? size * 2 : 3);
      }
			out("srca += %d;\n", size);
			out("amask = movem_next[amask];\n");
			out("}\n");
		}
	} else {
		if (table68k[opcode].dmode == Apdi) {
			out("uae_u16 amask = mask & 0xff, dmask = (mask >> 8) & 0xff;\n");
			movem_ex3(1);
      out("while (amask) {\n");
			out("srca -= %d;\n", size);
      if(cpu_level >= 2)
  			out("m68k_areg(regs, dstreg) = srca;\n");
      out("%s, m68k_areg(regs, movem_index2[amask]));\n", putcode);
		  if (cpu_level <= 3) {
        addcycles000_nonce(cpu_level <= 1 ? size * 2 : 3);
      }
			out("amask = movem_next[amask];\n");
			out("}\n");
			out("while (dmask) {\n");
			out("srca -= %d;\n", size);
			out("%s, m68k_dreg(regs, movem_index2[dmask]));\n", putcode);
		  if (cpu_level <= 3) {
        addcycles000_nonce(cpu_level <= 1 ? size * 2 : 3);
      }
			out("dmask = movem_next[dmask];\n");
			out("}\n");
			out("m68k_areg(regs, dstreg) = srca;\n");
		} else {
			out("uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;\n");
			movem_ex3(1);
			out("while (dmask) {\n");
			out("%s, m68k_dreg(regs, movem_index1[dmask]));\n", putcode);
		  if (cpu_level <= 3) {
        addcycles000_nonce(cpu_level <= 1 ? size * 2 : 3);
      }
			out("srca += %d;\n", size);
			out("dmask = movem_next[dmask];\n");
			out("}\n");
			out("while (amask) {\n");
			out("%s, m68k_areg(regs, movem_index1[amask]));\n", putcode);
		  if (cpu_level <= 3) {
        addcycles000_nonce(cpu_level <= 1 ? size * 2 : 3);
      }
			out("srca += %d;\n", size);
			out("amask = movem_next[amask];\n");
			out("}\n");
		}
	}
	if (!next_level_040_to_030())
		next_level_020_to_010();
	count_ncycles++;
	fill_prefetch_next_t();
}

static void genmovemle_ce (uae_u16 opcode)
{
	int size = table68k[opcode].size == sz_long ? 4 : 2;
  amodes mode = table68k[opcode].dmode;
	int pcoffset = 0;

	out("uae_u16 mask = %s;\n", gen_nextiword(mode >= Ad8r && mode != absw && mode != absl ? GF_PCM2 : ((mode == Ad16 || mode == PC16 || mode == absw || mode == absl) ? 0 : GF_PCP2)));
	do_instruction_buserror();
	if (mode == Ad8r || mode == PC8r) {
		addcycles000 (2);
  }
	strcpy(bus_error_code2, "pcoffset += 2;\n");
	genamode (NULL, mode, "dstreg", table68k[opcode].size, "src", 2, 1, GF_AA | GF_MOVE | GF_REVERSE | GF_REVERSE2 | (mode == absl ? GF_PCM2 : GF_PCP2));
	if (mode >= Ad16) {
		pcoffset = 2;
	}
	if (table68k[opcode].size == sz_long) {
		if (mode == Apdi) {
			out("uae_u16 amask = mask & 0xff, dmask = (mask >> 8) & 0xff;\n");
			movem_ex3(1);
			out("while (amask) {\n");
      if(cpu_level >= 2)
        out("m68k_areg(regs, dstreg) = srca;\n");
	    out("%s(srca - 2, m68k_areg(regs, movem_index2[amask]));\n", dstw);
      addcycles000_nonce(cpu_level > 1 ? 1 : 4);
			check_bus_error("src", -2, 1, 1, "m68k_areg(regs, movem_index2[amask])", 1, pcoffset);
	    out("%s(srca - 4, m68k_areg(regs, movem_index2[amask]) >> 16);\n", dstw);
      addcycles000_nonce(cpu_level > 1 ? 2 : 4);
			check_bus_error("src", -4, 1, 1, "m68k_areg(regs, movem_index2[amask]) >> 16", 1, pcoffset);
			out("srca -= %d;\n", size);
			out("amask = movem_next[amask];\n");
			out("}\n");
			out("while (dmask) {\n");
	    out("%s(srca - 2, m68k_dreg(regs, movem_index2[dmask]));\n", dstw);
      addcycles000_nonce(cpu_level > 1 ? 1 : 4);
			check_bus_error("src", -2, 1, 1, "m68k_dreg(regs, movem_index2[dmask])", 1, pcoffset);
	    out("%s(srca - 4, m68k_dreg(regs, movem_index2[dmask]) >> 16);\n", dstw);
      addcycles000_nonce(cpu_level > 1 ? 2 : 4);
			check_bus_error("src", -4, 1, 1, "m68k_dreg(regs, movem_index2[dmask]) >> 16", 1, pcoffset);
			out("srca -= %d;\n", size);
			out("dmask = movem_next[dmask];\n");
			out("}\n");
			out("m68k_areg(regs, dstreg) = srca;\n");
		} else {
			out("uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;\n");
			movem_ex3(1);
			out("while (dmask) {\n");
	    out("%s(srca, m68k_dreg(regs, movem_index1[dmask]) >> 16);\n", dstw);
      addcycles000_nonce(cpu_level > 1 ? 1 : 4);
			check_bus_error("src", 0, 1, 1, "m68k_dreg(regs, movem_index1[dmask]) >> 16", 1, pcoffset);
	    out("%s(srca + 2, m68k_dreg (regs, movem_index1[dmask]));\n", dstw);
      addcycles000_nonce(cpu_level > 1 ? 2 : 4);
			check_bus_error("src", 2, 1, 1, "m68k_dreg(regs, movem_index1[dmask])", 1, pcoffset);
			out("srca += %d;\n", size);
			out("dmask = movem_next[dmask];\n");
			out("}\n");
			out("while (amask) {\n");
	    out("%s(srca, m68k_areg(regs, movem_index1[amask]) >> 16);\n", dstw);
      addcycles000_nonce(cpu_level > 1 ? 1 : 4);
			check_bus_error("src", 0, 1, 1, "m68k_areg(regs, movem_index1[amask]) >> 16", 1, pcoffset);
	    out("%s(srca + 2, m68k_areg(regs, movem_index1[amask]));\n", dstw);
      addcycles000_nonce(cpu_level > 1 ? 2 : 4);
			check_bus_error("src", 2, 1, 1, "m68k_areg(regs, movem_index1[amask])", 1, pcoffset);
			out("srca += %d;\n", size);
			out("amask = movem_next[amask];\n");
			out("}\n");
		}
	} else {
		if (mode == Apdi) {
			out("uae_u16 amask = mask & 0xff, dmask = (mask >> 8) & 0xff;\n");
			movem_ex3(1);
      out("while (amask) {\n");
			out("srca -= %d;\n", size);
      if(cpu_level >= 2)
  			out("m68k_areg(regs, dstreg) = srca;\n");
      out("%s(srca, m68k_areg(regs, movem_index2[amask]));\n", dstw);
      addcycles000_nonce(cpu_level > 1 ? 3 : 4);
			check_bus_error("src", 0, 1, 1, "m68k_areg(regs, movem_index2[amask])", 1, pcoffset);
			out("amask = movem_next[amask];\n");
			out("}\n");
			out("while (dmask) {\n");
			out("srca -= %d;\n", size);
			out("%s(srca, m68k_dreg(regs, movem_index2[dmask]));\n", dstw);
      addcycles000_nonce(cpu_level > 1 ? 3 : 4);
			check_bus_error("src", 0, 1, 1, "m68k_dreg(regs, movem_index2[dmask])", 1, pcoffset);
			out("dmask = movem_next[dmask];\n");
			out("}\n");
			out("m68k_areg(regs, dstreg) = srca;\n");
		} else {
			out("uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;\n");
			movem_ex3(1);
			out("while (dmask) {\n");
			out("%s(srca, m68k_dreg(regs, movem_index1[dmask]));\n", dstw);
      addcycles000_nonce(cpu_level > 1 ? 3 : 4);
			check_bus_error("src", 0, 1, 1, "m68k_dreg(regs, movem_index1[dmask])", 1, pcoffset);
			out("srca += %d;\n", size);
			out("dmask = movem_next[dmask];\n");
			out("}\n");
			out("while (amask) {\n");
			out("%s(srca, m68k_areg(regs, movem_index1[amask]));\n", dstw);
      addcycles000_nonce(cpu_level > 1 ? 3 : 4);
			check_bus_error("src", 0, 1, 1, "m68k_areg(regs, movem_index1[amask])", 1, pcoffset);
			out("srca += %d;\n", size);
			out("amask = movem_next[amask];\n");
			out("}\n");
		}
	}
	count_ncycles++;
	fill_prefetch_next_t();
}

static void force_range_for_rox (const char *var, wordsizes size)
{
	/* Could do a modulo operation here... which one is faster? */
	switch (size) {
	case sz_long:
		out("if (%s >= 33) %s -= 33;\n", var, var);
		break;
	case sz_word:
		out("if (%s >= 34) %s -= 34;\n", var, var);
		out("if (%s >= 17) %s -= 17;\n", var, var);
		break;
	case sz_byte:
		out("if (%s >= 36) %s -= 36;\n", var, var);
		out("if (%s >= 18) %s -= 18;\n", var, var);
		out("if (%s >= 9) %s -= 9;\n", var, var);
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
			out("{\n");
			out("int cycles = %d;\n", c);
			out("cycles += 2 * ccnt;\n");
			addcycles000_3();
			out("}\n");
		}
		next_level_020_to_010();
		if ((using_simple_cycles) && cpu_level <= 1) {
		  addcycles000_nonces("2 * ccnt");
		}
		count_cycles += c;
		insn_n_cycles += c;
		count_ncycles++;
	}
}

// BCHG/BSET/BCLR Dx,Dx or #xx,Dx adds 2 cycles if bit number > 15 
static void bsetcycles (struct instr *curi)
{
	if (curi->size == sz_byte) {
		out("src &= 7;\n");
	} else {
		out("src &= 31;\n");
		if (isreg (curi->dmode)) {
			addcycles000 (2);
			if (curi->mnemo != i_BTST) {
				if (using_ce) {
					out("if (src > 15) %s(2);\n", do_cycles);
				}
    		next_level_020_to_010();
				if ((using_simple_cycles) && cpu_level <= 1) {
          out("if (src > 15)  {\n");
					out("count_cycles += % d * CYCLE_UNIT / 2;\n", 2);
					out("}\n");
					count_ncycles++;
				}
			}
		}
	}
}

static int islongimm (struct instr *curi)
{
	return (curi->size == sz_long && (curi->smode == Dreg || curi->smode == imm || curi->smode == Areg));
}

static void exception_cpu(const char *s)
{
	if (need_exception_oldpc) {
		out("Exception_cpu_oldpc(%s,oldpc);\n", s);
	} else {
		out("Exception_cpu(%s);\n", s);
	}
}
static void exception_oldpc(void)
{
	if (need_exception_oldpc) {
		out("uaecptr oldpc = %s;\n", getpc);
	}
}

static void resetvars (void)
{
	insn_n_cycles = 0;
	genamode_cnt = 0;
	genamode8r_offset[0] = genamode8r_offset[1] = 0;
	m68k_pc_total = 0;
	branch_inst = 0;
	set_fpulimit = 0;
	bus_error_cycles = 0;
	exception_pc_offset = 0;
	exception_pc_offset_extra_000 = 0;

	ir2irc = 0;
	
	prefetch_long = NULL;
	prefetch_opcode = NULL;
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

	if (using_indirect > 0) {
		// tracer
		getpc = "m68k_getpci()";
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
		do_cycles = "do_cycles_ce000_internal";
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
		getpc = "m68k_getpci()";
	} else if (using_prefetch) {
		// 68000 prefetch
		prefetch_word = "get_word_000_prefetch";
		prefetch_long = "get_long_000_prefetch";
		srcwi = "get_wordi_000";
		srcl = "get_long_000";
		dstl = "put_long_000";
		srcw = "get_word_000";
		dstw = "put_word_000";
		srcb = "get_byte_000";
		dstb = "put_byte_000";
		getpc = "m68k_getpci ()";
	} else {
		// generic + direct
		prefetch_long = "get_dilong";
		prefetch_word = "get_diword";
		nextw = "next_diword";
		nextl = "next_dilong";
		srcli = "get_dilong";
		srcwi = "get_diword";
		srcbi = "get_dibyte";
		if (using_indirect < 0) {
			srcl = "get_long_jit";
			dstl = "put_long_jit";
			srcw = "get_word_jit";
			dstw = "put_word_jit";
			srcb = "get_byte_jit";
			dstb = "put_byte_jit";
		} else {
		  srcl = "get_long";
		  dstl = "put_long";
		  srcw = "get_word";
		  dstw = "put_word";
		  srcb = "get_byte";
		  dstb = "put_byte";
	  }
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
	if (!prefetch_opcode)
		prefetch_opcode = prefetch_word;
}

static void gen_opcode (unsigned int opcode)
{
	struct instr *curi = table68k + opcode;

	resetvars ();
	
	m68k_pc_offset = 2;
	g_instr = curi;
	g_srcname[0] = 0;
	bus_error_code[0] = 0;
	bus_error_code2[0] = 0;
  last_access_offset_ipl = -1;

	if (cpu_level == 1 && (using_ce || using_prefetch)) {
		if (curi->mnemo == i_DBcc) {
			next_level_000();
		}
	}

	// do not unnecessarily create useless mmuop030
	// functions when CPU is not 68030
	if (curi->mnemo == i_MMUOP030 && cpu_level != 3 && !cpu_generic) {
		out("op_illg(opcode);\n");
		did_prefetch = -1;
		goto end;
	}

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
		out(
			"if (!regs.s) {\n"
			"Exception(8);\n");
		returncycles_exception();
		out("}\n");
		break;
	case 3: /* privileged if size == word */
		if (curi->size == sz_byte)
			break;
		out(
			"if (!regs.s) {\n"
			"Exception(8);\n");
		returncycles_exception();
		out("}\n");
		break;
	}
	switch (curi->mnemo) {
	case i_OR:
	case i_AND:
	case i_EOR:
	{
		// documentation error: and.l #imm,dn = 2 idle, not 1 idle (same as OR and EOR)
		int c = 0;
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "src", 1, 0,
			curi->dmode, "dstreg", curi->size, "dst", 1, GF_RMW);
		out("src %c= dst;\n", curi->mnemo == i_OR ? '|' : curi->mnemo == i_AND ? '&' : '^');
		genflags (flag_logical, curi->size, "src", "", "");
		if (curi->size == sz_long) {
			if (curi->dmode == Dreg) {
			  c += 2;
			  if (curi->smode == imm || curi->smode == Dreg) {
					if (cpu_level == 0) {
				    c += 2;
          }
					if (cpu_level == 1 && curi->smode == imm) {
						c += 2;
					}
					if (cpu_level == 1 && (curi->smode == imm || curi->smode == Dreg)) {
						fill_prefetch_next_after(0, "m68k_dreg(regs, dstreg) = (src);\n");
					} else {
						fill_prefetch_next_after(1, "ccr_68000_long_move_ae_LZN(src);\ndreg_68000_long_replace_low(dstreg, src);\n");
					}
				} else {
					if (cpu_level == 1) {
						fill_prefetch_next_after(0, "m68k_dreg(regs, dstreg) = (src);\n");
					} else {
						fill_prefetch_next_after(1, "dreg_68000_long_replace_low(dstreg, src);\n");
					}
        }
				next_level_000();
			} else {
				fill_prefetch_next_after(0, "ccr_68000_long_move_ae_LZN(src);\n");
		  }
		  if (c > 0)
			  addcycles000 (c);
		  genastore_rev ("src", curi->dmode, "dstreg", curi->size, "dst");
		} else {
			if (curi->dmode == Dreg) {
				genastore_rev("src", curi->dmode, "dstreg", curi->size, "dst");
			}
			if ((curi->smode == imm || curi->smode == Dreg) && curi->dmode != Dreg) {
				fill_prefetch_next_after(1, NULL);
			} else {
        fill_prefetch_next_t();
      }
			if (c > 0)
				addcycles000(c);
			if (curi->dmode != Dreg) {
				genastore_rev("src", curi->dmode, "dstreg", curi->size, "dst");
			}
    }
		break;
	}
	// all SR/CCR modifications do full prefetch
	case i_ORSR:
	case i_ANDSR:
	case i_EORSR:
		out("MakeSR();\n");
		if (cpu_level == 0 && using_prefetch) {
			out("int t1 = regs.t1;\n");
		}
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, cpu_level == 1 ? GF_NOREFILL : 0);
		if (curi->size == sz_byte) {
			if (curi->mnemo == i_ANDSR)
				out("src |= 0xff00;\n");
      else
  			out("src &= 0xFF;\n");
		} else {
			check_trace();
		}
		addcycles000 (8);
		out("regs.sr %c= src;\n", curi->mnemo == i_ORSR ? '|' : curi->mnemo == i_ANDSR ? '&' : '^');
		if (cpu_level < 5 && curi->size == sz_word) {
		  makefromsr_t0();
		} else {
			makefromsr();
		}
		sync_m68k_pc ();
		if (cpu_level < 2 || curi->size == sz_word) {
			fill_prefetch_full_ntx(3);
		} else {
			fill_prefetch_next();
		}
		next_cpu_level = cpu_level - 1;
		break;
	case i_SUB:
	{
		int c = 0;
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "src", 1, 0,
			curi->dmode, "dstreg", curi->size, "dst", 1, GF_RMW);
		genflags(flag_sub, curi->size, "newv", "src", "dst");
		if (curi->size == sz_long) {
			if (curi->dmode == Dreg) {
				c += 2;
				if (curi->smode == imm || curi->smode == immi || curi->smode == Dreg || curi->smode == Areg) {
					if (cpu_level == 0) {
						c += 2;
					}
					if (cpu_level == 1 && curi->smode == immi) {
						c += 2;
					}
					fill_prefetch_next_after(1,
						"uae_s16 bnewv = (uae_s16)dst - (uae_s16)src;\n"
						"int bflgs = ((uae_s16)(src)) < 0;\n"
						"int bflgo = ((uae_s16)(dst)) < 0;\n"
						"int bflgn = bnewv < 0;\n"
						"ccr_68000_long_move_ae_LZN(bnewv);\n"
						"SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));\n"
						"SET_XFLG(GET_CFLG());\n"
						"SET_VFLG((bflgs ^ bflgo) & (bflgn ^ bflgo));\n"
						"dreg_68000_long_replace_low(dstreg, bnewv);\n");
				} else {
					fill_prefetch_next_after(1, "dreg_68000_long_replace_low(dstreg, newv);\n");
        }
				next_level_000();
			} else {
				fill_prefetch_next_after(0,
					"uae_s16 bnewv = (uae_s16)dst - (uae_s16)src;\n"
					"int bflgs = ((uae_s16)(src)) < 0;\n"
					"int bflgo = ((uae_s16)(dst)) < 0;\n"
					"int bflgn = bnewv < 0;\n"
					"ccr_68000_long_move_ae_LZN(bnewv);\n"
					"SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));\n"
					"SET_XFLG(GET_CFLG());\n"
					"SET_VFLG((bflgs ^ bflgo) & (bflgn ^ bflgo));\n");
			}
		  if (c > 0) {
			  addcycles000 (c);
			}
		  genastore_rev ("newv", curi->dmode, "dstreg", curi->size, "dst");
    } else {
			if (curi->dmode == Dreg) {
				genastore_rev("newv", curi->dmode, "dstreg", curi->size, "dst");
			}
			if ((curi->smode >= imm || curi->smode == Dreg) && curi->dmode != Dreg) {
				fill_prefetch_next_after(1, NULL);
			} else {
        fill_prefetch_next_t();
      }
			if (c > 0) {
				addcycles000(c);
			}
			if (curi->dmode != Dreg) {
				genastore_rev("newv", curi->dmode, "dstreg", curi->size, "dst");
			}
    }
		break;
	}
	case i_SUBA:
	{
		int c = 0;
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "src", 1, 0,
			curi->dmode, "dstreg", sz_long, "dst", 1, GF_RMW);
		out("uae_u32 newv = dst - src;\n");
    if (curi->smode == immi) {
			// SUBAQ.x is always 8 cycles
			c += 4;
		} else {
			c = curi->size == sz_long ? 2 : 4;
			if (islongimm (curi)) {
				c += 2;
      }
		}
		fill_prefetch_next_after(0, "areg_68000_long_replace_low(dstreg, newv);\n");
		if (c > 0) {
			addcycles000 (c);
    }
		genastore ("newv", curi->dmode, "dstreg", sz_long, "dst");
		break;
	}
	case i_SUBX:
		exception_pc_offset_extra_000 = 2;
		next_level_000();
		if (!isreg (curi->smode))
			addcycles000 (2);
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, GF_AA | GF_REVERSE);
		genamode (curi, curi->dmode, "dstreg", curi->size, "dst", 1, 0, GF_AA | GF_REVERSE | GF_RMW | GF_SECONDEA);
		out("uae_u32 newv = dst - src - (GET_XFLG () ? 1 : 0);\n");
    if (cpu_level >= 2) {
      genflags (flag_subx, curi->size, "newv", "src", "dst");
      genflags (flag_zn, curi->size, "newv", "", "");
		  fill_prefetch_next ();
      genastore ("newv", curi->dmode, "dstreg", curi->size, "dst");
		} else {
			if (curi->size != sz_long) {
        genflags (flag_subx, curi->size, "newv", "src", "dst");
        genflags (flag_zn, curi->size, "newv", "", "");
			} else {
				if (using_prefetch)
          out("int oldz = GET_ZFLG();\n");
			}
			if (isreg(curi->smode) && curi->size != sz_long) {
        genastore ("newv", curi->dmode, "dstreg", curi->size, "dst");
      }
      if (curi->size == sz_long) {
  		  genflags (flag_subx, curi->size, "newv", "src", "dst");
        genflags (flag_zn, curi->size, "newv", "", "");
      }
			if (isreg(curi->smode)) {
				if (curi->size == sz_long) {
					// set CCR using only low word if prefetch bus error
					fill_prefetch_next_after(1,
						"int bflgs = ((uae_s16)(src)) < 0;\n"
						"int bflgo = ((uae_s16)(dst)) < 0;\n"
						"int bflgn = ((uae_s16)(newv)) < 0;\n"
						"SET_VFLG((bflgs ^ bflgo) & (bflgo ^ bflgn));\n"
						"SET_CFLG(bflgs ^ ((bflgs ^ bflgn) & (bflgo ^ bflgn)));\n"
						"SET_XFLG(GET_CFLG());\n"
						"SET_ZFLG(oldz);\n"
						"if (newv & 0xffff) SET_ZFLG(0);\n"
						"SET_NFLG(newv & 0x8000); \n"
						"dreg_68000_long_replace_low(dstreg, newv);\n");
				} else {
					fill_prefetch_next_t();
				}
			} else if (curi->size == sz_long) {
				if (isreg(curi->smode)) {
					fill_prefetch_next_after(0, NULL);
				}
			} else {
				fill_prefetch_next_after(1, NULL);
			}
		  if (curi->size == sz_long && isreg(curi->smode)) {
				if (cpu_level == 0)
		      addcycles000(4);
				else
					addcycles000(2);
				next_level_000();
      }
		  exception_pc_offset_extra_000 = 0;
			if (curi->size == sz_long && !isreg(curi->dmode)) {
				// write addr + 2
				// prefetch
				// write addr + 0
				genastore_rev_prefetch("newv", curi->dmode, "dstreg", curi->size, "dst");
			} else if (!isreg(curi->smode) || curi->size == sz_long) {
		    genastore ("newv", curi->dmode, "dstreg", curi->size, "dst");
			}
		}
		break;
	case i_SBCD:
		exception_pc_offset_extra_000 = 2;
		if (!isreg (curi->smode))
			addcycles000 (2);
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, GF_AA);
		genamode (curi, curi->dmode, "dstreg", curi->size, "dst", 1, 0, GF_AA | GF_RMW);
		out("uae_u16 newv_lo = (dst & 0xF) - (src & 0xF) - (GET_XFLG() ? 1 : 0);\n");
		out("uae_u16 newv_hi = (dst & 0xF0) - (src & 0xF0);\n");
		out("uae_u16 newv, tmp_newv;\n");
		out("int bcd = 0;\n");
		out("newv = tmp_newv = newv_hi + newv_lo;\n");
		out("if (newv_lo & 0xF0) { newv -= 6; bcd = 6; };\n");
		out("if ((((dst & 0xFF) - (src & 0xFF) - (GET_XFLG() ? 1 : 0)) & 0x100) > 0xFF) { newv -= 0x60; }\n");
		out("SET_CFLG((((dst & 0xFF) - (src & 0xFF) - bcd - (GET_XFLG() ? 1 : 0)) & 0x300) > 0xFF);\n");
		duplicate_carry();
		/* Manual says bits NV are undefined though a real 68030 doesn't change V and 68040 don't change both */
		if (cpu_level >= xBCD_KEEPS_N_FLAG) {
			if (next_cpu_level < xBCD_KEEPS_N_FLAG)
				next_cpu_level = xBCD_KEEPS_N_FLAG - 1;
			genflags (flag_z, curi->size, "newv", "", "");
		} else {
			genflags (flag_zn, curi->size, "newv", "", "");
		}
		if (cpu_level >= xBCD_KEEPS_V_FLAG) {
			if (next_cpu_level < xBCD_KEEPS_V_FLAG)
				next_cpu_level = xBCD_KEEPS_V_FLAG - 1;
			if (cpu_level >= xBCD_KEEPS_V_FLAG && cpu_level < xBCD_KEEPS_N_FLAG)
				out("SET_VFLG(0);\n");
		} else {
			out("SET_VFLG((tmp_newv & 0x80) != 0 && (newv & 0x80) == 0);\n");
		}
		fill_prefetch_next_after(1, NULL);
		if (isreg (curi->smode)) {
			addcycles000 (2);
		}
		exception_pc_offset_extra_000 = 0;
		genastore ("newv", curi->dmode, "dstreg", curi->size, "dst");
		break;
	case i_ADD:
	{
		int c = 0;
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "src", 1, 0,
			curi->dmode, "dstreg", curi->size, "dst", 1, GF_RMW);
		genflags (flag_add, curi->size, "newv", "src", "dst");
		if (curi->size == sz_long) {
		  if (curi->dmode == Dreg) {
				c += 2;
				if (curi->smode == imm || curi->smode == immi || curi->smode == Dreg || curi->smode == Areg) {
					if (cpu_level == 0) {
						c += 2;
					}
					if (cpu_level == 1 && curi->smode == immi) {
						// 68010 Immediate long instructions 2 cycles faster, Q variants have same speed.
						c += 2;
					}
					fill_prefetch_next_after(1,
						"uae_s16 bnewv = (uae_s16)dst + (uae_s16)src;\n"
						"int bflgs = ((uae_s16)(src)) < 0;\n"
						"int bflgo = ((uae_s16)(dst)) < 0;\n"
						"int bflgn = bnewv < 0;\n"
						"ccr_68000_long_move_ae_LZN(bnewv);\n"
						"SET_CFLG(((uae_u16)(~dst)) < ((uae_u16)(src)));\n"
						"SET_XFLG(GET_CFLG());\n"
						"SET_VFLG((bflgs ^ bflgn) & (bflgo ^ bflgn));\n"
						"dreg_68000_long_replace_low(dstreg, bnewv);\n");
				} else {
					fill_prefetch_next_after(1, "dreg_68000_long_replace_low(dstreg, newv);\n");
        }
				next_level_000();
			} else {
				fill_prefetch_next_after(0,
					"uae_s16 bnewv = (uae_s16)dst + (uae_s16)src;\n"
					"int bflgs = ((uae_s16)(src)) < 0;\n"
					"int bflgo = ((uae_s16)(dst)) < 0;\n"
					"int bflgn = bnewv < 0;\n"
					"ccr_68000_long_move_ae_LZN(bnewv);\n"
					"SET_CFLG(((uae_u16)(~dst)) < ((uae_u16)(src)));\n"
					"SET_XFLG(GET_CFLG());\n"
					"SET_VFLG((bflgs ^ bflgn) & (bflgo ^ bflgn));\n");
			}
   		if (c > 0)
      	addcycles000 (c);
   		genastore_rev ("newv", curi->dmode, "dstreg", curi->size, "dst");
    } else {
			if (curi->dmode == Dreg) {
				genastore_rev("newv", curi->dmode, "dstreg", curi->size, "dst");
			}
			if ((curi->smode >= imm || curi->smode == Dreg) && curi->dmode != Dreg) {
				fill_prefetch_next_after(1, NULL);
			} else {
		    fill_prefetch_next_t();
      }
		  if (c > 0)
			  addcycles000 (c);
			if (curi->dmode != Dreg) {
		    genastore_rev ("newv", curi->dmode, "dstreg", curi->size, "dst");
			}
    }
		break;
	}
	case i_ADDA:
	{
		int c = 0;
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "src", 1, 0,
			curi->dmode, "dstreg", sz_long, "dst", 1, GF_RMW);
		out("uae_u32 newv = dst + src;\n");
    if (curi->smode == immi) {
			// ADDAQ.x is always 8 cycles
			c += 4;
		} else {
			c = curi->size == sz_long ? 2 : 4;
			if (islongimm (curi)) {
				c += 2;
      }
		}
		fill_prefetch_next_after(1, "areg_68000_long_replace_low(dstreg, newv);\n");
		if (c > 0) {
			addcycles000 (c);
    }
		genastore ("newv", curi->dmode, "dstreg", sz_long, "dst");
		break;
	}
	case i_ADDX:
		exception_pc_offset_extra_000 = 2;
		next_level_000();
		if (!isreg (curi->smode)) {
			addcycles000 (2);
		}
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, GF_AA | GF_REVERSE);
		genamode (curi, curi->dmode, "dstreg", curi->size, "dst", 1, 0, GF_AA | GF_REVERSE | GF_RMW | GF_SECONDEA);
		out("uae_u32 newv = dst + src + (GET_XFLG() ? 1 : 0);\n");
		if (cpu_level >= 2) {
			genflags(flag_addx, curi->size, "newv", "src", "dst");
			genflags(flag_zn, curi->size, "newv", "", "");
			fill_prefetch_next();
			genastore("newv", curi->dmode, "dstreg", curi->size, "dst");
		} else {
			if (curi->size != sz_long) {
				genflags(flag_addx, curi->size, "newv", "src", "dst");
				genflags(flag_zn, curi->size, "newv", "", "");
			} else {
        if (using_prefetch)
				  out("int oldz = GET_ZFLG();\n");
			}
			if (isreg(curi->smode) && curi->size != sz_long) {
        genastore ("newv", curi->dmode, "dstreg", curi->size, "dst");
			}
			if (curi->size == sz_long) {
				genflags(flag_addx, curi->size, "newv", "src", "dst");
				genflags(flag_zn, curi->size, "newv", "", "");
			}
			if (isreg(curi->smode)) {
				if (curi->size == sz_long) {
					// set CCR using only low word if prefetch bus error
					fill_prefetch_next_after(1,
						"int bflgs = ((uae_s16)(src)) < 0;\n"
						"int bflgo = ((uae_s16)(dst)) < 0;\n"
						"int bflgn = ((uae_s16)(newv)) < 0;\n"
						"SET_VFLG((bflgs ^ bflgn) & (bflgo ^ bflgn));\n"
						"SET_CFLG(bflgs ^ ((bflgs ^ bflgo) & (bflgo ^ bflgn)));\n"
						"SET_XFLG(GET_CFLG());\n"
						"SET_ZFLG(oldz);\n"
						"if (newv & 0xffff) SET_ZFLG(0);\n"
						"SET_NFLG(newv & 0x8000); \n"
						"dreg_68000_long_replace_low(dstreg, newv);\n");
				} else {
					fill_prefetch_next_t();
				}
			} else if (curi->size == sz_long) {
				if (isreg(curi->smode)) {
					fill_prefetch_next_after(0, NULL);
				}
			} else {
				fill_prefetch_next_after(1, NULL);
			}
			if (curi->size == sz_long && isreg(curi->smode)) {
				if (cpu_level == 0)
		      addcycles000(4);
				else
					addcycles000(2);
				next_level_000();
			}
		exception_pc_offset_extra_000 = 0;
			if (curi->size == sz_long && !isreg(curi->dmode)) {
				// write addr + 2
				// prefetch
				// write addr + 0
				genastore_rev_prefetch("newv", curi->dmode, "dstreg", curi->size, "dst");
			} else if (!isreg(curi->smode) || curi->size == sz_long) {
		    genastore ("newv", curi->dmode, "dstreg", curi->size, "dst");
			}
		}
		break;
	case i_ABCD:
		exception_pc_offset_extra_000 = 2;
		if (!isreg (curi->smode))
			addcycles000 (2);
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, GF_AA);
		genamode (curi, curi->dmode, "dstreg", curi->size, "dst", 1, 0, GF_AA | GF_RMW);
		out("uae_u16 newv_lo = (src & 0xF) + (dst & 0xF) + (GET_XFLG() ? 1 : 0);\n");
		out("uae_u16 newv_hi = (src & 0xF0) + (dst & 0xF0);\n");
		out("uae_u16 newv, tmp_newv;\n");
		out("int cflg;\n");
		out("newv = tmp_newv = newv_hi + newv_lo;\n");
		out("if (newv_lo > 9) { newv += 6; }\n");
		out("cflg = (newv & 0x3F0) > 0x90;\n");
		out("if (cflg) newv += 0x60;\n");
		out("SET_CFLG(cflg);\n");
		duplicate_carry();
		/* Manual says bits NV are undefined though a real 68030 clears V and 68040 don't change both */
		if (cpu_level >= xBCD_KEEPS_N_FLAG) {
			if (next_cpu_level < xBCD_KEEPS_N_FLAG)
				next_cpu_level = xBCD_KEEPS_N_FLAG - 1;
			genflags (flag_z, curi->size, "newv", "", "");
		} else {
			genflags (flag_zn, curi->size, "newv", "", "");
		}
		if (cpu_level >= xBCD_KEEPS_V_FLAG) {
			if (next_cpu_level < xBCD_KEEPS_V_FLAG)
				next_cpu_level = xBCD_KEEPS_V_FLAG - 1;
			if (cpu_level >= xBCD_KEEPS_V_FLAG && cpu_level < xBCD_KEEPS_N_FLAG)
				out("SET_VFLG(0);\n");
		} else {
			out("SET_VFLG((tmp_newv & 0x80) == 0 && (newv & 0x80) != 0);\n");
		}
		fill_prefetch_next_after(1, NULL);
		if (isreg (curi->smode)) {
			addcycles000 (2);
		}
		exception_pc_offset_extra_000 = 0;
		genastore ("newv", curi->dmode, "dstreg", curi->size, "dst");
		break;
	case i_NEG:
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, GF_RMW);
    genflags (flag_sub, curi->size, "dst", "src", "0");
		if (curi->smode == Dreg) {
			if (curi->size == sz_long) {
				// prefetch bus error and long register: only low word is updated
				fill_prefetch_next_after(1, "dreg_68000_long_replace_low(srcreg, dst);\n");
        genastore_rev ("dst", curi->smode, "srcreg", curi->size, "src");
			} else {
        genastore_rev ("dst", curi->smode, "srcreg", curi->size, "src");
		    fill_prefetch_next_t();
			}
		} else if (curi->size == sz_long) {
			// prefetch bus error and long memory: only low word CCR decoded
			fill_prefetch_next_after(0,
				"int bflgs = ((uae_s16)(src)) < 0;\n"
				"int bflgo = ((uae_s16)(0)) < 0;\n"
				"int bflgn = ((uae_s16)(dst)) < 0;\n"
				"SET_ZFLG(((uae_s16)(dst)) == 0);\n"
				"SET_VFLG((bflgs ^ bflgo) & (bflgn ^ bflgo));\n"
				"SET_CFLG(((uae_u16)(src)) > ((uae_u16)(0)));\n"
				"SET_NFLG(bflgn != 0);\n"
				"SET_XFLG(GET_CFLG());\n");
		} else {
			fill_prefetch_next_after(1, NULL);
    }
		if (isreg (curi->smode) && curi->size == sz_long)
			addcycles000 (2);
		if (curi->smode != Dreg) {
		  genastore_rev ("dst", curi->smode, "srcreg", curi->size, "src");
		}
		break;
	case i_NEGX:
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, GF_RMW);
		out("uae_u32 newv = 0 - src - (GET_XFLG() ? 1 : 0);\n");
		genflags (flag_subx, curi->size, "newv", "src", "0");
		genflags (flag_zn, curi->size, "newv", "", "");
		if (curi->smode == Dreg) {
			if (curi->size == sz_long) {
				// prefetch bus error and long register: only low word is updated
				fill_prefetch_next_after(1, "dreg_68000_long_replace_low(srcreg, newv);\n");
        genastore_rev ("newv", curi->smode, "srcreg", curi->size, "src");
			} else {
    		genastore_rev ("newv", curi->smode, "srcreg", curi->size, "src");
        fill_prefetch_next_t();
			}
		} else if (curi->size == sz_long) {
			// prefetch bus error and long memory: only low word CCR decoded
			fill_prefetch_next_after(0,
				"int bflgs = ((uae_s16)(src)) < 0;\n"
				"int bflgo = ((uae_s16)(0)) < 0;\n"
				"int bflgn = ((uae_s16)(newv)) < 0;\n"
				"SET_VFLG((bflgs ^ bflgo) & (bflgo ^ bflgn));\n"
				"SET_CFLG(bflgs ^ ((bflgs ^ bflgn) & (bflgo ^ bflgn)));\n"
				"SET_ZFLG(GET_ZFLG() & (((uae_s16)(newv)) == 0));\n"
				"SET_NFLG(((uae_s16)(newv)) < 0);\n"
				"SET_XFLG(GET_CFLG());\n");
		} else {
			fill_prefetch_next_after(1, NULL);
		}
    if (isreg (curi->smode) && curi->size == sz_long)
      addcycles000 (2);
		if (curi->smode != Dreg) {
			genastore_rev("newv", curi->smode, "srcreg", curi->size, "src");
		}
		break;
	case i_NBCD:
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, GF_RMW);
		out("uae_u16 newv_lo = - (src & 0xF) - (GET_XFLG() ? 1 : 0);\n");
		out("uae_u16 newv_hi = - (src & 0xF0);\n");
		out("uae_u16 newv;\n");
		out("int cflg, tmp_newv;\n");
		out("tmp_newv = newv_hi + newv_lo;\n");
		out("if (newv_lo > 9) newv_lo -= 6;\n");
		out("newv = newv_hi + newv_lo;\n");
		out("cflg = (newv & 0x1F0) > 0x90;\n");
		out("if (cflg) newv -= 0x60;\n");
		out("SET_CFLG(cflg);\n");
		duplicate_carry();
		/* Manual says bits NV are undefined though a real 68030 doesn't change V and 68040 don't change both */
		if (cpu_level >= xBCD_KEEPS_N_FLAG) {
			if (next_cpu_level < xBCD_KEEPS_N_FLAG)
				next_cpu_level = xBCD_KEEPS_N_FLAG - 1;
			genflags (flag_z, curi->size, "newv", "", "");
		} else {
			genflags (flag_zn, curi->size, "newv", "", "");
		}
		if (cpu_level >= xBCD_KEEPS_V_FLAG) {
			if (next_cpu_level < xBCD_KEEPS_V_FLAG)
				next_cpu_level = xBCD_KEEPS_V_FLAG - 1;
			if (cpu_level >= xBCD_KEEPS_V_FLAG && cpu_level < xBCD_KEEPS_N_FLAG)
				out("SET_VFLG(0);\n");
		} else {
			out("SET_VFLG((tmp_newv & 0x80) != 0 && (newv & 0x80) == 0);\n");
		}
		if (isreg(curi->smode)) {
			fill_prefetch_next_after(1, NULL);
			addcycles000(2);
		} else {
			fill_prefetch_next_after(1, NULL);
		}
		genastore ("newv", curi->smode, "srcreg", curi->size, "src");
		break;
	case i_CLR:
		if (cpu_level == 0) {
			genamode(curi, curi->smode, "srcreg", curi->size, "src", 1, 0, 0);
      genflags(flag_logical, curi->size, "0", "", "");
			if (curi->smode == Dreg) {
				if (curi->size == sz_long) {
					// prefetch bus error and long register: only low word is updated
					// N flag from high word. Z both.
					fill_prefetch_next_after(1, 
						"m68k_dreg(regs, srcreg) = (src & 0xffff0000);\n"
						"SET_VFLG(0);SET_ZFLG(1);SET_NFLG(0);SET_CFLG(0);\n");
          genastore_rev("0", curi->smode, "srcreg", curi->size, "src");
				} else {
          genastore_rev("0", curi->smode, "srcreg", curi->size, "src");
		      fill_prefetch_next_t();
				}
			} else if (curi->size == sz_long) {
				// prefetch bus error and long memory: only low word CCR decoded
				fill_prefetch_next_after(0, "SET_VFLG(0);SET_ZFLG(1);SET_NFLG(0);SET_CFLG(0);\n");
			} else {
				fill_prefetch_next_after(1, NULL);
			}
			if (isreg(curi->smode) && curi->size == sz_long)
				addcycles000(2);
			if (curi->smode != Dreg) {
			  genastore_rev("0", curi->smode, "srcreg", curi->size, "src");
      }
		} else if (cpu_level == 1 && using_prefetch) {
			out("#if defined(CPU_i386) || defined(CPU_x86_64)\n");
			out("uae_u16 oldflags = regs.ccrflags.cznv;\n");
			out("#else\n");
			out("uae_u16 oldflags = regs.ccrflags.nzcv;\n");
			out("#endif\n");
			genamode(curi, curi->smode, "srcreg", curi->size, "src", 3, 0, GF_CLR68010);
			if (isreg(curi->smode) && curi->size == sz_long) {
				addcycles000(2);
      }
			if (!isreg(curi->smode) && using_exception_3 && curi->size != sz_byte && (using_prefetch || using_ce)) {
				out("if(srca & 1) {\n");
				if (curi->size == sz_word) {
					if (curi->smode == Aipi) {
						out("m68k_areg(regs, srcreg) -= 2;\n");
					} else if (curi->smode == Apdi) {
						out("m68k_areg(regs, srcreg) += 2;\n");
					} else if (curi->smode >= Ad16) {
						out("%s(%d);\n", prefetch_word, m68k_pc_offset + 2);
						genflags(flag_logical, curi->size, "0", "", "");
					}
				} else {
					if (curi->smode == Aipi) {
						out("m68k_areg(regs, srcreg) -= 4;\n");
					} else if (curi->smode == Apdi) {
						out("m68k_areg(regs, srcreg) += 4;\n");
						out("srca += 2;\n");
					} else if (curi->smode >= Ad16) {
						out("srca += 2;\n");
						out("%s(%d);\n", prefetch_word, m68k_pc_offset + 2);
						genflags(flag_logical, curi->size, "0", "", "");
					}
				}
				incpc("%d", m68k_pc_offset + 2);
				out("exception3_write(opcode, srca, 1, 0, 1);\n");
				returncycles_exception();
				out("}\n");
      }
			fill_prefetch_next_after(0, "CLEAR_CZNV();\nSET_ZFLG(1);\n");
			genflags(flag_logical, curi->size, "0", "", "");
			genastore_rev("0", curi->smode, "srcreg", curi->size, "src");
		} else {
			genamode(curi, curi->smode, "srcreg", curi->size, "src", 2, 0, 0);
			fill_prefetch_next();
		  if (isreg (curi->smode) && curi->size == sz_long)
			  addcycles000 (2);
		  genflags (flag_logical, curi->size, "0", "", "");
		  genastore_rev ("0", curi->smode, "srcreg", curi->size, "src");
		}
		next_level_000();
		break;
	case i_NOT:
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, GF_RMW);
    out("uae_u32 dst = ~src;\n");
    genflags (flag_logical, curi->size, "dst", "", "");
		if (curi->smode == Dreg) {
			if (curi->size == sz_long) {
				// prefetch bus error and long register: only low word is updated
				// N flag from high word. Z both.
				fill_prefetch_next_after(1, 
					"dreg_68000_long_replace_low(srcreg, dst);\n"
					"SET_VFLG(0);SET_ZFLG(!dst);\n"
					"SET_NFLG(dst & 0x80000000);\n"
					"SET_CFLG(0);\n");
        genastore_rev ("dst", curi->smode, "srcreg", curi->size, "src");
			} else {
        genastore_rev ("dst", curi->smode, "srcreg", curi->size, "src");
		    fill_prefetch_next_t();
      }
		} else if (curi->size == sz_long) {
			// prefetch bus error and long memory: only low word CCR decoded
			fill_prefetch_next_after(0, 
				"SET_VFLG(0);\n"
				"SET_ZFLG(!(dst & 0xffff));\n"
				"SET_NFLG(dst & 0x8000);\n"
				"SET_CFLG(0);\n");
		} else {
			fill_prefetch_next_after(1, NULL);
		}
		if (isreg(curi->smode) && curi->size == sz_long) {
			addcycles000 (2);
    }
		if (curi->smode != Dreg) {
		  genastore_rev ("dst", curi->smode, "srcreg", curi->size, "src");
		}
		break;
	case i_TST:
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, 0);
		genflags (flag_logical, curi->size, "src", "", "");
		fill_prefetch_next_t();
		break;
	case i_BTST:
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "src", 1, 0,
			curi->dmode, "dstreg", curi->size, "dst", 1, 0);
		if (curi->size == sz_long) {
			fill_prefetch_next_after(1, NULL);
		  bsetcycles (curi);
		  out("SET_ZFLG(1 ^ ((dst >> src) & 1));\n");
		} else {
			bsetcycles(curi);
			if (curi->dmode == imm) {
				// btst dn,#x
				fill_prefetch_next_after(1, NULL);
				out("SET_ZFLG(1 ^ ((dst >> src) & 1));\n");
			} else {
				out("SET_ZFLG(1 ^ ((dst >> src) & 1));\n");
				fill_prefetch_next_t();
			}
		}
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
		if (curi->size == sz_long) {
			fill_prefetch_next_after(1, NULL);
		} else {
			if (curi->smode == Dreg || curi->smode >= imm) {
				fill_prefetch_next_after(1, NULL);
			}
		}
		bsetcycles (curi);
		// bclr needs 1 extra cycle
		if (curi->mnemo == i_BCLR && curi->dmode == Dreg) {
			addcycles000 (2);
		}
		if (curi->mnemo == i_BCLR && curi->size == sz_byte) {
			if (cpu_level == 1) {
				// 68010 BCLR.B is 2 cycles slower
				addcycles000(2);
			}
			next_level_000();
    }
		if (curi->mnemo == i_BCHG) {
			out("dst ^= (1 << src);\n");
			out("SET_ZFLG(((uae_u32)dst & (1 << src)) >> src);\n");
		} else if (curi->mnemo == i_BCLR) {
			out("SET_ZFLG(1 ^ ((dst >> src) & 1));\n");
			out("dst &= ~(1 << src);\n");
		} else if (curi->mnemo == i_BSET) {
			out("SET_ZFLG(1 ^ ((dst >> src) & 1));\n");
			out("dst |= (1 << src);\n");
		}
		if (curi->size != sz_long) {
			if (curi->smode < imm && curi->smode != Dreg) {
				fill_prefetch_next_t();
			}
		}
		genastore ("dst", curi->dmode, "dstreg", curi->size, "dst");
		break;
	case i_CMPM:
		exception_pc_offset_extra_000 = 2;
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "src", 1, GF_AA,
			curi->dmode, "dstreg", curi->size, "dst", 1, GF_AA);
		genflags (flag_cmp, curi->size, "newv", "src", "dst");
		fill_prefetch_next_t();
		break;
	case i_CMP:
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "src", 1, 0,
			curi->dmode, "dstreg", curi->size, "dst", 1, 0);
		genflags (flag_cmp, curi->size, "newv", "src", "dst");
		if (curi->dmode == Dreg && curi->size == sz_long) {
			fill_prefetch_next_after(1, NULL);
			addcycles000 (2);
		} else {
			fill_prefetch_next_t();
    }
		break;
	case i_CMPA:
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "src", 1, 0,
			curi->dmode, "dstreg", sz_long, "dst", 1, 0);
    genflags (flag_cmp, sz_long, "newv", "src", "dst");
		if (curi->dmode == Areg) {
			fill_prefetch_next_after(1, NULL);
		} else {
		fill_prefetch_next_t();
		}
		addcycles000 (2);
		break;
		/* The next two are coded a little unconventional, but they are doing
		* weird things... */
	case i_MVPRM: // MOVEP R->M
		genamode(curi, curi->smode, "srcreg", curi->size, "src", 1, 0, cpu_level == 1 ? GF_NOFETCH : 0);
		out("uaecptr mempa = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)%s;\n", gen_nextiword (0));
		check_prefetch_buserror(m68k_pc_offset, -2);
		if (curi->size == sz_word) {
			out("%s(mempa, src >> 8);\n", dstb);
			count_writew++;
			check_bus_error("memp", 0, 1, 0, "src >> 8", 1 | 0x10000, 2);
			out("%s(mempa + 2, src); \n", dstb);
			count_writew++;
			check_bus_error("memp", 2, 1, 0, "src", 1, 2);
		} else {
			out("%s(mempa, src >> 24);\n", dstb);
			count_writew++;
			check_bus_error("memp", 0, 1, 0, "src >> 24", 1 | 0x10000, 2);
			out("%s(mempa + 2, src >> 16);\n", dstb);
			count_writew++;
			check_bus_error("memp", 2, 1, 0, "src >> 16", 1, 2);
			out("%s(mempa + 4, src >> 8);\n", dstb);
			count_writew++;
			check_bus_error("memp", 4, 1, 0, "src >> 8", 1 | 0x10000, 2);
			out("%s(mempa + 6, src); \n", dstb);
			count_writew++;
			check_bus_error("memp", 6, 1, 0, "src", 1, 2);
		}
		fill_prefetch_next_t();
		next_level_000();
		break;
	case i_MVPMR: // MOVEP M->R
		out("uaecptr mempa = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)%s;\n", gen_nextiword (0));
		check_prefetch_buserror(m68k_pc_offset, -2);
		genamode(curi, curi->dmode, "dstreg", curi->size, "dst", 2, 0,  cpu_level == 1 ? GF_NOFETCH :0);
		if (curi->size == sz_word) {
			out("uae_u16 val  = (%s(mempa) & 0xff) << 8;\n", srcb);
			count_readw++;
			check_bus_error("memp", 0, 0, 0, NULL, 1 | 0x10000, 2);
			out("val |= (%s(mempa + 2) & 0xff);\n", srcb);
			count_readw++;
			check_bus_error("memp", 2, 0, 0, NULL, 1, 2);
		} else {
			out("uae_u32 val  = (%s(mempa) & 0xff) << 24;\n", srcb);
			count_readw++;
			check_bus_error("memp", 0, 0, 0, NULL, 1 | 0x10000, 2);
			out("val |= (%s(mempa + 2) & 0xff) << 16;\n", srcb);
			count_readw++;
			check_bus_error("memp", 2, 0, 0, NULL, 1, 2);

			// upper word gets updated after two bytes (makes only difference if bus error is possible)
			if (cpu_level <= 1) {
			  out("m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & 0x0000ffff) | val;\n");
			}

			out("val |= (%s(mempa + 4) & 0xff) << 8;\n", srcb);
			count_readw++;
			check_bus_error("memp", 4, 0, 0, NULL, 1 | 0x10000, 2);
			out("val |= (%s(mempa + 6) & 0xff);\n", srcb);
			count_readw++;
			check_bus_error("memp", 6, 0, 0, NULL, 1, 2);
		}
		genastore ("val", curi->dmode, "dstreg", curi->size, "dst");
		fill_prefetch_next_t();
		next_level_000();
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
			if (cpu_level < 2) {

				int prefetch_done = 0;
				int flags = 0;
			  int dualprefetch = curi->dmode == absl && (curi->smode != Dreg && curi->smode != Areg && curi->smode != imm);

				if (curi->size == sz_long) {
					if (curi->smode >= Aind && curi->smode != absw) {
						flags |= GF_PCM2;
					} else {
						flags |= GF_PCM2 | GF_PCP2;
					}
				} else {
					if (curi->smode >= Aind && curi->smode != absw && curi->smode != imm) {
						flags |= GF_PCM2;
					} else {
						flags |= GF_PCM2 | GF_PCP2;
					}
				}

			  genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, flags);
			
				flags = 0;
				// unexpectedly MOVEA vs MOVE have different prefetch bus error stack frame PC field behavior.
				if (curi->mnemo == i_MOVE) {
					if ((curi->smode == imm && curi->dmode == absl)
						|| (curi->smode >= Ad16 && curi->smode != imm && curi->smode != absw && curi->smode != absl && curi->dmode < Ad16)
						|| (curi->smode == absl && curi->dmode < Ad16)
						|| ((curi->smode == Areg || curi->smode == Dreg) && curi->dmode == absl)) {
						flags |= GF_PCM2;
					} else {
						flags |= GF_PCM2 | GF_PCP2;
					}
				}

				flags |= GF_MOVE | GF_APDI;
			  flags |= dualprefetch ? GF_NOREFILL : 0;
			  if (curi->dmode == Apdi && curi->size == sz_long)
				  flags |= GF_REVERSE;

				// prefetch bus error support
				if (curi->mnemo == i_MOVE) {
					if (curi->size == sz_long) {
						if (curi->smode == imm) {
							if (curi->dmode == absw) {
								strcpy(bus_error_code, "ccr_68000_long_move_ae_LZN(src);\n");
							} else if (curi->dmode == absl) {
								strcpy(bus_error_code2, "ccr_68000_long_move_ae_LZN(src);\n");
							}
						} else if (curi->smode == Dreg || curi->smode == Areg) {
							if (curi->dmode == absl) {
								strcpy(bus_error_code, "// nothing;\n");
								strcpy(bus_error_code2, "ccr_68000_long_move_ae_LZN(src);\n");
							} else if (curi->dmode == absw) {
								strcpy(bus_error_code, "ccr_68000_long_move_ae_LZN(src);\n");
							}
						} else {
							if (curi->dmode == absl) {
								strcpy(bus_error_code, "// nothing;\n");
								strcpy(bus_error_code2, "ccr_68000_long_move_ae_LZN(src);\n");
							} else {
								strcpy(bus_error_code, "ccr_68000_long_move_ae_LZN(src);\n");
							}
						}
					} else {
						// if data prefetches needed for destination EA, CCR is set before fetch completes.
						if (curi->smode == Areg || curi->smode == Dreg || curi->smode == imm) {
							if (curi->dmode == absl) {
								strcpy(bus_error_code2, "ccr_68000_word_move_ae_normal((uae_s16)src);\n");
							} else if (curi->dmode == absw) {
								strcpy(bus_error_code, "ccr_68000_word_move_ae_normal((uae_s16)src);\n");
							}
						} else {
							if (curi->dmode == Ad16 || curi->dmode == PC16 || curi->dmode == Ad8r || curi->dmode == PC8r || curi->dmode == absw || curi->dmode == absl) {
								strcpy(bus_error_code, "ccr_68000_word_move_ae_normal((uae_s16)src);\n");
							}
						}
					}
				}

				genamode(curi, curi->dmode, "dstreg", curi->size, "dst", 2, 0, flags | GF_NOEXC3);

			  if (curi->mnemo == i_MOVEA && curi->size == sz_word) {
				  out("src = (uae_s32)(uae_s16)src;\n");
			  }

			  if (curi->dmode == Apdi) {
				  // -(an) decrease is not done if bus error
				  if (curi->size == sz_long) {
						fill_prefetch_next_after(0,
							"m68k_areg(regs, dstreg) += 4;\n"
							"ccr_68000_long_move_ae_LZN(src);\n"
						);
				  } else {
					  // x,-(an): flags are set before prefetch detects possible bus error
					  if (curi->mnemo == i_MOVE) {
							fill_prefetch_next_after(1, 
								"m68k_areg(regs, dstreg) += %s;\n"
								"ccr_68000_word_move_ae_normal((uae_s16)src);\n",
								curi->size == sz_byte ? "areg_byteinc[dstreg]" : "2");
					  } else {
							fill_prefetch_next_after(1,
								"m68k_areg(regs, dstreg) += %s;\n",
								curi->size == sz_byte ? "areg_byteinc[dstreg]" : "2");
					  }
				  }
				  prefetch_done = 1;
				  // MOVE.L reg,-(an): 2 extra cycles if 68010
				  if (isreg(curi->smode) && curi->size == sz_long) {
					  if (cpu_level == 1) {
						  addcycles000(2);
					  }
					  next_level_000();
				  }
			  }
			
				int storeflags = flags & (GF_REVERSE | GF_APDI);

			  if (curi->mnemo == i_MOVE) {
					if (cpu_level == 1 && using_prefetch && (isreg(curi->smode) || curi->smode == imm)) {
						out("#if defined(CPU_i386) || defined(CPU_x86_64)\n");
						out("uae_u16 oldflags = regs.ccrflags.cznv;\n");
						out("#else\n");
						out("uae_u16 oldflags = regs.ccrflags.nzcv;\n");
						out("#endif\n");
					}
					if (curi->size == sz_long && (using_prefetch || using_ce) && curi->dmode >= Aind) {
					  // to support bus error exception correct flags, flags needs to be set
					  // after first word has been written.
					  storeflags |= GF_SECONDWORDSETFLAGS;
				  } else {
				    genflags (flag_logical, curi->size, "src", "", "");
				  }
			  }

				bus_error_code[0] = 0;
				bus_error_code2[0] = 0;

				storeflags &= ~(GF_PCM2 | GF_PCP2);
				if (curi->smode >= Aind && curi->smode < imm && curi->dmode == absl) {
					storeflags |= GF_PCM2;
				} else if (curi->dmode == Apdi) {
					storeflags |= GF_PCP2;
				}
			  // MOVE EA,-(An) long writes are always reversed. Reads are normal.
			  if (curi->dmode == Apdi && curi->size == sz_long) {
				  genastore_2("src", curi->dmode, "dstreg", curi->size, "dst", 1, storeflags | GF_EXC3 | GF_MOVE);
			  } else {
				  genastore_2("src", curi->dmode, "dstreg", curi->size, "dst", 0, storeflags | GF_EXC3 | GF_MOVE);
			  }
			  sync_m68k_pc ();
			  if (dualprefetch) {
					fill_prefetch_full_000(curi->mnemo == i_MOVE ? 2 : 1);
				  prefetch_done = 1;
			  }
			  if (!prefetch_done)
				  fill_prefetch_next_t();

			} else {

				genamode(curi, curi->smode, "srcreg", curi->size, "src", 1, 0, 0);
				genamode(curi, curi->dmode, "dstreg", curi->size, "dst", 2, 0, 0);
				if (curi->mnemo == i_MOVEA) {
					if (curi->size == sz_word)
						out("src = (uae_s32)(uae_s16)src;\n");
				} else {
					genflags(flag_logical, curi->size, "src", "", "");
				}
				fill_prefetch_next();
				genastore_2("src", curi->dmode, "dstreg", curi->size, "dst", 0, 0);

			}
	  }
		break;
	case i_MVSR2: // MOVE FROM SR
		if (cpu_level == 0) {
			if ((curi->smode != Apdi && curi->smode != absw && curi->smode != absl) && curi->size == sz_word) {
				exception_pc_offset_extra_000 = -2;
			}
		}
		genamode (curi, curi->smode, "srcreg", sz_word, "src", cpu_level == 0 ? 2 : 3, 0, cpu_level == 1 ? GF_NOFETCH : 0);
		out("MakeSR();\n");
		if (isreg (curi->smode)) {
			if (cpu_level == 0 && curi->size == sz_word) {
				fill_prefetch_next_after(1,
					"MakeSR();\n"
					"m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | ((regs.sr) & 0xffff);\n");
			} else {
				fill_prefetch_next_after(1, NULL);
			}
			if (cpu_level == 0) {
			  addcycles000 (2);
			}
		} else {
			// 68000: read first and ignore result
			if (cpu_level == 0 && curi->size == sz_word) {
				out("%s(srca);\n", srcw);
				count_writew++;
				check_bus_error("src", 0, 0, 1, NULL, 1, 0);
			}
			fill_prefetch_next_after(1, NULL);
		}
		exception_pc_offset_extra_000 = 0;
		if (!isreg(curi->smode) && cpu_level == 1 && using_exception_3 && (using_prefetch || using_ce)) {
			out("if(srca & 1) {\n");
			incpc("%d", m68k_pc_offset + 2);
			out("exception3_write(opcode, srca, 1, regs.sr & 0x%x, 1);\n", curi->size == sz_byte ? 0x00ff : 0xffff);
			returncycles_exception();
			out("}\n");
		}
		// real write
		if (curi->size == sz_byte)
			genastore ("regs.sr & 0xff", curi->smode, "srcreg", sz_word, "src");
		else
			genastore ("regs.sr", curi->smode, "srcreg", sz_word, "src");
		next_level_000();
		break;
	case i_MV2SR: // MOVE TO SR
		genamode (curi, curi->smode, "srcreg", sz_word, "src", 1, 0, 0);
		if (cpu_level == 0 && using_prefetch) {
			out("int t1 = regs.t1;\n");
		}
		if (curi->size == sz_byte) {
			// MOVE TO CCR
			addcycles000 (4);
			out("MakeSR();\nregs.sr &= 0xFF00;\nregs.sr |= src & 0xFF;\n");
			makefromsr();
		} else {
			// MOVE TO SR
			check_trace();
			addcycles000 (4);
			out("regs.sr = src;\n");
			makefromsr_t0();
		}
		if (cpu_level >= 2 && curi->size == sz_byte) {
			fill_prefetch_next();
		} else {
		  // does full prefetch because S-bit change may change memory mapping under the CPU
		  sync_m68k_pc ();
		  fill_prefetch_full_ntx(3);
		}
		next_level_000();
		break;
	case i_SWAP:
		genamode (curi, curi->smode, "srcreg", sz_long, "src", 1, 0, 0);
		out("uae_u32 dst = ((src >> 16)&0xFFFF) | ((src&0xFFFF)<<16);\n");
		if (cpu_level >= 2) {
		  genflags (flag_logical, sz_long, "dst", "", "");
		  genastore ("dst", curi->smode, "srcreg", sz_long, "src");
		} else {
      genastore ("dst", curi->smode, "srcreg", sz_long, "src");
      genflags (flag_logical, sz_long, "dst", "", "");
		}
		fill_prefetch_next_t();
		break;
	case i_EXG:
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "src", 1, 0,
			curi->dmode, "dstreg", curi->size, "dst", 1, 0);
		genastore ("dst", curi->smode, "srcreg", curi->size, "src");
		genastore ("src", curi->dmode, "dstreg", curi->size, "dst");
		fill_prefetch_next_after(1, NULL);
    addcycles000(2);
		break;
	case i_EXT:
		// confirmed
		genamode (curi, curi->smode, "srcreg", sz_long, "src", 1, 0, 0);
		switch (curi->size) {
		case sz_byte: out("uae_u32 dst = (uae_s32)(uae_s8)src;\n"); break;
		case sz_word: out("uae_u16 dst = (uae_s16)(uae_s8)src;\n"); break;
		case sz_long: out("uae_u32 dst = (uae_s32)(uae_s16)src;\n"); break;
		default: term ();
		}
		if (curi->size != sz_word) {
      genflags (flag_logical, sz_long, "dst", "", "");
      genastore ("dst", curi->smode, "srcreg", sz_long, "src");
		} else {
		  genflags (flag_logical, sz_word, "dst", "", "");
		  genastore ("dst", curi->smode, "srcreg", sz_word, "src");
		}
		fill_prefetch_next_t();
		break;
	case i_MVMEL:
		// confirmed
		if (using_ce || using_prefetch)
		  genmovemel_ce (opcode);
		else
			genmovemel(opcode);
		break;
	case i_MVMLE:
		// confirmed
		if (using_ce || using_prefetch)
		  genmovemle_ce (opcode);
		else
			genmovemle(opcode);
		break;
	case i_TRAP:
		exception_oldpc();
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, 0);
		sync_m68k_pc ();
		exception_cpu("src + 32");
		did_prefetch = -1;
		ipl_fetched = -1;
		clear_m68k_offset();
    insn_n_cycles += 4;
		break;
	case i_MVR2USP:
		next_level_000();
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, 0);
		out("regs.usp = src;\n");
		if (cpu_level == 1) {
			addcycles000(2);
		}
		fill_prefetch_next_t();
		trace_t0_68040_only();
		next_level_000();
		break;
	case i_MVUSP2R:
		next_level_000();
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 2, 0, 0);
		genastore ("regs.usp", curi->smode, "srcreg", curi->size, "src");
		if (cpu_level == 1) {
			addcycles000(2);
		}
		fill_prefetch_next_t();
		next_level_000();
		break;
	case i_RESET:
		out("cpureset();\n");
		addcycles000 (128);
		fill_prefetch_next_t();
		break;
	case i_NOP:
		fill_prefetch_next_t();
		trace_t0_68040_only();
		break;
	case i_STOP:
		if (using_prefetch) {
			out("uae_u16 sr = regs.irc;\n");
			m68k_pc_offset += 2;
		} else {
			genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, 0);
			out("uae_u16 sr = src;\n");
		}
		// STOP undocumented features:
		// if new SR S-bit is not set:
		// 68000/68010: Update SR, increase PC and then cause privilege violation exception (handled in newcpu)
		// 68000/68010: Traced STOP runs 4 cycles faster.
		// 68020 68030 68040: STOP works normally
		if ((cpu_level == 0 || cpu_level == 1) && using_ce) {
			out("%s(regs.t1 ? 4 : 8);\n", do_cycles);
		}
		if (cpu_level >= 5) {
			out("if (!(sr & 0x2000)) {\n");
			out("Exception(8);\n");
			returncycles_exception();
			out("}\n");
		}
		out("regs.sr = sr;\n");
		makefromsr ();
		out("m68k_setstopped();\n");
		sync_m68k_pc ();
		// STOP does not prefetch anything
		did_prefetch = -1;
		next_cpu_level = cpu_level - 1;
		next_level_000();
		break;
	case i_RTE:
		next_level_000 ();
		if (cpu_level <= 1 && using_exception_3) {
			out("if (m68k_areg(regs, 7) & 1) {\n");
			out("exception3_read_access(opcode, m68k_areg(regs, 7), 1, 1);\n");
			returncycles_exception();
			out("}\n");
		}
		if (cpu_level == 0) {
			// 68000
			// Read SR (SP+=6), Read PC high, Read PC low.
			out("uaecptr oldpc = %s;\n", getpc);
			out("uaecptr a = m68k_areg(regs, 7);\n");
			out("uae_u16 sr = %s(a);\n", srcw);
			count_readw++;
			check_bus_error("", 0, 0, 1, NULL, 1, 0);

			out("m68k_areg(regs, 7) += 6;\n");

			out("uae_u32 pc = %s(a + 2) << 16;\n", srcw);
			count_readw++;
			check_bus_error("", 2, 0, 1, NULL, 1, 0);
			out("pc |= %s(a + 2 + 2); \n", srcw);
			count_readw++;
			check_bus_error("", 4, 0, 1, NULL, 1, 0);

			out("uae_u16 oldt1 = regs.t1;\n");
			out("regs.sr = sr;\n");
			makefromsr();
			out("if (pc & 1) {\n");
			incpc("2");
			out("exception3_read_access(opcode | 0x20000, pc, 1, 2);\n");
			returncycles_exception();
			out("}\n");
			setpc ("pc");
			if (using_prefetch) {
				out("opcode |= 0x20000;\n");
			}
		} else if (cpu_level == 1 && using_prefetch) {
			// 68010
			// Read SR, Read Format, Read PC high, Read PC low.
			out("uaecptr oldpc = %s;\n", getpc);
			out("uae_u16 newsr;\n");
			out("uae_u32 newpc;\n");
			out("uaecptr a = m68k_areg(regs, 7);\n");
			out("uae_u16 sr = %s(a);\n", srcw);
			count_readw++;
			check_bus_error("", 0, 0, 1, NULL, 1, 0);

			out("uae_u16 format = %s (a + 2 + 4);\n", srcw);
			count_readw++;
			check_bus_error("", 6, 0, 1, NULL, 1, 0);

			out("uae_u32 pc = %s(a + 2) << 16;\n", srcw);
			count_readw++;
			check_bus_error("", 2, 0, 1, NULL, 1, 0);

			out("int frame = format >> 12;\n");
			out("int offset = 8;\n");
			out("if (frame == 0x0) {\n");
			out("m68k_areg(regs, 7) += offset;\n");
			out("} else if (frame == 0x8) {\n");
			out("m68k_areg(regs, 7) += offset + 50;\n");
			out("} else {\n");
			out("SET_NFLG(((uae_s16)format) < 0);\n");
			out("SET_ZFLG(format == 0);\n");
			out("SET_VFLG(0);\n");
			exception_cpu("14");
			returncycles_exception();
			out("}\n");

			out("pc |= %s(a + 2 + 2); \n", srcw);
			count_readw++;
			check_bus_error("", 4, 0, 1, NULL, 1, 0);
	    out("regs.sr = sr;\n");
			makefromsr ();
	    out("if (pc & 1) {\n");
			incpc("2");
	    out("exception3_read_prefetch_only(opcode, pc);\n");
			returncycles_exception();
			out("}\n");
			out("newsr = sr; newpc = pc;\n");
	    setpc ("newpc");
		} else {
			out("uaecptr oldpc = %s;\n", getpc);
			out("uae_u16 oldsr = regs.sr, newsr;\n");
			out("uae_u32 newpc;\n");
			out("for (;;) {\n");
			out("uaecptr a = m68k_areg(regs, 7);\n");
			out("uae_u16 sr = %s(a);\n", srcw);
			count_readw++;
			out("uae_u32 pc = %s(a + 2);\n", srcl);
			count_readl++;
			out("uae_u16 format = %s(a + 2 + 4);\n", srcw);
			count_readw++;
			out("int frame = format >> 12;\n");
			out("int offset = 8;\n");
			out("newsr = sr; newpc = pc;\n");
		  out("if (frame == 0x0) {\nm68k_areg(regs, 7) += offset; break; }\n");
			if (cpu_level >= 2) {
				// 68020+
				out("else if (frame == 0x1) {\nm68k_areg(regs, 7) += offset; }\n");
				out("else if (frame == 0x2) {\nm68k_areg(regs, 7) += offset + 4; break; }\n");
  	  }
	    if (cpu_level >= 4) {
				// 68040+
				out("else if (frame == 0x3) {\nm68k_areg(regs, 7) += offset + 4; break; }\n");
			}
   		if (cpu_level >= 4) {
				// 68040+
				out("else if (frame == 0x4) {\nm68k_areg(regs, 7) += offset + 8; break; }\n");
			}
			if (cpu_level == 1) {
			  // 68010 only
				out("else if (frame == 0x8) {\nm68k_areg(regs, 7) += offset + 50; break; }\n");
		  }
			if (cpu_level == 4) {
				// 68040 only
		    out("else if (frame == 0x7) {\nm68k_areg(regs, 7) += offset + 52; break; }\n");
			}
			if (cpu_level == 2 || cpu_level == 3) { 
			  // 68020/68030 only
				out("else if (frame == 0x9) {\nm68k_areg(regs, 7) += offset + 12; break; }\n");
				out("else if (frame == 0xa) {\nm68k_areg(regs, 7) += offset + 24; break; }\n");
				out("else if (frame == 0xb) {\nm68k_areg(regs, 7) += offset + 84; break; }\n");
      }
			out("else {\n");
			if (cpu_level == 1) {
				out("SET_NFLG(((uae_s16)format) < 0); \n");
				out("SET_ZFLG(format == 0);\n");
				out("SET_VFLG(0);\n");
				exception_cpu("14");
				returncycles_exception();
			} else if (cpu_level == 0) {
				exception_cpu("14");
				returncycles_exception();
			} else if (cpu_level == 3) {
				// 68030: trace bits are cleared
				out("regs.t1 = regs.t0 = 0;\n");
				exception_cpu("14");
				returncycles_exception();
			} else {
				exception_cpu("14");
				returncycles_exception();
      }
			out("}\n");
	    out("regs.sr = newsr;\n");
			out("oldsr = newsr;\n");
			makefromsr_t0();
			out ("}\n");
	    out("regs.sr = newsr;\n");
			makefromsr_t0();
	    out("if (newpc & 1) {\n");
			if (cpu_level == 5) {
				out("regs.sr = oldsr & 0xff00;\n");
				makefromsr();
				out("SET_ZFLG(newsr == 0);\n");
				out("SET_NFLG(newsr & 0x8000);\n");
  	    out("exception3_read_prefetch(opcode, newpc);\n");
			} else if (cpu_level == 4) {
				makefromsr();
				out("exception3_read_prefetch_68040bug(opcode, newpc, oldsr);\n");
			} else {
				out("exception3_read_prefetch(opcode, newpc);\n");
      }
			returncycles_exception();
			out("}\n");
	    setpc ("newpc");
		}
		/* PC is set and prefetch filled. */
		clear_m68k_offset();
		if (using_ce || using_prefetch) {
			fill_prefetch_full_000_special(2, NULL);
		} else {
		  fill_prefetch_full_ntx(0);
		}
		branch_inst = m68k_pc_total;
		next_cpu_level = cpu_level - 1;
		break;
	case i_RTD:
		if (using_prefetch)
      out("uaecptr oldpc = %s;\n", getpc);
		genamode (NULL, Aipi, "7", sz_long, "pc", 1, 0, 0);
		genamode (curi, curi->smode, "srcreg", curi->size, "offs", 1, 0, GF_NOREFILL);
		out("m68k_areg(regs, 7) += offs;\n");
		out("if (pc & 1) {\n");
		if (cpu_level >= 4) {
			out("m68k_areg(regs, 7) -= 4 + offs;\n");
		} else if (cpu_level == 1) {
			incpc("2");
		}
		out("exception3_read_prefetch_only(opcode, pc);\n");
		returncycles_exception();
		out("}\n");
		setpc ("pc");
		/* PC is set and prefetch filled. */
		clear_m68k_offset();
		if (using_prefetch || using_ce) {
			fill_prefetch_full_000_special(1, NULL);
		} else {
			fill_prefetch_full(0);
		}
		branch_inst = m68k_pc_total;
		next_level_040_to_030();
		break;
	case i_LINK:
		// ce confirmed
		// 68040 uses different order than other CPU models.
		// smode must be first in case it is A7. Except if 68040!
		if (cpu_level == 4) {
			genamode(NULL, Apdi, "7", sz_long, "old", 2, 0, GF_AA | GF_NOEXC3);
		  genamode (NULL, curi->smode, "srcreg", sz_long, "src", 1, 0, GF_AA);
		} else {
			genamode(NULL, curi->smode, "srcreg", sz_long, "src", 1, 0, GF_AA);
		  genamode (NULL, Apdi, "7", sz_long, "old", 2, 0, GF_AA | GF_NOEXC3);
		}
		strcpy(bus_error_code, "m68k_areg(regs, 7) += 4;\n");
		genamode (NULL, curi->dmode, "dstreg", curi->size, "offs", 1, 0, GF_PCP2);
		if (cpu_level <= 1 && using_exception_3) {
			out("if (olda & 1) {\n");
			out("m68k_areg(regs, 7) += 4;\n");
			out("m68k_areg(regs, srcreg) = olda;\n");
			incpc("6");
			out("exception3_write_access(opcode, olda, sz_word, src >> 16, 1);\n");
			returncycles_exception();
			out("}\n");
		}
		genastore ("src", Apdi, "7", sz_long, "old");
		genastore ("m68k_areg (regs, 7)", curi->smode, "srcreg", sz_long, "src");
		out("m68k_areg(regs, 7) += offs;\n");
		fill_prefetch_next_t();
		if (!next_level_060_to_040())
			next_level_020_to_010();
		break;
	case i_UNLK:
		// ce confirmed
		m68k_pc_offset = 4;
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, 0);
		genamode(NULL, am_unknown, "src", sz_long, "old", 1, 0, 0);
		out("m68k_areg(regs, 7) = src + 4;\n");
		m68k_pc_offset = 2;
		genastore ("old", curi->smode, "srcreg", curi->size, "src");
		fill_prefetch_next_t();
		break;
	case i_RTS:
		out("uaecptr oldpc = %s;\n", getpc);
		if (cpu_level <= 1 && using_exception_3) {
			out("if (m68k_areg(regs, 7) & 1) {\n");
			incpc("2");
			out("exception3_read_access(opcode, m68k_areg(regs, 7), 1, 1);\n");
			returncycles_exception();
			out("}\n");
		}
		if (using_ce || using_prefetch) {
			out("uaecptr newpc, dsta = m68k_areg(regs, 7);\n");
			out("newpc = %s(dsta) << 16;\n", srcw);
			count_readw++;
			check_bus_error("dst", 0, 0, 1, NULL, 1, 0);
			out("newpc |= %s(dsta + 2);\n", srcw);
			count_readw++;
			check_bus_error("dst", 2, 0, 1, NULL, 1, 0);
			out("m68k_areg(regs, 7) += 4;\n");
			setpc("newpc");
		} else {
			out("m68k_do_rts();\n");
			count_readl++;
		}
	  out("if (%s & 1) {\n", getpc);
		out("uaecptr faultpc = %s;\n", getpc);
		setpc("oldpc");
		if (cpu_level >= 4) {
			out("m68k_areg(regs, 7) -= 4;\n");
		}
		if (cpu_level <= 1) {
			incpc("2");
		}
		out("exception3_read_prefetch_only(opcode, faultpc);\n");
		returncycles_exception();
		out("}\n");
		clear_m68k_offset();
		if (using_prefetch || using_ce) {
			fill_prefetch_full_000_special(2, NULL);
		} else {
			fill_prefetch_full(0);
		}
		branch_inst = m68k_pc_total;
		if (!next_level_040_to_030()) {
			if (!next_level_020_to_010())
				next_level_000();
		}
		break;
	case i_TRAPV:
		exception_oldpc();
		sync_m68k_pc ();
		if (cpu_level == 0) {
			// 68000 TRAPV is really weird
			// If V is set but prefetch causes bus error: S is set.
			// for some reason T is also cleared!
			if (using_prefetch) {
				out("uae_u16 opcode_v = opcode;\n");
			}
			fill_prefetch_next_after(1,
				"if (GET_VFLG()) {\n"
				"MakeSR();\n"
				"regs.sr |= 0x2000;\n"
				"regs.sr &= ~0x8000;\n"
				"MakeFromSR();\n"
				"pcoffset -= 2;\n"
				"} else {\n"
				"opcode = regs.ir | 0x20000;\n"
				"if(regs.t1) opcode |= 0x10000;\n"
				"}\n");
			out("if (GET_VFLG()) {\n");
			if (using_prefetch) {
				// If exception vector is odd,
				// stacked opcode is TRAPV
				out("regs.ir = opcode_v;\n");
			}
			exception_cpu("7");
			returncycles_exception();
			out("}\n");
		} else if (cpu_level == 1) {
			push_ins_cnt();
			out("if (GET_VFLG()) {\n");
			addcycles000(2);
			exception_cpu("7");
			returncycles_exception();
			out("}\n");
			pop_ins_cnt();
			fill_prefetch_next();
		} else {
		  fill_prefetch_next ();
		  out("if (GET_VFLG()) {\n");
			exception_cpu("7");
		  returncycles_exception();
		  out("}\n");
		}
		next_level_000();
		break;
	case i_RTR:
		if (cpu_level <= 1 && using_exception_3) {
			out("if (m68k_areg(regs, 7) & 1) {\n");
			incpc("2");
			out("exception3_read_access(opcode, m68k_areg(regs, 7), 1, 1);\n");
			returncycles_exception();
			out("}\n");
		}
		out("uaecptr oldpc = %s;\n", getpc);
		out("MakeSR();\n");
		genamode (NULL, Aipi, "7", sz_word, "sr", 1, 0, 0);
		genamode (NULL, Aipi, "7", sz_long, "pc", 1, 0, 0);
		if (cpu_level >= 4) {
			out("if (pc & 1) {\n");
			out("m68k_areg(regs, 7) -= 6;\n");
			if (cpu_level == 5) {
				out("regs.sr &= 0xFF00; sr &= 0xFF;\n");
				out("regs.sr |= sr;\n");
				makefromsr();
			  out("exception3_read_prefetch(opcode, pc);\n");
			} else {
				// stacked SR is original SR. Real SR has CCR part zeroed.
				out("uae_u16 oldsr = regs.sr;\n");
				out("regs.sr &= 0xFF00; sr &= 0xFF;\n");
				out("regs.sr |= sr;\n");
				makefromsr();
				out("exception3_read_prefetch_68040bug(opcode, pc, oldsr);\n");
      }
			returncycles_exception();
			out("}\n");
		}
		out("regs.sr &= 0xFF00; sr &= 0xFF;\n");
		out("regs.sr |= sr;\n");
		makefromsr();
		setpc("pc");
		if (cpu_level < 4) {
		  out("if (%s & 1) {\n", getpc);
		  out("uaecptr faultpc = %s;\n", getpc);
		  setpc("oldpc + 2");
		  out("exception3_read_prefetch_only(opcode, faultpc);\n");
		  returncycles_exception();
		  out("}\n");
		}
		clear_m68k_offset();
		if (using_prefetch || using_ce) {
			fill_prefetch_full_000_special(2, NULL);
		} else {
			fill_prefetch_full(0);
		}
		branch_inst = m68k_pc_total;
		next_cpu_level = cpu_level - 1;
		break;
	case i_JSR:
		{
		  // possible idle cycle, prefetch from new address, stack high return addr, stack low, prefetch
		  genamode (curi, curi->smode, "srcreg", curi->size, "src", 0, 0, GF_AA|GF_NOREFILL);
		  out("uaecptr oldpc = %s;\n", getpc);
		  out("uaecptr nextpc = oldpc + %d;\n", m68k_pc_offset);
		  if (using_exception_3 && cpu_level <= 1) {
				push_ins_cnt();
			  out("if (srca & 1) {\n");
				if (curi->smode == Ad16 || curi->smode == absw || curi->smode == PC16) {
					addcycles000_onlyce(2);
					if (!using_ce)
            addcycles000(2);
				}
				if (curi->smode == Ad8r || curi->smode == PC8r) {
					addcycles000_onlyce(6);
					if (!using_ce)
            addcycles000(6);
				}
				if (curi->smode == absl) {
					if (cpu_level == 0) {
						incpc("6");
					} else {
						incpc("2");
					}
				} else if (curi->smode == absw) {
					incpc("4");
				} else {
					incpc("2");
				}
			  out("exception3_read_prefetch_only(opcode, srca);\n");
			  returncycles_exception();
			  out("}\n");
				pop_ins_cnt();
		  }
		  if (curi->smode == Ad16 || curi->smode == absw || curi->smode == PC16)
			  addcycles000 (2);
		  if (curi->smode == Ad8r || curi->smode == PC8r) {
			  addcycles000 (6);
			  if (cpu_level <= 1 && using_prefetch)
				  out("nextpc += 2;\n");
		  }
		  setpc("srca");
		  clear_m68k_offset();
		  if (cpu_level >= 2 && cpu_level < 4) {
			  out("m68k_areg(regs, 7) -= 4;\n");
			}
		  if (using_exception_3 && cpu_level >= 2) {
			  out("if (%s & 1) {\n", getpc);
			  out("exception3_read_prefetch(opcode, %s);\n", getpc);
			  returncycles_exception();
			  out("}\n");
		  }
		  fill_prefetch_1(0);
		  if (cpu_level < 2) {
      	out("m68k_areg(regs, 7) -= 4;\n");
			}
		  if (using_exception_3 && cpu_level <= 1) {
			  out("if (m68k_areg(regs, 7) & 1) {\n");
				setpc("oldpc");
				if (curi->smode == absl) {
					incpc("6");
				} else if (curi->smode == Ad8r || curi->smode == PC8r || curi->smode == Ad16 || curi->smode == PC16 || curi->smode == absw) {
					incpc("4");
				} else {
					incpc("2");
				}
				if (cpu_level == 1) {
			    out("exception3_write_access(opcode, m68k_areg(regs, 7), 1, oldpc >> 16, 1);\n");
				} else {
          out("exception3_write_access(opcode, m68k_areg(regs, 7), 1, m68k_areg(regs, 7) >> 16, 1);\n");
				}
			  returncycles_exception();
			  out("}\n");
		  }
			if (using_ce || using_prefetch) {
			  out("uaecptr dsta = m68k_areg(regs, 7);\n");
			  out("%s(dsta, nextpc >> 16);\n", dstw);
  			count_writew++;
				check_bus_error("dst", 0, 1, 1, "nextpc >> 16", 1, 0);
			  out("%s(dsta + 2, nextpc);\n", dstw);
  			count_writew++;
				check_bus_error("dst", 2, 1, 1, "nextpc", 1, 0);
		  } else {
			  if (cpu_level < 4)
    			out("%s (m68k_areg(regs, 7), nextpc);\n", dstl);
			  else
				  out("%s(m68k_areg(regs, 7) - 4, nextpc);\n", dstl);
				count_writel++;
		  }
		  if (cpu_level >= 4)
			  out("m68k_areg(regs, 7) -= 4;\n");
		  fill_prefetch_full_020();
			if (using_prefetch || using_ce) {
			  int sp = (curi->smode == Ad16 || curi->smode == absw || curi->smode == absl || curi->smode == PC16 || curi->smode == Ad8r || curi->smode == PC8r) ? -1 : 0;
			  irc2ir();
			  copy_opcode();
				if (sp < 0 && cpu_level == 0)
				  out("if(regs.t1) opcode |= 0x10000;\n");
			  out("%s(%d);\n", prefetch_word, 2);
  			count_readw++;
				check_prefetch_bus_error(-2, 0, sp);
			  did_prefetch = 1;
			  ir2irc = 0;
		  } else {
    		fill_prefetch_next_empty();
		  }
		  branch_inst = m68k_pc_total;
		  next_level_040_to_030();
		  next_level_000();
	  }
		break;
	case i_JMP:
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 0, 0, GF_AA|GF_NOREFILL);
		if (using_exception_3) {
			push_ins_cnt();
			out("if (srca & 1) {\n");
			if (curi->smode == Ad16 || curi->smode == absw || curi->smode == PC16) {
				addcycles000_onlyce(2);
				if (!using_ce)
          addcycles000(2);
			}
			if (curi->smode == Ad8r || curi->smode == PC8r) {
				addcycles000_onlyce(6);
				if (!using_ce)
          addcycles000(6);
			}
			incpc("2");
			out("exception3_read_prefetch_only(opcode, srca);\n");
			returncycles_exception();
			out("}\n");
			pop_ins_cnt();
		}
		if (curi->smode == Ad16 || curi->smode == absw || curi->smode == PC16)
			addcycles000 (2);
		if (curi->smode == Ad8r || curi->smode == PC8r)
			addcycles000 (6);
		setpc ("srca");
		clear_m68k_offset();
		if (using_prefetch || using_ce) {
			out("%s(%d);\n", prefetch_word, 0);
			count_readw++;
			check_prefetch_bus_error(-1, 0, 0);
			irc2ir();
      set_last_access_ipl();
			out("%s(%d);\n", prefetch_word, 2);
			int sp = (curi->smode == Ad16 || curi->smode == absw || curi->smode == absl || curi->smode == PC16 || curi->smode == Ad8r || curi->smode == PC8r) ? -1 : 0;
			copy_opcode();
			if (sp < 0 && cpu_level == 0)
				out("if(regs.t1) opcode |= 0x10000;\n");
			count_readw++;
			check_prefetch_bus_error(-2, 0, sp);
			did_prefetch = 1;
			ir2irc = 0;
		} else {
			fill_prefetch_full(0);
		}
		branch_inst = m68k_pc_total;
		next_level_000();
		break;
	case i_BSR:
		// .b/.w = idle cycle, store high, store low, 2xprefetch
		out("uae_s32 s;\n");
		if (curi->size == sz_long && cpu_level < 2) {
			out("uae_u32 src = 0xffffffff;\n");
		} else {
			genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, GF_AA|GF_NOREFILL);
		}
		out("s = (uae_s32)src + 2;\n");
    addcycles000(2);
    out("uaecptr oldpc = %s;\n", getpc);
    out("uaecptr nextpc = oldpc + %d;\n", m68k_pc_offset);
		if (using_exception_3) {
			if (cpu_level == 0) {
        out("if (m68k_areg(regs, 7) & 1) {\n");
        out("m68k_areg(regs, 7) -= 4;\n");
        incpc("2");
        out("exception3_write_access(opcode, m68k_areg(regs, 7), sz_word, oldpc, 1);\n");
        returncycles_exception();
        out("}\n");
			} else if (cpu_level == 1) {
				out("if (m68k_areg(regs, 7) & 1) {\n");
				incpc("2");
				out("exception3_write_access(opcode, oldpc + s, sz_word, oldpc, 1);\n");
  			returncycles_exception();
				out("}\n");
			}
		}
		if (using_exception_3 && cpu_level == 1) {
			// 68010 TODO: looks like prefetches are done first and stack writes last
			out("if (s & 1) {\n");
			incpc("2");
			out("exception3_read_prefetch_only(opcode, oldpc + s);\n");
			returncycles_exception();
			out("}\n");
		}
		if (using_exception_3 && cpu_level >= 2) {
			out("if (s & 1) {\n");
			if (cpu_level < 4)
				out("m68k_areg(regs, 7) -= 4;\n");
			out("exception3_read_prefetch(opcode, oldpc + s);\n");
			returncycles_exception();
			out("}\n");
		}
		if (using_ce || using_prefetch) {
			out("m68k_areg(regs, 7) -= 4;\n");
			out("uaecptr dsta = m68k_areg(regs, 7);\n");
			out("%s(dsta, nextpc >> 16);\n", dstw);
			check_bus_error("dst", 0, 1, 1, "nextpc >> 16", 1, 0);
			count_writew++;
			out("%s(dsta + 2, nextpc);\n", dstw);
			check_bus_error("dst", 2, 1, 1, "nextpc", 1, 0);
			count_writew++;
			incpc("s");
		} else {
			out("m68k_do_bsr(nextpc, s);\n");
			count_writel++;
		}
		if (using_exception_3 && cpu_level == 0) {
			// both stacked fields point to new PC
			out("if (%s & 1) {\n", getpc);
			out("uaecptr addr = %s;\n", getpc);
			incpc("-2");
			out("exception3_read_prefetch(opcode, addr);\n");
			returncycles_exception();
			out("}\n");
		}
		clear_m68k_offset();
		if (using_prefetch || using_ce) {
			fill_prefetch_full_000_special(0, NULL);
		} else {
			fill_prefetch_full(0);
		}
		branch_inst = m68k_pc_total;
		if (!next_level_040_to_030()) {
			if (!next_level_020_to_010())
				next_level_000();
		}
		break;
	case i_Bcc:
		if (using_prefetch)
      out("uaecptr oldpc = %s;\n", getpc);
		if (curi->size == sz_long) {
			if (cpu_level < 2) {
				addcycles000 (2);
				out("if (cctrue(%d)) {\n", curi->cc);
				out("exception3_read_prefetch(opcode, %s + 1);\n", getpc);
				returncycles_exception ();
				out("}\n");
				sync_m68k_pc ();
				addcycles000 (2);
				irc2ir ();
				fill_prefetch_bcc();
        if (cpu_level == 1) {
          count_cycles += 2;
          insn_n_cycles += 6;
        } else {
          count_cycles += 4;
          insn_n_cycles += 8;
        }
				goto bccl_not68020;
			} else {
				if (next_cpu_level < 1)
					next_cpu_level = 1;
			}
    }
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, GF_AA | (cpu_level < 2 ? GF_NOREFILL : 0));
		addcycles000 (2);
		if (using_exception_3 && cpu_level >= 4) {
			out("if (src & 1) {\n");
			out("exception3_read_prefetch(opcode, %s + (uae_s32)src + 2);\n", getpc);
			returncycles_exception ();
			out("}\n");
		}
		out("if (cctrue(%d)) {\n", curi->cc);
		if (using_exception_3 && cpu_level < 4) {
			out("if (src & 1) {\n");
			if (cpu_level == 1) {
				if (using_prefetch)
          out("uaecptr oldpc = %s;\n", getpc);
				out("uae_u16 rb = regs.irc;\n");
				incpc("((uae_s32)src + 2) & ~1");
				dummy_prefetch(NULL, "oldpc");
				out("uaecptr newpc = %s + (uae_s32)src + 2;\n", getpc);
				incpc("2");
				out("regs.read_buffer = rb;\n");
				out("exception3_read_prefetch(opcode, newpc);\n");
			} else {
			  // Addr = new address, PC = current PC
			  out("uaecptr addr = %s + (uae_s32)src + 2;\n", getpc);
			  out("exception3_read_prefetch(opcode, addr);\n");
			}
			returncycles_exception ();
			out("}\n");
		}
		push_ins_cnt();
		if (using_prefetch) {
			incpc ("(uae_s32)src + 2");
			fill_prefetch_full_000_special(2, NULL);
			check_ipl_always();
			if (using_ce)
				out("return;\n");
			else
			  out("return 10 * CYCLE_UNIT / 2;\n");
		} else {
			incpc ("(uae_s32)src + 2");
			fill_prefetch_full_020 ();
			check_ipl_always();
			returncycles (10);
		}
		pop_ins_cnt();
		out("}\n");
		sync_m68k_pc ();
		if (cpu_level == 1) {
			if (curi->size != sz_byte)
		    addcycles000 (2);
		} else if (cpu_level == 0) {
			addcycles000(2);
		}
		if (curi->size == sz_byte) {
			irc2ir ();
			fill_prefetch_bcc();
		} else if (curi->size == sz_word) {
			fill_prefetch_full_000_special(0, NULL);
		} else {
			fill_prefetch_full_000_special(0, NULL);
		}
    if (cpu_level == 1) {
      insn_n_cycles = curi->size == sz_byte ? 6 : 10;
    } else {
      insn_n_cycles = curi->size == sz_byte ? 8 : 12;
    }
		branch_inst = -m68k_pc_total;
bccl_not68020:
		if (!next_level_040_to_030())
			if (!next_level_020_to_010())
        next_level_000();
		break;
	case i_LEA:
		if (curi->smode == Ad8r || curi->smode == PC8r) {
			addcycles000 (2);
    }
		if (curi->smode == absl) {
			if (cpu_level == 0) {
				strcpy(bus_error_code, "m68k_areg(regs, dstreg) = (m68k_areg(regs, dstreg) & 0x0000ffff) | (srca & 0xffff0000);\n");
				strcpy(bus_error_code2, "m68k_areg(regs, dstreg) = (srca);\n");
			}
			next_level_000();
		}  else if (curi->smode != Ad8r && curi->smode != PC8r) {
			if (cpu_level == 0) {
				strcpy(bus_error_code, "m68k_areg(regs, dstreg) = (srca);\n");
			}
    }
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "src", 0, GF_AA | ((curi->smode == absw) ? GF_PCM2 : 0),
			curi->dmode, "dstreg", curi->size, "dst", 2, GF_AA);
		if (curi->smode == Ad8r || curi->smode == PC8r)
			addcycles000 (2);
		genastore ("srca", curi->dmode, "dstreg", curi->size, "dst");
		fill_prefetch_next_t();
		break;
	case i_PEA:
		if (curi->smode == Ad8r || curi->smode == PC8r)
			addcycles000 (2);
		if (cpu_level <= 1 && using_exception_3) {
			out("uae_u16 old_opcode = opcode;\n");
		}
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 0, 0, GF_AA);
		genamode (NULL, Apdi, "7", sz_long, "dst", 2, 0, GF_AA | GF_NOEXC3);
		if (curi->smode == Ad8r || curi->smode == PC8r) {
			addcycles000 (2);
    }
    if (!(curi->smode == absw || curi->smode == absl)) {
			fill_prefetch_next_after(0, "m68k_areg(regs, 7) += 4;\n");
    }
		if (cpu_level <= 1 && using_exception_3) {
			out("if (dsta & 1) {\n");
			out("regs.ir = old_opcode;\n");
			if (cpu_level == 1) {
				incpc("2");
			} else {
				incpc("%d", m68k_pc_offset + ((curi->smode == absw || curi->smode == absl) ? 0 : 2));
			}
			out("exception3_write_access(old_opcode, dsta, sz_word, srca >> 16, 1);\n");
			returncycles_exception();
			out("}\n");
		}
		genastore ("srca", Apdi, "7", sz_long, "dst");
		if ((curi->smode == absw || curi->smode == absl)) {
			fill_prefetch_next_t();
    }
		break;
	case i_DBcc:
		// cc true: idle cycle, prefetch
		// cc false, counter expired: idle cycle, prefetch (from branch address), 2xprefetch (from next address)
		// cc false, counter not expired: idle cycle, prefetch
		if(cpu_level <= 1 && using_prefetch) {
			// this is quite annoying instruction..
			out("int pcadjust = -2;\n");
		}
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "src", 1, GF_AA | (cpu_level < 2 ? GF_NOREFILL : 0),
			curi->dmode, "dstreg", curi->size, "offs", 1, GF_AA | (cpu_level < 2 ? GF_NOREFILL : 0));
		out("uaecptr oldpc = %s;\n", getpc);
		addcycles000 (2);
		if (using_exception_3 && cpu_level >= 4) {
			out("if (offs & 1) {\n");
			out("exception3_read_prefetch(opcode, oldpc + (uae_s32)offs + 2);\n");
			returncycles_exception();
			out("}\n");
		}
		push_ins_cnt();
		out("if (!cctrue(%d)) {\n", curi->cc);
		incpc ("(uae_s32)offs + 2");
		if (cpu_level >= 2 && cpu_level < 4) {
			genastore("(src - 1)", curi->smode, "srcreg", curi->size, "src");
		}
		if (using_exception_3 && cpu_level < 4) {
			out("if (offs & 1) {\n");
			if (cpu_level == 1 && using_prefetch) {
				out("%s(%d);\n", prefetch_word, -1);
			}
			out("exception3_read_prefetch(opcode, %s);\n", getpc);
			returncycles_exception();
			out("}\n");
		}
		sprintf(bus_error_code, "pcoffset = oldpc - %s + 4;\n", getpc);
		fill_prefetch_1 (0);
		bus_error_code[0] = 0;
		if (cpu_level >= 4) {
		  genastore ("(src - 1)", curi->smode, "srcreg", curi->size, "src");
		}
		out("if (src) {\n");
		if (cpu_level < 2) {
			genastore("(src - 1)", curi->smode, "srcreg", curi->size, "src");
		}
		irc2ir ();
		check_ipl_always();

		if (using_prefetch || using_ce) {
			copy_opcode();
			if (cpu_level == 0) {
			  out("if(regs.t1) opcode |= 0x10000;\n");
			}
			out("%s(%d);\n", prefetch_word, 2);
			check_prefetch_bus_error(-2, 0, -1);
			did_prefetch = 1;
			ir2irc = 0;
			count_readw++;
		}

		fill_prefetch_full_020 ();
		returncycles (10);
		out("}\n");
		if (cpu_level == 0) {
			addcycles000_nonce(2);
		} else {
			addcycles000_onlyce(2);
			addcycles000_nonce(6);
		}
		if (cpu_level <= 1 && using_prefetch) {
			out("pcadjust = 0;\n");
		}
		if (cpu_level == 0 && using_ce) {
			out("} else {\n");
			addcycles000_onlyce(2);
		}
		out("}\n");
		pop_ins_cnt();
		if (cpu_level == 0) {
			insn_n_cycles += 2;
			count_cycles += 2;
		}
		setpc ("oldpc + %d", m68k_pc_offset);
		clear_m68k_offset();
		fill_prefetch_full_000_special(-1, "if (!cctrue(%d)) {\nm68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);\n}\n", curi->cc);
    branch_inst = -m68k_pc_total;
		if (!next_level_040_to_030()) {
			if (!next_level_020_to_010())
				next_level_000();
		}		
		break;
	case i_Scc:
		genamode (curi, curi->smode, "srcreg", curi->size, "src", cpu_level == 0 ? 1 : 2, 0, cpu_level == 1 ? GF_NOFETCH : 0);
		if (isreg (curi->smode)) {
			// If mode is Dn and condition true = 2 extra cycles needed.
      out("int val = cctrue(%d) ? 0xff : 0x00;\n", curi->cc);
			if (using_ce)
				out("int cycles = val ? 2 : 0;\n");
			if (using_bus_error && cpu_level <= 1) {
        if (using_prefetch) {
				  out("if (!val) {\n");
				  genastore("val", curi->smode, "srcreg", curi->size, "src");
				  out("}\n");
        }
				if (cpu_level == 0 && (using_prefetch || using_ce)) {
					out("opcode |= 0x20000;\n");
				}
			}
			fill_prefetch_next_extra("if (!val)", "if(!val && regs.t1) opcode |= 0x10000;\n");
      genastore ("val", curi->smode, "srcreg", curi->size, "src");
			addcycles000_3();
			addcycles000_nonces("(val ? 2 : 0)");
		} else {
			fill_prefetch_next_after(1, NULL);
      out("int val = cctrue(%d) ? 0xff : 0x00;\n", curi->cc);
		  genastore ("val", curi->smode, "srcreg", curi->size, "src");
    }
		next_level_000();
		break;
	case i_DIVU:
		exception_oldpc();
		genamodedual (curi,
			curi->smode, "srcreg", sz_word, "src", 1, 0,
			curi->dmode, "dstreg", sz_long, "dst", 1, 0);
		push_ins_cnt();
		out("if (src == 0) {\n");
		out("divbyzero_special(0, dst);\n");
		incpc ("%d", m68k_pc_offset);
		addcycles000(4);
		exception_cpu("5");
		returncycles_exception();
		out("}\n");
		pop_ins_cnt();
		out("uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;\n");
		out("uae_u32 rem = (uae_u32)dst %% (uae_u32)(uae_u16)src;\n");
		if (using_ce) {
			out("int cycles = getDivu68kCycles((uae_u32)dst, (uae_u16)src);\n");
			addcycles000_3();
		}
		if (cpu_level <= 1) {
			addcycles000_nonces("getDivu68kCycles((uae_u32)dst, (uae_u16)src)");
		}
		out("if (newv > 0xffff) {\n");
		out("setdivuflags((uae_u32)dst, (uae_u16)src);\n");
		out("} else {\n");
    genflags (flag_logical, sz_word, "newv", "", "");
		out("newv = (newv & 0xffff) | ((uae_u32)rem << 16);\n");
    genastore ("newv", curi->dmode, "dstreg", sz_long, "dst");
		out("}\n");
		fill_prefetch_next_t();
		sync_m68k_pc ();
		count_ncycles++;
		next_level_020_to_010();
		break;
	case i_DIVS:
		exception_oldpc();
		genamodedual (curi,
			curi->smode, "srcreg", sz_word, "src", 1, 0,
			curi->dmode, "dstreg", sz_long, "dst", 1, 0);
		push_ins_cnt();
		out("if (src == 0) {\n");
		out("divbyzero_special(1, dst);\n");
		incpc ("%d", m68k_pc_offset);
		addcycles000(4);
		exception_cpu("5");
		returncycles_exception();
		out("}\n");
		pop_ins_cnt();
		if (using_ce) {
			out("int cycles = getDivs68kCycles((uae_s32)dst, (uae_s16)src);\n");
			addcycles000_3();
		}
		if (cpu_level <= 1) {
			addcycles000_nonces("getDivs68kCycles((uae_s32)dst, (uae_s16)src)");
		}
		out("if (dst == 0x80000000 && src == -1) {\n");
		out("setdivsflags((uae_s32)dst, (uae_s16)src);\n");
		out("} else {\n");
		out("uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;\n");
		out("uae_u16 rem = (uae_s32)dst %% (uae_s32)(uae_s16)src;\n");
		out("if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) {\n");
		out("setdivsflags((uae_s32)dst, (uae_s16)src);\n");
		out("} else {\n");
		out("if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;\n");
		genflags (flag_logical, sz_word, "newv", "", "");
		out("newv = (newv & 0xffff) | ((uae_u32)rem << 16);\n");
    genastore ("newv", curi->dmode, "dstreg", sz_long, "dst");
		out("}\n");
		out("}\n");
		fill_prefetch_next_t();
		sync_m68k_pc ();
		count_ncycles++;
		next_level_020_to_010();
		break;
	case i_MULU:
		genamodedual (curi,
			curi->smode, "srcreg", sz_word, "src", 1, 0,
			curi->dmode, "dstreg", sz_word, "dst", 1, 0);
		fill_prefetch_next_after(1,
			"CLEAR_CZNV();\n"
			"SET_ZFLG(1);\n"
			"pcoffset -= %d;\n"
			"m68k_dreg(regs, dstreg) &= 0xffff0000;\n",
			// strange EA behavior, no other instruction work like this
			curi->smode == Apdi ? 0 : (curi->smode == absl || curi->smode == absw || curi->smode == imm ? 2 : m68k_pc_offset));
		out("uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;\n");
		genflags (flag_logical, sz_long, "newv", "", "");
		if (using_ce) {
			out("int cycles = getMulu68kCycles(src);\n");
			addcycles000_3();
		}
    addcycles000_nonces("getMulu68kCycles(src)");
		genastore ("newv", curi->dmode, "dstreg", sz_long, "dst");
		sync_m68k_pc ();
		count_ncycles++;
		next_level_020_to_010();
		break;
	case i_MULS:
		genamodedual (curi,
			curi->smode, "srcreg", sz_word, "src", 1, 0,
			curi->dmode, "dstreg", sz_word, "dst", 1, 0);
		fill_prefetch_next_after(1, 
			"CLEAR_CZNV();\n"
			"SET_ZFLG(1);\n"
			"pcoffset -= %d;\n"
			"m68k_dreg(regs, dstreg) &= 0xffff0000;\n",
			curi->smode == Apdi ? 0 : (curi->smode == absl || curi->smode == absw || curi->smode == imm ? 2 : m68k_pc_offset));
		out("uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;\n");
		genflags (flag_logical, sz_long, "newv", "", "");
		if (using_ce) {
			out("int cycles = getMuls68kCycles(src);\n");
			addcycles000_3();
		}
		addcycles000_nonces("getMuls68kCycles(src)");
		genastore ("newv", curi->dmode, "dstreg", sz_long, "dst");
		count_ncycles++;
		next_level_020_to_010();
		break;
	case i_CHK:
		exception_oldpc();
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, 0);
		genamode (curi, curi->dmode, "dstreg", curi->size, "dst", 1, 0, 0);
		sync_m68k_pc ();
		addcycles000 (4);
		out("if (dst > src) {\n");
		out("setchkundefinedflags(src, dst, %d);\n", curi->size);
		exception_cpu("6");
		returncycles_exception();
		out("}\n");
		addcycles000 (2);
		out("if ((uae_s32)dst < 0) {\n");
		out("setchkundefinedflags(src, dst, %d);\n", curi->size);
		exception_cpu("6");
		returncycles_exception();
		out("}\n");
		out("setchkundefinedflags(src, dst, %d);\n", curi->size);
		fill_prefetch_next_t();
		break;
	case i_CHK2:
		exception_oldpc();
		genamode (curi, curi->smode, "srcreg", curi->size, "extra", 1, 0, 0);
		genamode (curi, curi->dmode, "dstreg", curi->size, "dst", 2, 0, 0);
		fill_prefetch_0 ();
		out("uae_s32 upper,lower,reg = regs.regs[(extra >> 12) & 15];\n");
		switch (curi->size) {
		case sz_byte:
			out("lower = (uae_s32)(uae_s8)%s(dsta);\nupper = (uae_s32)(uae_s8)%s(dsta + 1);\n", srcb, srcb);
			out("if ((extra & 0x8000) == 0) reg = (uae_s32)(uae_s8)reg;\n");
			break;
		case sz_word:
			out("lower = (uae_s32)(uae_s16)%s(dsta);\nupper = (uae_s32)(uae_s16)%s(dsta + 2);\n", srcw, srcw);
			out("if ((extra & 0x8000) == 0) reg = (uae_s32)(uae_s16)reg;\n");
			break;
		case sz_long:
			out("lower = %s(dsta); upper = %s(dsta + 4);\n", srcl, srcl);
			break;
		default:
			term ();
		}
		sync_m68k_pc();
		out("SET_CFLG(0);\n");
		out("SET_ZFLG(0);\n");
		out("setchk2undefinedflags(lower, upper, reg, (extra & 0x8000) ? 2 : %d);\n", curi->size);
		out("if(upper == reg || lower == reg) {\n");
		out("SET_ZFLG(1);\n");
		out("}else{\n");
		out("if (lower <= upper && (reg < lower || reg > upper)) SET_ALWAYS_CFLG(1);\n");
		out("if (lower > upper && reg > upper && reg < lower) SET_ALWAYS_CFLG(1);\n");
		out("}\n");
		out("if ((extra & 0x800) && GET_CFLG()) {\n");
		exception_cpu("6");
		returncycles_exception();
		out("}\n");
		break;
	case i_ASR:
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "cnt", 1, 0,
			curi->dmode, "dstreg", curi->size, "data", 1, GF_RMW);
		switch (curi->size) {
		case sz_byte: out("uae_u32 val = (uae_u8)data;\n"); break;
		case sz_word: out("uae_u32 val = (uae_u16)data;\n"); break;
		case sz_long: out("uae_u32 val = data;\n"); break;
		default: term ();
		}
		out("CLEAR_CZNV();\n");
		if (curi->size == sz_long) {
			fill_prefetch_next_noopcodecopy("SET_NFLG(val & 0x8000);\nSET_ZFLG(!(val & 0xffff));\n");
		} else {
			fill_prefetch_next_noopcodecopy("SET_ZFLG(!(val & %s));\nSET_NFLG(val & %s);\n", bit_mask(curi->size), cmask(curi->size));
		}
		out("uae_u32 sign = (%s & val) >> %d;\n", cmask (curi->size), bit_size (curi->size) - 1);
    if (cpu_level <= 1)
		  out("int ccnt = cnt & 63;\n");
		out("cnt &= 63;\n");
		out("if (cnt >= %d) {\n", bit_size (curi->size));
		out("val = %s & (uae_u32)(0 - sign);\n", bit_mask (curi->size));
		out("SET_CFLG(sign);\n");
		duplicate_carry();
		if (source_is_imm1_8 (curi))
			out("} else {\n");
		else
			out("} else if (cnt > 0) {\n");
		out("val >>= cnt - 1;\n");
		out("SET_CFLG(val & 1);\n");
		duplicate_carry();
		out("val >>= 1;\n");
		out("val |= (%s << (%d - cnt)) & (uae_u32)(0 - sign);\n",
			bit_mask (curi->size),
			bit_size (curi->size));
		out("val &= %s;\n", bit_mask (curi->size));
		out("}\n");
		genflags (flag_logical_noclobber, curi->size, "val", "", "");
		shift_ce (curi->dmode, curi->size);
		genastore ("val", curi->dmode, "dstreg", curi->size, "data");
		break;
	case i_ASL:
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "cnt", 1, 0,
			curi->dmode, "dstreg", curi->size, "data", 1, GF_RMW);
		switch (curi->size) {
		case sz_byte: out("uae_u32 val = (uae_u8)data;\n"); break;
		case sz_word: out("uae_u32 val = (uae_u16)data;\n"); break;
		case sz_long: out("uae_u32 val = data;\n"); break;
		default: term ();
		}
		out("CLEAR_CZNV();\n");
		if (curi->size == sz_long) {
			fill_prefetch_next_noopcodecopy("SET_NFLG(val & 0x8000);\nSET_ZFLG(!(val & 0xffff));\n");
		} else {
			fill_prefetch_next_noopcodecopy("SET_ZFLG(!(val & %s));\nSET_NFLG(val & %s);\n", bit_mask(curi->size), cmask(curi->size));
		}
		if (cpu_level <= 1)
  		out("int ccnt = cnt & 63;\n");
		out("cnt &= 63;\n");
		out("if (cnt >= %d) {\n", bit_size (curi->size));
		out("SET_VFLG(val != 0);\n");
		out("SET_CFLG(cnt == %d ? val & 1 : 0);\n",
			bit_size (curi->size));
		duplicate_carry();
		out("val = 0;\n");
		if (source_is_imm1_8 (curi))
			out("} else {\n");
		else
			out("} else if (cnt > 0) {\n");
		out("uae_u32 mask = (%s << (%d - cnt)) & %s;\n",
			bit_mask (curi->size),
			bit_size (curi->size) - 1,
			bit_mask (curi->size));
		out("SET_VFLG((val & mask) != mask && (val & mask) != 0);\n");
		out("val <<= cnt - 1;\n");
		out("SET_CFLG((val & %s) >> %d);\n", cmask (curi->size), bit_size (curi->size) - 1);
		duplicate_carry();
		out("val <<= 1;\n");
		out("val &= %s;\n", bit_mask (curi->size));
		out("}\n");
		genflags (flag_logical_noclobber, curi->size, "val", "", "");
		shift_ce (curi->dmode, curi->size);
		genastore ("val", curi->dmode, "dstreg", curi->size, "data");
		break;
	case i_LSR:
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "cnt", 1, 0,
			curi->dmode, "dstreg", curi->size, "data", 1, GF_RMW);
		switch (curi->size) {
		case sz_byte: out("uae_u32 val = (uae_u8)data;\n"); break;
		case sz_word: out("uae_u32 val = (uae_u16)data;\n"); break;
		case sz_long: out("uae_u32 val = data;\n"); break;
		default: term ();
		}
		out("CLEAR_CZNV();\n");
		if (curi->size == sz_long) {
			fill_prefetch_next_noopcodecopy("SET_NFLG(val & 0x8000);\nSET_ZFLG(!(val & 0xffff));\n");
		} else {
			fill_prefetch_next_noopcodecopy("SET_ZFLG(!(val & %s));\nSET_NFLG(val & %s);\n", bit_mask(curi->size), cmask(curi->size));
		}
		if (cpu_level <= 1)
  		out("int ccnt = cnt & 63;\n");
		out("cnt &= 63;\n");
		out("if (cnt >= %d) {\n", bit_size (curi->size));
		out("SET_CFLG((cnt == %d) & (val >> %d));\n",
			bit_size (curi->size), bit_size (curi->size) - 1);
		duplicate_carry();
		out("val = 0;\n");
		if (source_is_imm1_8 (curi))
			out("} else {\n");
		else
			out("} else if (cnt > 0) {\n");
		out("val >>= cnt - 1;\n");
		out("SET_CFLG(val & 1);\n");
		duplicate_carry();
		out("val >>= 1;\n");
		out("}\n");
		genflags (flag_logical_noclobber, curi->size, "val", "", "");
		shift_ce (curi->dmode, curi->size);
		genastore ("val", curi->dmode, "dstreg", curi->size, "data");
		break;
	case i_LSL:
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "cnt", 1, 0,
			curi->dmode, "dstreg", curi->size, "data", 1, GF_RMW);
		switch (curi->size) {
		case sz_byte: out("uae_u32 val = (uae_u8)data;\n"); break;
		case sz_word: out("uae_u32 val = (uae_u16)data;\n"); break;
		case sz_long: out("uae_u32 val = data;\n"); break;
		default: term ();
		}
		out("CLEAR_CZNV();\n");
		if (curi->size == sz_long) {
			fill_prefetch_next_noopcodecopy("SET_NFLG(val & 0x8000);\nSET_ZFLG(!(val & 0xffff));\n");
		} else {
			fill_prefetch_next_noopcodecopy("SET_ZFLG(!(val & %s));\nSET_NFLG(val & %s);\n", bit_mask(curi->size), cmask(curi->size));
		}
		if (cpu_level <= 1)
  		out("int ccnt = cnt & 63;\n");
		out("cnt &= 63;\n");
		out("if (cnt >= %d) {\n", bit_size (curi->size));
		out("SET_CFLG(cnt == %d ? val & 1 : 0);\n", bit_size(curi->size));
		duplicate_carry();
		out("val = 0;\n");
		if (source_is_imm1_8 (curi))
			out("} else {\n");
		else
			out("} else if (cnt > 0) {\n");
		out("val <<= (cnt - 1);\n");
		out("SET_CFLG((val & %s) >> %d);\n", cmask(curi->size), bit_size(curi->size) - 1);
		duplicate_carry();
		out("val <<= 1;\n");
		out("val &= %s;\n", bit_mask (curi->size));
		out("}\n");
		genflags (flag_logical_noclobber, curi->size, "val", "", "");
		shift_ce (curi->dmode, curi->size);
		genastore ("val", curi->dmode, "dstreg", curi->size, "data");
		break;
	case i_ROL:
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "cnt", 1, 0,
			curi->dmode, "dstreg", curi->size, "data", 1, GF_RMW);
		switch (curi->size) {
		case sz_byte: out("uae_u32 val = (uae_u8)data;\n"); break;
		case sz_word: out("uae_u32 val = (uae_u16)data;\n"); break;
		case sz_long: out("uae_u32 val = data;\n"); break;
		default: term ();
		}
		out("CLEAR_CZNV();\n");
		if (curi->size == sz_long) {
			fill_prefetch_next_noopcodecopy("SET_NFLG(val & 0x8000);\nSET_ZFLG(!(val & 0xffff));\n");
		} else {
			fill_prefetch_next_noopcodecopy("SET_ZFLG(!(val & %s));\nSET_NFLG(val & %s);\n", bit_mask(curi->size), cmask(curi->size));
		}
		if (cpu_level <= 1)
  		out("int ccnt = cnt & 63;\n");
		out("cnt &= 63;\n");
		if (source_is_imm1_8 (curi))
			out("{\n");
		else
			out("if (cnt > 0) {\n");
		out("uae_u32 loval;\n");
		out("cnt &= %d;\n", bit_size (curi->size) - 1);
		out("loval = val >> (%d - cnt);\n", bit_size (curi->size));
		out("val <<= cnt;\n");
		out("val |= loval;\n");
		out("val &= %s;\n", bit_mask (curi->size));
		out("SET_CFLG(val & 1);\n");
		out ("}\n");
		genflags (flag_logical_noclobber, curi->size, "val", "", "");
		shift_ce (curi->dmode, curi->size);
		genastore ("val", curi->dmode, "dstreg", curi->size, "data");
		break;
	case i_ROR:
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "cnt", 1, 0,
			curi->dmode, "dstreg", curi->size, "data", 1, GF_RMW);
		switch (curi->size) {
		case sz_byte: out("uae_u32 val = (uae_u8)data;\n"); break;
		case sz_word: out("uae_u32 val = (uae_u16)data;\n"); break;
		case sz_long: out("uae_u32 val = data;\n"); break;
		default: term ();
		}
		out("CLEAR_CZNV();\n");
		if (curi->size == sz_long) {
			fill_prefetch_next_noopcodecopy("SET_NFLG(val & 0x8000);\nSET_ZFLG(!(val & 0xffff));\n");
		} else {
			fill_prefetch_next_noopcodecopy("SET_ZFLG(!(val & %s));\nSET_NFLG(val & %s);\n", bit_mask(curi->size), cmask(curi->size));
		}
		if (cpu_level <= 1)
  		out("int ccnt = cnt & 63;\n");
		out("cnt &= 63;\n");
		if (source_is_imm1_8 (curi))
			out ("{\n");
		else
			out("if (cnt > 0) {\n");
		out("uae_u32 hival;\n");
		out("cnt &= %d;\n", bit_size (curi->size) - 1);
		out("hival = val << (%d - cnt);\n", bit_size (curi->size));
		out("val >>= cnt;\n");
		out("val |= hival;\n");
		out("val &= %s;\n", bit_mask (curi->size));
		out("SET_CFLG((val & %s) >> %d);\n", cmask (curi->size), bit_size (curi->size) - 1);
		out("}\n");
		genflags (flag_logical_noclobber, curi->size, "val", "", "");
		shift_ce (curi->dmode, curi->size);
		genastore ("val", curi->dmode, "dstreg", curi->size, "data");
		break;
	case i_ROXL:
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "cnt", 1, 0,
			curi->dmode, "dstreg", curi->size, "data", 1, GF_RMW);
		switch (curi->size) {
		case sz_byte: out("uae_u32 val = (uae_u8)data;\n"); break;
		case sz_word: out("uae_u32 val = (uae_u16)data;\n"); break;
		case sz_long: out("uae_u32 val = data;\n"); break;
		default: term ();
		}
		out("CLEAR_CZNV();\n");
		if (curi->size == sz_long) {
			fill_prefetch_next_noopcodecopy("SET_NFLG(val & 0x8000);\nSET_ZFLG(!(val & 0xffff));\nSET_CFLG(GET_XFLG());\n");
		} else {
			fill_prefetch_next_noopcodecopy("SET_ZFLG(!(val & %s));\nSET_NFLG(val & %s);\nSET_CFLG(GET_XFLG());\n", bit_mask(curi->size), cmask(curi->size));
		}
		if (cpu_level <= 1)
  		out("int ccnt = cnt & 63;\n");
		out("cnt &= 63;\n");
		if (source_is_imm1_8 (curi))
			out ("{\n");
		else {
			force_range_for_rox ("cnt", curi->size);
			out("if (cnt > 0) {\n");
		}
		out("cnt--;\n");
		out("{\nuae_u32 carry;\n");
		out("uae_u32 loval = val >> (%d - cnt);\n", bit_size (curi->size) - 1);
		out("carry = loval & 1;\n");
		out("val = (((val << 1) | GET_XFLG()) << cnt) | (loval >> 1);\n");
		out("SET_XFLG(carry);\n");
		out("val &= %s;\n", bit_mask (curi->size));
		out("}\n");
		out("}\n");
		out("SET_CFLG(GET_XFLG());\n");
		genflags (flag_logical_noclobber, curi->size, "val", "", "");
		shift_ce (curi->dmode, curi->size);
		genastore ("val", curi->dmode, "dstreg", curi->size, "data");
		break;
	case i_ROXR:
		genamodedual (curi,
			curi->smode, "srcreg", curi->size, "cnt", 1, 0,
			curi->dmode, "dstreg", curi->size, "data", 1, GF_RMW);
		switch (curi->size) {
		case sz_byte: out("uae_u32 val = (uae_u8)data;\n"); break;
		case sz_word: out("uae_u32 val = (uae_u16)data;\n"); break;
		case sz_long: out("uae_u32 val = data;\n"); break;
		default: term ();
		}
		out("CLEAR_CZNV();\n");
		if (curi->size == sz_long) {
			fill_prefetch_next_noopcodecopy("SET_NFLG(val & 0x8000);\nSET_ZFLG(!(val & 0xffff));\nSET_CFLG(GET_XFLG());\n");
		} else {
			fill_prefetch_next_noopcodecopy("SET_ZFLG(!(val & %s));\nSET_NFLG(val & %s);\nSET_CFLG(GET_XFLG());\n", bit_mask(curi->size), cmask(curi->size));
		}
		if (cpu_level <= 1)
  		out("int ccnt = cnt & 63;\n");
		out("cnt &= 63;\n");
		if (source_is_imm1_8 (curi))
			out ("{\n");
		else {
			force_range_for_rox ("cnt", curi->size);
			out("if (cnt > 0) {\n");
		}
		out("cnt--;\n");
		out("{\nuae_u32 carry;\n");
		out("uae_u32 hival = (val << 1) | GET_XFLG();\n");
		out("hival <<= (%d - cnt);\n", bit_size (curi->size) - 1);
		out("val >>= cnt;\n");
		out("carry = val & 1;\n");
		out("val >>= 1;\n");
		out("val |= hival;\n");
		out("SET_XFLG(carry);\n");
		out("val &= %s;\n", bit_mask (curi->size));
		out("}\n");
		out("}\n");
		out("SET_CFLG(GET_XFLG());\n");
		genflags (flag_logical_noclobber, curi->size, "val", "", "");
		shift_ce (curi->dmode, curi->size);
		genastore ("val", curi->dmode, "dstreg", curi->size, "data");
		break;
	case i_ASRW:
		genamode (curi, curi->smode, "srcreg", curi->size, "data", 1, 0, GF_RMW);
		switch (curi->size) {
		case sz_byte: out("uae_u32 val = (uae_u8)data;\n"); break;
		case sz_word: out("uae_u32 val = (uae_u16)data;\n"); break;
		case sz_long: out("uae_u32 val = data;\n"); break;
		default: term ();
		}
		fill_prefetch_next_noopcodecopy("CLEAR_CZNV();\nSET_CFLG(val & 1);\nSET_ZFLG(!(val >> 1));\nSET_NFLG(val & 0x8000);\nSET_XFLG(GET_CFLG());\n");
		out("uae_u32 sign = %s & val;\n", cmask (curi->size));
		out("uae_u32 cflg = val & 1;\n");
		out("val = (val >> 1) | sign;\n");
		genflags (flag_logical, curi->size, "val", "", "");
		out("SET_CFLG(cflg);\n");
		duplicate_carry();
		genastore ("val", curi->smode, "srcreg", curi->size, "data");
		break;
	case i_ASLW:
		genamode (curi, curi->smode, "srcreg", curi->size, "data", 1, 0, GF_RMW);
		switch (curi->size) {
		case sz_byte: out("uae_u32 val = (uae_u8)data;\n"); break;
		case sz_word: out("uae_u32 val = (uae_u16)data;\n"); break;
		case sz_long: out("uae_u32 val = data;\n"); break;
		default: term ();
		}
		fill_prefetch_next_noopcodecopy("CLEAR_CZNV();\nSET_CFLG(val & %s);\nSET_ZFLG(!(val & 0x7fff));\nSET_NFLG(val & 0x4000);\nSET_XFLG(GET_CFLG());\nSET_VFLG((val & 0x8000) != ((val << 1) & 0x8000));\n", cmask(curi->size));
		out("uae_u32 sign = %s & val;\n", cmask (curi->size));
		out("uae_u32 sign2;\n");
		out("val <<= 1;\n");
		genflags (flag_logical, curi->size, "val", "", "");
		out("sign2 = %s & val;\n", cmask (curi->size));
		out("SET_CFLG(sign != 0);\n");
		duplicate_carry();
		out("SET_VFLG(GET_VFLG() | (sign2 != sign));\n");
		genastore ("val", curi->smode, "srcreg", curi->size, "data");
		break;
	case i_LSRW:
		genamode (curi, curi->smode, "srcreg", curi->size, "data", 1, 0, GF_RMW);
		switch (curi->size) {
		case sz_byte: out("uae_u32 val = (uae_u8)data;\n"); break;
		case sz_word: out("uae_u32 val = (uae_u16)data;\n"); break;
		case sz_long: out("uae_u32 val = data;\n"); break;
		default: term ();
		}
		fill_prefetch_next_noopcodecopy("CLEAR_CZNV();\nSET_CFLG(val & 1);\nSET_ZFLG(!(val & 0xfffe));\nSET_NFLG(0);\nSET_XFLG(GET_CFLG());\n");
		out("uae_u32 carry = val & 1;\n");
		out("val >>= 1;\n");
		genflags (flag_logical, curi->size, "val", "", "");
		out("SET_CFLG(carry);\n");
		duplicate_carry();
		genastore ("val", curi->smode, "srcreg", curi->size, "data");
		break;
	case i_LSLW:
		genamode (curi, curi->smode, "srcreg", curi->size, "data", 1, 0, GF_RMW);
		switch (curi->size) {
		case sz_byte: out("uae_u8 val = data;\n"); break;
		case sz_word: out("uae_u16 val = data;\n"); break;
		case sz_long: out("uae_u32 val = data;\n"); break;
		default: term ();
		}
		fill_prefetch_next_noopcodecopy("CLEAR_CZNV();\nSET_CFLG(val & %s);\nSET_ZFLG(!(val & 0x7fff));\nSET_NFLG(val & 0x4000);\nSET_XFLG(GET_CFLG());\n", cmask(curi->size));
		out("uae_u32 carry = val & %s;\n", cmask (curi->size));
		out("val <<= 1;\n");
		genflags (flag_logical, curi->size, "val", "", "");
		out("SET_CFLG(carry >> %d);\n", bit_size (curi->size) - 1);
		duplicate_carry();
		genastore ("val", curi->smode, "srcreg", curi->size, "data");
		break;
	case i_ROLW:
		genamode (curi, curi->smode, "srcreg", curi->size, "data", 1, 0, GF_RMW);
		switch (curi->size) {
		case sz_byte: out("uae_u8 val = data;\n"); break;
		case sz_word: out("uae_u16 val = data;\n"); break;
		case sz_long: out("uae_u32 val = data;\n"); break;
		default: term ();
		}
		fill_prefetch_next_noopcodecopy("CLEAR_CZNV();SET_CFLG(val & %s);\nSET_ZFLG(!val);\nSET_NFLG(val & 0x4000);\n", cmask(curi->size));
		out("uae_u32 carry = val & %s;\n", cmask (curi->size));
		out("val <<= 1;\n");
		out("if (carry)  val |= 1;\n");
		genflags (flag_logical, curi->size, "val", "", "");
		out("SET_CFLG(carry >> %d);\n", bit_size (curi->size) - 1);
		genastore ("val", curi->smode, "srcreg", curi->size, "data");
		break;
	case i_RORW:
		genamode (curi, curi->smode, "srcreg", curi->size, "data", 1, 0, GF_RMW);
		switch (curi->size) {
		case sz_byte: out("uae_u8 val = data;\n"); break;
		case sz_word: out("uae_u16 val = data;\n"); break;
		case sz_long: out("uae_u32 val = data;\n"); break;
		default: term ();
		}
		fill_prefetch_next_noopcodecopy("CLEAR_CZNV();\nSET_CFLG(val & 1);\nSET_ZFLG(!val);\nSET_NFLG(val & 0x0001);\n");
		out("uae_u32 carry = val & 1;\n");
		out("val >>= 1;\n");
		out("if (carry) val |= %s;\n", cmask (curi->size));
		genflags (flag_logical, curi->size, "val", "", "");
		out("SET_CFLG(carry);\n");
		genastore ("val", curi->smode, "srcreg", curi->size, "data");
		break;
	case i_ROXLW:
		genamode (curi, curi->smode, "srcreg", curi->size, "data", 1, 0, GF_RMW);
		switch (curi->size) {
		case sz_byte: out("uae_u8 val = data;\n"); break;
		case sz_word: out("uae_u16 val = data;\n"); break;
		case sz_long: out("uae_u32 val = data;\n"); break;
		default: term ();
		}
		fill_prefetch_next_noopcodecopy("CLEAR_CZNV();\nSET_CFLG(val & 0x8000);\nSET_ZFLG(!((val & 0x7fff) | GET_XFLG()));\nSET_NFLG(val & 0x4000);\nSET_XFLG(GET_CFLG());\n", cmask(curi->size));
		out("uae_u32 carry = val & %s;\n", cmask (curi->size));
		out("val <<= 1;\n");
		out("if (GET_XFLG()) val |= 1;\n");
		genflags (flag_logical, curi->size, "val", "", "");
		out("SET_CFLG(carry >> %d);\n", bit_size (curi->size) - 1);
		duplicate_carry();
		genastore ("val", curi->smode, "srcreg", curi->size, "data");
		break;
	case i_ROXRW:
		genamode (curi, curi->smode, "srcreg", curi->size, "data", 1, 0, GF_RMW);
		switch (curi->size) {
		case sz_byte: out("uae_u8 val = data;\n"); break;
		case sz_word: out("uae_u16 val = data;\n"); break;
		case sz_long: out("uae_u32 val = data;\n"); break;
		default: term ();
		}
		fill_prefetch_next_noopcodecopy("CLEAR_CZNV();SET_CFLG(val & 1);\nSET_ZFLG(!((val &0x7ffe) | GET_XFLG()))\n;SET_NFLG(GET_XFLG())\n;SET_XFLG(GET_CFLG());\n", cmask(curi->size));
		out("uae_u32 carry = val & 1;\n");
		out("val >>= 1;\n");
		out("if (GET_XFLG()) val |= %s;\n", cmask (curi->size));
		genflags (flag_logical, curi->size, "val", "", "");
		out("SET_CFLG(carry);\n");
		duplicate_carry();
		genastore ("val", curi->smode, "srcreg", curi->size, "data");
		break;
	case i_MOVEC2:
		if (cpu_level == 1) {
			out("if(!regs.s) {\n");
			out("Exception(8);\n");
			returncycles_exception();
			out("}\n");
		}
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, 0);
		fill_prefetch_next ();
		out("int regno = (src >> 12) & 15;\n");
		out("uae_u32 *regp = regs.regs + regno;\n");
		out("if (!m68k_movec2(src & 0xFFF, regp)) {\n");
    returncycles_exception();
		out("}\n");
		addcycles000(4);
		break;
	case i_MOVE2C:
		if (cpu_level == 1) {
			out("if(!regs.s) {\n");
			out("Exception(8);\n");
			returncycles_exception();
			out("}\n");
		}
		genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, 0);
		fill_prefetch_next ();
		out("int regno = (src >> 12) & 15;\n");
		out("uae_u32 *regp = regs.regs + regno;\n");
		out("if (!m68k_move2c(src & 0xFFF, regp)) {\n");
    returncycles_exception();
		out("}\n");
		addcycles000(2);
		trace_t0_68040_only();
		break;
	case i_CAS:
		{
			genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, GF_LRMW);
			genamode (curi, curi->dmode, "dstreg", curi->size, "dst", 1, 0, GF_LRMW);
			fill_prefetch_0 ();
			out("int ru = (src >> 6) & 7;\n");
			out("int rc = src & 7;\n");
			genflags (flag_cmp, curi->size, "newv", "m68k_dreg (regs, rc)", "dst");
			out("if (GET_ZFLG()) {\n");
			genastore_cas ("(m68k_dreg (regs, ru))", curi->dmode, "dstreg", curi->size, "dst");
			out("} else {\n");
			if (cpu_level >= 4) {
				// apparently 68040 needs to always write at the end of RMW cycle
				genastore_cas ("dst", curi->dmode, "dstreg", curi->size, "dst");
			}
  			switch (curi->size) {
			    case sz_byte:
				out("m68k_dreg(regs, rc) = (m68k_dreg(regs, rc) & ~0xff) | (dst & 0xff);\n");
				break;
			    case sz_word:
				out("m68k_dreg(regs, rc) = (m68k_dreg(regs, rc) & ~0xffff) | (dst & 0xffff);\n");
				break;
	    		default:
				out("m68k_dreg(regs, rc) = dst;\n");
				break;
			}	
			out("}\n");
			trace_t0_68040_only();
		}
		break;
	case i_CAS2:
		genamode (curi, curi->smode, "srcreg", curi->size, "extra", 1, 0, GF_LRMW);
		out("uae_u32 rn1 = regs.regs[(extra >> 28) & 15];\n");
		out("uae_u32 rn2 = regs.regs[(extra >> 12) & 15];\n");
		if (curi->size == sz_word) {
			out("uae_u16 dst1 = %s(rn1), dst2 = %s(rn2);\n", srcwlrmw, srcwlrmw);
			genflags (flag_cmp, curi->size, "newv", "m68k_dreg (regs, (extra >> 16) & 7)", "dst1");
			out("if (GET_ZFLG()) {\n");
			genflags (flag_cmp, curi->size, "newv", "m68k_dreg (regs, extra & 7)", "dst2");
			out("if (GET_ZFLG()) {\n");
			out("%s(rn2, m68k_dreg(regs, (extra >> 6) & 7));\n", dstwlrmw);
			out("%s(rn1, m68k_dreg(regs, (extra >> 22) & 7));\n", dstwlrmw);
			out("}\n");
			out("}\n");
			out("if (!GET_ZFLG()) {\n");
			if (cpu_level >= 4) {
				// 68040: register update order swapped
				out("m68k_dreg(regs, (extra >> 16) & 7) = (m68k_dreg(regs, (extra >> 16) & 7) & ~0xffff) | (dst1 & 0xffff);\n");
				out("m68k_dreg(regs, (extra >> 0) & 7) = (m68k_dreg(regs, (extra >> 0) & 7) & ~0xffff) | (dst2 & 0xffff);\n");
			} else {
				out("m68k_dreg(regs, (extra >> 0) & 7) = (m68k_dreg(regs, (extra >> 0) & 7) & ~0xffff) | (dst2 & 0xffff);\n");
				out("m68k_dreg(regs, (extra >> 16) & 7) = (m68k_dreg(regs, (extra >> 16) & 7) & ~0xffff) | (dst1 & 0xffff);\n");
			}
			out ("}\n");
		} else {
			out("uae_u32 dst1 = %s(rn1), dst2 = %s(rn2);\n", srcllrmw, srcllrmw);
			genflags (flag_cmp, curi->size, "newv", "m68k_dreg (regs, (extra >> 16) & 7)", "dst1");
			out("if (GET_ZFLG()) {\n");
			genflags (flag_cmp, curi->size, "newv", "m68k_dreg (regs, extra & 7)", "dst2");
			out("if (GET_ZFLG()) {\n");
			out("%s(rn2, m68k_dreg(regs, (extra >> 6) & 7));\n", dstllrmw);
			out("%s(rn1, m68k_dreg(regs, (extra >> 22) & 7));\n", dstllrmw);
			out("}\n");
			out("}\n");
			out("if (!GET_ZFLG()) {\n");
			if (cpu_level >= 4) {
				// 68040: register update order swapped
				out("m68k_dreg(regs, (extra >> 16) & 7) = dst1;\n");
				out("m68k_dreg(regs, (extra >> 0) & 7) = dst2;\n");
			} else {
			  out("m68k_dreg(regs, (extra >> 0) & 7) = dst2;\n");
			  out("m68k_dreg(regs, (extra >> 16) & 7) = dst1;\n");
			}
			out("}\n");
		}
		trace_t0_68040_only();
		break;
	case i_MOVES: /* ignore DFC and SFC when using_mmu == false */
		{
			genamode (curi, curi->smode, "srcreg", curi->size, "extra", 1, 0, 0);
			strcpy(g_srcname, "src");
			addcycles000(4);
			out("if (extra & 0x800) {\n");
			{
				// reg -> memory
				int old_m68k_pc_offset = m68k_pc_offset;
				int old_m68k_pc_total = m68k_pc_total;
				push_ins_cnt();
				// 68060 stores original value, 68010 MOVES.L also stores original value.
				out("uae_u32 src = regs.regs[(extra >> 12) & 15];\n");
				genamode (curi, curi->dmode, "dstreg", curi->size, "dst", 2, 0, cpu_level == 1 ? GF_NOFETCH : 0);
				did_prefetch = 0;
				// Earlier models do -(an)/(an)+ EA calculation first
				if (curi->dmode == Apdi) {
					out("src = regs.regs[(extra >> 12) & 15];\n");
        }
				genastore_fc ("src", curi->dmode, "dstreg", curi->size, "dst");
				sync_m68k_pc();
				pop_ins_cnt();
				m68k_pc_offset = old_m68k_pc_offset;
				m68k_pc_total = old_m68k_pc_total;
			}
			out("} else {\n");
			{
				// memory -> reg
				genamode (curi, curi->dmode, "dstreg", curi->size, "src", 1, 0, GF_FC | (cpu_level == 1 ? GF_NOFETCH : 0));
				out("if (extra & 0x8000) {\n");
				switch (curi->size) {
				case sz_byte: out("m68k_areg(regs, (extra >> 12) & 7) = (uae_s32)(uae_s8)src;\n"); break;
				case sz_word: out("m68k_areg(regs, (extra >> 12) & 7) = (uae_s32)(uae_s16)src;\n"); break;
				case sz_long: out("m68k_areg(regs, (extra >> 12) & 7) = src;\n"); break;
				default: term ();
				}
				out("} else {\n");
				genastore ("src", Dreg, "(extra >> 12) & 7", curi->size, "");
				out("}\n");
				sync_m68k_pc();
			}
			out("}\n");
			trace_t0_68040_only();
			fill_prefetch_next();
			next_level_060_to_040();
		}
		break;
	case i_BKPT:		/* only needed for hardware emulators */
		sync_m68k_pc ();
		addcycles000(4);
		out("op_illg(opcode);\n");
		did_prefetch = -1;
		ipl_fetched = -1;
		break;
	case i_CALLM:		/* not present in 68030 */
		sync_m68k_pc ();
		out("op_illg(opcode);\n");
		did_prefetch = -1;
		break;
	case i_RTM:		/* not present in 68030 */
		sync_m68k_pc ();
		out("op_illg(opcode);\n");
		did_prefetch = -1;
		break;
	case i_TRAPcc:
		exception_oldpc();
		if (curi->smode != am_unknown && curi->smode != am_illg)
			genamode (curi, curi->smode, "srcreg", curi->size, "dummy", 1, 0, 0);
		fill_prefetch_0 ();
		sync_m68k_pc();
		out("if (cctrue(%d)) {\n", curi->cc);
		exception_cpu("7");
    returncycles_exception();
		out("}\n");
		break;
	case i_DIVL:
    out("uae_u32 cyc = 0;\n");
		out("uaecptr oldpc = %s;\n", getpc);
    genamode (curi, curi->smode, "srcreg", curi->size, "extra", 1, 0, 0);
    out("if (extra & 0x800) cyc = 12 * CYCLE_UNIT / 2;\n");
		genamode (curi, curi->dmode, "dstreg", curi->size, "dst", 1, 0, 0);
		sync_m68k_pc ();
		out("int e = m68k_divl(opcode, dst, extra, oldpc);\n");
		out("if (e <= 0) {\n");
	  returncycles_exception();
    out("}\n");
		break;
	case i_MULL:
		genamode (curi, curi->smode, "srcreg", curi->size, "extra", 1, 0, 0);
		genamode (curi, curi->dmode, "dstreg", curi->size, "dst", 1, 0, 0);
		sync_m68k_pc ();
		out("int e = m68k_mull(opcode, dst, extra);\n");
		out("if (e <= 0) {\n");
		returncycles_exception();
		out("}\n");
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
			const char *getb, *putb;

			getb = "get_bitfield";
			putb = "put_bitfield";

			genamode (curi, curi->smode, "srcreg", curi->size, "extra", 1, 0, 0);
			genamode (curi, curi->dmode, "dstreg", sz_long, "dst", 2, 0, 0);
			out("uae_u32 bdata[2];\n");
			out("uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;\n");
			out("int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) - 1) & 0x1f) + 1;\n");
			if (curi->mnemo == i_BFFFO)
				out("uae_u32 offset2 = offset;\n");
			if (curi->dmode == Dreg) {
				out("uae_u32 tmp = m68k_dreg(regs, dstreg);\n");
				out("offset &= 0x1f;\n");
				out("tmp = (tmp << offset) | (tmp >> (32 - offset));\n");
				out("bdata[0] = tmp & ((1 << (32 - width)) - 1);\n");
			} else {
				out("uae_u32 tmp;\n");
				out("dsta += offset >> 3;\n");
				out("tmp = %s(dsta, bdata, offset, width);\n", getb);
			}
			out("SET_ALWAYS_NFLG(((uae_s32)tmp) < 0 ? 1 : 0);\n");
			if (curi->mnemo == i_BFEXTS)
				out("tmp = (uae_s32)tmp >> (32 - width);\n");
			else
				out("tmp >>= (32 - width);\n");
			out("SET_ZFLG(tmp == 0); SET_VFLG(0); SET_CFLG(0);\n");
			switch (curi->mnemo) {
			case i_BFTST:
				break;
			case i_BFEXTU:
			case i_BFEXTS:
				out("m68k_dreg(regs, (extra >> 12) & 7) = tmp;\n");
				break;
			case i_BFCHG:
				out("tmp = tmp ^ (0xffffffffu >> (32 - width));\n");
				break;
			case i_BFCLR:
				out("tmp = 0;\n");
				break;
			case i_BFFFO:
				out("{ uae_u32 mask = 1 << (width - 1);\n");
				out("while (mask) { if (tmp & mask) break; mask >>= 1; offset2++; }}\n");
				out("m68k_dreg(regs, (extra >> 12) & 7) = offset2;\n");
				break;
			case i_BFSET:
				out("tmp = 0xffffffffu >> (32 - width);\n");
				break;
			case i_BFINS:
				out("tmp = m68k_dreg(regs, (extra >> 12) & 7);\n");
				out("tmp = tmp & (0xffffffffu >> (32 - width));\n");
				out("SET_ALWAYS_NFLG(tmp & (1 << (width - 1)) ? 1 : 0);\n");
				out("SET_ZFLG(tmp == 0);\n");
				break;
			default:
				break;
			}
			if (curi->mnemo == i_BFCHG
				|| curi->mnemo == i_BFCLR
				|| curi->mnemo == i_BFSET
				|| curi->mnemo == i_BFINS) {
					if (curi->dmode == Dreg) {
						out("tmp = bdata[0] | (tmp << (32 - width));\n");
						out("m68k_dreg(regs, dstreg) = (tmp >> offset) | (tmp << (32 - offset));\n");
					} else {
						out("%s(dsta, bdata, tmp, offset, width);\n", putb);
					}
			}
		}
		break;
	case i_PACK:
		if (curi->smode == Dreg) {
			out("uae_u16 val = m68k_dreg(regs, srcreg) + %s;\n", gen_nextiword (0));
			out("m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & 0xffffff00) | ((val >> 4) & 0xf0) | (val & 0xf);\n");
		} else {
			out("uae_u16 val;\n");
			out("m68k_areg(regs, srcreg) -= 2;\n");
			out("val = (uae_u16)(%s(m68k_areg(regs, srcreg)));\n", srcw);
			out("val += %s;\n", gen_nextiword(0));
			out("m68k_areg(regs, dstreg) -= areg_byteinc[dstreg];\n");
			out("%s(m68k_areg(regs, dstreg),((val >> 4) & 0xf0) | (val & 0xf));\n", dstb);
		}
		break;
	case i_UNPK:
		if (curi->smode == Dreg) {
			out("uae_u16 val = m68k_dreg(regs, srcreg);\n");
			out("val = ((val << 4) & 0xf00) | (val & 0xf);\n");
			out("val += %s;\n", gen_nextiword(0));
			out("m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & 0xffff0000) | (val & 0xffff);\n");
		} else {
			out("uae_u16 val;\n");
			out("m68k_areg(regs, srcreg) -= areg_byteinc[srcreg];\n");
			out("val = (uae_u16)(%s(m68k_areg(regs, srcreg)) & 0xff);\n", srcb);
			out("val = (((val << 4) & 0xf00) | (val & 0xf)) + %s;\n", gen_nextiword (0));
			out("m68k_areg(regs, dstreg) -= 2;\n");
			out("%s(m68k_areg(regs, dstreg), val);\n", dstw);
		}
		break;
	case i_TAS:
		if (cpu_level == 0) {
			bus_error_cycles = 2;
		  genamode (curi, curi->smode, "srcreg", curi->size, "src", 1, 0, GF_LRMW);
		  genflags (flag_logical, curi->size, "src", "", "");
		  if (!isreg (curi->smode)) {
			  addcycles000(6);
      }
		  out("src |= 0x80;\n");
			if (isreg(curi->smode) || !using_ce) {
		    genastore_tas ("src", curi->smode, "srcreg", curi->size, "src");
			} else {
				out("if (!is_cycle_ce(srca)) {\n");
				genastore("src", curi->smode, "srcreg", curi->size, "src");
				out("} else {\n");
				out("%s(4);\n", do_cycles);
				addcycles000_nonce(4);
				out("}\n");
			}
      fill_prefetch_next();
		} else if (cpu_level == 1) {
			if (isreg(curi->smode)) {
				genamode(curi, curi->smode, "srcreg", curi->size, "src", 1, 0, GF_LRMW);
			} else {
				genamode(curi, curi->smode, "srcreg", curi->size, "src", 2, 0, GF_LRMW | GF_NOFETCH);
				out("uae_u8 src = %s(srca);\n", srcb);
				check_bus_error("src", 0, 0, 0, "src", 1, 0);
			}
			genflags(flag_logical, curi->size, "src", "", "");
			if (!isreg(curi->smode)) {
				addcycles000(6);
			}
			out("src |= 0x80;\n");
			if (isreg(curi->smode) || !using_ce) {
			  genastore_tas("src", curi->smode, "srcreg", curi->size, "src");
			} else {
				out("if (!is_cycle_ce(srca)) {\n");
				genastore("src", curi->smode, "srcreg", curi->size, "src");
				out("} else {\n");
				out("%s(4);\n", do_cycles);
				addcycles000_nonce(4);
				out("}\n");
			}
			fill_prefetch_next();
		} else {
			genamode(curi, curi->smode, "srcreg", curi->size, "src", 1, 0, GF_LRMW);
			genflags(flag_logical, curi->size, "src", "", "");
			out("src |= 0x80;\n");
			if (next_cpu_level < 2)
				next_cpu_level = 2 - 1;
			genastore_tas("src", curi->smode, "srcreg", curi->size, "src");
			fill_prefetch_next();
    }
		next_level_000();
		break;
	case i_FPP:
		fpulimit();
		genamode (curi, curi->smode, "srcreg", curi->size, "extra", 1, 0, 0);
		sync_m68k_pc ();
		out("fpuop_arithmetic(opcode, extra);\n");
		if (using_prefetch) {
			out("if (regs.fp_exception) {\n");
			returncycles_exception();
			out("}\n");
		}
		break;
	case i_FDBcc:
		fpulimit();
		genamode (curi, curi->smode, "srcreg", curi->size, "extra", 1, 0, 0);
		sync_m68k_pc ();
		out("fpuop_dbcc (opcode, extra);\n");
		if (using_prefetch) {
			out("if (regs.fp_exception) {\n");
			returncycles_exception();
			out("}\n");
			out("if (regs.fp_branch) {\n");
			out("regs.fp_branch = false;\n");
			out("fill_prefetch();\n");
			returncycles_exception();
			out("}\n");
		} else {
			out("if (regs.fp_branch) {\n");
			out("regs.fp_branch = false;\n");
			out("if(regs.t0) check_t0_trace();\n");
			out("}\n");
		}
		break;
	case i_FScc:
		fpulimit();
		genamode (curi, curi->smode, "srcreg", curi->size, "extra", 1, 0, 0);
		sync_m68k_pc ();
		out("fpuop_scc (opcode, extra);\n");
		if (using_prefetch) {
			out("if (regs.fp_exception) {\n");
			returncycles_exception();
			out("}\n");
		}
		break;
	case i_FTRAPcc:
		fpulimit();
		out("uaecptr oldpc = %s;\n", getpc);
		out("uae_u16 extra = %s;\n", gen_nextiword (0));
		if (curi->smode != am_unknown && curi->smode != am_illg)
			genamode (curi, curi->smode, "srcreg", curi->size, "dummy", 1, 0, 0);
		sync_m68k_pc ();
		out("fpuop_trapcc (opcode, oldpc, extra);\n");
		if (using_prefetch) {
			out("if (regs.fp_exception) {\n");
			returncycles_exception();
			out("}\n");
		}
		break;
	case i_FBcc:
		fpulimit();
		sync_m68k_pc ();
		out("uaecptr pc = %s;\n", getpc);
		genamode (curi, curi->dmode, "srcreg", curi->size, "extra", 1, 0, 0);
		sync_m68k_pc ();
		out("fpuop_bcc (opcode, pc,extra);\n");
		if (using_prefetch) {
			out("if (regs.fp_exception) {\n");
			returncycles_exception();
			out("}\n");
			out("if (regs.fp_branch) {\n");
			out("regs.fp_branch = false;\n");
			out("fill_prefetch();\n");
			returncycles_exception();
			out("}\n");
		} else {
			out("if (regs.fp_branch) {\n");
			out("regs.fp_branch = false;\n");
			out("if(regs.t0) check_t0_trace();\n");
			out("}\n");
		}
		break;
	case i_FSAVE:
		fpulimit();
		sync_m68k_pc ();
		out("fpuop_save (opcode);\n");
		if (using_prefetch) {
			out("if (regs.fp_exception) {\n");
			returncycles_exception();
			out("}\n");
		}
		break;
	case i_FRESTORE:
		fpulimit();
		sync_m68k_pc ();
		out("fpuop_restore (opcode);\n");
		if (using_prefetch) {
			out("if (regs.fp_exception) {\n");
			returncycles_exception();
			out("}\n");
		}
		break;

	case i_CINVL:
	case i_CINVP:
	case i_CINVA:
	case i_CPUSHL:
	case i_CPUSHP:
	case i_CPUSHA:
		out("flush_cpu_caches_040(opcode);\n");
		out("if (opcode & 0x80) {\n");
		out("flush_icache((opcode >> 6) & 3);\n");
		out("}\n");
		out("check_t0_trace();\n");
		break;

	case i_MOVE16:
		{
			if ((opcode & 0xfff8) == 0xf620) {
				/* MOVE16 (Ax)+,(Ay)+ */
				out("uaecptr mems = m68k_areg(regs, srcreg) & ~15, memd;\n");
				out("dstreg = (%s >> 12) & 7;\n", gen_nextiword (0));
				out("memd = m68k_areg(regs, dstreg) & ~15;\n");
				out("uae_u32 v[4];\n");
				out("v[0] = %s(mems);\n", srcl);
				out("v[1] = %s(mems + 4);\n", srcl);
				out("v[2] = %s(mems + 8);\n", srcl);
				out("v[3] = %s(mems + 12);\n", srcl);
				out("%s(memd , v[0]);\n", dstl);
				out("%s(memd + 4, v[1]);\n", dstl);
				out("%s(memd + 8, v[2]);\n", dstl);
				out("%s(memd + 12, v[3]);\n", dstl);
				out("if (srcreg != dstreg)\n");
				out("m68k_areg(regs, srcreg) += 16;\n");
				out("m68k_areg(regs, dstreg) += 16;\n");
			} else {
				/* Other variants */
				genamode (curi, curi->smode, "srcreg", curi->size, "mems", 0, 2, 0);
				genamode (curi, curi->dmode, "dstreg", curi->size, "memd", 0, 2, 0);
				out("memsa &= ~15;\n");
				out("memda &= ~15;\n");
				out("uae_u32 v[4];\n");
				out("v[0] = %s(memsa);\n", srcl);
				out("v[1] = %s(memsa + 4);\n", srcl);
				out("v[2] = %s(memsa + 8);\n", srcl);
				out("v[3] = %s(memsa + 12);\n", srcl);
				out("%s(memda , v[0]);\n", dstl);
				out("%s(memda + 4, v[1]);\n", dstl);
				out("%s(memda + 8, v[2]);\n", dstl);
				out("%s(memda + 12, v[3]);\n", dstl);
				if ((opcode & 0xfff8) == 0xf600)
					out("m68k_areg(regs, srcreg) += 16;\n");
				else if ((opcode & 0xfff8) == 0xf608)
					out("m68k_areg(regs, dstreg) += 16;\n");
			}
		}
		break;

	case i_PFLUSHN:
	case i_PFLUSH:
	case i_PFLUSHAN:
	case i_PFLUSHA:
		sync_m68k_pc();
		out("mmu_op(opcode, 0);\n");
		trace_t0_68040_only();
		break;
	case i_PTESTR:
	case i_PTESTW:
		sync_m68k_pc ();
		out ("mmu_op(opcode, 0);\n");
		trace_t0_68040_only();
		break;
	case i_MMUOP030:
		out("uaecptr pc = %s;\n", getpc);
		out("uae_u16 extra = %s(2);\n", prefetch_word);
		m68k_pc_offset += 2;
		sync_m68k_pc ();
		if (curi->smode == Areg || curi->smode == Dreg)
			out("uae_u16 extraa = 0;\n");
		else
			genamode (curi, curi->smode, "srcreg", curi->size, "extra", 0, 0, 0);
		sync_m68k_pc ();
		out("mmu_op30(pc, opcode, extra, extraa);\n");
		break;
	default:
		term ();
		break;
	}
end:
	if (set_fpulimit) {
		out("\n#endif\n");
	}
	if (did_prefetch >= 0)
		fill_prefetch_finish ();
	sync_m68k_pc ();
	if ((using_ce || using_prefetch) && did_prefetch >= 0) {
		if (last_access_offset_ipl > 0) {
			insertstring("ipl_fetch();\n", last_access_offset_ipl);
		}
	}
	did_prefetch = 0;
	ipl_fetched = 0;
	if (cpu_level >= 2 && !using_ce) {
		int v = curi->clocks;
		if (v < 4)
			v = 4;
		count_cycles = v;
	}
}

static void generate_macros(FILE *f)
{
	fprintf(f,
		"#define SET_ALWAYS_CFLG(x) SET_CFLG(x)\n"
		"#define SET_ALWAYS_NFLG(x) SET_NFLG(x)\n"
    "\n");
}

static void generate_includes (FILE * f, int id)
{
	fprintf (f, "#include \"sysdeps.h\"\n");
	fprintf (f, "#include \"options.h\"\n");
	fprintf (f, "#include \"memory.h\"\n");
  if (using_ce || using_prefetch)
	  fprintf(f, "#include \"custom.h\"\n");
	fprintf (f, "#include \"newcpu.h\"\n");
	fprintf (f, "#include \"cpu_prefetch.h\"\n");
	fprintf (f, "#include \"cputbl.h\"\n");
	generate_macros(f);
}

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

static void generate_one_opcode (int rp)
{
	int idx;
	uae_u16 smsk, dmsk;
	unsigned int opcode = opcode_map[rp];

	brace_level = 0;

	if (table68k[opcode].mnemo == i_ILLG
		|| table68k[opcode].clev > cpu_level)
		return;

	for (idx = 0; lookuptab[idx].name[0]; idx++) {
		if (table68k[opcode].mnemo == lookuptab[idx].mnemo)
			break;
	}

	if (table68k[opcode].handler != -1)
		return;

	outbuffer[0] = 0;

	if (opcode_next_clev[rp] != cpu_level) {
		char *name = ua (lookuptab[idx].name);
		if (generate_stbl)
			fprintf (stblfile, "{ %sop_%04x_%d_ff, 0x%04x, %d, { %d, %d }, %d }, /* %s */\n",
				using_ce ? "(cpuop_func*)" : "", opcode, opcode_last_postfix[rp],
				opcode,
				cputbltmp[opcode].length, cputbltmp[opcode].disp020[0], cputbltmp[opcode].disp020[1], cputbltmp[opcode].branch, name);
		xfree (name);
		return;
	}
	fprintf (headerfile, "extern %s op_%04x_%d_nf;\n", 
		using_ce ? "cpuop_func_ce" : "cpuop_func", opcode, postfix);
	fprintf (headerfile, "extern %s op_%04x_%d_ff;\n", 
		using_ce ? "cpuop_func_ce" : "cpuop_func", opcode, postfix);
	out("/* %s */\n", outopcode (opcode));
	out("%s REGPARAM2 op_%04x_%d_ff(uae_u32 opcode)\n{\n", using_ce ? "void" : "uae_u32", opcode, postfix);
  int org_using_simple_cycles = using_simple_cycles;
  if(count_required(opcode) && !using_ce)
    using_simple_cycles = 1;
	if (using_simple_cycles)
		out("int count_cycles = 0;\n");

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
				out("uae_u32 srcreg = (uae_s32)(uae_s8)%d;\n", (int) table68k[opcode].sreg);
			else
				out("uae_u32 srcreg = %d;\n", (int) table68k[opcode].sreg);
		} else {
			char source[100];
			int pos = table68k[opcode].spos;

			if (pos)
				sprintf (source, "((opcode >> %d) & %d)", pos, smsk);
			else
				sprintf (source, "(opcode & %d)", smsk);

			if (table68k[opcode].stype == 3)
				out("uae_u32 srcreg = imm8_table[%s];\n", source);
			else if (table68k[opcode].stype == 1)
				out("uae_u32 srcreg = (uae_s32)(uae_s8)%s;\n", source);
			else
				out("uae_u32 srcreg = %s;\n", source);
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
				out("uae_u32 dstreg = (uae_s32)(uae_s8)%d;\n", (int) table68k[opcode].dreg);
			else
				out("uae_u32 dstreg = %d;\n", (int) table68k[opcode].dreg);
		} else {
			int pos = table68k[opcode].dpos;
			if (pos)
				out("uae_u32 dstreg = (opcode >> %d) & %d;\n",
				pos, dmsk);
			else
				out("uae_u32 dstreg = opcode & %d;\n", dmsk);
		}
	}
	count_readw = count_readl = count_writew = count_writel = count_ncycles = count_cycles = 0;
	gen_opcode (opcode);
  write_return_cycles(1);
	using_simple_cycles = org_using_simple_cycles;

	if ((opcode & 0xf000) == 0xf000)
		m68k_pc_total = -1;

	cputbltmp[opcode].length = m68k_pc_total;
	cputbltmp[opcode].disp020[0] = 0;
	if (genamode8r_offset[0] > 0)
		cputbltmp[opcode].disp020[0] = m68k_pc_total - genamode8r_offset[0] + 2;
	cputbltmp[opcode].disp020[1] = 0;
	if (genamode8r_offset[1] > 0)
		cputbltmp[opcode].disp020[1] = m68k_pc_total - genamode8r_offset[1] + 2;
	cputbltmp[opcode].branch = branch_inst;

	if (m68k_pc_total > 0)
		out("/* %d %d,%d %c */\n",
			m68k_pc_total, cputbltmp[opcode].disp020[0], cputbltmp[opcode].disp020[1], cputbltmp[opcode].branch ? 'B' : ' ');

	out("\n");
	opcode_next_clev[rp] = next_cpu_level;
	opcode_last_postfix[rp] = postfix;

	printf("%s", outbuffer);

	if (generate_stbl) {
		char *name = ua (lookuptab[idx].name);
		fprintf(stblfile, "{ %sop_%04x_%d_ff, 0x%04x, %d, { %d, %d }, %d }, /* %s */\n",
			using_ce ? "(cpuop_func*)" : "",
			opcode, postfix, opcode,
			cputbltmp[opcode].length, cputbltmp[opcode].disp020[0], cputbltmp[opcode].disp020[1], cputbltmp[opcode].branch, name);
		xfree (name);
	}
}

static void generate_func (void)
{
	int j, rp;

	/* sam: this is for people with low memory (eg. me :)) */
	out("\n"
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
		out("#ifdef PART_%d\n",j);
		for (; rp < k; rp++)
			generate_one_opcode (rp);
		out("#endif\n\n");
	}

	if (generate_stbl)
		fprintf(stblfile, "{ 0, 0 }};\n");
}

static void generate_cpu (int id)
{
	char fname[100];
	static int postfix2 = -1;
	int rp;

	postfix = id;
	if (id == 0 || id == 4 || id == 11 || id == 13 || id == 40 || id == 44) {
		if (generate_stbl && id != 4 && id != 44)
			fprintf(stblfile, "#ifdef CPUEMU_%d\n", postfix);
		postfix2 = postfix;
		sprintf(fname, "cpuemu_%d.cpp", postfix);
		if (freopen (fname, "wb", stdout) == NULL) {
			abort();
		}
		generate_includes (stdout, id);
	}

	using_exception_3 = 1;
	using_prefetch = 0;
	using_ce = 0;
	using_simple_cycles = 0;
	using_indirect = 0;
	cpu_generic = false;
	need_exception_oldpc = 0;

	if (id == 11 || id == 12) { // 11 = 68010 prefetch, 12 = 68000 prefetch
		cpu_level = id == 11 ? 1 : 0;
		using_prefetch = 1;
		using_exception_3 = 1;
		using_simple_cycles = 1;
		if (id == 11) {
			read_counts();
			for (rp = 0; rp < nr_cpuop_funcs; rp++)
				opcode_next_clev[rp] = cpu_level;
		}
	} else if (id == 13 || id == 14) { // 13 = 68010 cycle-exact, 14 = 68000 cycle-exact
		cpu_level = id == 13 ? 1 : 0;
		using_prefetch = 1;
		using_exception_3 = 1;
		using_ce = 1;
		if (id == 13) {
			read_counts();
			for (rp = 0; rp < nr_cpuop_funcs; rp++)
				opcode_next_clev[rp] = cpu_level;
		}
	} else if (id < 6) {
		cpu_level = 5 - (id - 0); // "generic"
		cpu_generic = true;
	} else if (id >= 40 && id < 46) {
		cpu_level = 5 - (id - 40); // "generic" + direct
		cpu_generic = true;
		need_exception_oldpc = 1;
		if (id == 40) {
				read_counts();
			for (rp = 0; rp < nr_cpuop_funcs; rp++)
				opcode_next_clev[rp] = cpu_level;
		}
		using_indirect = -1;
	}
 
	if (!using_indirect)
		using_indirect = using_ce;

	if (id == 4 || id == 44) {
		cpu_level = 1;
		for (rp = 0; rp < nr_cpuop_funcs; rp++) {
			opcode_next_clev[rp] = cpu_level;
		}
	}

	if (generate_stbl) {
		fprintf(stblfile, "const struct cputbl op_smalltbl_%d[] = {\n", postfix);
	}
	generate_func ();
	if (generate_stbl) {
		if (postfix2 >= 0 && postfix2 != 4 && postfix2 != 44)
			fprintf(stblfile, "#endif /* CPUEMU_%d */\n", postfix2);
	}
	postfix2 = -1;
}

int main(int argc, char *argv[])
{
	init_table68k();

	opcode_map =  xmalloc (int, nr_cpuop_funcs);
	opcode_last_postfix = xmalloc (int, nr_cpuop_funcs);
	opcode_next_clev = xmalloc (int, nr_cpuop_funcs);
	counts = xmalloc (unsigned long, 65536);
	read_counts();

	headerfile = fopen("cputbl.h", "wb");

	fprintf(headerfile, "#define HARDWARE_BUS_ERROR_EMULATION %d\n", using_bus_error);

	stblfile = fopen("cpustbl.cpp", "wb");
	generate_includes(stblfile, 0);

	for (int i = 0; i <= 45; i++) {
		if ((i >= 6 && i < 11) || (i > 14 && i < 40))
			continue;
		generate_stbl = 1;
		generate_cpu (i);
	}

	free (table68k);
	fclose(headerfile);
	fclose(stblfile);
	return 0;
}

void write_log (const TCHAR *format,...)
{
}
