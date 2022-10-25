/*
  * UAE - The Un*x Amiga Emulator
  *
  * MC68000 emulation - machine dependent bits
  *
  * Copyright 1996 Bernd Schmidt
  * Copyright 2004-2005 Richard Drummond
  */

 /*
  * Machine dependent structure for holding the 68k CCR flags
  */

extern int cctrue(int cc);

#ifndef SAHF_SETO_PROFITABLE

struct flag_struct {
#if defined(CPU_x86_64)
    uae_u64 cznv;
    uae_u64 x;
#else
    uae_u32 cznv;
    uae_u32 x;
#endif
};

extern struct flag_struct regflags;

/*
 * The bits in the cznv field in the above structure are assigned to
 * allow the easy mirroring of the x86 rFLAGS register.
 *
 * The 68k CZNV flags are thus assigned in cznv as:
 *
 * 76543210  FEDCBA98 --------- ---------
 * SZxxxxxC  xxxxVxxx xxxxxxxxx xxxxxxxxx
 */

#define FLAGBIT_N	7
#define FLAGBIT_Z	6
#define FLAGBIT_C	0
#define FLAGBIT_V	11
#define FLAGBIT_X	0

#define FLAGVAL_N	(1 << FLAGBIT_N)
#define FLAGVAL_Z   (1 << FLAGBIT_Z)
#define FLAGVAL_C	(1 << FLAGBIT_C)
#define FLAGVAL_V	(1 << FLAGBIT_V)
#define FLAGVAL_X	(1 << FLAGBIT_X)

#define SET_ZFLG(y)		(regflags.cznv = (regflags.cznv & ~FLAGVAL_Z) | (((y) & 1) << FLAGBIT_Z))
#define SET_CFLG(y)		(regflags.cznv = (regflags.cznv & ~FLAGVAL_C) | (((y) & 1) << FLAGBIT_C))
#define SET_VFLG(y)		(regflags.cznv = (regflags.cznv & ~FLAGVAL_V) | (((y) & 1) << FLAGBIT_V))
#define SET_NFLG(y)		(regflags.cznv = (regflags.cznv & ~FLAGVAL_N) | (((y) & 1) << FLAGBIT_N))
#define SET_XFLG(y)		(regflags.x    = ((y) & 1) << FLAGBIT_X)

#define GET_ZFLG()		((regflags.cznv >> FLAGBIT_Z) & 1)
#define GET_CFLG()		((regflags.cznv >> FLAGBIT_C) & 1)
#define GET_VFLG()		((regflags.cznv >> FLAGBIT_V) & 1)
#define GET_NFLG()		((regflags.cznv >> FLAGBIT_N) & 1)
#define GET_XFLG()		((regflags.x    >> FLAGBIT_X) & 1)

#define CLEAR_CZNV()	(regflags.cznv = 0)
#define GET_CZNV()		(regflags.cznv)
#define IOR_CZNV(X)		(regflags.cznv |= (X))
#define SET_CZNV(X)		(regflags.cznv  = (X))

#define COPY_CARRY() 	(regflags.x = regflags.cznv >> (FLAGBIT_C - FLAGBIT_X))

#else /* !SAHF_SETO_PROFITABLE */

  /*
   * Machine dependent structure for holding the 68k CCR flags
   */
struct flag_struct {
    uae_u32 cznv;
    uae_u32 x;
};

extern struct flag_struct regflags;

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

#endif
