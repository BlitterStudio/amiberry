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

#if (defined(CPU_i386) && defined(X86_ASSEMBLY)) || (defined(CPU_x86_64) && defined(X86_64_ASSEMBLY))

#ifndef REGS_DEFINED

 /*
  * Machine dependent structure for holding the 68k CCR flags
  */
struct flag_struct {
    unsigned int cznv;
    unsigned int x;
};


/*
 * The bits in the cznv field in the above structure are assigned to
 * allow the easy mirroring of the x86 condition flags. (For example,
 * from the AX register - the x86 overflow flag can be copied to AL
 * with a setto %AL instr and the other flags copied to AH with an
 * lahf instr).
 *
 * The 68k CZNV flags are thus assinged in cznv as:
 *
 * <--AL-->  <--AH-->
 * 76543210  FEDCBA98 --------- ---------
 * xxxxxxxV  NZxxxxxC xxxxxxxxx xxxxxxxxx
 */

#define FLAGBIT_N	15
#define FLAGBIT_Z	14
#define FLAGBIT_C	8
#define FLAGBIT_V	0
#define FLAGBIT_X	8

#define FLAGVAL_N	(1 << FLAGBIT_N)
#define FLAGVAL_Z 	(1 << FLAGBIT_Z)
#define FLAGVAL_C	(1 << FLAGBIT_C)
#define FLAGVAL_V	(1 << FLAGBIT_V)
#define FLAGVAL_X	(1 << FLAGBIT_X)

#else /* REGS_DEFINED */

#define SET_ZFLG(y)	(regs.ccrflags.cznv = (regs.ccrflags.cznv & ~FLAGVAL_Z) | (((y) ? 1 : 0) << FLAGBIT_Z))
#define SET_CFLG(y)	(regs.ccrflags.cznv = (regs.ccrflags.cznv & ~FLAGVAL_C) | (((y) ? 1 : 0) << FLAGBIT_C))
#define SET_VFLG(y)	(regs.ccrflags.cznv = (regs.ccrflags.cznv & ~FLAGVAL_V) | (((y) ? 1 : 0) << FLAGBIT_V))
#define SET_NFLG(y)	(regs.ccrflags.cznv = (regs.ccrflags.cznv & ~FLAGVAL_N) | (((y) ? 1 : 0) << FLAGBIT_N))
#define SET_XFLG(y)	(regs.ccrflags.x    = ((y) ? 1 : 0) << FLAGBIT_X)

#define GET_ZFLG()	((regs.ccrflags.cznv >> FLAGBIT_Z) & 1)
#define GET_CFLG()	((regs.ccrflags.cznv >> FLAGBIT_C) & 1)
#define GET_VFLG()	((regs.ccrflags.cznv >> FLAGBIT_V) & 1)
#define GET_NFLG()	((regs.ccrflags.cznv >> FLAGBIT_N) & 1)
#define GET_XFLG()	((regs.ccrflags.x    >> FLAGBIT_X) & 1)

#define CLEAR_CZNV()	(regs.ccrflags.cznv  = 0)
#define GET_CZNV()	(regs.ccrflags.cznv)
#define IOR_CZNV(X)	(regs.ccrflags.cznv |= (X))
#define SET_CZNV(X)	(regs.ccrflags.cznv  = (X))

#define COPY_CARRY() (regs.ccrflags.x = regs.ccrflags.cznv)


/*
 * Test CCR condition
 */
STATIC_INLINE int cctrue (int cc)
{
    uae_u32 cznv = regs.ccrflags.cznv;

    switch (cc) {
	case 0:  return 1;								/*				T  */
	case 1:  return 0;								/*				F  */
	case 2:  return (cznv & (FLAGVAL_C | FLAGVAL_Z)) == 0;				/* !CFLG && !ZFLG		HI */
	case 3:  return (cznv & (FLAGVAL_C | FLAGVAL_Z)) != 0;				/*  CFLG || ZFLG		LS */
	case 4:  return (cznv & FLAGVAL_C) == 0;					/* !CFLG			CC */
	case 5:  return (cznv & FLAGVAL_C) != 0;					/*  CFLG			CS */
	case 6:  return (cznv & FLAGVAL_Z) == 0;					/* !ZFLG			NE */
	case 7:  return (cznv & FLAGVAL_Z) != 0;					/*  ZFLG			EQ */
	case 8:  return (cznv & FLAGVAL_V) == 0;					/* !VFLG			VC */
	case 9:  return (cznv & FLAGVAL_V) != 0;					/*  VFLG			VS */
	case 10: return (cznv & FLAGVAL_N) == 0;					/* !NFLG			PL */
	case 11: return (cznv & FLAGVAL_N) != 0;					/*  NFLG			MI */
	case 12: return (((cznv << (FLAGBIT_N - FLAGBIT_V)) ^ cznv) & FLAGVAL_N) == 0;	/*  NFLG == VFLG		GE */
	case 13: return (((cznv << (FLAGBIT_N - FLAGBIT_V)) ^ cznv) & FLAGVAL_N) != 0;	/*  NFLG != VFLG		LT */
	case 14: cznv &= (FLAGVAL_N | FLAGVAL_Z | FLAGVAL_V);				/* ZFLG && (NFLG == VFLG)	GT */
		 return (((cznv << (FLAGBIT_N - FLAGBIT_V)) ^ cznv) & (FLAGVAL_N | FLAGVAL_Z)) == 0;
	case 15: cznv &= (FLAGVAL_N | FLAGVAL_Z | FLAGVAL_V);				/* ZFLG && (NFLG != VFLG)	LE */
		 return (((cznv << (FLAGBIT_N - FLAGBIT_V)) ^ cznv) & (FLAGVAL_N | FLAGVAL_Z)) != 0;
    }
    return 0;
}

#endif /* REGS_DEFINED */

#elif defined(CPU_arm) && defined(ARMV6_ASSEMBLY)

#ifndef REGS_DEFINED

struct flag_struct {
	uae_u32 nzcv;
	uae_u32 x;
};

#define FLAGVAL_Q       0x08000000
#define FLAGVAL_V       0x10000000
#define FLAGVAL_C       0x20000000
#define FLAGVAL_Z       0x40000000
#define FLAGVAL_N       0x80000000

#else /* REGS_DEFINED */

#define SET_NFLG(y)     (regs.ccrflags.nzcv = (regs.ccrflags.nzcv & ~0x80000000) | (((y) ? 1 : 0) << 31))
#define SET_ZFLG(y)     (regs.ccrflags.nzcv = (regs.ccrflags.nzcv & ~0x40000000) | (((y) ? 1 : 0) << 30))
#define SET_CFLG(y)     (regs.ccrflags.nzcv = (regs.ccrflags.nzcv & ~0x20000000) | (((y) ? 1 : 0) << 29))
#define SET_VFLG(y)     (regs.ccrflags.nzcv = (regs.ccrflags.nzcv & ~0x10000000) | (((y) ? 1 : 0) << 28))

#define SET_XFLG(y)	(regs.ccrflags.x = ((y) ? 1 : 0))

#define GET_NFLG()  ((regs.ccrflags.nzcv >> 31) & 1)
#define GET_ZFLG()	((regs.ccrflags.nzcv >> 30) & 1)
#define GET_CFLG()	((regs.ccrflags.nzcv >> 29) & 1)
#define GET_VFLG()	((regs.ccrflags.nzcv >> 28) & 1)
#define GET_XFLG()	(regs.ccrflags.x & 1)

#define CLEAR_CZNV()    (regs.ccrflags.nzcv = 0)
#define GET_CZNV()      (regs.ccrflags.nzcv)
#define IOR_CZNV(X)     (regs.ccrflags.nzcv |= (X))
#define SET_CZNV(X)     (regs.ccrflags.nzcv = (X))

#define COPY_CARRY()      (regs.ccrflags.x = ((regs.ccrflags.nzcv >> 29) & 1))

STATIC_INLINE int cctrue(int cc)
{
    unsigned int nzcv = regs.ccrflags.nzcv;
    switch(cc){
     case 0: return 1;                       /* T */
     case 1: return 0;                       /* F */
     case 2: return (nzcv & 0x60000000) == 0; /* !GET_CFLG && !GET_ZFLG;  HI */
     case 3: return (nzcv & 0x60000000) != 0; /* GET_CFLG || GET_ZFLG;    LS */
     case 4: return (nzcv & 0x20000000) == 0; /* !GET_CFLG;               CC */
     case 5: return (nzcv & 0x20000000) != 0; /* GET_CFLG;                CS */
     case 6: return (nzcv & 0x40000000) == 0; /* !GET_ZFLG;               NE */
     case 7: return (nzcv & 0x40000000) != 0; /* GET_ZFLG;                EQ */
     case 8: return (nzcv & 0x10000000) == 0; /* !GET_VFLG;               VC */
     case 9: return (nzcv & 0x10000000) != 0; /* GET_VFLG;                VS */
     case 10:return (nzcv & 0x80000000) == 0; /* !GET_NFLG;               PL */
     case 11:return (nzcv & 0x80000000) != 0; /* GET_NFLG;                MI */
     case 12:return (((nzcv << 3) ^ nzcv) & 0x80000000) == 0; /* GET_NFLG == GET_VFLG;             GE */
     case 13:return (((nzcv << 3) ^ nzcv) & 0x80000000) != 0; /* GET_NFLG != GET_VFLG;             LT */
     case 14:
        nzcv &= 0xd0000000;
        return (((nzcv << 3) ^ nzcv) & 0xc0000000) == 0; /* !GET_ZFLG && (GET_NFLG == GET_VFLG);  GT */
     case 15:
        nzcv &= 0xd0000000;
        return (((nzcv << 3) ^ nzcv) & 0xc0000000) != 0; /* GET_ZFLG || (GET_NFLG != GET_VFLG);   LE */
    }
    return 0;
}

#endif /* REGS_DEFINED */

#else

#ifndef REGS_DEFINED

struct flag_struct {
    unsigned int c;
    unsigned int z;
    unsigned int n;
    unsigned int v; 
    unsigned int x;
};

#else /* REGS_DEFINED */

#define ZFLG (regs.ccrflags.z)
#define NFLG (regs.ccrflags.n)
#define CFLG (regs.ccrflags.c)
#define VFLG (regs.ccrflags.v)
#define XFLG (regs.ccrflags.x)

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

#define COPY_CARRY() (regs.ccrflags.x = regs.ccrflags.c)

STATIC_INLINE int cctrue(const int cc)
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

#endif /* REGS_DEFINED */

#endif
