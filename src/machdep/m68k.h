/*
 * m68k.h - machine dependent bits
 *
 * Copyright (c) 2001-2004 Milan Jurik of ARAnyM dev team (see AUTHORS)
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
  * MC68000 emulation - machine dependent bits
  *
  * Copyright 1996 Bernd Schmidt
  * Copyright 2004-2005 Richard Drummond
  */

#ifndef M68K_FLAGS_H
#define M68K_FLAGS_H

#if defined(CPU_i386) || defined(CPU_x86_64)

#ifdef __cplusplus
# include <cstdlib>
#else
# include <stdlib.h>
#endif

#ifndef SAHF_SETO_PROFITABLE

/*
 * Machine dependent structure for holding the 68k CCR flags
 */
/* PUSH/POP instructions are naturally 64-bit sized on x86-64, thus
   unsigned long hereunder is either 64-bit or 32-bit wide depending
   on the target.  */
struct flag_struct {
#if defined(CPU_x86_64)
	uint64 cznv;
	uint64 x;
#else
    uint32 cznv;
    uint32 x;
#endif
};

/*
 * The bits in the cznv field in the above structure are assigned to
 * allow the easy mirroring of the x86 rFLAGS register.
 *
 * The 68k CZNV flags are thus assigned in cznv as:
 *
 * 76543210  FEDCBA98 --------- ---------
 * SZxxxxxC  xxxxVxxx xxxxxxxxx xxxxxxxxx
 */

#define FLAGBIT_N   7
#define FLAGBIT_Z   6
#define FLAGBIT_C   0
#define FLAGBIT_V   11
#define FLAGBIT_X   0 /* must be in position 0 for duplicate_carry() to work */

#define FLAGVAL_N   (1 << FLAGBIT_N)
#define FLAGVAL_Z   (1 << FLAGBIT_Z)
#define FLAGVAL_C   (1 << FLAGBIT_C)
#define FLAGVAL_V   (1 << FLAGBIT_V)
#define FLAGVAL_X   (1 << FLAGBIT_X)

#define SET_ZFLG(y) (regflags.cznv = (((uae_u32)regflags.cznv) & ~FLAGVAL_Z) | (((y) & 1) << FLAGBIT_Z))
#define SET_CFLG(y) (regflags.cznv = (((uae_u32)regflags.cznv) & ~FLAGVAL_C) | (((y) & 1) << FLAGBIT_C))
#define SET_VFLG(y) (regflags.cznv = (((uae_u32)regflags.cznv) & ~FLAGVAL_V) | (((y) & 1) << FLAGBIT_V))
#define SET_NFLG(y) (regflags.cznv = (((uae_u32)regflags.cznv) & ~FLAGVAL_N) | (((y) & 1) << FLAGBIT_N))
#define SET_XFLG(y) (regflags.x = ((y) & 1) << FLAGBIT_X)

#define GET_ZFLG()	((regflags.cznv >> FLAGBIT_Z) & 1)
#define GET_CFLG()	((regflags.cznv >> FLAGBIT_C) & 1)
#define GET_VFLG()	((regflags.cznv >> FLAGBIT_V) & 1)
#define GET_NFLG()	((regflags.cznv >> FLAGBIT_N) & 1)
#define GET_XFLG()	((regflags.x    >> FLAGBIT_X) & 1)

#define CLEAR_CZNV()	(regflags.cznv = 0)
#define GET_CZNV()		(regflags.cznv)
#define IOR_CZNV(X)		(regflags.cznv |= (X))
#define SET_CZNV(X)		(regflags.cznv = (X))

#define COPY_CARRY()	(regflags.x = regflags.cznv >> (FLAGBIT_C - FLAGBIT_X))

extern struct flag_struct regflags __asm__ ("regflags");

/*
 * Test CCR condition
 */
static inline int cctrue(int cc)
{
    uae_u32 cznv = regflags.cznv;

    switch (cc) {
    case 0:  return 1;                              /*              T  */
    case 1:  return 0;                              /*              F  */
    case 2:  return (cznv & (FLAGVAL_C | FLAGVAL_Z)) == 0;              /* !CFLG && !ZFLG       HI */
    case 3:  return (cznv & (FLAGVAL_C | FLAGVAL_Z)) != 0;              /*  CFLG || ZFLG        LS */
    case 4:  return (cznv & FLAGVAL_C) == 0;                    /* !CFLG            CC */
    case 5:  return (cznv & FLAGVAL_C) != 0;                    /*  CFLG            CS */
    case 6:  return (cznv & FLAGVAL_Z) == 0;                    /* !ZFLG            NE */
    case 7:  return (cznv & FLAGVAL_Z) != 0;                    /*  ZFLG            EQ */
    case 8:  return (cznv & FLAGVAL_V) == 0;                    /* !VFLG            VC */
    case 9:  return (cznv & FLAGVAL_V) != 0;                    /*  VFLG            VS */
    case 10: return (cznv & FLAGVAL_N) == 0;                    /* !NFLG            PL */
    case 11: return (cznv & FLAGVAL_N) != 0;                    /*  NFLG            MI */
#if FLAGBIT_N > FLAGBIT_V
    case 12: return (((cznv << (FLAGBIT_N - FLAGBIT_V)) ^ cznv) & FLAGVAL_N) == 0;  /*  NFLG == VFLG        GE */
    case 13: return (((cznv << (FLAGBIT_N - FLAGBIT_V)) ^ cznv) & FLAGVAL_N) != 0;  /*  NFLG != VFLG        LT */
    case 14: cznv &= (FLAGVAL_N | FLAGVAL_Z | FLAGVAL_V);               /* !ZFLG && (NFLG == VFLG)   GT */
         return (((cznv << (FLAGBIT_N - FLAGBIT_V)) ^ cznv) & (FLAGVAL_N | FLAGVAL_Z)) == 0;
    case 15: cznv &= (FLAGVAL_N | FLAGVAL_Z | FLAGVAL_V);               /* ZFLG || (NFLG != VFLG)   LE */
         return (((cznv << (FLAGBIT_N - FLAGBIT_V)) ^ cznv) & (FLAGVAL_N | FLAGVAL_Z)) != 0;
#else
    case 12: return (((cznv << (FLAGBIT_V - FLAGBIT_N)) ^ cznv) & FLAGVAL_V) == 0;  /*  NFLG == VFLG        GE */
    case 13: return (((cznv << (FLAGBIT_V - FLAGBIT_N)) ^ cznv) & FLAGVAL_V) != 0;  /*  NFLG != VFLG        LT */
    case 14: cznv &= (FLAGVAL_N | FLAGVAL_Z | FLAGVAL_V);               /* !ZFLG && (NFLG == VFLG)   GT */
         return (((cznv << (FLAGBIT_V - FLAGBIT_N)) ^ cznv) & (FLAGVAL_V | FLAGVAL_Z)) == 0;
    case 15: cznv &= (FLAGVAL_N | FLAGVAL_Z | FLAGVAL_V);               /* ZFLG || (NFLG != VFLG)   LE */
         return (((cznv << (FLAGBIT_V - FLAGBIT_N)) ^ cznv) & (FLAGVAL_V | FLAGVAL_Z)) != 0;
#endif
    }
    abort ();
    return 0;
}


#define optflag_testl(v) \
  __asm__ __volatile__ ("andl %1,%1\n\t" \
			"pushf\n\t" \
			"pop %0\n\t" \
			: "=rm" (regflags.cznv) : "r" (v) : "memory", "cc")

#define optflag_testw(v) \
  __asm__ __volatile__ ("andw %w1,%w1\n\t" \
			"pushf\n\t" \
			"pop %0\n\t" \
			: "=rm" (regflags.cznv) : "r" (v) : "memory", "cc")

#define optflag_testb(v) \
  __asm__ __volatile__ ("andb %b1,%b1\n\t" \
			"pushf\n\t" \
			"pop %0\n\t" \
			: "=rm" (regflags.cznv) : "q" (v) : "memory", "cc")

#define optflag_addl(v, s, d) do { \
  __asm__ __volatile__ ("addl %k2,%k1\n\t" \
			"pushf\n\t" \
			"pop %0\n\t" \
			: "=rm" (regflags.cznv), "=r" (v) : "rmi" (s), "1" (d) : "memory", "cc"); \
    COPY_CARRY(); \
    } while (0)

#define optflag_addw(v, s, d) do { \
  __asm__ __volatile__ ("addw %w2,%w1\n\t" \
			"pushf\n\t" \
			"pop %0\n\t" \
			: "=rm" (regflags.cznv), "=r" (v) : "rmi" (s), "1" (d) : "memory", "cc"); \
    COPY_CARRY(); \
    } while (0)

#define optflag_addb(v, s, d) do { \
  __asm__ __volatile__ ("addb %b2,%b1\n\t" \
			"pushf\n\t" \
			"pop %0\n\t" \
			: "=r" (regflags.cznv), "=q" (v) : "qmi" (s), "1" (d) : "cc"); \
    COPY_CARRY(); \
    } while (0)

#define optflag_subl(v, s, d) do { \
  __asm__ __volatile__ ("subl %k2,%k1\n\t" \
			"pushf\n\t" \
			"pop %0\n\t" \
			: "=rm" (regflags.cznv), "=r" (v) : "rmi" (s), "1" (d) : "memory", "cc"); \
    COPY_CARRY(); \
    } while (0)

#define optflag_subw(v, s, d) do { \
  __asm__ __volatile__ ("subw %w2,%w1\n\t" \
			"pushf\n\t" \
			"pop %0\n\t" \
			: "=rm" (regflags.cznv), "=r" (v) : "rmi" (s), "1" (d) : "memory", "cc"); \
    COPY_CARRY(); \
    } while (0)

#define optflag_subb(v, s, d) do { \
  __asm__ __volatile__ ("subb %b2,%b1\n\t" \
			"pushf\n\t" \
			"pop %0\n\t" \
			: "=rm" (regflags.cznv), "=q" (v) : "qmi" (s), "1" (d) : "memory", "cc"); \
    COPY_CARRY(); \
    } while (0)

#define optflag_cmpl(s, d) \
  __asm__ __volatile__ ("cmpl %k1,%k2\n\t" \
			"pushf\n\t" \
			"pop %0\n\t" \
			: "=rm" (regflags.cznv) : "rmi" (s), "r" (d) : "memory", "cc")

#define optflag_cmpw(s, d) \
  __asm__ __volatile__ ("cmpw %w1,%w2\n\t" \
			"pushf\n\t" \
			"pop %0\n\t" \
			: "=rm" (regflags.cznv) : "rmi" (s), "r" (d) : "memory", "cc")

#define optflag_cmpb(s, d) \
  __asm__ __volatile__ ("cmpb %b1,%b2\n\t" \
			"pushf\n\t" \
			"pop %0\n\t" \
			: "=rm" (regflags.cznv) : "qmi" (s), "q" (d) : "memory", "cc")

#else /* !SAHF_SETO_PROFITABLE */

/*
 * Machine dependent structure for holding the 68k CCR flags
 */
struct flag_struct {
	uae_u32 cznv;
	uae_u32 x;
};

extern struct flag_struct regflags __asm__ ("regflags");

/*
 * The bits in the cznv field in the above structure are assigned to
 * allow the easy mirroring of the x86 condition flags. (For example,
 * from the AX register - the x86 overflow flag can be copied to AL
 * with a setto %AL instr and the other flags copied to AH with an
 * lahf instr).
 *
 * The 68k CZNV flags are thus assigned in cznv as:
 *
 * <--AL-->  <--AH-->
 * 76543210  FEDCBA98 --------- ---------
 * xxxxxxxV  NZxxxxxC xxxxxxxxx xxxxxxxxx
 */

#define FLAGBIT_N   15
#define FLAGBIT_Z   14
#define FLAGBIT_C   8
#define FLAGBIT_V   0
#define FLAGBIT_X   0 /* must be in position 0 for duplicate_carry() to work */

#define FLAGVAL_N   (1 << FLAGBIT_N)
#define FLAGVAL_Z   (1 << FLAGBIT_Z)
#define FLAGVAL_C   (1 << FLAGBIT_C)
#define FLAGVAL_V   (1 << FLAGBIT_V)
#define FLAGVAL_X   (1 << FLAGBIT_X)

#define SET_ZFLG(y) (regflags.cznv = (((uae_u32)regflags.cznv) & ~FLAGVAL_Z) | (((y) & 1) << FLAGBIT_Z))
#define SET_CFLG(y) (regflags.cznv = (((uae_u32)regflags.cznv) & ~FLAGVAL_C) | (((y) & 1) << FLAGBIT_C))
#define SET_VFLG(y) (regflags.cznv = (((uae_u32)regflags.cznv) & ~FLAGVAL_V) | (((y) & 1) << FLAGBIT_V))
#define SET_NFLG(y) (regflags.cznv = (((uae_u32)regflags.cznv) & ~FLAGVAL_N) | (((y) & 1) << FLAGBIT_N))
#define SET_XFLG(y) (regflags.x    = ((y) & 1) << FLAGBIT_X)

#define GET_ZFLG()	((regflags.cznv >> FLAGBIT_Z) & 1)
#define GET_CFLG()	((regflags.cznv >> FLAGBIT_C) & 1)
#define GET_VFLG()	((regflags.cznv >> FLAGBIT_V) & 1)
#define GET_NFLG()	((regflags.cznv >> FLAGBIT_N) & 1)
#define GET_XFLG()	((regflags.x    >> FLAGBIT_X) & 1)

#define CLEAR_CZNV()	(regflags.cznv = 0)
#define GET_CZNV()		(regflags.cznv)
#define IOR_CZNV(X)		(regflags.cznv |= (X))
#define SET_CZNV(X)		(regflags.cznv = (X))

#define COPY_CARRY()	(regflags.x = regflags.cznv >> (FLAGBIT_C - FLAGBIT_X))


/*
 * Test CCR condition
 */
static inline int cctrue(int cc)
{
	uae_u32 cznv = regflags.cznv;

	switch (cc) {
		case 0:  return 1;                              /*              T  */
		case 1:  return 0;                              /*              F  */
		case 2:  return (cznv & (FLAGVAL_C | FLAGVAL_Z)) == 0;              /* !CFLG && !ZFLG       HI */
		case 3:  return (cznv & (FLAGVAL_C | FLAGVAL_Z)) != 0;              /*  CFLG || ZFLG        LS */
		case 4:  return (cznv & FLAGVAL_C) == 0;                    /* !CFLG            CC */
		case 5:  return (cznv & FLAGVAL_C) != 0;                    /*  CFLG            CS */
		case 6:  return (cznv & FLAGVAL_Z) == 0;                    /* !ZFLG            NE */
		case 7:  return (cznv & FLAGVAL_Z) != 0;                    /*  ZFLG            EQ */
		case 8:  return (cznv & FLAGVAL_V) == 0;                    /* !VFLG            VC */
		case 9:  return (cznv & FLAGVAL_V) != 0;                    /*  VFLG            VS */
		case 10: return (cznv & FLAGVAL_N) == 0;                    /* !NFLG            PL */
		case 11: return (cznv & FLAGVAL_N) != 0;                    /*  NFLG            MI */
#if FLAGBIT_N > FLAGBIT_V
		case 12: return (((cznv << (FLAGBIT_N - FLAGBIT_V)) ^ cznv) & FLAGVAL_N) == 0;  /*  NFLG == VFLG        GE */
		case 13: return (((cznv << (FLAGBIT_N - FLAGBIT_V)) ^ cznv) & FLAGVAL_N) != 0;  /*  NFLG != VFLG        LT */
		case 14: cznv &= (FLAGVAL_N | FLAGVAL_Z | FLAGVAL_V);               /* !ZFLG && (NFLG == VFLG)   GT */
			return (((cznv << (FLAGBIT_N - FLAGBIT_V)) ^ cznv) & (FLAGVAL_N | FLAGVAL_Z)) == 0;
		case 15: cznv &= (FLAGVAL_N | FLAGVAL_Z | FLAGVAL_V);               /* ZFLG || (NFLG != VFLG)   LE */
			return (((cznv << (FLAGBIT_N - FLAGBIT_V)) ^ cznv) & (FLAGVAL_N | FLAGVAL_Z)) != 0;
#else
		case 12: return (((cznv << (FLAGBIT_V - FLAGBIT_N)) ^ cznv) & FLAGVAL_V) == 0;  /*  NFLG == VFLG        GE */
		case 13: return (((cznv << (FLAGBIT_V - FLAGBIT_N)) ^ cznv) & FLAGVAL_V) != 0;  /*  NFLG != VFLG        LT */
		case 14: cznv &= (FLAGVAL_N | FLAGVAL_Z | FLAGVAL_V);               /* !ZFLG && (NFLG == VFLG)   GT */
			return (((cznv << (FLAGBIT_V - FLAGBIT_N)) ^ cznv) & (FLAGVAL_V | FLAGVAL_Z)) == 0;
		case 15: cznv &= (FLAGVAL_N | FLAGVAL_Z | FLAGVAL_V);               /* ZFLG || (NFLG != VFLG)   LE */
			return (((cznv << (FLAGBIT_V - FLAGBIT_N)) ^ cznv) & (FLAGVAL_V | FLAGVAL_Z)) != 0;
#endif
	}
	abort ();
	return 0;
}


/* Manually emit LAHF instruction so that 64-bit assemblers can grok it */
#if defined CPU_x86_64 && defined __GNUC__
#define ASM_LAHF ".byte 0x9f"
#else
#define ASM_LAHF "lahf"
#endif

/* Is there any way to do this without declaring *all* memory clobbered?
   I.e. any way to tell gcc that some byte-sized value is in %al? */
#define optflag_testl(v) \
  __asm__ __volatile__ ("andl %0,%0\n\t" \
			ASM_LAHF "\n\t" \
			"seto %%al\n\t" \
			"movb %%al,regflags\n\t" \
			"movb %%ah,regflags+1\n\t" \
			: : "r" (v) : "%eax","cc","memory")

#define optflag_testw(v) \
  __asm__ __volatile__ ("andw %w0,%w0\n\t" \
			ASM_LAHF "\n\t" \
			"seto %%al\n\t" \
			"movb %%al,regflags\n\t" \
			"movb %%ah,regflags+1\n\t" \
			: : "r" (v) : "%eax","cc","memory")

#define optflag_testb(v) \
  __asm__ __volatile__ ("andb %b0,%b0\n\t" \
			ASM_LAHF "\n\t" \
			"seto %%al\n\t" \
			"movb %%al,regflags\n\t" \
			"movb %%ah,regflags+1\n\t" \
			: : "q" (v) : "%eax","cc","memory")

#define optflag_addl(v, s, d) do { \
  __asm__ __volatile__ ("addl %k1,%k0\n\t" \
			ASM_LAHF "\n\t" \
			"seto %%al\n\t" \
			"movb %%al,regflags\n\t" \
			"movb %%ah,regflags+1\n\t" \
			: "=r" (v) : "rmi" (s), "0" (d) : "%eax","cc","memory"); \
			COPY_CARRY(); \
	} while (0)

#define optflag_addw(v, s, d) do { \
  __asm__ __volatile__ ("addw %w1,%w0\n\t" \
			ASM_LAHF "\n\t" \
			"seto %%al\n\t" \
			"movb %%al,regflags\n\t" \
			"movb %%ah,regflags+1\n\t" \
			: "=r" (v) : "rmi" (s), "0" (d) : "%eax","cc","memory"); \
			COPY_CARRY(); \
    } while (0)

#define optflag_addb(v, s, d) do { \
  __asm__ __volatile__ ("addb %b1,%b0\n\t" \
			ASM_LAHF "\n\t" \
			"seto %%al\n\t" \
			"movb %%al,regflags\n\t" \
			"movb %%ah,regflags+1\n\t" \
			: "=q" (v) : "qmi" (s), "0" (d) : "%eax","cc","memory"); \
			COPY_CARRY(); \
    } while (0)

#define optflag_subl(v, s, d) do { \
  __asm__ __volatile__ ("subl %k1,%k0\n\t" \
			ASM_LAHF "\n\t" \
			"seto %%al\n\t" \
			"movb %%al,regflags\n\t" \
			"movb %%ah,regflags+1\n\t" \
			: "=r" (v) : "rmi" (s), "0" (d) : "%eax","cc","memory"); \
			COPY_CARRY(); \
    } while (0)

#define optflag_subw(v, s, d) do { \
  __asm__ __volatile__ ("subw %w1,%w0\n\t" \
			ASM_LAHF "\n\t" \
			"seto %%al\n\t" \
			"movb %%al,regflags\n\t" \
			"movb %%ah,regflags+1\n\t" \
			: "=r" (v) : "rmi" (s), "0" (d) : "%eax","cc","memory"); \
			COPY_CARRY(); \
    } while (0)

#define optflag_subb(v, s, d) do { \
   __asm__ __volatile__ ("subb %b1,%b0\n\t" \
			ASM_LAHF "\n\t" \
			"seto %%al\n\t" \
			"movb %%al,regflags\n\t" \
			"movb %%ah,regflags+1\n\t" \
			: "=q" (v) : "qmi" (s), "0" (d) : "%eax","cc","memory"); \
			COPY_CARRY(); \
    } while (0)

#define optflag_cmpl(s, d) \
  __asm__ __volatile__ ("cmpl %k0,%k1\n\t" \
			ASM_LAHF "\n\t" \
			"seto %%al\n\t" \
			"movb %%al,regflags\n\t" \
			"movb %%ah,regflags+1\n\t" \
			: : "rmi" (s), "r" (d) : "%eax","cc","memory")

#define optflag_cmpw(s, d) \
  __asm__ __volatile__ ("cmpw %w0,%w1\n\t" \
			ASM_LAHF "\n\t" \
			"seto %%al\n\t" \
			"movb %%al,regflags\n\t" \
			"movb %%ah,regflags+1\n\t" \
			: : "rmi" (s), "r" (d) : "%eax","cc","memory")

#define optflag_cmpb(s, d) \
  __asm__ __volatile__ ("cmpb %b0,%b1\n\t" \
			ASM_LAHF "\n\t" \
			"seto %%al\n\t" \
			"movb %%al,regflags\n\t" \
			"movb %%ah,regflags+1\n\t" \
			: : "qmi" (s), "q" (d) : "%eax","cc","memory")

#endif /* SAHF_SETO_PROFITABLE */

#elif defined(CPU_riscv64) || defined(CPU_arm) || defined(CPU_AARCH64)

#ifdef __cplusplus
# include <cstdlib>
#else
# include <stdlib.h>
#endif

/*
 * Machine dependent structure for holding the 68k CCR flags
 */
struct flag_struct {
#if defined(CPU_riscv64) || defined (CPU_AARCH64)
	uae_u64 nzcv;
	uae_u64 x;
#else
	uae_u32 nzcv;
	uae_u32 x;
#endif
};

#define FLAGBIT_N   31
#define FLAGBIT_Z   30
#define FLAGBIT_C   29
#define FLAGBIT_V   28
#define FLAGBIT_X   FLAGBIT_C /* must be in the same position in as x flag */

#define FLAGVAL_N   (1 << FLAGBIT_N)
#define FLAGVAL_Z   (1 << FLAGBIT_Z)
#define FLAGVAL_C   (1 << FLAGBIT_C)
#define FLAGVAL_V   (1 << FLAGBIT_V)
#define FLAGVAL_X   (1 << FLAGBIT_X)

#define SET_NFLG(y)		(regflags.nzcv = (regflags.nzcv & ~FLAGVAL_N) | (((y) & 1) << FLAGBIT_N))
#define SET_ZFLG(y)		(regflags.nzcv = (regflags.nzcv & ~FLAGVAL_Z) | (((y) & 1) << FLAGBIT_Z))
#define SET_CFLG(y)		(regflags.nzcv = (regflags.nzcv & ~FLAGVAL_C) | (((y) & 1) << FLAGBIT_C))
#define SET_VFLG(y)		(regflags.nzcv = (regflags.nzcv & ~FLAGVAL_V) | (((y) & 1) << FLAGBIT_V))
#define SET_XFLG(y)		(regflags.x = ((y) & 1) << FLAGBIT_X)

#define GET_NFLG()		((regflags.nzcv >> FLAGBIT_N) & 1)
#define GET_ZFLG()		((regflags.nzcv >> FLAGBIT_Z) & 1)
#define GET_CFLG()		((regflags.nzcv >> FLAGBIT_C) & 1)
#define GET_VFLG()		((regflags.nzcv >> FLAGBIT_V) & 1)
#define GET_XFLG()		((regflags.x    >> FLAGBIT_X) & 1)

#define CLEAR_CZNV()	(regflags.nzcv = 0)
#define GET_CZNV()		(regflags.nzcv)
#define IOR_CZNV(X)		(regflags.nzcv |= (X))
#define SET_CZNV(X)		(regflags.nzcv = (X))

#define COPY_CARRY()	(regflags.x = regflags.nzcv >> (FLAGBIT_C - FLAGBIT_X))

extern struct flag_struct regflags __asm__("regflags");

static inline int cctrue(int cc)
{
	unsigned int nzcv = regflags.nzcv;
	switch (cc) {
	case 0: return 1;                       /* T */
	case 1: return 0;                       /* F */
	case 2: return (nzcv & (FLAGVAL_C | FLAGVAL_Z)) == 0; /* !GET_CFLG && !GET_ZFLG;  HI */
	case 3: return (nzcv & (FLAGVAL_C | FLAGVAL_Z)) != 0; /* GET_CFLG || GET_ZFLG;    LS */
	case 4: return (nzcv & FLAGVAL_C) == 0; /* !GET_CFLG;               CC */
	case 5: return (nzcv & FLAGVAL_C) != 0; /* GET_CFLG;                CS */
	case 6: return (nzcv & FLAGVAL_Z) == 0; /* !GET_ZFLG;               NE */
	case 7: return (nzcv & FLAGVAL_Z) != 0; /* GET_ZFLG;                EQ */
	case 8: return (nzcv & FLAGVAL_V) == 0; /* !GET_VFLG;               VC */
	case 9: return (nzcv & FLAGVAL_V) != 0; /* GET_VFLG;                VS */
	case 10:return (nzcv & FLAGVAL_N) == 0; /* !GET_NFLG;               PL */
	case 11:return (nzcv & FLAGVAL_N) != 0; /* GET_NFLG;                MI */
	case 12:return (((nzcv << (FLAGBIT_N - FLAGBIT_V)) ^ nzcv) & FLAGVAL_N) == 0; /* GET_NFLG == GET_VFLG;             GE */
	case 13:return (((nzcv << (FLAGBIT_N - FLAGBIT_V)) ^ nzcv) & FLAGVAL_N) != 0; /* GET_NFLG != GET_VFLG;             LT */
	case 14: nzcv &= (FLAGVAL_N | FLAGVAL_Z | FLAGVAL_V);
		return (((nzcv << (FLAGBIT_N - FLAGBIT_V)) ^ nzcv) & (FLAGVAL_N | FLAGVAL_Z)) == 0; /* !GET_ZFLG && (GET_NFLG == GET_VFLG);  GT */
	case 15: nzcv &= (FLAGVAL_N | FLAGVAL_Z | FLAGVAL_V);
		return (((nzcv << (FLAGBIT_N - FLAGBIT_V)) ^ nzcv) & (FLAGVAL_N | FLAGVAL_Z)) != 0; /* GET_ZFLG || (GET_NFLG != GET_VFLG);   LE */
	}
	return 0;
}


#define optflag_testl(v) do {\
  __asm__ __volatile__ ("tst %[rv],%[rv]\n\t" \
                        "mrs %[nzcv],cpsr\n\t" \
                        "bic %[nzcv],#0x30000000\n\t" \
                        : [nzcv] "=r" (regflags.nzcv) \
                        : [rv] "r" (v) \
                        : "cc"); \
  } while(0)

#define optflag_addl(v, s, d) do { \
  __asm__ __volatile__ ("adds %[rv],%[rd],%[rs]\n\t" \
                        "mrs %[nzcv],cpsr\n\t" \
                        : [nzcv] "=r" (regflags.nzcv), [rv] "=r" (v) \
                        : [rs] "ri" (s), [rd] "1" (d) \
                        : "cc"); \
    COPY_CARRY(); \
    } while(0)

#define optflag_subl(v, s, d) do { \
  __asm__ __volatile__ ("subs %[rv],%[rd],%[rs]\n\t" \
                        "mrs %[nzcv],cpsr\n\t" \
                        "eor %[nzcv],#0x20000000\n\t" \
                        : [nzcv] "=r" (regflags.nzcv), [rv] "=r" (v) \
                        : [rs] "ri" (s), [rd] "1" (d) \
                        : "cc"); \
    COPY_CARRY(); \
    } while(0)

#define optflag_cmpl(s, d) do { \
  __asm__ __volatile__ ("cmp %[rd],%[rs]\n\t" \
                        "mrs %[nzcv],cpsr\n\t" \
                        "eor %[nzcv],#0x20000000\n\t" \
                        : [nzcv] "=r" (regflags.nzcv) \
                        : [rs] "ri" (s), [rd] "0" (d) \
                        : "cc"); \
  } while(0)

#if defined(ARMV6_ASSEMBLY)

// #pragma message "ARM/v6 Assembly optimized flags"

#define optflag_testw(v) do { \
  __asm__ __volatile__ ("sxth %[rv],%[rv]\n\t" \
                        "tst %[rv],%[rv]\n\t" \
                        "mrs %[nzcv],cpsr\n\t" \
                        "bic %[nzcv],#0x30000000\n\t" \
                        : [nzcv] "=r" (regflags.nzcv) \
                        : [rv] "0" (v) \
                        : "cc"); \
        }while(0)

#define optflag_testb(v) do {\
  __asm__ __volatile__ ("sxtb %[rv],%[rv]\n\t" \
                        "tst %[rv],%[rv]\n\t" \
                        "mrs %[nzcv],cpsr\n\t" \
                        "bic %[nzcv],#0x30000000\n\t" \
                        : [nzcv] "=r" (regflags.nzcv) \
                        : [rv] "0" (v) \
                        : "cc"); \
        }while(0)

#define optflag_addw(v, s, d) do { \
  __asm__ __volatile__ ("sxth %[rd],%[rd]\n\t" \
			"sxth %[rs],%[rs]\n\t" \
                        "adds %[rd],%[rd],%[rs]\n\t" \
                        "mrs %[nzcv],cpsr\n\t" \
                        : [nzcv] "=r" (regflags.nzcv), [rv] "=r" (v) \
                        : [rs] "ri" (s), [rd] "1" (d) \
                        : "cc"); \
    COPY_CARRY(); \
    } while(0)

#define optflag_addb(v, s, d) do { \
  __asm__ __volatile__ ("sxtb %[rd],%[rd]\n\t" \
			"sxtb %[rs],%[rs]\n\t" \
                        "adds %[rd],%[rd],%[rs]\n\t" \
                        "mrs %[nzcv],cpsr\n\t" \
                        : [nzcv] "=r" (regflags.nzcv), [rv] "=r" (v) \
                        : [rs] "ri" (s), [rd] "1" (d) \
                        : "cc"); \
    COPY_CARRY(); \
    } while(0)

#define optflag_subw(v, s, d) do { \
  __asm__ __volatile__ ("sxth %[rd],%[rd]\n\t" \
			"sxth %[rs],%[rs]\n\t" \
                        "subs %[rd],%[rd],%[rs]\n\t" \
                        "mrs %[nzcv],cpsr\n\t" \
                        "eor %[nzcv],#0x20000000\n\t" \
                        : [nzcv] "=r" (regflags.nzcv), [rv] "=r" (v) \
                        : [rs] "ri" (s), [rd] "1" (d) \
                        : "cc"); \
    COPY_CARRY(); \
    } while(0)

#define optflag_subb(v, s, d) do { \
  __asm__ __volatile__ ("sxtb %[rd],%[rd]\n\t" \
			"sxtb %[rs],%[rs]\n\t" \
                        "subs %[rd],%[rd],%[rs]\n\t" \
                        "mrs %[nzcv],cpsr\n\t" \
                        "eor %[nzcv],#0x20000000\n\t" \
                        : [nzcv] "=r" (regflags.nzcv), [rv] "=r" (v) \
                        : [rs] "ri" (s), [rd] "1" (d) \
                        : "cc"); \
    COPY_CARRY(); \
    } while(0)

#define optflag_cmpw(s, d) do { \
  __asm__ __volatile__ ("sxth %[rd],%[rd]\n\t" \
			"sxth %[rs],%[rs]\n\t" \
                        "cmp %[rd],%[rs]\n\t" \
                        "mrs %[nzcv],cpsr\n\t" \
                        "eor %[nzcv],#0x20000000\n\t" \
                        : [nzcv] "=r" (regflags.nzcv) \
                        : [rs] "ri" (s), [rd] "0" (d) \
                        : "cc"); \
  } while(0)

#define optflag_cmpb(s, d) do { \
  __asm__ __volatile__ ("sxtb %[rd],%[rd]\n\t" \
			"sxtb %[rs],%[rs]\n\t" \
                        "cmp %[rd],%[rs]\n\t" \
                        "mrs %[nzcv],cpsr\n\t" \
                        "eor %[nzcv],#0x20000000\n\t" \
                        : [nzcv] "=r" (regflags.nzcv) \
                        : [rs] "ri" (s), [rd] "0" (d) \
                        : "cc"); \
  } while(0)

#else

// #pragma message "ARM/generic Assembly optimized flags"

#define optflag_testw(v) do { \
  __asm__ __volatile__ ("lsl %[rv],%[rv],#16\n\t" \
                        "tst %[rv],%[rv]\n\t" \
                        "mrs %[nzcv],cpsr\n\t" \
                        "bic %[nzcv],#0x30000000\n\t" \
                        : [nzcv] "=r" (regflags.nzcv) \
                        : [rv] "0" (v) \
                        : "cc"); \
	}while(0)

#define optflag_testb(v) do {\
  __asm__ __volatile__ ("lsl %[rv],%[rv],#24\n\t" \
                        "tst %[rv],%[rv]\n\t" \
                        "mrs %[nzcv],cpsr\n\t" \
                        "bic %[nzcv],#0x30000000\n\t" \
                        : [nzcv] "=r" (regflags.nzcv) \
                        : [rv] "0" (v) \
                        : "cc"); \
	}while(0)

#define optflag_addw(v, s, d) do { \
  __asm__ __volatile__ ("lsl %[rd],%[rd],#16\n\t" \
                        "adds %[rd],%[rd],%[rs],lsl #16\n\t" \
                        "mrs %[nzcv],cpsr\n\t" \
                        "lsr %[rv],%[rd],#16\n\t" \
                        : [nzcv] "=r" (regflags.nzcv), [rv] "=r" (v) \
                        : [rs] "ri" (s), [rd] "1" (d) \
                        : "cc"); \
    COPY_CARRY(); \
    } while(0)

#define optflag_addb(v, s, d) do { \
  __asm__ __volatile__ ("lsl %[rd],%[rd],#24\n\t" \
                        "adds %[rd],%[rd],%[rs],lsl #24\n\t" \
                        "mrs %[nzcv],cpsr\n\t" \
                        "lsr %[rv],%[rd],#24\n\t" \
                        : [nzcv] "=r" (regflags.nzcv), [rv] "=r" (v) \
                        : [rs] "ri" (s), [rd] "1" (d) \
                        : "cc"); \
    COPY_CARRY(); \
    } while(0)

#define optflag_subw(v, s, d) do { \
  __asm__ __volatile__ ("lsl %[rd],%[rd],#16\n\t" \
                        "subs %[rd],%[rd],%[rs],lsl #16\n\t" \
                        "mrs %[nzcv],cpsr\n\t" \
                        "eor %[nzcv],#0x20000000\n\t" \
                        "lsr %[rv],%[rd],#16\n\t" \
                        : [nzcv] "=r" (regflags.nzcv), [rv] "=r" (v) \
                        : [rs] "ri" (s), [rd] "1" (d) \
                        : "cc"); \
    COPY_CARRY(); \
    } while(0)

#define optflag_subb(v, s, d) do { \
  __asm__ __volatile__ ("lsl %[rd],%[rd],#24\n\t" \
                        "subs %[rd],%[rd],%[rs],lsl #24\n\t" \
                        "mrs %[nzcv],cpsr\n\t" \
                        "eor %[nzcv],#0x20000000\n\t" \
                        "lsr %[rv],%[rd],#24\n\t" \
                        : [nzcv] "=r" (regflags.nzcv), [rv] "=r" (v) \
                        : [rs] "ri" (s), [rd] "1" (d) \
                        : "cc"); \
    COPY_CARRY(); \
    } while(0)

#define optflag_cmpw(s, d) do { \
  __asm__ __volatile__ ("lsl %[rd],%[rd],#16\n\t" \
                        "cmp %[rd],%[rs],lsl #16\n\t" \
                        "mrs %[nzcv],cpsr\n\t" \
                        "eor %[nzcv],#0x20000000\n\t" \
                        : [nzcv] "=r" (regflags.nzcv) \
                        : [rs] "ri" (s), [rd] "0" (d) \
                        : "cc"); \
  } while(0)

#define optflag_cmpb(s, d) do { \
  __asm__ __volatile__ ("lsl %[rd],%[rd],#24\n\t" \
                        "cmp %[rd],%[rs],lsl #24\n\t" \
                        "mrs %[nzcv],cpsr\n\t" \
                        "eor %[nzcv],#0x20000000\n\t" \
                        : [nzcv] "=r" (regflags.nzcv) \
                        : [rs] "ri" (s), [rd] "0" (d) \
                        : "cc"); \
  } while(0)

#endif

#else

/*
 * Machine independent structure for holding the 68k CCR flags
 */
struct flag_struct {
    unsigned int c;
    unsigned int z;
    unsigned int n;
    unsigned int v; 
    unsigned int x;
};

extern struct flag_struct regflags;

#define ZFLG (regflags.z)
#define NFLG (regflags.n)
#define CFLG (regflags.c)
#define VFLG (regflags.v)
#define XFLG (regflags.x)

#define SET_CFLG(x) (CFLG = (x))
#define SET_NFLG(x) (NFLG = (x))
#define SET_VFLG(x) (VFLG = (x))
#define SET_ZFLG(x) (ZFLG = (x))
#define SET_XFLG(x) (XFLG = (x))

#define GET_CFLG() CFLG
#define GET_NFLG() NFLG
#define GET_VFLG() VFLG
#define GET_ZFLG() ZFLG
#define GET_XFLG() XFLG

#define CLEAR_CZNV() do { \
 SET_CFLG (0); \
 SET_ZFLG (0); \
 SET_NFLG (0); \
 SET_VFLG (0); \
} while (0)

#define COPY_CARRY() (SET_XFLG (GET_CFLG ()))

/*
 * Test CCR condition
 */
static inline int cctrue(const int cc)
{
    switch(cc){
     case 0: return 1;                       /* T */
     case 1: return 0;                       /* F */
     case 2: return !CFLG && !ZFLG;          /* HI */
     case 3: return CFLG || ZFLG;            /* LS */
     case 4: return !CFLG;                   /* CC */
     case 5: return CFLG;                    /* CS */
     case 6: return !ZFLG;                   /* NE */
     case 7: return ZFLG;                    /* EQ */
     case 8: return !VFLG;                   /* VC */
     case 9: return VFLG;                    /* VS */
     case 10:return !NFLG;                   /* PL */
     case 11:return NFLG;                    /* MI */
     case 12:return NFLG == VFLG;            /* GE */
     case 13:return NFLG != VFLG;            /* LT */
     case 14:return !ZFLG && (NFLG == VFLG); /* GT */
     case 15:return ZFLG || (NFLG != VFLG);  /* LE */
    }
    return 0;
}

#endif /* OPTIMIZED_FLAGS */

#endif /* M68K_FLAGS_H */
