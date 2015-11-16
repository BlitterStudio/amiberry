/*
 * gencpu.c - m68k emulation generator
 *
 * Copyright (c) 2009 ARAnyM dev team (see AUTHORS)
 * 
 * Inspired by Christian Bauer's Basilisk II
 *
 * This file is part of the ARAnyM project which builds a new and powerful
 * TOS/FreeMiNT compatible virtual machine running on almost any hardware.
 *
 * ARAnyM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ARAnyM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ARAnyM; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
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
static int using_exception_3;
static int cpu_level;

static int optimized_flags;

#define GF_APDI 1
#define GF_AD8R 2
#define GF_PC8R 4
#define GF_AA 7
#define GF_NOREFILL 8
#define GF_PREFETCH 16

/* For the current opcode, the next lower level that will have different code.
 * Initialized to -1 for each opcode. If it remains unchanged, indicates we
 * are done with that opcode.  */
static int next_cpu_level;

static int *opcode_map;
static int *opcode_next_clev;
static int *opcode_last_postfix;
static unsigned long *counts;
static int generate_stbl;

#define GENA_GETV_NO_FETCH	0
#define GENA_GETV_FETCH		1
#define GENA_GETV_FETCH_ALIGN 2
#define GENA_MOVEM_DO_INC	0
#define GENA_MOVEM_NO_INC	1
#define GENA_MOVEM_MOVE16	2

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
	abort ();
}

static char endlabelstr[80];
static int endlabelno = 0;
static int need_endlabel;

static int n_braces = 0, limit_braces;
static int m68k_pc_offset = 0;
static int insn_n_cycles;

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

static void returncycles (char *s, int cycles)
{
    printf ("%sreturn %d * CYCLE_UNIT / 2;\n", s, cycles);
}

static int isreg(amodes mode)
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
     default: abort ();
    }
    return 0;
}

static const char *bit_mask (int size)
{
    switch (size) {
     case sz_byte: return "0xff";
     case sz_word: return "0xffff";
     case sz_long: return "0xffffffff";
     default: abort ();
    }
    return 0;
}

static void gen_nextilong (char *type, char *name, int norefill)
{
    int r = m68k_pc_offset;
    m68k_pc_offset += 4;

	if (using_prefetch) {
	    if (norefill) {
		printf ("\t%s %s;\n", type, name);
		printf ("\t%s = get_word_prefetch (regs, %d) << 16;\n", name, r + 2);
		printf ("\t%s |= regs->irc;\n", name);
		insn_n_cycles += 4;
	    } else {
		printf ("\t%s %s = get_long_prefetch (regs, %d);\n", type, name, r + 2);
		insn_n_cycles += 8;
	    }
	} else if (using_indirect) {
	    insn_n_cycles += 8;
	    printf ("\t%s %s = get_ilongi (%d);\n", type, name, r);
	} else if (using_mmu) {
	    insn_n_cycles += 8;
	    printf ("\t%s %s = get_ilong_mmu (regs, %d);\n", type, name, r);
	} else {
	    insn_n_cycles += 8;
	    printf ("\t%s %s = get_ilong (regs, %d);\n", type, name, r);
	}
}

static const char *gen_nextiword (int norefill)
{
    static char buffer[80];
    int r = m68k_pc_offset;
    m68k_pc_offset += 2;

	if (using_prefetch) {
	    if (norefill) {
		sprintf (buffer, "regs->irc", r);
	    } else {
		sprintf (buffer, "get_word_prefetch (regs, %d)", r + 2);
		insn_n_cycles += 4;
	    }
	} else if (using_indirect) {
	    sprintf (buffer, "get_iwordi(%d)", r);
	    insn_n_cycles += 4;
	} else if (using_mmu) {
	    sprintf (buffer, "get_iword_mmu (regs, %d)", r);
	    insn_n_cycles += 4;
	} else {
	    sprintf (buffer, "get_iword (regs, %d)", r);
	    insn_n_cycles += 4;
	}
    return buffer;
}

static const char *gen_nextibyte (int norefill)
{
    static char buffer[80];
    int r = m68k_pc_offset;
    m68k_pc_offset += 2;

	if (using_prefetch) {
	    if (norefill) {
		sprintf (buffer, "(uae_u8)regs->irc", r);
	    } else {
		sprintf (buffer, "(uae_u8)get_word_prefetch (regs, %d)", r + 2);
		insn_n_cycles += 4;
	    }
	} else if (using_indirect)  {
	    sprintf (buffer, "get_ibytei (%d)", r);
	    insn_n_cycles += 4;
	} else if (using_mmu)  {
	    sprintf (buffer, "get_ibyte_mmu (%d)", r);
	    insn_n_cycles += 4;
	} else {
	    sprintf (buffer, "get_ibyte (regs, %d)", r);
	    insn_n_cycles += 4;
	}
    return buffer;
}

static void irc2ir (void)
{
    if (!using_prefetch)
	return;
    printf ("\tregs->ir = regs->irc;\n");
}

static int did_prefetch;

static void fill_prefetch_2 (void)
{
    if (!using_prefetch)
	return;
		printf ("\tget_word_prefetch (regs, %d);\n", m68k_pc_offset + 2);
    did_prefetch = 1;
    insn_n_cycles += 4;
}

static void fill_prefetch_1 (int o)
{
    if (!using_prefetch)
	return;
		printf ("\tget_word_prefetch (regs, %d);\n", o);
    did_prefetch = 1;
    insn_n_cycles += 4;
}

static void fill_prefetch_full (void)
{
    fill_prefetch_1 (0);
    irc2ir ();
    fill_prefetch_1 (2);
}

static void fill_prefetch_0 (void)
{
    if (!using_prefetch)
	return;
		printf ("\tget_word_prefetch (regs, 0);\n");
    did_prefetch = 1;
    insn_n_cycles += 4;
}

static void fill_prefetch_next_1 (void)
{
    irc2ir ();
    fill_prefetch_1 (m68k_pc_offset + 2);
}

static void fill_prefetch_next (void)
{
    fill_prefetch_next_1 ();
}

static void fill_prefetch_finish (void)
{
    if (did_prefetch || !using_prefetch)
	return;
    fill_prefetch_1 (m68k_pc_offset);
}

static void swap_opcode (void)
{
#if ((defined(HAVE_GET_WORD_UNSWAPPED)) && (!defined(FULLMMU)))
  printf ("\topcode = do_byteswap_16(opcode);\n");
#endif
}

static void sync_m68k_pc (void)
{
    if (m68k_pc_offset == 0)
	return;
    printf ("\tm68k_incpc (regs, %d);\n", m68k_pc_offset);
    m68k_pc_offset = 0;
}

/* getv == 1: fetch data; getv != 0: check for odd address. If movem != 0,
 * the calling routine handles Apdi and Aipi modes.
 * gb-- movem == 2 means the same thing but for a MOVE16 instruction */
static void genamode2 (amodes mode, char *reg, wordsizes size, char *name, int getv, int movem, int flags, int e3fudge)
{
    char namea[100];
    int m68k_pc_offset_last = m68k_pc_offset;

    sprintf (namea, "%sa", name);

    start_brace ();
    switch (mode) {
    case Dreg:
	if (movem)
	    abort ();
	if (getv == GENA_GETV_FETCH)
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
		printf ("\tuae_s16 %s = ((uae_s16*)&m68k_dreg(regs, %s))[1];\n", name, reg);
#else
		printf ("\tuae_s16 %s = m68k_dreg(regs, %s);\n", name, reg);
#endif
		break;
	    case sz_long:
		printf ("\tuae_s32 %s = m68k_dreg(regs, %s);\n", name, reg);
		break;
	    default:
		abort ();
	    }
	return;
    case Areg:
	if (movem)
	    abort ();
	if (getv == GENA_GETV_FETCH)
	    switch (size) {
	    case sz_word:
		printf ("\tuae_s16 %s = m68k_areg(regs, %s);\n", name, reg);
		break;
	    case sz_long:
		printf ("\tuae_s32 %s = m68k_areg(regs, %s);\n", name, reg);
		break;
	    default:
		abort ();
	    }
	return;
    case Aind:
	printf ("\tuaecptr %sa = m68k_areg(regs, %s);\n", name, reg);
	break;
    case Aipi:
	printf ("\tuaecptr %sa = m68k_areg(regs, %s);\n", name, reg);
	break;
    case Apdi:
	printf ("\tuaecptr %sa;\n", name);
	switch (size) {
	case sz_byte:
	    if (movem)
		printf ("\t%sa = m68k_areg(regs, %s);\n", name, reg);
	    else
		printf ("\t%sa = m68k_areg(regs, %s) - areg_byteinc[%s];\n", name, reg, reg);
	    break;
	case sz_word:
	    printf ("\t%sa = m68k_areg(regs, %s) - %d;\n", name, reg, movem ? 0 : 2);
	    break;
	case sz_long:
	    printf ("\t%sa = m68k_areg(regs, %s) - %d;\n", name, reg, movem ? 0 : 4);
	    break;
	default:
	    abort ();
	}
	if (!(flags & GF_APDI)) {
	    insn_n_cycles += 2;
	}
	break;
    case Ad16:
	printf ("\tuaecptr %sa = m68k_areg(regs, %s) + (uae_s32)(uae_s16)%s;\n", name, reg, gen_nextiword (flags & GF_NOREFILL));
	break;
    case Ad8r:
	printf ("\tuaecptr %sa;\n", name);
	if (cpu_level > 1) {
	    if (next_cpu_level < 1)
		next_cpu_level = 1;
	    sync_m68k_pc ();
	    start_brace ();
	    /* This would ordinarily be done in gen_nextiword, which we bypass.  */
	    insn_n_cycles += 4;
	    if (using_mmu)
		printf ("\t%sa = get_disp_ea_020 (regs, m68k_areg (regs, %s), next_iword_mmu (regs));\n", name, reg);
	    else
	  printf ("\t%sa = get_disp_ea_020(regs, m68k_areg(regs, %s), next_iword(regs));\n", name, reg);
	} else {
	    printf ("\t%sa = get_disp_ea_000(regs, m68k_areg(regs, %s), %s);\n", name, reg, gen_nextiword (flags & GF_NOREFILL));
	}
	if (!(flags & GF_AD8R)) {
	    insn_n_cycles += 2;
	}
	break;
    case PC16:
	printf ("\tuaecptr %sa = m68k_getpc (regs) + %d;\n", name, m68k_pc_offset);
	printf ("\t%sa += (uae_s32)(uae_s16)%s;\n", name, gen_nextiword (flags & GF_NOREFILL));
	break;
    case PC8r:
	printf ("\tuaecptr tmppc;\n");
	printf ("\tuaecptr %sa;\n", name);
	if (cpu_level > 1) {
	    if (next_cpu_level < 1)
		next_cpu_level = 1;
	    sync_m68k_pc ();
	    start_brace ();
	    /* This would ordinarily be done in gen_nextiword, which we bypass.  */
	    insn_n_cycles += 4;
	    printf ("\ttmppc = m68k_getpc(regs);\n");
	    if (using_mmu)
		printf ("\t%sa = get_disp_ea_020 (regs, tmppc, next_iword_mmu (regs));\n", name);
	    else
	  printf ("\t%sa = get_disp_ea_020(regs, tmppc, next_iword(regs));\n", name);
	} else {
	    printf ("\ttmppc = m68k_getpc(regs) + %d;\n", m68k_pc_offset);
	    printf ("\t%sa = get_disp_ea_000(regs, tmppc, %s);\n", name, gen_nextiword (flags & GF_NOREFILL));
	}
	if (!(flags & GF_PC8R)) {
	    insn_n_cycles += 2;
	}

	break;
    case absw:
	printf ("\tuaecptr %sa = (uae_s32)(uae_s16)%s;\n", name, gen_nextiword (flags & GF_NOREFILL));
	break;
    case absl:
	gen_nextilong ("uaecptr", namea, flags & GF_NOREFILL);
	break;
    case imm:
	if (getv != GENA_GETV_FETCH)
	    abort ();
	switch (size) {
	case sz_byte:
	    printf ("\tuae_s8 %s = %s;\n", name, gen_nextibyte (flags & GF_NOREFILL));
	    break;
	case sz_word:
	    printf ("\tuae_s16 %s = %s;\n", name, gen_nextiword (flags & GF_NOREFILL));
	    break;
	case sz_long:
	    gen_nextilong ("uae_s32", name, flags & GF_NOREFILL);
	    break;
	default:
	    abort ();
	}
	return;
    case imm0:
	if (getv != GENA_GETV_FETCH)
	    abort ();
	printf ("\tuae_s8 %s = %s;\n", name, gen_nextibyte (flags & GF_NOREFILL));
	return;
    case imm1:
	if (getv != GENA_GETV_FETCH)
	    abort ();
	printf ("\tuae_s16 %s = %s;\n", name, gen_nextiword (flags & GF_NOREFILL));
	return;
    case imm2:
	if (getv != GENA_GETV_FETCH)
	    abort ();
	gen_nextilong ("uae_s32", name, flags & GF_NOREFILL);
	return;
    case immi:
	if (getv != GENA_GETV_FETCH)
	    abort ();
	printf ("\tuae_u32 %s = %s;\n", name, reg);
	return;
    default:
	abort ();
    }

    /* We get here for all non-reg non-immediate addressing modes to
     * actually fetch the value. */

    if ((using_prefetch) && using_exception_3 && getv != GENA_GETV_NO_FETCH && size != sz_byte) {
	printf ("\tif (%sa & 1) {\n", name);
	printf ("\t\texception3 (opcode, m68k_getpc(regs) + %d, %sa);\n",
	    m68k_pc_offset_last + e3fudge, name);
	printf ("\t\tgoto %s;\n", endlabelstr);
	printf ("\t}\n");
	need_endlabel = 1;
	start_brace ();
    }

    if (flags & GF_PREFETCH)
	fill_prefetch_next ();

    if (getv == GENA_GETV_FETCH) {
	start_brace ();
  if (using_mmu) {
	    switch (size) {
	    case sz_byte: insn_n_cycles += 4; printf ("\tuae_s8 %s = uae_mmu_get_byte (%sa);\n", name, name); break;
	    case sz_word: insn_n_cycles += 4; printf ("\tuae_s16 %s = uae_mmu_get_word (%sa);\n", name, name); break;
	    case sz_long: insn_n_cycles += 8; printf ("\tuae_s32 %s = uae_mmu_get_long (%sa);\n", name, name); break;
	    default: abort ();
	    }
	} else {
	    switch (size) {
	    case sz_byte: insn_n_cycles += 4; printf ("\tuae_s8 %s = get_byte (%sa);\n", name, name); break;
	    case sz_word: insn_n_cycles += 4; printf ("\tuae_s16 %s = get_word (%sa);\n", name, name); break;
	    case sz_long: insn_n_cycles += 8; printf ("\tuae_s32 %s = get_long (%sa);\n", name, name); break;
	    default: abort ();
	    }
	}
    }

    /* We now might have to fix up the register for pre-dec or post-inc
     * addressing modes. */
    if (!movem)
	switch (mode) {
	case Aipi:
	    switch (size) {
	    case sz_byte:
		printf ("\tm68k_areg(regs, %s) += areg_byteinc[%s];\n", reg, reg);
		break;
	    case sz_word:
		printf ("\tm68k_areg(regs, %s) += 2;\n", reg);
		break;
	    case sz_long:
		printf ("\tm68k_areg(regs, %s) += 4;\n", reg);
		break;
	    default:
		abort ();
	    }
	    break;
	case Apdi:
	    printf ("\tm68k_areg (regs, %s) = %sa;\n", reg, name);
	    break;
	default:
	    break;
	}
}

static void genamode (amodes mode, char *reg, wordsizes size, char *name, int getv, int movem, int flags)
{
    genamode2 (mode, reg, size, name, getv, movem, flags, 0);
}
static void genamode_e3 (amodes mode, char *reg, wordsizes size, char *name, int getv, int movem, int flags, int e3fudge)
{
    genamode2 (mode, reg, size, name, getv, movem, flags, e3fudge);
}

static void genastore_2 (char *from, amodes mode, char *reg, wordsizes size, char *to, int store_dir)
{
    switch (mode) {
     case Dreg:
	switch (size) {
	 case sz_byte:
	    printf ("\tm68k_dreg(regs, %s) = (m68k_dreg(regs, %s) & ~0xff) | ((%s) & 0xff);\n", reg, reg, from);
	    break;
	 case sz_word:
	    printf ("\tm68k_dreg(regs, %s) = (m68k_dreg(regs, %s) & ~0xffff) | ((%s) & 0xffff);\n", reg, reg, from);
	    break;
	 case sz_long:
	    printf ("\tm68k_dreg(regs, %s) = (%s);\n", reg, from);
	    break;
	 default:
	    abort ();
	}
	break;
     case Areg:
	switch (size) {
	 case sz_word:
	    printf ("\tm68k_areg(regs, %s) = (uae_s32)(uae_s16)(%s);\n", reg, from);
	    break;
	 case sz_long:
	    printf ("\tm68k_areg(regs, %s) = (%s);\n", reg, from);
	    break;
	 default:
	    abort ();
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
  if (using_mmu) {
	    switch (size) {
	     case sz_byte:
		insn_n_cycles += 4;
		printf ("\uae_mmu_put_byte (%sa,%s);\n", to, from);
		break;
	     case sz_word:
		insn_n_cycles += 4;
		if (cpu_level < 2 && (mode == PC16 || mode == PC8r))
		    abort ();
		printf ("\uae_mmu_put_word (%sa,%s);\n", to, from);
		break;
	     case sz_long:
		insn_n_cycles += 8;
		if (cpu_level < 2 && (mode == PC16 || mode == PC8r))
		    abort ();
		printf ("\uae_mmu_put_long (%sa,%s);\n", to, from);
		break;
	     default:
		abort ();
	    }
	} else {
	    switch (size) {
	     case sz_byte:
		insn_n_cycles += 4;
		printf ("\tput_byte (%sa,%s);\n", to, from);
		break;
	     case sz_word:
		insn_n_cycles += 4;
		if (cpu_level < 2 && (mode == PC16 || mode == PC8r))
		    abort ();
		printf ("\tput_word (%sa,%s);\n", to, from);
		break;
	     case sz_long:
		insn_n_cycles += 8;
		if (cpu_level < 2 && (mode == PC16 || mode == PC8r))
		    abort ();
		printf ("\tput_long (%sa,%s);\n", to, from);
		break;
	     default:
		abort ();
	    }
	}
	break;
     case imm:
     case imm0:
     case imm1:
     case imm2:
     case immi:
	abort ();
	break;
     default:
	abort ();
    }
}

static void genastore (char *from, amodes mode, char *reg, wordsizes size, char *to)
{
    genastore_2 (from, mode, reg, size, to, 0);
}
static void genastore_rev (char *from, amodes mode, char *reg, wordsizes size, char *to)
{
    genastore_2 (from, mode, reg, size, to, 1);
}


static void genmovemel (uae_u16 opcode)
{
    char getcode[100];
    int size = table68k[opcode].size == sz_long ? 4 : 2;

    if (using_mmu) {
	    if (table68k[opcode].size == sz_long) {
	        strcpy (getcode, "uae_mmu_get_long (srca)");
	    } else {
	        strcpy (getcode, "(uae_s32)(uae_s16)uae_mmu_get_word (srca)");
	    }
    } else {
      if (table68k[opcode].size == sz_long) {
    strcpy (getcode, "get_long(srca)");
      } else {
    strcpy (getcode, "(uae_s32)(uae_s16)get_word(srca)");
      }
    }
    printf ("\tuae_u16 mask = %s;\n", gen_nextiword (0));
    printf ("\tunsigned int dmask = mask & 0xff, amask = (mask >> 8) & 0xff;\n");
    genamode (table68k[opcode].dmode, "dstreg", table68k[opcode].size, "src", GENA_GETV_FETCH_ALIGN, GENA_MOVEM_NO_INC, 0);
    start_brace ();
    printf ("\twhile (dmask) { m68k_dreg(regs, movem_index1[dmask]) = %s; srca += %d; dmask = movem_next[dmask]; }\n",
	    getcode, size);
    printf ("\twhile (amask) { m68k_areg(regs, movem_index1[amask]) = %s; srca += %d; amask = movem_next[amask]; }\n",
	    getcode, size);

    if (table68k[opcode].dmode == Aipi)
	printf ("\tm68k_areg(regs, dstreg) = srca;\n");
}

static void genmovemle (uae_u16 opcode)
{
    char putcode[100];
    int size = table68k[opcode].size == sz_long ? 4 : 2;
    if (using_mmu) {
	    if (table68k[opcode].size == sz_long) {
	        strcpy (putcode, "uae_mmu_put_long (srca");
	    } else {
	        strcpy (putcode, "uae_mmu_put_word (srca");
	    }
    } else {
      if (table68k[opcode].size == sz_long) {
	  strcpy (putcode, "put_long(srca");
      } else {
	  strcpy (putcode, "put_word(srca");
  	  }
    }

    printf ("\tuae_u16 mask = %s;\n", gen_nextiword (0));
    genamode (table68k[opcode].dmode, "dstreg", table68k[opcode].size, "src", 
			GENA_GETV_FETCH_ALIGN, GENA_MOVEM_NO_INC, 0);
    if (using_prefetch)
	sync_m68k_pc ();

    start_brace ();
    if (table68k[opcode].dmode == Apdi) {
	printf ("\tuae_u16 amask = mask & 0xff, dmask = (mask >> 8) & 0xff;\n");
	printf ("\tint type = get_cpu_model() >= 68020;\n");
	printf ("\twhile (amask) {\n");
  printf ("\t\tsrca -= %d;\n", size);
	printf ("\t\tif (type) m68k_areg(regs, dstreg) = srca;\n");
  printf ("\t\t%s, m68k_areg(regs, movem_index2[amask]));\n", putcode);
  printf ("\t\tamask = movem_next[amask];\n");
  printf ("\t}\n");
	printf ("\twhile (dmask) { srca -= %d; %s, m68k_dreg(regs, movem_index2[dmask])); dmask = movem_next[dmask]; }\n",
		size, putcode);
	printf ("\tm68k_areg(regs, dstreg) = srca;\n");
    } else {
	printf ("\tuae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;\n");
	printf ("\twhile (dmask) { %s, m68k_dreg(regs, movem_index1[dmask])); srca += %d; dmask = movem_next[dmask]; }\n",
		putcode, size);
	printf ("\twhile (amask) { %s, m68k_areg(regs, movem_index1[amask])); srca += %d; amask = movem_next[amask]; }\n",
		putcode, size);
    }
}

static void duplicate_carry (int n)
{
    int i;
    for (i = 0; i <= n; i++)
	printf ("\t");
    printf ("COPY_CARRY (&regs->ccrflags);\n");
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
	abort ();
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
	printf ("\tCLEAR_CZNV (&regs->ccrflags);\n");
	printf ("\tSET_ZFLG   (&regs->ccrflags, %s == 0);\n", vstr);
	printf ("\tSET_NFLG   (&regs->ccrflags, %s < 0);\n", vstr);
	break;
     case flag_logical_noclobber:
	printf ("\tSET_ZFLG (&regs->ccrflags, %s == 0);\n", vstr);
	printf ("\tSET_NFLG (&regs->ccrflags, %s < 0);\n", vstr);
	break;
     case flag_av:
	printf ("\tSET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));\n");
	break;
     case flag_sv:
	printf ("\tSET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));\n");
	break;
     case flag_z:
	printf ("\tSET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (%s == 0));\n", vstr);
	break;
     case flag_zn:
	printf ("\tSET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (%s == 0));\n", vstr);
	printf ("\tSET_NFLG (&regs->ccrflags, %s < 0);\n", vstr);
	break;
     case flag_add:
	printf ("\tSET_ZFLG (&regs->ccrflags, %s == 0);\n", vstr);
	printf ("\tSET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));\n");
	printf ("\tSET_CFLG (&regs->ccrflags, %s < %s);\n", undstr, usstr);
	duplicate_carry (0);
	printf ("\tSET_NFLG (&regs->ccrflags, flgn != 0);\n");
	break;
     case flag_sub:
	printf ("\tSET_ZFLG (&regs->ccrflags, %s == 0);\n", vstr);
	printf ("\tSET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));\n");
	printf ("\tSET_CFLG (&regs->ccrflags, %s > %s);\n", usstr, udstr);
	duplicate_carry (0);
	printf ("\tSET_NFLG (&regs->ccrflags, flgn != 0);\n");
	break;
     case flag_addx:
	printf ("\tSET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));\n"); /* minterm SON: 0x42 */
	printf ("\tSET_CFLG (&regs->ccrflags, flgs ^ ((flgs ^ flgo) & (flgo ^ flgn)));\n"); /* minterm SON: 0xD4 */
	duplicate_carry (0);
	break;
     case flag_subx:
	printf ("\tSET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgo ^ flgn));\n"); /* minterm SON: 0x24 */
	printf ("\tSET_CFLG (&regs->ccrflags, flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));\n"); /* minterm SON: 0xB2 */
	duplicate_carry (0);
	break;
     case flag_cmp:
	printf ("\tSET_ZFLG (&regs->ccrflags, %s == 0);\n", vstr);
	printf ("\tSET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));\n");
	printf ("\tSET_CFLG (&regs->ccrflags, %s > %s);\n", usstr, udstr);
	printf ("\tSET_NFLG (&regs->ccrflags, flgn != 0);\n");
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
	    printf ("\tSET_CZNV (&regs->ccrflags, olcznv | FLAGVAL_Z);\n");
	} else {
	    switch (size) {
	     case sz_byte: printf ("\toptflag_testb (regs, (uae_s8)(%s));\n", value); break;
	     case sz_word: printf ("\toptflag_testw (regs, (uae_s16)(%s));\n", value); break;
	     case sz_long: printf ("\toptflag_testl (regs, (uae_s32)(%s));\n", value); break;
	    }
	    printf ("\tIOR_CZNV (&regs->ccrflags, oldcznv);\n");
	}
	printf ("\t}\n");
	return;
     case flag_logical:
	if (strcmp (value, "0") == 0) {
	    printf ("\tSET_CZNV (&regs->ccrflags, FLAGVAL_Z);\n");
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
     default: abort ();
    }
}

static int source_is_imm1_8 (struct instr *i)
{
    return i->stype == 3;
}

static void gen_opcode (unsigned long int opcode)
{
    struct instr *curi = table68k + opcode;
    insn_n_cycles = using_prefetch ? 0 : 4;

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
	printf ("if (!regs->s) { Exception(8, regs, 0); goto %s; }\n", endlabelstr);
	need_endlabel = 1;
	start_brace ();
	break;
    case 3: /* privileged if size == word */
	if (curi->size == sz_byte)
	    break;
	printf ("if (!regs->s) { Exception(8, regs, 0); goto %s; }\n", endlabelstr);
	need_endlabel = 1;
	start_brace ();
	break;
    }
    switch (curi->mnemo) {
    case i_OR:
    case i_AND:
    case i_EOR:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	printf ("\tsrc %c= dst;\n", curi->mnemo == i_OR ? '|' : curi->mnemo == i_AND ? '&' : '^');
	genflags (flag_logical, curi->size, "src", "", "");
	fill_prefetch_next ();
	genastore ("src", curi->dmode, "dstreg", curi->size, "dst");
	break;
    case i_ORSR:
    case i_EORSR:
  insn_n_cycles += 12;
	printf ("\tMakeSR(regs);\n");
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	if (curi->size == sz_byte) {
	    printf ("\tsrc &= 0xFF;\n");
	}
	fill_prefetch_next ();
	printf ("\tregs->sr %c= src;\n", curi->mnemo == i_EORSR ? '^' : '|');
	printf ("\tMakeFromSR(regs);\n");
	break;
    case i_ANDSR:
      insn_n_cycles += 12;
	printf ("\tMakeSR(regs);\n");
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	if (curi->size == sz_byte) {
	    printf ("\tsrc |= 0xFF00;\n");
	}
	fill_prefetch_next ();
	printf ("\tregs->sr &= src;\n");
	printf ("\tMakeFromSR(regs);\n");
	break;
    case i_SUB:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next ();
	start_brace ();
	genflags (flag_sub, curi->size, "newv", "src", "dst");
	genastore ("newv", curi->dmode, "dstreg", curi->size, "dst");
	break;
    case i_SUBA:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	genamode (curi->dmode, "dstreg", sz_long, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next ();
	start_brace ();
	printf ("\tuae_u32 newv = dst - src;\n");
	genastore ("newv", curi->dmode, "dstreg", sz_long, "dst");
	break;
    case i_SUBX:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, GF_AA);
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, GF_AA);
	fill_prefetch_next ();
	start_brace ();
	printf ("\tuae_u32 newv = dst - src - (GET_XFLG (&regs->ccrflags) ? 1 : 0);\n");
	genflags (flag_subx, curi->size, "newv", "src", "dst");
	genflags (flag_zn, curi->size, "newv", "", "");
	genastore ("newv", curi->dmode, "dstreg", curi->size, "dst");
	break;
    case i_SBCD:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, GF_AA);
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, GF_AA);
	fill_prefetch_next ();
	start_brace ();
	printf ("\tuae_u16 newv_lo = (dst & 0xF) - (src & 0xF) - (GET_XFLG (&regs->ccrflags) ? 1 : 0);\n");
	printf ("\tuae_u16 newv_hi = (dst & 0xF0) - (src & 0xF0);\n");
	printf ("\tuae_u16 newv, tmp_newv;\n");
	printf ("\tint bcd = 0;\n");
	printf ("\tnewv = tmp_newv = newv_hi + newv_lo;\n");
	printf ("\tif (newv_lo & 0xF0) { newv -= 6; bcd = 6; };\n");
	printf ("\tif ((((dst & 0xFF) - (src & 0xFF) - (GET_XFLG (&regs->ccrflags) ? 1 : 0)) & 0x100) > 0xFF) { newv -= 0x60; }\n");
	printf ("\tSET_CFLG (&regs->ccrflags, (((dst & 0xFF) - (src & 0xFF) - bcd - (GET_XFLG (&regs->ccrflags) ? 1 : 0)) & 0x300) > 0xFF);\n");
	duplicate_carry (0);
	/* Manual says bits NV are undefined though a real 68040/060 don't change them */
	if (cpu_level >= xBCD_KEEPS_NV_FLAGS) {
	    if (next_cpu_level < xBCD_KEEPS_NV_FLAGS)
		next_cpu_level = xBCD_KEEPS_NV_FLAGS - 1;
	    genflags (flag_z, curi->size, "newv", "", "");
	} else {
		genflags (flag_zn, curi->size, "newv", "", "");
		printf ("\tSET_VFLG (&regs->ccrflags, (tmp_newv & 0x80) != 0 && (newv & 0x80) == 0);\n");
	}
	genastore ("newv", curi->dmode, "dstreg", curi->size, "dst");
	break;
    case i_ADD:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next ();
	start_brace ();
	genflags (flag_add, curi->size, "newv", "src", "dst");
	genastore ("newv", curi->dmode, "dstreg", curi->size, "dst");
	break;
    case i_ADDA:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	genamode (curi->dmode, "dstreg", sz_long, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next ();
	start_brace ();
	printf ("\tuae_u32 newv = dst + src;\n");
	genastore ("newv", curi->dmode, "dstreg", sz_long, "dst");
	break;
    case i_ADDX:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, GF_AA);
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, GF_AA);
	fill_prefetch_next ();
	start_brace ();
	printf ("\tuae_u32 newv = dst + src + (GET_XFLG (&regs->ccrflags) ? 1 : 0);\n");
	genflags (flag_addx, curi->size, "newv", "src", "dst");
	genflags (flag_zn, curi->size, "newv", "", "");
	genastore ("newv", curi->dmode, "dstreg", curi->size, "dst");
	break;
    case i_ABCD:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, GF_AA);
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, GF_AA);
	fill_prefetch_next ();
	start_brace ();
	printf ("\tuae_u16 newv_lo = (src & 0xF) + (dst & 0xF) + (GET_XFLG (&regs->ccrflags) ? 1 : 0);\n");
	printf ("\tuae_u16 newv_hi = (src & 0xF0) + (dst & 0xF0);\n");
	printf ("\tuae_u16 newv, tmp_newv;\n");
	printf ("\tint cflg;\n");
	printf ("\tnewv = tmp_newv = newv_hi + newv_lo;\n");
	printf ("\tif (newv_lo > 9) { newv += 6; }\n");
	printf ("\tcflg = (newv & 0x3F0) > 0x90;\n");
	printf ("\tif (cflg) newv += 0x60;\n");
	printf ("\tSET_CFLG (&regs->ccrflags, cflg);\n");
	duplicate_carry (0);
	/* Manual says bits NV are undefined though a real 68040 don't change them */
	if (cpu_level >= xBCD_KEEPS_NV_FLAGS) {
	    if (next_cpu_level < xBCD_KEEPS_NV_FLAGS)
		next_cpu_level = xBCD_KEEPS_NV_FLAGS - 1;
	    genflags (flag_z, curi->size, "newv", "", "");
	}
	else {
		genflags (flag_zn, curi->size, "newv", "", "");
		printf ("\tSET_VFLG (&regs->ccrflags, (tmp_newv & 0x80) == 0 && (newv & 0x80) != 0);\n");
	}
	genastore ("newv", curi->dmode, "dstreg", curi->size, "dst");
	break;
    case i_NEG:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next ();
	start_brace ();
	genflags (flag_sub, curi->size, "dst", "src", "0");
	genastore_rev ("dst", curi->smode, "srcreg", curi->size, "src");
	break;
    case i_NEGX:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next ();
	start_brace ();
	printf ("\tuae_u32 newv = 0 - src - (GET_XFLG (&regs->ccrflags) ? 1 : 0);\n");
	genflags (flag_subx, curi->size, "newv", "src", "0");
	genflags (flag_zn, curi->size, "newv", "", "");
	genastore_rev ("newv", curi->smode, "srcreg", curi->size, "src");
	break;
    case i_NBCD:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next ();
	start_brace ();
	printf ("\tuae_u16 newv_lo = - (src & 0xF) - (GET_XFLG (&regs->ccrflags) ? 1 : 0);\n");
	printf ("\tuae_u16 newv_hi = - (src & 0xF0);\n");
	printf ("\tuae_u16 newv;\n");
	printf ("\tint cflg;\n");
	printf ("\tif (newv_lo > 9) { newv_lo -= 6; }\n");
	printf ("\tnewv = newv_hi + newv_lo;\n");
	printf ("\tcflg = (newv & 0x1F0) > 0x90;\n");
	printf ("\tif (cflg) newv -= 0x60;\n");
	printf ("\tSET_CFLG (&regs->ccrflags, cflg);\n");
	duplicate_carry(0);
	/* Manual says bits NV are undefined though a real 68040 don't change them */
	if (cpu_level >= xBCD_KEEPS_NV_FLAGS) {
	    if (next_cpu_level < xBCD_KEEPS_NV_FLAGS)
		next_cpu_level = xBCD_KEEPS_NV_FLAGS - 1;
	    genflags (flag_z, curi->size, "newv", "", "");
	}
	else {
		genflags (flag_zn, curi->size, "newv", "", "");
	}
	genastore ("newv", curi->smode, "srcreg", curi->size, "src");
	break;
    case i_CLR:
	genamode (curi->smode, "srcreg", curi->size, "src", cpu_level == 0 ? GENA_GETV_FETCH : GENA_GETV_FETCH_ALIGN, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next ();
	genflags (flag_logical, curi->size, "0", "", "");
	genastore_rev ("0", curi->smode, "srcreg", curi->size, "src");
	break;
    case i_NOT:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next ();
	start_brace ();
	printf ("\tuae_u32 dst = ~src;\n");
	genflags (flag_logical, curi->size, "dst", "", "");
	genastore_rev ("dst", curi->smode, "srcreg", curi->size, "src");
	break;
    case i_TST:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next ();
	genflags (flag_logical, curi->size, "src", "", "");
	break;
    case i_BTST:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next ();
	if (curi->size == sz_byte)
	    printf ("\tsrc &= 7;\n");
	else
	    printf ("\tsrc &= 31;\n");
	printf ("\tSET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));\n");
	break;
    case i_BCHG:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next ();
	if (curi->size == sz_byte)
	    printf ("\tsrc &= 7;\n");
	else
	    printf ("\tsrc &= 31;\n");
	printf ("\tdst ^= (1 << src);\n");
	printf ("\tSET_ZFLG (&regs->ccrflags, ((uae_u32)dst & (1 << src)) >> src);\n");
	genastore ("dst", curi->dmode, "dstreg", curi->size, "dst");
	break;
    case i_BCLR:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next ();
	if (curi->size == sz_byte)
	    printf ("\tsrc &= 7;\n");
	else
	    printf ("\tsrc &= 31;\n");
	printf ("\tSET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));\n");
	printf ("\tdst &= ~(1 << src);\n");
	genastore ("dst", curi->dmode, "dstreg", curi->size, "dst");
	break;
    case i_BSET:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next ();
	if (curi->size == sz_byte)
	    printf ("\tsrc &= 7;\n");
	else
	    printf ("\tsrc &= 31;\n");
	printf ("\tSET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));\n");
	printf ("\tdst |= (1 << src);\n");
	genastore ("dst", curi->dmode, "dstreg", curi->size, "dst");
	break;
    case i_CMPM:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, GF_AA);
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, GF_AA);
	fill_prefetch_next ();
	start_brace ();
	genflags (flag_cmp, curi->size, "newv", "src", "dst");
	break;
    case i_CMP:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next ();
	start_brace ();
	genflags (flag_cmp, curi->size, "newv", "src", "dst");
	break;
    case i_CMPA:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	genamode (curi->dmode, "dstreg", sz_long, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next ();
	start_brace ();
	genflags (flag_cmp, sz_long, "newv", "src", "dst");
	break;
	/* The next two are coded a little unconventional, but they are doing
	 * weird things... */
    case i_MVPRM:
      insn_n_cycles += 4;
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);

	printf ("\tuaecptr memp = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)%s;\n", gen_nextiword (0));
	    if (curi->size == sz_word) {
		printf ("\tput_byte (memp, src >> 8); put_byte (memp + 2, src);\n");
	    } else {
		printf ("\tput_byte (memp, src >> 24); put_byte (memp + 2, src >> 16);\n");
		printf ("\tput_byte (memp + 4, src >> 8); put_byte (memp + 6, src);\n");
	    }
	fill_prefetch_next ();
	break;
    case i_MVPMR:
      insn_n_cycles += 4;
	printf ("\tuaecptr memp = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)%s;\n", gen_nextiword (0));
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH_ALIGN, GENA_MOVEM_DO_INC, 0);
	    if (curi->size == sz_word) {
		printf ("\tuae_u16 val = (get_byte (memp) << 8) + get_byte (memp + 2);\n");
	    } else {
		printf ("\tuae_u32 val = (get_byte (memp) << 24) + (get_byte (memp + 2) << 16)\n");
		printf ("              + (get_byte (memp + 4) << 8) + get_byte (memp + 6);\n");
	    }
	fill_prefetch_next ();
	genastore ("val", curi->dmode, "dstreg", curi->size, "dst");
	break;
    case i_MOVE:
    case i_MOVEA:
	{
	     /* moves have special prefetch sequences:
	      * - MOVE <x>,-(An) = prefetch is before writes
	      * - MOVE <x>,(xxx).L, the most stupid ever. 2 prefetches after write
	      *   if <x> is not register or immediate
	      * - all others = prefetch is done after writes
	      */
	    int prefetch_done = 0;
	    int dualprefetch = curi->dmode == absl && (curi->smode != Dreg && curi->smode != Areg && curi->smode != imm);
	    genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	    /* MOVE.L dx,(ax) exception3 PC points to next instruction. hackhackhack */
	    genamode_e3 (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH_ALIGN, GENA_MOVEM_DO_INC, 1 | (dualprefetch ? GF_NOREFILL : 0), curi->smode == Dreg && curi->dmode == Aind ? 2 : 0);
	    if (curi->mnemo == i_MOVEA && curi->size == sz_word)
		printf ("\tsrc = (uae_s32)(uae_s16)src;\n");
	    if (curi->dmode == Apdi) {
	    	fill_prefetch_next ();
	    	prefetch_done = 1;
	    }
	    genastore ("src", curi->dmode, "dstreg", curi->size, "dst");
	    if (curi->mnemo == i_MOVE)
		genflags (flag_logical, curi->size, "src", "", "");
	    sync_m68k_pc ();
	    if (dualprefetch) {
		fill_prefetch_full ();
		prefetch_done = 1;
	    }
	    if (!prefetch_done)
		fill_prefetch_next ();
	}
	break;
    case i_MVSR2:
	genamode (curi->smode, "srcreg", sz_word, "src", GENA_GETV_FETCH_ALIGN, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next ();
	printf ("\tMakeSR(regs);\n");
	if (curi->size == sz_byte)
	    genastore ("regs->sr & 0xff", curi->smode, "srcreg", sz_word, "src");
	else
	    genastore ("regs->sr", curi->smode, "srcreg", sz_word, "src");
	break;
    case i_MV2SR:
	genamode (curi->smode, "srcreg", sz_word, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	if (curi->size == sz_byte) {
	    printf ("\tMakeSR(regs);\n\tregs->sr &= 0xFF00;\n\tregs->sr |= src & 0xFF;\n");
	} else {
	    printf ("\tregs->sr = src;\n");
	}
	fill_prefetch_next ();
	printf ("\tMakeFromSR(regs);\n");
	break;
    case i_SWAP:
	genamode (curi->smode, "srcreg", sz_long, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next ();
	start_brace ();
	printf ("\tuae_u32 dst = ((src >> 16)&0xFFFF) | ((src&0xFFFF)<<16);\n");
	genflags (flag_logical, sz_long, "dst", "", "");
	genastore ("dst", curi->smode, "srcreg", sz_long, "src");
	break;
    case i_EXG:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next ();
	genastore ("dst", curi->smode, "srcreg", curi->size, "src");
	genastore ("src", curi->dmode, "dstreg", curi->size, "dst");
	break;
    case i_EXT:
	genamode (curi->smode, "srcreg", sz_long, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next ();
	start_brace ();
	switch (curi->size) {
	case sz_byte: printf ("\tuae_u32 dst = (uae_s32)(uae_s8)src;\n"); break;
	case sz_word: printf ("\tuae_u16 dst = (uae_s16)(uae_s8)src;\n"); break;
	case sz_long: printf ("\tuae_u32 dst = (uae_s32)(uae_s16)src;\n"); break;
	default: abort ();
	}
	genflags (flag_logical,
		  curi->size == sz_word ? sz_word : sz_long, "dst", "", "");
	genastore ("dst", curi->smode, "srcreg",
		   curi->size == sz_word ? sz_word : sz_long, "src");
	break;
    case i_MVMEL:
	    genmovemel (opcode);
	fill_prefetch_next ();
	break;
    case i_MVMLE:
	    genmovemle (opcode);
	fill_prefetch_next ();
	break;
    case i_TRAP:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	sync_m68k_pc ();
	printf ("\tException (src + 32, regs, 0);\n");
	did_prefetch = 1;
	m68k_pc_offset = 0;
	break;
    case i_MVR2USP:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next ();
	printf ("\tregs->usp = src;\n");
	break;
    case i_MVUSP2R:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH_ALIGN, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next ();
	genastore ("regs->usp", curi->smode, "srcreg", curi->size, "src");
	break;
    case i_RESET:
	fill_prefetch_next ();
	printf ("\tcpureset();\n");
	if (using_prefetch)
	    printf ("\tregs->irc = get_iword(regs, 4);\n");
	break;
    case i_NOP:
	fill_prefetch_next ();
	break;
    case i_STOP:
	/* real stop do not prefetch anything, later... */
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	printf ("\tregs->sr = src;\n");
	printf ("\tMakeFromSR(regs);\n");
	printf ("\tm68k_setstopped(regs, 1);\n");
	sync_m68k_pc ();
	fill_prefetch_full ();
	break;
    case i_LPSTOP: /* 68060 */
	printf ("\tuae_u16 sw = get_iword (regs, 2);\n");
	printf ("\tuae_u16 sr;\n");
	printf ("\tif (sw != (0x100|0x80|0x40)) { Exception (4, regs, 0); goto %s; }\n", endlabelstr);
	printf ("\tsr = get_iword (regs, 4);\n");
	printf ("\tif (!(sr & 0x8000)) { Exception (8, regs, 0); goto %s; }\n", endlabelstr);
	printf ("\tregs->sr = sr;\n");
	printf ("\tMakeFromSR (regs);\n");
	printf ("\tm68k_setstopped(regs, 1);\n");
	m68k_pc_offset += 4;
	sync_m68k_pc ();
	fill_prefetch_full ();
	break;
    case i_RTE:
	if (cpu_level == 0) {
	    genamode (Aipi, "7", sz_word, "sr", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, GF_NOREFILL);
	    genamode (Aipi, "7", sz_long, "pc", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, GF_NOREFILL);
	    printf ("\tregs->sr = sr; m68k_setpc(regs, pc);\n");
	    printf ("\tMakeFromSR(regs);\n");
	} else {
	    int old_brace_level = n_braces;
	    if (next_cpu_level < 0)
		next_cpu_level = 0;
	    printf ("\tuae_u16 newsr; uae_u32 newpc; for (;;) {\n");
	    genamode (Aipi, "7", sz_word, "sr", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	    genamode (Aipi, "7", sz_long, "pc", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	    genamode (Aipi, "7", sz_word, "format", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	    printf ("\tnewsr = sr; newpc = pc;\n");
	    printf ("\tif ((format & 0xF000) == 0x0000) { break; }\n");
	    printf ("\telse if ((format & 0xF000) == 0x1000) { ; }\n");
	    printf ("\telse if ((format & 0xF000) == 0x2000) { m68k_areg(regs, 7) += 4; break; }\n");
	    printf ("\telse if ((format & 0xF000) == 0x4000) { m68k_areg(regs, 7) += 8; break; }\n");
	    printf ("\telse if ((format & 0xF000) == 0x7000) { m68k_areg(regs, 7) += 52; break; }\n");
	    printf ("\telse if ((format & 0xF000) == 0x8000) { m68k_areg(regs, 7) += 50; break; }\n");
	    printf ("\telse if ((format & 0xF000) == 0x9000) { m68k_areg(regs, 7) += 12; break; }\n");
	    printf ("\telse if ((format & 0xF000) == 0xa000) { m68k_areg(regs, 7) += 24; break; }\n");
	    printf ("\telse if ((format & 0xF000) == 0xb000) { m68k_areg(regs, 7) += 84; break; }\n");
	    printf ("\telse { Exception(14, regs, 0); goto %s; }\n", endlabelstr);
	    printf ("\tregs->sr = newsr; MakeFromSR(regs);\n}\n");
	    pop_braces (old_brace_level);
	    printf ("\tregs->sr = newsr; MakeFromSR(regs);\n");
	    printf ("\tif (newpc & 1)\n");
	    printf ("\t\texception3 (0x%04X, m68k_getpc(regs), newpc);\n", opcode);
	    printf ("\telse\n");
	    printf ("\t\tm68k_setpc(regs, newpc);\n");
	    need_endlabel = 1;
	}
	/* PC is set and prefetch filled. */
	m68k_pc_offset = 0;
	fill_prefetch_full ();
	break;
    case i_RTD:
	genamode (Aipi, "7", sz_long, "pc", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	genamode (curi->smode, "srcreg", curi->size, "offs", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	printf ("\tm68k_areg(regs, 7) += offs;\n");
	printf ("\tif (pc & 1)\n");
	printf ("\t\texception3 (0x%04X, m68k_getpc(regs), pc);\n", opcode);
	printf ("\telse\n");
	printf ("\t\tm68k_setpc(regs, pc);\n");
	/* PC is set and prefetch filled. */
	m68k_pc_offset = 0;
	fill_prefetch_full ();
	break;
    case i_LINK:
	genamode (Apdi, "7", sz_long, "old", GENA_GETV_FETCH_ALIGN, GENA_MOVEM_DO_INC, GF_AA);
	genamode (curi->smode, "srcreg", sz_long, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, GF_AA);
	genastore ("src", Apdi, "7", sz_long, "old");
	genastore ("m68k_areg(regs, 7)", curi->smode, "srcreg", sz_long, "src");
	genamode (curi->dmode, "dstreg", curi->size, "offs", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	printf ("\tm68k_areg(regs, 7) += offs;\n");
	fill_prefetch_next ();
	break;
    case i_UNLK:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	printf ("\tm68k_areg(regs, 7) = src;\n");
	genamode (Aipi, "7", sz_long, "old", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next ();
	genastore ("old", curi->smode, "srcreg", curi->size, "src");
	break;
    case i_RTS:
	if (using_indirect)
    printf ("\tm68k_do_rtsi(regs);\n");
	else
		printf ("\tm68k_do_rts(regs);\n");
	m68k_pc_offset = 0;
	fill_prefetch_full ();
	break;
    case i_TRAPV:
	sync_m68k_pc ();
	printf ("\tif (GET_VFLG (&regs->ccrflags)) {\n");
	printf ("\t\tException (7, regs, m68k_getpc (regs));\n");
	printf ("\t\tgoto %s;\n", endlabelstr);
	printf ("\t}\n");
	fill_prefetch_next ();
	need_endlabel = 1;
	break;
    case i_RTR:
	printf ("\tMakeSR(regs);\n");
	genamode (Aipi, "7", sz_word, "sr", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	genamode (Aipi, "7", sz_long, "pc", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	printf ("\tregs->sr &= 0xFF00; sr &= 0xFF;\n");
	printf ("\tregs->sr |= sr; m68k_setpc(regs, pc);\n");
	printf ("\tMakeFromSR(regs);\n");
	m68k_pc_offset = 0;
	fill_prefetch_full ();
	break;
    case i_JSR:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_NO_FETCH, GENA_MOVEM_DO_INC, GF_AA|GF_NOREFILL);
	start_brace ();
	printf ("\tuaecptr oldpc = m68k_getpc(regs) + %d;\n", m68k_pc_offset);
	if (using_exception_3) {
	    printf ("\tif (srca & 1) {\n");
	    printf ("\t\texception3i (opcode, oldpc, srca);\n");
	    printf ("\t\tgoto %s;\n", endlabelstr);
	    printf ("\t}\n");
	    need_endlabel = 1;
	}
	printf ("\tm68k_setpc (regs, srca);\n");
	m68k_pc_offset = 0;
	fill_prefetch_1 (0);
	printf("\tm68k_areg (regs, 7) -= 4;\n");
  printf("\tput_long (m68k_areg (regs, 7), oldpc);\n");
	fill_prefetch_next ();
	break;
    case i_JMP:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_NO_FETCH, GENA_MOVEM_DO_INC, GF_AA|GF_NOREFILL);
	if (using_exception_3) {
	    printf ("\tif (srca & 1) {\n");
	    printf ("\t\texception3i (opcode, m68k_getpc(regs) + 6, srca);\n");
	    printf ("\t\tgoto %s;\n", endlabelstr);
	    printf ("\t}\n");
	    need_endlabel = 1;
	}
	printf ("\tm68k_setpc(regs, srca);\n");
	m68k_pc_offset = 0;
	fill_prefetch_full ();
    break;
    case i_BSR:
	printf ("\tuae_s32 s;\n");
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, GF_AA|GF_NOREFILL);
	printf ("\ts = (uae_s32)src + 2;\n");
	if (using_exception_3) {
	    printf ("\tif (src & 1) {\n");
	    printf ("\t\texception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + s);\n");
	    printf ("\t\tgoto %s;\n", endlabelstr);
	    printf ("\t}\n");
	    need_endlabel = 1;
	}
	if (using_indirect)
	  printf ("\tm68k_do_bsri (regs, m68k_getpc(regs) + %d, s);\n", m68k_pc_offset);
	else
		printf ("\tm68k_do_bsr (regs, m68k_getpc(regs) + %d, s);\n", m68k_pc_offset);
	m68k_pc_offset = 0;
	fill_prefetch_full ();
	break;
    case i_Bcc:
	if (curi->size == sz_long) {
	    if (cpu_level < 2) {
		printf ("\tif (cctrue(&regs->ccrflags, %d)) {\n", curi->cc, endlabelstr);
		printf ("\t\texception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 1);\n");
		printf ("\t\tgoto %s;\n", endlabelstr);
		printf ("\t}\n");
		sync_m68k_pc ();
		irc2ir ();
		fill_prefetch_2 ();
		printf ("\tgoto %s;\n", endlabelstr);
		need_endlabel = 1;
	    } else {
		if (next_cpu_level < 1)
		    next_cpu_level = 1;
	    }
	}
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, GF_AA | GF_NOREFILL);
	printf ("\tif (!cctrue(&regs->ccrflags, %d)) goto didnt_jump;\n", curi->cc);
	if (using_exception_3) {
	    printf ("\tif (src & 1) {\n");
	    printf ("\t\texception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)src);\n");
	    printf ("\t\tgoto %s;\n", endlabelstr);
	    printf ("\t}\n");
	    need_endlabel = 1;
	}
	if (using_prefetch) {
	    if (curi->size == sz_byte) {
		printf ("\tm68k_incpc (regs, (uae_s32)src + 2);\n");
	    } else {
		printf ("\tm68k_incpc (regs, (uae_s32)src + 2);\n");
	    }
	    fill_prefetch_full ();
		printf ("\treturn 10 * CYCLE_UNIT / 2;\n");
	} else {
	    printf ("\tm68k_incpc (regs, (uae_s32)src + 2);\n");
	    returncycles ("\t", 10);
	}
	printf ("didnt_jump:;\n");
	need_endlabel = 1;
	sync_m68k_pc ();
	if (curi->size == sz_byte) {
	    irc2ir ();
	    fill_prefetch_2 ();
	} else
	    fill_prefetch_full ();
	insn_n_cycles = curi->size == sz_byte ? 8 : 12;
	break;
    case i_LEA:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_NO_FETCH, GENA_MOVEM_DO_INC, GF_AA);
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH_ALIGN, GENA_MOVEM_DO_INC, GF_AA);
	fill_prefetch_next ();
	genastore ("srca", curi->dmode, "dstreg", curi->size, "dst");
	break;
    case i_PEA:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_NO_FETCH, GENA_MOVEM_DO_INC, GF_AA);
	genamode (Apdi, "7", sz_long, "dst", GENA_GETV_FETCH_ALIGN, GENA_MOVEM_DO_INC, GF_AA);
	if (!(curi->smode == absw || curi->smode == absl))
		fill_prefetch_next ();
	genastore ("srca", Apdi, "7", sz_long, "dst");
	if ((curi->smode == absw || curi->smode == absl))
		fill_prefetch_next ();
	break;
    case i_DBcc:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, GF_AA | GF_NOREFILL);
	genamode (curi->dmode, "dstreg", curi->size, "offs", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, GF_AA | GF_NOREFILL);

	printf ("\tuaecptr oldpc = m68k_getpc(regs);\n");
	printf ("\tif (!cctrue(&regs->ccrflags, %d)) {\n", curi->cc);
	printf ("\t\tm68k_incpc(regs, (uae_s32)offs + 2);\n");
	printf ("\t"); fill_prefetch_1 (0);
	printf ("\t"); genastore ("(src-1)", curi->smode, "srcreg", curi->size, "src");

	printf ("\t\tif (src) {\n");
	if (using_exception_3) {
	    printf ("\t\t\tif (offs & 1) {\n");
	    printf ("\t\t\t\texception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)offs + 2);\n");
	    printf ("\t\t\t\tgoto %s;\n", endlabelstr);
	    printf ("\t\t\t}\n");
	    need_endlabel = 1;
	}
	irc2ir ();
	fill_prefetch_1 (2);
	returncycles ("\t\t\t", 12);
	printf ("\t\t}\n");
	printf ("\t} else {\n");
	printf ("\t}\n");
	printf ("\tm68k_setpc (regs, oldpc + %d);\n", m68k_pc_offset);
	m68k_pc_offset = 0;
	fill_prefetch_full ();
	insn_n_cycles = 12;
	need_endlabel = 1;
	break;
    case i_Scc:
	genamode (curi->smode, "srcreg", curi->size, "src", cpu_level == 0 ? GENA_GETV_FETCH : GENA_GETV_FETCH_ALIGN, GENA_MOVEM_DO_INC, 0);
	start_brace ();
	fill_prefetch_next();
	start_brace ();
	printf ("\tint val = cctrue(&regs->ccrflags, %d) ? 0xff : 0;\n", curi->cc);
	genastore ("val", curi->smode, "srcreg", curi->size, "src");
	break;
    case i_DIVU:
	printf ("\tuaecptr oldpc = m68k_getpc(regs);\n");
	genamode (curi->smode, "srcreg", sz_word, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	genamode (curi->dmode, "dstreg", sz_long, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	printf ("\tCLEAR_CZNV (&regs->ccrflags);\n");
	printf ("\tif (src == 0) {\n");
	if (cpu_level > 0) {
	    /* 68020 sets V when dividing by zero and N if dst is negative
	     * 68000 clears both
	     */
	    printf("\t\tSET_VFLG (&regs->ccrflags, 1);\n");
	    printf("\t\tif (dst < 0) SET_NFLG (&regs->ccrflags, 1);\n");
	}
	printf ("\t\tm68k_incpc (regs, %d);\n", m68k_pc_offset);
	printf ("\t\tException (5, regs, oldpc);\n");
	printf ("\t\tgoto %s;\n", endlabelstr);
	printf ("\t} else {\n");
	printf ("\t\tuae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;\n");
	printf ("\t\tuae_u32 rem = (uae_u32)dst %% (uae_u32)(uae_u16)src;\n");
	/* The N flag appears to be set each time there is an overflow.
	 * Weird. but 68020 only sets N when dst is negative.. */
	printf ("\t\tif (newv > 0xffff) {\n");
	printf ("\t\t\tSET_VFLG (&regs->ccrflags, 1);\n");
#ifdef UNDEF68020
	if (cpu_level >= 2)
	    printf ("\t\t\tif (currprefs.cpu_level == 0 || dst < 0) SET_NFLG (&regs, 1);\n");
	else /* ??? some 68000 revisions may not set NFLG when overflow happens.. */
#endif
	    printf ("\t\t\tSET_NFLG (&regs->ccrflags, 1);\n");
	printf ("\t\t} else {\n");
	printf ("\t\t"); genflags (flag_logical, sz_word, "newv", "", "");
	printf ("\t\t\tnewv = (newv & 0xffff) | ((uae_u32)rem << 16);\n");
	printf ("\t\t"); genastore ("newv", curi->dmode, "dstreg", sz_long, "dst");
	printf ("\t\t}\n");
	fill_prefetch_next ();
	sync_m68k_pc ();
	printf ("\t}\n");
	insn_n_cycles += 136 - (136 - 76) / 2; /* average */
	need_endlabel = 1;
	break;
    case i_DIVS:
	printf ("\tuaecptr oldpc = m68k_getpc(regs);\n");
	genamode (curi->smode, "srcreg", sz_word, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	genamode (curi->dmode, "dstreg", sz_long, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	printf ("\tCLEAR_CZNV (&regs->ccrflags);\n");
	printf ("\tif (src == 0) {\n");
	if (cpu_level > 0) {
	    /* 68020 sets V when dividing by zero. Z is also set.
	     * 68000 clears both
	     */
	    printf("\t\tSET_VFLG (&regs->ccrflags, 1);\n");
	    printf("\t\tSET_ZFLG (&regs->ccrflags, 1);\n");
	}
	printf ("\t\tm68k_incpc (regs, %d);\n", m68k_pc_offset);
	printf ("\t\tException (5, regs, oldpc);\n");
	printf ("\t\tgoto %s;\n", endlabelstr);
	printf ("\t} else {\n");
	printf ("\t\tuae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;\n");
	printf ("\t\tuae_u16 rem = (uae_s32)dst %% (uae_s32)(uae_s16)src;\n");
	printf ("\t\tif ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) {\n");
	printf ("\t\t\tSET_VFLG (&regs->ccrflags, 1);\n");
#ifdef UNDEF68020
	if (cpu_level > 0)
	    printf ("\t\t\tif (currprefs.cpu_level == 0) SET_NFLG (&regs, 1);\n");
	else
#endif
	    printf ("\t\t\tSET_NFLG (&regs->ccrflags, 1);\n");
	printf ("\t\t} else {\n");
	printf ("\t\t\tif (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;\n");
	genflags (flag_logical, sz_word, "newv", "", "");
	printf ("\t\t\tnewv = (newv & 0xffff) | ((uae_u32)rem << 16);\n");
	printf ("\t\t"); genastore ("newv", curi->dmode, "dstreg", sz_long, "dst");
	printf ("\t\t}\n");
	fill_prefetch_next ();
	sync_m68k_pc ();
	printf ("\t}\n");
	insn_n_cycles += 156 - (156 - 120) / 2; /* average */
	need_endlabel = 1;
	break;
    case i_MULU:
	genamode (curi->smode, "srcreg", sz_word, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	genamode (curi->dmode, "dstreg", sz_word, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next();
	start_brace ();
	printf ("\tuae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;\n");
	genflags (flag_logical, sz_long, "newv", "", "");
	genastore ("newv", curi->dmode, "dstreg", sz_long, "dst");
	sync_m68k_pc ();
	insn_n_cycles += (70 - 38) / 2 + 38; /* average */
	break;
    case i_MULS:
	genamode (curi->smode, "srcreg", sz_word, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	genamode (curi->dmode, "dstreg", sz_word, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next();
	start_brace ();
	printf ("\tuae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;\n");
	genflags (flag_logical, sz_long, "newv", "", "");
	genastore ("newv", curi->dmode, "dstreg", sz_long, "dst");
	insn_n_cycles += (70 - 38) / 2 + 38; /* average */
	break;
    case i_CHK:
	printf ("\tuaecptr oldpc = m68k_getpc(regs);\n");
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	sync_m68k_pc ();
	fill_prefetch_next ();
	printf ("\tif ((uae_s32)dst < 0) {\n");
	printf ("\t\tSET_NFLG (&regs->ccrflags, 1);\n");
	printf ("\t\tException (6, regs, oldpc);\n");
	printf ("\t\tgoto %s;\n", endlabelstr);
	printf ("\t} else if (dst > src) {\n");
	printf ("\t\tSET_NFLG (&regs->ccrflags, 0);\n");
	printf ("\t\tException (6, regs, oldpc);\n");
	printf ("\t\tgoto %s;\n", endlabelstr);
	printf ("\t}\n");
	need_endlabel = 1;
	break;
    case i_CHK2:
	printf ("\tuaecptr oldpc = m68k_getpc(regs);\n");
	genamode (curi->smode, "srcreg", curi->size, "extra", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH_ALIGN, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_0 ();
	printf ("\t{uae_s32 upper,lower,reg = regs->regs[(extra >> 12) & 15];\n");
	switch (curi->size) {
	case sz_byte:
	    printf ("\tlower = (uae_s32)(uae_s8)get_byte(dsta); upper = (uae_s32)(uae_s8)get_byte(dsta+1);\n");
	    printf ("\tif ((extra & 0x8000) == 0) reg = (uae_s32)(uae_s8)reg;\n");
	    break;
	case sz_word:
	    printf ("\tlower = (uae_s32)(uae_s16)get_word(dsta); upper = (uae_s32)(uae_s16)get_word(dsta+2);\n");
	    printf ("\tif ((extra & 0x8000) == 0) reg = (uae_s32)(uae_s16)reg;\n");
	    break;
	case sz_long:
	    printf ("\tlower = get_long(dsta); upper = get_long(dsta+4);\n");
	    break;
	default:
	    abort ();
	}
	printf ("\tSET_ZFLG (&regs->ccrflags, upper == reg || lower == reg);\n");
	printf ("\tSET_CFLG_ALWAYS (&regs->ccrflags, lower <= upper ? reg < lower || reg > upper : reg > upper || reg < lower);\n");
	printf ("\tif ((extra & 0x800) && GET_CFLG (&regs->ccrflags)) { Exception(6, regs, oldpc); goto %s; }\n}\n", endlabelstr);
	need_endlabel = 1;
	break;

    case i_ASR:
	genamode (curi->smode, "srcreg", curi->size, "cnt", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	genamode (curi->dmode, "dstreg", curi->size, "data", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next();
	start_brace ();
	switch (curi->size) {
	case sz_byte: printf ("\tuae_u32 val = (uae_u8)data;\n"); break;
	case sz_word: printf ("\tuae_u32 val = (uae_u16)data;\n"); break;
	case sz_long: printf ("\tuae_u32 val = data;\n"); break;
	default: abort ();
	}
	printf ("\tuae_u32 sign = (%s & val) >> %d;\n", cmask (curi->size), bit_size (curi->size) - 1);
	printf ("\tcnt &= 63;\n");
	printf ("\tCLEAR_CZNV (&regs->ccrflags);\n");
	printf ("\tif (cnt >= %d) {\n", bit_size (curi->size));
	printf ("\t\tval = %s & (uae_u32)-sign;\n", bit_mask (curi->size));
	printf ("\t\tSET_CFLG (&regs->ccrflags, sign);\n");
	duplicate_carry (1);
	if (source_is_imm1_8 (curi))
	    printf ("\t} else {\n");
	else
	    printf ("\t} else if (cnt > 0) {\n");
	printf ("\t\tval >>= cnt - 1;\n");
	printf ("\t\tSET_CFLG (&regs->ccrflags, val & 1);\n");
	duplicate_carry (1);
	printf ("\t\tval >>= 1;\n");
	printf ("\t\tval |= (%s << (%d - cnt)) & (uae_u32)-sign;\n",
		bit_mask (curi->size),
		bit_size (curi->size));
	printf ("\t\tval &= %s;\n", bit_mask (curi->size));
	printf ("\t}\n");
	genflags (flag_logical_noclobber, curi->size, "val", "", "");
	genastore ("val", curi->dmode, "dstreg", curi->size, "data");
	break;
    case i_ASL:
	genamode (curi->smode, "srcreg", curi->size, "cnt", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	genamode (curi->dmode, "dstreg", curi->size, "data", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next();
	start_brace ();
	switch (curi->size) {
	case sz_byte: printf ("\tuae_u32 val = (uae_u8)data;\n"); break;
	case sz_word: printf ("\tuae_u32 val = (uae_u16)data;\n"); break;
	case sz_long: printf ("\tuae_u32 val = data;\n"); break;
	default: abort ();
	}
	printf ("\tcnt &= 63;\n");
	printf ("\tCLEAR_CZNV (&regs->ccrflags);\n");
	printf ("\tif (cnt >= %d) {\n", bit_size (curi->size));
	printf ("\t\tSET_VFLG (&regs->ccrflags, val != 0);\n");
	printf ("\t\tSET_CFLG (&regs->ccrflags, cnt == %d ? val & 1 : 0);\n",
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
	printf ("\t\tSET_VFLG (&regs->ccrflags, (val & mask) != mask && (val & mask) != 0);\n");
	printf ("\t\tval <<= cnt - 1;\n");
	printf ("\t\tSET_CFLG (&regs->ccrflags, (val & %s) >> %d);\n", cmask (curi->size), bit_size (curi->size) - 1);
	duplicate_carry (1);
	printf ("\t\tval <<= 1;\n");
	printf ("\t\tval &= %s;\n", bit_mask (curi->size));
	printf ("\t}\n");
	genflags (flag_logical_noclobber, curi->size, "val", "", "");
	genastore ("val", curi->dmode, "dstreg", curi->size, "data");
	break;
    case i_LSR:
	genamode (curi->smode, "srcreg", curi->size, "cnt", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	genamode (curi->dmode, "dstreg", curi->size, "data", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next();
	start_brace ();
	switch (curi->size) {
	case sz_byte: printf ("\tuae_u32 val = (uae_u8)data;\n"); break;
	case sz_word: printf ("\tuae_u32 val = (uae_u16)data;\n"); break;
	case sz_long: printf ("\tuae_u32 val = data;\n"); break;
	default: abort ();
	}
	printf ("\tcnt &= 63;\n");
	printf ("\tCLEAR_CZNV (&regs->ccrflags);\n");
	printf ("\tif (cnt >= %d) {\n", bit_size (curi->size));
	printf ("\t\tSET_CFLG (&regs->ccrflags, (cnt == %d) & (val >> %d));\n",
		bit_size (curi->size), bit_size (curi->size) - 1);
	duplicate_carry (1);
	printf ("\t\tval = 0;\n");
	if (source_is_imm1_8 (curi))
	    printf ("\t} else {\n");
	else
	    printf ("\t} else if (cnt > 0) {\n");
	printf ("\t\tval >>= cnt - 1;\n");
	printf ("\t\tSET_CFLG (&regs->ccrflags, val & 1);\n");
	duplicate_carry (1);
	printf ("\t\tval >>= 1;\n");
	printf ("\t}\n");
	genflags (flag_logical_noclobber, curi->size, "val", "", "");
	genastore ("val", curi->dmode, "dstreg", curi->size, "data");
	break;
    case i_LSL:
	genamode (curi->smode, "srcreg", curi->size, "cnt", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	genamode (curi->dmode, "dstreg", curi->size, "data", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next();
	start_brace ();
	switch (curi->size) {
	case sz_byte: printf ("\tuae_u32 val = (uae_u8)data;\n"); break;
	case sz_word: printf ("\tuae_u32 val = (uae_u16)data;\n"); break;
	case sz_long: printf ("\tuae_u32 val = data;\n"); break;
	default: abort ();
	}
	printf ("\tcnt &= 63;\n");
	printf ("\tCLEAR_CZNV (&regs->ccrflags);\n");
	printf ("\tif (cnt >= %d) {\n", bit_size (curi->size));
	printf ("\t\tSET_CFLG (&regs->ccrflags, cnt == %d ? val & 1 : 0);\n",
		bit_size (curi->size));
	duplicate_carry (1);
	printf ("\t\tval = 0;\n");
	if (source_is_imm1_8 (curi))
	    printf ("\t} else {\n");
	else
	    printf ("\t} else if (cnt > 0) {\n");
	printf ("\t\tval <<= (cnt - 1);\n");
	printf ("\t\tSET_CFLG (&regs->ccrflags, (val & %s) >> %d);\n", cmask (curi->size), bit_size (curi->size) - 1);
	duplicate_carry (1);
	printf ("\t\tval <<= 1;\n");
	printf ("\tval &= %s;\n", bit_mask (curi->size));
	printf ("\t}\n");
	genflags (flag_logical_noclobber, curi->size, "val", "", "");
	genastore ("val", curi->dmode, "dstreg", curi->size, "data");
	break;
    case i_ROL:
	genamode (curi->smode, "srcreg", curi->size, "cnt", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	genamode (curi->dmode, "dstreg", curi->size, "data", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next();
	start_brace ();
	switch (curi->size) {
	case sz_byte: printf ("\tuae_u32 val = (uae_u8)data;\n"); break;
	case sz_word: printf ("\tuae_u32 val = (uae_u16)data;\n"); break;
	case sz_long: printf ("\tuae_u32 val = data;\n"); break;
	default: abort ();
	}
	printf ("\tcnt &= 63;\n");
	printf ("\tCLEAR_CZNV (&regs->ccrflags);\n");
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
	printf ("\tSET_CFLG (&regs->ccrflags, val & 1);\n");
	printf ("}\n");
	genflags (flag_logical_noclobber, curi->size, "val", "", "");
	genastore ("val", curi->dmode, "dstreg", curi->size, "data");
	break;
    case i_ROR:
	genamode (curi->smode, "srcreg", curi->size, "cnt", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	genamode (curi->dmode, "dstreg", curi->size, "data", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next();
	start_brace ();
	switch (curi->size) {
	case sz_byte: printf ("\tuae_u32 val = (uae_u8)data;\n"); break;
	case sz_word: printf ("\tuae_u32 val = (uae_u16)data;\n"); break;
	case sz_long: printf ("\tuae_u32 val = data;\n"); break;
	default: abort ();
	}
	printf ("\tcnt &= 63;\n");
	printf ("\tCLEAR_CZNV (&regs->ccrflags);\n");
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
	printf ("\tSET_CFLG (&regs->ccrflags, (val & %s) >> %d);\n", cmask (curi->size), bit_size (curi->size) - 1);
	printf ("\t}\n");
	genflags (flag_logical_noclobber, curi->size, "val", "", "");
	genastore ("val", curi->dmode, "dstreg", curi->size, "data");
	break;
    case i_ROXL:
	genamode (curi->smode, "srcreg", curi->size, "cnt", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	genamode (curi->dmode, "dstreg", curi->size, "data", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next();
	start_brace ();
	switch (curi->size) {
	case sz_byte: printf ("\tuae_u32 val = (uae_u8)data;\n"); break;
	case sz_word: printf ("\tuae_u32 val = (uae_u16)data;\n"); break;
	case sz_long: printf ("\tuae_u32 val = data;\n"); break;
	default: abort ();
	}
	printf ("\tcnt &= 63;\n");
	printf ("\tCLEAR_CZNV (&regs->ccrflags);\n");
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
	printf ("\tval = (((val << 1) | GET_XFLG (&regs->ccrflags)) << cnt) | (loval >> 1);\n");
	printf ("\tSET_XFLG (&regs->ccrflags, carry);\n");
	printf ("\tval &= %s;\n", bit_mask (curi->size));
	printf ("\t} }\n");
	printf ("\tSET_CFLG (&regs->ccrflags, GET_XFLG (&regs->ccrflags));\n");
	genflags (flag_logical_noclobber, curi->size, "val", "", "");
	genastore ("val", curi->dmode, "dstreg", curi->size, "data");
	break;
    case i_ROXR:
	genamode (curi->smode, "srcreg", curi->size, "cnt", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	genamode (curi->dmode, "dstreg", curi->size, "data", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next();
	start_brace ();
	switch (curi->size) {
	case sz_byte: printf ("\tuae_u32 val = (uae_u8)data;\n"); break;
	case sz_word: printf ("\tuae_u32 val = (uae_u16)data;\n"); break;
	case sz_long: printf ("\tuae_u32 val = data;\n"); break;
	default: abort ();
	}
	printf ("\tcnt &= 63;\n");
	printf ("\tCLEAR_CZNV (&regs->ccrflags);\n");
	if (source_is_imm1_8 (curi))
	    printf ("{");
	else {
	    force_range_for_rox ("cnt", curi->size);
	    printf ("\tif (cnt > 0) {\n");
	}
	printf ("\tcnt--;\n");
	printf ("\t{\n\tuae_u32 carry;\n");
	printf ("\tuae_u32 hival = (val << 1) | GET_XFLG (&regs->ccrflags);\n");
	printf ("\thival <<= (%d - cnt);\n", bit_size (curi->size) - 1);
	printf ("\tval >>= cnt;\n");
	printf ("\tcarry = val & 1;\n");
	printf ("\tval >>= 1;\n");
	printf ("\tval |= hival;\n");
	printf ("\tSET_XFLG (&regs->ccrflags, carry);\n");
	printf ("\tval &= %s;\n", bit_mask (curi->size));
	printf ("\t} }\n");
	printf ("\tSET_CFLG (&regs->ccrflags, GET_XFLG (&regs->ccrflags));\n");
	genflags (flag_logical_noclobber, curi->size, "val", "", "");
	genastore ("val", curi->dmode, "dstreg", curi->size, "data");
	break;
    case i_ASRW:
	genamode (curi->smode, "srcreg", curi->size, "data", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next ();
	start_brace ();
	switch (curi->size) {
	case sz_byte: printf ("\tuae_u32 val = (uae_u8)data;\n"); break;
	case sz_word: printf ("\tuae_u32 val = (uae_u16)data;\n"); break;
	case sz_long: printf ("\tuae_u32 val = data;\n"); break;
	default: abort ();
	}
	printf ("\tuae_u32 sign = %s & val;\n", cmask (curi->size));
	printf ("\tuae_u32 cflg = val & 1;\n");
	printf ("\tval = (val >> 1) | sign;\n");
	genflags (flag_logical, curi->size, "val", "", "");
	printf ("\tSET_CFLG (&regs->ccrflags, cflg);\n");
	duplicate_carry (0);
	genastore ("val", curi->smode, "srcreg", curi->size, "data");
	break;
    case i_ASLW:
	genamode (curi->smode, "srcreg", curi->size, "data", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next ();
	start_brace ();
	switch (curi->size) {
	case sz_byte: printf ("\tuae_u32 val = (uae_u8)data;\n"); break;
	case sz_word: printf ("\tuae_u32 val = (uae_u16)data;\n"); break;
	case sz_long: printf ("\tuae_u32 val = data;\n"); break;
	default: abort ();
	}
	printf ("\tuae_u32 sign = %s & val;\n", cmask (curi->size));
	printf ("\tuae_u32 sign2;\n");
	printf ("\tval <<= 1;\n");
	genflags (flag_logical, curi->size, "val", "", "");
	printf ("\tsign2 = %s & val;\n", cmask (curi->size));
	printf ("\tSET_CFLG (&regs->ccrflags, sign != 0);\n");
	duplicate_carry (0);

	printf ("\tSET_VFLG (&regs->ccrflags, GET_VFLG (&regs->ccrflags) | (sign2 != sign));\n");
	genastore ("val", curi->smode, "srcreg", curi->size, "data");
	break;
    case i_LSRW:
	genamode (curi->smode, "srcreg", curi->size, "data", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next ();
	start_brace ();
	switch (curi->size) {
	case sz_byte: printf ("\tuae_u32 val = (uae_u8)data;\n"); break;
	case sz_word: printf ("\tuae_u32 val = (uae_u16)data;\n"); break;
	case sz_long: printf ("\tuae_u32 val = data;\n"); break;
	default: abort ();
	}
	printf ("\tuae_u32 carry = val & 1;\n");
	printf ("\tval >>= 1;\n");
	genflags (flag_logical, curi->size, "val", "", "");
	printf ("SET_CFLG (&regs->ccrflags, carry);\n");
	duplicate_carry (0);
	genastore ("val", curi->smode, "srcreg", curi->size, "data");
	break;
    case i_LSLW:
	genamode (curi->smode, "srcreg", curi->size, "data", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next ();
	start_brace ();
	switch (curi->size) {
	case sz_byte: printf ("\tuae_u8 val = data;\n"); break;
	case sz_word: printf ("\tuae_u16 val = data;\n"); break;
	case sz_long: printf ("\tuae_u32 val = data;\n"); break;
	default: abort ();
	}
	printf ("\tuae_u32 carry = val & %s;\n", cmask (curi->size));
	printf ("\tval <<= 1;\n");
	genflags (flag_logical, curi->size, "val", "", "");
	printf ("SET_CFLG (&regs->ccrflags, carry >> %d);\n", bit_size (curi->size) - 1);
	duplicate_carry (0);
	genastore ("val", curi->smode, "srcreg", curi->size, "data");
	break;
    case i_ROLW:
	genamode (curi->smode, "srcreg", curi->size, "data", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next ();
	start_brace ();
	switch (curi->size) {
	case sz_byte: printf ("\tuae_u8 val = data;\n"); break;
	case sz_word: printf ("\tuae_u16 val = data;\n"); break;
	case sz_long: printf ("\tuae_u32 val = data;\n"); break;
	default: abort ();
	}
	printf ("\tuae_u32 carry = val & %s;\n", cmask (curi->size));
	printf ("\tval <<= 1;\n");
	printf ("\tif (carry)  val |= 1;\n");
	genflags (flag_logical, curi->size, "val", "", "");
	printf ("SET_CFLG (&regs->ccrflags, carry >> %d);\n", bit_size (curi->size) - 1);
	genastore ("val", curi->smode, "srcreg", curi->size, "data");
	break;
    case i_RORW:
	genamode (curi->smode, "srcreg", curi->size, "data", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next ();
	start_brace ();
	switch (curi->size) {
	case sz_byte: printf ("\tuae_u8 val = data;\n"); break;
	case sz_word: printf ("\tuae_u16 val = data;\n"); break;
	case sz_long: printf ("\tuae_u32 val = data;\n"); break;
	default: abort ();
	}
	printf ("\tuae_u32 carry = val & 1;\n");
	printf ("\tval >>= 1;\n");
	printf ("\tif (carry) val |= %s;\n", cmask (curi->size));
	genflags (flag_logical, curi->size, "val", "", "");
	printf ("SET_CFLG (&regs->ccrflags, carry);\n");
	genastore ("val", curi->smode, "srcreg", curi->size, "data");
	break;
    case i_ROXLW:
	genamode (curi->smode, "srcreg", curi->size, "data", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next ();
	start_brace ();
	switch (curi->size) {
	case sz_byte: printf ("\tuae_u8 val = data;\n"); break;
	case sz_word: printf ("\tuae_u16 val = data;\n"); break;
	case sz_long: printf ("\tuae_u32 val = data;\n"); break;
	default: abort ();
	}
	printf ("\tuae_u32 carry = val & %s;\n", cmask (curi->size));
	printf ("\tval <<= 1;\n");
	printf ("\tif (GET_XFLG (&regs->ccrflags)) val |= 1;\n");
	genflags (flag_logical, curi->size, "val", "", "");
	printf ("SET_CFLG (&regs->ccrflags, carry >> %d);\n", bit_size (curi->size) - 1);
	duplicate_carry (0);
	genastore ("val", curi->smode, "srcreg", curi->size, "data");
	break;
    case i_ROXRW:
	genamode (curi->smode, "srcreg", curi->size, "data", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next ();
	start_brace ();
	switch (curi->size) {
	case sz_byte: printf ("\tuae_u8 val = data;\n"); break;
	case sz_word: printf ("\tuae_u16 val = data;\n"); break;
	case sz_long: printf ("\tuae_u32 val = data;\n"); break;
	default: abort ();
	}
	printf ("\tuae_u32 carry = val & 1;\n");
	printf ("\tval >>= 1;\n");
	printf ("\tif (GET_XFLG (&regs->ccrflags)) val |= %s;\n", cmask (curi->size));
	genflags (flag_logical, curi->size, "val", "", "");
	printf ("SET_CFLG (&regs->ccrflags, carry);\n");
	duplicate_carry (0);
	genastore ("val", curi->smode, "srcreg", curi->size, "data");
	break;
    case i_MOVEC2:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next ();
	start_brace ();
	printf ("\tint regno = (src >> 12) & 15;\n");
	printf ("\tuae_u32 *regp = regs->regs + regno;\n");
	printf ("\tif (! m68k_movec2(src & 0xFFF, regp)) goto %s;\n", endlabelstr);
	break;
    case i_MOVE2C:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_next ();
	start_brace ();
	printf ("\tint regno = (src >> 12) & 15;\n");
	printf ("\tuae_u32 *regp = regs->regs + regno;\n");
	printf ("\tif (! m68k_move2c(src & 0xFFF, regp)) goto %s;\n", endlabelstr);
	break;
    case i_CAS:
    {
	int old_brace_level;
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_0 ();
	start_brace ();
	printf ("\tint ru = (src >> 6) & 7;\n");
	printf ("\tint rc = src & 7;\n");
	genflags (flag_cmp, curi->size, "newv", "m68k_dreg(regs, rc)", "dst");
	printf ("\tif (GET_ZFLG (&regs->ccrflags))");
	old_brace_level = n_braces;
	start_brace ();
	genastore ("(m68k_dreg(regs, ru))", curi->dmode, "dstreg", curi->size, "dst");
	pop_braces (old_brace_level);
	printf ("else");
	start_brace ();
	    switch (curi->size) {
	    case sz_byte:
		printf ("m68k_dreg(regs, rc) = (m68k_dreg(regs, rc) & ~0xff) | (dst & 0xff);\n");
		break;
	    case sz_word:
		printf ("m68k_dreg(regs, rc) = (m68k_dreg(regs, rc) & ~0xffff) | (dst & 0xffff);\n");
		break;
	    default:
	printf ("m68k_dreg(regs, rc) = dst;\n");
		break;
	    }
	pop_braces (old_brace_level);
    }
    break;
    case i_CAS2:
	genamode (curi->smode, "srcreg", curi->size, "extra", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	printf ("\tuae_u32 rn1 = regs->regs[(extra >> 28) & 15];\n");
	printf ("\tuae_u32 rn2 = regs->regs[(extra >> 12) & 15];\n");
	if (curi->size == sz_word) {
	    int old_brace_level = n_braces;
	    printf ("\tuae_u16 dst1 = get_word(rn1), dst2 = get_word(rn2);\n");
	    genflags (flag_cmp, curi->size, "newv", "m68k_dreg(regs, (extra >> 16) & 7)", "dst1");
	    printf ("\tif (GET_ZFLG (&regs->ccrflags)) {\n");
	    genflags (flag_cmp, curi->size, "newv", "m68k_dreg(regs, extra & 7)", "dst2");
	    printf ("\tif (GET_ZFLG (&regs->ccrflags)) {\n");
	    printf ("\tput_word(rn1, m68k_dreg(regs, (extra >> 22) & 7));\n");
	    printf ("\tput_word(rn1, m68k_dreg(regs, (extra >> 6) & 7));\n");
	    printf ("\t}}\n");
	    pop_braces (old_brace_level);
	    printf ("\tif (! GET_ZFLG (&regs->ccrflags)) {\n");
	    printf ("\tm68k_dreg(regs, (extra >> 22) & 7) = (m68k_dreg(regs, (extra >> 22) & 7) & ~0xffff) | (dst1 & 0xffff);\n");
	    printf ("\tm68k_dreg(regs, (extra >> 6) & 7) = (m68k_dreg(regs, (extra >> 6) & 7) & ~0xffff) | (dst2 & 0xffff);\n");
	    printf ("\t}\n");
	} else {
	    int old_brace_level = n_braces;
	    printf ("\tuae_u32 dst1 = get_long(rn1), dst2 = get_long(rn2);\n");
	    genflags (flag_cmp, curi->size, "newv", "m68k_dreg(regs, (extra >> 16) & 7)", "dst1");
	    printf ("\tif (GET_ZFLG (&regs->ccrflags)) {\n");
	    genflags (flag_cmp, curi->size, "newv", "m68k_dreg(regs, extra & 7)", "dst2");
	    printf ("\tif (GET_ZFLG (&regs->ccrflags)) {\n");
	    printf ("\tput_long(rn1, m68k_dreg(regs, (extra >> 22) & 7));\n");
	    printf ("\tput_long(rn1, m68k_dreg(regs, (extra >> 6) & 7));\n");
	    printf ("\t}}\n");
	    pop_braces (old_brace_level);
	    printf ("\tif (! GET_ZFLG (&regs->ccrflags)) {\n");
	    printf ("\tm68k_dreg(regs, (extra >> 22) & 7) = dst1;\n");
	    printf ("\tm68k_dreg(regs, (extra >> 6) & 7) = dst2;\n");
	    printf ("\t}\n");
	}
	break;
    case i_MOVES:		/* ignore DFC and SFC because we have no MMU */
    {
	int old_brace_level;
	genamode (curi->smode, "srcreg", curi->size, "extra", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	printf ("\tif (extra & 0x800)\n");
	old_brace_level = n_braces;
	start_brace ();
	printf ("\tuae_u32 src = regs->regs[(extra >> 12) & 15];\n");
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH_ALIGN, GENA_MOVEM_DO_INC, 0);
	genastore ("src", curi->dmode, "dstreg", curi->size, "dst");
	pop_braces (old_brace_level);
	printf ("else");
	start_brace ();
	genamode (curi->dmode, "dstreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	printf ("\tif (extra & 0x8000) {\n");
	switch (curi->size) {
	case sz_byte: printf ("\tm68k_areg(regs, (extra >> 12) & 7) = (uae_s32)(uae_s8)src;\n"); break;
	case sz_word: printf ("\tm68k_areg(regs, (extra >> 12) & 7) = (uae_s32)(uae_s16)src;\n"); break;
	case sz_long: printf ("\tm68k_areg(regs, (extra >> 12) & 7) = src;\n"); break;
	default: abort ();
	}
	printf ("\t} else {\n");
	genastore ("src", Dreg, "(extra >> 12) & 7", curi->size, "");
	printf ("\t}\n");
	pop_braces (old_brace_level);
    }
    break;
    case i_BKPT:		/* only needed for hardware emulators */
	sync_m68k_pc ();
	printf ("\top_illg(opcode, regs);\n");
	break;
    case i_CALLM:		/* not present in 68030 */
	sync_m68k_pc ();
	printf ("\top_illg(opcode, regs);\n");
	break;
    case i_RTM:		/* not present in 68030 */
	sync_m68k_pc ();
	printf ("\top_illg(opcode, regs);\n");
	break;
    case i_TRAPcc:
	if (curi->smode != am_unknown && curi->smode != am_illg)
	    genamode (curi->smode, "srcreg", curi->size, "dummy", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	fill_prefetch_0 ();
	printf ("\tif (cctrue(&regs->ccrflags, %d)) { Exception(7, regs, m68k_getpc(regs)); goto %s; }\n", curi->cc, endlabelstr);
	need_endlabel = 1;
	break;
    case i_DIVL:
	sync_m68k_pc ();
	start_brace ();
	printf ("\tuaecptr oldpc = m68k_getpc(regs);\n");
	genamode (curi->smode, "srcreg", curi->size, "extra", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	sync_m68k_pc ();
	printf ("\tm68k_divl(opcode, dst, extra, oldpc);\n");
	break;
    case i_MULL:
	genamode (curi->smode, "srcreg", curi->size, "extra", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	genamode (curi->dmode, "dstreg", curi->size, "dst", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
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
	genamode (curi->smode, "srcreg", curi->size, "extra", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	genamode (curi->dmode, "dstreg", sz_long, "dst", GENA_GETV_FETCH_ALIGN, GENA_MOVEM_DO_INC, 0);
	start_brace ();
	printf ("\tuae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;\n");
	printf ("\tint width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;\n");
	if (curi->dmode == Dreg) {
	    printf ("\tuae_u32 tmp = m68k_dreg(regs, dstreg) << (offset & 0x1f);\n");
	} else {
	    printf ("\tuae_u32 tmp,bf0,bf1;\n");
	    printf ("\tdsta += (offset >> 3) | (offset & 0x80000000 ? ~0x1fffffff : 0);\n");
	    printf ("\tbf0 = get_long(dsta);bf1 = get_byte(dsta+4) & 0xff;\n");
	    printf ("\ttmp = (bf0 << (offset & 7)) | (bf1 >> (8 - (offset & 7)));\n");
	}
	printf ("\ttmp >>= (32 - width);\n");
	printf ("\tSET_NFLG_ALWAYS (&regs->ccrflags, tmp & (1 << (width-1)) ? 1 : 0);\n");
	printf ("\tSET_ZFLG (&regs->ccrflags, tmp == 0); SET_VFLG (&regs->ccrflags, 0); SET_CFLG (&regs->ccrflags, 0);\n");
	switch (curi->mnemo) {
	case i_BFTST:
	    break;
	case i_BFEXTU:
	    printf ("\tm68k_dreg(regs, (extra >> 12) & 7) = tmp;\n");
	    break;
	case i_BFCHG:
	    printf ("\ttmp = ~tmp;\n");
	    break;
	case i_BFEXTS:
	    printf ("\tif (GET_NFLG (&regs->ccrflags)) tmp |= width == 32 ? 0 : (-1 << width);\n");
	    printf ("\tm68k_dreg(regs, (extra >> 12) & 7) = tmp;\n");
	    break;
	case i_BFCLR:
	    printf ("\ttmp = 0;\n");
	    break;
	case i_BFFFO:
	    printf ("\t{ uae_u32 mask = 1 << (width-1);\n");
	    printf ("\twhile (mask) { if (tmp & mask) break; mask >>= 1; offset++; }}\n");
	    printf ("\tm68k_dreg(regs, (extra >> 12) & 7) = offset;\n");
	    break;
	case i_BFSET:
	    printf ("\ttmp = 0xffffffff;\n");
	    break;
	case i_BFINS:
	    printf ("\ttmp = m68k_dreg(regs, (extra >> 12) & 7);\n");
	    printf ("\tSET_NFLG (&regs->ccrflags, tmp & (1 << (width - 1)) ? 1 : 0);\n");
	    printf ("\tSET_ZFLG (&regs->ccrflags, tmp == 0);\n");
	    break;
	default:
	    break;
	}
	if (curi->mnemo == i_BFCHG
	    || curi->mnemo == i_BFCLR
	    || curi->mnemo == i_BFSET
	    || curi->mnemo == i_BFINS)
	    {
		printf ("\ttmp <<= (32 - width);\n");
		if (curi->dmode == Dreg) {
		    printf ("\tm68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ((offset & 0x1f) == 0 ? 0 :\n");
		    printf ("\t\t(0xffffffff << (32 - (offset & 0x1f))))) |\n");
		    printf ("\t\t(tmp >> (offset & 0x1f)) |\n");
		    printf ("\t\t(((offset & 0x1f) + width) >= 32 ? 0 :\n");
		    printf (" (m68k_dreg(regs, dstreg) & ((uae_u32)0xffffffff >> ((offset & 0x1f) + width))));\n");
		} else {
		    printf ("\tbf0 = (bf0 & (0xff000000 << (8 - (offset & 7)))) |\n");
		    printf ("\t\t(tmp >> (offset & 7)) |\n");
		    printf ("\t\t(((offset & 7) + width) >= 32 ? 0 :\n");
		    printf ("\t\t (bf0 & ((uae_u32)0xffffffff >> ((offset & 7) + width))));\n");
		    printf ("\tput_long(dsta,bf0 );\n");
		    printf ("\tif (((offset & 7) + width) > 32) {\n");
		    printf ("\t\tbf1 = (bf1 & (0xff >> (width - 32 + (offset & 7)))) |\n");
		    printf ("\t\t\t(tmp << (8 - (offset & 7)));\n");
		    printf ("\t\tput_byte(dsta+4,bf1);\n");
		    printf ("\t}\n");
		}
	    }
	break;
    case i_PACK:
	if (curi->smode == Dreg) {
	    printf ("\tuae_u16 val = m68k_dreg(regs, srcreg) + %s;\n", gen_nextiword (0));
	    printf ("\tm68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & 0xffffff00) | ((val >> 4) & 0xf0) | (val & 0xf);\n");
	} else {
	    printf ("\tuae_u16 val;\n");
	    printf ("\tm68k_areg(regs, srcreg) -= areg_byteinc[srcreg];\n");
	    printf ("\tval = (uae_u16)get_byte(m68k_areg(regs, srcreg));\n");
	    printf ("\tm68k_areg(regs, srcreg) -= areg_byteinc[srcreg];\n");
	    printf ("\tval = (val | ((uae_u16)get_byte(m68k_areg(regs, srcreg)) << 8)) + %s;\n", gen_nextiword (0));
	    printf ("\tm68k_areg(regs, dstreg) -= areg_byteinc[dstreg];\n");
	    printf ("\tput_byte(m68k_areg(regs, dstreg),((val >> 4) & 0xf0) | (val & 0xf));\n");
	}
	break;
    case i_UNPK:
	if (curi->smode == Dreg) {
	    printf ("\tuae_u16 val = m68k_dreg(regs, srcreg);\n");
	    printf ("\tval = (((val << 4) & 0xf00) | (val & 0xf)) + %s;\n", gen_nextiword (0));
	    printf ("\tm68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & 0xffff0000) | (val & 0xffff);\n");
	} else {
	    printf ("\tuae_u16 val;\n");
	    printf ("\tm68k_areg(regs, srcreg) -= areg_byteinc[srcreg];\n");
	    printf ("\tval = (uae_u16)get_byte(m68k_areg(regs, srcreg));\n");
	    printf ("\tval = (((val << 4) & 0xf00) | (val & 0xf)) + %s;\n", gen_nextiword (0));
	    printf ("\tm68k_areg(regs, dstreg) -= areg_byteinc[dstreg];\n");
	    printf ("\tput_byte(m68k_areg(regs, dstreg),val);\n");
	    printf ("\tm68k_areg(regs, dstreg) -= areg_byteinc[dstreg];\n");
	    printf ("\tput_byte(m68k_areg(regs, dstreg),val >> 8);\n");
	}
	break;
    case i_TAS:
	genamode (curi->smode, "srcreg", curi->size, "src", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	genflags (flag_logical, curi->size, "src", "", "");
	fill_prefetch_next ();
	if (1 || cpu_level >= 2 || curi->smode == Dreg) {
	printf ("\tsrc |= 0x80;\n");
    	if (next_cpu_level < 2)
		    next_cpu_level = 2 - 1;
	genastore ("src", curi->smode, "srcreg", curi->size, "src");
	} else {
	    /* not exactly like this either.. */
	    printf ("\tif (src >= 0x200000 || (src >= 0xc00000 && src < 0xe00000)) {\n");
	    printf ("\t    src |= 0x80; \n");
	    genastore ("src", curi->smode, "srcreg", curi->size, "src");
	    printf ("\t}\n");
	}
	break;
    case i_FPP:
	fpulimit();
	genamode (curi->smode, "srcreg", curi->size, "extra", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	sync_m68k_pc ();
	swap_opcode ();
	printf ("\tfpuop_arithmetic(opcode, regs, extra);\n");
	break;
    case i_FDBcc:
	fpulimit();
	genamode (curi->smode, "srcreg", curi->size, "extra", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	sync_m68k_pc ();
	swap_opcode ();
	printf ("\tfpuop_dbcc(opcode, regs, extra);\n");
	break;
    case i_FScc:
	fpulimit();
	genamode (curi->smode, "srcreg", curi->size, "extra", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	sync_m68k_pc ();
	swap_opcode ();
	printf ("\tfpuop_scc(opcode, regs, extra);\n");
	break;
    case i_FTRAPcc:
	fpulimit();
	sync_m68k_pc ();
	start_brace ();
	printf ("\tuaecptr oldpc = m68k_getpc(regs);\n");
	if (curi->smode != am_unknown && curi->smode != am_illg)
	    genamode (curi->smode, "srcreg", curi->size, "dummy", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	sync_m68k_pc ();
	swap_opcode ();
	printf ("\tfpuop_trapcc(opcode, regs, oldpc);\n");
	break;
    case i_FBcc:
	fpulimit();
	sync_m68k_pc ();
	start_brace ();
	printf ("\tuaecptr pc = m68k_getpc(regs);\n");
	genamode (curi->dmode, "srcreg", curi->size, "extra", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	sync_m68k_pc ();
	swap_opcode ();
	printf ("\tfpuop_bcc(opcode,regs, pc,extra);\n");
	break;
    case i_FSAVE:
	fpulimit();
	sync_m68k_pc ();
	swap_opcode ();
	printf ("\tfpuop_save(opcode, regs);\n");
	break;
    case i_FRESTORE:
	fpulimit();
	sync_m68k_pc ();
	swap_opcode ();
	printf ("\tfpuop_restore(opcode, regs);\n");
	break;

     case i_CINVL:
	printf ("\tif (opcode&0x80)\n"
		"\t\tflush_icache(31);\n");
	break;
     case i_CINVP:
	printf ("\tif (opcode&0x80)\n"
		"\t\tflush_icache(32);\n");
	break;
     case i_CINVA:
	printf ("\tif (opcode&0x80)\n"
		"\t\tflush_icache(33);\n");
	break;
     case i_CPUSHL:
	printf ("\tif (opcode&0x80)\n"
		"\t\tflush_icache(41);\n");
	break;
     case i_CPUSHP:
	printf ("\tif (opcode&0x80)\n"
		"\t\tflush_icache(42);\n");
	break;
     case i_CPUSHA:
	printf ("\tif (opcode&0x80)\n"
		"\t\tflush_icache(43);\n");
	break;
     case i_MOVE16:
	if ((opcode & 0xfff8) == 0xf620) {
	     /* MOVE16 (Ax)+,(Ay)+ */
	     printf ("\tuaecptr mems = m68k_areg(regs, srcreg) & ~15, memd;\n");
	     printf ("\tdstreg = (%s >> 12) & 7;\n", gen_nextiword(0));
	     printf ("\tmemd = m68k_areg(regs, dstreg) & ~15;\n");
	     printf ("\tput_long(memd, get_long(mems));\n");
	     printf ("\tput_long(memd+4, get_long(mems+4));\n");
	     printf ("\tput_long(memd+8, get_long(mems+8));\n");
	     printf ("\tput_long(memd+12, get_long(mems+12));\n");
	     printf ("\tif (srcreg != dstreg)\n");
	     printf ("\tm68k_areg(regs, srcreg) += 16;\n");
	     printf ("\tm68k_areg(regs, dstreg) += 16;\n");
	} else {
	     /* Other variants */
	     genamode (curi->smode, "srcreg", curi->size, "mems", GENA_GETV_NO_FETCH, GENA_MOVEM_MOVE16, 0);
	     genamode (curi->dmode, "dstreg", curi->size, "memd", GENA_GETV_NO_FETCH, GENA_MOVEM_MOVE16, 0);
	     printf ("\tmemsa &= ~15;\n");
	     printf ("\tmemda &= ~15;\n");
	     printf ("\tput_long(memda, get_long(memsa));\n");
	     printf ("\tput_long(memda+4, get_long(memsa+4));\n");
	     printf ("\tput_long(memda+8, get_long(memsa+8));\n");
	     printf ("\tput_long(memda+12, get_long(memsa+12));\n");
	     if ((opcode & 0xfff8) == 0xf600)
		 printf ("\tm68k_areg(regs, srcreg) += 16;\n");
	     else if ((opcode & 0xfff8) == 0xf608)
		 printf ("\tm68k_areg(regs, dstreg) += 16;\n");
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
	genamode (curi->smode, "srcreg", curi->size, "extra", GENA_GETV_FETCH, GENA_MOVEM_DO_INC, 0);
	sync_m68k_pc ();
	swap_opcode ();
	printf ("\tmmu_op(opcode, regs, extra);\n");
	break;
    case i_MMUOP030:
	printf ("\tuaecptr pc = m68k_getpc (regs);\n");
	if (curi->smode == Areg || curi->smode == Dreg)
	    printf("\tuae_u16 extraa = 0;\n");
  else
		genamode (curi->smode, "srcreg", curi->size, "extra", GENA_GETV_NO_FETCH, GENA_MOVEM_DO_INC, 0);
	sync_m68k_pc ();
	swap_opcode ();
	printf ("\tmmu_op30(pc, opcode, regs, 1, extraa);\n");
	break;
    default:
	abort ();
	break;
    }
    finish_braces ();
    if (limit_braces) {
	printf ("\n#endif\n");
	n_braces = limit_braces;
	limit_braces = 0;
	finish_braces ();
    }
    fill_prefetch_finish ();
    sync_m68k_pc ();
    did_prefetch = 0;
}

static void generate_includes (FILE * f)
{
    fprintf (f, "#include \"sysconfig.h\"\n");
    fprintf (f, "#include \"sysdeps.h\"\n");
    fprintf (f, "#include \"options.h\"\n");
    fprintf (f, "#include \"memory.h\"\n");
    fprintf (f, "#include \"custom.h\"\n");
    fprintf (f, "#include \"events.h\"\n");
    fprintf (f, "#include \"newcpu.h\"\n");
    fprintf (f, "#include \"cpu_prefetch.h\"\n");
    fprintf (f, "#include \"cputbl.h\"\n");

    fprintf (f, "#define CPUFUNC(x) x##_ff\n"
	     "#define SET_CFLG_ALWAYS(flags, x) SET_CFLG(flags, x)\n"
	     "#define SET_NFLG_ALWAYS(flags, x) SET_NFLG(flags, x)\n"
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
    strcpy (out, lookuptab[i].name);
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
    return out;
}

static void generate_one_opcode (int rp)
{
    int i;
    uae_u16 smsk, dmsk;
    long int opcode = opcode_map[rp];
    int i68000 = table68k[opcode].clev > 0;

    if (table68k[opcode].mnemo == i_ILLG
	|| table68k[opcode].clev > cpu_level)
	return;

    for (i = 0; lookuptab[i].name[0]; i++) {
	if (table68k[opcode].mnemo == lookuptab[i].mnemo)
	    break;
    }

    if (table68k[opcode].handler != -1)
	return;

    if (opcode_next_clev[rp] != cpu_level) {
	if (generate_stbl)
			fprintf (stblfile, "{ CPUFUNC(op_%04lx_%d), %ld }, /* %s */\n", opcode, opcode_last_postfix[rp],
		 		opcode, lookuptab[i].name);
	return;
    }
    if (generate_stbl) {
    if (i68000)
	fprintf (stblfile, "#ifndef CPUEMU_68000_ONLY\n");
    fprintf (stblfile, "{ CPUFUNC(op_%04lx_%d), %ld }, /* %s */\n", opcode, postfix, opcode, lookuptab[i].name);
    if (i68000)
	fprintf (stblfile, "#endif\n");
    }
    fprintf (headerfile, "extern cpuop_func op_%04lx_%d_nf;\n", opcode, postfix);
    fprintf (headerfile, "extern cpuop_func op_%04lx_%d_ff;\n", opcode, postfix);
    printf ("/* %s */\n", outopcode (opcode));
    if (i68000)
	printf("#ifndef CPUEMU_68000_ONLY\n");
    printf ("unsigned long REGPARAM2 CPUFUNC(op_%04lx_%d)(uae_u32 opcode, struct regstruct *regs)\n{\n", opcode, postfix);

    switch (table68k[opcode].stype) {
    case 0: smsk = 7; break;
    case 1: smsk = 255; break;
    case 2: smsk = 15; break;
    case 3: smsk = 7; break;
    case 4: smsk = 7; break;
    case 5: smsk = 63; break;
    case 6: smsk = 255; break;
    case 7: smsk = 3; break;
    default: abort ();
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

#if ((defined(HAVE_GET_WORD_UNSWAPPED)) && (!defined(FULLMMU)))

	    if (pos < 8 && (smsk >> (8 - pos)) != 0)
		sprintf (source, "(((opcode >> %d) | (opcode << %d)) & %d)",
			pos ^ 8, 8 - pos, dmsk);
	    else if (pos != 8)
		sprintf (source, "((opcode >> %d) & %d)", pos ^ 8, smsk);
	    else
		sprintf (source, "(opcode & %d)", smsk);

	    if (table68k[opcode].stype == 3)
		printf ("\tuae_u32 srcreg = imm8_table[%s];\n", source);
	    else if (table68k[opcode].stype == 1)
		printf ("\tuae_u32 srcreg = (uae_s32)(uae_s8)%s;\n", source);
	    else
		printf ("\tuae_u32 srcreg = %s;\n", source);

#else

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

#endif
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

#if ((defined(HAVE_GET_WORD_UNSWAPPED)) && (!defined(FULLMMU)))

	    if (pos < 8 && (dmsk >> (8 - pos)) != 0)
		printf ("\tuae_u32 dstreg = ((opcode >> %d) | (opcode << %d)) & %d;\n",
			pos ^ 8, 8 - pos, dmsk);
	    else if (pos != 8)
		printf ("\tuae_u32 dstreg = (opcode >> %d) & %d;\n",
			pos ^ 8, dmsk);
	    else
		printf ("\tuae_u32 dstreg = opcode & %d;\n", dmsk);

#else

	    if (pos)
		printf ("\tuae_u32 dstreg = (opcode >> %d) & %d;\n",
			pos, dmsk);
	    else
		printf ("\tuae_u32 dstreg = opcode & %d;\n", dmsk);

#endif
	}
    }
    need_endlabel = 0;
    endlabelno++;
    sprintf (endlabelstr, "endlabel%d", endlabelno);
    gen_opcode (opcode);
    if (need_endlabel)
	printf ("%s: ;\n", endlabelstr);
    returncycles ("", insn_n_cycles);
    printf ("}\n");
    if (i68000)
	printf("#endif\n");
    opcode_next_clev[rp] = next_cpu_level;
    opcode_last_postfix[rp] = postfix;
}

static void generate_func (void)
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
		   generate_one_opcode (rp);
		printf ("#endif\n\n");
	}

	if (generate_stbl)
		fprintf (stblfile, "{ 0, 0 }};\n");
}

int main (int argc, char **argv)
{
    int i, rp, postfix2;
    char fname[100];

    read_table68k ();
    do_merges ();

    opcode_map = (int *) malloc (sizeof (int) * nr_cpuop_funcs);
    opcode_last_postfix = (int *) malloc (sizeof (int) * nr_cpuop_funcs);
    opcode_next_clev = (int *) malloc (sizeof (int) * nr_cpuop_funcs);
    counts = (unsigned long *) malloc (65536 * sizeof (unsigned long));
    read_counts ();

    /* It would be a lot nicer to put all in one file (we'd also get rid of
     * cputbl.h that way), but cpuopti can't cope.  That could be fixed, but
     * I don't dare to touch the 68k version.  */

    headerfile = fopen ("cputbl.h", "wb");

    stblfile = fopen ("cpustbl.c", "wb");
    generate_includes (stblfile);

    using_prefetch = 0;
    using_indirect = 0;
    using_exception_3 = 1;

    postfix2 = -1;
    for (i = 0; i < 12; i++) {
	postfix = i;
	if (i >= 6 && i < 11)
	    continue;
	generate_stbl = 1;
	if (i == 0 || i == 11 || i == 12) {
	    if (generate_stbl)
	    	fprintf (stblfile, "#ifdef CPUEMU_%d\n", postfix);
	    postfix2 = postfix;
	    sprintf (fname, "cpuemu_%d.c", postfix);
	    freopen (fname, "wb", stdout);
	    generate_includes (stdout);
	}
	using_mmu = 0;
	cpu_level = 5 - i;
	if (i == 11 || i == 12) {
	    cpu_level = 0;
	    using_prefetch = 1;
	    using_exception_3 = 1;
	    for (rp = 0; rp < nr_cpuop_funcs; rp++)
		opcode_next_clev[rp] = 0;
	}
	if (generate_stbl) {
		if (i > 0 && i < 10)
			  fprintf (stblfile, "#ifndef CPUEMU_68000_ONLY\n");
		fprintf (stblfile, "const struct cputbl CPUFUNC(op_smalltbl_%d)[] = {\n", postfix);
		}
	generate_func ();
	if (generate_stbl) {
		if (i > 0 && i < 10)
			  fprintf (stblfile, "#endif /* CPUEMU_68000_ONLY */\n");
		if (postfix2 >= 0)
			  fprintf (stblfile, "#endif /* CPUEMU_%d */\n", postfix2);
	}
	postfix2 = -1;
    }

    free (table68k);
    fclose(headerfile);
    fclose(stblfile);
    return 0;
}
