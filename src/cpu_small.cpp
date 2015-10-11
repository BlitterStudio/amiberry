#include "sysconfig.h"
#include "sysdeps.h"

#include "cpu_small.h"

#include "readcpu.h"

int xmovem_index1[256];
int xmovem_index2[256];
int xmovem_next[256];

struct xcpu xregs;

const int xareg_byteinc[] = { 1,1,1,1,1,1,1,2 };
const int ximm8_table[] = { 8,1,2,3,4,5,6,7 };

uae_u32 xnext_iword (void)
{
    uae_u32 r = xget_iword (0);
    xm68k_incpc (2);
    return r;
}
uae_u32 xnext_ilong (void)
{
    uae_u32 r = xget_ilong (0);
    xm68k_incpc (4);
    return r;
}

uae_u32 xget_ibyte (int offset)
{
    return xget_byte (xm68k_getpc() + offset);
}
uae_u32 xget_iword (int offset)
{
    return xget_word (xm68k_getpc() + offset);
}
uae_u32 xget_ilong (int offset)
{
    return xget_long (xm68k_getpc() + offset);
}

void xm68k_incpc (int offset)
{
    xregs.pc += offset;
}
uaecptr xm68k_getpc(void)
{
    return xregs.pc;
}
void xm68k_setpc (uaecptr pc)
{
    xregs.pc = pc;
}

uae_u32 xget_disp_ea_020 (uae_u32 base, uae_u32 dp)
{
    int reg = (dp >> 12) & 15;
    uae_s32 regd = xregs.regs[reg];
    if ((dp & 0x800) == 0)
	regd = (uae_s32)(uae_s16)regd;
    regd <<= (dp >> 9) & 3;
    if (dp & 0x100) {
	uae_s32 outer = 0;
	if (dp & 0x80) base = 0;
	if (dp & 0x40) regd = 0;

	if ((dp & 0x30) == 0x20) base += (uae_s32)(uae_s16)xnext_iword();
	if ((dp & 0x30) == 0x30) base += xnext_ilong();

	if ((dp & 0x3) == 0x2) outer = (uae_s32)(uae_s16)xnext_iword();
	if ((dp & 0x3) == 0x3) outer = xnext_ilong();

	if ((dp & 0x4) == 0) base += regd;
	if (dp & 0x3) base = xget_long (base);
	if (dp & 0x4) base += regd;

	return base + outer;
    } else {
	return base + (uae_s32)((uae_s8)dp) + regd;
    }
}

uae_u32 xget_disp_ea_000 (uae_u32 base, uae_u32 dp)
{
    int reg = (dp >> 12) & 15;
    uae_s32 regd = xregs.regs[reg];
    if ((dp & 0x800) == 0)
	regd = (uae_s32)(uae_s16)regd;
    return base + (uae_s8)dp + regd;

}

/*
 * Test CCR condition
 */
int xcctrue (int cc)
{
    uae_u32 cznv = xregs.cznv;

    switch (cc) {
	case 0:  return 1;								/*				T  */
	case 1:  return 0;								/*				F  */
	case 2:  return (cznv & (XFLAGVAL_C | XFLAGVAL_Z)) == 0;				/* !CFLG && !ZFLG		HI */
	case 3:  return (cznv & (XFLAGVAL_C | XFLAGVAL_Z)) != 0;				/*  CFLG || ZFLG		LS */
	case 4:  return (cznv & XFLAGVAL_C) == 0;					/* !CFLG			CC */
	case 5:  return (cznv & XFLAGVAL_C) != 0;					/*  CFLG			CS */
	case 6:  return (cznv & XFLAGVAL_Z) == 0;					/* !ZFLG			NE */
	case 7:  return (cznv & XFLAGVAL_Z) != 0;					/*  ZFLG			EQ */
	case 8:  return (cznv & XFLAGVAL_V) == 0;					/* !VFLG			VC */
	case 9:  return (cznv & XFLAGVAL_V) != 0;					/*  VFLG			VS */
	case 10: return (cznv & XFLAGVAL_N) == 0;					/* !NFLG			PL */
	case 11: return (cznv & XFLAGVAL_N) != 0;					/*  NFLG			MI */
	case 12: return (((cznv << (XFLAGBIT_N - XFLAGBIT_V)) ^ cznv) & XFLAGVAL_N) == 0;	/*  NFLG == VFLG		GE */
	case 13: return (((cznv << (XFLAGBIT_N - XFLAGBIT_V)) ^ cznv) & XFLAGVAL_N) != 0;	/*  NFLG != VFLG		LT */
	case 14: cznv &= (XFLAGVAL_N | XFLAGVAL_Z | XFLAGVAL_V);				/* ZFLG && (NFLG == VFLG)	GT */
		 return (((cznv << (XFLAGBIT_N - XFLAGBIT_V)) ^ cznv) & (XFLAGVAL_N | XFLAGVAL_Z)) == 0;
	case 15: cznv &= (XFLAGVAL_N | XFLAGVAL_Z | XFLAGVAL_V);				/* ZFLG && (NFLG != VFLG)	LE */
		 return (((cznv << (XFLAGBIT_N - XFLAGBIT_V)) ^ cznv) & (XFLAGVAL_N | XFLAGVAL_Z)) != 0;
    }
    return 0;
}

xcpuop_func *xcpufunctbl[65536];

void init_cpu_small(void)
{
    struct xcputbl *tbl = xop_smalltbl_0;
    int opcode, i;
    int lvl = 5, opcnt;

    for (opcode = 0; opcode < 65536; opcode++)
	xcpufunctbl[opcode] = xop_illg;
    for (i = 0; tbl[i].handler != NULL; i++)
	xcpufunctbl[tbl[i].opcode] = tbl[i].handler;

    opcnt = 0;
    for (opcode = 0; opcode < 65536; opcode++) {
	xcpuop_func *f;

	if (table68k[opcode].mnemo == i_ILLG || table68k[opcode].clev > lvl)
	    continue;

	if (table68k[opcode].handler != -1) {
	    f = xcpufunctbl[table68k[opcode].handler];
	    xcpufunctbl[opcode] = f;
	    opcnt++;
	}
    }
    write_log ("MiniCPU initialized, %d opcodes\n", opcnt);
}
