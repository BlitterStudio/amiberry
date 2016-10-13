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
#include "include/memory.h"
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
#include "audio.h"
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
bool m68k_pc_indirect;
static int cpu_prefs_changed_flag;

const int areg_byteinc[] = { 1,1,1,1,1,1,1,2 };
const int imm8_table[] = { 8,1,2,3,4,5,6,7 };

int movem_index1[256];
int movem_index2[256];
int movem_next[256];

cpuop_func *cpufunctbl[65536];

extern uae_u32 get_fpsr(void);

#define COUNT_INSTRS 0

static uae_u64 fake_srp_030, fake_crp_030;
static uae_u32 fake_tt0_030, fake_tt1_030, fake_tc_030;
static uae_u16 fake_mmusr_030;

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

void set_cpu_caches(bool flush)
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
		if ((regs.cacr & 0x800) || flush) { // clear data cache
			regs.cacr &= ~0x800;
		}
		if (regs.cacr & 0x400) { // clear entry in data cache
			regs.cacr &= ~0x400;
		}
  }
}

static uae_u32 REGPARAM2 op_illg_1 (uae_u32 opcode)
{
	op_illg (opcode);
	return 4;
}
static uae_u32 REGPARAM2 op_unimpl_1 (uae_u32 opcode)
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
#ifdef CPUEMU_11
		if (currprefs.cpu_compatible)
			tbl = op_smalltbl_11_ff; /* prefetch */
#endif
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
	    tbl = op_smalltbl_12_ff; /* prefetch */
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
    		abort();
	    cpufunctbl[opcode] = f;
  	}
  }
#ifdef JIT
  compiler_init ();
  build_comp ();
#endif

	write_log(_T("CPU=%d, FPU=%d, JIT%s=%d."),
			  currprefs.cpu_model, currprefs.fpu_model,
			  currprefs.cachesize ? _T("=CPU") : _T(""),
			  currprefs.cachesize);

  regs.address_space_mask = 0xffffffff;
  if (currprefs.cpu_compatible) {
  	if (currprefs.address_space_24 && currprefs.cpu_model >= 68030)
	    currprefs.address_space_24 = false;
  }
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

	m68k_pc_indirect = (currprefs.cpu_compatible) && !currprefs.cachesize;
	if (tbl == op_smalltbl_1_ff || tbl == op_smalltbl_2_ff || tbl == op_smalltbl_3_ff || tbl == op_smalltbl_4_ff || tbl == op_smalltbl_5_ff)
		m68k_pc_indirect = false;
  set_cpu_caches (true);
}

static unsigned long cycles_shift;
static unsigned long cycles_shift_2;

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
  if(cycles_shift_2)
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

static int check_prefs_changed_cpu2(void)
{
	int changed = 0;

#ifdef JIT
  changed = check_prefs_changed_comp () ? 1 : 0;
#endif
  if (changed
	|| currprefs.cpu_model != changed_prefs.cpu_model
	|| currprefs.fpu_model != changed_prefs.fpu_model
	|| currprefs.cpu_compatible != changed_prefs.cpu_compatible) {
			cpu_prefs_changed_flag |= 1;

  }
  if (changed 
    || currprefs.m68k_speed != changed_prefs.m68k_speed) {
			cpu_prefs_changed_flag |= 2;
  }
	return cpu_prefs_changed_flag;
}

void check_prefs_changed_cpu(void)
{
	if (check_prefs_changed_cpu2()) {
  	set_special (SPCFLAG_MODE_CHANGE);
		reset_frame_rate_hack ();
		set_speedup_values();
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
  	movem_index2[i] = 7-j;
  	movem_next[i] = i & (~(1 << j));
  }

#if COUNT_INSTRS
	memset (instrcount, 0, sizeof instrcount);
#endif

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

STATIC_INLINE int in_rom (uaecptr pc)
{
  return (munge24 (pc) & 0xFFF80000) == 0xF80000;
}

STATIC_INLINE int in_rtarea (uaecptr pc)
{
  return (munge24 (pc) & 0xFFFF0000) == rtarea_base && uae_boot_rom;
}

void REGPARAM2 MakeSR (void)
{
  regs.sr = ((regs.t1 << 15) | (regs.t0 << 14)
    | (regs.s << 13) | (regs.m << 12) | (regs.intmask << 8)
	  | (GET_XFLG() << 4) | (GET_NFLG() << 3)
	  | (GET_ZFLG() << 2) | (GET_VFLG() << 1)
	  |  GET_CFLG());
}

void REGPARAM2 MakeFromSR (void)
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
  		cycles = (34 - 4) * CYCLE_UNIT / 2;
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

void Exception (int nr)
{
  uae_u32 currpc, newpc;
  int sv = regs.s;

  if (currprefs.cachesize && nr != 61)
	  regs.instruction_pc = m68k_getpc ();

  if (nr >= 24 && nr < 24 + 8 && currprefs.cpu_model <= 68010)
	  nr = get_byte (0x00fffff1 | (nr << 1));

  MakeSR ();

  if (!regs.s) {
  	regs.usp = m68k_areg(regs, 7);
  	if (currprefs.cpu_model >= 68020) {
	    m68k_areg(regs, 7) = regs.m ? regs.msp : regs.isp;
  	} else {
	    m68k_areg(regs, 7) = regs.isp;
    }
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
			regs.m = 0;
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
			cpu_halt (2);
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

static void do_interrupt (int nr)
{
  regs.stopped = 0;
  unset_special (SPCFLAG_STOP);

  Exception (nr + 24);

  regs.intmask = nr;
  doint();
}

static void m68k_reset (bool hardreset)
{
	uae_u32 v;

  regs.pissoff = 0;
  cpu_cycles = 0;
  
  regs.spcflags = 0;
#ifdef SAVESTATE
  if (isrestore ()) {
  	m68k_setpc_normal (regs.pc);
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
	v = get_long (4);
  m68k_areg (regs, 7) = get_long (0);
	m68k_setpc_normal(v);
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
	  set_cpu_caches (false);
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

	regs.halted = 0;
	gui_data.cpu_halted = false;
	gui_led (LED_CPU, 0);

  fill_prefetch ();
}

void REGPARAM2 op_unimpl (uae_u16 opcode)
{
	Exception (61);
}

uae_u32 REGPARAM2 op_illg (uae_u32 opcode)
{
  uaecptr pc = m68k_getpc ();
  int inrom = in_rom(pc);
  int inrt = in_rtarea(pc);

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
		}
  }

#ifdef AUTOCONFIG
  if (opcode == 0xFF0D && inrt) {
    /* User-mode STOP replacement */
    m68k_setstopped ();
		return 4;
  }

  if ((opcode & 0xF000) == 0xA000 && inrt) {
  	/* Calltrap. */
		m68k_incpc_normal (2);
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

static void mmu_op30fake_pmove(uaecptr pc, uae_u32 opcode, uae_u16 next, uaecptr extra)
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
			x_put_long (extra, fake_tc_030);
  	else
			fake_tc_030 = x_get_long (extra);
    break;
  case 0x12: // SRP
  	reg = _T("SRP");
  	siz = 8;
  	if (rw) {
			x_put_long (extra, fake_srp_030 >> 32);
			x_put_long (extra + 4, fake_srp_030);
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
			x_put_long (extra + 4, fake_crp_030);
  	} else {
			fake_crp_030 = (uae_u64)x_get_long (extra) << 32;
			fake_crp_030 |= x_get_long (extra + 4);
  	}
    break;
  case 0x18: // MMUSR
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

  if (!reg) {
  	op_illg(opcode);
    return;
  }
}

static void mmu_op30fake_ptest(uaecptr pc, uae_u32 opcode, uae_u16 next, uaecptr extra)
{
  fake_mmusr_030 = 0;
}

static void mmu_op30fake_pflush(uaecptr pc, uae_u32 opcode, uae_u16 next, uaecptr extra)
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

// 68030 (68851) MMU instructions only
void mmu_op30 (uaecptr pc, uae_u32 opcode, uae_u16 extra, uaecptr extraa)
{
	int type = extra >> 13;

	switch (type)
	{
	  case 0:
	  case 2:
	  case 3:
		  mmu_op30fake_pmove (pc, opcode, extra, extraa);
	    break;
	  case 1:
		  mmu_op30fake_pflush (pc, opcode, extra, extraa);
	    break;
	  case 4:
		  mmu_op30fake_ptest (pc, opcode, extra, extraa);
	    break;
	  default:
		  op_illg (opcode);
	    break;
	}
}

// 68040+ MMU instructions only
void mmu_op(uae_u32 opcode, uae_u32 extra)
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
  m68k_setpc_normal (m68k_getpc () - 2);
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
  	m68k_setpc_normal (m68k_getpc ());
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

static int do_specialties (int cycles)
{
	if (regs.spcflags & SPCFLAG_MODE_CHANGE)
		return 1;

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
	  x_do_cycles (c * CYCLE_UNIT);
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

    x_do_cycles (4 * CYCLE_UNIT);
    if (regs.spcflags & SPCFLAG_COPPER)
      do_copper ();

	  if (regs.spcflags & (SPCFLAG_INT | SPCFLAG_DOINT)) {
	    int intr = intlev ();
  	  unset_special (SPCFLAG_INT | SPCFLAG_DOINT);
	     if (intr > 0 && intr > regs.intmask)
		     do_interrupt (intr);
    }

		if (regs.spcflags & SPCFLAG_MODE_CHANGE) {
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
      do_interrupt (intr);
  }

  if (regs.spcflags & SPCFLAG_DOINT) {
    unset_special (SPCFLAG_DOINT);
    set_special (SPCFLAG_INT);
  }

  if (regs.spcflags & SPCFLAG_BRK) {
		unset_special(SPCFLAG_BRK);
  }

  return 0;
}

/* It's really sad to have two almost identical functions for this, but we
   do it all for performance... :(
   This version emulates 68000's prefetch "cache" */
static void m68k_run_1 (void)
{
	struct regstruct *r = &regs;

  for (;;) {
		uae_u16 opcode = r->ir;

#ifdef CPU_arm
    /* Load ARM code for next opcode into L2 cache during execute of do_cycles() */
    __asm__ volatile ("pli [%[radr]]\n\t" \
      : : [radr] "r" (cpufunctbl[opcode]) : );
#endif
		count_instr (opcode);

  	do_cycles (cpu_cycles);
		r->instruction_pc = m68k_getpc ();
  	cpu_cycles = (*cpufunctbl[opcode])(opcode);
  	cpu_cycles = adjust_cycles(cpu_cycles);
		if (r->spcflags) {
      if (do_specialties (cpu_cycles)) {
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
	struct regstruct *r = &regs;

  for (;;)
  {
  	uae_u16 opcode = get_diword (0);
  	cpu_cycles = (*cpufunctbl[opcode])(opcode);
  	cpu_cycles = adjust_cycles(cpu_cycles);
  	do_cycles (cpu_cycles);

  	if (end_block(opcode) || r->spcflags || uae_int_requested)
	    return; /* We will deal with the spcflags in the caller */
  }
}

void execute_normal(void)
{
	struct regstruct *r = &regs;
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
  	uae_u16 opcode;

		regs.instruction_pc = m68k_getpc ();
    opcode = get_diword (0);

  	special_mem = DISTRUST_CONSISTENT_MEM;
  	pc_hist[blocklen].location = (uae_u16*)r->pc_p;

  	cpu_cycles = (*cpufunctbl[opcode])(opcode);
  	cpu_cycles = adjust_cycles(cpu_cycles);
  	do_cycles (cpu_cycles);
  	total_cycles += cpu_cycles;
  	pc_hist[blocklen].specmem = special_mem;
  	blocklen++;
  	if (end_block(opcode) || blocklen >= MAXRUN || r->spcflags || uae_int_requested) {
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
	    if (do_specialties (0)) {
    		return;
	    }
  	}
  }
}
#endif /* JIT */

void cpu_halt (int id)
{
	if (!regs.halted) {
		write_log (_T("CPU halted: reason = %d\n"), id);
		regs.halted = id;
		gui_data.cpu_halted = true;
		gui_led (LED_CPU, 0);
		regs.intmask = 7;
		MakeSR ();
		audio_deactivate ();
	}
	while (regs.halted) {
		if (vpos == 0)
			sleep_millis_main (8);
		x_do_cycles (100 * CYCLE_UNIT);
		if (regs.spcflags) {
			if ((regs.spcflags & (SPCFLAG_BRK | SPCFLAG_MODE_CHANGE)))
				return;
		}
	}
}

/* Same thing, but don't use prefetch to get opcode.  */
static void m68k_run_2 (void)
{
	struct regstruct *r = &regs;

  for (;;) {
		r->instruction_pc = m68k_getpc ();
	  uae_u16 opcode = get_diword (0);
    
#ifdef CPU_arm
    /* Load ARM code for next opcode into L2 cache during execute of do_cycles() */
    __asm__ volatile ("pli [%[radr]]\n\t" \
       : : [radr] "r" (cpufunctbl[opcode]) : );
#endif
		count_instr (opcode);

	  do_cycles (cpu_cycles);
	  cpu_cycles = (*cpufunctbl[opcode])(opcode);
	  cpu_cycles = adjust_cycles(cpu_cycles);
		if (r->spcflags) {
	    if (do_specialties (cpu_cycles)) {
		    break;
      }
	  }
  }
}

static int in_m68k_go = 0;

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

	cpu_prefs_changed_flag = 0;
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
			m68k_reset (hardreset != 0);
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
			m68k_setpc_normal (regs.pc);
			check_prefs_changed_audio ();

			if (!restored || hsync_counter == 0)
				savestate_check ();
	  }

	  if (regs.panic) {
	    regs.panic = 0;
	    /* program jumped to non-existing memory and cpu was >= 68020 */
			get_real_address (regs.isp); /* stack in no one's land? -> halt */
	    if (regs.isp & 1)
				regs.panic = 5;
	    if (!regs.panic)
		    exception2_handle (regs.panic_pc, regs.panic_addr);
	    if (regs.panic) {
				int id = regs.panic;
		    /* system is very badly confused */
		    regs.panic = 0;
				cpu_halt (id);
	    }
  	}

		if (regs.spcflags & SPCFLAG_MODE_CHANGE) {
			if (cpu_prefs_changed_flag & 1) {
				uaecptr pc = m68k_getpc();
				prefs_changed_cpu();
				build_cpufunctbl();
				m68k_setpc_normal(pc);
				fill_prefetch();
      }
			if (cpu_prefs_changed_flag & 2) {
				fixup_cpu(&changed_prefs);
				currprefs.m68k_speed = changed_prefs.m68k_speed;
				update_68k_cycles();
			}
			cpu_prefs_changed_flag = 0;
		}

	  if (startup) {
		  custom_prepare ();
			protect_roms (true);
		}
	  startup = 0;
		if (regs.halted) {
			cpu_halt (regs.halted);
			continue;
		}
    run_func = 
      currprefs.cpu_compatible && currprefs.cpu_model <= 68010 ? m68k_run_1 :
#ifdef JIT
      currprefs.cpu_model >= 68020 && currprefs.cachesize ? m68k_run_jit :
#endif
      m68k_run_2;
		unset_special(SPCFLAG_MODE_CHANGE);
		unset_special(SPCFLAG_BRK);
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
		fake_crp_030 = restore_u64 ();
		fake_srp_030 = restore_u64 ();
		fake_tt0_030 = restore_u32 ();
		fake_tt1_030 = restore_u32 ();
		fake_tc_030 = restore_u32 ();
		fake_mmusr_030 = restore_u16 ();
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
  	restore_u32();
  	if (khz > 0 && khz < 800000)
	    currprefs.m68k_speed = changed_prefs.m68k_speed = 0;
  }
  set_cpu_caches(true);

  write_log (_T("CPU: %d%s%03d, PC=%08X\n"),
  	model / 1000, flags & 1 ? _T("EC") : _T(""), model % 1000, regs.pc);

  return src;
}

static void fill_prefetch_quick (void)
{
	if (currprefs.cpu_model >= 68020) {
		return;
	}
	// old statefile compatibility, this needs to done,
	// even in 68000 cycle-exact mode
	regs.ir = get_word (m68k_getpc ());
	regs.irc = get_word (m68k_getpc () + 2);
}

void restore_cpu_finish(void)
{
  init_m68k ();
	m68k_setpc_normal (regs.pc);
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
  MakeSR ();
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
  	save_u64 (fake_crp_030);				/* CRP */
  	save_u64 (fake_srp_030);				/* SRP */
  	save_u32 (fake_tt0_030);				/* TT0/AC0 */
  	save_u32 (fake_tt1_030);				/* TT1/AC1 */
  	save_u32 (fake_tc_030);				/* TCR */
  	save_u16 (fake_mmusr_030);				/* MMUSR/ACUSR */
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

static void exception3f (uae_u32 opcode, uaecptr addr, int writeaccess, int instructionaccess, uaecptr pc)
{
  if (currprefs.cpu_model >= 68040)
	  addr &= ~1;
  if (currprefs.cpu_model >= 68020) {
		if (pc == 0xffffffff)
		  last_addr_for_exception_3 = regs.instruction_pc;
		else
			last_addr_for_exception_3 = pc;
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
void exception3b (uae_u32 opcode, uaecptr addr, bool w, bool i, uaecptr pc)
{
	exception3f (opcode, addr, w, i, pc);
}

void exception2 (uaecptr addr)
{
	exception2_handle (addr, addr);
}

void exception2_fake (uaecptr addr)
{
	write_log (_T("delayed exception2!\n"));
	regs.panic_pc = m68k_getpc ();
	regs.panic_addr = addr;
	regs.panic = 6;
	set_special (SPCFLAG_BRK);
	m68k_setpc_normal (0xf80000);
#ifdef JIT
	set_special (SPCFLAG_END_COMPILE);
#endif
	fill_prefetch ();
}

void cpureset (void)
{
    /* RESET hasn't increased PC yet, 1 word offset */
  uaecptr pc;
  uaecptr ksboot = 0xf80002 - 2;
  uae_u16 ins;
	addrbank *ab;

  if (currprefs.cpu_compatible && currprefs.cpu_model <= 68020) {
  	custom_reset (false, false);
  	return;
  }
  pc = m68k_getpc () + 2;
	ab = &get_mem_bank (pc);
	if (ab->check (pc, 2)) {
		write_log (_T("CPU reset PC=%x (%s)..\n"), pc - 2, ab->name);
		ins = get_word (pc);
    custom_reset (false, false);
		// did memory disappear under us?
		if (ab == &get_mem_bank (pc))
	    return;
		// it did
    if ((ins & ~7) == 0x4ed0) {
    	int reg = ins & 7;
    	uae_u32 addr = m68k_areg (regs, reg);
    	if (addr < 0x80000)
	      addr += 0xf80000;
			write_log (_T("reset/jmp (ax) combination emulated -> %x\n"), addr);
			m68k_setpc_normal (addr - 2);
    	return;
    }
	}
	// the best we can do, jump directly to ROM entrypoint
	// (which is probably what program wanted anyway)
	write_log (_T("CPU Reset PC=%x (%s), invalid memory -> %x.\n"), pc, ab->name, ksboot + 2);
  custom_reset (false, false);
	m68k_setpc_normal (ksboot);
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

void fill_prefetch (void)
{
	if (currprefs.cachesize)
		return;
	if (currprefs.cpu_model <= 68010) {
	  uaecptr pc = m68k_getpc ();
	  regs.ir = x_get_word (pc);
	  regs.irc = x_get_word (pc + 2);
	}
}
