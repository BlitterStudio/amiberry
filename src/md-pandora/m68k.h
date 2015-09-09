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

#define SET_ZFLG(flags, y)	((flags)->cznv = ((flags)->cznv & ~FLAGVAL_Z) | (((y) ? 1 : 0) << FLAGBIT_Z))
#define SET_CFLG(flags, y)	((flags)->cznv = ((flags)->cznv & ~FLAGVAL_C) | (((y) ? 1 : 0) << FLAGBIT_C))
#define SET_VFLG(flags, y)	((flags)->cznv = ((flags)->cznv & ~FLAGVAL_V) | (((y) ? 1 : 0) << FLAGBIT_V))
#define SET_NFLG(flags, y)	((flags)->cznv = ((flags)->cznv & ~FLAGVAL_N) | (((y) ? 1 : 0) << FLAGBIT_N))
#define SET_XFLG(flags, y)	((flags)->x    = ((y) ? 1 : 0) << FLAGBIT_X)

#define GET_ZFLG(flags)		(((flags)->cznv >> FLAGBIT_Z) & 1)
#define GET_CFLG(flags)		(((flags)->cznv >> FLAGBIT_C) & 1)
#define GET_VFLG(flags)		(((flags)->cznv >> FLAGBIT_V) & 1)
#define GET_NFLG(flags)		(((flags)->cznv >> FLAGBIT_N) & 1)
#define GET_XFLG(flags)		(((flags)->x    >> FLAGBIT_X) & 1)

#define CLEAR_CZNV(flags)	((flags)->cznv  = 0)
#define GET_CZNV(flags)		((flags)->cznv)
#define IOR_CZNV(flags, X)	((flags)->cznv |= (X))
#define SET_CZNV(flags, X)	((flags)->cznv  = (X))

#define COPY_CARRY(flags)	((flags)->x = (flags)->cznv)


/*
 * Test CCR condition
 */
STATIC_INLINE int cctrue (struct flag_struct *flags, int cc)
{
    uae_u32 cznv = flags->cznv;

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
    abort ();
    return 0;
}

#elif defined(CPU_arm) && defined(ARM_ASSEMBLY)

struct flag_struct {
	uae_u32 nzcv;
	uae_u32 x;
};

#define FLAGVAL_Q       0x08000000
#define FLAGVAL_V       0x10000000
#define FLAGVAL_C       0x20000000
#define FLAGVAL_Z       0x40000000
#define FLAGVAL_N       0x80000000

#define SET_NFLG(flags, y)     ((flags)->nzcv = ((flags)->nzcv & ~0x80000000) | (((y) ? 1 : 0) << 31))
#define SET_ZFLG(flags, y)     ((flags)->nzcv = ((flags)->nzcv & ~0x40000000) | (((y) ? 1 : 0) << 30))
#define SET_CFLG(flags, y)     ((flags)->nzcv = ((flags)->nzcv & ~0x20000000) | (((y) ? 1 : 0) << 29))
#define SET_VFLG(flags, y)     ((flags)->nzcv = ((flags)->nzcv & ~0x10000000) | (((y) ? 1 : 0) << 28))

#define SET_XFLG(flags, y)	((flags)->x = ((y) ? 1 : 0))

#define GET_NFLG(flags) (((flags)->nzcv >> 31) & 1)
#define GET_ZFLG(flags)	(((flags)->nzcv >> 30) & 1)
#define GET_CFLG(flags)	(((flags)->nzcv >> 29) & 1)
#define GET_VFLG(flags)	(((flags)->nzcv >> 28) & 1)
#define GET_XFLG(flags)	((flags)->x & 1)

#define CLEAR_CZNV(flags)      ((flags)->nzcv = 0)
#define GET_CZNV(flags)        ((flags)->nzcv)
#define IOR_CZNV(flags,X)     ((flags)->nzcv |= (X))
#define SET_CZNV(flags,X)     ((flags)->nzcv = (X))

#define COPY_CARRY(flags)      ((flags)->x = ((flags)->nzcv)>>29)

extern struct flag_struct regflags __asm__ ("regflags");

static inline int cctrue(struct flag_struct *flags, int cc)
{
    unsigned int nzcv = flags->nzcv;
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
    COPY_CARRY; \
    } while(0)

#define optflag_subl(v, s, d) do { \
  __asm__ __volatile__ ("subs %[rv],%[rd],%[rs]\n\t" \
                        "mrs %[nzcv],cpsr\n\t" \
                        "eor %[nzcv],#0x20000000\n\t" \
                        : [nzcv] "=r" (regflags.nzcv), [rv] "=r" (v) \
                        : [rs] "ri" (s), [rd] "1" (d) \
                        : "cc"); \
    COPY_CARRY; \
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
    COPY_CARRY; \
    } while(0)

#define optflag_addb(v, s, d) do { \
  __asm__ __volatile__ ("sxtb %[rd],%[rd]\n\t" \
			"sxtb %[rs],%[rs]\n\t" \
                        "adds %[rd],%[rd],%[rs]\n\t" \
                        "mrs %[nzcv],cpsr\n\t" \
                        : [nzcv] "=r" (regflags.nzcv), [rv] "=r" (v) \
                        : [rs] "ri" (s), [rd] "1" (d) \
                        : "cc"); \
    COPY_CARRY; \
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
    COPY_CARRY; \
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
    COPY_CARRY; \
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
    COPY_CARRY; \
    } while(0)

#define optflag_addb(v, s, d) do { \
  __asm__ __volatile__ ("lsl %[rd],%[rd],#24\n\t" \
                        "adds %[rd],%[rd],%[rs],lsl #24\n\t" \
                        "mrs %[nzcv],cpsr\n\t" \
                        "lsr %[rv],%[rd],#24\n\t" \
                        : [nzcv] "=r" (regflags.nzcv), [rv] "=r" (v) \
                        : [rs] "ri" (s), [rd] "1" (d) \
                        : "cc"); \
    COPY_CARRY; \
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
    COPY_CARRY; \
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
    COPY_CARRY; \
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

#define SET_CFLG(flags, x) (CFLG(flags) = (x))
#define SET_NFLG(flags, x) (NFLG(flags) = (x))
#define SET_VFLG(flags, x) (VFLG(flags) = (x))
#define SET_ZFLG(flags, x) (ZFLG(flags) = (x))
#define SET_XFLG(flags, x) (XFLG(flags) = (x))

#define GET_CFLG(flags) CFLG(flags)
#define GET_NFLG(flags) NFLG(flags)
#define GET_VFLG(flags) VFLG(flags)
#define GET_ZFLG(flags) ZFLG)flags)
#define GET_XFLG(flags) XFLG(flags)

#define CLEAR_CZNV(flags) do { \
 SET_CFLG (flags, 0); \
 SET_ZFLG (flags, 0); \
 SET_NFLG (flags, 0); \
 SET_VFLG (flags, 0); \
} while (0)

#define COPY_CARRY(flags) ((flags)->x = (flags)->cznv)

static inline int cctrue(struct flag_struct *flags, const int cc)
{
    switch(flags->cc){
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
