#include "sysconfig.h"
#include "sysdeps.h"

typedef void xcpuop_func (uae_u32);

struct xcputbl {
    xcpuop_func *handler;
    uae_u16 opcode;
};

struct xcpu
{
    uae_u32 regs[16];
    uae_u32 cznv;
    uae_u32 x;
    uae_u32 pc;
};

extern struct xcpu xregs;

extern struct xcputbl xop_smalltbl_0[];

extern void init_cpu_small(void);
extern xcpuop_func *xcpufunctbl[65536];
extern void xop_illg (uae_u32);

extern uae_u32 xget_ibyte(int);
extern uae_u32 xget_iword(int);
extern uae_u32 xget_ilong(int);

extern uae_u32 xget_byte(uaecptr);
extern uae_u32 xget_word(uaecptr);
extern uae_u32 xget_long(uaecptr);

extern void xput_byte(uaecptr, uae_u32);
extern void xput_word(uaecptr, uae_u32);
extern void xput_long(uaecptr, uae_u32);

extern uae_u32 xnext_iword (void);
extern uae_u32 xnext_ilong (void);

extern void xm68k_incpc(int);
extern uaecptr xm68k_getpc(void);
extern void xm68k_setpc(uaecptr);

#define xm68k_dreg(num) (xregs.regs[(num)])
#define xm68k_areg(num) (xregs.regs[(num + 8)])

extern uae_u32 xget_disp_ea_020 (uae_u32 base, uae_u32 dp);
extern uae_u32 xget_disp_ea_000 (uae_u32 base, uae_u32 dp);

extern int xcctrue (int cc);

extern const int xareg_byteinc[];
extern const int ximm8_table[];

extern int xmovem_index1[256];
extern int xmovem_index2[256];
extern int xmovem_next[256];

#define XFLAGBIT_N	15
#define XFLAGBIT_Z	14
#define XFLAGBIT_C	8
#define XFLAGBIT_V	0
#define XFLAGBIT_X	8

#define XFLAGVAL_N	(1 << XFLAGBIT_N)
#define XFLAGVAL_Z 	(1 << XFLAGBIT_Z)
#define XFLAGVAL_C	(1 << XFLAGBIT_C)
#define XFLAGVAL_V	(1 << XFLAGBIT_V)
#define XFLAGVAL_X	(1 << XFLAGBIT_X)

#define XSET_ZFLG(y)	(xregs.cznv = (xregs.cznv & ~XFLAGVAL_Z) | (((y) ? 1 : 0) << XFLAGBIT_Z))
#define XSET_CFLG(y)	(xregs.cznv = (xregs.cznv & ~XFLAGVAL_C) | (((y) ? 1 : 0) << XFLAGBIT_C))
#define XSET_VFLG(y)	(xregs.cznv = (xregs.cznv & ~XFLAGVAL_V) | (((y) ? 1 : 0) << XFLAGBIT_V))
#define XSET_NFLG(y)	(xregs.cznv = (xregs.cznv & ~XFLAGVAL_N) | (((y) ? 1 : 0) << XFLAGBIT_N))
#define XSET_XFLG(y)	(xregs.x    = ((y) ? 1 : 0) << XFLAGBIT_X)

#define XGET_ZFLG	((xregs.cznv >> XFLAGBIT_Z) & 1)
#define XGET_CFLG	((xregs.cznv >> XFLAGBIT_C) & 1)
#define XGET_VFLG	((xregs.cznv >> XFLAGBIT_V) & 1)
#define XGET_NFLG	((xregs.cznv >> XFLAGBIT_N) & 1)
#define XGET_XFLG	((xregs.x    >> XFLAGBIT_X) & 1)

#define XCLEAR_CZNV	(xregs.cznv  = 0)
#define XGET_CZNV	(xregs.cznv)
#define XIOR_CZNV(X)	(xregs.cznv |= (X))
#define XSET_CZNV(X)	(xregs.cznv  = (X))

#define XCOPY_CARRY	(xregs.x = xregs.cznv)

