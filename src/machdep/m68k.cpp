
#include "sysconfig.h"
#include "sysdeps.h"
#include "m68k.h"

/*
 * Test CCR condition
 */

#ifndef SAHF_SETO_PROFITABLE

int cctrue(int cc)
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
    return 0;
}

#else /* !SAHF_SETO_PROFITABLE */

int cctrue(int cc)
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
    return 0;
}

#endif
