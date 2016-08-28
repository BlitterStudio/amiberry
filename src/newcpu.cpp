/*
 * newcpu.cpp - CPU emulation
 *
 * Copyright (c) 2010 ARAnyM dev team (see AUTHORS)
 * 
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
  * MC68000 emulation
  *
  * (c) 1995 Bernd Schmidt
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "cpu_prefetch.h"
#include "autoconf.h"
#include "traps.h"
#include "debug.h"
#include "gui.h"
#include "savestate.h"
#include "blitter.h"
#include "ar.h"
#include "cia.h"
#include "inputdevice.h"

#ifdef JIT
#include "jit/compemu.h"
#include <signal.h>
#else
/* Need to have these somewhere */
static void build_comp(void) {}
bool check_prefs_changed_comp (void) { return false; }
#endif

/* Opcode of faulting instruction */
static uae_u16 last_op_for_exception_3;
/* PC at fault time */
static uaecptr last_addr_for_exception_3;
/* Address that generated the exception */
static uaecptr last_fault_for_exception_3;
/* read (0) or write (1) access */
static int last_writeaccess_for_exception_3;
/* instruction (1) or data (0) access */
static int last_instructionaccess_for_exception_3;
int cpu_cycles;

const int areg_byteinc[] = { 1,1,1,1,1,1,1,2 };
const int imm8_table[] = { 8,1,2,3,4,5,6,7 };

int movem_index1[256];
int movem_index2[256];
int movem_next[256];

cpuop_func *cpufunctbl[65536];

extern uae_u32 get_fpsr(void);

#define COUNT_INSTRS 0
#define MC68060_PCR   0x04300000
#define MC68EC060_PCR 0x04310000

static uae_u64 srp_030, crp_030;
static uae_u32 tt0_030, tt1_030, tc_030;
static uae_u16 mmusr_030;

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

STATIC_INLINE void count_instr (unsigned int opcode)
{
  instrcount[opcode]++;
}
#else
void dump_counts (void)
{
}
STATIC_INLINE void count_instr (unsigned int opcode)
{
}
#endif

static void set_cpu_caches(void)
{
#ifdef JIT
	if (currprefs.cachesize) {
    if (currprefs.cpu_model < 68040) {
    	set_cache_state(regs.cacr & 1);
    	if (regs.cacr & 0x08) {
	      flush_icache (0, 3);
    	}
    } else {
    	set_cache_state((regs.cacr & 0x8000) ? 1 : 0);
	  }
  }
#endif
	if (currprefs.cpu_model == 68020) {
		if (regs.cacr & 0x04) { // clear entry in instr cache
			regs.cacr &= ~0x04;
		}
  } else if (currprefs.cpu_model == 68030) {
		if (regs.cacr & 0x04) { // clear entry in instr cache
			regs.cacr &= ~0x04;
		}
		if (regs.cacr & 0x800) { // clear data cache
			regs.cacr &= ~0x800;
		}
		if (regs.cacr & 0x400) { // clear entry in data cache
			regs.cacr &= ~0x400;
		}
  }
}

STATIC_INLINE uae_u32 op_illg_1 (uae_u32 opcode, struct regstruct &regs)
{
	op_illg (opcode);
	return 4;
}
static uae_u32 REGPARAM2 op_unimpl_1 (uae_u32 opcode, struct regstruct &regs)
{
	op_illg (opcode);
	return 4;
}

static void build_cpufunctbl (void)
{
  int i;
  unsigned long opcode;
  const struct cputbl *tbl = 0;
  int lvl;

  switch (currprefs.cpu_model)
  {
#ifdef CPUEMU_0
#ifndef CPUEMU_68000_ONLY
	case 68040:
  	lvl = 4;
  	tbl = op_smalltbl_1_ff;
  	break;
	case 68030:
  	lvl = 3;
  	tbl = op_smalltbl_2_ff;
  	break;
	case 68020:
  	lvl = 2;
  	tbl = op_smalltbl_3_ff;
  	break;
	case 68010:
  	lvl = 1;
  	tbl = op_smalltbl_4_ff;
  	break;
#endif
#endif
	default:
  	changed_prefs.cpu_model = currprefs.cpu_model = 68000;
  	case 68000:
  	lvl = 0;
  	tbl = op_smalltbl_5_ff;
#ifdef CPUEMU_11
  	if (currprefs.cpu_compatible)
	    tbl = op_smalltbl_11_ff; /* prefetch */
#endif
  	break;
  }

  if (tbl == 0) {
		write_log (_T("no CPU emulation cores available CPU=%d!\n"), currprefs.cpu_model);
  	abort ();
  }

  for (opcode = 0; opcode < 65536; opcode++)
  	cpufunctbl[opcode] = op_illg_1;
  for (i = 0; tbl[i].handler != NULL; i++) {
    opcode = tbl[i].opcode;
  	cpufunctbl[opcode] = tbl[i].handler;
  }

  /* hack fpu to 68000/68010 mode */
  if (currprefs.fpu_model && currprefs.cpu_model < 68020) {
  	tbl = op_smalltbl_3_ff;
  	for (i = 0; tbl[i].handler != NULL; i++) {
	    if ((tbl[i].opcode & 0xfe00) == 0xf200)
    		cpufunctbl[tbl[i].opcode] = tbl[i].handler;
  	}
  }
  for (opcode = 0; opcode < 65536; opcode++) {
  	cpuop_func *f;
		instr *table = &table68k[opcode];

		if (table->mnemo == i_ILLG)
	    continue;

		/* unimplemented opcode? */
		if (table->unimpclev > 0 && lvl >= table->unimpclev) {
			if (currprefs.cpu_compatible && currprefs.cpu_model == 68060) {
				cpufunctbl[opcode] = op_unimpl_1;
			} else {
				cpufunctbl[opcode] = op_illg_1;
			}
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
    		abort();
	    cpufunctbl[opcode] = f;
  	}
  }
#ifdef JIT
  compiler_init ();
  build_comp ();
#endif
  set_cpu_caches ();
}

void fill_prefetch (void)
{
	if (currprefs.cpu_model >= 68020)
		return;
	regs.ir = x_get_word (m68k_getpc ());
	regs.irc = x_get_word (m68k_getpc () + 2);
}
static void fill_prefetch_quick (void)
{
	if (currprefs.cpu_model >= 68020)
		return;
	regs.ir = get_word (m68k_getpc ());
	regs.irc = get_word (m68k_getpc () + 2);
}

unsigned long cycles_shift;
unsigned long cycles_shift_2;

static void update_68k_cycles (void)
{
  cycles_shift = 0;
  cycles_shift_2 = 0;
  if(currprefs.m68k_speed == M68K_SPEED_14MHZ_CYCLES)
    cycles_shift = 1;
  else if(currprefs.m68k_speed == M68K_SPEED_25MHZ_CYCLES)
  {
    cycles_shift = 2;
    cycles_shift_2 = 5;
  }

  if(currprefs.m68k_speed <  0 || currprefs.cachesize > 0)
    do_cycles = do_cycles_cpu_fastest;
  else
    do_cycles = do_cycles_cpu_norm;
}

STATIC_INLINE unsigned long adjust_cycles(unsigned long cycles)
{
  unsigned long res = cycles >> cycles_shift;
  if(cycles_shift_2 > 0)
    return res + (cycles >> cycles_shift_2);
  else
    return res;
}


void check_prefs_changed_adr24 (void)
{
  if(currprefs.address_space_24 != changed_prefs.address_space_24)
  {
    currprefs.address_space_24 = changed_prefs.address_space_24;
    
    if (currprefs.address_space_24) {
    	regs.address_space_mask = 0x00ffffff;
    	fixup_prefs(&changed_prefs);
    } else {
    	regs.address_space_mask = 0xffffffff;
    }
  }
}

static void prefs_changed_cpu (void)
{
  fixup_cpu (&changed_prefs);
  currprefs.cpu_model = changed_prefs.cpu_model;
  currprefs.fpu_model = changed_prefs.fpu_model;
  currprefs.cpu_compatible = changed_prefs.cpu_compatible;
}

void check_prefs_changed_cpu (void)
{
  bool changed = false;

#ifdef JIT
  changed = check_prefs_changed_comp ();
#endif
  if (changed
	|| currprefs.cpu_model != changed_prefs.cpu_model
	|| currprefs.fpu_model != changed_prefs.fpu_model
	|| currprefs.cpu_compatible != changed_prefs.cpu_compatible) {

  	prefs_changed_cpu ();
  	if (!currprefs.cpu_compatible && changed_prefs.cpu_compatible)
  	  fill_prefetch_quick ();
  	build_cpufunctbl ();
  	changed = true;
  }
  if (changed 
    || currprefs.m68k_speed != changed_prefs.m68k_speed) {
	  currprefs.m68k_speed = changed_prefs.m68k_speed;
	  update_68k_cycles ();
	  changed = true;
  }

  if (changed) {
  	set_special (SPCFLAG_BRK);
		reset_frame_rate_hack ();
		set_speedup_values();
	}
}

void init_m68k (void)
{
  int i;

  prefs_changed_cpu ();
  update_68k_cycles ();

  for (i = 0 ; i < 256 ; i++) {
  	int j;
  	for (j = 0 ; j < 8 ; j++) {
  		if (i & (1 << j)) break;
  	}
  	movem_index1[i] = j;
  	movem_index2[i] = 7-j;
  	movem_next[i] = i & (~(1 << j));
  }

#if COUNT_INSTRS
	memset (instrcount, 0, sizeof instrcount);
#endif
	write_log (_T("Building CPU table for configuration: %d"), currprefs.cpu_model);
  regs.address_space_mask = 0xffffffff;
  if (currprefs.cpu_compatible) {
  	if (currprefs.address_space_24 && currprefs.cpu_model >= 68030)
	    currprefs.address_space_24 = false;
  }
	if (currprefs.fpu_model > 0)
		write_log (_T("/%d"), currprefs.fpu_model);
  if (currprefs.cpu_compatible) {
		if (currprefs.cpu_model <= 68020) {
			write_log (_T(" prefetch"));
		} else {
			write_log (_T(" fake prefetch"));
		}
  }
  if (currprefs.address_space_24) {
  	regs.address_space_mask = 0x00ffffff;
		write_log (_T(" 24-bit"));
  }
	write_log (_T("\n"));

  read_table68k ();
  do_merges ();

  build_cpufunctbl ();

#ifdef JIT
  /* We need to check whether NATMEM settings have changed
   * before starting the CPU */
  check_prefs_changed_comp ();
#endif
}

unsigned long nextevent, is_syncline, currcycle;

struct regstruct regs;

int get_cpu_model(void)
{
  return currprefs.cpu_model;
}

/*
* extract bitfield data from memory and return it in the MSBs
* bdata caches the unmodified data for put_bitfield()
*/
uae_u32 REGPARAM2 get_bitfield (uae_u32 src, uae_u32 bdata[2], uae_s32 offset, int width)
{
	uae_u32 tmp, res, mask;

	offset &= 7;
	mask = 0xffffffffu << (32 - width);
	switch ((offset + width + 7) >> 3) {
	case 1:
		tmp = get_byte (src);
		res = tmp << (24 + offset);
		bdata[0] = tmp & ~(mask >> (24 + offset));
		break;
	case 2:
		tmp = get_word (src);
		res = tmp << (16 + offset);
		bdata[0] = tmp & ~(mask >> (16 + offset));
		break;
	case 3:
		tmp = get_word (src);
		res = tmp << (16 + offset);
		bdata[0] = tmp & ~(mask >> (16 + offset));
		tmp = get_byte (src + 2);
		res |= tmp << (8 + offset);
		bdata[1] = tmp & ~(mask >> (8 + offset));
		break;
	case 4:
		tmp = get_long (src);
		res = tmp << offset;
		bdata[0] = tmp & ~(mask >> offset);
		break;
	case 5:
		tmp = get_long (src);
		res = tmp << offset;
		bdata[0] = tmp & ~(mask >> offset);
		tmp = get_byte (src + 4);
		res |= tmp >> (8 - offset);
		bdata[1] = tmp & ~(mask << (8 - offset));
		break;
	default:
		/* Panic? */
		write_log (_T("get_bitfield() can't happen %d\n"), (offset + width + 7) >> 3);
		res = 0;
		break;
	}
	return res;
}
/*
* write bitfield data (in the LSBs) back to memory, upper bits
* must be cleared already.
*/
void REGPARAM2 put_bitfield (uae_u32 dst, uae_u32 bdata[2], uae_u32 val, uae_s32 offset, int width)
{
	offset = (offset & 7) + width;
	switch ((offset + 7) >> 3) {
	case 1:
		put_byte (dst, bdata[0] | (val << (8 - offset)));
		break;
	case 2:
		put_word (dst, bdata[0] | (val << (16 - offset)));
		break;
	case 3:
		put_word (dst, bdata[0] | (val >> (offset - 16)));
		put_byte (dst + 2, bdata[1] | (val << (24 - offset)));
		break;
	case 4:
		put_long (dst, bdata[0] | (val << (32 - offset)));
		break;
	case 5:
		put_long (dst, bdata[0] | (val >> (offset - 32)));
		put_byte (dst + 4, bdata[1] | (val << (40 - offset)));
		break;
	default:
		write_log (_T("put_bitfield() can't happen %d\n"), (offset + 7) >> 3);
		break;
	}
}

uae_u32 REGPARAM2 _get_disp_ea_020 (struct regstruct &regs, uae_u32 base, uae_u32 dp)
{
  int reg = (dp >> 12) & 15;
  uae_s32 regd = regs.regs[reg];
  if ((dp & 0x800) == 0)
  	regd = (uae_s32)(uae_s16)regd;
  regd <<= (dp >> 9) & 3;
  if (dp & 0x100) {
  	uae_s32 outer = 0;
	  if (dp & 0x80) base = 0;
	  if (dp & 0x40) regd = 0;

	  if ((dp & 0x30) == 0x20) 
      base += (uae_s32)(uae_s16) next_iword (regs);
	  if ((dp & 0x30) == 0x30) 
      base += next_ilong (regs);

	  if ((dp & 0x3) == 0x2) 
      outer = (uae_s32)(uae_s16) next_iword (regs);
	  if ((dp & 0x3) == 0x3) 
      outer = next_ilong (regs);

	  if ((dp & 0x4) == 0) 
      base += regd;
	  if (dp & 0x3) 
      base = get_long (base);
	  if (dp & 0x4) 
      base += regd;

	  return base + outer;
  } else {
  	return base + (uae_s32)((uae_s8)dp) + regd;
  }
}

STATIC_INLINE int in_rom (uaecptr pc)
{
  return (munge24 (pc) & 0xFFF80000) == 0xF80000;
}

STATIC_INLINE int in_rtarea (uaecptr pc)
{
  return (munge24 (pc) & 0xFFFF0000) == rtarea_base && uae_boot_rom;
}

void REGPARAM2 MakeSR (struct regstruct &regs)
{
  regs.sr = ((regs.t1 << 15) | (regs.t0 << 14)
    | (regs.s << 13) | (regs.m << 12) | (regs.intmask << 8)
	  | (GET_XFLG() << 4) | (GET_NFLG() << 3)
	  | (GET_ZFLG() << 2) | (GET_VFLG() << 1)
	  |  GET_CFLG());
}

void REGPARAM2 MakeFromSR (struct regstruct &regs)
{
  int oldm = regs.m;
  int olds = regs.s;

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
  		    regs.msp = m68k_areg(regs, 7);
    		else
  		    regs.isp = m68k_areg(regs, 7);
    		m68k_areg(regs, 7) = regs.usp;
	    } else {
    		regs.usp = m68k_areg(regs, 7);
    		m68k_areg(regs, 7) = regs.m ? regs.msp : regs.isp;
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

  doint();
  if (regs.t1 || regs.t0)
  	set_special (SPCFLAG_TRACE);
  else
  	/* Keep SPCFLAG_DOTRACE, we still want a trace exception for
  	   SR-modifying instructions (including STOP).  */
  	unset_special (SPCFLAG_TRACE);
}

static void add_approximate_exception_cycles(int nr)
{
	int cycles;

	if (currprefs.cpu_model > 68000) {
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
  			case 7: cycles = 32 * CYCLE_UNIT / 2; break;		/* TRAPV */
  			case 8: cycles = 20 * CYCLE_UNIT / 2; break;		/* Privilege violation */
  			case 9: cycles = 25 * CYCLE_UNIT / 2; break;		/* Trace */
  			case 10: cycles = 20 * CYCLE_UNIT / 2; break;	/* Line-A */
  			case 11: cycles = 20 * CYCLE_UNIT / 2; break;	/* Line-F */
  			default:
  			cycles = 4 * CYCLE_UNIT / 2;
  			break;
  		}
  	}
	} else {	
  	// 68000 exceptions
  	if (nr >= 24 && nr <= 31) {
  		/* Interrupts */
  		cycles = (44 + 4) * CYCLE_UNIT / 2; 
  	} else if (nr >= 32 && nr <= 47) {
  		/* Trap (total is 34, but cpuemux.c already adds 4) */ 
  		cycles = (34 -4) * CYCLE_UNIT / 2;
  	} else {
  		switch (nr)
  		{
  			case 2: cycles = 50 * CYCLE_UNIT / 2; break;		/* Bus error */
  			case 3: cycles = 50 * CYCLE_UNIT / 2; break;		/* Address error */
  			case 4: cycles = 34 * CYCLE_UNIT / 2; break;		/* Illegal instruction */
  			case 5: cycles = 38 * CYCLE_UNIT / 2; break;		/* Division by zero */
  			case 6: cycles = 40 * CYCLE_UNIT / 2; break;		/* CHK */
  			case 7: cycles = 34 * CYCLE_UNIT / 2; break;		/* TRAPV */
  			case 8: cycles = 34 * CYCLE_UNIT / 2; break;		/* Privilege violation */
  			case 9: cycles = 34 * CYCLE_UNIT / 2; break;		/* Trace */
  			case 10: cycles = 34 * CYCLE_UNIT / 2; break;	/* Line-A */
  			case 11: cycles = 34 * CYCLE_UNIT / 2; break;	/* Line-F */
  			default:
    			cycles = 4 * CYCLE_UNIT / 2;
    			break;
  		}
  	}
  }
	cycles = adjust_cycles(cycles);
	do_cycles(cycles);
}

static void exception_trace (int nr)
{
  unset_special (SPCFLAG_TRACE | SPCFLAG_DOTRACE);
  if (regs.t1 && !regs.t0) {
  	/* trace stays pending if exception is div by zero, chk,
  	 * trapv or trap #x
  	 */
  	if (nr == 5 || nr == 6 || nr ==  7 || (nr >= 32 && nr <= 47))
	    set_special (SPCFLAG_DOTRACE);
  }
  regs.t1 = regs.t0 = regs.m = 0;
}

static uae_u32 exception_pc (int nr)
{
	// zero divide, chk, trapcc/trapv, trace, trap#
	if (nr == 5 || nr == 6 || nr == 7 || nr == 9 || (nr >= 32 && nr <= 47))
		return m68k_getpc ();
	return regs.instruction_pc;
}

void REGPARAM2 _Exception (int nr, struct regstruct &regs)
{
  uae_u32 currpc, newpc;
  int sv = regs.s;

  if (currprefs.cachesize)
	  regs.instruction_pc = m68k_getpc ();

  if (nr >= 24 && nr < 24 + 8 && currprefs.cpu_model <= 68010)
	  nr = get_byte (0x00fffff1 | (nr << 1));

  MakeSR (regs);

  if (!regs.s) {
  	regs.usp = m68k_areg(regs, 7);
  	if (currprefs.cpu_model >= 68020)
	    m68k_areg(regs, 7) = regs.m ? regs.msp : regs.isp;
  	else
	    m68k_areg(regs, 7) = regs.isp;
  	regs.s = 1;
  }
  
  add_approximate_exception_cycles(nr);
  if (currprefs.cpu_model > 68000) {
  	currpc = exception_pc (nr);
  	if (nr == 2 || nr == 3) {
	    int i;
	    if (currprefs.cpu_model >= 68040) {
    		if (nr == 2) {

  				// 68040 bus error (not really, some garbage?)
	  	    for (i = 0 ; i < 18 ; i++) {
	      		m68k_areg(regs, 7) -= 2;
	      		x_put_word (m68k_areg(regs, 7), 0);
    	    }
		      m68k_areg(regs, 7) -= 4;
		      x_put_long (m68k_areg(regs, 7), last_fault_for_exception_3);
		      m68k_areg(regs, 7) -= 2;
		      x_put_word (m68k_areg(regs, 7), 0);
		      m68k_areg(regs, 7) -= 2;
		      x_put_word (m68k_areg(regs, 7), 0);
		      m68k_areg(regs, 7) -= 2;
		      x_put_word (m68k_areg(regs, 7), 0);
		      m68k_areg(regs, 7) -= 2;
		      x_put_word (m68k_areg(regs, 7), 0x0140 | (sv ? 6 : 2)); /* SSW */
		      m68k_areg(regs, 7) -= 4;
		      x_put_long (m68k_areg(regs, 7), last_addr_for_exception_3);
		      m68k_areg(regs, 7) -= 2;
		      x_put_word (m68k_areg(regs, 7), 0x7000 + nr * 4);
				  m68k_areg (regs, 7) -= 4;
				  x_put_long (m68k_areg (regs, 7), regs.instruction_pc);
				  m68k_areg (regs, 7) -= 2;
				  x_put_word (m68k_areg (regs, 7), regs.sr);
				  goto kludge_me_do;

    		} else {
		      m68k_areg(regs, 7) -= 4;
		      x_put_long (m68k_areg(regs, 7), last_fault_for_exception_3);
		      m68k_areg(regs, 7) -= 2;
		      x_put_word (m68k_areg(regs, 7), 0x2000 + nr * 4);
    		}
	    } else {
				// 68020 address error
    		uae_u16 ssw = (sv ? 4 : 0) | (last_instructionaccess_for_exception_3 ? 2 : 1);
    		ssw |= last_writeaccess_for_exception_3 ? 0 : 0x40;
    		ssw |= 0x20;
    		for (i = 0 ; i < 36; i++) {
  		    m68k_areg(regs, 7) -= 2;
		      x_put_word (m68k_areg(regs, 7), 0);
		    }
		    m68k_areg(regs, 7) -= 4;
		    x_put_long (m68k_areg(regs, 7), last_fault_for_exception_3);
		    m68k_areg(regs, 7) -= 2;
		    x_put_word (m68k_areg(regs, 7), 0);
		    m68k_areg(regs, 7) -= 2;
		    x_put_word (m68k_areg(regs, 7), 0);
		    m68k_areg(regs, 7) -= 2;
		    x_put_word (m68k_areg(regs, 7), 0);
		    m68k_areg(regs, 7) -= 2;
		    x_put_word (m68k_areg(regs, 7), ssw);
		    m68k_areg(regs, 7) -= 2;
		    x_put_word (m68k_areg(regs, 7), 0xb000 + nr * 4);
	    }
	    write_log (_T("Exception %d (%x) at %x -> %x!\n"), nr, regs.instruction_pc, currpc, x_get_long (regs.vbr + 4*nr));
	  } else if (nr ==5 || nr == 6 || nr == 7 || nr == 9) {
	    m68k_areg(regs, 7) -= 4;
	    x_put_long (m68k_areg(regs, 7), regs.instruction_pc);
	    m68k_areg(regs, 7) -= 2;
	    x_put_word (m68k_areg(regs, 7), 0x2000 + nr * 4);
	  } else if (regs.m && nr >= 24 && nr < 32) { /* M + Interrupt */
	    m68k_areg(regs, 7) -= 2;
	    x_put_word (m68k_areg(regs, 7), nr * 4);
	    m68k_areg(regs, 7) -= 4;
	    x_put_long (m68k_areg(regs, 7), currpc);
	    m68k_areg(regs, 7) -= 2;
	    x_put_word (m68k_areg(regs, 7), regs.sr);
	    regs.sr |= (1 << 13);
	    regs.msp = m68k_areg(regs, 7);
	    m68k_areg(regs, 7) = regs.isp;
	    m68k_areg(regs, 7) -= 2;
	    x_put_word (m68k_areg(regs, 7), 0x1000 + nr * 4);
	  } else {
	    m68k_areg(regs, 7) -= 2;
	    x_put_word (m68k_areg(regs, 7), nr * 4);
	  }
  } else {
	  currpc = m68k_getpc ();
    if (nr == 2 || nr == 3) {
	    // 68000 address error
	    uae_u16 mode = (sv ? 4 : 0) | (last_instructionaccess_for_exception_3 ? 2 : 1);
	    mode |= last_writeaccess_for_exception_3 ? 0 : 16;
	    m68k_areg(regs, 7) -= 14;
	    /* fixme: bit3=I/N */
	    x_put_word (m68k_areg(regs, 7) + 0, mode);
	    x_put_long (m68k_areg(regs, 7) + 2, last_fault_for_exception_3);
	    x_put_word (m68k_areg(regs, 7) + 6, last_op_for_exception_3);
	    x_put_word (m68k_areg(regs, 7) + 8, regs.sr);
	    x_put_long (m68k_areg(regs, 7) + 10, last_addr_for_exception_3);
	    write_log (_T("Exception %d (%x) at %x -> %x!\n"), nr, last_fault_for_exception_3, currpc, x_get_long (regs.vbr + 4*nr));
	    goto kludge_me_do;
		}
  }
  m68k_areg(regs, 7) -= 4;
  x_put_long (m68k_areg(regs, 7), currpc);
  m68k_areg(regs, 7) -= 2;
  x_put_word (m68k_areg(regs, 7), regs.sr);
kludge_me_do:
  newpc = x_get_long (regs.vbr + 4 * nr);
  if (newpc & 1) {
  	if (nr == 2 || nr == 3)
			uae_reset (1, 0); /* there is nothing else we can do.. */
  	else
	    exception3 (regs.ir, newpc);
  	return;
  }
  m68k_setpc (newpc);
#ifdef JIT
  set_special(SPCFLAG_END_COMPILE);
#endif
  fill_prefetch ();
  exception_trace (nr);
}

STATIC_INLINE void do_interrupt(int nr, struct regstruct &regs)
{
  regs.stopped = 0;
  unset_special (SPCFLAG_STOP);

  Exception (nr + 24);

  regs.intmask = nr;
  doint();
}

int movec_illg (int regno)
{
  int regno2 = regno & 0x7ff;

  if (currprefs.cpu_model == 68010) {
  	if (regno2 < 2)
	    return 0;
  	return 1;
  } else if (currprefs.cpu_model == 68020) {
		if (regno == 3)
			return 1; /* 68040/060 only */
	  /* 4 is >=68040, but 0x804 is in 68020 */
	  if (regno2 < 4 || regno == 0x804)
	    return 0;
	  return 1;
  } else if (currprefs.cpu_model == 68030) {
		if (regno2 <= 2)
			return 0;
  	if (regno == 0x803 || regno == 0x804)
	    return 0;
  	return 1;
  } else if (currprefs.cpu_model == 68040) {
		if (regno == 0x802)
			return 1; /* 68020 only */
  	if (regno2 < 8) return 0;
  	return 1;
  }
  return 1;
}

int m68k_move2c (int regno, uae_u32 *regp)
{
  if (movec_illg (regno)) {
		op_illg (0x4E7B);
		return 0;
  } else {
  	switch (regno) {
  	case 0: regs.sfc = *regp & 7; break;
  	case 1: regs.dfc = *regp & 7; break;
  	case 2:
    	{
	      uae_u32 cacr_mask = 0;
	      if (currprefs.cpu_model == 68020)
      		cacr_mask = 0x0000000f;
	      else if (currprefs.cpu_model == 68030)
      		cacr_mask = 0x00003f1f;
	      else if (currprefs.cpu_model == 68040)
      		cacr_mask = 0x80008000;
	      regs.cacr = *regp & cacr_mask;
	      set_cpu_caches();
    	}
	    break;
	  /* 68040/060 only */
	  case 3: 
      regs.tcr = *regp & 0xc000;
	    break;

  	/* no differences between 68040 and 68060 */
  	case 4: regs.itt0 = *regp & 0xffffe364; break;
	  case 5: regs.itt1 = *regp & 0xffffe364; break;
	  case 6: regs.dtt0 = *regp & 0xffffe364; break;
	  case 7: regs.dtt1 = *regp & 0xffffe364; break;
	  /* 68060 only */
	  case 8: regs.buscr = *regp & 0xf0000000; break;
	  case 0x800: regs.usp = *regp; break;
	  case 0x801: regs.vbr = *regp; break;
	  case 0x802: regs.caar = *regp & 0xfc; break;
	  case 0x803: regs.msp = *regp; if (regs.m == 1) m68k_areg(regs, 7) = regs.msp; break;
	  case 0x804: regs.isp = *regp; if (regs.m == 0) m68k_areg(regs, 7) = regs.isp; break;
	  /* 68040 only */
	  case 0x805: regs.mmusr = *regp; break;
	  /* 68040/060 */
    case 0x806: regs.urp = *regp & 0xfffffe00; break;
	  case 0x807: regs.srp = *regp & 0xfffffe00; break;
    /* 68060 only */
	  case 0x808:
	    {
  	    uae_u32 opcr = regs.pcr;
  	    regs.pcr &= ~(0x40 | 2 | 1);
  	    regs.pcr |= (*regp) & (0x40 | 2 | 1);
  	    if (((opcr ^ regs.pcr) & 2) == 2) {
      		write_log(_T("68060 FPU state: %s\n"), regs.pcr & 2 ? _T("disabled") : _T("enabled"));
      		/* flush possible already translated FPU instructions */
      		flush_icache (0, 3);
  	    }
    	}
    	break;
	  default:
			op_illg (0x4E7B);
			return 0;
	  }
  }
	return 1;
}

int m68k_movec2 (int regno, uae_u32 *regp)
{
  if (movec_illg (regno)) {
		op_illg (0x4E7A);
		return 0;
  } else {
  	switch (regno) {
  	case 0: *regp = regs.sfc; break;
  	case 1: *regp = regs.dfc; break;
  	case 2: 
    	{
  	    uae_u32 v = regs.cacr;
  	    uae_u32 cacr_mask = 0;
  	    if (currprefs.cpu_model == 68020)
      		cacr_mask = 0x00000003;
  	    else if (currprefs.cpu_model == 68030)
      		cacr_mask = 0x00003313;
  	    else if (currprefs.cpu_model == 68040)
      		cacr_mask = 0x80008000;
  	    *regp = v & cacr_mask;
    	}
    	break;
  	case 3: *regp = regs.tcr; break;
  	case 4: *regp = regs.itt0; break;
  	case 5: *regp = regs.itt1; break;
  	case 6: *regp = regs.dtt0; break;
  	case 7: *regp = regs.dtt1; break;
  	case 8: *regp = regs.buscr; break;

  	case 0x800: *regp = regs.usp; break;
  	case 0x801: *regp = regs.vbr; break;
  	case 0x802: *regp = regs.caar; break;
  	case 0x803: *regp = regs.m == 1 ? m68k_areg(regs, 7) : regs.msp; break;
  	case 0x804: *regp = regs.m == 0 ? m68k_areg(regs, 7) : regs.isp; break;
  	case 0x805: *regp = regs.mmusr; break;
  	case 0x806: *regp = regs.urp; break;
  	case 0x807: *regp = regs.srp; break;
  	case 0x808: *regp = regs.pcr; break;

  	default:
			op_illg (0x4E7A);
			return 0;
  	}
  }
	return 1;
}

STATIC_INLINE int div_unsigned(uae_u32 src_hi, uae_u32 src_lo, uae_u32 div, uae_u32 *quot, uae_u32 *rem)
{
	uae_u32 q = 0, cbit = 0;
	int i;

	if (div <= src_hi) {
	    return 1;
	}
	for (i = 0 ; i < 32 ; i++) {
		cbit = src_hi & 0x80000000ul;
		src_hi <<= 1;
		if (src_lo & 0x80000000ul) src_hi++;
		src_lo <<= 1;
		q = q << 1;
		if (cbit || div <= src_hi) {
			q |= 1;
			src_hi -= div;
		}
	}
	*quot = q;
	*rem = src_hi;
	return 0;
}

void m68k_divl (uae_u32 opcode, uae_u32 src, uae_u16 extra)
{
  // Done in caller
  //if (src == 0) {
  //  Exception (5);
  //  return;
  //}
#if defined(uae_s64)
  if (extra & 0x800) {
  	/* signed variant */
  	uae_s64 a = (uae_s64)(uae_s32)m68k_dreg(regs, (extra >> 12) & 7);
  	uae_s64 quot, rem;

  	if (extra & 0x400) {
	    a &= 0xffffffffu;
	    a |= (uae_s64)m68k_dreg(regs, extra & 7) << 32;
  	}
  	rem = a % (uae_s64)(uae_s32)src;
  	quot = a / (uae_s64)(uae_s32)src;
  	if ((quot & UVAL64(0xffffffff80000000)) != 0
	    && (quot & UVAL64(0xffffffff80000000)) != UVAL64(0xffffffff80000000))
  	{
	    SET_VFLG (1);
	    SET_NFLG (1);
	    SET_CFLG (0);
  	} else {
	    if (((uae_s32)rem < 0) != ((uae_s64)a < 0)) rem = -rem;
	    SET_VFLG (0);
	    SET_CFLG (0);
	    SET_ZFLG (((uae_s32)quot) == 0);
	    SET_NFLG (((uae_s32)quot) < 0);
	    m68k_dreg(regs, extra & 7) = (uae_u32)rem;
	    m68k_dreg(regs, (extra >> 12) & 7) = (uae_u32)quot;
  	}
  } else {
  	/* unsigned */
  	uae_u64 a = (uae_u64)(uae_u32)m68k_dreg(regs, (extra >> 12) & 7);
  	uae_u64 quot, rem;

  	if (extra & 0x400) {
	    a &= 0xffffffffu;
	    a |= (uae_u64)m68k_dreg(regs, extra & 7) << 32;
  	}
  	rem = a % (uae_u64)src;
  	quot = a / (uae_u64)src;
  	if (quot > 0xffffffffu) {
	    SET_VFLG (1);
	    SET_NFLG (1);
	    SET_CFLG (0);
  	} else {
	    SET_VFLG (0);
	    SET_CFLG (0);
	    SET_ZFLG (((uae_s32)quot) == 0);
	    SET_NFLG (((uae_s32)quot) < 0);
	    m68k_dreg(regs, extra & 7) = (uae_u32)rem;
	    m68k_dreg(regs, (extra >> 12) & 7) = (uae_u32)quot;
  	}
  }
#else
  if (extra & 0x800) {
  	/* signed variant */
  	uae_s32 lo = (uae_s32)m68k_dreg(regs, (extra >> 12) & 7);
  	uae_s32 hi = lo < 0 ? -1 : 0;
  	uae_s32 save_high;
  	uae_u32 quot, rem;
  	uae_u32 sign;

  	if (extra & 0x400) {
	    hi = (uae_s32)m68k_dreg(regs, extra & 7);
  	}
  	save_high = hi;
  	sign = (hi ^ src);
  	if (hi < 0) {
	    hi = ~hi;
	    lo = -lo;
	    if (lo == 0) hi++;
  	}
  	if ((uae_s32)src < 0) src = -src;
  	if (div_unsigned(hi, lo, src, &quot, &rem) ||
	    (sign & 0x80000000) ? quot > 0x80000000 : quot > 0x7fffffff) {
	    SET_VFLG (1);
	    SET_NFLG (1);
	    SET_CFLG (0);
  	} else {
	    if (sign & 0x80000000) quot = -quot;
	    if (((uae_s32)rem < 0) != (save_high < 0)) rem = -rem;
	    SET_VFLG (0);
	    SET_CFLG (0);
	    SET_ZFLG (((uae_s32)quot) == 0);
	    SET_NFLG (((uae_s32)quot) < 0);
	    m68k_dreg(regs, extra & 7) = rem;
	    m68k_dreg(regs, (extra >> 12) & 7) = quot;
  	}
  } else {
  	/* unsigned */
  	uae_u32 lo = (uae_u32)m68k_dreg(regs, (extra >> 12) & 7);
  	uae_u32 hi = 0;
  	uae_u32 quot, rem;

  	if (extra & 0x400) {
	    hi = (uae_u32)m68k_dreg(regs, extra & 7);
  	}
  	if (div_unsigned(hi, lo, src, &quot, &rem)) {
	    SET_VFLG (1);
	    SET_NFLG (1);
	    SET_CFLG (0);
  	} else {
	    SET_VFLG (0);
	    SET_CFLG (0);
	    SET_ZFLG (((uae_s32)quot) == 0);
	    SET_NFLG (((uae_s32)quot) < 0);
	    m68k_dreg(regs, extra & 7) = rem;
	    m68k_dreg(regs, (extra >> 12) & 7) = quot;
  	}
  }
#endif
}

STATIC_INLINE void mul_unsigned(uae_u32 src1, uae_u32 src2, uae_u32 *dst_hi, uae_u32 *dst_lo)
{
	uae_u32 r0 = (src1 & 0xffff) * (src2 & 0xffff);
	uae_u32 r1 = ((src1 >> 16) & 0xffff) * (src2 & 0xffff);
	uae_u32 r2 = (src1 & 0xffff) * ((src2 >> 16) & 0xffff);
	uae_u32 r3 = ((src1 >> 16) & 0xffff) * ((src2 >> 16) & 0xffff);
	uae_u32 lo;

	lo = r0 + ((r1 << 16) & 0xffff0000ul);
	if (lo < r0) r3++;
	r0 = lo;
	lo = r0 + ((r2 << 16) & 0xffff0000ul);
	if (lo < r0) r3++;
	r3 += ((r1 >> 16) & 0xffff) + ((r2 >> 16) & 0xffff);
	*dst_lo = lo;
	*dst_hi = r3;
}

void m68k_mull (uae_u32 opcode, uae_u32 src, uae_u16 extra)
{
#if defined(uae_s64)
  if (extra & 0x800) {
  	/* signed variant */
  	uae_s64 a = (uae_s64)(uae_s32)m68k_dreg(regs, (extra >> 12) & 7);

  	a *= (uae_s64)(uae_s32)src;
  	SET_VFLG (0);
  	SET_CFLG (0);
  	SET_ZFLG (a == 0);
  	SET_NFLG (a < 0);
  	if (extra & 0x400)
	    m68k_dreg(regs, extra & 7) = (uae_u32)(a >> 32);
  	else if ((a & UVAL64(0xffffffff80000000)) != 0
		  && (a & UVAL64(0xffffffff80000000)) != UVAL64(0xffffffff80000000))
	  {
	    SET_VFLG (1);
	  }
  	m68k_dreg(regs, (extra >> 12) & 7) = (uae_u32)a;
  } else {
	  /* unsigned */
	  uae_u64 a = (uae_u64)(uae_u32)m68k_dreg(regs, (extra >> 12) & 7);

  	a *= (uae_u64)src;
	  SET_VFLG (0);
	  SET_CFLG (0);
	  SET_ZFLG (a == 0);
	  SET_NFLG (((uae_s64)a) < 0);
	  if (extra & 0x400)
	    m68k_dreg(regs, extra & 7) = (uae_u32)(a >> 32);
	  else if ((a & UVAL64(0xffffffff00000000)) != 0) {
	    SET_VFLG (1);
	  }
	  m68k_dreg(regs, (extra >> 12) & 7) = (uae_u32)a;
  }
#else
  if (extra & 0x800) {
	  /* signed variant */
	  uae_s32 src1,src2;
	  uae_u32 dst_lo,dst_hi;
	  uae_u32 sign;

  	src1 = (uae_s32)src;
	  src2 = (uae_s32)m68k_dreg(regs, (extra >> 12) & 7);
	  sign = (src1 ^ src2);
	  if (src1 < 0) src1 = -src1;
	  if (src2 < 0) src2 = -src2;
	  mul_unsigned((uae_u32)src1,(uae_u32)src2,&dst_hi,&dst_lo);
	  if (sign & 0x80000000) {
	  	dst_hi = ~dst_hi;
	  	dst_lo = -dst_lo;
	  	if (dst_lo == 0) dst_hi++;
	  }
  	SET_VFLG (0);
	  SET_CFLG (0);
	  SET_ZFLG (dst_hi == 0 && dst_lo == 0);
	  SET_NFLG (((uae_s32)dst_hi) < 0);
	  if (extra & 0x400)
	    m68k_dreg(regs, extra & 7) = dst_hi;
	  else if ((dst_hi != 0 || (dst_lo & 0x80000000) != 0)
		  && ((dst_hi & 0xffffffff) != 0xffffffff
		  || (dst_lo & 0x80000000) != 0x80000000))
  	{
	    SET_VFLG (1);
  	}
  	m68k_dreg(regs, (extra >> 12) & 7) = dst_lo;
  } else {
  	/* unsigned */
  	uae_u32 dst_lo,dst_hi;

  	mul_unsigned(src,(uae_u32)m68k_dreg(regs, (extra >> 12) & 7),&dst_hi,&dst_lo);

  	SET_VFLG (0);
  	SET_CFLG (0);
  	SET_ZFLG (dst_hi == 0 && dst_lo == 0);
  	SET_NFLG (((uae_s32)dst_hi) < 0);
  	if (extra & 0x400)
	    m68k_dreg(regs, extra & 7) = dst_hi;
  	else if (dst_hi != 0) {
	    SET_VFLG (1);
  	}
  	m68k_dreg(regs, (extra >> 12) & 7) = dst_lo;
  }
#endif
}

void m68k_reset (int hardreset)
{
  regs.pissoff = 0;
  cpu_cycles = 0;
  
  regs.spcflags = 0;
#ifdef SAVESTATE
  if (isrestore ()) {
  	m68k_setpc (regs.pc);
  	SET_XFLG ((regs.sr >> 4) & 1);
  	SET_NFLG ((regs.sr >> 3) & 1);
  	SET_ZFLG ((regs.sr >> 2) & 1);
  	SET_VFLG ((regs.sr >> 1) & 1);
  	SET_CFLG (regs.sr & 1);
  	regs.t1 = (regs.sr >> 15) & 1;
  	regs.t0 = (regs.sr >> 14) & 1;
  	regs.s = (regs.sr >> 13) & 1;
  	regs.m  = (regs.sr >> 12) & 1;
  	regs.intmask = (regs.sr >> 8) & 7;
  	/* set stack pointer */
  	if (regs.s)
	    m68k_areg(regs, 7) = regs.isp;
  	else
	    m68k_areg(regs, 7) = regs.usp;
  	return;
  }
#endif
  m68k_areg (regs, 7) = get_long (0);
  m68k_setpc (get_long (4));
  regs.s = 1;
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
  regs.tcr = regs.mmusr = regs.urp = regs.srp = regs.buscr = 0;
  if (currprefs.cpu_model == 68020) {
	  regs.cacr |= 8;
	  set_cpu_caches ();
  }

  /* only (E)nable bit is zeroed when CPU is reset, A3000 SuperKickstart expects this */
  tc_030 &= ~0x80000000;
  tt0_030 &= ~0x80000000;
  tt1_030 &= ~0x80000000;
  if (hardreset) {
  	srp_030 = crp_030 = 0;
  	tt0_030 = tt1_030 = tc_030 = 0;
  }
  mmusr_030 = 0;

  /* 68060 FPU is not compatible with 68040,
   * 68060 accelerators' boot ROM disables the FPU
   */
  regs.pcr = 0;
  fill_prefetch_quick ();
}

void REGPARAM2 _op_unimpl (struct regstruct &regs)
{
	Exception (61);
}

uae_u32 REGPARAM2 _op_illg (uae_u32 opcode, struct regstruct &regs)
{
  uaecptr pc = m68k_getpc ();
  int inrom = in_rom(pc);
  int inrt = in_rtarea(pc);

  if (cloanto_rom && (opcode & 0xF100) == 0x7100) {
  	m68k_dreg (regs, (opcode >> 9) & 7) = (uae_s8)(opcode & 0xFF);
  	m68k_incpc (2);
  	fill_prefetch ();
		return 4;
  }

  if (opcode == 0x4E7B && inrom && get_long (0x10) == 0) {
  	notify_user (NUMSG_KS68020);
  	uae_restart (-1, NULL);
  }

#ifdef AUTOCONFIG
  if (opcode == 0xFF0D && inrt) {
    /* User-mode STOP replacement */
    m68k_setstopped ();
		return 4;
  }

  if ((opcode & 0xF000) == 0xA000 && inrt) {
  	/* Calltrap. */
  	m68k_incpc(2);
  	m68k_handle_trap (opcode & 0xFFF);
  	fill_prefetch ();
		return 4;
  }
#endif

  if ((opcode & 0xF000) == 0xF000) {
	  Exception (0xB);
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

static void mmu_op30_pmove(uaecptr pc, uae_u32 opcode, uae_u16 next, uaecptr extra)
{
	int mode = (opcode >> 3) & 7;
  int preg = (next >> 10) & 31;
  int rw = (next >> 9) & 1;
  int fd = (next >> 8) & 1;
  TCHAR *reg = NULL;
  int siz;

	// Dn, An, (An)+, -(An), abs and indirect
	if (mode == 0 || mode == 1 || mode == 3 || mode == 4 || mode >= 6) {
		op_illg (opcode);
		return;
	}

  switch (preg)
  {
  case 0x10: // TC
  	reg = _T("TC");
  	siz = 4;
	  if (rw)
	    x_put_long(extra, tc_030);
  	else
	    tc_030 = x_get_long(extra);
    break;
  case 0x12: // SRP
  	reg = _T("SRP");
  	siz = 8;
  	if (rw) {
	    x_put_long(extra, srp_030 >> 32);
	    x_put_long(extra + 4, srp_030);
  	} else {
	    srp_030 = (uae_u64)x_get_long(extra) << 32;
	    srp_030 |= x_get_long(extra + 4);
  	}
    break;
  case 0x13: // CRP
  	reg = _T("CRP");
  	siz = 8;
  	if (rw) {
	    x_put_long(extra, crp_030 >> 32);
	    x_put_long(extra + 4, crp_030);
  	} else {
	    crp_030 = (uae_u64)x_get_long(extra) << 32;
	    crp_030 |= x_get_long(extra + 4);
  	}
    break;
  case 0x18: // MMUSR
  	reg = _T("MMUSR");
  	siz = 2;
  	if (rw)
	    x_put_word(extra, mmusr_030);
  	else
	    mmusr_030 = x_get_word(extra);
    break;
  case 0x02: // TT0
  	reg = _T("TT0");
  	siz = 4;
  	if (rw)
	    x_put_long(extra, tt0_030);
	  else
	    tt0_030 = x_get_long(extra);
    break;
  case 0x03: // TT1
  	reg = _T("TT1");
  	siz = 4;
  	if (rw)
	    x_put_long(extra, tt1_030);
  	else
	    tt1_030 = x_get_long(extra);
    break;
  }

  if (!reg) {
  	op_illg(opcode);
    return;
  }
}

static void mmu_op30_ptest(uaecptr pc, uae_u32 opcode, uae_u16 next, uaecptr extra)
{
  mmusr_030 = 0;
}

static void mmu_op30_pflush(uaecptr pc, uae_u32 opcode, uae_u16 next, uaecptr extra)
{
	int mode = (opcode >> 3) & 7;
	int flushmode = (next >> 10) & 7;

	switch (flushmode)
	{
	  case 6:
		  // Dn, An, (An)+, -(An), abs and indirect
		  if (mode == 0 || mode == 1 || mode == 3 || mode == 4 || mode >= 6) {
			  op_illg (opcode);
			  return;
		  }
		  break;
	  case 4:
		  break;
	  case 1:
		  break;
	  default:
		  op_illg (opcode);
		  return;
	}
}

void mmu_op30 (uaecptr pc, uae_u32 opcode, struct regstruct &regs, uae_u16 extra, uaecptr extraa)
{
	int type = extra >> 13;

	switch (type)
	{
	  case 0:
	  case 2:
	  case 3:
		  mmu_op30_pmove (pc, opcode, extra, extraa);
	    break;
	  case 1:
		  mmu_op30_pflush (pc, opcode, extra, extraa);
	    break;
	  case 4:
		  mmu_op30_ptest (pc, opcode, extra, extraa);
	    break;
	  default:
		  op_illg (opcode);
	    break;
	}
}

void mmu_op(uae_u32 opcode, struct regstruct &regs, uae_u32 extra)
{
  if ((opcode & 0xFE0) == 0x0500) {
  	/* PFLUSH */
  	regs.mmusr = 0;
  	return;
  } else if ((opcode & 0x0FD8) == 0x548) {
  	/* PTEST */
    return;
  } else if ((opcode & 0x0FB8) == 0x588) {
  	/* PLPA */
  }
  m68k_setpc (m68k_getpc () - 2);
	op_illg (opcode);
}

#endif

static void do_trace (void)
{
  if (regs.t0 && currprefs.cpu_model >= 68020) {
  	uae_u16 opcode;
  	/* should also include TRAP, CHK, SR modification FPcc */
  	/* probably never used so why bother */
  	/* We can afford this to be inefficient... */
  	m68k_setpc (m68k_getpc ());
  	fill_prefetch ();
  	opcode = x_get_word (regs.pc);
  	if (opcode == 0x4e73 		/* RTE */
	    || opcode == 0x4e74 		/* RTD */
	    || opcode == 0x4e75 		/* RTS */
	    || opcode == 0x4e77 		/* RTR */
	    || opcode == 0x4e76 		/* TRAPV */
	    || (opcode & 0xffc0) == 0x4e80 	/* JSR */
	    || (opcode & 0xffc0) == 0x4ec0 	/* JMP */
	    || (opcode & 0xff00) == 0x6100  /* BSR */
	    || ((opcode & 0xf000) == 0x6000	/* Bcc */
  		&& cctrue (regs.ccrflags, (opcode >> 8) & 0xf))
	    || ((opcode & 0xf0f0) == 0x5050 /* DBcc */
  		&& !cctrue (regs.ccrflags, (opcode >> 8) & 0xf)
  		&& (uae_s16)m68k_dreg (regs, opcode & 7) != 0))
  	{
	    unset_special (SPCFLAG_TRACE);
	    set_special (SPCFLAG_DOTRACE);
  	}
  } else if (regs.t1) {
  	unset_special (SPCFLAG_TRACE);
  	set_special (SPCFLAG_DOTRACE);
  }
}

void doint (void)
{
  if (currprefs.cpu_compatible)
  	set_special (SPCFLAG_INT);
  else
  	set_special (SPCFLAG_DOINT);
}

STATIC_INLINE int do_specialties (int cycles, struct regstruct &regs)
{
	regs.instruction_pc = m68k_getpc ();

  if (regs.spcflags & SPCFLAG_COPPER)
    do_copper ();

#ifdef JIT
  unset_special(SPCFLAG_END_COMPILE);   /* has done its job */
#endif

  while ((regs.spcflags & SPCFLAG_BLTNASTY) && dmaen (DMA_BLITTER) && cycles > 0) {
	  int c = blitnasty();
	  if (c > 0) {
	    cycles -= c * CYCLE_UNIT * 2;
	    if (cycles < CYCLE_UNIT)
	      cycles = 0;
	  } else {
	    c = 4;
    }
	  do_cycles (c * CYCLE_UNIT);
	  if (regs.spcflags & SPCFLAG_COPPER)
	    do_copper ();
  }

  if (regs.spcflags & SPCFLAG_DOTRACE)
   	Exception (9);

  while (regs.spcflags & SPCFLAG_STOP) {

    if (uae_int_requested) {
		  INTREQ_f (0x8008);
		  set_special (SPCFLAG_INT);
	  }
 		{
 			extern void bsdsock_fake_int_handler (void);
 			extern int volatile bsd_int_requested;
 			if (bsd_int_requested)
 				bsdsock_fake_int_handler ();
 		}

    do_cycles (4 * CYCLE_UNIT);
    if (regs.spcflags & SPCFLAG_COPPER)
      do_copper ();

	  if (regs.spcflags & (SPCFLAG_INT | SPCFLAG_DOINT)) {
	    int intr = intlev ();
  	  unset_special (SPCFLAG_INT | SPCFLAG_DOINT);
	     if (intr > 0 && intr > regs.intmask)
		     do_interrupt (intr, regs);
    }

    if ((regs.spcflags & (SPCFLAG_BRK | SPCFLAG_MODE_CHANGE))) {
      unset_special (SPCFLAG_BRK | SPCFLAG_MODE_CHANGE);
		  // SPCFLAG_BRK breaks STOP condition, need to prefetch
		  m68k_resumestopped ();
      return 1;
    }
	}

  if (regs.spcflags & SPCFLAG_TRACE)
    do_trace ();

  if (regs.spcflags & SPCFLAG_INT) {
	  int intr = intlev ();
    unset_special (SPCFLAG_INT | SPCFLAG_DOINT);
	  if (intr > 0 && (intr > regs.intmask || intr == 7))
      do_interrupt (intr, regs);
  }

  if (regs.spcflags & SPCFLAG_DOINT) {
    unset_special (SPCFLAG_DOINT);
    set_special (SPCFLAG_INT);
  }

  if ((regs.spcflags & (SPCFLAG_BRK | SPCFLAG_MODE_CHANGE))) {
    unset_special (SPCFLAG_BRK | SPCFLAG_MODE_CHANGE);
    return 1;
  }
  
  return 0;
}

/* It's really sad to have two almost identical functions for this, but we
   do it all for performance... :(
   This version emulates 68000's prefetch "cache" */
static void m68k_run_1 (void)
{
  struct regstruct &r = regs;

  for (;;) {
  	uae_u16 opcode = r.ir;

#if defined (CPU_arm) & defined(USE_ARMNEON)
    // Well not really since pli is ArmV7...
    /* Load ARM code for next opcode into L2 cache during execute of do_cycles() */
    __asm__ volatile ("pli [%[radr]]\n\t" \
      : : [radr] "r" (cpufunctbl[opcode]) : );
#endif
		count_instr (opcode);

  	do_cycles (cpu_cycles);
  	cpu_cycles = (*cpufunctbl[opcode])(opcode, r);
  	cpu_cycles = adjust_cycles(cpu_cycles);
  	if (r.spcflags) {
      if (do_specialties (cpu_cycles, r)) {
    		return;
      }
  	}
  	if (!currprefs.cpu_compatible)
      return;
  }
}

#ifdef JIT  /* Completely different run_2 replacement */

void do_nothing(void)
{
  /* What did you expect this to do? */
  do_cycles(0);
  /* I bet you didn't expect *that* ;-) */
}

void exec_nostats(void)
{
  struct regstruct &r = regs;

  for (;;)
  {
  	uae_u16 opcode = get_iword2 (r, 0);
  	cpu_cycles = (*cpufunctbl[opcode])(opcode, r);
  	cpu_cycles = adjust_cycles(cpu_cycles);
  	do_cycles (cpu_cycles);

  	if (end_block(opcode) || r.spcflags || uae_int_requested)
	    return; /* We will deal with the spcflags in the caller */
  }
}

void execute_normal(void)
{
  struct regstruct &r = regs;
  int blocklen;
  cpu_history pc_hist[MAXRUN];
  int total_cycles;

  if (check_for_cache_miss())
  	return;

  total_cycles = 0;
  blocklen = 0;
  start_pc_p = r.pc_oldp;
  start_pc = r.pc;
  for (;;) { 
    /* Take note: This is the do-it-normal loop */
  	uae_u16 opcode;

    opcode = get_iword2 (r, 0);

  	special_mem = DISTRUST_CONSISTENT_MEM;
  	pc_hist[blocklen].location = (uae_u16*)r.pc_p;

  	cpu_cycles = (*cpufunctbl[opcode])(opcode, r);
  	cpu_cycles = adjust_cycles(cpu_cycles);
  	do_cycles (cpu_cycles);
  	total_cycles += cpu_cycles;
  	pc_hist[blocklen].specmem = special_mem;
  	blocklen++;
  	if (end_block(opcode) || blocklen >= MAXRUN || r.spcflags || uae_int_requested) {
	    compile_block(pc_hist,blocklen,total_cycles);
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
  	if (uae_int_requested) {
	    INTREQ_f (0x8008);
	    set_special (SPCFLAG_INT);
  	}
  	if (regs.spcflags) {
	    if (do_specialties (0, regs)) {
    		return;
	    }
  	}
  }
}
#endif /* JIT */

/* Same thing, but don't use prefetch to get opcode.  */
static void m68k_run_2 (void)
{
  struct regstruct &r = regs;

  for (;;) {
		r.instruction_pc = m68k_getpc ();
	  uae_u16 opcode = get_iword2 (r, 0);
    
#if defined (CPU_arm) & defined(USE_ARMNEON)
    // Well not really since pli is ArmV7...
    /* Load ARM code for next opcode into L2 cache during execute of do_cycles() */
    __asm__ volatile ("pli [%[radr]]\n\t" \
       : : [radr] "r" (cpufunctbl[opcode]) : );
#endif
	  count_instr (opcode);
	  do_cycles (cpu_cycles);
	  cpu_cycles = (*cpufunctbl[opcode])(opcode, r);
	  cpu_cycles = adjust_cycles(cpu_cycles);
	  if (r.spcflags) {
	    if (do_specialties (cpu_cycles, r))
		    return;
	  }
  }
}

int in_m68k_go = 0;

static void exception2_handle (uaecptr addr, uaecptr fault)
{
  last_addr_for_exception_3 = addr;
  last_fault_for_exception_3 = fault;
  last_writeaccess_for_exception_3 = 0;
  last_instructionaccess_for_exception_3 = 0;
  Exception (2);
}

void m68k_go (int may_quit)
{
  int hardboot = 1;
	int startup = 1;

  if (in_m68k_go || !may_quit) {
		write_log (_T("Bug! m68k_go is not reentrant.\n"));
	  abort ();
  }

  reset_frame_rate_hack ();
  update_68k_cycles ();

  in_m68k_go++;
  for (;;) {
  	void (*run_func)(void);

  	if (quit_program > 0) {
	    int hardreset = (quit_program == UAE_RESET_HARD ? 1 : 0) | hardboot;
			bool kbreset = quit_program == UAE_RESET_KEYBOARD;
			if (quit_program == UAE_QUIT)
    		break;
	    if(quit_program == UAE_RESET_HARD)
	      reinit_amiga();
			int restored = 0;

			hsync_counter = 0;
	    quit_program = 0;
	    hardboot = 0;
#ifdef SAVESTATE
			if (savestate_state == STATE_DORESTORE)
				savestate_state = STATE_RESTORE;
	    if (savestate_state == STATE_RESTORE)
		    restore_state (savestate_fname);
#endif
			set_cycles (0);
      check_prefs_changed_adr24();
	    custom_reset (hardreset != 0, kbreset);
	    m68k_reset (hardreset);
	    if (hardreset) {
				memory_clear ();
				write_log (_T("hardreset, memory cleared\n"));
	    }
#ifdef SAVESTATE
	    /* We may have been restoring state, but we're done now.  */
	    if (isrestore ()) {
		    savestate_restore_finish ();
				startup = 1;
				restored = 1;
	    }
#endif
	    if (currprefs.produce_sound == 0)
		    eventtab[ev_audio].active = 0;
	    m68k_setpc (regs.pc);
			check_prefs_changed_audio ();

			if (!restored || hsync_counter == 0)
				savestate_check ();
	  }

	  if (regs.panic) {
	    regs.panic = 0;
	    /* program jumped to non-existing memory and cpu was >= 68020 */
	    get_real_address (regs.isp); /* stack in no one's land? -> reboot */
	    if (regs.isp & 1)
		    regs.panic = 1;
	    if (!regs.panic)
		    exception2_handle (regs.panic_pc, regs.panic_addr);
	    if (regs.panic) {
		    /* system is very badly confused */
				write_log (_T("double bus error or corrupted stack, forcing reboot..\n"));
		    regs.panic = 0;
				uae_reset (1, 0);
	    }
  	}

	  if (startup) {
		  custom_prepare ();
			protect_roms (true);
		}
	  startup = 0;
    run_func = 
      currprefs.cpu_compatible && currprefs.cpu_model == 68000 ? m68k_run_1 :
#ifdef JIT
      currprefs.cpu_model >= 68020 && currprefs.cachesize ? m68k_run_jit :
#endif
      m68k_run_2;
	  run_func ();
  }
	protect_roms (false);
  in_m68k_go--;
}

#ifdef SAVESTATE

/* CPU save/restore code */

#define CPUTYPE_EC 1
#define CPUMODE_HALT 1

uae_u8 *restore_cpu (uae_u8 *src)
{
  int i, flags, model;
  uae_u32 l;

	currprefs.cpu_model = changed_prefs.cpu_model = model = restore_u32 ();
  flags = restore_u32();
  changed_prefs.address_space_24 = 0;
  if (flags & CPUTYPE_EC)
  	changed_prefs.address_space_24 = 1;
  currprefs.address_space_24 = changed_prefs.address_space_24;
  currprefs.cpu_compatible = changed_prefs.cpu_compatible;
  for (i = 0; i < 15; i++)
  	regs.regs[i] = restore_u32 ();
  regs.pc = restore_u32 ();
  regs.irc = restore_u16 ();
  regs.ir = restore_u16 ();
  regs.usp = restore_u32 ();
  regs.isp = restore_u32 ();
  regs.sr = restore_u16 ();
  l = restore_u32();
  if (l & CPUMODE_HALT) {
  	regs.stopped = 1;
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
  	crp_030 = restore_u64();
  	srp_030 = restore_u64();
  	tt0_030 =restore_u32();
  	tt1_030 = restore_u32();
  	tc_030 = restore_u32();
  	mmusr_030 = restore_u16();
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
  if (model >= 68060) {
  	regs.buscr = restore_u32();
  	regs.pcr = restore_u32();
  }
  if (flags & 0x80000000) {
  	int khz = restore_u32();
  	restore_u32();
  	if (khz > 0 && khz < 800000)
	    currprefs.m68k_speed = changed_prefs.m68k_speed = 0;
  }
  set_cpu_caches();

  write_log (_T("CPU: %d%s%03d, PC=%08X\n"),
  	model / 1000, flags & 1 ? _T("EC") : _T(""), model % 1000, regs.pc);

  return src;
}

void restore_cpu_finish(void)
{
  init_m68k ();
  m68k_setpc (regs.pc);
	doint ();
	fill_prefetch_quick ();
	set_cycles (0);
	events_schedule ();
	if (regs.stopped)
		set_special (SPCFLAG_STOP);
}

uae_u8 *restore_cpu_extra (uae_u8 *src)
{
	restore_u32 ();
	uae_u32 flags = restore_u32 ();

	currprefs.cpu_compatible = changed_prefs.cpu_compatible = (flags & 2) ? true : false;
	//currprefs.cachesize = changed_prefs.cachesize = (flags & 8) ? 8192 : 0;

	currprefs.m68k_speed = changed_prefs.m68k_speed = 0;
	if (flags & 4)
		currprefs.m68k_speed = changed_prefs.m68k_speed = -1;
	if (flags & 16)
		currprefs.m68k_speed = changed_prefs.m68k_speed = (flags >> 24) * CYCLE_UNIT;

	return src;
}

uae_u8 *save_cpu_extra (int *len, uae_u8 *dstptr)
{
	uae_u8 *dstbak, *dst;
	uae_u32 flags;

	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc (uae_u8, 1000);
	save_u32 (0); // version
	flags = 0;
	flags |= currprefs.cpu_compatible ? 2 : 0;
	flags |= currprefs.m68k_speed < 0 ? 4 : 0;
	flags |= currprefs.cachesize > 0 ? 8 : 0;
	flags |= currprefs.m68k_speed > 0 ? 16 : 0;
	if (currprefs.m68k_speed > 0)
		flags |= (currprefs.m68k_speed / CYCLE_UNIT) << 24;
	save_u32 (flags);
	*len = dst - dstbak;
	return dstbak;
}

uae_u8 *save_cpu (int *len, uae_u8 *dstptr)
{
  uae_u8 *dstbak,*dst;
  int model, i, khz;

  if (dstptr)
  	dstbak = dst = dstptr;
  else
  	dstbak = dst = xmalloc (uae_u8, 1000);
  model = currprefs.cpu_model;
  save_u32 (model);					/* MODEL */
  save_u32 (0x80000000 | 0x40000000 | (currprefs.address_space_24 ? 1 : 0));	/* FLAGS */
  for(i = 0;i < 15; i++) 
    save_u32 (regs.regs[i]);	/* D0-D7 A0-A6 */
  save_u32 (m68k_getpc ());			/* PC */
  save_u16 (regs.irc);				/* prefetch */
  save_u16 (regs.ir);					/* instruction prefetch */
  MakeSR (regs);
  save_u32 (!regs.s ? regs.regs[15] : regs.usp);	/* USP */
  save_u32 (regs.s ? regs.regs[15] : regs.isp);	/* ISP */
  save_u16 (regs.sr);					/* SR/CCR */
  save_u32 (regs.stopped ? CPUMODE_HALT : 0);		/* flags */
  if(model >= 68010) {
  	save_u32 (regs.dfc);				/* DFC */
  	save_u32 (regs.sfc);				/* SFC */
  	save_u32 (regs.vbr);				/* VBR */
  }
  if(model >= 68020) {
  	save_u32 (regs.caar);				/* CAAR */
  	save_u32 (regs.cacr);				/* CACR */
  	save_u32 (regs.msp);				/* MSP */
  }
  if(model >= 68030) {
  	save_u64 (crp_030);				/* CRP */
  	save_u64 (srp_030);				/* SRP */
  	save_u32 (tt0_030);				/* TT0/AC0 */
  	save_u32 (tt1_030);				/* TT1/AC1 */
  	save_u32 (tc_030);				/* TCR */
  	save_u16 (mmusr_030);				/* MMUSR/ACUSR */
  }
  if(model >= 68040) {
  	save_u32 (regs.itt0);				/* ITT0 */
  	save_u32 (regs.itt1);				/* ITT1 */
  	save_u32 (regs.dtt0);				/* DTT0 */
  	save_u32 (regs.dtt1);				/* DTT1 */
  	save_u32 (regs.tcr);				/* TCR */
  	save_u32 (regs.urp);				/* URP */
  	save_u32 (regs.srp);				/* SRP */
  }
  if(model >= 68060) {
  	save_u32 (regs.buscr);				/* BUSCR */
  	save_u32 (regs.pcr);				/* PCR */
  }
  khz = -1;
  if (currprefs.m68k_speed == 0) {
  	khz = currprefs.ntscmode ? 715909 : 709379;
  	if (currprefs.cpu_model >= 68020)
	    khz *= 2;
  }
  save_u32 (khz); // clock rate in KHz: -1 = fastest possible 
  save_u32 (0); // spare
  *len = dst - dstbak;
  return dstbak;
}

#endif /* SAVESTATE */

static void exception3f (uae_u32 opcode, uaecptr addr, int writeaccess, int instructionaccess, uae_u32 pc)
{
  if (currprefs.cpu_model >= 68040)
	  addr &= ~1;
  if (currprefs.cpu_model >= 68020) {
		last_addr_for_exception_3 = regs.instruction_pc;
	} else if (pc == 0xffffffff) {
	  last_addr_for_exception_3 = m68k_getpc () + 2;
  } else {
	  last_addr_for_exception_3 = pc;
  }
  last_fault_for_exception_3 = addr;
  last_op_for_exception_3 = opcode;
  last_writeaccess_for_exception_3 = writeaccess;
  last_instructionaccess_for_exception_3 = instructionaccess;
  Exception (3);
}

void exception3 (uae_u32 opcode, uaecptr addr)
{
  exception3f (opcode, addr, 0, 0, 0xffffffff);
}

void exception3i (uae_u32 opcode, uaecptr addr)
{
  exception3f (opcode, addr, 0, 1, 0xffffffff);
}
void exception3 (uae_u32 opcode, uaecptr addr, int w, int i, uaecptr pc)
{
	exception3f (opcode, addr, w, i, pc);
}

void exception2 (uaecptr addr)
{
	write_log (_T("delayed exception2!\n"));
	regs.panic_pc = m68k_getpc ();
	regs.panic_addr = addr;
	regs.panic = 2;
	set_special (SPCFLAG_BRK);
	m68k_setpc (0xf80000);
#ifdef JIT
	set_special (SPCFLAG_END_COMPILE);
#endif
	fill_prefetch ();
}

void cpureset (void)
{
  uaecptr pc;
  uaecptr ksboot = 0xf80002 - 2; /* -2 = RESET hasn't increased PC yet */
  uae_u16 ins;

  if (currprefs.cpu_compatible && currprefs.cpu_model <= 68020) {
  	custom_reset (false, false);
  	return;
  }
  pc = m68k_getpc();
  if (pc >= currprefs.chipmem_size) {
  	addrbank *b = &get_mem_bank(pc);
  	if (b->check(pc, 2 + 2)) {
	    /* We have memory, hope for the best.. */
	    custom_reset (false, false);
	    return;
  	}
		write_log (_T("M68K RESET PC=%x, rebooting..\n"), pc);
  	custom_reset (false, false);
  	m68k_setpc (ksboot);
  	return;
  }
  /* panic, RAM is going to disappear under PC */
  ins = get_word (pc + 2);
  if ((ins & ~7) == 0x4ed0) {
  	int reg = ins & 7;
  	uae_u32 addr = m68k_areg (regs, reg);
		write_log (_T("reset/jmp (ax) combination emulated -> %x\n"), addr);
  	custom_reset (false, false);
  	if (addr < 0x80000)
	    addr += 0xf80000;
  	m68k_setpc (addr - 2);
  	return;
  }
	write_log (_T("M68K RESET PC=%x, rebooting..\n"), pc);
  custom_reset (false, false);
  m68k_setpc (ksboot);
}

void m68k_setstopped (void)
{
	regs.stopped = 1;
	/* A traced STOP instruction drops through immediately without
	actually stopping.  */
	if ((regs.spcflags & SPCFLAG_DOTRACE) == 0)
		set_special (SPCFLAG_STOP);
	else
		m68k_resumestopped ();
}

void m68k_resumestopped (void)
{
	if (!regs.stopped)
		return;
	regs.stopped = 0;
	fill_prefetch ();
	unset_special (SPCFLAG_STOP);
}

/*
* Compute exact number of CPU cycles taken
* by DIVU and DIVS on a 68000 processor.
*
* Copyright (c) 2005 by Jorge Cwik, pasti@fxatari.com
*
* This is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This software is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this software; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*/


/*

 The routines below take dividend and divisor as parameters.
 They return 0 if division by zero, or exact number of cycles otherwise.

 The number of cycles returned assumes a register operand.
 Effective address time must be added if memory operand.

 For 68000 only (not 68010, 68012, 68020, etc).
 Probably valid for 68008 after adding the extra prefetch cycle.


 Best and worst cases for register operand:
 (Note the difference with the documented range.)


 DIVU:

 Overflow (always): 10 cycles.
 Worst case: 136 cycles.
 Best case: 76 cycles.


 DIVS:

 Absolute overflow: 16-18 cycles.
 Signed overflow is not detected prematurely.

 Worst case: 156 cycles.
 Best case without signed overflow: 122 cycles.
 Best case with signed overflow: 120 cycles


 */

int getDivu68kCycles (uae_u32 dividend, uae_u16 divisor)
{
	int mcycles;
	uae_u32 hdivisor;
	int i;

	if(divisor == 0)
		return 0;

	// Overflow
	if((dividend >> 16) >= divisor)
		return (mcycles = 5) * 2;

	mcycles = 38;
	hdivisor = divisor << 16;

	for( i = 0; i < 15; i++) {
		uae_u32 temp;
		temp = dividend;

		dividend <<= 1;

		// If carry from shift
		if((uae_s32)temp < 0)
			dividend -= hdivisor;
		else {
			mcycles += 2;
			if(dividend >= hdivisor) {
				dividend -= hdivisor;
				mcycles--;
			}
		}
	}
	return mcycles * 2;
}

int getDivs68kCycles (uae_s32 dividend, uae_s16 divisor)
{
	int mcycles;
	uae_u32 aquot;
	int i;

	if(divisor == 0)
		return 0;

	mcycles = 6;

	if( dividend < 0)
		mcycles++;

	// Check for absolute overflow
	if(((uae_u32)abs(dividend) >> 16) >= (uae_u16)abs(divisor))
		return (mcycles + 2) * 2;

	// Absolute quotient
	aquot = (uae_u32) abs(dividend) / (uae_u16)abs(divisor);

	mcycles += 55;

	if(divisor >= 0) {
		if(dividend >= 0)
			mcycles--;
		else
			mcycles++;
	}

	// Count 15 msbits in absolute of quotient

	for( i = 0; i < 15; i++) {
		if((uae_s16)aquot >= 0)
			mcycles++;
		aquot <<= 1;
	}

	return mcycles * 2;
}
