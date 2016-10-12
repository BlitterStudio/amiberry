#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "include/memory.h"
#include "custom.h"
#include "newcpu.h"
#include "cpu_prefetch.h"
#include "cputbl.h"
#define CPUFUNC(x) x##_ff
#define SET_CFLG_ALWAYS(x) SET_CFLG(x)
#define SET_NFLG_ALWAYS(x) SET_NFLG(x)
#ifdef NOFLAGS
#include "noflags.h"
#endif

#if !defined(PART_1) && !defined(PART_2) && !defined(PART_3) && !defined(PART_4) && !defined(PART_5) && !defined(PART_6) && !defined(PART_7) && !defined(PART_8)
#define PART_1 1
#define PART_2 1
#define PART_3 1
#define PART_4 1
#define PART_5 1
#define PART_6 1
#define PART_7 1
#define PART_8 1
#endif

#ifdef PART_1
/* OR.B #<data>.B,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0000_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}	m68k_incpc (4);
return 8 * CYCLE_UNIT / 2;
}

/* OR.B #<data>.B,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0010_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* OR.B #<data>.B,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0018_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* OR.B #<data>.B,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0020_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* OR.B #<data>.B,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0028_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* OR.B #<data>.B,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0030_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (4));
{	uae_s8 dst = get_byte (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (6);
return 22 * CYCLE_UNIT / 2;
}

/* OR.B #<data>.B,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0038_4)(uae_u32 opcode)
{
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* OR.B #<data>.B,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0039_4)(uae_u32 opcode)
{
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_s8 dst = get_byte (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (8);
return 24 * CYCLE_UNIT / 2;
}

/* ORSR.B #<data>.W */
uae_u32 REGPARAM2 CPUFUNC(op_003c_4)(uae_u32 opcode)
{
{	MakeSR ();
{	uae_s16 src = get_diword (2);
	src &= 0xFF;
	m68k_incpc (4);
	regs.sr |= src;
	MakeFromSR ();
}}return 20 * CYCLE_UNIT / 2;
}

/* OR.W #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0040_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}	m68k_incpc (4);
return 8 * CYCLE_UNIT / 2;
}

/* OR.W #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0050_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* OR.W #<data>.W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0058_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) += 2;
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* OR.W #<data>.W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0060_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* OR.W #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0068_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_s16 dst = get_word (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* OR.W #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0070_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (4));
{	uae_s16 dst = get_word (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (6);
return 22 * CYCLE_UNIT / 2;
}

/* OR.W #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0078_4)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_s16 dst = get_word (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* OR.W #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0079_4)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_s16 dst = get_word (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (8);
return 24 * CYCLE_UNIT / 2;
}

/* ORSR.W #<data>.W */
uae_u32 REGPARAM2 CPUFUNC(op_007c_4)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	MakeSR ();
{	uae_s16 src = get_diword (2);
	m68k_incpc (4);
	regs.sr |= src;
	MakeFromSR ();
}}}return 20 * CYCLE_UNIT / 2;
}

/* OR.L #<data>.L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0080_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}	m68k_incpc (6);
return 16 * CYCLE_UNIT / 2;
}

/* OR.L #<data>.L,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0090_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (6);
return 28 * CYCLE_UNIT / 2;
}

/* OR.L #<data>.L,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0098_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) += 4;
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (6);
return 28 * CYCLE_UNIT / 2;
}

/* OR.L #<data>.L,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_00a0_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (6);
return 30 * CYCLE_UNIT / 2;
}

/* OR.L #<data>.L,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_00a8_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (6);
{	uae_s32 dst = get_long (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (8);
return 32 * CYCLE_UNIT / 2;
}

/* OR.L #<data>.L,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_00b0_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (6));
{	uae_s32 dst = get_long (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (8);
return 34 * CYCLE_UNIT / 2;
}

/* OR.L #<data>.L,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_00b8_4)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (6);
{	uae_s32 dst = get_long (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (8);
return 32 * CYCLE_UNIT / 2;
}

/* OR.L #<data>.L,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_00b9_4)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = get_dilong (6);
{	uae_s32 dst = get_long (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (10);
return 36 * CYCLE_UNIT / 2;
}

/* BTST.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0100_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src &= 31;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* MVPMR.W (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0108_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr memp = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_u16 val = ((get_byte (memp) & 0xff) << 8) + (get_byte (memp + 2) & 0xff);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* BTST.B Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0110_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* BTST.B Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0118_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* BTST.B Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0120_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* BTST.B Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0128_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* BTST.B Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0130_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (2));
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* BTST.B Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0138_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* BTST.B Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0139_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (6);
return 16 * CYCLE_UNIT / 2;
}

/* BTST.B Dn,(d16,PC) */
uae_u32 REGPARAM2 CPUFUNC(op_013a_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = 2;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_getpc () + 2;
	dsta += (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* BTST.B Dn,(d8,PC,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_013b_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = 3;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr tmppc;
	uaecptr dsta;
	tmppc = m68k_getpc () + 2;
	dsta = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* BTST.B Dn,#<data>.B */
uae_u32 REGPARAM2 CPUFUNC(op_013c_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uae_s8 dst = get_dibyte (2);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}	m68k_incpc (4);
return 8 * CYCLE_UNIT / 2;
}

/* BCHG.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0140_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src &= 31;
	dst ^= (1 << src);
	SET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	m68k_dreg (regs, dstreg) = (dst);
}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* MVPMR.L (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0148_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr memp = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_u32 val = ((get_byte (memp) & 0xff) << 24) + ((get_byte (memp + 2) & 0xff) << 16)
              + ((get_byte (memp + 4) & 0xff) << 8) + (get_byte (memp + 6) & 0xff);
	m68k_dreg (regs, dstreg) = (val);
}}	m68k_incpc (4);
return 24 * CYCLE_UNIT / 2;
}

/* BCHG.B Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0150_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* BCHG.B Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0158_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* BCHG.B Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0160_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* BCHG.B Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0168_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* BCHG.B Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0170_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (2));
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* BCHG.B Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0178_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* BCHG.B Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0179_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* BCLR.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0180_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src &= 31;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	m68k_dreg (regs, dstreg) = (dst);
}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* MVPRM.W Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0188_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
	uaecptr memp = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	put_byte (memp, src >> 8);
	put_byte (memp + 2, src);
}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* BCLR.B Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0190_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* BCLR.B Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0198_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* BCLR.B Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_01a0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* BCLR.B Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_01a8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* BCLR.B Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_01b0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (2));
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* BCLR.B Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_01b8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* BCLR.B Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_01b9_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* BSET.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_01c0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src &= 31;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	m68k_dreg (regs, dstreg) = (dst);
}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* MVPRM.L Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_01c8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
	uaecptr memp = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	put_byte (memp, src >> 24);
	put_byte (memp + 2, src >> 16);
	put_byte (memp + 4, src >> 8);
	put_byte (memp + 6, src);
}}	m68k_incpc (4);
return 24 * CYCLE_UNIT / 2;
}

/* BSET.B Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_01d0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* BSET.B Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_01d8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* BSET.B Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_01e0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* BSET.B Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_01e8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* BSET.B Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_01f0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (2));
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* BSET.B Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_01f8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* BSET.B Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_01f9_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* AND.B #<data>.B,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0200_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}	m68k_incpc (4);
return 8 * CYCLE_UNIT / 2;
}

/* AND.B #<data>.B,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0210_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* AND.B #<data>.B,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0218_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* AND.B #<data>.B,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0220_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* AND.B #<data>.B,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0228_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* AND.B #<data>.B,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0230_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (4));
{	uae_s8 dst = get_byte (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (6);
return 22 * CYCLE_UNIT / 2;
}

/* AND.B #<data>.B,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0238_4)(uae_u32 opcode)
{
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* AND.B #<data>.B,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0239_4)(uae_u32 opcode)
{
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_s8 dst = get_byte (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (8);
return 24 * CYCLE_UNIT / 2;
}

/* ANDSR.B #<data>.W */
uae_u32 REGPARAM2 CPUFUNC(op_023c_4)(uae_u32 opcode)
{
{	MakeSR ();
{	uae_s16 src = get_diword (2);
	src |= 0xFF00;
	m68k_incpc (4);
	regs.sr &= src;
	MakeFromSR ();
}}return 20 * CYCLE_UNIT / 2;
}

/* AND.W #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0240_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}	m68k_incpc (4);
return 8 * CYCLE_UNIT / 2;
}

/* AND.W #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0250_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* AND.W #<data>.W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0258_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) += 2;
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* AND.W #<data>.W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0260_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* AND.W #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0268_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_s16 dst = get_word (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* AND.W #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0270_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (4));
{	uae_s16 dst = get_word (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (6);
return 22 * CYCLE_UNIT / 2;
}

/* AND.W #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0278_4)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_s16 dst = get_word (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* AND.W #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0279_4)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_s16 dst = get_word (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (8);
return 24 * CYCLE_UNIT / 2;
}

/* ANDSR.W #<data>.W */
uae_u32 REGPARAM2 CPUFUNC(op_027c_4)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	MakeSR ();
{	uae_s16 src = get_diword (2);
	m68k_incpc (4);
	regs.sr &= src;
	MakeFromSR ();
}}}return 20 * CYCLE_UNIT / 2;
}

/* AND.L #<data>.L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0280_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}	m68k_incpc (6);
return 16 * CYCLE_UNIT / 2;
}

/* AND.L #<data>.L,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0290_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (6);
return 28 * CYCLE_UNIT / 2;
}

/* AND.L #<data>.L,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0298_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) += 4;
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (6);
return 28 * CYCLE_UNIT / 2;
}

/* AND.L #<data>.L,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_02a0_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (6);
return 30 * CYCLE_UNIT / 2;
}

/* AND.L #<data>.L,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_02a8_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (6);
{	uae_s32 dst = get_long (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (8);
return 32 * CYCLE_UNIT / 2;
}

/* AND.L #<data>.L,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_02b0_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (6));
{	uae_s32 dst = get_long (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (8);
return 34 * CYCLE_UNIT / 2;
}

/* AND.L #<data>.L,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_02b8_4)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (6);
{	uae_s32 dst = get_long (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (8);
return 32 * CYCLE_UNIT / 2;
}

/* AND.L #<data>.L,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_02b9_4)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = get_dilong (6);
{	uae_s32 dst = get_long (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (10);
return 36 * CYCLE_UNIT / 2;
}

/* SUB.B #<data>.B,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0400_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}	m68k_incpc (4);
return 8 * CYCLE_UNIT / 2;
}

/* SUB.B #<data>.B,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0410_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* SUB.B #<data>.B,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0418_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* SUB.B #<data>.B,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0420_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* SUB.B #<data>.B,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0428_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* SUB.B #<data>.B,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0430_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (4));
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (6);
return 22 * CYCLE_UNIT / 2;
}

/* SUB.B #<data>.B,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0438_4)(uae_u32 opcode)
{
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* SUB.B #<data>.B,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0439_4)(uae_u32 opcode)
{
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (8);
return 24 * CYCLE_UNIT / 2;
}

/* SUB.W #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0440_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}	m68k_incpc (4);
return 8 * CYCLE_UNIT / 2;
}

/* SUB.W #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0450_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* SUB.W #<data>.W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0458_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) += 2;
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* SUB.W #<data>.W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0460_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* SUB.W #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0468_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* SUB.W #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0470_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (4));
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (6);
return 22 * CYCLE_UNIT / 2;
}

/* SUB.W #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0478_4)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* SUB.W #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0479_4)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (8);
return 24 * CYCLE_UNIT / 2;
}

/* SUB.L #<data>.L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0480_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}}	m68k_incpc (6);
return 16 * CYCLE_UNIT / 2;
}

/* SUB.L #<data>.L,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0490_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (6);
return 28 * CYCLE_UNIT / 2;
}

/* SUB.L #<data>.L,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0498_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) += 4;
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (6);
return 28 * CYCLE_UNIT / 2;
}

/* SUB.L #<data>.L,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_04a0_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (6);
return 30 * CYCLE_UNIT / 2;
}

/* SUB.L #<data>.L,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_04a8_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (6);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (8);
return 32 * CYCLE_UNIT / 2;
}

/* SUB.L #<data>.L,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_04b0_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (6));
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (8);
return 34 * CYCLE_UNIT / 2;
}

/* SUB.L #<data>.L,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_04b8_4)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (6);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (8);
return 32 * CYCLE_UNIT / 2;
}

/* SUB.L #<data>.L,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_04b9_4)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = get_dilong (6);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (10);
return 36 * CYCLE_UNIT / 2;
}

/* ADD.B #<data>.B,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0600_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}	m68k_incpc (4);
return 8 * CYCLE_UNIT / 2;
}

/* ADD.B #<data>.B,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0610_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* ADD.B #<data>.B,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0618_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* ADD.B #<data>.B,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0620_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* ADD.B #<data>.B,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0628_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* ADD.B #<data>.B,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0630_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (4));
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (6);
return 22 * CYCLE_UNIT / 2;
}

/* ADD.B #<data>.B,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0638_4)(uae_u32 opcode)
{
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* ADD.B #<data>.B,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0639_4)(uae_u32 opcode)
{
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (8);
return 24 * CYCLE_UNIT / 2;
}

/* ADD.W #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0640_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}	m68k_incpc (4);
return 8 * CYCLE_UNIT / 2;
}

/* ADD.W #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0650_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* ADD.W #<data>.W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0658_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) += 2;
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* ADD.W #<data>.W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0660_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* ADD.W #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0668_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* ADD.W #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0670_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (4));
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (6);
return 22 * CYCLE_UNIT / 2;
}

/* ADD.W #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0678_4)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* ADD.W #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0679_4)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (8);
return 24 * CYCLE_UNIT / 2;
}

/* ADD.L #<data>.L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0680_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}}	m68k_incpc (6);
return 16 * CYCLE_UNIT / 2;
}

/* ADD.L #<data>.L,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0690_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (6);
return 28 * CYCLE_UNIT / 2;
}

/* ADD.L #<data>.L,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0698_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) += 4;
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (6);
return 28 * CYCLE_UNIT / 2;
}

/* ADD.L #<data>.L,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_06a0_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (6);
return 30 * CYCLE_UNIT / 2;
}

/* ADD.L #<data>.L,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_06a8_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (6);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (8);
return 32 * CYCLE_UNIT / 2;
}

/* ADD.L #<data>.L,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_06b0_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (6));
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (8);
return 34 * CYCLE_UNIT / 2;
}

/* ADD.L #<data>.L,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_06b8_4)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (6);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (8);
return 32 * CYCLE_UNIT / 2;
}

/* ADD.L #<data>.L,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_06b9_4)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = get_dilong (6);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (10);
return 36 * CYCLE_UNIT / 2;
}

/* BTST.L #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0800_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src &= 31;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}	m68k_incpc (4);
return 10 * CYCLE_UNIT / 2;
}

/* BTST.B #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0810_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* BTST.B #<data>.W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0818_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* BTST.B #<data>.W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0820_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* BTST.B #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0828_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (6);
return 16 * CYCLE_UNIT / 2;
}

/* BTST.B #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0830_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (4));
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (6);
return 18 * CYCLE_UNIT / 2;
}

/* BTST.B #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0838_4)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (6);
return 16 * CYCLE_UNIT / 2;
}

/* BTST.B #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0839_4)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (8);
return 20 * CYCLE_UNIT / 2;
}

/* BTST.B #<data>.W,(d16,PC) */
uae_u32 REGPARAM2 CPUFUNC(op_083a_4)(uae_u32 opcode)
{
	uae_u32 dstreg = 2;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_getpc () + 4;
	dsta += (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (6);
return 16 * CYCLE_UNIT / 2;
}

/* BTST.B #<data>.W,(d8,PC,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_083b_4)(uae_u32 opcode)
{
	uae_u32 dstreg = 3;
{{	uae_s16 src = get_diword (2);
{	uaecptr tmppc;
	uaecptr dsta;
	tmppc = m68k_getpc () + 4;
	dsta = get_disp_ea_000 (tmppc, get_diword (4));
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (6);
return 18 * CYCLE_UNIT / 2;
}

/* BCHG.L #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0840_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src &= 31;
	dst ^= (1 << src);
	SET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	m68k_dreg (regs, dstreg) = (dst);
}}}	m68k_incpc (4);
return 10 * CYCLE_UNIT / 2;
}

/* BCHG.B #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0850_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* BCHG.B #<data>.W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0858_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* BCHG.B #<data>.W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0860_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* BCHG.B #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0868_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* BCHG.B #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0870_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (4));
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (6);
return 22 * CYCLE_UNIT / 2;
}

/* BCHG.B #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0878_4)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* BCHG.B #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0879_4)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (8);
return 24 * CYCLE_UNIT / 2;
}

/* BCLR.L #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0880_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src &= 31;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	m68k_dreg (regs, dstreg) = (dst);
}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* BCLR.B #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0890_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* BCLR.B #<data>.W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0898_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* BCLR.B #<data>.W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_08a0_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* BCLR.B #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_08a8_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* BCLR.B #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_08b0_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (4));
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (6);
return 22 * CYCLE_UNIT / 2;
}

/* BCLR.B #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_08b8_4)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* BCLR.B #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_08b9_4)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (8);
return 24 * CYCLE_UNIT / 2;
}

/* BSET.L #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_08c0_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src &= 31;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	m68k_dreg (regs, dstreg) = (dst);
}}}	m68k_incpc (4);
return 10 * CYCLE_UNIT / 2;
}

/* BSET.B #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_08d0_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* BSET.B #<data>.W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_08d8_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* BSET.B #<data>.W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_08e0_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* BSET.B #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_08e8_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* BSET.B #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_08f0_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (4));
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (6);
return 22 * CYCLE_UNIT / 2;
}

/* BSET.B #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_08f8_4)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* BSET.B #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_08f9_4)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte (dsta, dst);
}}}}	m68k_incpc (8);
return 24 * CYCLE_UNIT / 2;
}

/* EOR.B #<data>.B,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0a00_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}	m68k_incpc (4);
return 8 * CYCLE_UNIT / 2;
}

/* EOR.B #<data>.B,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0a10_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* EOR.B #<data>.B,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0a18_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* EOR.B #<data>.B,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0a20_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* EOR.B #<data>.B,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0a28_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* EOR.B #<data>.B,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0a30_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (4));
{	uae_s8 dst = get_byte (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (6);
return 22 * CYCLE_UNIT / 2;
}

/* EOR.B #<data>.B,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0a38_4)(uae_u32 opcode)
{
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* EOR.B #<data>.B,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0a39_4)(uae_u32 opcode)
{
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_s8 dst = get_byte (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (8);
return 24 * CYCLE_UNIT / 2;
}

/* EORSR.B #<data>.W */
uae_u32 REGPARAM2 CPUFUNC(op_0a3c_4)(uae_u32 opcode)
{
{	MakeSR ();
{	uae_s16 src = get_diword (2);
	src &= 0xFF;
	m68k_incpc (4);
	regs.sr ^= src;
	MakeFromSR ();
}}return 20 * CYCLE_UNIT / 2;
}

/* EOR.W #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0a40_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}	m68k_incpc (4);
return 8 * CYCLE_UNIT / 2;
}

/* EOR.W #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0a50_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* EOR.W #<data>.W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0a58_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) += 2;
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* EOR.W #<data>.W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0a60_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* EOR.W #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0a68_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_s16 dst = get_word (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* EOR.W #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0a70_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (4));
{	uae_s16 dst = get_word (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (6);
return 22 * CYCLE_UNIT / 2;
}

/* EOR.W #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0a78_4)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_s16 dst = get_word (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* EOR.W #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0a79_4)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_s16 dst = get_word (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (8);
return 24 * CYCLE_UNIT / 2;
}

/* EORSR.W #<data>.W */
uae_u32 REGPARAM2 CPUFUNC(op_0a7c_4)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	MakeSR ();
{	uae_s16 src = get_diword (2);
	m68k_incpc (4);
	regs.sr ^= src;
	MakeFromSR ();
}}}return 20 * CYCLE_UNIT / 2;
}

/* EOR.L #<data>.L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0a80_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}	m68k_incpc (6);
return 16 * CYCLE_UNIT / 2;
}

/* EOR.L #<data>.L,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0a90_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (6);
return 28 * CYCLE_UNIT / 2;
}

/* EOR.L #<data>.L,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0a98_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) += 4;
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (6);
return 28 * CYCLE_UNIT / 2;
}

/* EOR.L #<data>.L,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0aa0_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (6);
return 30 * CYCLE_UNIT / 2;
}

/* EOR.L #<data>.L,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0aa8_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (6);
{	uae_s32 dst = get_long (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (8);
return 32 * CYCLE_UNIT / 2;
}

/* EOR.L #<data>.L,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0ab0_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (6));
{	uae_s32 dst = get_long (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (8);
return 34 * CYCLE_UNIT / 2;
}

/* EOR.L #<data>.L,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0ab8_4)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (6);
{	uae_s32 dst = get_long (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (8);
return 32 * CYCLE_UNIT / 2;
}

/* EOR.L #<data>.L,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0ab9_4)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = get_dilong (6);
{	uae_s32 dst = get_long (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (10);
return 36 * CYCLE_UNIT / 2;
}

#endif

#ifdef PART_2
/* CMP.B #<data>.B,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0c00_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}	m68k_incpc (4);
return 8 * CYCLE_UNIT / 2;
}

/* CMP.B #<data>.B,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0c10_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* CMP.B #<data>.B,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0c18_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* CMP.B #<data>.B,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0c20_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* CMP.B #<data>.B,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0c28_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (6);
return 16 * CYCLE_UNIT / 2;
}

/* CMP.B #<data>.B,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0c30_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (4));
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (6);
return 18 * CYCLE_UNIT / 2;
}

/* CMP.B #<data>.B,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0c38_4)(uae_u32 opcode)
{
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (6);
return 16 * CYCLE_UNIT / 2;
}

/* CMP.B #<data>.B,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0c39_4)(uae_u32 opcode)
{
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (8);
return 20 * CYCLE_UNIT / 2;
}

/* CMP.W #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0c40_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}	m68k_incpc (4);
return 8 * CYCLE_UNIT / 2;
}

/* CMP.W #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0c50_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* CMP.W #<data>.W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0c58_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) += 2;
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* CMP.W #<data>.W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0c60_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* CMP.W #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0c68_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (6);
return 16 * CYCLE_UNIT / 2;
}

/* CMP.W #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0c70_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (4));
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (6);
return 18 * CYCLE_UNIT / 2;
}

/* CMP.W #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0c78_4)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (6);
return 16 * CYCLE_UNIT / 2;
}

/* CMP.W #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0c79_4)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (8);
return 20 * CYCLE_UNIT / 2;
}

/* CMP.L #<data>.L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0c80_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}	m68k_incpc (6);
return 14 * CYCLE_UNIT / 2;
}

/* CMP.L #<data>.L,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0c90_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* CMP.L #<data>.L,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0c98_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) += 4;
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* CMP.L #<data>.L,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0ca0_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (6);
return 22 * CYCLE_UNIT / 2;
}

/* CMP.L #<data>.L,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0ca8_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (6);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (8);
return 24 * CYCLE_UNIT / 2;
}

/* CMP.L #<data>.L,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0cb0_4)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (6));
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (8);
return 26 * CYCLE_UNIT / 2;
}

/* CMP.L #<data>.L,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0cb8_4)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (6);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (8);
return 24 * CYCLE_UNIT / 2;
}

/* CMP.L #<data>.L,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0cb9_4)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = get_dilong (6);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (10);
return 28 * CYCLE_UNIT / 2;
}

/* MOVE.B Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_1000_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc (2);
}}}return 4 * CYCLE_UNIT / 2;
}

/* MOVE.B (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_1010_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc (2);
}}}}return 8 * CYCLE_UNIT / 2;
}

/* MOVE.B (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_1018_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc (2);
}}}}return 8 * CYCLE_UNIT / 2;
}

/* MOVE.B -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_1020_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc (2);
}}}}return 10 * CYCLE_UNIT / 2;
}

/* MOVE.B (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_1028_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc (4);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.B (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_1030_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s8 src = get_byte (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc (4);
}}}}return 14 * CYCLE_UNIT / 2;
}

/* MOVE.B (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_1038_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc (4);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.B (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_1039_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc (6);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.B (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_103a_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc (4);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.B (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_103b_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s8 src = get_byte (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc (4);
}}}}return 14 * CYCLE_UNIT / 2;
}

/* MOVE.B #<data>.B,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_103c_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = get_dibyte (2);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc (4);
}}}return 8 * CYCLE_UNIT / 2;
}

/* MOVE.B Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_1080_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (2);
}}}return 8 * CYCLE_UNIT / 2;
}

/* MOVE.B (An),(An) */
uae_u32 REGPARAM2 CPUFUNC(op_1090_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (2);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.B (An)+,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_1098_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (2);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.B -(An),(An) */
uae_u32 REGPARAM2 CPUFUNC(op_10a0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (2);
}}}}return 14 * CYCLE_UNIT / 2;
}

/* MOVE.B (d16,An),(An) */
uae_u32 REGPARAM2 CPUFUNC(op_10a8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (4);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.B (d8,An,Xn),(An) */
uae_u32 REGPARAM2 CPUFUNC(op_10b0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (4);
}}}}return 18 * CYCLE_UNIT / 2;
}

/* MOVE.B (xxx).W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_10b8_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (4);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.B (xxx).L,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_10b9_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (6);
}}}}return 20 * CYCLE_UNIT / 2;
}

/* MOVE.B (d16,PC),(An) */
uae_u32 REGPARAM2 CPUFUNC(op_10ba_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (4);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.B (d8,PC,Xn),(An) */
uae_u32 REGPARAM2 CPUFUNC(op_10bb_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (4);
}}}}return 18 * CYCLE_UNIT / 2;
}

/* MOVE.B #<data>.B,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_10bc_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (4);
}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.B Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_10c0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (2);
}}}return 8 * CYCLE_UNIT / 2;
}

/* MOVE.B (An),(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_10d0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (2);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.B (An)+,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_10d8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (2);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.B -(An),(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_10e0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (2);
}}}}return 14 * CYCLE_UNIT / 2;
}

/* MOVE.B (d16,An),(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_10e8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (4);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.B (d8,An,Xn),(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_10f0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (4);
}}}}return 18 * CYCLE_UNIT / 2;
}

/* MOVE.B (xxx).W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_10f8_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (4);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.B (xxx).L,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_10f9_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (6);
}}}}return 20 * CYCLE_UNIT / 2;
}

/* MOVE.B (d16,PC),(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_10fa_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (4);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.B (d8,PC,Xn),(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_10fb_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (4);
}}}}return 18 * CYCLE_UNIT / 2;
}

/* MOVE.B #<data>.B,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_10fc_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (4);
}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.B Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_1100_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (2);
}}}return 8 * CYCLE_UNIT / 2;
}

/* MOVE.B (An),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_1110_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (2);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.B (An)+,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_1118_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (2);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.B -(An),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_1120_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (2);
}}}}return 14 * CYCLE_UNIT / 2;
}

/* MOVE.B (d16,An),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_1128_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (4);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.B (d8,An,Xn),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_1130_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (4);
}}}}return 18 * CYCLE_UNIT / 2;
}

/* MOVE.B (xxx).W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_1138_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (4);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.B (xxx).L,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_1139_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (6);
}}}}return 20 * CYCLE_UNIT / 2;
}

/* MOVE.B (d16,PC),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_113a_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (4);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.B (d8,PC,Xn),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_113b_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (4);
}}}}return 18 * CYCLE_UNIT / 2;
}

/* MOVE.B #<data>.B,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_113c_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (4);
}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.B Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_1140_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (4);
}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.B (An),(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_1150_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (4);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.B (An)+,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_1158_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (4);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.B -(An),(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_1160_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (4);
}}}}return 18 * CYCLE_UNIT / 2;
}

/* MOVE.B (d16,An),(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_1168_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (6);
}}}}return 20 * CYCLE_UNIT / 2;
}

/* MOVE.B (d8,An,Xn),(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_1170_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (6);
}}}}return 22 * CYCLE_UNIT / 2;
}

/* MOVE.B (xxx).W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_1178_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (6);
}}}}return 20 * CYCLE_UNIT / 2;
}

/* MOVE.B (xxx).L,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_1179_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (6);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (8);
}}}}return 24 * CYCLE_UNIT / 2;
}

/* MOVE.B (d16,PC),(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_117a_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (6);
}}}}return 20 * CYCLE_UNIT / 2;
}

/* MOVE.B (d8,PC,Xn),(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_117b_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (6);
}}}}return 22 * CYCLE_UNIT / 2;
}

/* MOVE.B #<data>.B,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_117c_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (6);
}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.B Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_1180_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (2));
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (4);
}}}return 14 * CYCLE_UNIT / 2;
}

/* MOVE.B (An),(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_1190_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (2));
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (4);
}}}}return 18 * CYCLE_UNIT / 2;
}

/* MOVE.B (An)+,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_1198_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (2));
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (4);
}}}}return 18 * CYCLE_UNIT / 2;
}

/* MOVE.B -(An),(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_11a0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (2));
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (4);
}}}}return 20 * CYCLE_UNIT / 2;
}

/* MOVE.B (d16,An),(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_11a8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (4));
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (6);
}}}}return 22 * CYCLE_UNIT / 2;
}

/* MOVE.B (d8,An,Xn),(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_11b0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (4));
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (6);
}}}}return 24 * CYCLE_UNIT / 2;
}

/* MOVE.B (xxx).W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_11b8_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (4));
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (6);
}}}}return 22 * CYCLE_UNIT / 2;
}

/* MOVE.B (xxx).L,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_11b9_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (6));
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (8);
}}}}return 26 * CYCLE_UNIT / 2;
}

/* MOVE.B (d16,PC),(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_11ba_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (4));
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (6);
}}}}return 22 * CYCLE_UNIT / 2;
}

/* MOVE.B (d8,PC,Xn),(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_11bb_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (4));
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (6);
}}}}return 24 * CYCLE_UNIT / 2;
}

/* MOVE.B #<data>.B,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_11bc_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (4));
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (6);
}}}return 18 * CYCLE_UNIT / 2;
}

/* MOVE.B Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_11c0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (4);
}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.B (An),(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_11d0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (4);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.B (An)+,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_11d8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (4);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.B -(An),(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_11e0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (4);
}}}}return 18 * CYCLE_UNIT / 2;
}

/* MOVE.B (d16,An),(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_11e8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (6);
}}}}return 20 * CYCLE_UNIT / 2;
}

/* MOVE.B (d8,An,Xn),(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_11f0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (6);
}}}}return 22 * CYCLE_UNIT / 2;
}

/* MOVE.B (xxx).W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_11f8_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (6);
}}}}return 20 * CYCLE_UNIT / 2;
}

/* MOVE.B (xxx).L,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_11f9_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (6);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (8);
}}}}return 24 * CYCLE_UNIT / 2;
}

/* MOVE.B (d16,PC),(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_11fa_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (6);
}}}}return 20 * CYCLE_UNIT / 2;
}

/* MOVE.B (d8,PC,Xn),(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_11fb_4)(uae_u32 opcode)
{
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (6);
}}}}return 22 * CYCLE_UNIT / 2;
}

/* MOVE.B #<data>.B,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_11fc_4)(uae_u32 opcode)
{
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (6);
}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.B Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_13c0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (6);
}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.B (An),(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_13d0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = get_dilong (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (6);
}}}}return 20 * CYCLE_UNIT / 2;
}

/* MOVE.B (An)+,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_13d8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{	uaecptr dsta;
	dsta = get_dilong (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (6);
}}}}return 20 * CYCLE_UNIT / 2;
}

/* MOVE.B -(An),(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_13e0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = get_dilong (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (6);
}}}}return 22 * CYCLE_UNIT / 2;
}

/* MOVE.B (d16,An),(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_13e8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = get_dilong (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (8);
}}}}return 24 * CYCLE_UNIT / 2;
}

/* MOVE.B (d8,An,Xn),(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_13f0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = get_dilong (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (8);
}}}}return 26 * CYCLE_UNIT / 2;
}

/* MOVE.B (xxx).W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_13f8_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = get_dilong (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (8);
}}}}return 24 * CYCLE_UNIT / 2;
}

/* MOVE.B (xxx).L,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_13f9_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = get_dilong (6);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (10);
}}}}return 28 * CYCLE_UNIT / 2;
}

/* MOVE.B (d16,PC),(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_13fa_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = get_dilong (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (8);
}}}}return 24 * CYCLE_UNIT / 2;
}

/* MOVE.B (d8,PC,Xn),(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_13fb_4)(uae_u32 opcode)
{
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = get_dilong (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (8);
}}}}return 26 * CYCLE_UNIT / 2;
}

/* MOVE.B #<data>.B,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_13fc_4)(uae_u32 opcode)
{
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
	m68k_incpc (8);
}}}return 20 * CYCLE_UNIT / 2;
}

/* MOVE.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_2000_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
	m68k_incpc (2);
}}}return 4 * CYCLE_UNIT / 2;
}

/* MOVE.L An,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_2008_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_areg (regs, srcreg);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
	m68k_incpc (2);
}}}return 4 * CYCLE_UNIT / 2;
}

/* MOVE.L (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_2010_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
	m68k_incpc (2);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.L (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_2018_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) += 4;
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
	m68k_incpc (2);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.L -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_2020_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
	m68k_incpc (2);
}}}}return 14 * CYCLE_UNIT / 2;
}

/* MOVE.L (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_2028_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
	m68k_incpc (4);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.L (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_2030_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s32 src = get_long (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
	m68k_incpc (4);
}}}}return 18 * CYCLE_UNIT / 2;
}

/* MOVE.L (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_2038_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
	m68k_incpc (4);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.L (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_2039_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
	m68k_incpc (6);
}}}}return 20 * CYCLE_UNIT / 2;
}

/* MOVE.L (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_203a_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
	m68k_incpc (4);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.L (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_203b_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s32 src = get_long (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
	m68k_incpc (4);
}}}}return 18 * CYCLE_UNIT / 2;
}

/* MOVE.L #<data>.L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_203c_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
	m68k_incpc (6);
}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVEA.L Dn,An */
uae_u32 REGPARAM2 CPUFUNC(op_2040_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	m68k_areg (regs, dstreg) = (src);
	m68k_incpc (2);
}}}return 4 * CYCLE_UNIT / 2;
}

/* MOVEA.L An,An */
uae_u32 REGPARAM2 CPUFUNC(op_2048_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_areg (regs, srcreg);
{	m68k_areg (regs, dstreg) = (src);
	m68k_incpc (2);
}}}return 4 * CYCLE_UNIT / 2;
}

/* MOVEA.L (An),An */
uae_u32 REGPARAM2 CPUFUNC(op_2050_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
{	m68k_areg (regs, dstreg) = (src);
	m68k_incpc (2);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVEA.L (An)+,An */
uae_u32 REGPARAM2 CPUFUNC(op_2058_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) += 4;
{	m68k_areg (regs, dstreg) = (src);
	m68k_incpc (2);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVEA.L -(An),An */
uae_u32 REGPARAM2 CPUFUNC(op_2060_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	m68k_areg (regs, dstreg) = (src);
	m68k_incpc (2);
}}}}return 14 * CYCLE_UNIT / 2;
}

/* MOVEA.L (d16,An),An */
uae_u32 REGPARAM2 CPUFUNC(op_2068_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	m68k_areg (regs, dstreg) = (src);
	m68k_incpc (4);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVEA.L (d8,An,Xn),An */
uae_u32 REGPARAM2 CPUFUNC(op_2070_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s32 src = get_long (srca);
{	m68k_areg (regs, dstreg) = (src);
	m68k_incpc (4);
}}}}return 18 * CYCLE_UNIT / 2;
}

/* MOVEA.L (xxx).W,An */
uae_u32 REGPARAM2 CPUFUNC(op_2078_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	m68k_areg (regs, dstreg) = (src);
	m68k_incpc (4);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVEA.L (xxx).L,An */
uae_u32 REGPARAM2 CPUFUNC(op_2079_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long (srca);
{	m68k_areg (regs, dstreg) = (src);
	m68k_incpc (6);
}}}}return 20 * CYCLE_UNIT / 2;
}

/* MOVEA.L (d16,PC),An */
uae_u32 REGPARAM2 CPUFUNC(op_207a_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	m68k_areg (regs, dstreg) = (src);
	m68k_incpc (4);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVEA.L (d8,PC,Xn),An */
uae_u32 REGPARAM2 CPUFUNC(op_207b_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s32 src = get_long (srca);
{	m68k_areg (regs, dstreg) = (src);
	m68k_incpc (4);
}}}}return 18 * CYCLE_UNIT / 2;
}

/* MOVEA.L #<data>.L,An */
uae_u32 REGPARAM2 CPUFUNC(op_207c_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	m68k_areg (regs, dstreg) = (src);
	m68k_incpc (6);
}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.L Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_2080_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (2);
}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.L An,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_2088_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_areg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (2);
}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.L (An),(An) */
uae_u32 REGPARAM2 CPUFUNC(op_2090_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (2);
}}}}return 20 * CYCLE_UNIT / 2;
}

/* MOVE.L (An)+,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_2098_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) += 4;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (2);
}}}}return 20 * CYCLE_UNIT / 2;
}

/* MOVE.L -(An),(An) */
uae_u32 REGPARAM2 CPUFUNC(op_20a0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (2);
}}}}return 22 * CYCLE_UNIT / 2;
}

/* MOVE.L (d16,An),(An) */
uae_u32 REGPARAM2 CPUFUNC(op_20a8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (4);
}}}}return 24 * CYCLE_UNIT / 2;
}

/* MOVE.L (d8,An,Xn),(An) */
uae_u32 REGPARAM2 CPUFUNC(op_20b0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (4);
}}}}return 26 * CYCLE_UNIT / 2;
}

/* MOVE.L (xxx).W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_20b8_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (4);
}}}}return 24 * CYCLE_UNIT / 2;
}

/* MOVE.L (xxx).L,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_20b9_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (6);
}}}}return 28 * CYCLE_UNIT / 2;
}

/* MOVE.L (d16,PC),(An) */
uae_u32 REGPARAM2 CPUFUNC(op_20ba_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (4);
}}}}return 24 * CYCLE_UNIT / 2;
}

/* MOVE.L (d8,PC,Xn),(An) */
uae_u32 REGPARAM2 CPUFUNC(op_20bb_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (4);
}}}}return 26 * CYCLE_UNIT / 2;
}

/* MOVE.L #<data>.L,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_20bc_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (6);
}}}return 20 * CYCLE_UNIT / 2;
}

/* MOVE.L Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_20c0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 4;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (2);
}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.L An,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_20c8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_areg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 4;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (2);
}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.L (An),(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_20d0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 4;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (2);
}}}}return 20 * CYCLE_UNIT / 2;
}

/* MOVE.L (An)+,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_20d8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) += 4;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 4;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (2);
}}}}return 20 * CYCLE_UNIT / 2;
}

/* MOVE.L -(An),(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_20e0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 4;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (2);
}}}}return 22 * CYCLE_UNIT / 2;
}

/* MOVE.L (d16,An),(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_20e8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 4;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (4);
}}}}return 24 * CYCLE_UNIT / 2;
}

/* MOVE.L (d8,An,Xn),(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_20f0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 4;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (4);
}}}}return 26 * CYCLE_UNIT / 2;
}

/* MOVE.L (xxx).W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_20f8_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 4;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (4);
}}}}return 24 * CYCLE_UNIT / 2;
}

/* MOVE.L (xxx).L,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_20f9_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 4;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (6);
}}}}return 28 * CYCLE_UNIT / 2;
}

/* MOVE.L (d16,PC),(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_20fa_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 4;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (4);
}}}}return 24 * CYCLE_UNIT / 2;
}

/* MOVE.L (d8,PC,Xn),(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_20fb_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 4;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (4);
}}}}return 26 * CYCLE_UNIT / 2;
}

/* MOVE.L #<data>.L,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_20fc_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 4;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (6);
}}}return 20 * CYCLE_UNIT / 2;
}

/* MOVE.L Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_2100_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (2);
}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.L An,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_2108_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_areg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (2);
}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.L (An),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_2110_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (2);
}}}}return 20 * CYCLE_UNIT / 2;
}

/* MOVE.L (An)+,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_2118_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) += 4;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (2);
}}}}return 20 * CYCLE_UNIT / 2;
}

/* MOVE.L -(An),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_2120_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (2);
}}}}return 22 * CYCLE_UNIT / 2;
}

/* MOVE.L (d16,An),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_2128_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (4);
}}}}return 24 * CYCLE_UNIT / 2;
}

/* MOVE.L (d8,An,Xn),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_2130_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (4);
}}}}return 26 * CYCLE_UNIT / 2;
}

/* MOVE.L (xxx).W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_2138_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (4);
}}}}return 24 * CYCLE_UNIT / 2;
}

/* MOVE.L (xxx).L,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_2139_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (6);
}}}}return 28 * CYCLE_UNIT / 2;
}

/* MOVE.L (d16,PC),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_213a_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (4);
}}}}return 24 * CYCLE_UNIT / 2;
}

/* MOVE.L (d8,PC,Xn),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_213b_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (4);
}}}}return 26 * CYCLE_UNIT / 2;
}

/* MOVE.L #<data>.L,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_213c_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (6);
}}}return 20 * CYCLE_UNIT / 2;
}

/* MOVE.L Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_2140_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (4);
}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.L An,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_2148_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_areg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (4);
}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.L (An),(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_2150_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (4);
}}}}return 24 * CYCLE_UNIT / 2;
}

/* MOVE.L (An)+,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_2158_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) += 4;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (4);
}}}}return 24 * CYCLE_UNIT / 2;
}

/* MOVE.L -(An),(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_2160_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (4);
}}}}return 26 * CYCLE_UNIT / 2;
}

/* MOVE.L (d16,An),(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_2168_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (6);
}}}}return 28 * CYCLE_UNIT / 2;
}

/* MOVE.L (d8,An,Xn),(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_2170_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (6);
}}}}return 30 * CYCLE_UNIT / 2;
}

/* MOVE.L (xxx).W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_2178_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (6);
}}}}return 28 * CYCLE_UNIT / 2;
}

/* MOVE.L (xxx).L,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_2179_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (6);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (8);
}}}}return 32 * CYCLE_UNIT / 2;
}

/* MOVE.L (d16,PC),(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_217a_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (6);
}}}}return 28 * CYCLE_UNIT / 2;
}

/* MOVE.L (d8,PC,Xn),(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_217b_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (6);
}}}}return 30 * CYCLE_UNIT / 2;
}

/* MOVE.L #<data>.L,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_217c_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (6);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (8);
}}}return 24 * CYCLE_UNIT / 2;
}

/* MOVE.L Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_2180_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (2));
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (4);
}}}return 18 * CYCLE_UNIT / 2;
}

/* MOVE.L An,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_2188_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_areg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (2));
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (4);
}}}return 18 * CYCLE_UNIT / 2;
}

/* MOVE.L (An),(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_2190_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (2));
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (4);
}}}}return 26 * CYCLE_UNIT / 2;
}

#endif

#ifdef PART_3
/* MOVE.L (An)+,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_2198_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) += 4;
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (2));
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (4);
}}}}return 26 * CYCLE_UNIT / 2;
}

/* MOVE.L -(An),(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_21a0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (2));
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (4);
}}}}return 28 * CYCLE_UNIT / 2;
}

/* MOVE.L (d16,An),(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_21a8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (4));
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (6);
}}}}return 30 * CYCLE_UNIT / 2;
}

/* MOVE.L (d8,An,Xn),(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_21b0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (4));
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (6);
}}}}return 32 * CYCLE_UNIT / 2;
}

/* MOVE.L (xxx).W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_21b8_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (4));
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (6);
}}}}return 30 * CYCLE_UNIT / 2;
}

/* MOVE.L (xxx).L,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_21b9_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (6));
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (8);
}}}}return 34 * CYCLE_UNIT / 2;
}

/* MOVE.L (d16,PC),(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_21ba_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (4));
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (6);
}}}}return 30 * CYCLE_UNIT / 2;
}

/* MOVE.L (d8,PC,Xn),(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_21bb_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (4));
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (6);
}}}}return 32 * CYCLE_UNIT / 2;
}

/* MOVE.L #<data>.L,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_21bc_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (6));
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (8);
}}}return 26 * CYCLE_UNIT / 2;
}

/* MOVE.L Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_21c0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (4);
}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.L An,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_21c8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = m68k_areg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (4);
}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.L (An),(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_21d0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (4);
}}}}return 24 * CYCLE_UNIT / 2;
}

/* MOVE.L (An)+,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_21d8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) += 4;
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (4);
}}}}return 24 * CYCLE_UNIT / 2;
}

/* MOVE.L -(An),(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_21e0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (4);
}}}}return 26 * CYCLE_UNIT / 2;
}

/* MOVE.L (d16,An),(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_21e8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (6);
}}}}return 28 * CYCLE_UNIT / 2;
}

/* MOVE.L (d8,An,Xn),(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_21f0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (6);
}}}}return 30 * CYCLE_UNIT / 2;
}

/* MOVE.L (xxx).W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_21f8_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (6);
}}}}return 28 * CYCLE_UNIT / 2;
}

/* MOVE.L (xxx).L,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_21f9_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (6);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (8);
}}}}return 32 * CYCLE_UNIT / 2;
}

/* MOVE.L (d16,PC),(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_21fa_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (6);
}}}}return 28 * CYCLE_UNIT / 2;
}

/* MOVE.L (d8,PC,Xn),(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_21fb_4)(uae_u32 opcode)
{
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (6);
}}}}return 30 * CYCLE_UNIT / 2;
}

/* MOVE.L #<data>.L,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_21fc_4)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (6);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (8);
}}}return 24 * CYCLE_UNIT / 2;
}

/* MOVE.L Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_23c0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (6);
}}}return 20 * CYCLE_UNIT / 2;
}

/* MOVE.L An,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_23c8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = m68k_areg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (6);
}}}return 20 * CYCLE_UNIT / 2;
}

/* MOVE.L (An),(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_23d0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = get_dilong (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (6);
}}}}return 28 * CYCLE_UNIT / 2;
}

/* MOVE.L (An)+,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_23d8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) += 4;
{	uaecptr dsta;
	dsta = get_dilong (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (6);
}}}}return 28 * CYCLE_UNIT / 2;
}

/* MOVE.L -(An),(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_23e0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = get_dilong (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (6);
}}}}return 30 * CYCLE_UNIT / 2;
}

/* MOVE.L (d16,An),(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_23e8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = get_dilong (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (8);
}}}}return 32 * CYCLE_UNIT / 2;
}

/* MOVE.L (d8,An,Xn),(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_23f0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = get_dilong (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (8);
}}}}return 34 * CYCLE_UNIT / 2;
}

/* MOVE.L (xxx).W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_23f8_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = get_dilong (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (8);
}}}}return 32 * CYCLE_UNIT / 2;
}

/* MOVE.L (xxx).L,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_23f9_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = get_dilong (6);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (10);
}}}}return 36 * CYCLE_UNIT / 2;
}

/* MOVE.L (d16,PC),(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_23fa_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = get_dilong (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (8);
}}}}return 32 * CYCLE_UNIT / 2;
}

/* MOVE.L (d8,PC,Xn),(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_23fb_4)(uae_u32 opcode)
{
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = get_dilong (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (8);
}}}}return 34 * CYCLE_UNIT / 2;
}

/* MOVE.L #<data>.L,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_23fc_4)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = get_dilong (6);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
	m68k_incpc (10);
}}}return 28 * CYCLE_UNIT / 2;
}

/* MOVE.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_3000_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc (2);
}}}return 4 * CYCLE_UNIT / 2;
}

/* MOVE.W An,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_3008_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_areg (regs, srcreg);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc (2);
}}}return 4 * CYCLE_UNIT / 2;
}

/* MOVE.W (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_3010_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc (2);
}}}}return 8 * CYCLE_UNIT / 2;
}

/* MOVE.W (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_3018_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) += 2;
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc (2);
}}}}return 8 * CYCLE_UNIT / 2;
}

/* MOVE.W -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_3020_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc (2);
}}}}return 10 * CYCLE_UNIT / 2;
}

/* MOVE.W (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_3028_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc (4);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.W (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_3030_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s16 src = get_word (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc (4);
}}}}return 14 * CYCLE_UNIT / 2;
}

/* MOVE.W (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_3038_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc (4);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.W (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_3039_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc (6);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.W (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_303a_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc (4);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.W (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_303b_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s16 src = get_word (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc (4);
}}}}return 14 * CYCLE_UNIT / 2;
}

/* MOVE.W #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_303c_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_diword (2);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc (4);
}}}return 8 * CYCLE_UNIT / 2;
}

/* MOVEA.W Dn,An */
uae_u32 REGPARAM2 CPUFUNC(op_3040_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	src = (uae_s32)(uae_s16)src;
	m68k_areg (regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc (2);
}}}return 4 * CYCLE_UNIT / 2;
}

/* MOVEA.W An,An */
uae_u32 REGPARAM2 CPUFUNC(op_3048_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_areg (regs, srcreg);
{	src = (uae_s32)(uae_s16)src;
	m68k_areg (regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc (2);
}}}return 4 * CYCLE_UNIT / 2;
}

/* MOVEA.W (An),An */
uae_u32 REGPARAM2 CPUFUNC(op_3050_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
{	src = (uae_s32)(uae_s16)src;
	m68k_areg (regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc (2);
}}}}return 8 * CYCLE_UNIT / 2;
}

/* MOVEA.W (An)+,An */
uae_u32 REGPARAM2 CPUFUNC(op_3058_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) += 2;
{	src = (uae_s32)(uae_s16)src;
	m68k_areg (regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc (2);
}}}}return 8 * CYCLE_UNIT / 2;
}

/* MOVEA.W -(An),An */
uae_u32 REGPARAM2 CPUFUNC(op_3060_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	src = (uae_s32)(uae_s16)src;
	m68k_areg (regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc (2);
}}}}return 10 * CYCLE_UNIT / 2;
}

/* MOVEA.W (d16,An),An */
uae_u32 REGPARAM2 CPUFUNC(op_3068_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	src = (uae_s32)(uae_s16)src;
	m68k_areg (regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc (4);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVEA.W (d8,An,Xn),An */
uae_u32 REGPARAM2 CPUFUNC(op_3070_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s16 src = get_word (srca);
{	src = (uae_s32)(uae_s16)src;
	m68k_areg (regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc (4);
}}}}return 14 * CYCLE_UNIT / 2;
}

/* MOVEA.W (xxx).W,An */
uae_u32 REGPARAM2 CPUFUNC(op_3078_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	src = (uae_s32)(uae_s16)src;
	m68k_areg (regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc (4);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVEA.W (xxx).L,An */
uae_u32 REGPARAM2 CPUFUNC(op_3079_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word (srca);
{	src = (uae_s32)(uae_s16)src;
	m68k_areg (regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc (6);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVEA.W (d16,PC),An */
uae_u32 REGPARAM2 CPUFUNC(op_307a_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	src = (uae_s32)(uae_s16)src;
	m68k_areg (regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc (4);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVEA.W (d8,PC,Xn),An */
uae_u32 REGPARAM2 CPUFUNC(op_307b_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s16 src = get_word (srca);
{	src = (uae_s32)(uae_s16)src;
	m68k_areg (regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc (4);
}}}}return 14 * CYCLE_UNIT / 2;
}

/* MOVEA.W #<data>.W,An */
uae_u32 REGPARAM2 CPUFUNC(op_307c_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_diword (2);
{	src = (uae_s32)(uae_s16)src;
	m68k_areg (regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc (4);
}}}return 8 * CYCLE_UNIT / 2;
}

/* MOVE.W Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_3080_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (2);
}}}return 8 * CYCLE_UNIT / 2;
}

/* MOVE.W An,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_3088_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_areg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (2);
}}}return 8 * CYCLE_UNIT / 2;
}

/* MOVE.W (An),(An) */
uae_u32 REGPARAM2 CPUFUNC(op_3090_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (2);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.W (An)+,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_3098_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) += 2;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (2);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.W -(An),(An) */
uae_u32 REGPARAM2 CPUFUNC(op_30a0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (2);
}}}}return 14 * CYCLE_UNIT / 2;
}

/* MOVE.W (d16,An),(An) */
uae_u32 REGPARAM2 CPUFUNC(op_30a8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (4);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.W (d8,An,Xn),(An) */
uae_u32 REGPARAM2 CPUFUNC(op_30b0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (4);
}}}}return 18 * CYCLE_UNIT / 2;
}

/* MOVE.W (xxx).W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_30b8_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (4);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.W (xxx).L,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_30b9_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (6);
}}}}return 20 * CYCLE_UNIT / 2;
}

/* MOVE.W (d16,PC),(An) */
uae_u32 REGPARAM2 CPUFUNC(op_30ba_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (4);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.W (d8,PC,Xn),(An) */
uae_u32 REGPARAM2 CPUFUNC(op_30bb_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (4);
}}}}return 18 * CYCLE_UNIT / 2;
}

/* MOVE.W #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_30bc_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (4);
}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.W Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_30c0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 2;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (2);
}}}return 8 * CYCLE_UNIT / 2;
}

/* MOVE.W An,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_30c8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_areg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 2;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (2);
}}}return 8 * CYCLE_UNIT / 2;
}

/* MOVE.W (An),(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_30d0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 2;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (2);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.W (An)+,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_30d8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) += 2;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 2;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (2);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.W -(An),(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_30e0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 2;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (2);
}}}}return 14 * CYCLE_UNIT / 2;
}

/* MOVE.W (d16,An),(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_30e8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 2;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (4);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.W (d8,An,Xn),(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_30f0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 2;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (4);
}}}}return 18 * CYCLE_UNIT / 2;
}

/* MOVE.W (xxx).W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_30f8_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 2;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (4);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.W (xxx).L,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_30f9_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 2;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (6);
}}}}return 20 * CYCLE_UNIT / 2;
}

/* MOVE.W (d16,PC),(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_30fa_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 2;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (4);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.W (d8,PC,Xn),(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_30fb_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 2;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (4);
}}}}return 18 * CYCLE_UNIT / 2;
}

/* MOVE.W #<data>.W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_30fc_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 2;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (4);
}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.W Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_3100_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (2);
}}}return 8 * CYCLE_UNIT / 2;
}

/* MOVE.W An,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_3108_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_areg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (2);
}}}return 8 * CYCLE_UNIT / 2;
}

/* MOVE.W (An),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_3110_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (2);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.W (An)+,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_3118_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) += 2;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (2);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.W -(An),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_3120_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (2);
}}}}return 14 * CYCLE_UNIT / 2;
}

/* MOVE.W (d16,An),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_3128_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (4);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.W (d8,An,Xn),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_3130_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (4);
}}}}return 18 * CYCLE_UNIT / 2;
}

/* MOVE.W (xxx).W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_3138_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (4);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.W (xxx).L,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_3139_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (6);
}}}}return 20 * CYCLE_UNIT / 2;
}

/* MOVE.W (d16,PC),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_313a_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (4);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.W (d8,PC,Xn),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_313b_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (4);
}}}}return 18 * CYCLE_UNIT / 2;
}

/* MOVE.W #<data>.W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_313c_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (4);
}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.W Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_3140_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (4);
}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.W An,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_3148_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_areg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (4);
}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.W (An),(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_3150_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (4);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.W (An)+,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_3158_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) += 2;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (4);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.W -(An),(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_3160_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (4);
}}}}return 18 * CYCLE_UNIT / 2;
}

/* MOVE.W (d16,An),(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_3168_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (6);
}}}}return 20 * CYCLE_UNIT / 2;
}

/* MOVE.W (d8,An,Xn),(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_3170_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (6);
}}}}return 22 * CYCLE_UNIT / 2;
}

/* MOVE.W (xxx).W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_3178_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (6);
}}}}return 20 * CYCLE_UNIT / 2;
}

/* MOVE.W (xxx).L,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_3179_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (6);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (8);
}}}}return 24 * CYCLE_UNIT / 2;
}

/* MOVE.W (d16,PC),(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_317a_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (6);
}}}}return 20 * CYCLE_UNIT / 2;
}

/* MOVE.W (d8,PC,Xn),(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_317b_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (6);
}}}}return 22 * CYCLE_UNIT / 2;
}

/* MOVE.W #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_317c_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (6);
}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.W Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_3180_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (2));
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (4);
}}}return 14 * CYCLE_UNIT / 2;
}

/* MOVE.W An,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_3188_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_areg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (2));
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (4);
}}}return 14 * CYCLE_UNIT / 2;
}

/* MOVE.W (An),(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_3190_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (2));
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (4);
}}}}return 18 * CYCLE_UNIT / 2;
}

/* MOVE.W (An)+,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_3198_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) += 2;
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (2));
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (4);
}}}}return 18 * CYCLE_UNIT / 2;
}

/* MOVE.W -(An),(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_31a0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (2));
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (4);
}}}}return 20 * CYCLE_UNIT / 2;
}

/* MOVE.W (d16,An),(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_31a8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (4));
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (6);
}}}}return 22 * CYCLE_UNIT / 2;
}

/* MOVE.W (d8,An,Xn),(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_31b0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (4));
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (6);
}}}}return 24 * CYCLE_UNIT / 2;
}

/* MOVE.W (xxx).W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_31b8_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (4));
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (6);
}}}}return 22 * CYCLE_UNIT / 2;
}

/* MOVE.W (xxx).L,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_31b9_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (6));
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (8);
}}}}return 26 * CYCLE_UNIT / 2;
}

/* MOVE.W (d16,PC),(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_31ba_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (4));
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (6);
}}}}return 22 * CYCLE_UNIT / 2;
}

/* MOVE.W (d8,PC,Xn),(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_31bb_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (4));
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (6);
}}}}return 24 * CYCLE_UNIT / 2;
}

/* MOVE.W #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_31bc_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (4));
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (6);
}}}return 18 * CYCLE_UNIT / 2;
}

/* MOVE.W Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_31c0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (4);
}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.W An,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_31c8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_areg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (4);
}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.W (An),(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_31d0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (4);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.W (An)+,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_31d8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) += 2;
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (4);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.W -(An),(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_31e0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (4);
}}}}return 18 * CYCLE_UNIT / 2;
}

/* MOVE.W (d16,An),(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_31e8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (6);
}}}}return 20 * CYCLE_UNIT / 2;
}

/* MOVE.W (d8,An,Xn),(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_31f0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (6);
}}}}return 22 * CYCLE_UNIT / 2;
}

/* MOVE.W (xxx).W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_31f8_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (6);
}}}}return 20 * CYCLE_UNIT / 2;
}

/* MOVE.W (xxx).L,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_31f9_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (6);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (8);
}}}}return 24 * CYCLE_UNIT / 2;
}

/* MOVE.W (d16,PC),(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_31fa_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (6);
}}}}return 20 * CYCLE_UNIT / 2;
}

/* MOVE.W (d8,PC,Xn),(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_31fb_4)(uae_u32 opcode)
{
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (6);
}}}}return 22 * CYCLE_UNIT / 2;
}

/* MOVE.W #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_31fc_4)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (6);
}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.W Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_33c0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (6);
}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.W An,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_33c8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_areg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (6);
}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.W (An),(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_33d0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = get_dilong (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (6);
}}}}return 20 * CYCLE_UNIT / 2;
}

/* MOVE.W (An)+,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_33d8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) += 2;
{	uaecptr dsta;
	dsta = get_dilong (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (6);
}}}}return 20 * CYCLE_UNIT / 2;
}

/* MOVE.W -(An),(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_33e0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = get_dilong (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (6);
}}}}return 22 * CYCLE_UNIT / 2;
}

/* MOVE.W (d16,An),(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_33e8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = get_dilong (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (8);
}}}}return 24 * CYCLE_UNIT / 2;
}

/* MOVE.W (d8,An,Xn),(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_33f0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = get_dilong (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (8);
}}}}return 26 * CYCLE_UNIT / 2;
}

/* MOVE.W (xxx).W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_33f8_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = get_dilong (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (8);
}}}}return 24 * CYCLE_UNIT / 2;
}

/* MOVE.W (xxx).L,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_33f9_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = get_dilong (6);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (10);
}}}}return 28 * CYCLE_UNIT / 2;
}

/* MOVE.W (d16,PC),(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_33fa_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = get_dilong (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (8);
}}}}return 24 * CYCLE_UNIT / 2;
}

/* MOVE.W (d8,PC,Xn),(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_33fb_4)(uae_u32 opcode)
{
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = get_dilong (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (8);
}}}}return 26 * CYCLE_UNIT / 2;
}

/* MOVE.W #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_33fc_4)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
	m68k_incpc (8);
}}}return 20 * CYCLE_UNIT / 2;
}

/* NEGX.B Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4000_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s8)(newv)) == 0));
	SET_NFLG (((uae_s8)(newv)) < 0);
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((newv) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* NEGX.B (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4010_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s8)(newv)) == 0));
	SET_NFLG (((uae_s8)(newv)) < 0);
	put_byte (srca, newv);
}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* NEGX.B (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4018_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{	uae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s8)(newv)) == 0));
	SET_NFLG (((uae_s8)(newv)) < 0);
	put_byte (srca, newv);
}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* NEGX.B -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_4020_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s8)(newv)) == 0));
	SET_NFLG (((uae_s8)(newv)) < 0);
	put_byte (srca, newv);
}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* NEGX.B (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4028_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s8)(newv)) == 0));
	SET_NFLG (((uae_s8)(newv)) < 0);
	put_byte (srca, newv);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* NEGX.B (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4030_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s8 src = get_byte (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s8)(newv)) == 0));
	SET_NFLG (((uae_s8)(newv)) < 0);
	put_byte (srca, newv);
}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* NEGX.B (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4038_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s8)(newv)) == 0));
	SET_NFLG (((uae_s8)(newv)) < 0);
	put_byte (srca, newv);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* NEGX.B (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4039_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s8)(newv)) == 0));
	SET_NFLG (((uae_s8)(newv)) < 0);
	put_byte (srca, newv);
}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* NEGX.W Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4040_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s16)(newv)) == 0));
	SET_NFLG (((uae_s16)(newv)) < 0);
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | ((newv) & 0xffff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* NEGX.W (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4050_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s16)(newv)) == 0));
	SET_NFLG (((uae_s16)(newv)) < 0);
	put_word (srca, newv);
}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* NEGX.W (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4058_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) += 2;
{	uae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s16)(newv)) == 0));
	SET_NFLG (((uae_s16)(newv)) < 0);
	put_word (srca, newv);
}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* NEGX.W -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_4060_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s16)(newv)) == 0));
	SET_NFLG (((uae_s16)(newv)) < 0);
	put_word (srca, newv);
}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* NEGX.W (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4068_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s16)(newv)) == 0));
	SET_NFLG (((uae_s16)(newv)) < 0);
	put_word (srca, newv);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* NEGX.W (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4070_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s16 src = get_word (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s16)(newv)) == 0));
	SET_NFLG (((uae_s16)(newv)) < 0);
	put_word (srca, newv);
}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* NEGX.W (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4078_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s16)(newv)) == 0));
	SET_NFLG (((uae_s16)(newv)) < 0);
	put_word (srca, newv);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* NEGX.W (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4079_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s16)(newv)) == 0));
	SET_NFLG (((uae_s16)(newv)) < 0);
	put_word (srca, newv);
}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* NEGX.L Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4080_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s32)(newv)) == 0));
	SET_NFLG (((uae_s32)(newv)) < 0);
	m68k_dreg (regs, srcreg) = (newv);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* NEGX.L (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4090_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s32)(newv)) == 0));
	SET_NFLG (((uae_s32)(newv)) < 0);
	put_long (srca, newv);
}}}}}	m68k_incpc (2);
return 20 * CYCLE_UNIT / 2;
}

/* NEGX.L (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4098_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) += 4;
{	uae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s32)(newv)) == 0));
	SET_NFLG (((uae_s32)(newv)) < 0);
	put_long (srca, newv);
}}}}}	m68k_incpc (2);
return 20 * CYCLE_UNIT / 2;
}

/* NEGX.L -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_40a0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s32)(newv)) == 0));
	SET_NFLG (((uae_s32)(newv)) < 0);
	put_long (srca, newv);
}}}}}	m68k_incpc (2);
return 22 * CYCLE_UNIT / 2;
}

/* NEGX.L (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_40a8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s32)(newv)) == 0));
	SET_NFLG (((uae_s32)(newv)) < 0);
	put_long (srca, newv);
}}}}}	m68k_incpc (4);
return 24 * CYCLE_UNIT / 2;
}

/* NEGX.L (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_40b0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s32 src = get_long (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s32)(newv)) == 0));
	SET_NFLG (((uae_s32)(newv)) < 0);
	put_long (srca, newv);
}}}}}	m68k_incpc (4);
return 26 * CYCLE_UNIT / 2;
}

/* NEGX.L (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_40b8_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s32)(newv)) == 0));
	SET_NFLG (((uae_s32)(newv)) < 0);
	put_long (srca, newv);
}}}}}	m68k_incpc (4);
return 24 * CYCLE_UNIT / 2;
}

/* NEGX.L (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_40b9_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s32)(newv)) == 0));
	SET_NFLG (((uae_s32)(newv)) < 0);
	put_long (srca, newv);
}}}}}	m68k_incpc (6);
return 28 * CYCLE_UNIT / 2;
}

/* MVSR2.W Dn */
uae_u32 REGPARAM2 CPUFUNC(op_40c0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	MakeSR ();
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | ((regs.sr) & 0xffff);
}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* MVSR2.W (An) */
uae_u32 REGPARAM2 CPUFUNC(op_40d0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	put_word (srca, regs.sr | 0x0010);
	MakeSR ();
	put_word (srca, regs.sr);
}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* MVSR2.W (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_40d8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += 2;
	put_word (srca, regs.sr | 0x0010);
	MakeSR ();
	put_word (srca, regs.sr);
}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* MVSR2.W -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_40e0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
	m68k_areg (regs, srcreg) = srca;
	put_word (srca, regs.sr | 0x0010);
	MakeSR ();
	put_word (srca, regs.sr);
}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* MVSR2.W (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_40e8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	put_word (srca, regs.sr | 0x0010);
	MakeSR ();
	put_word (srca, regs.sr);
}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* MVSR2.W (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_40f0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
	put_word (srca, regs.sr | 0x0010);
	MakeSR ();
	put_word (srca, regs.sr);
}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* MVSR2.W (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_40f8_4)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	put_word (srca, regs.sr | 0x0010);
	MakeSR ();
	put_word (srca, regs.sr);
}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* MVSR2.W (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_40f9_4)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uaecptr srca;
	srca = get_dilong (2);
	put_word (srca, regs.sr | 0x0010);
	MakeSR ();
	put_word (srca, regs.sr);
}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* CHK.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4180_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	m68k_incpc (2);
	if (dst > src) {
		SET_NFLG (0);
		Exception (6);
		return 8 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		SET_NFLG (1);
		Exception (6);
		return 10 * CYCLE_UNIT / 2;
	}
}}}return 10 * CYCLE_UNIT / 2;
}

/* CHK.W (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4190_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	m68k_incpc (2);
	if (dst > src) {
		SET_NFLG (0);
		Exception (6);
		return 12 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		SET_NFLG (1);
		Exception (6);
		return 14 * CYCLE_UNIT / 2;
	}
}}}}return 14 * CYCLE_UNIT / 2;
}

/* CHK.W (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4198_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) += 2;
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	m68k_incpc (2);
	if (dst > src) {
		SET_NFLG (0);
		Exception (6);
		return 12 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		SET_NFLG (1);
		Exception (6);
		return 14 * CYCLE_UNIT / 2;
	}
}}}}return 14 * CYCLE_UNIT / 2;
}

/* CHK.W -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_41a0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	m68k_incpc (2);
	if (dst > src) {
		SET_NFLG (0);
		Exception (6);
		return 14 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		SET_NFLG (1);
		Exception (6);
		return 16 * CYCLE_UNIT / 2;
	}
}}}}return 16 * CYCLE_UNIT / 2;
}

/* CHK.W (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_41a8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	m68k_incpc (4);
	if (dst > src) {
		SET_NFLG (0);
		Exception (6);
		return 16 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		SET_NFLG (1);
		Exception (6);
		return 18 * CYCLE_UNIT / 2;
	}
}}}}return 18 * CYCLE_UNIT / 2;
}

/* CHK.W (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_41b0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	m68k_incpc (4);
	if (dst > src) {
		SET_NFLG (0);
		Exception (6);
		return 18 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		SET_NFLG (1);
		Exception (6);
		return 20 * CYCLE_UNIT / 2;
	}
}}}}return 20 * CYCLE_UNIT / 2;
}

/* CHK.W (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_41b8_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	m68k_incpc (4);
	if (dst > src) {
		SET_NFLG (0);
		Exception (6);
		return 16 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		SET_NFLG (1);
		Exception (6);
		return 18 * CYCLE_UNIT / 2;
	}
}}}}return 18 * CYCLE_UNIT / 2;
}

/* CHK.W (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_41b9_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	m68k_incpc (6);
	if (dst > src) {
		SET_NFLG (0);
		Exception (6);
		return 20 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		SET_NFLG (1);
		Exception (6);
		return 22 * CYCLE_UNIT / 2;
	}
}}}}return 22 * CYCLE_UNIT / 2;
}

/* CHK.W (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_41ba_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	m68k_incpc (4);
	if (dst > src) {
		SET_NFLG (0);
		Exception (6);
		return 16 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		SET_NFLG (1);
		Exception (6);
		return 18 * CYCLE_UNIT / 2;
	}
}}}}return 18 * CYCLE_UNIT / 2;
}

/* CHK.W (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_41bb_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	m68k_incpc (4);
	if (dst > src) {
		SET_NFLG (0);
		Exception (6);
		return 18 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		SET_NFLG (1);
		Exception (6);
		return 20 * CYCLE_UNIT / 2;
	}
}}}}return 20 * CYCLE_UNIT / 2;
}

/* CHK.W #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_41bc_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	m68k_incpc (4);
	if (dst > src) {
		SET_NFLG (0);
		Exception (6);
		return 12 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		SET_NFLG (1);
		Exception (6);
		return 14 * CYCLE_UNIT / 2;
	}
}}}return 14 * CYCLE_UNIT / 2;
}

/* LEA.L (An),An */
uae_u32 REGPARAM2 CPUFUNC(op_41d0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	m68k_areg (regs, dstreg) = (srca);
}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* LEA.L (d16,An),An */
uae_u32 REGPARAM2 CPUFUNC(op_41e8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	m68k_areg (regs, dstreg) = (srca);
}}}	m68k_incpc (4);
return 8 * CYCLE_UNIT / 2;
}

/* LEA.L (d8,An,Xn),An */
uae_u32 REGPARAM2 CPUFUNC(op_41f0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	m68k_areg (regs, dstreg) = (srca);
}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* LEA.L (xxx).W,An */
uae_u32 REGPARAM2 CPUFUNC(op_41f8_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	m68k_areg (regs, dstreg) = (srca);
}}}	m68k_incpc (4);
return 8 * CYCLE_UNIT / 2;
}

/* LEA.L (xxx).L,An */
uae_u32 REGPARAM2 CPUFUNC(op_41f9_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	m68k_areg (regs, dstreg) = (srca);
}}}	m68k_incpc (6);
return 12 * CYCLE_UNIT / 2;
}

/* LEA.L (d16,PC),An */
uae_u32 REGPARAM2 CPUFUNC(op_41fa_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	m68k_areg (regs, dstreg) = (srca);
}}}	m68k_incpc (4);
return 8 * CYCLE_UNIT / 2;
}

/* LEA.L (d8,PC,Xn),An */
uae_u32 REGPARAM2 CPUFUNC(op_41fb_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	m68k_areg (regs, dstreg) = (srca);
}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* CLR.B Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4200_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(0)) == 0);
	SET_NFLG   (((uae_s8)(0)) < 0);
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((0) & 0xff);
}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* CLR.B (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4210_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(0)) == 0);
	SET_NFLG   (((uae_s8)(0)) < 0);
	put_byte (srca, 0);
}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* CLR.B (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4218_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(0)) == 0);
	SET_NFLG   (((uae_s8)(0)) < 0);
	put_byte (srca, 0);
}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* CLR.B -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_4220_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(0)) == 0);
	SET_NFLG   (((uae_s8)(0)) < 0);
	put_byte (srca, 0);
}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* CLR.B (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4228_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(0)) == 0);
	SET_NFLG   (((uae_s8)(0)) < 0);
	put_byte (srca, 0);
}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* CLR.B (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4230_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(0)) == 0);
	SET_NFLG   (((uae_s8)(0)) < 0);
	put_byte (srca, 0);
}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* CLR.B (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4238_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(0)) == 0);
	SET_NFLG   (((uae_s8)(0)) < 0);
	put_byte (srca, 0);
}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* CLR.B (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4239_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(0)) == 0);
	SET_NFLG   (((uae_s8)(0)) < 0);
	put_byte (srca, 0);
}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* CLR.W Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4240_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(0)) == 0);
	SET_NFLG   (((uae_s16)(0)) < 0);
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | ((0) & 0xffff);
}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* CLR.W (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4250_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(0)) == 0);
	SET_NFLG   (((uae_s16)(0)) < 0);
	put_word (srca, 0);
}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* CLR.W (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4258_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += 2;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(0)) == 0);
	SET_NFLG   (((uae_s16)(0)) < 0);
	put_word (srca, 0);
}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* CLR.W -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_4260_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
	m68k_areg (regs, srcreg) = srca;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(0)) == 0);
	SET_NFLG   (((uae_s16)(0)) < 0);
	put_word (srca, 0);
}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* CLR.W (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4268_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(0)) == 0);
	SET_NFLG   (((uae_s16)(0)) < 0);
	put_word (srca, 0);
}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* CLR.W (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4270_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(0)) == 0);
	SET_NFLG   (((uae_s16)(0)) < 0);
	put_word (srca, 0);
}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* CLR.W (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4278_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(0)) == 0);
	SET_NFLG   (((uae_s16)(0)) < 0);
	put_word (srca, 0);
}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* CLR.W (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4279_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(0)) == 0);
	SET_NFLG   (((uae_s16)(0)) < 0);
	put_word (srca, 0);
}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* CLR.L Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4280_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(0)) == 0);
	SET_NFLG   (((uae_s32)(0)) < 0);
	m68k_dreg (regs, srcreg) = (0);
}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* CLR.L (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4290_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(0)) == 0);
	SET_NFLG   (((uae_s32)(0)) < 0);
	put_long (srca, 0);
}}	m68k_incpc (2);
return 20 * CYCLE_UNIT / 2;
}

/* CLR.L (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4298_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += 4;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(0)) == 0);
	SET_NFLG   (((uae_s32)(0)) < 0);
	put_long (srca, 0);
}}	m68k_incpc (2);
return 20 * CYCLE_UNIT / 2;
}

/* CLR.L -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_42a0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
	m68k_areg (regs, srcreg) = srca;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(0)) == 0);
	SET_NFLG   (((uae_s32)(0)) < 0);
	put_long (srca, 0);
}}	m68k_incpc (2);
return 22 * CYCLE_UNIT / 2;
}

/* CLR.L (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_42a8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(0)) == 0);
	SET_NFLG   (((uae_s32)(0)) < 0);
	put_long (srca, 0);
}}	m68k_incpc (4);
return 24 * CYCLE_UNIT / 2;
}

/* CLR.L (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_42b0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(0)) == 0);
	SET_NFLG   (((uae_s32)(0)) < 0);
	put_long (srca, 0);
}}	m68k_incpc (4);
return 26 * CYCLE_UNIT / 2;
}

/* CLR.L (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_42b8_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(0)) == 0);
	SET_NFLG   (((uae_s32)(0)) < 0);
	put_long (srca, 0);
}}	m68k_incpc (4);
return 24 * CYCLE_UNIT / 2;
}

/* CLR.L (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_42b9_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(0)) == 0);
	SET_NFLG   (((uae_s32)(0)) < 0);
	put_long (srca, 0);
}}	m68k_incpc (6);
return 28 * CYCLE_UNIT / 2;
}

/* MVSR2.B Dn */
uae_u32 REGPARAM2 CPUFUNC(op_42c0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	MakeSR ();
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | ((regs.sr & 0xff) & 0xffff);
}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* MVSR2.B (An) */
uae_u32 REGPARAM2 CPUFUNC(op_42d0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	MakeSR ();
	put_word (srca, regs.sr & 0xff);
}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* MVSR2.B (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_42d8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += 2;
	MakeSR ();
	put_word (srca, regs.sr & 0xff);
}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* MVSR2.B -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_42e0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
	m68k_areg (regs, srcreg) = srca;
	MakeSR ();
	put_word (srca, regs.sr & 0xff);
}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* MVSR2.B (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_42e8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	MakeSR ();
	put_word (srca, regs.sr & 0xff);
}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* MVSR2.B (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_42f0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
	MakeSR ();
	put_word (srca, regs.sr & 0xff);
}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* MVSR2.B (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_42f8_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	MakeSR ();
	put_word (srca, regs.sr & 0xff);
}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* MVSR2.B (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_42f9_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
	MakeSR ();
	put_word (srca, regs.sr & 0xff);
}}	m68k_incpc (6);
return 16 * CYCLE_UNIT / 2;
}

#endif

#ifdef PART_4
/* NEG.B Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4400_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{{uae_u32 dst = ((uae_s8)(0)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(dst)) < 0;
	SET_ZFLG (((uae_s8)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((dst) & 0xff);
}}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* NEG.B (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4410_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
{{uae_u32 dst = ((uae_s8)(0)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(dst)) < 0;
	SET_ZFLG (((uae_s8)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (srca, dst);
}}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* NEG.B (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4418_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{uae_u32 dst = ((uae_s8)(0)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(dst)) < 0;
	SET_ZFLG (((uae_s8)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (srca, dst);
}}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* NEG.B -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_4420_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{{uae_u32 dst = ((uae_s8)(0)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(dst)) < 0;
	SET_ZFLG (((uae_s8)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (srca, dst);
}}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* NEG.B (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4428_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{{uae_u32 dst = ((uae_s8)(0)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(dst)) < 0;
	SET_ZFLG (((uae_s8)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (srca, dst);
}}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* NEG.B (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4430_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s8 src = get_byte (srca);
{{uae_u32 dst = ((uae_s8)(0)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(dst)) < 0;
	SET_ZFLG (((uae_s8)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (srca, dst);
}}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* NEG.B (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4438_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{{uae_u32 dst = ((uae_s8)(0)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(dst)) < 0;
	SET_ZFLG (((uae_s8)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (srca, dst);
}}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* NEG.B (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4439_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte (srca);
{{uae_u32 dst = ((uae_s8)(0)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(dst)) < 0;
	SET_ZFLG (((uae_s8)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (srca, dst);
}}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* NEG.W Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4440_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{{uae_u32 dst = ((uae_s16)(0)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(dst)) < 0;
	SET_ZFLG (((uae_s16)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | ((dst) & 0xffff);
}}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* NEG.W (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4450_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
{{uae_u32 dst = ((uae_s16)(0)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(dst)) < 0;
	SET_ZFLG (((uae_s16)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (srca, dst);
}}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* NEG.W (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4458_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) += 2;
{{uae_u32 dst = ((uae_s16)(0)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(dst)) < 0;
	SET_ZFLG (((uae_s16)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (srca, dst);
}}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* NEG.W -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_4460_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{{uae_u32 dst = ((uae_s16)(0)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(dst)) < 0;
	SET_ZFLG (((uae_s16)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (srca, dst);
}}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* NEG.W (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4468_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{{uae_u32 dst = ((uae_s16)(0)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(dst)) < 0;
	SET_ZFLG (((uae_s16)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (srca, dst);
}}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* NEG.W (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4470_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s16 src = get_word (srca);
{{uae_u32 dst = ((uae_s16)(0)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(dst)) < 0;
	SET_ZFLG (((uae_s16)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (srca, dst);
}}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* NEG.W (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4478_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{{uae_u32 dst = ((uae_s16)(0)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(dst)) < 0;
	SET_ZFLG (((uae_s16)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (srca, dst);
}}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* NEG.W (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4479_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word (srca);
{{uae_u32 dst = ((uae_s16)(0)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(dst)) < 0;
	SET_ZFLG (((uae_s16)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (srca, dst);
}}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* NEG.L Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4480_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{{uae_u32 dst = ((uae_s32)(0)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(dst)) < 0;
	SET_ZFLG (((uae_s32)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, srcreg) = (dst);
}}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* NEG.L (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4490_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
{{uae_u32 dst = ((uae_s32)(0)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(dst)) < 0;
	SET_ZFLG (((uae_s32)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (srca, dst);
}}}}}}	m68k_incpc (2);
return 20 * CYCLE_UNIT / 2;
}

/* NEG.L (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4498_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) += 4;
{{uae_u32 dst = ((uae_s32)(0)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(dst)) < 0;
	SET_ZFLG (((uae_s32)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (srca, dst);
}}}}}}	m68k_incpc (2);
return 20 * CYCLE_UNIT / 2;
}

/* NEG.L -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_44a0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{{uae_u32 dst = ((uae_s32)(0)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(dst)) < 0;
	SET_ZFLG (((uae_s32)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (srca, dst);
}}}}}}	m68k_incpc (2);
return 22 * CYCLE_UNIT / 2;
}

/* NEG.L (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_44a8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{{uae_u32 dst = ((uae_s32)(0)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(dst)) < 0;
	SET_ZFLG (((uae_s32)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (srca, dst);
}}}}}}	m68k_incpc (4);
return 24 * CYCLE_UNIT / 2;
}

/* NEG.L (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_44b0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s32 src = get_long (srca);
{{uae_u32 dst = ((uae_s32)(0)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(dst)) < 0;
	SET_ZFLG (((uae_s32)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (srca, dst);
}}}}}}	m68k_incpc (4);
return 26 * CYCLE_UNIT / 2;
}

/* NEG.L (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_44b8_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{{uae_u32 dst = ((uae_s32)(0)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(dst)) < 0;
	SET_ZFLG (((uae_s32)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (srca, dst);
}}}}}}	m68k_incpc (4);
return 24 * CYCLE_UNIT / 2;
}

/* NEG.L (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_44b9_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long (srca);
{{uae_u32 dst = ((uae_s32)(0)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(dst)) < 0;
	SET_ZFLG (((uae_s32)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (srca, dst);
}}}}}}	m68k_incpc (6);
return 28 * CYCLE_UNIT / 2;
}

/* MV2SR.B Dn */
uae_u32 REGPARAM2 CPUFUNC(op_44c0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
	MakeSR ();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR ();
	m68k_incpc (2);
}}return 12 * CYCLE_UNIT / 2;
}

/* MV2SR.B (An) */
uae_u32 REGPARAM2 CPUFUNC(op_44d0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
	MakeSR ();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR ();
	m68k_incpc (2);
}}}return 16 * CYCLE_UNIT / 2;
}

/* MV2SR.B (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_44d8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) += 2;
	MakeSR ();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR ();
	m68k_incpc (2);
}}}return 16 * CYCLE_UNIT / 2;
}

/* MV2SR.B -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_44e0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
	MakeSR ();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR ();
	m68k_incpc (2);
}}}return 18 * CYCLE_UNIT / 2;
}

/* MV2SR.B (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_44e8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
	MakeSR ();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR ();
	m68k_incpc (4);
}}}return 20 * CYCLE_UNIT / 2;
}

/* MV2SR.B (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_44f0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s16 src = get_word (srca);
	MakeSR ();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR ();
	m68k_incpc (4);
}}}return 22 * CYCLE_UNIT / 2;
}

/* MV2SR.B (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_44f8_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
	MakeSR ();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR ();
	m68k_incpc (4);
}}}return 20 * CYCLE_UNIT / 2;
}

/* MV2SR.B (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_44f9_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word (srca);
	MakeSR ();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR ();
	m68k_incpc (6);
}}}return 24 * CYCLE_UNIT / 2;
}

/* MV2SR.B (d16,PC) */
uae_u32 REGPARAM2 CPUFUNC(op_44fa_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
	MakeSR ();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR ();
	m68k_incpc (4);
}}}return 20 * CYCLE_UNIT / 2;
}

/* MV2SR.B (d8,PC,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_44fb_4)(uae_u32 opcode)
{
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s16 src = get_word (srca);
	MakeSR ();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR ();
	m68k_incpc (4);
}}}return 22 * CYCLE_UNIT / 2;
}

/* MV2SR.B #<data>.B */
uae_u32 REGPARAM2 CPUFUNC(op_44fc_4)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
	MakeSR ();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR ();
	m68k_incpc (4);
}}return 16 * CYCLE_UNIT / 2;
}

/* NOT.B Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4600_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(dst)) == 0);
	SET_NFLG   (((uae_s8)(dst)) < 0);
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((dst) & 0xff);
}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* NOT.B (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4610_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(dst)) == 0);
	SET_NFLG   (((uae_s8)(dst)) < 0);
	put_byte (srca, dst);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* NOT.B (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4618_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(dst)) == 0);
	SET_NFLG   (((uae_s8)(dst)) < 0);
	put_byte (srca, dst);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* NOT.B -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_4620_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(dst)) == 0);
	SET_NFLG   (((uae_s8)(dst)) < 0);
	put_byte (srca, dst);
}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* NOT.B (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4628_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(dst)) == 0);
	SET_NFLG   (((uae_s8)(dst)) < 0);
	put_byte (srca, dst);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* NOT.B (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4630_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s8 src = get_byte (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(dst)) == 0);
	SET_NFLG   (((uae_s8)(dst)) < 0);
	put_byte (srca, dst);
}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* NOT.B (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4638_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(dst)) == 0);
	SET_NFLG   (((uae_s8)(dst)) < 0);
	put_byte (srca, dst);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* NOT.B (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4639_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(dst)) == 0);
	SET_NFLG   (((uae_s8)(dst)) < 0);
	put_byte (srca, dst);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* NOT.W Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4640_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(dst)) == 0);
	SET_NFLG   (((uae_s16)(dst)) < 0);
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | ((dst) & 0xffff);
}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* NOT.W (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4650_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(dst)) == 0);
	SET_NFLG   (((uae_s16)(dst)) < 0);
	put_word (srca, dst);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* NOT.W (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4658_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) += 2;
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(dst)) == 0);
	SET_NFLG   (((uae_s16)(dst)) < 0);
	put_word (srca, dst);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* NOT.W -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_4660_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(dst)) == 0);
	SET_NFLG   (((uae_s16)(dst)) < 0);
	put_word (srca, dst);
}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* NOT.W (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4668_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(dst)) == 0);
	SET_NFLG   (((uae_s16)(dst)) < 0);
	put_word (srca, dst);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* NOT.W (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4670_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s16 src = get_word (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(dst)) == 0);
	SET_NFLG   (((uae_s16)(dst)) < 0);
	put_word (srca, dst);
}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* NOT.W (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4678_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(dst)) == 0);
	SET_NFLG   (((uae_s16)(dst)) < 0);
	put_word (srca, dst);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* NOT.W (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4679_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(dst)) == 0);
	SET_NFLG   (((uae_s16)(dst)) < 0);
	put_word (srca, dst);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* NOT.L Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4680_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(dst)) == 0);
	SET_NFLG   (((uae_s32)(dst)) < 0);
	m68k_dreg (regs, srcreg) = (dst);
}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* NOT.L (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4690_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(dst)) == 0);
	SET_NFLG   (((uae_s32)(dst)) < 0);
	put_long (srca, dst);
}}}}	m68k_incpc (2);
return 20 * CYCLE_UNIT / 2;
}

/* NOT.L (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4698_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) += 4;
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(dst)) == 0);
	SET_NFLG   (((uae_s32)(dst)) < 0);
	put_long (srca, dst);
}}}}	m68k_incpc (2);
return 20 * CYCLE_UNIT / 2;
}

/* NOT.L -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_46a0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(dst)) == 0);
	SET_NFLG   (((uae_s32)(dst)) < 0);
	put_long (srca, dst);
}}}}	m68k_incpc (2);
return 22 * CYCLE_UNIT / 2;
}

/* NOT.L (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_46a8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(dst)) == 0);
	SET_NFLG   (((uae_s32)(dst)) < 0);
	put_long (srca, dst);
}}}}	m68k_incpc (4);
return 24 * CYCLE_UNIT / 2;
}

/* NOT.L (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_46b0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s32 src = get_long (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(dst)) == 0);
	SET_NFLG   (((uae_s32)(dst)) < 0);
	put_long (srca, dst);
}}}}	m68k_incpc (4);
return 26 * CYCLE_UNIT / 2;
}

/* NOT.L (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_46b8_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(dst)) == 0);
	SET_NFLG   (((uae_s32)(dst)) < 0);
	put_long (srca, dst);
}}}}	m68k_incpc (4);
return 24 * CYCLE_UNIT / 2;
}

/* NOT.L (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_46b9_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(dst)) == 0);
	SET_NFLG   (((uae_s32)(dst)) < 0);
	put_long (srca, dst);
}}}}	m68k_incpc (6);
return 28 * CYCLE_UNIT / 2;
}

/* MV2SR.W Dn */
uae_u32 REGPARAM2 CPUFUNC(op_46c0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uae_s16 src = m68k_dreg (regs, srcreg);
	regs.sr = src;
	MakeFromSR ();
	m68k_incpc (2);
}}}return 12 * CYCLE_UNIT / 2;
}

/* MV2SR.W (An) */
uae_u32 REGPARAM2 CPUFUNC(op_46d0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
	regs.sr = src;
	MakeFromSR ();
	m68k_incpc (2);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MV2SR.W (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_46d8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) += 2;
	regs.sr = src;
	MakeFromSR ();
	m68k_incpc (2);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MV2SR.W -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_46e0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
	regs.sr = src;
	MakeFromSR ();
	m68k_incpc (2);
}}}}return 18 * CYCLE_UNIT / 2;
}

/* MV2SR.W (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_46e8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
	regs.sr = src;
	MakeFromSR ();
	m68k_incpc (4);
}}}}return 20 * CYCLE_UNIT / 2;
}

/* MV2SR.W (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_46f0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s16 src = get_word (srca);
	regs.sr = src;
	MakeFromSR ();
	m68k_incpc (4);
}}}}return 22 * CYCLE_UNIT / 2;
}

/* MV2SR.W (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_46f8_4)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
	regs.sr = src;
	MakeFromSR ();
	m68k_incpc (4);
}}}}return 20 * CYCLE_UNIT / 2;
}

/* MV2SR.W (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_46f9_4)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word (srca);
	regs.sr = src;
	MakeFromSR ();
	m68k_incpc (6);
}}}}return 24 * CYCLE_UNIT / 2;
}

/* MV2SR.W (d16,PC) */
uae_u32 REGPARAM2 CPUFUNC(op_46fa_4)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
	regs.sr = src;
	MakeFromSR ();
	m68k_incpc (4);
}}}}return 20 * CYCLE_UNIT / 2;
}

/* MV2SR.W (d8,PC,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_46fb_4)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s16 src = get_word (srca);
	regs.sr = src;
	MakeFromSR ();
	m68k_incpc (4);
}}}}return 22 * CYCLE_UNIT / 2;
}

/* MV2SR.W #<data>.W */
uae_u32 REGPARAM2 CPUFUNC(op_46fc_4)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uae_s16 src = get_diword (2);
	regs.sr = src;
	MakeFromSR ();
	m68k_incpc (4);
}}}return 16 * CYCLE_UNIT / 2;
}

/* NBCD.B Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4800_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uae_u16 newv_lo = - (src & 0xF) - (GET_XFLG () ? 1 : 0);
	uae_u16 newv_hi = - (src & 0xF0);
	uae_u16 newv;
	int cflg, tmp_newv;
	if (newv_lo > 9) { newv_lo -= 6; }
	tmp_newv = newv = newv_hi + newv_lo;
	cflg = (newv & 0x1F0) > 0x90;
	if (cflg) newv -= 0x60;
	SET_CFLG (cflg);
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s8)(newv)) == 0));
	SET_NFLG (((uae_s8)(newv)) < 0);
	SET_VFLG ((tmp_newv & 0x80) != 0 && (newv & 0x80) == 0);
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((newv) & 0xff);
}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* NBCD.B (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4810_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
{	uae_u16 newv_lo = - (src & 0xF) - (GET_XFLG () ? 1 : 0);
	uae_u16 newv_hi = - (src & 0xF0);
	uae_u16 newv;
	int cflg, tmp_newv;
	if (newv_lo > 9) { newv_lo -= 6; }
	tmp_newv = newv = newv_hi + newv_lo;
	cflg = (newv & 0x1F0) > 0x90;
	if (cflg) newv -= 0x60;
	SET_CFLG (cflg);
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s8)(newv)) == 0));
	SET_NFLG (((uae_s8)(newv)) < 0);
	SET_VFLG ((tmp_newv & 0x80) != 0 && (newv & 0x80) == 0);
	put_byte (srca, newv);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* NBCD.B (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4818_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{	uae_u16 newv_lo = - (src & 0xF) - (GET_XFLG () ? 1 : 0);
	uae_u16 newv_hi = - (src & 0xF0);
	uae_u16 newv;
	int cflg, tmp_newv;
	if (newv_lo > 9) { newv_lo -= 6; }
	tmp_newv = newv = newv_hi + newv_lo;
	cflg = (newv & 0x1F0) > 0x90;
	if (cflg) newv -= 0x60;
	SET_CFLG (cflg);
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s8)(newv)) == 0));
	SET_NFLG (((uae_s8)(newv)) < 0);
	SET_VFLG ((tmp_newv & 0x80) != 0 && (newv & 0x80) == 0);
	put_byte (srca, newv);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* NBCD.B -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_4820_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_u16 newv_lo = - (src & 0xF) - (GET_XFLG () ? 1 : 0);
	uae_u16 newv_hi = - (src & 0xF0);
	uae_u16 newv;
	int cflg, tmp_newv;
	if (newv_lo > 9) { newv_lo -= 6; }
	tmp_newv = newv = newv_hi + newv_lo;
	cflg = (newv & 0x1F0) > 0x90;
	if (cflg) newv -= 0x60;
	SET_CFLG (cflg);
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s8)(newv)) == 0));
	SET_NFLG (((uae_s8)(newv)) < 0);
	SET_VFLG ((tmp_newv & 0x80) != 0 && (newv & 0x80) == 0);
	put_byte (srca, newv);
}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* NBCD.B (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4828_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uae_u16 newv_lo = - (src & 0xF) - (GET_XFLG () ? 1 : 0);
	uae_u16 newv_hi = - (src & 0xF0);
	uae_u16 newv;
	int cflg, tmp_newv;
	if (newv_lo > 9) { newv_lo -= 6; }
	tmp_newv = newv = newv_hi + newv_lo;
	cflg = (newv & 0x1F0) > 0x90;
	if (cflg) newv -= 0x60;
	SET_CFLG (cflg);
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s8)(newv)) == 0));
	SET_NFLG (((uae_s8)(newv)) < 0);
	SET_VFLG ((tmp_newv & 0x80) != 0 && (newv & 0x80) == 0);
	put_byte (srca, newv);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* NBCD.B (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4830_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s8 src = get_byte (srca);
{	uae_u16 newv_lo = - (src & 0xF) - (GET_XFLG () ? 1 : 0);
	uae_u16 newv_hi = - (src & 0xF0);
	uae_u16 newv;
	int cflg, tmp_newv;
	if (newv_lo > 9) { newv_lo -= 6; }
	tmp_newv = newv = newv_hi + newv_lo;
	cflg = (newv & 0x1F0) > 0x90;
	if (cflg) newv -= 0x60;
	SET_CFLG (cflg);
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s8)(newv)) == 0));
	SET_NFLG (((uae_s8)(newv)) < 0);
	SET_VFLG ((tmp_newv & 0x80) != 0 && (newv & 0x80) == 0);
	put_byte (srca, newv);
}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* NBCD.B (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4838_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uae_u16 newv_lo = - (src & 0xF) - (GET_XFLG () ? 1 : 0);
	uae_u16 newv_hi = - (src & 0xF0);
	uae_u16 newv;
	int cflg, tmp_newv;
	if (newv_lo > 9) { newv_lo -= 6; }
	tmp_newv = newv = newv_hi + newv_lo;
	cflg = (newv & 0x1F0) > 0x90;
	if (cflg) newv -= 0x60;
	SET_CFLG (cflg);
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s8)(newv)) == 0));
	SET_NFLG (((uae_s8)(newv)) < 0);
	SET_VFLG ((tmp_newv & 0x80) != 0 && (newv & 0x80) == 0);
	put_byte (srca, newv);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* NBCD.B (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4839_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte (srca);
{	uae_u16 newv_lo = - (src & 0xF) - (GET_XFLG () ? 1 : 0);
	uae_u16 newv_hi = - (src & 0xF0);
	uae_u16 newv;
	int cflg, tmp_newv;
	if (newv_lo > 9) { newv_lo -= 6; }
	tmp_newv = newv = newv_hi + newv_lo;
	cflg = (newv & 0x1F0) > 0x90;
	if (cflg) newv -= 0x60;
	SET_CFLG (cflg);
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s8)(newv)) == 0));
	SET_NFLG (((uae_s8)(newv)) < 0);
	SET_VFLG ((tmp_newv & 0x80) != 0 && (newv & 0x80) == 0);
	put_byte (srca, newv);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* SWAP.W Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4840_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uae_u32 dst = ((src >> 16)&0xFFFF) | ((src&0xFFFF)<<16);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(dst)) == 0);
	SET_NFLG   (((uae_s32)(dst)) < 0);
	m68k_dreg (regs, srcreg) = (dst);
}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* PEA.L (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4850_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, 7) - 4;
	m68k_areg (regs, 7) = dsta;
	put_long (dsta, srca);
}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* PEA.L (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4868_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, 7) - 4;
	m68k_areg (regs, 7) = dsta;
	put_long (dsta, srca);
}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* PEA.L (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4870_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uaecptr dsta;
	dsta = m68k_areg (regs, 7) - 4;
	m68k_areg (regs, 7) = dsta;
	put_long (dsta, srca);
}}}	m68k_incpc (4);
return 20 * CYCLE_UNIT / 2;
}

/* PEA.L (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4878_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, 7) - 4;
	m68k_areg (regs, 7) = dsta;
	put_long (dsta, srca);
}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* PEA.L (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4879_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, 7) - 4;
	m68k_areg (regs, 7) = dsta;
	put_long (dsta, srca);
}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* PEA.L (d16,PC) */
uae_u32 REGPARAM2 CPUFUNC(op_487a_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, 7) - 4;
	m68k_areg (regs, 7) = dsta;
	put_long (dsta, srca);
}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* PEA.L (d8,PC,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_487b_4)(uae_u32 opcode)
{
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uaecptr dsta;
	dsta = m68k_areg (regs, 7) - 4;
	m68k_areg (regs, 7) = dsta;
	put_long (dsta, srca);
}}}	m68k_incpc (4);
return 20 * CYCLE_UNIT / 2;
}

/* EXT.W Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4880_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uae_u16 dst = (uae_s16)(uae_s8)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(dst)) == 0);
	SET_NFLG   (((uae_s16)(dst)) < 0);
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | ((dst) & 0xffff);
}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* MVMLE.W #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_4890_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_diword (2);
{	uaecptr srca;
	srca = m68k_areg (regs, dstreg);
{	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) {
		put_word (srca, m68k_dreg (regs, movem_index1[dmask]));
		srca += 2;
		dmask = movem_next[dmask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
	while (amask) {
		put_word (srca, m68k_areg (regs, movem_index1[amask]));
		srca += 2;
		amask = movem_next[amask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (4);
return 8 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMLE.W #<data>.W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_48a0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_diword (2);
{	uaecptr srca;
	srca = m68k_areg (regs, dstreg) - 0;
{	uae_u16 amask = mask & 0xff, dmask = (mask >> 8) & 0xff;
	while (amask) {
		srca -= 2;
		put_word (srca, m68k_areg (regs, movem_index2[amask]));
	amask = movem_next[amask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
	while (dmask) {
		srca -= 2;
		put_word (srca, m68k_dreg (regs, movem_index2[dmask]));
		dmask = movem_next[dmask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
	m68k_areg (regs, dstreg) = srca;
}}}	m68k_incpc (4);
return 8 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMLE.W #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_48a8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_diword (2);
{	uaecptr srca;
	srca = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) {
		put_word (srca, m68k_dreg (regs, movem_index1[dmask]));
		srca += 2;
		dmask = movem_next[dmask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
	while (amask) {
		put_word (srca, m68k_areg (regs, movem_index1[amask]));
		srca += 2;
		amask = movem_next[amask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (6);
return 12 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMLE.W #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_48b0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_diword (2);
{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (4));
{	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) {
		put_word (srca, m68k_dreg (regs, movem_index1[dmask]));
		srca += 2;
		dmask = movem_next[dmask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
	while (amask) {
		put_word (srca, m68k_areg (regs, movem_index1[amask]));
		srca += 2;
		amask = movem_next[amask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (6);
return 14 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMLE.W #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_48b8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
{	uae_u16 mask = get_diword (2);
{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (4);
{	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) {
		put_word (srca, m68k_dreg (regs, movem_index1[dmask]));
		srca += 2;
		dmask = movem_next[dmask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
	while (amask) {
		put_word (srca, m68k_areg (regs, movem_index1[amask]));
		srca += 2;
		amask = movem_next[amask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (6);
return 12 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMLE.W #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_48b9_4)(uae_u32 opcode)
{
	int count_cycles = 0;
{	uae_u16 mask = get_diword (2);
{	uaecptr srca;
	srca = get_dilong (4);
{	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) {
		put_word (srca, m68k_dreg (regs, movem_index1[dmask]));
		srca += 2;
		dmask = movem_next[dmask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
	while (amask) {
		put_word (srca, m68k_areg (regs, movem_index1[amask]));
		srca += 2;
		amask = movem_next[amask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (8);
return 16 * CYCLE_UNIT / 2 + count_cycles;
}

/* EXT.L Dn */
uae_u32 REGPARAM2 CPUFUNC(op_48c0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uae_u32 dst = (uae_s32)(uae_s16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(dst)) == 0);
	SET_NFLG   (((uae_s32)(dst)) < 0);
	m68k_dreg (regs, srcreg) = (dst);
}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* MVMLE.L #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_48d0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_diword (2);
{	uaecptr srca;
	srca = m68k_areg (regs, dstreg);
{	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) {
		put_long (srca, m68k_dreg (regs, movem_index1[dmask]));
		srca += 4;
		dmask = movem_next[dmask];
		count_cycles += 8 * CYCLE_UNIT / 2;
	}
	while (amask) {
		put_long (srca, m68k_areg (regs, movem_index1[amask]));
		srca += 4;
		amask = movem_next[amask];
		count_cycles += 8 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (4);
return 8 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMLE.L #<data>.W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_48e0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_diword (2);
{	uaecptr srca;
	srca = m68k_areg (regs, dstreg) - 0;
{	uae_u16 amask = mask & 0xff, dmask = (mask >> 8) & 0xff;
	while (amask) {
		srca -= 4;
		put_long (srca, m68k_areg (regs, movem_index2[amask]));
		amask = movem_next[amask];
		count_cycles += 8 * CYCLE_UNIT / 2;
	}
	while (dmask) {
		srca -= 4;
		put_long (srca, m68k_dreg (regs, movem_index2[dmask]));
		dmask = movem_next[dmask];
		count_cycles += 8 * CYCLE_UNIT / 2;
	}
	m68k_areg (regs, dstreg) = srca;
}}}	m68k_incpc (4);
return 8 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMLE.L #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_48e8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_diword (2);
{	uaecptr srca;
	srca = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) {
		put_long (srca, m68k_dreg (regs, movem_index1[dmask]));
		srca += 4;
		dmask = movem_next[dmask];
		count_cycles += 8 * CYCLE_UNIT / 2;
	}
	while (amask) {
		put_long (srca, m68k_areg (regs, movem_index1[amask]));
		srca += 4;
		amask = movem_next[amask];
		count_cycles += 8 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (6);
return 12 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMLE.L #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_48f0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_diword (2);
{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (4));
{	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) {
		put_long (srca, m68k_dreg (regs, movem_index1[dmask]));
		srca += 4;
		dmask = movem_next[dmask];
		count_cycles += 8 * CYCLE_UNIT / 2;
	}
	while (amask) {
		put_long (srca, m68k_areg (regs, movem_index1[amask]));
		srca += 4;
		amask = movem_next[amask];
		count_cycles += 8 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (6);
return 14 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMLE.L #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_48f8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
{	uae_u16 mask = get_diword (2);
{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (4);
{	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) {
		put_long (srca, m68k_dreg (regs, movem_index1[dmask]));
		srca += 4;
		dmask = movem_next[dmask];
		count_cycles += 8 * CYCLE_UNIT / 2;
	}
	while (amask) {
		put_long (srca, m68k_areg (regs, movem_index1[amask]));
		srca += 4;
		amask = movem_next[amask];
		count_cycles += 8 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (6);
return 12 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMLE.L #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_48f9_4)(uae_u32 opcode)
{
	int count_cycles = 0;
{	uae_u16 mask = get_diword (2);
{	uaecptr srca;
	srca = get_dilong (4);
{	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) {
		put_long (srca, m68k_dreg (regs, movem_index1[dmask]));
		srca += 4;
		dmask = movem_next[dmask];
		count_cycles += 8 * CYCLE_UNIT / 2;
	}
	while (amask) {
		put_long (srca, m68k_areg (regs, movem_index1[amask]));
		srca += 4;
		amask = movem_next[amask];
		count_cycles += 8 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (8);
return 16 * CYCLE_UNIT / 2 + count_cycles;
}

/* TST.B Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4a00_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* TST.B (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4a10_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* TST.B (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4a18_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* TST.B -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_4a20_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* TST.B (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4a28_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* TST.B (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4a30_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s8 src = get_byte (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* TST.B (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4a38_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* TST.B (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4a39_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
}}}	m68k_incpc (6);
return 16 * CYCLE_UNIT / 2;
}

/* TST.W Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4a40_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* TST.W (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4a50_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* TST.W (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4a58_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) += 2;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* TST.W -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_4a60_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* TST.W (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4a68_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* TST.W (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4a70_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s16 src = get_word (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* TST.W (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4a78_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* TST.W (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4a79_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
}}}	m68k_incpc (6);
return 16 * CYCLE_UNIT / 2;
}

/* TST.L Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4a80_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = m68k_dreg (regs, srcreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* TST.L (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4a90_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* TST.L (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4a98_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) += 4;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* TST.L -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_4aa0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* TST.L (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4aa8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* TST.L (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4ab0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s32 src = get_long (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* TST.L (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4ab8_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* TST.L (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4ab9_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* TAS.B Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4ac0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	src |= 0x80;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((src) & 0xff);
}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* TAS.B (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4ad0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	src |= 0x80;
	put_byte (srca, src);
}}}	m68k_incpc (2);
return 18 * CYCLE_UNIT / 2;
}

/* TAS.B (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4ad8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	src |= 0x80;
	put_byte (srca, src);
}}}	m68k_incpc (2);
return 18 * CYCLE_UNIT / 2;
}

/* TAS.B -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_4ae0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	src |= 0x80;
	put_byte (srca, src);
}}}	m68k_incpc (2);
return 20 * CYCLE_UNIT / 2;
}

/* TAS.B (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4ae8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	src |= 0x80;
	put_byte (srca, src);
}}}	m68k_incpc (4);
return 22 * CYCLE_UNIT / 2;
}

/* TAS.B (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4af0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s8 src = get_byte (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	src |= 0x80;
	put_byte (srca, src);
}}}	m68k_incpc (4);
return 24 * CYCLE_UNIT / 2;
}

/* TAS.B (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4af8_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	src |= 0x80;
	put_byte (srca, src);
}}}	m68k_incpc (4);
return 22 * CYCLE_UNIT / 2;
}

/* TAS.B (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4af9_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	src |= 0x80;
	put_byte (srca, src);
}}}	m68k_incpc (6);
return 26 * CYCLE_UNIT / 2;
}

/* MVMEL.W #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_4c90_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca;
	srca = m68k_areg (regs, dstreg);
{	while (dmask) {
		m68k_dreg (regs, movem_index1[dmask]) = (uae_s32)(uae_s16)get_word (srca);
		srca += 2;
		dmask = movem_next[dmask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
	while (amask) {
		m68k_areg (regs, movem_index1[amask]) = (uae_s32)(uae_s16)get_word (srca);
		srca += 2;
		amask = movem_next[amask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMEL.W #<data>.W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4c98_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca;
	srca = m68k_areg (regs, dstreg);
{	while (dmask) {
		m68k_dreg (regs, movem_index1[dmask]) = (uae_s32)(uae_s16)get_word (srca);
		srca += 2;
		dmask = movem_next[dmask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
	while (amask) {
		m68k_areg (regs, movem_index1[amask]) = (uae_s32)(uae_s16)get_word (srca);
		srca += 2;
		amask = movem_next[amask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
	m68k_areg (regs, dstreg) = srca;
}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMEL.W #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4ca8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca;
	srca = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	while (dmask) {
		m68k_dreg (regs, movem_index1[dmask]) = (uae_s32)(uae_s16)get_word (srca);
		srca += 2;
		dmask = movem_next[dmask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
	while (amask) {
		m68k_areg (regs, movem_index1[amask]) = (uae_s32)(uae_s16)get_word (srca);
		srca += 2;
		amask = movem_next[amask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (6);
return 16 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMEL.W #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4cb0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (4));
{	while (dmask) {
		m68k_dreg (regs, movem_index1[dmask]) = (uae_s32)(uae_s16)get_word (srca);
		srca += 2;
		dmask = movem_next[dmask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
	while (amask) {
		m68k_areg (regs, movem_index1[amask]) = (uae_s32)(uae_s16)get_word (srca);
		srca += 2;
		amask = movem_next[amask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (6);
return 18 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMEL.W #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4cb8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
{	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (4);
{	while (dmask) {
		m68k_dreg (regs, movem_index1[dmask]) = (uae_s32)(uae_s16)get_word (srca);
		srca += 2;
		dmask = movem_next[dmask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
	while (amask) {
		m68k_areg (regs, movem_index1[amask]) = (uae_s32)(uae_s16)get_word (srca);
		srca += 2;
		amask = movem_next[amask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (6);
return 16 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMEL.W #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4cb9_4)(uae_u32 opcode)
{
	int count_cycles = 0;
{	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca;
	srca = get_dilong (4);
{	while (dmask) {
		m68k_dreg (regs, movem_index1[dmask]) = (uae_s32)(uae_s16)get_word (srca);
		srca += 2;
		dmask = movem_next[dmask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
	while (amask) {
		m68k_areg (regs, movem_index1[amask]) = (uae_s32)(uae_s16)get_word (srca);
		srca += 2;
		amask = movem_next[amask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (8);
return 20 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMEL.W #<data>.W,(d16,PC) */
uae_u32 REGPARAM2 CPUFUNC(op_4cba_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = 2;
{	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca;
	srca = m68k_getpc () + 4;
	srca += (uae_s32)(uae_s16)get_diword (4);
{	while (dmask) {
		m68k_dreg (regs, movem_index1[dmask]) = (uae_s32)(uae_s16)get_word (srca);
		srca += 2;
		dmask = movem_next[dmask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
	while (amask) {
		m68k_areg (regs, movem_index1[amask]) = (uae_s32)(uae_s16)get_word (srca);
		srca += 2;
		amask = movem_next[amask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (6);
return 16 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMEL.W #<data>.W,(d8,PC,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4cbb_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = 3;
{	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 4;
	srca = get_disp_ea_000 (tmppc, get_diword (4));
{	while (dmask) {
		m68k_dreg (regs, movem_index1[dmask]) = (uae_s32)(uae_s16)get_word (srca);
		srca += 2;
		dmask = movem_next[dmask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
	while (amask) {
		m68k_areg (regs, movem_index1[amask]) = (uae_s32)(uae_s16)get_word (srca);
		srca += 2;
		amask = movem_next[amask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (6);
return 18 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMEL.L #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_4cd0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca;
	srca = m68k_areg (regs, dstreg);
{	while (dmask) {
		m68k_dreg (regs, movem_index1[dmask]) = get_long (srca);
		srca += 4;
		dmask = movem_next[dmask];
		count_cycles += 8 * CYCLE_UNIT / 2;
	}
	while (amask) {
		m68k_areg (regs, movem_index1[amask]) = get_long (srca);
		srca += 4;
		amask = movem_next[amask];
		count_cycles += 8 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMEL.L #<data>.W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4cd8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca;
	srca = m68k_areg (regs, dstreg);
{	while (dmask) {
		m68k_dreg (regs, movem_index1[dmask]) = get_long (srca);
		srca += 4;
		dmask = movem_next[dmask];
		count_cycles += 8 * CYCLE_UNIT / 2;
	}
	while (amask) {
		m68k_areg (regs, movem_index1[amask]) = get_long (srca);
		srca += 4;
		amask = movem_next[amask];
		count_cycles += 8 * CYCLE_UNIT / 2;
	}
	m68k_areg (regs, dstreg) = srca;
}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMEL.L #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4ce8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca;
	srca = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	while (dmask) {
		m68k_dreg (regs, movem_index1[dmask]) = get_long (srca);
		srca += 4;
		dmask = movem_next[dmask];
		count_cycles += 8 * CYCLE_UNIT / 2;
	}
	while (amask) {
		m68k_areg (regs, movem_index1[amask]) = get_long (srca);
		srca += 4;
		amask = movem_next[amask];
		count_cycles += 8 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (6);
return 16 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMEL.L #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4cf0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (4));
{	while (dmask) {
		m68k_dreg (regs, movem_index1[dmask]) = get_long (srca);
		srca += 4;
		dmask = movem_next[dmask];
		count_cycles += 8 * CYCLE_UNIT / 2;
	}
	while (amask) {
		m68k_areg (regs, movem_index1[amask]) = get_long (srca);
		srca += 4;
		amask = movem_next[amask];
		count_cycles += 8 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (6);
return 18 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMEL.L #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4cf8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
{	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (4);
{	while (dmask) {
		m68k_dreg (regs, movem_index1[dmask]) = get_long (srca);
		srca += 4;
		dmask = movem_next[dmask];
		count_cycles += 8 * CYCLE_UNIT / 2;
	}
	while (amask) {
		m68k_areg (regs, movem_index1[amask]) = get_long (srca);
		srca += 4;
		amask = movem_next[amask];
		count_cycles += 8 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (6);
return 16 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMEL.L #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4cf9_4)(uae_u32 opcode)
{
	int count_cycles = 0;
{	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca;
	srca = get_dilong (4);
{	while (dmask) {
		m68k_dreg (regs, movem_index1[dmask]) = get_long (srca);
		srca += 4;
		dmask = movem_next[dmask];
		count_cycles += 8 * CYCLE_UNIT / 2;
	}
	while (amask) {
		m68k_areg (regs, movem_index1[amask]) = get_long (srca);
		srca += 4;
		amask = movem_next[amask];
		count_cycles += 8 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (8);
return 20 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMEL.L #<data>.W,(d16,PC) */
uae_u32 REGPARAM2 CPUFUNC(op_4cfa_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = 2;
{	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca;
	srca = m68k_getpc () + 4;
	srca += (uae_s32)(uae_s16)get_diword (4);
{	while (dmask) {
		m68k_dreg (regs, movem_index1[dmask]) = get_long (srca);
		srca += 4;
		dmask = movem_next[dmask];
		count_cycles += 8 * CYCLE_UNIT / 2;
	}
	while (amask) {
		m68k_areg (regs, movem_index1[amask]) = get_long (srca);
		srca += 4;
		amask = movem_next[amask];
		count_cycles += 8 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (6);
return 16 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMEL.L #<data>.W,(d8,PC,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4cfb_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = 3;
{	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 4;
	srca = get_disp_ea_000 (tmppc, get_diword (4));
{	while (dmask) {
		m68k_dreg (regs, movem_index1[dmask]) = get_long (srca);
		srca += 4;
		dmask = movem_next[dmask];
		count_cycles += 8 * CYCLE_UNIT / 2;
	}
	while (amask) {
		m68k_areg (regs, movem_index1[amask]) = get_long (srca);
		srca += 4;
		amask = movem_next[amask];
		count_cycles += 8 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (6);
return 18 * CYCLE_UNIT / 2 + count_cycles;
}

/* TRAPQ.L #<data> */
uae_u32 REGPARAM2 CPUFUNC(op_4e40_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 15);
{{	uae_u32 src = srcreg;
	m68k_incpc (2);
	Exception (src + 32);
}}return 4 * CYCLE_UNIT / 2;
}

/* LINK.W An,#<data>.W */
uae_u32 REGPARAM2 CPUFUNC(op_4e50_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr olda;
	olda = m68k_areg (regs, 7) - 4;
	m68k_areg (regs, 7) = olda;
{	uae_s32 src = m68k_areg (regs, srcreg);
{	uae_s16 offs = get_diword (2);
	put_long (olda, src);
	m68k_areg (regs, srcreg) = (m68k_areg (regs, 7));
	m68k_areg (regs, 7) += offs;
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* UNLK.L An */
uae_u32 REGPARAM2 CPUFUNC(op_4e58_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = m68k_areg (regs, srcreg);
	m68k_areg (regs, 7) = src;
{	uaecptr olda;
	olda = m68k_areg (regs, 7);
{	uae_s32 old = get_long (olda);
	m68k_areg (regs, 7) += 4;
	m68k_areg (regs, srcreg) = (old);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* MVR2USP.L An */
uae_u32 REGPARAM2 CPUFUNC(op_4e60_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uae_s32 src = m68k_areg (regs, srcreg);
	regs.usp = src;
}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* MVUSP2R.L An */
uae_u32 REGPARAM2 CPUFUNC(op_4e68_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	m68k_areg (regs, srcreg) = (regs.usp);
}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* RESET.L  */
uae_u32 REGPARAM2 CPUFUNC(op_4e70_4)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	cpureset ();
	m68k_incpc (2);
}}return 132 * CYCLE_UNIT / 2;
}

/* NOP.L  */
uae_u32 REGPARAM2 CPUFUNC(op_4e71_4)(uae_u32 opcode)
{
{}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* STOP.L #<data>.W */
uae_u32 REGPARAM2 CPUFUNC(op_4e72_4)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uae_s16 src = get_diword (2);
	regs.sr = src;
	MakeFromSR ();
	m68k_setstopped ();
	m68k_incpc (4);
}}}return 8 * CYCLE_UNIT / 2;
}

/* RTE.L  */
uae_u32 REGPARAM2 CPUFUNC(op_4e73_4)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	uae_u16 newsr; uae_u32 newpc;
	for (;;) {
		uaecptr a = m68k_areg (regs, 7);
		uae_u16 sr = get_word (a);
		uae_u32 pc = get_long (a + 2);
		uae_u16 format = get_word (a + 2 + 4);
		int frame = format >> 12;
		int offset = 8;
		newsr = sr; newpc = pc;
		if (frame == 0x0) { m68k_areg (regs, 7) += offset; break; }
		else if (frame == 0x1) { m68k_areg (regs, 7) += offset; }
		else if (frame == 0x2) { m68k_areg (regs, 7) += offset + 4; break; }
		else if (frame == 0x8) { m68k_areg (regs, 7) += offset + 50; break; }
		else if (frame == 0x9) { m68k_areg (regs, 7) += offset + 12; break; }
		else if (frame == 0xa) { m68k_areg (regs, 7) += offset + 24; break; }
		else if (frame == 0xb) { m68k_areg (regs, 7) += offset + 84; break; }
		else { m68k_areg (regs, 7) += offset; Exception (14); return 4 * CYCLE_UNIT / 2; }
		regs.sr = newsr;
	MakeFromSR ();
}
	regs.sr = newsr;
	MakeFromSR ();
	if (newpc & 1) {
		exception3i (0x4E73, newpc);
		return 4 * CYCLE_UNIT / 2;
	}
	m68k_setpc (newpc);
}}return 24 * CYCLE_UNIT / 2;
}

/* RTD.L #<data>.W */
uae_u32 REGPARAM2 CPUFUNC(op_4e74_4)(uae_u32 opcode)
{
{{	uaecptr pca;
	pca = m68k_areg (regs, 7);
{	uae_s32 pc = get_long (pca);
	m68k_areg (regs, 7) += 4;
{	uae_s16 offs = get_diword (2);
	m68k_areg (regs, 7) += offs;
	if (pc & 1) {
		exception3i (0x4E74, pc);
		return 16 * CYCLE_UNIT / 2;
	}
	m68k_setpc (pc);
}}}}return 20 * CYCLE_UNIT / 2;
}

/* RTS.L  */
uae_u32 REGPARAM2 CPUFUNC(op_4e75_4)(uae_u32 opcode)
{
{	uaecptr pc = m68k_getpc ();
	m68k_do_rts ();
	if (m68k_getpc () & 1) {
		uaecptr faultpc = m68k_getpc ();
	m68k_setpc (pc);
		exception3i (0x4E75, faultpc);
		return 8 * CYCLE_UNIT / 2;
	}
}return 16 * CYCLE_UNIT / 2;
}

/* TRAPV.L  */
uae_u32 REGPARAM2 CPUFUNC(op_4e76_4)(uae_u32 opcode)
{
{	m68k_incpc (2);
	if (GET_VFLG ()) {
		Exception (7);
		return 4 * CYCLE_UNIT / 2;
	}
}return 4 * CYCLE_UNIT / 2;
}

/* RTR.L  */
uae_u32 REGPARAM2 CPUFUNC(op_4e77_4)(uae_u32 opcode)
{
{	uaecptr oldpc = m68k_getpc ();
	MakeSR ();
{	uaecptr sra;
	sra = m68k_areg (regs, 7);
{	uae_s16 sr = get_word (sra);
	m68k_areg (regs, 7) += 2;
{	uaecptr pca;
	pca = m68k_areg (regs, 7);
{	uae_s32 pc = get_long (pca);
	m68k_areg (regs, 7) += 4;
	regs.sr &= 0xFF00; sr &= 0xFF;
	regs.sr |= sr;
	m68k_setpc (pc);
	MakeFromSR ();
	if (m68k_getpc () & 1) {
		uaecptr faultpc = m68k_getpc ();
	m68k_setpc (oldpc);
		exception3i (0x4E77, faultpc);
		return 8 * CYCLE_UNIT / 2;
	}
}}}}}return 20 * CYCLE_UNIT / 2;
}

/* MOVEC2.L #<data>.W */
uae_u32 REGPARAM2 CPUFUNC(op_4e7a_4)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uae_s16 src = get_diword (2);
{	int regno = (src >> 12) & 15;
	uae_u32 *regp = regs.regs + regno;
	if (! m68k_movec2(src & 0xFFF, regp)) goto l_40771;
}}}}	m68k_incpc (4);
l_40771: ;
return 8 * CYCLE_UNIT / 2;
}

/* MOVE2C.L #<data>.W */
uae_u32 REGPARAM2 CPUFUNC(op_4e7b_4)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uae_s16 src = get_diword (2);
{	int regno = (src >> 12) & 15;
	uae_u32 *regp = regs.regs + regno;
	if (! m68k_move2c(src & 0xFFF, regp)) goto l_40772;
}}}}	m68k_incpc (4);
l_40772: ;
return 8 * CYCLE_UNIT / 2;
}

/* JSR.L (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4e90_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uaecptr oldpc = m68k_getpc () + 2;
	if (srca & 1) {
		exception3i (opcode, srca);
		return 4 * CYCLE_UNIT / 2;
	}
	m68k_setpc (srca);
	m68k_areg (regs, 7) -= 4;
	put_long (m68k_areg (regs, 7), oldpc);
}}}return 16 * CYCLE_UNIT / 2;
}

/* JSR.L (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4ea8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uaecptr oldpc = m68k_getpc () + 4;
	if (srca & 1) {
		exception3i (opcode, srca);
		return 8 * CYCLE_UNIT / 2;
	}
	m68k_setpc (srca);
	m68k_areg (regs, 7) -= 4;
	put_long (m68k_areg (regs, 7), oldpc);
}}}return 18 * CYCLE_UNIT / 2;
}

/* JSR.L (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4eb0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uaecptr oldpc = m68k_getpc () + 4;
	if (srca & 1) {
		exception3i (opcode, srca);
		return 8 * CYCLE_UNIT / 2;
	}
	m68k_setpc (srca);
	m68k_areg (regs, 7) -= 4;
	put_long (m68k_areg (regs, 7), oldpc);
}}}return 22 * CYCLE_UNIT / 2;
}

/* JSR.L (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4eb8_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uaecptr oldpc = m68k_getpc () + 4;
	if (srca & 1) {
		exception3i (opcode, srca);
		return 8 * CYCLE_UNIT / 2;
	}
	m68k_setpc (srca);
	m68k_areg (regs, 7) -= 4;
	put_long (m68k_areg (regs, 7), oldpc);
}}}return 18 * CYCLE_UNIT / 2;
}

/* JSR.L (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4eb9_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uaecptr oldpc = m68k_getpc () + 6;
	if (srca & 1) {
		exception3i (opcode, srca);
		return 12 * CYCLE_UNIT / 2;
	}
	m68k_setpc (srca);
	m68k_areg (regs, 7) -= 4;
	put_long (m68k_areg (regs, 7), oldpc);
}}}return 20 * CYCLE_UNIT / 2;
}

/* JSR.L (d16,PC) */
uae_u32 REGPARAM2 CPUFUNC(op_4eba_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uaecptr oldpc = m68k_getpc () + 4;
	if (srca & 1) {
		exception3i (opcode, srca);
		return 8 * CYCLE_UNIT / 2;
	}
	m68k_setpc (srca);
	m68k_areg (regs, 7) -= 4;
	put_long (m68k_areg (regs, 7), oldpc);
}}}return 18 * CYCLE_UNIT / 2;
}

/* JSR.L (d8,PC,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4ebb_4)(uae_u32 opcode)
{
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uaecptr oldpc = m68k_getpc () + 4;
	if (srca & 1) {
		exception3i (opcode, srca);
		return 8 * CYCLE_UNIT / 2;
	}
	m68k_setpc (srca);
	m68k_areg (regs, 7) -= 4;
	put_long (m68k_areg (regs, 7), oldpc);
}}}return 22 * CYCLE_UNIT / 2;
}

/* JMP.L (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4ed0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	if (srca & 1) {
		exception3i (opcode, srca);
		return 4 * CYCLE_UNIT / 2;
	}
	m68k_setpc (srca);
}}return 8 * CYCLE_UNIT / 2;
}

/* JMP.L (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4ee8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	if (srca & 1) {
		exception3i (opcode, srca);
		return 8 * CYCLE_UNIT / 2;
	}
	m68k_setpc (srca);
}}return 10 * CYCLE_UNIT / 2;
}

/* JMP.L (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4ef0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
	if (srca & 1) {
		exception3i (opcode, srca);
		return 8 * CYCLE_UNIT / 2;
	}
	m68k_setpc (srca);
}}return 14 * CYCLE_UNIT / 2;
}

/* JMP.L (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4ef8_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	if (srca & 1) {
		exception3i (opcode, srca);
		return 8 * CYCLE_UNIT / 2;
	}
	m68k_setpc (srca);
}}return 10 * CYCLE_UNIT / 2;
}

/* JMP.L (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4ef9_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
	if (srca & 1) {
		exception3i (opcode, srca);
		return 12 * CYCLE_UNIT / 2;
	}
	m68k_setpc (srca);
}}return 12 * CYCLE_UNIT / 2;
}

/* JMP.L (d16,PC) */
uae_u32 REGPARAM2 CPUFUNC(op_4efa_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	if (srca & 1) {
		exception3i (opcode, srca);
		return 8 * CYCLE_UNIT / 2;
	}
	m68k_setpc (srca);
}}return 10 * CYCLE_UNIT / 2;
}

/* JMP.L (d8,PC,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4efb_4)(uae_u32 opcode)
{
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
	if (srca & 1) {
		exception3i (opcode, srca);
		return 8 * CYCLE_UNIT / 2;
	}
	m68k_setpc (srca);
}}return 14 * CYCLE_UNIT / 2;
}

/* ADDQ.B #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_5000_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* ADDQ.B #<data>,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_5010_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* ADDQ.B #<data>,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_5018_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* ADDQ.B #<data>,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_5020_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* ADDQ.B #<data>,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_5028_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* ADDQ.B #<data>,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_5030_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (2));
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* ADDQ.B #<data>,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_5038_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* ADDQ.B #<data>,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_5039_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* ADDQ.W #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_5040_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* ADDAQ.W #<data>,An */
uae_u32 REGPARAM2 CPUFUNC(op_5048_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* ADDQ.W #<data>,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_5050_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* ADDQ.W #<data>,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_5058_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) += 2;
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* ADDQ.W #<data>,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_5060_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* ADDQ.W #<data>,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_5068_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* ADDQ.W #<data>,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_5070_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (2));
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* ADDQ.W #<data>,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_5078_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* ADDQ.W #<data>,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_5079_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* ADDQ.L #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_5080_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* ADDAQ.L #<data>,An */
uae_u32 REGPARAM2 CPUFUNC(op_5088_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* ADDQ.L #<data>,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_5090_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 20 * CYCLE_UNIT / 2;
}

/* ADDQ.L #<data>,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_5098_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) += 4;
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 20 * CYCLE_UNIT / 2;
}

/* ADDQ.L #<data>,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_50a0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 22 * CYCLE_UNIT / 2;
}

/* ADDQ.L #<data>,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_50a8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 24 * CYCLE_UNIT / 2;
}

#endif

#ifdef PART_5
/* ADDQ.L #<data>,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_50b0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (2));
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 26 * CYCLE_UNIT / 2;
}

/* ADDQ.L #<data>,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_50b8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 24 * CYCLE_UNIT / 2;
}

/* ADDQ.L #<data>,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_50b9_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (6);
return 28 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (T) */
uae_u32 REGPARAM2 CPUFUNC(op_50c0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue (regs.ccrflags, 0) ? 0xff : 0;
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2 + count_cycles;
}

/* DBcc.W Dn,#<data>.W (T) */
uae_u32 REGPARAM2 CPUFUNC(op_50c8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue (regs.ccrflags, 0)) {
	m68k_incpc ((uae_s32)offs + 2);
			m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc () + 2 + (uae_s32)offs + 2);
		return 10 * CYCLE_UNIT / 2;
			}
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 4 * CYCLE_UNIT / 2;
	} else {
		count_cycles += 2 * CYCLE_UNIT / 2;
	}
	m68k_setpc (oldpc + 4);
}}}return 10 * CYCLE_UNIT / 2 + count_cycles;
}

/* Scc.B (An) (T) */
uae_u32 REGPARAM2 CPUFUNC(op_50d0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{{	int val = cctrue (regs.ccrflags, 0) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (T) */
uae_u32 REGPARAM2 CPUFUNC(op_50d8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (regs.ccrflags, 0) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (T) */
uae_u32 REGPARAM2 CPUFUNC(op_50e0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (regs.ccrflags, 0) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (T) */
uae_u32 REGPARAM2 CPUFUNC(op_50e8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (regs.ccrflags, 0) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (T) */
uae_u32 REGPARAM2 CPUFUNC(op_50f0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{{	int val = cctrue (regs.ccrflags, 0) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (T) */
uae_u32 REGPARAM2 CPUFUNC(op_50f8_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (regs.ccrflags, 0) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (T) */
uae_u32 REGPARAM2 CPUFUNC(op_50f9_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{{	int val = cctrue (regs.ccrflags, 0) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (6);
return 18 * CYCLE_UNIT / 2;
}

/* SUBQ.B #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_5100_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* SUBQ.B #<data>,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_5110_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* SUBQ.B #<data>,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_5118_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* SUBQ.B #<data>,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_5120_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* SUBQ.B #<data>,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_5128_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* SUBQ.B #<data>,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_5130_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (2));
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* SUBQ.B #<data>,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_5138_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* SUBQ.B #<data>,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_5139_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* SUBQ.W #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_5140_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* SUBAQ.W #<data>,An */
uae_u32 REGPARAM2 CPUFUNC(op_5148_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* SUBQ.W #<data>,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_5150_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* SUBQ.W #<data>,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_5158_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) += 2;
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* SUBQ.W #<data>,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_5160_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* SUBQ.W #<data>,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_5168_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* SUBQ.W #<data>,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_5170_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (2));
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* SUBQ.W #<data>,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_5178_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* SUBQ.W #<data>,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_5179_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* SUBQ.L #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_5180_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* SUBAQ.L #<data>,An */
uae_u32 REGPARAM2 CPUFUNC(op_5188_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* SUBQ.L #<data>,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_5190_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 20 * CYCLE_UNIT / 2;
}

/* SUBQ.L #<data>,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_5198_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) += 4;
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 20 * CYCLE_UNIT / 2;
}

/* SUBQ.L #<data>,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_51a0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 22 * CYCLE_UNIT / 2;
}

/* SUBQ.L #<data>,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_51a8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 24 * CYCLE_UNIT / 2;
}

/* SUBQ.L #<data>,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_51b0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (2));
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 26 * CYCLE_UNIT / 2;
}

/* SUBQ.L #<data>,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_51b8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 24 * CYCLE_UNIT / 2;
}

/* SUBQ.L #<data>,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_51b9_4)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (6);
return 28 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (F) */
uae_u32 REGPARAM2 CPUFUNC(op_51c0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue (regs.ccrflags, 1) ? 0xff : 0;
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2 + count_cycles;
}

/* DBcc.W Dn,#<data>.W (F) */
uae_u32 REGPARAM2 CPUFUNC(op_51c8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue (regs.ccrflags, 1)) {
	m68k_incpc ((uae_s32)offs + 2);
			m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc () + 2 + (uae_s32)offs + 2);
		return 10 * CYCLE_UNIT / 2;
			}
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 4 * CYCLE_UNIT / 2;
	} else {
		count_cycles += 2 * CYCLE_UNIT / 2;
	}
	m68k_setpc (oldpc + 4);
}}}return 10 * CYCLE_UNIT / 2 + count_cycles;
}

/* Scc.B (An) (F) */
uae_u32 REGPARAM2 CPUFUNC(op_51d0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{{	int val = cctrue (regs.ccrflags, 1) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (F) */
uae_u32 REGPARAM2 CPUFUNC(op_51d8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (regs.ccrflags, 1) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (F) */
uae_u32 REGPARAM2 CPUFUNC(op_51e0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (regs.ccrflags, 1) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (F) */
uae_u32 REGPARAM2 CPUFUNC(op_51e8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (regs.ccrflags, 1) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (F) */
uae_u32 REGPARAM2 CPUFUNC(op_51f0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{{	int val = cctrue (regs.ccrflags, 1) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (F) */
uae_u32 REGPARAM2 CPUFUNC(op_51f8_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (regs.ccrflags, 1) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (F) */
uae_u32 REGPARAM2 CPUFUNC(op_51f9_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{{	int val = cctrue (regs.ccrflags, 1) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (6);
return 18 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (HI) */
uae_u32 REGPARAM2 CPUFUNC(op_52c0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue (regs.ccrflags, 2) ? 0xff : 0;
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2 + count_cycles;
}

/* DBcc.W Dn,#<data>.W (HI) */
uae_u32 REGPARAM2 CPUFUNC(op_52c8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue (regs.ccrflags, 2)) {
	m68k_incpc ((uae_s32)offs + 2);
			m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc () + 2 + (uae_s32)offs + 2);
		return 10 * CYCLE_UNIT / 2;
			}
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 4 * CYCLE_UNIT / 2;
	} else {
		count_cycles += 2 * CYCLE_UNIT / 2;
	}
	m68k_setpc (oldpc + 4);
}}}return 10 * CYCLE_UNIT / 2 + count_cycles;
}

/* Scc.B (An) (HI) */
uae_u32 REGPARAM2 CPUFUNC(op_52d0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{{	int val = cctrue (regs.ccrflags, 2) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (HI) */
uae_u32 REGPARAM2 CPUFUNC(op_52d8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (regs.ccrflags, 2) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (HI) */
uae_u32 REGPARAM2 CPUFUNC(op_52e0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (regs.ccrflags, 2) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (HI) */
uae_u32 REGPARAM2 CPUFUNC(op_52e8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (regs.ccrflags, 2) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (HI) */
uae_u32 REGPARAM2 CPUFUNC(op_52f0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{{	int val = cctrue (regs.ccrflags, 2) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (HI) */
uae_u32 REGPARAM2 CPUFUNC(op_52f8_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (regs.ccrflags, 2) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (HI) */
uae_u32 REGPARAM2 CPUFUNC(op_52f9_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{{	int val = cctrue (regs.ccrflags, 2) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (6);
return 18 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (LS) */
uae_u32 REGPARAM2 CPUFUNC(op_53c0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue (regs.ccrflags, 3) ? 0xff : 0;
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2 + count_cycles;
}

/* DBcc.W Dn,#<data>.W (LS) */
uae_u32 REGPARAM2 CPUFUNC(op_53c8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue (regs.ccrflags, 3)) {
	m68k_incpc ((uae_s32)offs + 2);
			m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc () + 2 + (uae_s32)offs + 2);
		return 10 * CYCLE_UNIT / 2;
			}
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 4 * CYCLE_UNIT / 2;
	} else {
		count_cycles += 2 * CYCLE_UNIT / 2;
	}
	m68k_setpc (oldpc + 4);
}}}return 10 * CYCLE_UNIT / 2 + count_cycles;
}

/* Scc.B (An) (LS) */
uae_u32 REGPARAM2 CPUFUNC(op_53d0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{{	int val = cctrue (regs.ccrflags, 3) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (LS) */
uae_u32 REGPARAM2 CPUFUNC(op_53d8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (regs.ccrflags, 3) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (LS) */
uae_u32 REGPARAM2 CPUFUNC(op_53e0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (regs.ccrflags, 3) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (LS) */
uae_u32 REGPARAM2 CPUFUNC(op_53e8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (regs.ccrflags, 3) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (LS) */
uae_u32 REGPARAM2 CPUFUNC(op_53f0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{{	int val = cctrue (regs.ccrflags, 3) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (LS) */
uae_u32 REGPARAM2 CPUFUNC(op_53f8_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (regs.ccrflags, 3) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (LS) */
uae_u32 REGPARAM2 CPUFUNC(op_53f9_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{{	int val = cctrue (regs.ccrflags, 3) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (6);
return 18 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (CC) */
uae_u32 REGPARAM2 CPUFUNC(op_54c0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue (regs.ccrflags, 4) ? 0xff : 0;
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2 + count_cycles;
}

/* DBcc.W Dn,#<data>.W (CC) */
uae_u32 REGPARAM2 CPUFUNC(op_54c8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue (regs.ccrflags, 4)) {
	m68k_incpc ((uae_s32)offs + 2);
			m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc () + 2 + (uae_s32)offs + 2);
		return 10 * CYCLE_UNIT / 2;
			}
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 4 * CYCLE_UNIT / 2;
	} else {
		count_cycles += 2 * CYCLE_UNIT / 2;
	}
	m68k_setpc (oldpc + 4);
}}}return 10 * CYCLE_UNIT / 2 + count_cycles;
}

/* Scc.B (An) (CC) */
uae_u32 REGPARAM2 CPUFUNC(op_54d0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{{	int val = cctrue (regs.ccrflags, 4) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (CC) */
uae_u32 REGPARAM2 CPUFUNC(op_54d8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (regs.ccrflags, 4) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (CC) */
uae_u32 REGPARAM2 CPUFUNC(op_54e0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (regs.ccrflags, 4) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (CC) */
uae_u32 REGPARAM2 CPUFUNC(op_54e8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (regs.ccrflags, 4) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (CC) */
uae_u32 REGPARAM2 CPUFUNC(op_54f0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{{	int val = cctrue (regs.ccrflags, 4) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (CC) */
uae_u32 REGPARAM2 CPUFUNC(op_54f8_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (regs.ccrflags, 4) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (CC) */
uae_u32 REGPARAM2 CPUFUNC(op_54f9_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{{	int val = cctrue (regs.ccrflags, 4) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (6);
return 18 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (CS) */
uae_u32 REGPARAM2 CPUFUNC(op_55c0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue (regs.ccrflags, 5) ? 0xff : 0;
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2 + count_cycles;
}

/* DBcc.W Dn,#<data>.W (CS) */
uae_u32 REGPARAM2 CPUFUNC(op_55c8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue (regs.ccrflags, 5)) {
	m68k_incpc ((uae_s32)offs + 2);
			m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc () + 2 + (uae_s32)offs + 2);
		return 10 * CYCLE_UNIT / 2;
			}
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 4 * CYCLE_UNIT / 2;
	} else {
		count_cycles += 2 * CYCLE_UNIT / 2;
	}
	m68k_setpc (oldpc + 4);
}}}return 10 * CYCLE_UNIT / 2 + count_cycles;
}

/* Scc.B (An) (CS) */
uae_u32 REGPARAM2 CPUFUNC(op_55d0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{{	int val = cctrue (regs.ccrflags, 5) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (CS) */
uae_u32 REGPARAM2 CPUFUNC(op_55d8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (regs.ccrflags, 5) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (CS) */
uae_u32 REGPARAM2 CPUFUNC(op_55e0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (regs.ccrflags, 5) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (CS) */
uae_u32 REGPARAM2 CPUFUNC(op_55e8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (regs.ccrflags, 5) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (CS) */
uae_u32 REGPARAM2 CPUFUNC(op_55f0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{{	int val = cctrue (regs.ccrflags, 5) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (CS) */
uae_u32 REGPARAM2 CPUFUNC(op_55f8_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (regs.ccrflags, 5) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (CS) */
uae_u32 REGPARAM2 CPUFUNC(op_55f9_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{{	int val = cctrue (regs.ccrflags, 5) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (6);
return 18 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (NE) */
uae_u32 REGPARAM2 CPUFUNC(op_56c0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue (regs.ccrflags, 6) ? 0xff : 0;
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2 + count_cycles;
}

/* DBcc.W Dn,#<data>.W (NE) */
uae_u32 REGPARAM2 CPUFUNC(op_56c8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue (regs.ccrflags, 6)) {
	m68k_incpc ((uae_s32)offs + 2);
			m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc () + 2 + (uae_s32)offs + 2);
		return 10 * CYCLE_UNIT / 2;
			}
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 4 * CYCLE_UNIT / 2;
	} else {
		count_cycles += 2 * CYCLE_UNIT / 2;
	}
	m68k_setpc (oldpc + 4);
}}}return 10 * CYCLE_UNIT / 2 + count_cycles;
}

/* Scc.B (An) (NE) */
uae_u32 REGPARAM2 CPUFUNC(op_56d0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{{	int val = cctrue (regs.ccrflags, 6) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (NE) */
uae_u32 REGPARAM2 CPUFUNC(op_56d8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (regs.ccrflags, 6) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (NE) */
uae_u32 REGPARAM2 CPUFUNC(op_56e0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (regs.ccrflags, 6) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (NE) */
uae_u32 REGPARAM2 CPUFUNC(op_56e8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (regs.ccrflags, 6) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (NE) */
uae_u32 REGPARAM2 CPUFUNC(op_56f0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{{	int val = cctrue (regs.ccrflags, 6) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (NE) */
uae_u32 REGPARAM2 CPUFUNC(op_56f8_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (regs.ccrflags, 6) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (NE) */
uae_u32 REGPARAM2 CPUFUNC(op_56f9_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{{	int val = cctrue (regs.ccrflags, 6) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (6);
return 18 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (EQ) */
uae_u32 REGPARAM2 CPUFUNC(op_57c0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue (regs.ccrflags, 7) ? 0xff : 0;
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2 + count_cycles;
}

/* DBcc.W Dn,#<data>.W (EQ) */
uae_u32 REGPARAM2 CPUFUNC(op_57c8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue (regs.ccrflags, 7)) {
	m68k_incpc ((uae_s32)offs + 2);
			m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc () + 2 + (uae_s32)offs + 2);
		return 10 * CYCLE_UNIT / 2;
			}
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 4 * CYCLE_UNIT / 2;
	} else {
		count_cycles += 2 * CYCLE_UNIT / 2;
	}
	m68k_setpc (oldpc + 4);
}}}return 10 * CYCLE_UNIT / 2 + count_cycles;
}

/* Scc.B (An) (EQ) */
uae_u32 REGPARAM2 CPUFUNC(op_57d0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{{	int val = cctrue (regs.ccrflags, 7) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (EQ) */
uae_u32 REGPARAM2 CPUFUNC(op_57d8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (regs.ccrflags, 7) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (EQ) */
uae_u32 REGPARAM2 CPUFUNC(op_57e0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (regs.ccrflags, 7) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (EQ) */
uae_u32 REGPARAM2 CPUFUNC(op_57e8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (regs.ccrflags, 7) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (EQ) */
uae_u32 REGPARAM2 CPUFUNC(op_57f0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{{	int val = cctrue (regs.ccrflags, 7) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (EQ) */
uae_u32 REGPARAM2 CPUFUNC(op_57f8_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (regs.ccrflags, 7) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (EQ) */
uae_u32 REGPARAM2 CPUFUNC(op_57f9_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{{	int val = cctrue (regs.ccrflags, 7) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (6);
return 18 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (VC) */
uae_u32 REGPARAM2 CPUFUNC(op_58c0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue (regs.ccrflags, 8) ? 0xff : 0;
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2 + count_cycles;
}

/* DBcc.W Dn,#<data>.W (VC) */
uae_u32 REGPARAM2 CPUFUNC(op_58c8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue (regs.ccrflags, 8)) {
	m68k_incpc ((uae_s32)offs + 2);
			m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc () + 2 + (uae_s32)offs + 2);
		return 10 * CYCLE_UNIT / 2;
			}
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 4 * CYCLE_UNIT / 2;
	} else {
		count_cycles += 2 * CYCLE_UNIT / 2;
	}
	m68k_setpc (oldpc + 4);
}}}return 10 * CYCLE_UNIT / 2 + count_cycles;
}

/* Scc.B (An) (VC) */
uae_u32 REGPARAM2 CPUFUNC(op_58d0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{{	int val = cctrue (regs.ccrflags, 8) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (VC) */
uae_u32 REGPARAM2 CPUFUNC(op_58d8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (regs.ccrflags, 8) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (VC) */
uae_u32 REGPARAM2 CPUFUNC(op_58e0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (regs.ccrflags, 8) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (VC) */
uae_u32 REGPARAM2 CPUFUNC(op_58e8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (regs.ccrflags, 8) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (VC) */
uae_u32 REGPARAM2 CPUFUNC(op_58f0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{{	int val = cctrue (regs.ccrflags, 8) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (VC) */
uae_u32 REGPARAM2 CPUFUNC(op_58f8_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (regs.ccrflags, 8) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (VC) */
uae_u32 REGPARAM2 CPUFUNC(op_58f9_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{{	int val = cctrue (regs.ccrflags, 8) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (6);
return 18 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (VS) */
uae_u32 REGPARAM2 CPUFUNC(op_59c0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue (regs.ccrflags, 9) ? 0xff : 0;
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2 + count_cycles;
}

/* DBcc.W Dn,#<data>.W (VS) */
uae_u32 REGPARAM2 CPUFUNC(op_59c8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue (regs.ccrflags, 9)) {
	m68k_incpc ((uae_s32)offs + 2);
			m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc () + 2 + (uae_s32)offs + 2);
		return 10 * CYCLE_UNIT / 2;
			}
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 4 * CYCLE_UNIT / 2;
	} else {
		count_cycles += 2 * CYCLE_UNIT / 2;
	}
	m68k_setpc (oldpc + 4);
}}}return 10 * CYCLE_UNIT / 2 + count_cycles;
}

/* Scc.B (An) (VS) */
uae_u32 REGPARAM2 CPUFUNC(op_59d0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{{	int val = cctrue (regs.ccrflags, 9) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (VS) */
uae_u32 REGPARAM2 CPUFUNC(op_59d8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (regs.ccrflags, 9) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (VS) */
uae_u32 REGPARAM2 CPUFUNC(op_59e0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (regs.ccrflags, 9) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (VS) */
uae_u32 REGPARAM2 CPUFUNC(op_59e8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (regs.ccrflags, 9) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (VS) */
uae_u32 REGPARAM2 CPUFUNC(op_59f0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{{	int val = cctrue (regs.ccrflags, 9) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (VS) */
uae_u32 REGPARAM2 CPUFUNC(op_59f8_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (regs.ccrflags, 9) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (VS) */
uae_u32 REGPARAM2 CPUFUNC(op_59f9_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{{	int val = cctrue (regs.ccrflags, 9) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (6);
return 18 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (PL) */
uae_u32 REGPARAM2 CPUFUNC(op_5ac0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue (regs.ccrflags, 10) ? 0xff : 0;
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2 + count_cycles;
}

/* DBcc.W Dn,#<data>.W (PL) */
uae_u32 REGPARAM2 CPUFUNC(op_5ac8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue (regs.ccrflags, 10)) {
	m68k_incpc ((uae_s32)offs + 2);
			m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc () + 2 + (uae_s32)offs + 2);
		return 10 * CYCLE_UNIT / 2;
			}
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 4 * CYCLE_UNIT / 2;
	} else {
		count_cycles += 2 * CYCLE_UNIT / 2;
	}
	m68k_setpc (oldpc + 4);
}}}return 10 * CYCLE_UNIT / 2 + count_cycles;
}

/* Scc.B (An) (PL) */
uae_u32 REGPARAM2 CPUFUNC(op_5ad0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{{	int val = cctrue (regs.ccrflags, 10) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (PL) */
uae_u32 REGPARAM2 CPUFUNC(op_5ad8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (regs.ccrflags, 10) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (PL) */
uae_u32 REGPARAM2 CPUFUNC(op_5ae0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (regs.ccrflags, 10) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (PL) */
uae_u32 REGPARAM2 CPUFUNC(op_5ae8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (regs.ccrflags, 10) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (PL) */
uae_u32 REGPARAM2 CPUFUNC(op_5af0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{{	int val = cctrue (regs.ccrflags, 10) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (PL) */
uae_u32 REGPARAM2 CPUFUNC(op_5af8_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (regs.ccrflags, 10) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (PL) */
uae_u32 REGPARAM2 CPUFUNC(op_5af9_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{{	int val = cctrue (regs.ccrflags, 10) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (6);
return 18 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (MI) */
uae_u32 REGPARAM2 CPUFUNC(op_5bc0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue (regs.ccrflags, 11) ? 0xff : 0;
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2 + count_cycles;
}

/* DBcc.W Dn,#<data>.W (MI) */
uae_u32 REGPARAM2 CPUFUNC(op_5bc8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue (regs.ccrflags, 11)) {
	m68k_incpc ((uae_s32)offs + 2);
			m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc () + 2 + (uae_s32)offs + 2);
		return 10 * CYCLE_UNIT / 2;
			}
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 4 * CYCLE_UNIT / 2;
	} else {
		count_cycles += 2 * CYCLE_UNIT / 2;
	}
	m68k_setpc (oldpc + 4);
}}}return 10 * CYCLE_UNIT / 2 + count_cycles;
}

/* Scc.B (An) (MI) */
uae_u32 REGPARAM2 CPUFUNC(op_5bd0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{{	int val = cctrue (regs.ccrflags, 11) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (MI) */
uae_u32 REGPARAM2 CPUFUNC(op_5bd8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (regs.ccrflags, 11) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (MI) */
uae_u32 REGPARAM2 CPUFUNC(op_5be0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (regs.ccrflags, 11) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (MI) */
uae_u32 REGPARAM2 CPUFUNC(op_5be8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (regs.ccrflags, 11) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (MI) */
uae_u32 REGPARAM2 CPUFUNC(op_5bf0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{{	int val = cctrue (regs.ccrflags, 11) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (MI) */
uae_u32 REGPARAM2 CPUFUNC(op_5bf8_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (regs.ccrflags, 11) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (MI) */
uae_u32 REGPARAM2 CPUFUNC(op_5bf9_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{{	int val = cctrue (regs.ccrflags, 11) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (6);
return 18 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (GE) */
uae_u32 REGPARAM2 CPUFUNC(op_5cc0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue (regs.ccrflags, 12) ? 0xff : 0;
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2 + count_cycles;
}

/* DBcc.W Dn,#<data>.W (GE) */
uae_u32 REGPARAM2 CPUFUNC(op_5cc8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue (regs.ccrflags, 12)) {
	m68k_incpc ((uae_s32)offs + 2);
			m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc () + 2 + (uae_s32)offs + 2);
		return 10 * CYCLE_UNIT / 2;
			}
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 4 * CYCLE_UNIT / 2;
	} else {
		count_cycles += 2 * CYCLE_UNIT / 2;
	}
	m68k_setpc (oldpc + 4);
}}}return 10 * CYCLE_UNIT / 2 + count_cycles;
}

/* Scc.B (An) (GE) */
uae_u32 REGPARAM2 CPUFUNC(op_5cd0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{{	int val = cctrue (regs.ccrflags, 12) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (GE) */
uae_u32 REGPARAM2 CPUFUNC(op_5cd8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (regs.ccrflags, 12) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (GE) */
uae_u32 REGPARAM2 CPUFUNC(op_5ce0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (regs.ccrflags, 12) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (GE) */
uae_u32 REGPARAM2 CPUFUNC(op_5ce8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (regs.ccrflags, 12) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (GE) */
uae_u32 REGPARAM2 CPUFUNC(op_5cf0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{{	int val = cctrue (regs.ccrflags, 12) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (GE) */
uae_u32 REGPARAM2 CPUFUNC(op_5cf8_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (regs.ccrflags, 12) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (GE) */
uae_u32 REGPARAM2 CPUFUNC(op_5cf9_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{{	int val = cctrue (regs.ccrflags, 12) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (6);
return 18 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (LT) */
uae_u32 REGPARAM2 CPUFUNC(op_5dc0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue (regs.ccrflags, 13) ? 0xff : 0;
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2 + count_cycles;
}

/* DBcc.W Dn,#<data>.W (LT) */
uae_u32 REGPARAM2 CPUFUNC(op_5dc8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue (regs.ccrflags, 13)) {
	m68k_incpc ((uae_s32)offs + 2);
			m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc () + 2 + (uae_s32)offs + 2);
		return 10 * CYCLE_UNIT / 2;
			}
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 4 * CYCLE_UNIT / 2;
	} else {
		count_cycles += 2 * CYCLE_UNIT / 2;
	}
	m68k_setpc (oldpc + 4);
}}}return 10 * CYCLE_UNIT / 2 + count_cycles;
}

/* Scc.B (An) (LT) */
uae_u32 REGPARAM2 CPUFUNC(op_5dd0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{{	int val = cctrue (regs.ccrflags, 13) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (LT) */
uae_u32 REGPARAM2 CPUFUNC(op_5dd8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (regs.ccrflags, 13) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (LT) */
uae_u32 REGPARAM2 CPUFUNC(op_5de0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (regs.ccrflags, 13) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (LT) */
uae_u32 REGPARAM2 CPUFUNC(op_5de8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (regs.ccrflags, 13) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (LT) */
uae_u32 REGPARAM2 CPUFUNC(op_5df0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{{	int val = cctrue (regs.ccrflags, 13) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (LT) */
uae_u32 REGPARAM2 CPUFUNC(op_5df8_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (regs.ccrflags, 13) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (LT) */
uae_u32 REGPARAM2 CPUFUNC(op_5df9_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{{	int val = cctrue (regs.ccrflags, 13) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (6);
return 18 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (GT) */
uae_u32 REGPARAM2 CPUFUNC(op_5ec0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue (regs.ccrflags, 14) ? 0xff : 0;
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2 + count_cycles;
}

/* DBcc.W Dn,#<data>.W (GT) */
uae_u32 REGPARAM2 CPUFUNC(op_5ec8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue (regs.ccrflags, 14)) {
	m68k_incpc ((uae_s32)offs + 2);
			m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc () + 2 + (uae_s32)offs + 2);
		return 10 * CYCLE_UNIT / 2;
			}
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 4 * CYCLE_UNIT / 2;
	} else {
		count_cycles += 2 * CYCLE_UNIT / 2;
	}
	m68k_setpc (oldpc + 4);
}}}return 10 * CYCLE_UNIT / 2 + count_cycles;
}

/* Scc.B (An) (GT) */
uae_u32 REGPARAM2 CPUFUNC(op_5ed0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{{	int val = cctrue (regs.ccrflags, 14) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (GT) */
uae_u32 REGPARAM2 CPUFUNC(op_5ed8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (regs.ccrflags, 14) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (GT) */
uae_u32 REGPARAM2 CPUFUNC(op_5ee0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (regs.ccrflags, 14) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (GT) */
uae_u32 REGPARAM2 CPUFUNC(op_5ee8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (regs.ccrflags, 14) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (GT) */
uae_u32 REGPARAM2 CPUFUNC(op_5ef0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{{	int val = cctrue (regs.ccrflags, 14) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (GT) */
uae_u32 REGPARAM2 CPUFUNC(op_5ef8_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (regs.ccrflags, 14) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (GT) */
uae_u32 REGPARAM2 CPUFUNC(op_5ef9_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{{	int val = cctrue (regs.ccrflags, 14) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (6);
return 18 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (LE) */
uae_u32 REGPARAM2 CPUFUNC(op_5fc0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue (regs.ccrflags, 15) ? 0xff : 0;
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2 + count_cycles;
}

/* DBcc.W Dn,#<data>.W (LE) */
uae_u32 REGPARAM2 CPUFUNC(op_5fc8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue (regs.ccrflags, 15)) {
	m68k_incpc ((uae_s32)offs + 2);
			m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc () + 2 + (uae_s32)offs + 2);
		return 10 * CYCLE_UNIT / 2;
			}
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 4 * CYCLE_UNIT / 2;
	} else {
		count_cycles += 2 * CYCLE_UNIT / 2;
	}
	m68k_setpc (oldpc + 4);
}}}return 10 * CYCLE_UNIT / 2 + count_cycles;
}

/* Scc.B (An) (LE) */
uae_u32 REGPARAM2 CPUFUNC(op_5fd0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{{	int val = cctrue (regs.ccrflags, 15) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (LE) */
uae_u32 REGPARAM2 CPUFUNC(op_5fd8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (regs.ccrflags, 15) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (LE) */
uae_u32 REGPARAM2 CPUFUNC(op_5fe0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (regs.ccrflags, 15) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (LE) */
uae_u32 REGPARAM2 CPUFUNC(op_5fe8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (regs.ccrflags, 15) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (LE) */
uae_u32 REGPARAM2 CPUFUNC(op_5ff0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{{	int val = cctrue (regs.ccrflags, 15) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (LE) */
uae_u32 REGPARAM2 CPUFUNC(op_5ff8_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (regs.ccrflags, 15) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (LE) */
uae_u32 REGPARAM2 CPUFUNC(op_5ff9_4)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{{	int val = cctrue (regs.ccrflags, 15) ? 0xff : 0;
	put_byte (srca, val);
}}}}	m68k_incpc (6);
return 18 * CYCLE_UNIT / 2;
}

/* Bcc.W #<data>.W (T) */
uae_u32 REGPARAM2 CPUFUNC(op_6000_4)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
	if (!cctrue (regs.ccrflags, 0)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (4);
}}return 12 * CYCLE_UNIT / 2;
}

/* BccQ.B #<data> (T) */
uae_u32 REGPARAM2 CPUFUNC(op_6001_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue (regs.ccrflags, 0)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (2);
}}return 8 * CYCLE_UNIT / 2;
}

/* Bcc.L #<data>.L (T) */
uae_u32 REGPARAM2 CPUFUNC(op_60ff_4)(uae_u32 opcode)
{
{	if (cctrue (regs.ccrflags, 0)) {
		exception3i (opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc (2);
}return 8 * CYCLE_UNIT / 2;
}

/* BSR.W #<data>.W */
uae_u32 REGPARAM2 CPUFUNC(op_6100_4)(uae_u32 opcode)
{
{	uae_s32 s;
{	uae_s16 src = get_diword (2);
	s = (uae_s32)src + 2;
	if (src & 1) {
		exception3b (opcode, m68k_getpc () + s, 0, 1, m68k_getpc () + s);
		return 8 * CYCLE_UNIT / 2;
	}
	m68k_do_bsr (m68k_getpc () + 4, s);
}}return 18 * CYCLE_UNIT / 2;
}

/* BSRQ.B #<data> */
uae_u32 REGPARAM2 CPUFUNC(op_6101_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{	uae_s32 s;
{	uae_u32 src = srcreg;
	s = (uae_s32)src + 2;
	if (src & 1) {
		exception3b (opcode, m68k_getpc () + s, 0, 1, m68k_getpc () + s);
		return 4 * CYCLE_UNIT / 2;
	}
	m68k_do_bsr (m68k_getpc () + 2, s);
}}return 18 * CYCLE_UNIT / 2;
}

/* BSR.L #<data>.L */
uae_u32 REGPARAM2 CPUFUNC(op_61ff_4)(uae_u32 opcode)
{
{	uae_s32 s;
	uae_u32 src = 0xffffffff;
	s = (uae_s32)src + 2;
	if (src & 1) {
		exception3b (opcode, m68k_getpc () + s, 0, 1, m68k_getpc () + s);
		return 4 * CYCLE_UNIT / 2;
	}
	m68k_do_bsr (m68k_getpc () + 2, s);
}return 18 * CYCLE_UNIT / 2;
}

/* Bcc.W #<data>.W (HI) */
uae_u32 REGPARAM2 CPUFUNC(op_6200_4)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
	if (!cctrue (regs.ccrflags, 2)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (4);
}}return 12 * CYCLE_UNIT / 2;
}

/* BccQ.B #<data> (HI) */
uae_u32 REGPARAM2 CPUFUNC(op_6201_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue (regs.ccrflags, 2)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (2);
}}return 8 * CYCLE_UNIT / 2;
}

/* Bcc.L #<data>.L (HI) */
uae_u32 REGPARAM2 CPUFUNC(op_62ff_4)(uae_u32 opcode)
{
{	if (cctrue (regs.ccrflags, 2)) {
		exception3i (opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc (2);
}return 8 * CYCLE_UNIT / 2;
}

/* Bcc.W #<data>.W (LS) */
uae_u32 REGPARAM2 CPUFUNC(op_6300_4)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
	if (!cctrue (regs.ccrflags, 3)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (4);
}}return 12 * CYCLE_UNIT / 2;
}

/* BccQ.B #<data> (LS) */
uae_u32 REGPARAM2 CPUFUNC(op_6301_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue (regs.ccrflags, 3)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (2);
}}return 8 * CYCLE_UNIT / 2;
}

/* Bcc.L #<data>.L (LS) */
uae_u32 REGPARAM2 CPUFUNC(op_63ff_4)(uae_u32 opcode)
{
{	if (cctrue (regs.ccrflags, 3)) {
		exception3i (opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc (2);
}return 8 * CYCLE_UNIT / 2;
}

#endif

#ifdef PART_6
/* Bcc.W #<data>.W (CC) */
uae_u32 REGPARAM2 CPUFUNC(op_6400_4)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
	if (!cctrue (regs.ccrflags, 4)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (4);
}}return 12 * CYCLE_UNIT / 2;
}

/* BccQ.B #<data> (CC) */
uae_u32 REGPARAM2 CPUFUNC(op_6401_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue (regs.ccrflags, 4)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (2);
}}return 8 * CYCLE_UNIT / 2;
}

/* Bcc.L #<data>.L (CC) */
uae_u32 REGPARAM2 CPUFUNC(op_64ff_4)(uae_u32 opcode)
{
{	if (cctrue (regs.ccrflags, 4)) {
		exception3i (opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc (2);
}return 8 * CYCLE_UNIT / 2;
}

/* Bcc.W #<data>.W (CS) */
uae_u32 REGPARAM2 CPUFUNC(op_6500_4)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
	if (!cctrue (regs.ccrflags, 5)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (4);
}}return 12 * CYCLE_UNIT / 2;
}

/* BccQ.B #<data> (CS) */
uae_u32 REGPARAM2 CPUFUNC(op_6501_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue (regs.ccrflags, 5)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (2);
}}return 8 * CYCLE_UNIT / 2;
}

/* Bcc.L #<data>.L (CS) */
uae_u32 REGPARAM2 CPUFUNC(op_65ff_4)(uae_u32 opcode)
{
{	if (cctrue (regs.ccrflags, 5)) {
		exception3i (opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc (2);
}return 8 * CYCLE_UNIT / 2;
}

/* Bcc.W #<data>.W (NE) */
uae_u32 REGPARAM2 CPUFUNC(op_6600_4)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
	if (!cctrue (regs.ccrflags, 6)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (4);
}}return 12 * CYCLE_UNIT / 2;
}

/* BccQ.B #<data> (NE) */
uae_u32 REGPARAM2 CPUFUNC(op_6601_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue (regs.ccrflags, 6)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (2);
}}return 8 * CYCLE_UNIT / 2;
}

/* Bcc.L #<data>.L (NE) */
uae_u32 REGPARAM2 CPUFUNC(op_66ff_4)(uae_u32 opcode)
{
{	if (cctrue (regs.ccrflags, 6)) {
		exception3i (opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc (2);
}return 8 * CYCLE_UNIT / 2;
}

/* Bcc.W #<data>.W (EQ) */
uae_u32 REGPARAM2 CPUFUNC(op_6700_4)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
	if (!cctrue (regs.ccrflags, 7)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (4);
}}return 12 * CYCLE_UNIT / 2;
}

/* BccQ.B #<data> (EQ) */
uae_u32 REGPARAM2 CPUFUNC(op_6701_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue (regs.ccrflags, 7)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (2);
}}return 8 * CYCLE_UNIT / 2;
}

/* Bcc.L #<data>.L (EQ) */
uae_u32 REGPARAM2 CPUFUNC(op_67ff_4)(uae_u32 opcode)
{
{	if (cctrue (regs.ccrflags, 7)) {
		exception3i (opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc (2);
}return 8 * CYCLE_UNIT / 2;
}

/* Bcc.W #<data>.W (VC) */
uae_u32 REGPARAM2 CPUFUNC(op_6800_4)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
	if (!cctrue (regs.ccrflags, 8)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (4);
}}return 12 * CYCLE_UNIT / 2;
}

/* BccQ.B #<data> (VC) */
uae_u32 REGPARAM2 CPUFUNC(op_6801_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue (regs.ccrflags, 8)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (2);
}}return 8 * CYCLE_UNIT / 2;
}

/* Bcc.L #<data>.L (VC) */
uae_u32 REGPARAM2 CPUFUNC(op_68ff_4)(uae_u32 opcode)
{
{	if (cctrue (regs.ccrflags, 8)) {
		exception3i (opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc (2);
}return 8 * CYCLE_UNIT / 2;
}

/* Bcc.W #<data>.W (VS) */
uae_u32 REGPARAM2 CPUFUNC(op_6900_4)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
	if (!cctrue (regs.ccrflags, 9)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (4);
}}return 12 * CYCLE_UNIT / 2;
}

/* BccQ.B #<data> (VS) */
uae_u32 REGPARAM2 CPUFUNC(op_6901_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue (regs.ccrflags, 9)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (2);
}}return 8 * CYCLE_UNIT / 2;
}

/* Bcc.L #<data>.L (VS) */
uae_u32 REGPARAM2 CPUFUNC(op_69ff_4)(uae_u32 opcode)
{
{	if (cctrue (regs.ccrflags, 9)) {
		exception3i (opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc (2);
}return 8 * CYCLE_UNIT / 2;
}

/* Bcc.W #<data>.W (PL) */
uae_u32 REGPARAM2 CPUFUNC(op_6a00_4)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
	if (!cctrue (regs.ccrflags, 10)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (4);
}}return 12 * CYCLE_UNIT / 2;
}

/* BccQ.B #<data> (PL) */
uae_u32 REGPARAM2 CPUFUNC(op_6a01_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue (regs.ccrflags, 10)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (2);
}}return 8 * CYCLE_UNIT / 2;
}

/* Bcc.L #<data>.L (PL) */
uae_u32 REGPARAM2 CPUFUNC(op_6aff_4)(uae_u32 opcode)
{
{	if (cctrue (regs.ccrflags, 10)) {
		exception3i (opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc (2);
}return 8 * CYCLE_UNIT / 2;
}

/* Bcc.W #<data>.W (MI) */
uae_u32 REGPARAM2 CPUFUNC(op_6b00_4)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
	if (!cctrue (regs.ccrflags, 11)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (4);
}}return 12 * CYCLE_UNIT / 2;
}

/* BccQ.B #<data> (MI) */
uae_u32 REGPARAM2 CPUFUNC(op_6b01_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue (regs.ccrflags, 11)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (2);
}}return 8 * CYCLE_UNIT / 2;
}

/* Bcc.L #<data>.L (MI) */
uae_u32 REGPARAM2 CPUFUNC(op_6bff_4)(uae_u32 opcode)
{
{	if (cctrue (regs.ccrflags, 11)) {
		exception3i (opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc (2);
}return 8 * CYCLE_UNIT / 2;
}

/* Bcc.W #<data>.W (GE) */
uae_u32 REGPARAM2 CPUFUNC(op_6c00_4)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
	if (!cctrue (regs.ccrflags, 12)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (4);
}}return 12 * CYCLE_UNIT / 2;
}

/* BccQ.B #<data> (GE) */
uae_u32 REGPARAM2 CPUFUNC(op_6c01_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue (regs.ccrflags, 12)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (2);
}}return 8 * CYCLE_UNIT / 2;
}

/* Bcc.L #<data>.L (GE) */
uae_u32 REGPARAM2 CPUFUNC(op_6cff_4)(uae_u32 opcode)
{
{	if (cctrue (regs.ccrflags, 12)) {
		exception3i (opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc (2);
}return 8 * CYCLE_UNIT / 2;
}

/* Bcc.W #<data>.W (LT) */
uae_u32 REGPARAM2 CPUFUNC(op_6d00_4)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
	if (!cctrue (regs.ccrflags, 13)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (4);
}}return 12 * CYCLE_UNIT / 2;
}

/* BccQ.B #<data> (LT) */
uae_u32 REGPARAM2 CPUFUNC(op_6d01_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue (regs.ccrflags, 13)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (2);
}}return 8 * CYCLE_UNIT / 2;
}

/* Bcc.L #<data>.L (LT) */
uae_u32 REGPARAM2 CPUFUNC(op_6dff_4)(uae_u32 opcode)
{
{	if (cctrue (regs.ccrflags, 13)) {
		exception3i (opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc (2);
}return 8 * CYCLE_UNIT / 2;
}

/* Bcc.W #<data>.W (GT) */
uae_u32 REGPARAM2 CPUFUNC(op_6e00_4)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
	if (!cctrue (regs.ccrflags, 14)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (4);
}}return 12 * CYCLE_UNIT / 2;
}

/* BccQ.B #<data> (GT) */
uae_u32 REGPARAM2 CPUFUNC(op_6e01_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue (regs.ccrflags, 14)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (2);
}}return 8 * CYCLE_UNIT / 2;
}

/* Bcc.L #<data>.L (GT) */
uae_u32 REGPARAM2 CPUFUNC(op_6eff_4)(uae_u32 opcode)
{
{	if (cctrue (regs.ccrflags, 14)) {
		exception3i (opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc (2);
}return 8 * CYCLE_UNIT / 2;
}

/* Bcc.W #<data>.W (LE) */
uae_u32 REGPARAM2 CPUFUNC(op_6f00_4)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
	if (!cctrue (regs.ccrflags, 15)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (4);
}}return 12 * CYCLE_UNIT / 2;
}

/* BccQ.B #<data> (LE) */
uae_u32 REGPARAM2 CPUFUNC(op_6f01_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue (regs.ccrflags, 15)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (2);
}}return 8 * CYCLE_UNIT / 2;
}

/* Bcc.L #<data>.L (LE) */
uae_u32 REGPARAM2 CPUFUNC(op_6fff_4)(uae_u32 opcode)
{
{	if (cctrue (regs.ccrflags, 15)) {
		exception3i (opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc (2);
}return 8 * CYCLE_UNIT / 2;
}

/* MOVEQ.L #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_7000_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_u32 src = srcreg;
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
	m68k_incpc (2);
}}}return 4 * CYCLE_UNIT / 2;
}

/* OR.B Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8000_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* OR.B (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8010_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* OR.B (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8018_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* OR.B -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8020_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* OR.B (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8028_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* OR.B (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8030_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* OR.B (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8038_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* OR.B (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8039_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (6);
return 16 * CYCLE_UNIT / 2;
}

/* OR.B (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_803a_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* OR.B (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_803b_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* OR.B #<data>.B,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_803c_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = get_dibyte (2);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}	m68k_incpc (4);
return 8 * CYCLE_UNIT / 2;
}

/* OR.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8040_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* OR.W (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8050_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* OR.W (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8058_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) += 2;
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* OR.W -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8060_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* OR.W (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8068_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* OR.W (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8070_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* OR.W (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8078_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* OR.W (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8079_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (6);
return 16 * CYCLE_UNIT / 2;
}

/* OR.W (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_807a_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* OR.W (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_807b_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* OR.W #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_807c_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}	m68k_incpc (4);
return 8 * CYCLE_UNIT / 2;
}

/* OR.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8080_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* OR.L (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8090_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* OR.L (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8098_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) += 4;
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* OR.L -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_80a0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}}	m68k_incpc (2);
return 16 * CYCLE_UNIT / 2;
}

/* OR.L (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_80a8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* OR.L (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_80b0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}}	m68k_incpc (4);
return 20 * CYCLE_UNIT / 2;
}

/* OR.L (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_80b8_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* OR.L (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_80b9_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}}	m68k_incpc (6);
return 22 * CYCLE_UNIT / 2;
}

/* OR.L (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_80ba_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* OR.L (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_80bb_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}}	m68k_incpc (4);
return 20 * CYCLE_UNIT / 2;
}

/* OR.L #<data>.L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_80bc_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}	m68k_incpc (6);
return 16 * CYCLE_UNIT / 2;
}

/* DIVU.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_80c0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	CLEAR_CZNV ();
	if (src == 0) {
	m68k_incpc (2);
		Exception (5);
		return 4 * CYCLE_UNIT / 2;
	} else {
		uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
		uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
		count_cycles += ((getDivu68kCycles((uae_u32)dst, (uae_u16)src)) - 4) * CYCLE_UNIT / 2;
		if (newv > 0xffff) {
			SET_VFLG (1);
			SET_NFLG (1);
		} else {
			CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(newv)) == 0);
	SET_NFLG   (((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg (regs, dstreg) = (newv);
		}
	m68k_incpc (2);
	}
}}}return 4 * CYCLE_UNIT / 2 + count_cycles;
}

/* DIVU.W (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_80d0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	CLEAR_CZNV ();
	if (src == 0) {
	m68k_incpc (2);
		Exception (5);
		return 8 * CYCLE_UNIT / 2;
	} else {
		uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
		uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
		count_cycles += ((getDivu68kCycles((uae_u32)dst, (uae_u16)src)) - 4) * CYCLE_UNIT / 2;
		if (newv > 0xffff) {
			SET_VFLG (1);
			SET_NFLG (1);
		} else {
			CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(newv)) == 0);
	SET_NFLG   (((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg (regs, dstreg) = (newv);
		}
	m68k_incpc (2);
	}
}}}}return 8 * CYCLE_UNIT / 2 + count_cycles;
}

/* DIVU.W (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_80d8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) += 2;
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	CLEAR_CZNV ();
	if (src == 0) {
	m68k_incpc (2);
		Exception (5);
		return 8 * CYCLE_UNIT / 2;
	} else {
		uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
		uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
		count_cycles += ((getDivu68kCycles((uae_u32)dst, (uae_u16)src)) - 4) * CYCLE_UNIT / 2;
		if (newv > 0xffff) {
			SET_VFLG (1);
			SET_NFLG (1);
		} else {
			CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(newv)) == 0);
	SET_NFLG   (((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg (regs, dstreg) = (newv);
		}
	m68k_incpc (2);
	}
}}}}return 8 * CYCLE_UNIT / 2 + count_cycles;
}

/* DIVU.W -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_80e0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	CLEAR_CZNV ();
	if (src == 0) {
	m68k_incpc (2);
		Exception (5);
		return 10 * CYCLE_UNIT / 2;
	} else {
		uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
		uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
		count_cycles += ((getDivu68kCycles((uae_u32)dst, (uae_u16)src)) - 4) * CYCLE_UNIT / 2;
		if (newv > 0xffff) {
			SET_VFLG (1);
			SET_NFLG (1);
		} else {
			CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(newv)) == 0);
	SET_NFLG   (((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg (regs, dstreg) = (newv);
		}
	m68k_incpc (2);
	}
}}}}return 10 * CYCLE_UNIT / 2 + count_cycles;
}

/* DIVU.W (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_80e8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	CLEAR_CZNV ();
	if (src == 0) {
	m68k_incpc (4);
		Exception (5);
		return 12 * CYCLE_UNIT / 2;
	} else {
		uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
		uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
		count_cycles += ((getDivu68kCycles((uae_u32)dst, (uae_u16)src)) - 4) * CYCLE_UNIT / 2;
		if (newv > 0xffff) {
			SET_VFLG (1);
			SET_NFLG (1);
		} else {
			CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(newv)) == 0);
	SET_NFLG   (((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg (regs, dstreg) = (newv);
		}
	m68k_incpc (4);
	}
}}}}return 12 * CYCLE_UNIT / 2 + count_cycles;
}

/* DIVU.W (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_80f0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	CLEAR_CZNV ();
	if (src == 0) {
	m68k_incpc (4);
		Exception (5);
		return 14 * CYCLE_UNIT / 2;
	} else {
		uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
		uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
		count_cycles += ((getDivu68kCycles((uae_u32)dst, (uae_u16)src)) - 4) * CYCLE_UNIT / 2;
		if (newv > 0xffff) {
			SET_VFLG (1);
			SET_NFLG (1);
		} else {
			CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(newv)) == 0);
	SET_NFLG   (((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg (regs, dstreg) = (newv);
		}
	m68k_incpc (4);
	}
}}}}return 14 * CYCLE_UNIT / 2 + count_cycles;
}

/* DIVU.W (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_80f8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	CLEAR_CZNV ();
	if (src == 0) {
	m68k_incpc (4);
		Exception (5);
		return 12 * CYCLE_UNIT / 2;
	} else {
		uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
		uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
		count_cycles += ((getDivu68kCycles((uae_u32)dst, (uae_u16)src)) - 4) * CYCLE_UNIT / 2;
		if (newv > 0xffff) {
			SET_VFLG (1);
			SET_NFLG (1);
		} else {
			CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(newv)) == 0);
	SET_NFLG   (((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg (regs, dstreg) = (newv);
		}
	m68k_incpc (4);
	}
}}}}return 12 * CYCLE_UNIT / 2 + count_cycles;
}

/* DIVU.W (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_80f9_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	CLEAR_CZNV ();
	if (src == 0) {
	m68k_incpc (6);
		Exception (5);
		return 16 * CYCLE_UNIT / 2;
	} else {
		uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
		uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
		count_cycles += ((getDivu68kCycles((uae_u32)dst, (uae_u16)src)) - 4) * CYCLE_UNIT / 2;
		if (newv > 0xffff) {
			SET_VFLG (1);
			SET_NFLG (1);
		} else {
			CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(newv)) == 0);
	SET_NFLG   (((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg (regs, dstreg) = (newv);
		}
	m68k_incpc (6);
	}
}}}}return 16 * CYCLE_UNIT / 2 + count_cycles;
}

/* DIVU.W (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_80fa_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	CLEAR_CZNV ();
	if (src == 0) {
	m68k_incpc (4);
		Exception (5);
		return 12 * CYCLE_UNIT / 2;
	} else {
		uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
		uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
		count_cycles += ((getDivu68kCycles((uae_u32)dst, (uae_u16)src)) - 4) * CYCLE_UNIT / 2;
		if (newv > 0xffff) {
			SET_VFLG (1);
			SET_NFLG (1);
		} else {
			CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(newv)) == 0);
	SET_NFLG   (((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg (regs, dstreg) = (newv);
		}
	m68k_incpc (4);
	}
}}}}return 12 * CYCLE_UNIT / 2 + count_cycles;
}

/* DIVU.W (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_80fb_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	CLEAR_CZNV ();
	if (src == 0) {
	m68k_incpc (4);
		Exception (5);
		return 14 * CYCLE_UNIT / 2;
	} else {
		uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
		uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
		count_cycles += ((getDivu68kCycles((uae_u32)dst, (uae_u16)src)) - 4) * CYCLE_UNIT / 2;
		if (newv > 0xffff) {
			SET_VFLG (1);
			SET_NFLG (1);
		} else {
			CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(newv)) == 0);
	SET_NFLG   (((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg (regs, dstreg) = (newv);
		}
	m68k_incpc (4);
	}
}}}}return 14 * CYCLE_UNIT / 2 + count_cycles;
}

/* DIVU.W #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_80fc_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	CLEAR_CZNV ();
	if (src == 0) {
	m68k_incpc (4);
		Exception (5);
		return 8 * CYCLE_UNIT / 2;
	} else {
		uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
		uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
		count_cycles += ((getDivu68kCycles((uae_u32)dst, (uae_u16)src)) - 4) * CYCLE_UNIT / 2;
		if (newv > 0xffff) {
			SET_VFLG (1);
			SET_NFLG (1);
		} else {
			CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(newv)) == 0);
	SET_NFLG   (((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg (regs, dstreg) = (newv);
		}
	m68k_incpc (4);
	}
}}}return 8 * CYCLE_UNIT / 2 + count_cycles;
}

/* SBCD.B Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8100_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{	uae_u16 newv_lo = (dst & 0xF) - (src & 0xF) - (GET_XFLG () ? 1 : 0);
	uae_u16 newv_hi = (dst & 0xF0) - (src & 0xF0);
	uae_u16 newv, tmp_newv;
	int bcd = 0;
	newv = tmp_newv = newv_hi + newv_lo;
	if (newv_lo & 0xF0) { newv -= 6; bcd = 6; };
	if ((((dst & 0xFF) - (src & 0xFF) - (GET_XFLG () ? 1 : 0)) & 0x100) > 0xFF) { newv -= 0x60; }
	SET_CFLG ((((dst & 0xFF) - (src & 0xFF) - bcd - (GET_XFLG () ? 1 : 0)) & 0x300) > 0xFF);
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s8)(newv)) == 0));
	SET_NFLG (((uae_s8)(newv)) < 0);
	SET_VFLG ((tmp_newv & 0x80) != 0 && (newv & 0x80) == 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* SBCD.B -(An),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_8108_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
{	uae_u16 newv_lo = (dst & 0xF) - (src & 0xF) - (GET_XFLG () ? 1 : 0);
	uae_u16 newv_hi = (dst & 0xF0) - (src & 0xF0);
	uae_u16 newv, tmp_newv;
	int bcd = 0;
	newv = tmp_newv = newv_hi + newv_lo;
	if (newv_lo & 0xF0) { newv -= 6; bcd = 6; };
	if ((((dst & 0xFF) - (src & 0xFF) - (GET_XFLG () ? 1 : 0)) & 0x100) > 0xFF) { newv -= 0x60; }
	SET_CFLG ((((dst & 0xFF) - (src & 0xFF) - bcd - (GET_XFLG () ? 1 : 0)) & 0x300) > 0xFF);
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s8)(newv)) == 0));
	SET_NFLG (((uae_s8)(newv)) < 0);
	SET_VFLG ((tmp_newv & 0x80) != 0 && (newv & 0x80) == 0);
	put_byte (dsta, newv);
}}}}}}	m68k_incpc (2);
return 18 * CYCLE_UNIT / 2;
}

/* OR.B Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_8110_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* OR.B Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_8118_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* OR.B Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_8120_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* OR.B Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_8128_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* OR.B Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_8130_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (2));
{	uae_s8 dst = get_byte (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* OR.B Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_8138_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* OR.B Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_8139_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s8 dst = get_byte (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* OR.W Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_8150_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* OR.W Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_8158_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) += 2;
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* OR.W Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_8160_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* OR.W Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_8168_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 dst = get_word (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* OR.W Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_8170_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (2));
{	uae_s16 dst = get_word (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* OR.W Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_8178_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 dst = get_word (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* OR.W Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_8179_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s16 dst = get_word (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* OR.L Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_8190_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (2);
return 20 * CYCLE_UNIT / 2;
}

/* OR.L Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_8198_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) += 4;
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (2);
return 20 * CYCLE_UNIT / 2;
}

/* OR.L Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_81a0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (2);
return 22 * CYCLE_UNIT / 2;
}

/* OR.L Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_81a8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 dst = get_long (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (4);
return 24 * CYCLE_UNIT / 2;
}

/* OR.L Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_81b0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (2));
{	uae_s32 dst = get_long (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (4);
return 26 * CYCLE_UNIT / 2;
}

/* OR.L Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_81b8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 dst = get_long (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (4);
return 24 * CYCLE_UNIT / 2;
}

/* OR.L Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_81b9_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s32 dst = get_long (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (6);
return 28 * CYCLE_UNIT / 2;
}

/* DIVS.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_81c0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	if (src == 0) {
	m68k_incpc (2);
		Exception (5);
		return 4 * CYCLE_UNIT / 2;
	}
	CLEAR_CZNV ();
		count_cycles += ((getDivs68kCycles((uae_s32)dst, (uae_s16)src)) - 4) * CYCLE_UNIT / 2;
	if (dst == 0x80000000 && src == -1) {
		SET_VFLG (1);
		SET_NFLG (1);
	} else {
		uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
		uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
		if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) {
			SET_VFLG (1);
			SET_NFLG (1);
		} else {
			if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(newv)) == 0);
	SET_NFLG   (((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg (regs, dstreg) = (newv);
		}
	}
	m68k_incpc (2);
}}}return 4 * CYCLE_UNIT / 2 + count_cycles;
}

/* DIVS.W (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_81d0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	if (src == 0) {
	m68k_incpc (2);
		Exception (5);
		return 8 * CYCLE_UNIT / 2;
	}
	CLEAR_CZNV ();
		count_cycles += ((getDivs68kCycles((uae_s32)dst, (uae_s16)src)) - 4) * CYCLE_UNIT / 2;
	if (dst == 0x80000000 && src == -1) {
		SET_VFLG (1);
		SET_NFLG (1);
	} else {
		uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
		uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
		if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) {
			SET_VFLG (1);
			SET_NFLG (1);
		} else {
			if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(newv)) == 0);
	SET_NFLG   (((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg (regs, dstreg) = (newv);
		}
	}
	m68k_incpc (2);
}}}}return 8 * CYCLE_UNIT / 2 + count_cycles;
}

/* DIVS.W (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_81d8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) += 2;
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	if (src == 0) {
	m68k_incpc (2);
		Exception (5);
		return 8 * CYCLE_UNIT / 2;
	}
	CLEAR_CZNV ();
		count_cycles += ((getDivs68kCycles((uae_s32)dst, (uae_s16)src)) - 4) * CYCLE_UNIT / 2;
	if (dst == 0x80000000 && src == -1) {
		SET_VFLG (1);
		SET_NFLG (1);
	} else {
		uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
		uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
		if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) {
			SET_VFLG (1);
			SET_NFLG (1);
		} else {
			if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(newv)) == 0);
	SET_NFLG   (((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg (regs, dstreg) = (newv);
		}
	}
	m68k_incpc (2);
}}}}return 8 * CYCLE_UNIT / 2 + count_cycles;
}

/* DIVS.W -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_81e0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	if (src == 0) {
	m68k_incpc (2);
		Exception (5);
		return 10 * CYCLE_UNIT / 2;
	}
	CLEAR_CZNV ();
		count_cycles += ((getDivs68kCycles((uae_s32)dst, (uae_s16)src)) - 4) * CYCLE_UNIT / 2;
	if (dst == 0x80000000 && src == -1) {
		SET_VFLG (1);
		SET_NFLG (1);
	} else {
		uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
		uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
		if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) {
			SET_VFLG (1);
			SET_NFLG (1);
		} else {
			if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(newv)) == 0);
	SET_NFLG   (((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg (regs, dstreg) = (newv);
		}
	}
	m68k_incpc (2);
}}}}return 10 * CYCLE_UNIT / 2 + count_cycles;
}

/* DIVS.W (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_81e8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	if (src == 0) {
	m68k_incpc (4);
		Exception (5);
		return 12 * CYCLE_UNIT / 2;
	}
	CLEAR_CZNV ();
		count_cycles += ((getDivs68kCycles((uae_s32)dst, (uae_s16)src)) - 4) * CYCLE_UNIT / 2;
	if (dst == 0x80000000 && src == -1) {
		SET_VFLG (1);
		SET_NFLG (1);
	} else {
		uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
		uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
		if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) {
			SET_VFLG (1);
			SET_NFLG (1);
		} else {
			if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(newv)) == 0);
	SET_NFLG   (((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg (regs, dstreg) = (newv);
		}
	}
	m68k_incpc (4);
}}}}return 12 * CYCLE_UNIT / 2 + count_cycles;
}

/* DIVS.W (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_81f0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	if (src == 0) {
	m68k_incpc (4);
		Exception (5);
		return 14 * CYCLE_UNIT / 2;
	}
	CLEAR_CZNV ();
		count_cycles += ((getDivs68kCycles((uae_s32)dst, (uae_s16)src)) - 4) * CYCLE_UNIT / 2;
	if (dst == 0x80000000 && src == -1) {
		SET_VFLG (1);
		SET_NFLG (1);
	} else {
		uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
		uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
		if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) {
			SET_VFLG (1);
			SET_NFLG (1);
		} else {
			if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(newv)) == 0);
	SET_NFLG   (((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg (regs, dstreg) = (newv);
		}
	}
	m68k_incpc (4);
}}}}return 14 * CYCLE_UNIT / 2 + count_cycles;
}

/* DIVS.W (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_81f8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	if (src == 0) {
	m68k_incpc (4);
		Exception (5);
		return 12 * CYCLE_UNIT / 2;
	}
	CLEAR_CZNV ();
		count_cycles += ((getDivs68kCycles((uae_s32)dst, (uae_s16)src)) - 4) * CYCLE_UNIT / 2;
	if (dst == 0x80000000 && src == -1) {
		SET_VFLG (1);
		SET_NFLG (1);
	} else {
		uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
		uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
		if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) {
			SET_VFLG (1);
			SET_NFLG (1);
		} else {
			if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(newv)) == 0);
	SET_NFLG   (((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg (regs, dstreg) = (newv);
		}
	}
	m68k_incpc (4);
}}}}return 12 * CYCLE_UNIT / 2 + count_cycles;
}

/* DIVS.W (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_81f9_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	if (src == 0) {
	m68k_incpc (6);
		Exception (5);
		return 16 * CYCLE_UNIT / 2;
	}
	CLEAR_CZNV ();
		count_cycles += ((getDivs68kCycles((uae_s32)dst, (uae_s16)src)) - 4) * CYCLE_UNIT / 2;
	if (dst == 0x80000000 && src == -1) {
		SET_VFLG (1);
		SET_NFLG (1);
	} else {
		uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
		uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
		if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) {
			SET_VFLG (1);
			SET_NFLG (1);
		} else {
			if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(newv)) == 0);
	SET_NFLG   (((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg (regs, dstreg) = (newv);
		}
	}
	m68k_incpc (6);
}}}}return 16 * CYCLE_UNIT / 2 + count_cycles;
}

/* DIVS.W (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_81fa_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	if (src == 0) {
	m68k_incpc (4);
		Exception (5);
		return 12 * CYCLE_UNIT / 2;
	}
	CLEAR_CZNV ();
		count_cycles += ((getDivs68kCycles((uae_s32)dst, (uae_s16)src)) - 4) * CYCLE_UNIT / 2;
	if (dst == 0x80000000 && src == -1) {
		SET_VFLG (1);
		SET_NFLG (1);
	} else {
		uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
		uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
		if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) {
			SET_VFLG (1);
			SET_NFLG (1);
		} else {
			if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(newv)) == 0);
	SET_NFLG   (((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg (regs, dstreg) = (newv);
		}
	}
	m68k_incpc (4);
}}}}return 12 * CYCLE_UNIT / 2 + count_cycles;
}

/* DIVS.W (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_81fb_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	if (src == 0) {
	m68k_incpc (4);
		Exception (5);
		return 14 * CYCLE_UNIT / 2;
	}
	CLEAR_CZNV ();
		count_cycles += ((getDivs68kCycles((uae_s32)dst, (uae_s16)src)) - 4) * CYCLE_UNIT / 2;
	if (dst == 0x80000000 && src == -1) {
		SET_VFLG (1);
		SET_NFLG (1);
	} else {
		uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
		uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
		if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) {
			SET_VFLG (1);
			SET_NFLG (1);
		} else {
			if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(newv)) == 0);
	SET_NFLG   (((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg (regs, dstreg) = (newv);
		}
	}
	m68k_incpc (4);
}}}}return 14 * CYCLE_UNIT / 2 + count_cycles;
}

/* DIVS.W #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_81fc_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	if (src == 0) {
	m68k_incpc (4);
		Exception (5);
		return 8 * CYCLE_UNIT / 2;
	}
	CLEAR_CZNV ();
		count_cycles += ((getDivs68kCycles((uae_s32)dst, (uae_s16)src)) - 4) * CYCLE_UNIT / 2;
	if (dst == 0x80000000 && src == -1) {
		SET_VFLG (1);
		SET_NFLG (1);
	} else {
		uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
		uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
		if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) {
			SET_VFLG (1);
			SET_NFLG (1);
		} else {
			if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(newv)) == 0);
	SET_NFLG   (((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg (regs, dstreg) = (newv);
		}
	}
	m68k_incpc (4);
}}}return 8 * CYCLE_UNIT / 2 + count_cycles;
}

/* SUB.B Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9000_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* SUB.B (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9010_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* SUB.B (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9018_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* SUB.B -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9020_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* SUB.B (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9028_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* SUB.B (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9030_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* SUB.B (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9038_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* SUB.B (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9039_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}	m68k_incpc (6);
return 16 * CYCLE_UNIT / 2;
}

/* SUB.B (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_903a_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* SUB.B (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_903b_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* SUB.B #<data>.B,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_903c_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = get_dibyte (2);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}	m68k_incpc (4);
return 8 * CYCLE_UNIT / 2;
}

/* SUB.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9040_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* SUB.W An,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9048_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_areg (regs, srcreg);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* SUB.W (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9050_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* SUB.W (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9058_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) += 2;
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* SUB.W -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9060_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* SUB.W (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9068_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* SUB.W (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9070_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* SUB.W (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9078_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* SUB.W (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9079_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}	m68k_incpc (6);
return 16 * CYCLE_UNIT / 2;
}

/* SUB.W (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_907a_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* SUB.W (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_907b_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* SUB.W #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_907c_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}	m68k_incpc (4);
return 8 * CYCLE_UNIT / 2;
}

/* SUB.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9080_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* SUB.L An,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9088_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_areg (regs, srcreg);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* SUB.L (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9090_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* SUB.L (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9098_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) += 4;
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* SUB.L -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_90a0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}}}	m68k_incpc (2);
return 16 * CYCLE_UNIT / 2;
}

/* SUB.L (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_90a8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* SUB.L (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_90b0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}}}	m68k_incpc (4);
return 20 * CYCLE_UNIT / 2;
}

/* SUB.L (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_90b8_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* SUB.L (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_90b9_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}}}	m68k_incpc (6);
return 22 * CYCLE_UNIT / 2;
}

/* SUB.L (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_90ba_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* SUB.L (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_90bb_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}}}	m68k_incpc (4);
return 20 * CYCLE_UNIT / 2;
}

/* SUB.L #<data>.L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_90bc_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}}	m68k_incpc (6);
return 16 * CYCLE_UNIT / 2;
}

/* SUBA.W Dn,An */
uae_u32 REGPARAM2 CPUFUNC(op_90c0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* SUBA.W An,An */
uae_u32 REGPARAM2 CPUFUNC(op_90c8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_areg (regs, srcreg);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* SUBA.W (An),An */
uae_u32 REGPARAM2 CPUFUNC(op_90d0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* SUBA.W (An)+,An */
uae_u32 REGPARAM2 CPUFUNC(op_90d8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) += 2;
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* SUBA.W -(An),An */
uae_u32 REGPARAM2 CPUFUNC(op_90e0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* SUBA.W (d16,An),An */
uae_u32 REGPARAM2 CPUFUNC(op_90e8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* SUBA.W (d8,An,Xn),An */
uae_u32 REGPARAM2 CPUFUNC(op_90f0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* SUBA.W (xxx).W,An */
uae_u32 REGPARAM2 CPUFUNC(op_90f8_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* SUBA.W (xxx).L,An */
uae_u32 REGPARAM2 CPUFUNC(op_90f9_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* SUBA.W (d16,PC),An */
uae_u32 REGPARAM2 CPUFUNC(op_90fa_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* SUBA.W (d8,PC,Xn),An */
uae_u32 REGPARAM2 CPUFUNC(op_90fb_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* SUBA.W #<data>.W,An */
uae_u32 REGPARAM2 CPUFUNC(op_90fc_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* SUBX.B Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9100_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = dst - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s8)(newv)) == 0));
	SET_NFLG (((uae_s8)(newv)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* SUBX.B -(An),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_9108_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
{	uae_u32 newv = dst - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s8)(newv)) == 0));
	SET_NFLG (((uae_s8)(newv)) < 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 18 * CYCLE_UNIT / 2;
}

/* SUB.B Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_9110_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* SUB.B Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_9118_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* SUB.B Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_9120_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* SUB.B Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_9128_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* SUB.B Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_9130_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (2));
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* SUB.B Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_9138_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* SUB.B Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_9139_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* SUBX.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9140_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = dst - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s16)(newv)) == 0));
	SET_NFLG (((uae_s16)(newv)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* SUBX.W -(An),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_9148_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) = dsta;
{	uae_u32 newv = dst - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s16)(newv)) == 0));
	SET_NFLG (((uae_s16)(newv)) < 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 18 * CYCLE_UNIT / 2;
}

/* SUB.W Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_9150_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* SUB.W Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_9158_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) += 2;
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* SUB.W Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_9160_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* SUB.W Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_9168_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* SUB.W Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_9170_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (2));
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* SUB.W Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_9178_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* SUB.W Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_9179_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* SUBX.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9180_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = dst - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s32)(newv)) == 0));
	SET_NFLG (((uae_s32)(newv)) < 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* SUBX.L -(An),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_9188_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) = dsta;
{	uae_u32 newv = dst - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s32)(newv)) == 0));
	SET_NFLG (((uae_s32)(newv)) < 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 30 * CYCLE_UNIT / 2;
}

/* SUB.L Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_9190_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 20 * CYCLE_UNIT / 2;
}

/* SUB.L Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_9198_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) += 4;
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 20 * CYCLE_UNIT / 2;
}

/* SUB.L Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_91a0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 22 * CYCLE_UNIT / 2;
}

/* SUB.L Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_91a8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 24 * CYCLE_UNIT / 2;
}

/* SUB.L Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_91b0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (2));
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 26 * CYCLE_UNIT / 2;
}

/* SUB.L Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_91b8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 24 * CYCLE_UNIT / 2;
}

/* SUB.L Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_91b9_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (6);
return 28 * CYCLE_UNIT / 2;
}

/* SUBA.L Dn,An */
uae_u32 REGPARAM2 CPUFUNC(op_91c0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* SUBA.L An,An */
uae_u32 REGPARAM2 CPUFUNC(op_91c8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_areg (regs, srcreg);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* SUBA.L (An),An */
uae_u32 REGPARAM2 CPUFUNC(op_91d0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* SUBA.L (An)+,An */
uae_u32 REGPARAM2 CPUFUNC(op_91d8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) += 4;
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* SUBA.L -(An),An */
uae_u32 REGPARAM2 CPUFUNC(op_91e0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (2);
return 16 * CYCLE_UNIT / 2;
}

/* SUBA.L (d16,An),An */
uae_u32 REGPARAM2 CPUFUNC(op_91e8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* SUBA.L (d8,An,Xn),An */
uae_u32 REGPARAM2 CPUFUNC(op_91f0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (4);
return 20 * CYCLE_UNIT / 2;
}

/* SUBA.L (xxx).W,An */
uae_u32 REGPARAM2 CPUFUNC(op_91f8_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* SUBA.L (xxx).L,An */
uae_u32 REGPARAM2 CPUFUNC(op_91f9_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (6);
return 22 * CYCLE_UNIT / 2;
}

/* SUBA.L (d16,PC),An */
uae_u32 REGPARAM2 CPUFUNC(op_91fa_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* SUBA.L (d8,PC,Xn),An */
uae_u32 REGPARAM2 CPUFUNC(op_91fb_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (4);
return 20 * CYCLE_UNIT / 2;
}

/* SUBA.L #<data>.L,An */
uae_u32 REGPARAM2 CPUFUNC(op_91fc_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}	m68k_incpc (6);
return 16 * CYCLE_UNIT / 2;
}

/* CMP.B Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b000_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* CMP.B (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b010_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* CMP.B (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b018_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* CMP.B -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b020_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* CMP.B (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b028_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* CMP.B (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b030_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* CMP.B (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b038_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* CMP.B (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b039_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (6);
return 16 * CYCLE_UNIT / 2;
}

/* CMP.B (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b03a_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* CMP.B (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b03b_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* CMP.B #<data>.B,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b03c_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = get_dibyte (2);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}	m68k_incpc (4);
return 8 * CYCLE_UNIT / 2;
}

/* CMP.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b040_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* CMP.W An,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b048_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_areg (regs, srcreg);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* CMP.W (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b050_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* CMP.W (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b058_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) += 2;
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* CMP.W -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b060_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* CMP.W (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b068_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* CMP.W (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b070_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* CMP.W (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b078_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* CMP.W (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b079_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (6);
return 16 * CYCLE_UNIT / 2;
}

/* CMP.W (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b07a_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* CMP.W (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b07b_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* CMP.W #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b07c_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}	m68k_incpc (4);
return 8 * CYCLE_UNIT / 2;
}

/* CMP.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b080_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* CMP.L An,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b088_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_areg (regs, srcreg);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* CMP.L (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b090_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* CMP.L (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b098_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) += 4;
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* CMP.L -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b0a0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (2);
return 16 * CYCLE_UNIT / 2;
}

/* CMP.L (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b0a8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

#endif

#ifdef PART_7
/* CMP.L (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b0b0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 20 * CYCLE_UNIT / 2;
}

/* CMP.L (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b0b8_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* CMP.L (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b0b9_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (6);
return 22 * CYCLE_UNIT / 2;
}

/* CMP.L (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b0ba_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* CMP.L (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b0bb_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 20 * CYCLE_UNIT / 2;
}

/* CMP.L #<data>.L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b0bc_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}	m68k_incpc (6);
return 14 * CYCLE_UNIT / 2;
}

/* CMPA.W Dn,An */
uae_u32 REGPARAM2 CPUFUNC(op_b0c0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* CMPA.W An,An */
uae_u32 REGPARAM2 CPUFUNC(op_b0c8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_areg (regs, srcreg);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* CMPA.W (An),An */
uae_u32 REGPARAM2 CPUFUNC(op_b0d0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* CMPA.W (An)+,An */
uae_u32 REGPARAM2 CPUFUNC(op_b0d8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) += 2;
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* CMPA.W -(An),An */
uae_u32 REGPARAM2 CPUFUNC(op_b0e0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* CMPA.W (d16,An),An */
uae_u32 REGPARAM2 CPUFUNC(op_b0e8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* CMPA.W (d8,An,Xn),An */
uae_u32 REGPARAM2 CPUFUNC(op_b0f0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* CMPA.W (xxx).W,An */
uae_u32 REGPARAM2 CPUFUNC(op_b0f8_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* CMPA.W (xxx).L,An */
uae_u32 REGPARAM2 CPUFUNC(op_b0f9_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (6);
return 18 * CYCLE_UNIT / 2;
}

/* CMPA.W (d16,PC),An */
uae_u32 REGPARAM2 CPUFUNC(op_b0fa_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* CMPA.W (d8,PC,Xn),An */
uae_u32 REGPARAM2 CPUFUNC(op_b0fb_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* CMPA.W #<data>.W,An */
uae_u32 REGPARAM2 CPUFUNC(op_b0fc_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}	m68k_incpc (4);
return 10 * CYCLE_UNIT / 2;
}

/* EOR.B Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b100_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* CMPM.B (An)+,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_b108_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* EOR.B Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_b110_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* EOR.B Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_b118_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* EOR.B Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_b120_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* EOR.B Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_b128_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* EOR.B Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_b130_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (2));
{	uae_s8 dst = get_byte (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* EOR.B Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_b138_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* EOR.B Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_b139_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s8 dst = get_byte (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* EOR.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b140_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* CMPM.W (An)+,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_b148_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) += 2;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) += 2;
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* EOR.W Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_b150_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* EOR.W Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_b158_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) += 2;
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* EOR.W Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_b160_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* EOR.W Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_b168_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 dst = get_word (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* EOR.W Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_b170_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (2));
{	uae_s16 dst = get_word (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* EOR.W Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_b178_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 dst = get_word (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* EOR.W Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_b179_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s16 dst = get_word (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* EOR.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b180_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* CMPM.L (An)+,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_b188_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) += 4;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) += 4;
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}}	m68k_incpc (2);
return 20 * CYCLE_UNIT / 2;
}

/* EOR.L Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_b190_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (2);
return 20 * CYCLE_UNIT / 2;
}

/* EOR.L Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_b198_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) += 4;
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (2);
return 20 * CYCLE_UNIT / 2;
}

/* EOR.L Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_b1a0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (2);
return 22 * CYCLE_UNIT / 2;
}

/* EOR.L Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_b1a8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 dst = get_long (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (4);
return 24 * CYCLE_UNIT / 2;
}

/* EOR.L Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_b1b0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (2));
{	uae_s32 dst = get_long (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (4);
return 26 * CYCLE_UNIT / 2;
}

/* EOR.L Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_b1b8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 dst = get_long (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (4);
return 24 * CYCLE_UNIT / 2;
}

/* EOR.L Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_b1b9_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s32 dst = get_long (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (6);
return 28 * CYCLE_UNIT / 2;
}

/* CMPA.L Dn,An */
uae_u32 REGPARAM2 CPUFUNC(op_b1c0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* CMPA.L An,An */
uae_u32 REGPARAM2 CPUFUNC(op_b1c8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_areg (regs, srcreg);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* CMPA.L (An),An */
uae_u32 REGPARAM2 CPUFUNC(op_b1d0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* CMPA.L (An)+,An */
uae_u32 REGPARAM2 CPUFUNC(op_b1d8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) += 4;
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* CMPA.L -(An),An */
uae_u32 REGPARAM2 CPUFUNC(op_b1e0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (2);
return 16 * CYCLE_UNIT / 2;
}

/* CMPA.L (d16,An),An */
uae_u32 REGPARAM2 CPUFUNC(op_b1e8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* CMPA.L (d8,An,Xn),An */
uae_u32 REGPARAM2 CPUFUNC(op_b1f0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 20 * CYCLE_UNIT / 2;
}

/* CMPA.L (xxx).W,An */
uae_u32 REGPARAM2 CPUFUNC(op_b1f8_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* CMPA.L (xxx).L,An */
uae_u32 REGPARAM2 CPUFUNC(op_b1f9_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (6);
return 22 * CYCLE_UNIT / 2;
}

/* CMPA.L (d16,PC),An */
uae_u32 REGPARAM2 CPUFUNC(op_b1fa_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* CMPA.L (d8,PC,Xn),An */
uae_u32 REGPARAM2 CPUFUNC(op_b1fb_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 20 * CYCLE_UNIT / 2;
}

/* CMPA.L #<data>.L,An */
uae_u32 REGPARAM2 CPUFUNC(op_b1fc_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}	m68k_incpc (6);
return 14 * CYCLE_UNIT / 2;
}

/* AND.B Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c000_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* AND.B (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c010_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* AND.B (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c018_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* AND.B -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c020_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* AND.B (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c028_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* AND.B (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c030_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* AND.B (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c038_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* AND.B (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c039_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (6);
return 16 * CYCLE_UNIT / 2;
}

/* AND.B (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c03a_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* AND.B (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c03b_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* AND.B #<data>.B,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c03c_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = get_dibyte (2);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}	m68k_incpc (4);
return 8 * CYCLE_UNIT / 2;
}

/* AND.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c040_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* AND.W (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c050_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* AND.W (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c058_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) += 2;
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* AND.W -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c060_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* AND.W (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c068_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* AND.W (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c070_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* AND.W (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c078_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* AND.W (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c079_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (6);
return 16 * CYCLE_UNIT / 2;
}

/* AND.W (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c07a_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* AND.W (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c07b_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* AND.W #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c07c_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}	m68k_incpc (4);
return 8 * CYCLE_UNIT / 2;
}

/* AND.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c080_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* AND.L (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c090_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* AND.L (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c098_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) += 4;
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* AND.L -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c0a0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}}	m68k_incpc (2);
return 16 * CYCLE_UNIT / 2;
}

/* AND.L (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c0a8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* AND.L (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c0b0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}}	m68k_incpc (4);
return 20 * CYCLE_UNIT / 2;
}

/* AND.L (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c0b8_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* AND.L (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c0b9_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}}	m68k_incpc (6);
return 22 * CYCLE_UNIT / 2;
}

/* AND.L (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c0ba_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* AND.L (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c0bb_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}}	m68k_incpc (4);
return 20 * CYCLE_UNIT / 2;
}

/* AND.L #<data>.L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c0bc_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}	m68k_incpc (6);
return 16 * CYCLE_UNIT / 2;
}

/* MULU.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c0c0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	count_cycles += (bitset_count16(src) * 2) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (newv);
	m68k_incpc (2);
}}}}return 38 * CYCLE_UNIT / 2 + count_cycles;
}

/* MULU.W (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c0d0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	count_cycles += (bitset_count16(src) * 2) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (newv);
	m68k_incpc (2);
}}}}}return 42 * CYCLE_UNIT / 2 + count_cycles;
}

/* MULU.W (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c0d8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) += 2;
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	count_cycles += (bitset_count16(src) * 2) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (newv);
	m68k_incpc (2);
}}}}}return 42 * CYCLE_UNIT / 2 + count_cycles;
}

/* MULU.W -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c0e0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	count_cycles += (bitset_count16(src) * 2) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (newv);
	m68k_incpc (2);
}}}}}return 44 * CYCLE_UNIT / 2 + count_cycles;
}

/* MULU.W (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c0e8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	count_cycles += (bitset_count16(src) * 2) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (newv);
	m68k_incpc (4);
}}}}}return 46 * CYCLE_UNIT / 2 + count_cycles;
}

/* MULU.W (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c0f0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	count_cycles += (bitset_count16(src) * 2) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (newv);
	m68k_incpc (4);
}}}}}return 48 * CYCLE_UNIT / 2 + count_cycles;
}

/* MULU.W (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c0f8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	count_cycles += (bitset_count16(src) * 2) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (newv);
	m68k_incpc (4);
}}}}}return 46 * CYCLE_UNIT / 2 + count_cycles;
}

/* MULU.W (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c0f9_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	count_cycles += (bitset_count16(src) * 2) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (newv);
	m68k_incpc (6);
}}}}}return 50 * CYCLE_UNIT / 2 + count_cycles;
}

/* MULU.W (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c0fa_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	count_cycles += (bitset_count16(src) * 2) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (newv);
	m68k_incpc (4);
}}}}}return 46 * CYCLE_UNIT / 2 + count_cycles;
}

/* MULU.W (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c0fb_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	count_cycles += (bitset_count16(src) * 2) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (newv);
	m68k_incpc (4);
}}}}}return 48 * CYCLE_UNIT / 2 + count_cycles;
}

/* MULU.W #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c0fc_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	count_cycles += (bitset_count16(src) * 2) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (newv);
	m68k_incpc (4);
}}}}return 42 * CYCLE_UNIT / 2 + count_cycles;
}

/* ABCD.B Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c100_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{	uae_u16 newv_lo = (src & 0xF) + (dst & 0xF) + (GET_XFLG () ? 1 : 0);
	uae_u16 newv_hi = (src & 0xF0) + (dst & 0xF0);
	uae_u16 newv, tmp_newv;
	int cflg;
	newv = tmp_newv = newv_hi + newv_lo;
	if (newv_lo > 9) { newv += 6; }
	cflg = (newv & 0x3F0) > 0x90;
	if (cflg) newv += 0x60;
	SET_CFLG (cflg);
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s8)(newv)) == 0));
	SET_NFLG (((uae_s8)(newv)) < 0);
	SET_VFLG ((tmp_newv & 0x80) == 0 && (newv & 0x80) != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* ABCD.B -(An),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_c108_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
{	uae_u16 newv_lo = (src & 0xF) + (dst & 0xF) + (GET_XFLG () ? 1 : 0);
	uae_u16 newv_hi = (src & 0xF0) + (dst & 0xF0);
	uae_u16 newv, tmp_newv;
	int cflg;
	newv = tmp_newv = newv_hi + newv_lo;
	if (newv_lo > 9) { newv += 6; }
	cflg = (newv & 0x3F0) > 0x90;
	if (cflg) newv += 0x60;
	SET_CFLG (cflg);
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s8)(newv)) == 0));
	SET_NFLG (((uae_s8)(newv)) < 0);
	SET_VFLG ((tmp_newv & 0x80) == 0 && (newv & 0x80) != 0);
	put_byte (dsta, newv);
}}}}}}	m68k_incpc (2);
return 18 * CYCLE_UNIT / 2;
}

/* AND.B Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_c110_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* AND.B Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_c118_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* AND.B Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_c120_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* AND.B Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_c128_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* AND.B Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_c130_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (2));
{	uae_s8 dst = get_byte (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* AND.B Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_c138_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* AND.B Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_c139_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s8 dst = get_byte (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte (dsta, src);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* EXG.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c140_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	m68k_dreg (regs, srcreg) = (dst);
	m68k_dreg (regs, dstreg) = (src);
}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* EXG.L An,An */
uae_u32 REGPARAM2 CPUFUNC(op_c148_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_areg (regs, srcreg);
{	uae_s32 dst = m68k_areg (regs, dstreg);
	m68k_areg (regs, srcreg) = (dst);
	m68k_areg (regs, dstreg) = (src);
}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* AND.W Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_c150_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* AND.W Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_c158_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) += 2;
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* AND.W Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_c160_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* AND.W Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_c168_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 dst = get_word (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* AND.W Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_c170_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (2));
{	uae_s16 dst = get_word (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* AND.W Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_c178_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 dst = get_word (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* AND.W Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_c179_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s16 dst = get_word (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word (dsta, src);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* EXG.L Dn,An */
uae_u32 REGPARAM2 CPUFUNC(op_c188_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uae_s32 dst = m68k_areg (regs, dstreg);
	m68k_dreg (regs, srcreg) = (dst);
	m68k_areg (regs, dstreg) = (src);
}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* AND.L Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_c190_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (2);
return 20 * CYCLE_UNIT / 2;
}

/* AND.L Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_c198_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) += 4;
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (2);
return 20 * CYCLE_UNIT / 2;
}

/* AND.L Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_c1a0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (2);
return 22 * CYCLE_UNIT / 2;
}

/* AND.L Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_c1a8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 dst = get_long (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (4);
return 24 * CYCLE_UNIT / 2;
}

/* AND.L Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_c1b0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (2));
{	uae_s32 dst = get_long (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (4);
return 26 * CYCLE_UNIT / 2;
}

/* AND.L Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_c1b8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 dst = get_long (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (4);
return 24 * CYCLE_UNIT / 2;
}

/* AND.L Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_c1b9_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s32 dst = get_long (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long (dsta, src);
}}}}	m68k_incpc (6);
return 28 * CYCLE_UNIT / 2;
}

/* MULS.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c1c0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	count_cycles += (bitset_count16(src ^ (src << 1)) * 2) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (newv);
}}}}	m68k_incpc (2);
return 38 * CYCLE_UNIT / 2 + count_cycles;
}

/* MULS.W (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c1d0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	count_cycles += (bitset_count16(src ^ (src << 1)) * 2) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (2);
return 42 * CYCLE_UNIT / 2 + count_cycles;
}

/* MULS.W (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c1d8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) += 2;
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	count_cycles += (bitset_count16(src ^ (src << 1)) * 2) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (2);
return 42 * CYCLE_UNIT / 2 + count_cycles;
}

/* MULS.W -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c1e0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	count_cycles += (bitset_count16(src ^ (src << 1)) * 2) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (2);
return 44 * CYCLE_UNIT / 2 + count_cycles;
}

/* MULS.W (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c1e8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	count_cycles += (bitset_count16(src ^ (src << 1)) * 2) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (4);
return 46 * CYCLE_UNIT / 2 + count_cycles;
}

/* MULS.W (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c1f0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	count_cycles += (bitset_count16(src ^ (src << 1)) * 2) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (4);
return 48 * CYCLE_UNIT / 2 + count_cycles;
}

/* MULS.W (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c1f8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	count_cycles += (bitset_count16(src ^ (src << 1)) * 2) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (4);
return 46 * CYCLE_UNIT / 2 + count_cycles;
}

/* MULS.W (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c1f9_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	count_cycles += (bitset_count16(src ^ (src << 1)) * 2) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (6);
return 50 * CYCLE_UNIT / 2 + count_cycles;
}

/* MULS.W (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c1fa_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	count_cycles += (bitset_count16(src ^ (src << 1)) * 2) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (4);
return 46 * CYCLE_UNIT / 2 + count_cycles;
}

/* MULS.W (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c1fb_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	count_cycles += (bitset_count16(src ^ (src << 1)) * 2) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (4);
return 48 * CYCLE_UNIT / 2 + count_cycles;
}

/* MULS.W #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c1fc_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	count_cycles += (bitset_count16(src ^ (src << 1)) * 2) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (newv);
}}}}	m68k_incpc (4);
return 42 * CYCLE_UNIT / 2 + count_cycles;
}

/* ADD.B Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d000_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* ADD.B (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d010_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* ADD.B (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d018_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* ADD.B -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d020_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* ADD.B (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d028_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* ADD.B (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d030_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* ADD.B (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d038_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* ADD.B (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d039_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}	m68k_incpc (6);
return 16 * CYCLE_UNIT / 2;
}

/* ADD.B (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d03a_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* ADD.B (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d03b_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* ADD.B #<data>.B,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d03c_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = get_dibyte (2);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}	m68k_incpc (4);
return 8 * CYCLE_UNIT / 2;
}

/* ADD.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d040_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* ADD.W An,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d048_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_areg (regs, srcreg);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* ADD.W (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d050_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* ADD.W (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d058_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) += 2;
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* ADD.W -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d060_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* ADD.W (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d068_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* ADD.W (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d070_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* ADD.W (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d078_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* ADD.W (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d079_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}	m68k_incpc (6);
return 16 * CYCLE_UNIT / 2;
}

/* ADD.W (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d07a_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* ADD.W (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d07b_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2;
}

/* ADD.W #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d07c_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}	m68k_incpc (4);
return 8 * CYCLE_UNIT / 2;
}

/* ADD.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d080_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* ADD.L An,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d088_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_areg (regs, srcreg);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* ADD.L (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d090_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* ADD.L (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d098_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) += 4;
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* ADD.L -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d0a0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}}}	m68k_incpc (2);
return 16 * CYCLE_UNIT / 2;
}

/* ADD.L (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d0a8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* ADD.L (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d0b0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}}}	m68k_incpc (4);
return 20 * CYCLE_UNIT / 2;
}

/* ADD.L (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d0b8_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* ADD.L (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d0b9_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}}}	m68k_incpc (6);
return 22 * CYCLE_UNIT / 2;
}

/* ADD.L (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d0ba_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* ADD.L (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d0bb_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}}}	m68k_incpc (4);
return 20 * CYCLE_UNIT / 2;
}

/* ADD.L #<data>.L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d0bc_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}}	m68k_incpc (6);
return 16 * CYCLE_UNIT / 2;
}

/* ADDA.W Dn,An */
uae_u32 REGPARAM2 CPUFUNC(op_d0c0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* ADDA.W An,An */
uae_u32 REGPARAM2 CPUFUNC(op_d0c8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_areg (regs, srcreg);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* ADDA.W (An),An */
uae_u32 REGPARAM2 CPUFUNC(op_d0d0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* ADDA.W (An)+,An */
uae_u32 REGPARAM2 CPUFUNC(op_d0d8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) += 2;
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* ADDA.W -(An),An */
uae_u32 REGPARAM2 CPUFUNC(op_d0e0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* ADDA.W (d16,An),An */
uae_u32 REGPARAM2 CPUFUNC(op_d0e8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* ADDA.W (d8,An,Xn),An */
uae_u32 REGPARAM2 CPUFUNC(op_d0f0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* ADDA.W (xxx).W,An */
uae_u32 REGPARAM2 CPUFUNC(op_d0f8_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* ADDA.W (xxx).L,An */
uae_u32 REGPARAM2 CPUFUNC(op_d0f9_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* ADDA.W (d16,PC),An */
uae_u32 REGPARAM2 CPUFUNC(op_d0fa_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* ADDA.W (d8,PC,Xn),An */
uae_u32 REGPARAM2 CPUFUNC(op_d0fb_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* ADDA.W #<data>.W,An */
uae_u32 REGPARAM2 CPUFUNC(op_d0fc_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* ADDX.B Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d100_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = dst + src + (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgo) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s8)(newv)) == 0));
	SET_NFLG (((uae_s8)(newv)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* ADDX.B -(An),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_d108_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
{	uae_u32 newv = dst + src + (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgo) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s8)(newv)) == 0));
	SET_NFLG (((uae_s8)(newv)) < 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 18 * CYCLE_UNIT / 2;
}

/* ADD.B Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_d110_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* ADD.B Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_d118_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* ADD.B Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_d120_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* ADD.B Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_d128_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* ADD.B Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_d130_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (2));
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* ADD.B Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_d138_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* ADD.B Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_d139_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte (dsta, newv);
}}}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* ADDX.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d140_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = dst + src + (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgo) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s16)(newv)) == 0));
	SET_NFLG (((uae_s16)(newv)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* ADDX.W -(An),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_d148_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) = dsta;
{	uae_u32 newv = dst + src + (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgo) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s16)(newv)) == 0));
	SET_NFLG (((uae_s16)(newv)) < 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 18 * CYCLE_UNIT / 2;
}

/* ADD.W Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_d150_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* ADD.W Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_d158_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) += 2;
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* ADD.W Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_d160_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* ADD.W Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_d168_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* ADD.W Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_d170_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (2));
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* ADD.W Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_d178_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* ADD.W Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_d179_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word (dsta, newv);
}}}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* ADDX.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d180_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = dst + src + (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgo) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s32)(newv)) == 0));
	SET_NFLG (((uae_s32)(newv)) < 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* ADDX.L -(An),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_d188_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) = dsta;
{	uae_u32 newv = dst + src + (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgo) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s32)(newv)) == 0));
	SET_NFLG (((uae_s32)(newv)) < 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 30 * CYCLE_UNIT / 2;
}

/* ADD.L Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_d190_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 20 * CYCLE_UNIT / 2;
}

/* ADD.L Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_d198_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) += 4;
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 20 * CYCLE_UNIT / 2;
}

/* ADD.L Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_d1a0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (2);
return 22 * CYCLE_UNIT / 2;
}

/* ADD.L Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_d1a8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 24 * CYCLE_UNIT / 2;
}

/* ADD.L Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_d1b0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000 (m68k_areg (regs, dstreg), get_diword (2));
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 26 * CYCLE_UNIT / 2;
}

/* ADD.L Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_d1b8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (4);
return 24 * CYCLE_UNIT / 2;
}

/* ADD.L Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_d1b9_4)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long (dsta, newv);
}}}}}}}	m68k_incpc (6);
return 28 * CYCLE_UNIT / 2;
}

/* ADDA.L Dn,An */
uae_u32 REGPARAM2 CPUFUNC(op_d1c0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* ADDA.L An,An */
uae_u32 REGPARAM2 CPUFUNC(op_d1c8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_areg (regs, srcreg);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* ADDA.L (An),An */
uae_u32 REGPARAM2 CPUFUNC(op_d1d0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* ADDA.L (An)+,An */
uae_u32 REGPARAM2 CPUFUNC(op_d1d8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) += 4;
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* ADDA.L -(An),An */
uae_u32 REGPARAM2 CPUFUNC(op_d1e0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (2);
return 16 * CYCLE_UNIT / 2;
}

/* ADDA.L (d16,An),An */
uae_u32 REGPARAM2 CPUFUNC(op_d1e8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* ADDA.L (d8,An,Xn),An */
uae_u32 REGPARAM2 CPUFUNC(op_d1f0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (4);
return 20 * CYCLE_UNIT / 2;
}

/* ADDA.L (xxx).W,An */
uae_u32 REGPARAM2 CPUFUNC(op_d1f8_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* ADDA.L (xxx).L,An */
uae_u32 REGPARAM2 CPUFUNC(op_d1f9_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (6);
return 22 * CYCLE_UNIT / 2;
}

/* ADDA.L (d16,PC),An */
uae_u32 REGPARAM2 CPUFUNC(op_d1fa_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* ADDA.L (d8,PC,Xn),An */
uae_u32 REGPARAM2 CPUFUNC(op_d1fb_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000 (tmppc, get_diword (2));
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (4);
return 20 * CYCLE_UNIT / 2;
}

/* ADDA.L #<data>.L,An */
uae_u32 REGPARAM2 CPUFUNC(op_d1fc_4)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}	m68k_incpc (6);
return 16 * CYCLE_UNIT / 2;
}

/* ASRQ.B #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e000_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s8 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u8)data;
	uae_u32 sign = (0x80 & val) >> 7;
	cnt &= 63;
	CLEAR_CZNV ();
	if (cnt >= 8) {
		val = 0xff & (uae_u32)-sign;
		SET_CFLG (sign);
		COPY_CARRY ();
	} else {
		val >>= cnt - 1;
		SET_CFLG (val & 1);
		COPY_CARRY ();
		val >>= 1;
		val |= (0xff << (8 - cnt)) & (uae_u32)-sign;
		val &= 0xff;
	}
	SET_ZFLG (((uae_s8)(val)) == 0);
	SET_NFLG (((uae_s8)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2 + count_cycles;
}

/* LSRQ.B #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e008_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s8 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	CLEAR_CZNV ();
	if (cnt >= 8) {
		SET_CFLG ((cnt == 8) & (val >> 7));
		COPY_CARRY ();
		val = 0;
	} else {
		val >>= cnt - 1;
		SET_CFLG (val & 1);
		COPY_CARRY ();
		val >>= 1;
	}
	SET_ZFLG (((uae_s8)(val)) == 0);
	SET_NFLG (((uae_s8)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2 + count_cycles;
}

/* ROXRQ.B #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e010_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s8 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	CLEAR_CZNV ();
{	cnt--;
	{
	uae_u32 carry;
	uae_u32 hival = (val << 1) | GET_XFLG ();
	hival <<= (7 - cnt);
	val >>= cnt;
	carry = val & 1;
	val >>= 1;
	val |= hival;
	SET_XFLG (carry);
	val &= 0xff;
	} }
	SET_CFLG (GET_XFLG ());
	SET_ZFLG (((uae_s8)(val)) == 0);
	SET_NFLG (((uae_s8)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2 + count_cycles;
}

/* RORQ.B #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e018_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s8 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	CLEAR_CZNV ();
{	uae_u32 hival;
	cnt &= 7;
	hival = val << (8 - cnt);
	val >>= cnt;
	val |= hival;
	val &= 0xff;
	SET_CFLG ((val & 0x80) >> 7);
	}
	SET_ZFLG (((uae_s8)(val)) == 0);
	SET_NFLG (((uae_s8)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2 + count_cycles;
}

/* ASR.B Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e020_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 cnt = m68k_dreg (regs, srcreg);
{	uae_s8 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u8)data;
	uae_u32 sign = (0x80 & val) >> 7;
	cnt &= 63;
	CLEAR_CZNV ();
	if (cnt >= 8) {
		val = 0xff & (uae_u32)-sign;
		SET_CFLG (sign);
		COPY_CARRY ();
	} else if (cnt > 0) {
		val >>= cnt - 1;
		SET_CFLG (val & 1);
		COPY_CARRY ();
		val >>= 1;
		val |= (0xff << (8 - cnt)) & (uae_u32)-sign;
		val &= 0xff;
	}
	SET_ZFLG (((uae_s8)(val)) == 0);
	SET_NFLG (((uae_s8)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2 + count_cycles;
}

/* LSR.B Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e028_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 cnt = m68k_dreg (regs, srcreg);
{	uae_s8 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	CLEAR_CZNV ();
	if (cnt >= 8) {
		SET_CFLG ((cnt == 8) & (val >> 7));
		COPY_CARRY ();
		val = 0;
	} else if (cnt > 0) {
		val >>= cnt - 1;
		SET_CFLG (val & 1);
		COPY_CARRY ();
		val >>= 1;
	}
	SET_ZFLG (((uae_s8)(val)) == 0);
	SET_NFLG (((uae_s8)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2 + count_cycles;
}

/* ROXR.B Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e030_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 cnt = m68k_dreg (regs, srcreg);
{	uae_s8 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	CLEAR_CZNV ();
	if (cnt >= 36) cnt -= 36;
	if (cnt >= 18) cnt -= 18;
	if (cnt >= 9) cnt -= 9;
	if (cnt > 0) {
	cnt--;
	{
	uae_u32 carry;
	uae_u32 hival = (val << 1) | GET_XFLG ();
	hival <<= (7 - cnt);
	val >>= cnt;
	carry = val & 1;
	val >>= 1;
	val |= hival;
	SET_XFLG (carry);
	val &= 0xff;
	} }
	SET_CFLG (GET_XFLG ());
	SET_ZFLG (((uae_s8)(val)) == 0);
	SET_NFLG (((uae_s8)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2 + count_cycles;
}

/* ROR.B Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e038_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 cnt = m68k_dreg (regs, srcreg);
{	uae_s8 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	CLEAR_CZNV ();
	if (cnt > 0) {	uae_u32 hival;
	cnt &= 7;
	hival = val << (8 - cnt);
	val >>= cnt;
	val |= hival;
	val &= 0xff;
	SET_CFLG ((val & 0x80) >> 7);
	}
	SET_ZFLG (((uae_s8)(val)) == 0);
	SET_NFLG (((uae_s8)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2 + count_cycles;
}

/* ASRQ.W #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e040_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s16 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = (0x8000 & val) >> 15;
	cnt &= 63;
	CLEAR_CZNV ();
	if (cnt >= 16) {
		val = 0xffff & (uae_u32)-sign;
		SET_CFLG (sign);
		COPY_CARRY ();
	} else {
		val >>= cnt - 1;
		SET_CFLG (val & 1);
		COPY_CARRY ();
		val >>= 1;
		val |= (0xffff << (16 - cnt)) & (uae_u32)-sign;
		val &= 0xffff;
	}
	SET_ZFLG (((uae_s16)(val)) == 0);
	SET_NFLG (((uae_s16)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2 + count_cycles;
}

#endif

#ifdef PART_8
/* LSRQ.W #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e048_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s16 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	CLEAR_CZNV ();
	if (cnt >= 16) {
		SET_CFLG ((cnt == 16) & (val >> 15));
		COPY_CARRY ();
		val = 0;
	} else {
		val >>= cnt - 1;
		SET_CFLG (val & 1);
		COPY_CARRY ();
		val >>= 1;
	}
	SET_ZFLG (((uae_s16)(val)) == 0);
	SET_NFLG (((uae_s16)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2 + count_cycles;
}

/* ROXRQ.W #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e050_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s16 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	CLEAR_CZNV ();
{	cnt--;
	{
	uae_u32 carry;
	uae_u32 hival = (val << 1) | GET_XFLG ();
	hival <<= (15 - cnt);
	val >>= cnt;
	carry = val & 1;
	val >>= 1;
	val |= hival;
	SET_XFLG (carry);
	val &= 0xffff;
	} }
	SET_CFLG (GET_XFLG ());
	SET_ZFLG (((uae_s16)(val)) == 0);
	SET_NFLG (((uae_s16)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2 + count_cycles;
}

/* RORQ.W #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e058_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s16 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	CLEAR_CZNV ();
{	uae_u32 hival;
	cnt &= 15;
	hival = val << (16 - cnt);
	val >>= cnt;
	val |= hival;
	val &= 0xffff;
	SET_CFLG ((val & 0x8000) >> 15);
	}
	SET_ZFLG (((uae_s16)(val)) == 0);
	SET_NFLG (((uae_s16)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2 + count_cycles;
}

/* ASR.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e060_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 cnt = m68k_dreg (regs, srcreg);
{	uae_s16 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = (0x8000 & val) >> 15;
	cnt &= 63;
	CLEAR_CZNV ();
	if (cnt >= 16) {
		val = 0xffff & (uae_u32)-sign;
		SET_CFLG (sign);
		COPY_CARRY ();
	} else if (cnt > 0) {
		val >>= cnt - 1;
		SET_CFLG (val & 1);
		COPY_CARRY ();
		val >>= 1;
		val |= (0xffff << (16 - cnt)) & (uae_u32)-sign;
		val &= 0xffff;
	}
	SET_ZFLG (((uae_s16)(val)) == 0);
	SET_NFLG (((uae_s16)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2 + count_cycles;
}

/* LSR.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e068_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 cnt = m68k_dreg (regs, srcreg);
{	uae_s16 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	CLEAR_CZNV ();
	if (cnt >= 16) {
		SET_CFLG ((cnt == 16) & (val >> 15));
		COPY_CARRY ();
		val = 0;
	} else if (cnt > 0) {
		val >>= cnt - 1;
		SET_CFLG (val & 1);
		COPY_CARRY ();
		val >>= 1;
	}
	SET_ZFLG (((uae_s16)(val)) == 0);
	SET_NFLG (((uae_s16)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2 + count_cycles;
}

/* ROXR.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e070_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 cnt = m68k_dreg (regs, srcreg);
{	uae_s16 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	CLEAR_CZNV ();
	if (cnt >= 34) cnt -= 34;
	if (cnt >= 17) cnt -= 17;
	if (cnt > 0) {
	cnt--;
	{
	uae_u32 carry;
	uae_u32 hival = (val << 1) | GET_XFLG ();
	hival <<= (15 - cnt);
	val >>= cnt;
	carry = val & 1;
	val >>= 1;
	val |= hival;
	SET_XFLG (carry);
	val &= 0xffff;
	} }
	SET_CFLG (GET_XFLG ());
	SET_ZFLG (((uae_s16)(val)) == 0);
	SET_NFLG (((uae_s16)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2 + count_cycles;
}

/* ROR.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e078_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 cnt = m68k_dreg (regs, srcreg);
{	uae_s16 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	CLEAR_CZNV ();
	if (cnt > 0) {	uae_u32 hival;
	cnt &= 15;
	hival = val << (16 - cnt);
	val >>= cnt;
	val |= hival;
	val &= 0xffff;
	SET_CFLG ((val & 0x8000) >> 15);
	}
	SET_ZFLG (((uae_s16)(val)) == 0);
	SET_NFLG (((uae_s16)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2 + count_cycles;
}

/* ASRQ.L #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e080_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s32 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = data;
	uae_u32 sign = (0x80000000 & val) >> 31;
	cnt &= 63;
	CLEAR_CZNV ();
	if (cnt >= 32) {
		val = 0xffffffff & (uae_u32)-sign;
		SET_CFLG (sign);
		COPY_CARRY ();
	} else {
		val >>= cnt - 1;
		SET_CFLG (val & 1);
		COPY_CARRY ();
		val >>= 1;
		val |= (0xffffffff << (32 - cnt)) & (uae_u32)-sign;
		val &= 0xffffffff;
	}
	SET_ZFLG (((uae_s32)(val)) == 0);
	SET_NFLG (((uae_s32)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2 + count_cycles;
}

/* LSRQ.L #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e088_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s32 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	CLEAR_CZNV ();
	if (cnt >= 32) {
		SET_CFLG ((cnt == 32) & (val >> 31));
		COPY_CARRY ();
		val = 0;
	} else {
		val >>= cnt - 1;
		SET_CFLG (val & 1);
		COPY_CARRY ();
		val >>= 1;
	}
	SET_ZFLG (((uae_s32)(val)) == 0);
	SET_NFLG (((uae_s32)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2 + count_cycles;
}

/* ROXRQ.L #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e090_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s32 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	CLEAR_CZNV ();
{	cnt--;
	{
	uae_u32 carry;
	uae_u32 hival = (val << 1) | GET_XFLG ();
	hival <<= (31 - cnt);
	val >>= cnt;
	carry = val & 1;
	val >>= 1;
	val |= hival;
	SET_XFLG (carry);
	val &= 0xffffffff;
	} }
	SET_CFLG (GET_XFLG ());
	SET_ZFLG (((uae_s32)(val)) == 0);
	SET_NFLG (((uae_s32)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2 + count_cycles;
}

/* RORQ.L #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e098_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s32 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	CLEAR_CZNV ();
{	uae_u32 hival;
	cnt &= 31;
	hival = val << (32 - cnt);
	val >>= cnt;
	val |= hival;
	val &= 0xffffffff;
	SET_CFLG ((val & 0x80000000) >> 31);
	}
	SET_ZFLG (((uae_s32)(val)) == 0);
	SET_NFLG (((uae_s32)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2 + count_cycles;
}

/* ASR.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e0a0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 cnt = m68k_dreg (regs, srcreg);
{	uae_s32 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = data;
	uae_u32 sign = (0x80000000 & val) >> 31;
	cnt &= 63;
	CLEAR_CZNV ();
	if (cnt >= 32) {
		val = 0xffffffff & (uae_u32)-sign;
		SET_CFLG (sign);
		COPY_CARRY ();
	} else if (cnt > 0) {
		val >>= cnt - 1;
		SET_CFLG (val & 1);
		COPY_CARRY ();
		val >>= 1;
		val |= (0xffffffff << (32 - cnt)) & (uae_u32)-sign;
		val &= 0xffffffff;
	}
	SET_ZFLG (((uae_s32)(val)) == 0);
	SET_NFLG (((uae_s32)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2 + count_cycles;
}

/* LSR.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e0a8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 cnt = m68k_dreg (regs, srcreg);
{	uae_s32 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	CLEAR_CZNV ();
	if (cnt >= 32) {
		SET_CFLG ((cnt == 32) & (val >> 31));
		COPY_CARRY ();
		val = 0;
	} else if (cnt > 0) {
		val >>= cnt - 1;
		SET_CFLG (val & 1);
		COPY_CARRY ();
		val >>= 1;
	}
	SET_ZFLG (((uae_s32)(val)) == 0);
	SET_NFLG (((uae_s32)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2 + count_cycles;
}

/* ROXR.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e0b0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 cnt = m68k_dreg (regs, srcreg);
{	uae_s32 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	CLEAR_CZNV ();
	if (cnt >= 33) cnt -= 33;
	if (cnt > 0) {
	cnt--;
	{
	uae_u32 carry;
	uae_u32 hival = (val << 1) | GET_XFLG ();
	hival <<= (31 - cnt);
	val >>= cnt;
	carry = val & 1;
	val >>= 1;
	val |= hival;
	SET_XFLG (carry);
	val &= 0xffffffff;
	} }
	SET_CFLG (GET_XFLG ());
	SET_ZFLG (((uae_s32)(val)) == 0);
	SET_NFLG (((uae_s32)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2 + count_cycles;
}

/* ROR.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e0b8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 cnt = m68k_dreg (regs, srcreg);
{	uae_s32 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	CLEAR_CZNV ();
	if (cnt > 0) {	uae_u32 hival;
	cnt &= 31;
	hival = val << (32 - cnt);
	val >>= cnt;
	val |= hival;
	val &= 0xffffffff;
	SET_CFLG ((val & 0x80000000) >> 31);
	}
	SET_ZFLG (((uae_s32)(val)) == 0);
	SET_NFLG (((uae_s32)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2 + count_cycles;
}

/* ASRW.W (An) */
uae_u32 REGPARAM2 CPUFUNC(op_e0d0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg);
{	uae_s16 data = get_word (dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 cflg = val & 1;
	val = (val >> 1) | sign;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (cflg);
	COPY_CARRY ();
	put_word (dataa, val);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* ASRW.W (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_e0d8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg);
{	uae_s16 data = get_word (dataa);
	m68k_areg (regs, srcreg) += 2;
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 cflg = val & 1;
	val = (val >> 1) | sign;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (cflg);
	COPY_CARRY ();
	put_word (dataa, val);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* ASRW.W -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_e0e0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg) - 2;
{	uae_s16 data = get_word (dataa);
	m68k_areg (regs, srcreg) = dataa;
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 cflg = val & 1;
	val = (val >> 1) | sign;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (cflg);
	COPY_CARRY ();
	put_word (dataa, val);
}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* ASRW.W (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_e0e8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 data = get_word (dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 cflg = val & 1;
	val = (val >> 1) | sign;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (cflg);
	COPY_CARRY ();
	put_word (dataa, val);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* ASRW.W (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_e0f0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s16 data = get_word (dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 cflg = val & 1;
	val = (val >> 1) | sign;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (cflg);
	COPY_CARRY ();
	put_word (dataa, val);
}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* ASRW.W (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_e0f8_4)(uae_u32 opcode)
{
{{	uaecptr dataa;
	dataa = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 data = get_word (dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 cflg = val & 1;
	val = (val >> 1) | sign;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (cflg);
	COPY_CARRY ();
	put_word (dataa, val);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* ASRW.W (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_e0f9_4)(uae_u32 opcode)
{
{{	uaecptr dataa;
	dataa = get_dilong (2);
{	uae_s16 data = get_word (dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 cflg = val & 1;
	val = (val >> 1) | sign;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (cflg);
	COPY_CARRY ();
	put_word (dataa, val);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* ASLQ.B #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e100_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s8 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	CLEAR_CZNV ();
	if (cnt >= 8) {
		SET_VFLG (val != 0);
		SET_CFLG (cnt == 8 ? val & 1 : 0);
		COPY_CARRY ();
		val = 0;
	} else {
		uae_u32 mask = (0xff << (7 - cnt)) & 0xff;
		SET_VFLG ((val & mask) != mask && (val & mask) != 0);
		val <<= cnt - 1;
		SET_CFLG ((val & 0x80) >> 7);
		COPY_CARRY ();
		val <<= 1;
		val &= 0xff;
	}
	SET_ZFLG (((uae_s8)(val)) == 0);
	SET_NFLG (((uae_s8)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2 + count_cycles;
}

/* LSLQ.B #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e108_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s8 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	CLEAR_CZNV ();
	if (cnt >= 8) {
		SET_CFLG (cnt == 8 ? val & 1 : 0);
		COPY_CARRY ();
		val = 0;
	} else {
		val <<= (cnt - 1);
		SET_CFLG ((val & 0x80) >> 7);
		COPY_CARRY ();
		val <<= 1;
	val &= 0xff;
	}
	SET_ZFLG (((uae_s8)(val)) == 0);
	SET_NFLG (((uae_s8)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2 + count_cycles;
}

/* ROXLQ.B #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e110_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s8 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	CLEAR_CZNV ();
{	cnt--;
	{
	uae_u32 carry;
	uae_u32 loval = val >> (7 - cnt);
	carry = loval & 1;
	val = (((val << 1) | GET_XFLG ()) << cnt) | (loval >> 1);
	SET_XFLG (carry);
	val &= 0xff;
	} }
	SET_CFLG (GET_XFLG ());
	SET_ZFLG (((uae_s8)(val)) == 0);
	SET_NFLG (((uae_s8)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2 + count_cycles;
}

/* ROLQ.B #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e118_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s8 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	CLEAR_CZNV ();
{	uae_u32 loval;
	cnt &= 7;
	loval = val >> (8 - cnt);
	val <<= cnt;
	val |= loval;
	val &= 0xff;
	SET_CFLG (val & 1);
}
	SET_ZFLG (((uae_s8)(val)) == 0);
	SET_NFLG (((uae_s8)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2 + count_cycles;
}

/* ASL.B Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e120_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 cnt = m68k_dreg (regs, srcreg);
{	uae_s8 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	CLEAR_CZNV ();
	if (cnt >= 8) {
		SET_VFLG (val != 0);
		SET_CFLG (cnt == 8 ? val & 1 : 0);
		COPY_CARRY ();
		val = 0;
	} else if (cnt > 0) {
		uae_u32 mask = (0xff << (7 - cnt)) & 0xff;
		SET_VFLG ((val & mask) != mask && (val & mask) != 0);
		val <<= cnt - 1;
		SET_CFLG ((val & 0x80) >> 7);
		COPY_CARRY ();
		val <<= 1;
		val &= 0xff;
	}
	SET_ZFLG (((uae_s8)(val)) == 0);
	SET_NFLG (((uae_s8)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2 + count_cycles;
}

/* LSL.B Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e128_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 cnt = m68k_dreg (regs, srcreg);
{	uae_s8 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	CLEAR_CZNV ();
	if (cnt >= 8) {
		SET_CFLG (cnt == 8 ? val & 1 : 0);
		COPY_CARRY ();
		val = 0;
	} else if (cnt > 0) {
		val <<= (cnt - 1);
		SET_CFLG ((val & 0x80) >> 7);
		COPY_CARRY ();
		val <<= 1;
	val &= 0xff;
	}
	SET_ZFLG (((uae_s8)(val)) == 0);
	SET_NFLG (((uae_s8)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2 + count_cycles;
}

/* ROXL.B Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e130_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 cnt = m68k_dreg (regs, srcreg);
{	uae_s8 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	CLEAR_CZNV ();
	if (cnt >= 36) cnt -= 36;
	if (cnt >= 18) cnt -= 18;
	if (cnt >= 9) cnt -= 9;
	if (cnt > 0) {
	cnt--;
	{
	uae_u32 carry;
	uae_u32 loval = val >> (7 - cnt);
	carry = loval & 1;
	val = (((val << 1) | GET_XFLG ()) << cnt) | (loval >> 1);
	SET_XFLG (carry);
	val &= 0xff;
	} }
	SET_CFLG (GET_XFLG ());
	SET_ZFLG (((uae_s8)(val)) == 0);
	SET_NFLG (((uae_s8)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2 + count_cycles;
}

/* ROL.B Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e138_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 cnt = m68k_dreg (regs, srcreg);
{	uae_s8 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	CLEAR_CZNV ();
	if (cnt > 0) {
	uae_u32 loval;
	cnt &= 7;
	loval = val >> (8 - cnt);
	val <<= cnt;
	val |= loval;
	val &= 0xff;
	SET_CFLG (val & 1);
}
	SET_ZFLG (((uae_s8)(val)) == 0);
	SET_NFLG (((uae_s8)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2 + count_cycles;
}

/* ASLQ.W #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e140_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s16 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	CLEAR_CZNV ();
	if (cnt >= 16) {
		SET_VFLG (val != 0);
		SET_CFLG (cnt == 16 ? val & 1 : 0);
		COPY_CARRY ();
		val = 0;
	} else {
		uae_u32 mask = (0xffff << (15 - cnt)) & 0xffff;
		SET_VFLG ((val & mask) != mask && (val & mask) != 0);
		val <<= cnt - 1;
		SET_CFLG ((val & 0x8000) >> 15);
		COPY_CARRY ();
		val <<= 1;
		val &= 0xffff;
	}
	SET_ZFLG (((uae_s16)(val)) == 0);
	SET_NFLG (((uae_s16)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2 + count_cycles;
}

/* LSLQ.W #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e148_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s16 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	CLEAR_CZNV ();
	if (cnt >= 16) {
		SET_CFLG (cnt == 16 ? val & 1 : 0);
		COPY_CARRY ();
		val = 0;
	} else {
		val <<= (cnt - 1);
		SET_CFLG ((val & 0x8000) >> 15);
		COPY_CARRY ();
		val <<= 1;
	val &= 0xffff;
	}
	SET_ZFLG (((uae_s16)(val)) == 0);
	SET_NFLG (((uae_s16)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2 + count_cycles;
}

/* ROXLQ.W #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e150_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s16 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	CLEAR_CZNV ();
{	cnt--;
	{
	uae_u32 carry;
	uae_u32 loval = val >> (15 - cnt);
	carry = loval & 1;
	val = (((val << 1) | GET_XFLG ()) << cnt) | (loval >> 1);
	SET_XFLG (carry);
	val &= 0xffff;
	} }
	SET_CFLG (GET_XFLG ());
	SET_ZFLG (((uae_s16)(val)) == 0);
	SET_NFLG (((uae_s16)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2 + count_cycles;
}

/* ROLQ.W #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e158_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s16 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	CLEAR_CZNV ();
{	uae_u32 loval;
	cnt &= 15;
	loval = val >> (16 - cnt);
	val <<= cnt;
	val |= loval;
	val &= 0xffff;
	SET_CFLG (val & 1);
}
	SET_ZFLG (((uae_s16)(val)) == 0);
	SET_NFLG (((uae_s16)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2 + count_cycles;
}

/* ASL.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e160_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 cnt = m68k_dreg (regs, srcreg);
{	uae_s16 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	CLEAR_CZNV ();
	if (cnt >= 16) {
		SET_VFLG (val != 0);
		SET_CFLG (cnt == 16 ? val & 1 : 0);
		COPY_CARRY ();
		val = 0;
	} else if (cnt > 0) {
		uae_u32 mask = (0xffff << (15 - cnt)) & 0xffff;
		SET_VFLG ((val & mask) != mask && (val & mask) != 0);
		val <<= cnt - 1;
		SET_CFLG ((val & 0x8000) >> 15);
		COPY_CARRY ();
		val <<= 1;
		val &= 0xffff;
	}
	SET_ZFLG (((uae_s16)(val)) == 0);
	SET_NFLG (((uae_s16)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2 + count_cycles;
}

/* LSL.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e168_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 cnt = m68k_dreg (regs, srcreg);
{	uae_s16 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	CLEAR_CZNV ();
	if (cnt >= 16) {
		SET_CFLG (cnt == 16 ? val & 1 : 0);
		COPY_CARRY ();
		val = 0;
	} else if (cnt > 0) {
		val <<= (cnt - 1);
		SET_CFLG ((val & 0x8000) >> 15);
		COPY_CARRY ();
		val <<= 1;
	val &= 0xffff;
	}
	SET_ZFLG (((uae_s16)(val)) == 0);
	SET_NFLG (((uae_s16)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2 + count_cycles;
}

/* ROXL.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e170_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 cnt = m68k_dreg (regs, srcreg);
{	uae_s16 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	CLEAR_CZNV ();
	if (cnt >= 34) cnt -= 34;
	if (cnt >= 17) cnt -= 17;
	if (cnt > 0) {
	cnt--;
	{
	uae_u32 carry;
	uae_u32 loval = val >> (15 - cnt);
	carry = loval & 1;
	val = (((val << 1) | GET_XFLG ()) << cnt) | (loval >> 1);
	SET_XFLG (carry);
	val &= 0xffff;
	} }
	SET_CFLG (GET_XFLG ());
	SET_ZFLG (((uae_s16)(val)) == 0);
	SET_NFLG (((uae_s16)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2 + count_cycles;
}

/* ROL.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e178_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 cnt = m68k_dreg (regs, srcreg);
{	uae_s16 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	CLEAR_CZNV ();
	if (cnt > 0) {
	uae_u32 loval;
	cnt &= 15;
	loval = val >> (16 - cnt);
	val <<= cnt;
	val |= loval;
	val &= 0xffff;
	SET_CFLG (val & 1);
}
	SET_ZFLG (((uae_s16)(val)) == 0);
	SET_NFLG (((uae_s16)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2 + count_cycles;
}

/* ASLQ.L #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e180_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s32 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	CLEAR_CZNV ();
	if (cnt >= 32) {
		SET_VFLG (val != 0);
		SET_CFLG (cnt == 32 ? val & 1 : 0);
		COPY_CARRY ();
		val = 0;
	} else {
		uae_u32 mask = (0xffffffff << (31 - cnt)) & 0xffffffff;
		SET_VFLG ((val & mask) != mask && (val & mask) != 0);
		val <<= cnt - 1;
		SET_CFLG ((val & 0x80000000) >> 31);
		COPY_CARRY ();
		val <<= 1;
		val &= 0xffffffff;
	}
	SET_ZFLG (((uae_s32)(val)) == 0);
	SET_NFLG (((uae_s32)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2 + count_cycles;
}

/* LSLQ.L #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e188_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s32 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	CLEAR_CZNV ();
	if (cnt >= 32) {
		SET_CFLG (cnt == 32 ? val & 1 : 0);
		COPY_CARRY ();
		val = 0;
	} else {
		val <<= (cnt - 1);
		SET_CFLG ((val & 0x80000000) >> 31);
		COPY_CARRY ();
		val <<= 1;
	val &= 0xffffffff;
	}
	SET_ZFLG (((uae_s32)(val)) == 0);
	SET_NFLG (((uae_s32)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2 + count_cycles;
}

/* ROXLQ.L #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e190_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s32 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	CLEAR_CZNV ();
{	cnt--;
	{
	uae_u32 carry;
	uae_u32 loval = val >> (31 - cnt);
	carry = loval & 1;
	val = (((val << 1) | GET_XFLG ()) << cnt) | (loval >> 1);
	SET_XFLG (carry);
	val &= 0xffffffff;
	} }
	SET_CFLG (GET_XFLG ());
	SET_ZFLG (((uae_s32)(val)) == 0);
	SET_NFLG (((uae_s32)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2 + count_cycles;
}

/* ROLQ.L #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e198_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s32 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	CLEAR_CZNV ();
{	uae_u32 loval;
	cnt &= 31;
	loval = val >> (32 - cnt);
	val <<= cnt;
	val |= loval;
	val &= 0xffffffff;
	SET_CFLG (val & 1);
}
	SET_ZFLG (((uae_s32)(val)) == 0);
	SET_NFLG (((uae_s32)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2 + count_cycles;
}

/* ASL.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e1a0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 cnt = m68k_dreg (regs, srcreg);
{	uae_s32 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	CLEAR_CZNV ();
	if (cnt >= 32) {
		SET_VFLG (val != 0);
		SET_CFLG (cnt == 32 ? val & 1 : 0);
		COPY_CARRY ();
		val = 0;
	} else if (cnt > 0) {
		uae_u32 mask = (0xffffffff << (31 - cnt)) & 0xffffffff;
		SET_VFLG ((val & mask) != mask && (val & mask) != 0);
		val <<= cnt - 1;
		SET_CFLG ((val & 0x80000000) >> 31);
		COPY_CARRY ();
		val <<= 1;
		val &= 0xffffffff;
	}
	SET_ZFLG (((uae_s32)(val)) == 0);
	SET_NFLG (((uae_s32)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2 + count_cycles;
}

/* LSL.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e1a8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 cnt = m68k_dreg (regs, srcreg);
{	uae_s32 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	CLEAR_CZNV ();
	if (cnt >= 32) {
		SET_CFLG (cnt == 32 ? val & 1 : 0);
		COPY_CARRY ();
		val = 0;
	} else if (cnt > 0) {
		val <<= (cnt - 1);
		SET_CFLG ((val & 0x80000000) >> 31);
		COPY_CARRY ();
		val <<= 1;
	val &= 0xffffffff;
	}
	SET_ZFLG (((uae_s32)(val)) == 0);
	SET_NFLG (((uae_s32)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2 + count_cycles;
}

/* ROXL.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e1b0_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 cnt = m68k_dreg (regs, srcreg);
{	uae_s32 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	CLEAR_CZNV ();
	if (cnt >= 33) cnt -= 33;
	if (cnt > 0) {
	cnt--;
	{
	uae_u32 carry;
	uae_u32 loval = val >> (31 - cnt);
	carry = loval & 1;
	val = (((val << 1) | GET_XFLG ()) << cnt) | (loval >> 1);
	SET_XFLG (carry);
	val &= 0xffffffff;
	} }
	SET_CFLG (GET_XFLG ());
	SET_ZFLG (((uae_s32)(val)) == 0);
	SET_NFLG (((uae_s32)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2 + count_cycles;
}

/* ROL.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e1b8_4)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 cnt = m68k_dreg (regs, srcreg);
{	uae_s32 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	CLEAR_CZNV ();
	if (cnt > 0) {
	uae_u32 loval;
	cnt &= 31;
	loval = val >> (32 - cnt);
	val <<= cnt;
	val |= loval;
	val &= 0xffffffff;
	SET_CFLG (val & 1);
}
	SET_ZFLG (((uae_s32)(val)) == 0);
	SET_NFLG (((uae_s32)(val)) < 0);
	count_cycles += (2 * cnt) * CYCLE_UNIT / 2;
	m68k_dreg (regs, dstreg) = (val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2 + count_cycles;
}

/* ASLW.W (An) */
uae_u32 REGPARAM2 CPUFUNC(op_e1d0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg);
{	uae_s16 data = get_word (dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 sign2;
	val <<= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	sign2 = 0x8000 & val;
	SET_CFLG (sign != 0);
	COPY_CARRY ();
	SET_VFLG (GET_VFLG () | (sign2 != sign));
	put_word (dataa, val);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* ASLW.W (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_e1d8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg);
{	uae_s16 data = get_word (dataa);
	m68k_areg (regs, srcreg) += 2;
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 sign2;
	val <<= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	sign2 = 0x8000 & val;
	SET_CFLG (sign != 0);
	COPY_CARRY ();
	SET_VFLG (GET_VFLG () | (sign2 != sign));
	put_word (dataa, val);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* ASLW.W -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_e1e0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg) - 2;
{	uae_s16 data = get_word (dataa);
	m68k_areg (regs, srcreg) = dataa;
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 sign2;
	val <<= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	sign2 = 0x8000 & val;
	SET_CFLG (sign != 0);
	COPY_CARRY ();
	SET_VFLG (GET_VFLG () | (sign2 != sign));
	put_word (dataa, val);
}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* ASLW.W (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_e1e8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 data = get_word (dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 sign2;
	val <<= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	sign2 = 0x8000 & val;
	SET_CFLG (sign != 0);
	COPY_CARRY ();
	SET_VFLG (GET_VFLG () | (sign2 != sign));
	put_word (dataa, val);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* ASLW.W (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_e1f0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s16 data = get_word (dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 sign2;
	val <<= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	sign2 = 0x8000 & val;
	SET_CFLG (sign != 0);
	COPY_CARRY ();
	SET_VFLG (GET_VFLG () | (sign2 != sign));
	put_word (dataa, val);
}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* ASLW.W (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_e1f8_4)(uae_u32 opcode)
{
{{	uaecptr dataa;
	dataa = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 data = get_word (dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 sign2;
	val <<= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	sign2 = 0x8000 & val;
	SET_CFLG (sign != 0);
	COPY_CARRY ();
	SET_VFLG (GET_VFLG () | (sign2 != sign));
	put_word (dataa, val);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* ASLW.W (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_e1f9_4)(uae_u32 opcode)
{
{{	uaecptr dataa;
	dataa = get_dilong (2);
{	uae_s16 data = get_word (dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 sign2;
	val <<= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	sign2 = 0x8000 & val;
	SET_CFLG (sign != 0);
	COPY_CARRY ();
	SET_VFLG (GET_VFLG () | (sign2 != sign));
	put_word (dataa, val);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* LSRW.W (An) */
uae_u32 REGPARAM2 CPUFUNC(op_e2d0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg);
{	uae_s16 data = get_word (dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 carry = val & 1;
	val >>= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry);
	COPY_CARRY ();
	put_word (dataa, val);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* LSRW.W (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_e2d8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg);
{	uae_s16 data = get_word (dataa);
	m68k_areg (regs, srcreg) += 2;
{	uae_u32 val = (uae_u16)data;
	uae_u32 carry = val & 1;
	val >>= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry);
	COPY_CARRY ();
	put_word (dataa, val);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* LSRW.W -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_e2e0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg) - 2;
{	uae_s16 data = get_word (dataa);
	m68k_areg (regs, srcreg) = dataa;
{	uae_u32 val = (uae_u16)data;
	uae_u32 carry = val & 1;
	val >>= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry);
	COPY_CARRY ();
	put_word (dataa, val);
}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* LSRW.W (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_e2e8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 data = get_word (dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 carry = val & 1;
	val >>= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry);
	COPY_CARRY ();
	put_word (dataa, val);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* LSRW.W (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_e2f0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s16 data = get_word (dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 carry = val & 1;
	val >>= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry);
	COPY_CARRY ();
	put_word (dataa, val);
}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* LSRW.W (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_e2f8_4)(uae_u32 opcode)
{
{{	uaecptr dataa;
	dataa = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 data = get_word (dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 carry = val & 1;
	val >>= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry);
	COPY_CARRY ();
	put_word (dataa, val);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* LSRW.W (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_e2f9_4)(uae_u32 opcode)
{
{{	uaecptr dataa;
	dataa = get_dilong (2);
{	uae_s16 data = get_word (dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 carry = val & 1;
	val >>= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry);
	COPY_CARRY ();
	put_word (dataa, val);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* LSLW.W (An) */
uae_u32 REGPARAM2 CPUFUNC(op_e3d0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg);
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry >> 15);
	COPY_CARRY ();
	put_word (dataa, val);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* LSLW.W (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_e3d8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg);
{	uae_s16 data = get_word (dataa);
	m68k_areg (regs, srcreg) += 2;
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry >> 15);
	COPY_CARRY ();
	put_word (dataa, val);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* LSLW.W -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_e3e0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg) - 2;
{	uae_s16 data = get_word (dataa);
	m68k_areg (regs, srcreg) = dataa;
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry >> 15);
	COPY_CARRY ();
	put_word (dataa, val);
}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* LSLW.W (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_e3e8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry >> 15);
	COPY_CARRY ();
	put_word (dataa, val);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* LSLW.W (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_e3f0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry >> 15);
	COPY_CARRY ();
	put_word (dataa, val);
}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* LSLW.W (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_e3f8_4)(uae_u32 opcode)
{
{{	uaecptr dataa;
	dataa = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry >> 15);
	COPY_CARRY ();
	put_word (dataa, val);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* LSLW.W (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_e3f9_4)(uae_u32 opcode)
{
{{	uaecptr dataa;
	dataa = get_dilong (2);
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry >> 15);
	COPY_CARRY ();
	put_word (dataa, val);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* ROXRW.W (An) */
uae_u32 REGPARAM2 CPUFUNC(op_e4d0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg);
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (GET_XFLG ()) val |= 0x8000;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry);
	COPY_CARRY ();
	put_word (dataa, val);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* ROXRW.W (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_e4d8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg);
{	uae_s16 data = get_word (dataa);
	m68k_areg (regs, srcreg) += 2;
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (GET_XFLG ()) val |= 0x8000;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry);
	COPY_CARRY ();
	put_word (dataa, val);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* ROXRW.W -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_e4e0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg) - 2;
{	uae_s16 data = get_word (dataa);
	m68k_areg (regs, srcreg) = dataa;
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (GET_XFLG ()) val |= 0x8000;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry);
	COPY_CARRY ();
	put_word (dataa, val);
}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* ROXRW.W (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_e4e8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (GET_XFLG ()) val |= 0x8000;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry);
	COPY_CARRY ();
	put_word (dataa, val);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* ROXRW.W (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_e4f0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (GET_XFLG ()) val |= 0x8000;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry);
	COPY_CARRY ();
	put_word (dataa, val);
}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* ROXRW.W (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_e4f8_4)(uae_u32 opcode)
{
{{	uaecptr dataa;
	dataa = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (GET_XFLG ()) val |= 0x8000;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry);
	COPY_CARRY ();
	put_word (dataa, val);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* ROXRW.W (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_e4f9_4)(uae_u32 opcode)
{
{{	uaecptr dataa;
	dataa = get_dilong (2);
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (GET_XFLG ()) val |= 0x8000;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry);
	COPY_CARRY ();
	put_word (dataa, val);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* ROXLW.W (An) */
uae_u32 REGPARAM2 CPUFUNC(op_e5d0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg);
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (GET_XFLG ()) val |= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry >> 15);
	COPY_CARRY ();
	put_word (dataa, val);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* ROXLW.W (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_e5d8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg);
{	uae_s16 data = get_word (dataa);
	m68k_areg (regs, srcreg) += 2;
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (GET_XFLG ()) val |= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry >> 15);
	COPY_CARRY ();
	put_word (dataa, val);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* ROXLW.W -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_e5e0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg) - 2;
{	uae_s16 data = get_word (dataa);
	m68k_areg (regs, srcreg) = dataa;
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (GET_XFLG ()) val |= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry >> 15);
	COPY_CARRY ();
	put_word (dataa, val);
}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* ROXLW.W (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_e5e8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (GET_XFLG ()) val |= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry >> 15);
	COPY_CARRY ();
	put_word (dataa, val);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* ROXLW.W (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_e5f0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (GET_XFLG ()) val |= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry >> 15);
	COPY_CARRY ();
	put_word (dataa, val);
}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* ROXLW.W (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_e5f8_4)(uae_u32 opcode)
{
{{	uaecptr dataa;
	dataa = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (GET_XFLG ()) val |= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry >> 15);
	COPY_CARRY ();
	put_word (dataa, val);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* ROXLW.W (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_e5f9_4)(uae_u32 opcode)
{
{{	uaecptr dataa;
	dataa = get_dilong (2);
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (GET_XFLG ()) val |= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry >> 15);
	COPY_CARRY ();
	put_word (dataa, val);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* RORW.W (An) */
uae_u32 REGPARAM2 CPUFUNC(op_e6d0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg);
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (carry) val |= 0x8000;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry);
	put_word (dataa, val);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* RORW.W (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_e6d8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg);
{	uae_s16 data = get_word (dataa);
	m68k_areg (regs, srcreg) += 2;
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (carry) val |= 0x8000;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry);
	put_word (dataa, val);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* RORW.W -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_e6e0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg) - 2;
{	uae_s16 data = get_word (dataa);
	m68k_areg (regs, srcreg) = dataa;
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (carry) val |= 0x8000;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry);
	put_word (dataa, val);
}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* RORW.W (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_e6e8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (carry) val |= 0x8000;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry);
	put_word (dataa, val);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* RORW.W (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_e6f0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (carry) val |= 0x8000;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry);
	put_word (dataa, val);
}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* RORW.W (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_e6f8_4)(uae_u32 opcode)
{
{{	uaecptr dataa;
	dataa = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (carry) val |= 0x8000;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry);
	put_word (dataa, val);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* RORW.W (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_e6f9_4)(uae_u32 opcode)
{
{{	uaecptr dataa;
	dataa = get_dilong (2);
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (carry) val |= 0x8000;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry);
	put_word (dataa, val);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* ROLW.W (An) */
uae_u32 REGPARAM2 CPUFUNC(op_e7d0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg);
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (carry)  val |= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry >> 15);
	put_word (dataa, val);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* ROLW.W (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_e7d8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg);
{	uae_s16 data = get_word (dataa);
	m68k_areg (regs, srcreg) += 2;
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (carry)  val |= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry >> 15);
	put_word (dataa, val);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* ROLW.W -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_e7e0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg) - 2;
{	uae_s16 data = get_word (dataa);
	m68k_areg (regs, srcreg) = dataa;
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (carry)  val |= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry >> 15);
	put_word (dataa, val);
}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* ROLW.W (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_e7e8_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (carry)  val |= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry >> 15);
	put_word (dataa, val);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* ROLW.W (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_e7f0_4)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (carry)  val |= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry >> 15);
	put_word (dataa, val);
}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* ROLW.W (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_e7f8_4)(uae_u32 opcode)
{
{{	uaecptr dataa;
	dataa = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (carry)  val |= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry >> 15);
	put_word (dataa, val);
}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* ROLW.W (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_e7f9_4)(uae_u32 opcode)
{
{{	uaecptr dataa;
	dataa = get_dilong (2);
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (carry)  val |= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry >> 15);
	put_word (dataa, val);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

#endif


#if !defined(PART_1) && !defined(PART_2) && !defined(PART_3) && !defined(PART_4) && !defined(PART_5) && !defined(PART_6) && !defined(PART_7) && !defined(PART_8)
#define PART_1 1
#define PART_2 1
#define PART_3 1
#define PART_4 1
#define PART_5 1
#define PART_6 1
#define PART_7 1
#define PART_8 1
#endif

#ifdef PART_1
#endif

#ifdef PART_2
#endif

#ifdef PART_3
/* MVSR2.W Dn */
uae_u32 REGPARAM2 CPUFUNC(op_40c0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	MakeSR ();
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | ((regs.sr) & 0xffff);
}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* MVSR2.W (An) */
uae_u32 REGPARAM2 CPUFUNC(op_40d0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	put_word (srca, regs.sr | 0x0010);
	MakeSR ();
	put_word (srca, regs.sr);
}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* MVSR2.W (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_40d8_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += 2;
	put_word (srca, regs.sr | 0x0010);
	MakeSR ();
	put_word (srca, regs.sr);
}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* MVSR2.W -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_40e0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
	m68k_areg (regs, srcreg) = srca;
	put_word (srca, regs.sr | 0x0010);
	MakeSR ();
	put_word (srca, regs.sr);
}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* MVSR2.W (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_40e8_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	put_word (srca, regs.sr | 0x0010);
	MakeSR ();
	put_word (srca, regs.sr);
}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* MVSR2.W (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_40f0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
	put_word (srca, regs.sr | 0x0010);
	MakeSR ();
	put_word (srca, regs.sr);
}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* MVSR2.W (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_40f8_5)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	put_word (srca, regs.sr | 0x0010);
	MakeSR ();
	put_word (srca, regs.sr);
}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* MVSR2.W (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_40f9_5)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
	put_word (srca, regs.sr | 0x0010);
	MakeSR ();
	put_word (srca, regs.sr);
}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* CLR.B Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4200_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(0)) == 0);
	SET_NFLG   (((uae_s8)(0)) < 0);
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((0) & 0xff);
}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* CLR.B (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4210_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(0)) == 0);
	SET_NFLG   (((uae_s8)(0)) < 0);
	put_byte (srca, 0);
}}}	m68k_incpc (2);
return 16 * CYCLE_UNIT / 2;
}

/* CLR.B (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4218_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(0)) == 0);
	SET_NFLG   (((uae_s8)(0)) < 0);
	put_byte (srca, 0);
}}}	m68k_incpc (2);
return 16 * CYCLE_UNIT / 2;
}

/* CLR.B -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_4220_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(0)) == 0);
	SET_NFLG   (((uae_s8)(0)) < 0);
	put_byte (srca, 0);
}}}	m68k_incpc (2);
return 18 * CYCLE_UNIT / 2;
}

/* CLR.B (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4228_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(0)) == 0);
	SET_NFLG   (((uae_s8)(0)) < 0);
	put_byte (srca, 0);
}}}	m68k_incpc (4);
return 20 * CYCLE_UNIT / 2;
}

/* CLR.B (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4230_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s8 src = get_byte (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(0)) == 0);
	SET_NFLG   (((uae_s8)(0)) < 0);
	put_byte (srca, 0);
}}}	m68k_incpc (4);
return 22 * CYCLE_UNIT / 2;
}

/* CLR.B (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4238_5)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(0)) == 0);
	SET_NFLG   (((uae_s8)(0)) < 0);
	put_byte (srca, 0);
}}}	m68k_incpc (4);
return 20 * CYCLE_UNIT / 2;
}

/* CLR.B (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4239_5)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(0)) == 0);
	SET_NFLG   (((uae_s8)(0)) < 0);
	put_byte (srca, 0);
}}}	m68k_incpc (6);
return 24 * CYCLE_UNIT / 2;
}

/* CLR.W Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4240_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(0)) == 0);
	SET_NFLG   (((uae_s16)(0)) < 0);
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | ((0) & 0xffff);
}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* CLR.W (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4250_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(0)) == 0);
	SET_NFLG   (((uae_s16)(0)) < 0);
	put_word (srca, 0);
}}}	m68k_incpc (2);
return 16 * CYCLE_UNIT / 2;
}

/* CLR.W (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4258_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) += 2;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(0)) == 0);
	SET_NFLG   (((uae_s16)(0)) < 0);
	put_word (srca, 0);
}}}	m68k_incpc (2);
return 16 * CYCLE_UNIT / 2;
}

/* CLR.W -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_4260_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(0)) == 0);
	SET_NFLG   (((uae_s16)(0)) < 0);
	put_word (srca, 0);
}}}	m68k_incpc (2);
return 18 * CYCLE_UNIT / 2;
}

/* CLR.W (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4268_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(0)) == 0);
	SET_NFLG   (((uae_s16)(0)) < 0);
	put_word (srca, 0);
}}}	m68k_incpc (4);
return 20 * CYCLE_UNIT / 2;
}

/* CLR.W (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4270_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s16 src = get_word (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(0)) == 0);
	SET_NFLG   (((uae_s16)(0)) < 0);
	put_word (srca, 0);
}}}	m68k_incpc (4);
return 22 * CYCLE_UNIT / 2;
}

/* CLR.W (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4278_5)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(0)) == 0);
	SET_NFLG   (((uae_s16)(0)) < 0);
	put_word (srca, 0);
}}}	m68k_incpc (4);
return 20 * CYCLE_UNIT / 2;
}

/* CLR.W (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4279_5)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(0)) == 0);
	SET_NFLG   (((uae_s16)(0)) < 0);
	put_word (srca, 0);
}}}	m68k_incpc (6);
return 24 * CYCLE_UNIT / 2;
}

/* CLR.L Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4280_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = m68k_dreg (regs, srcreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(0)) == 0);
	SET_NFLG   (((uae_s32)(0)) < 0);
	m68k_dreg (regs, srcreg) = (0);
}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* CLR.L (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4290_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(0)) == 0);
	SET_NFLG   (((uae_s32)(0)) < 0);
	put_long (srca, 0);
}}}	m68k_incpc (2);
return 28 * CYCLE_UNIT / 2;
}

/* CLR.L (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4298_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) += 4;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(0)) == 0);
	SET_NFLG   (((uae_s32)(0)) < 0);
	put_long (srca, 0);
}}}	m68k_incpc (2);
return 28 * CYCLE_UNIT / 2;
}

/* CLR.L -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_42a0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(0)) == 0);
	SET_NFLG   (((uae_s32)(0)) < 0);
	put_long (srca, 0);
}}}	m68k_incpc (2);
return 30 * CYCLE_UNIT / 2;
}

/* CLR.L (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_42a8_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(0)) == 0);
	SET_NFLG   (((uae_s32)(0)) < 0);
	put_long (srca, 0);
}}}	m68k_incpc (4);
return 32 * CYCLE_UNIT / 2;
}

/* CLR.L (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_42b0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s32 src = get_long (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(0)) == 0);
	SET_NFLG   (((uae_s32)(0)) < 0);
	put_long (srca, 0);
}}}	m68k_incpc (4);
return 34 * CYCLE_UNIT / 2;
}

/* CLR.L (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_42b8_5)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(0)) == 0);
	SET_NFLG   (((uae_s32)(0)) < 0);
	put_long (srca, 0);
}}}	m68k_incpc (4);
return 32 * CYCLE_UNIT / 2;
}

/* CLR.L (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_42b9_5)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(0)) == 0);
	SET_NFLG   (((uae_s32)(0)) < 0);
	put_long (srca, 0);
}}}	m68k_incpc (6);
return 36 * CYCLE_UNIT / 2;
}

#endif

#ifdef PART_4
/* RTE.L  */
uae_u32 REGPARAM2 CPUFUNC(op_4e73_5)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uaecptr sra;
	sra = m68k_areg (regs, 7);
{	uae_s16 sr = get_word (sra);
	m68k_areg (regs, 7) += 2;
{	uaecptr pca;
	pca = m68k_areg (regs, 7);
{	uae_s32 pc = get_long (pca);
	m68k_areg (regs, 7) += 4;
	regs.sr = sr;
	if (pc & 1) {
		exception3i (0x4E73, pc);
		return 16 * CYCLE_UNIT / 2;
	}
	m68k_setpc (pc);
	MakeFromSR ();
}}}}}}return 20 * CYCLE_UNIT / 2;
}

#endif

#ifdef PART_5
/* Scc.B Dn (T) */
uae_u32 REGPARAM2 CPUFUNC(op_50c0_5)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{{	int val = cctrue (regs.ccrflags, 0) ? 0xff : 0;
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2 + count_cycles;
}

/* Scc.B (An) (T) */
uae_u32 REGPARAM2 CPUFUNC(op_50d0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 0) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (T) */
uae_u32 REGPARAM2 CPUFUNC(op_50d8_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (regs.ccrflags, 0) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (T) */
uae_u32 REGPARAM2 CPUFUNC(op_50e0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (regs.ccrflags, 0) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (T) */
uae_u32 REGPARAM2 CPUFUNC(op_50e8_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 0) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (T) */
uae_u32 REGPARAM2 CPUFUNC(op_50f0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 0) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (T) */
uae_u32 REGPARAM2 CPUFUNC(op_50f8_5)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 0) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (T) */
uae_u32 REGPARAM2 CPUFUNC(op_50f9_5)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 0) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (F) */
uae_u32 REGPARAM2 CPUFUNC(op_51c0_5)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{{	int val = cctrue (regs.ccrflags, 1) ? 0xff : 0;
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2 + count_cycles;
}

/* Scc.B (An) (F) */
uae_u32 REGPARAM2 CPUFUNC(op_51d0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 1) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (F) */
uae_u32 REGPARAM2 CPUFUNC(op_51d8_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (regs.ccrflags, 1) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (F) */
uae_u32 REGPARAM2 CPUFUNC(op_51e0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (regs.ccrflags, 1) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (F) */
uae_u32 REGPARAM2 CPUFUNC(op_51e8_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 1) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (F) */
uae_u32 REGPARAM2 CPUFUNC(op_51f0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 1) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (F) */
uae_u32 REGPARAM2 CPUFUNC(op_51f8_5)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 1) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (F) */
uae_u32 REGPARAM2 CPUFUNC(op_51f9_5)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 1) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (HI) */
uae_u32 REGPARAM2 CPUFUNC(op_52c0_5)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{{	int val = cctrue (regs.ccrflags, 2) ? 0xff : 0;
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2 + count_cycles;
}

/* Scc.B (An) (HI) */
uae_u32 REGPARAM2 CPUFUNC(op_52d0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 2) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (HI) */
uae_u32 REGPARAM2 CPUFUNC(op_52d8_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (regs.ccrflags, 2) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (HI) */
uae_u32 REGPARAM2 CPUFUNC(op_52e0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (regs.ccrflags, 2) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (HI) */
uae_u32 REGPARAM2 CPUFUNC(op_52e8_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 2) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (HI) */
uae_u32 REGPARAM2 CPUFUNC(op_52f0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 2) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (HI) */
uae_u32 REGPARAM2 CPUFUNC(op_52f8_5)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 2) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (HI) */
uae_u32 REGPARAM2 CPUFUNC(op_52f9_5)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 2) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (LS) */
uae_u32 REGPARAM2 CPUFUNC(op_53c0_5)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{{	int val = cctrue (regs.ccrflags, 3) ? 0xff : 0;
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2 + count_cycles;
}

/* Scc.B (An) (LS) */
uae_u32 REGPARAM2 CPUFUNC(op_53d0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 3) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (LS) */
uae_u32 REGPARAM2 CPUFUNC(op_53d8_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (regs.ccrflags, 3) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (LS) */
uae_u32 REGPARAM2 CPUFUNC(op_53e0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (regs.ccrflags, 3) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (LS) */
uae_u32 REGPARAM2 CPUFUNC(op_53e8_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 3) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (LS) */
uae_u32 REGPARAM2 CPUFUNC(op_53f0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 3) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (LS) */
uae_u32 REGPARAM2 CPUFUNC(op_53f8_5)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 3) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (LS) */
uae_u32 REGPARAM2 CPUFUNC(op_53f9_5)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 3) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (CC) */
uae_u32 REGPARAM2 CPUFUNC(op_54c0_5)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{{	int val = cctrue (regs.ccrflags, 4) ? 0xff : 0;
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2 + count_cycles;
}

/* Scc.B (An) (CC) */
uae_u32 REGPARAM2 CPUFUNC(op_54d0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 4) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (CC) */
uae_u32 REGPARAM2 CPUFUNC(op_54d8_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (regs.ccrflags, 4) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (CC) */
uae_u32 REGPARAM2 CPUFUNC(op_54e0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (regs.ccrflags, 4) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (CC) */
uae_u32 REGPARAM2 CPUFUNC(op_54e8_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 4) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (CC) */
uae_u32 REGPARAM2 CPUFUNC(op_54f0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 4) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (CC) */
uae_u32 REGPARAM2 CPUFUNC(op_54f8_5)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 4) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (CC) */
uae_u32 REGPARAM2 CPUFUNC(op_54f9_5)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 4) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (CS) */
uae_u32 REGPARAM2 CPUFUNC(op_55c0_5)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{{	int val = cctrue (regs.ccrflags, 5) ? 0xff : 0;
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2 + count_cycles;
}

/* Scc.B (An) (CS) */
uae_u32 REGPARAM2 CPUFUNC(op_55d0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 5) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (CS) */
uae_u32 REGPARAM2 CPUFUNC(op_55d8_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (regs.ccrflags, 5) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (CS) */
uae_u32 REGPARAM2 CPUFUNC(op_55e0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (regs.ccrflags, 5) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (CS) */
uae_u32 REGPARAM2 CPUFUNC(op_55e8_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 5) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (CS) */
uae_u32 REGPARAM2 CPUFUNC(op_55f0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 5) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (CS) */
uae_u32 REGPARAM2 CPUFUNC(op_55f8_5)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 5) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (CS) */
uae_u32 REGPARAM2 CPUFUNC(op_55f9_5)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 5) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (NE) */
uae_u32 REGPARAM2 CPUFUNC(op_56c0_5)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{{	int val = cctrue (regs.ccrflags, 6) ? 0xff : 0;
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2 + count_cycles;
}

/* Scc.B (An) (NE) */
uae_u32 REGPARAM2 CPUFUNC(op_56d0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 6) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (NE) */
uae_u32 REGPARAM2 CPUFUNC(op_56d8_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (regs.ccrflags, 6) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (NE) */
uae_u32 REGPARAM2 CPUFUNC(op_56e0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (regs.ccrflags, 6) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (NE) */
uae_u32 REGPARAM2 CPUFUNC(op_56e8_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 6) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (NE) */
uae_u32 REGPARAM2 CPUFUNC(op_56f0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 6) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (NE) */
uae_u32 REGPARAM2 CPUFUNC(op_56f8_5)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 6) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (NE) */
uae_u32 REGPARAM2 CPUFUNC(op_56f9_5)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 6) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (EQ) */
uae_u32 REGPARAM2 CPUFUNC(op_57c0_5)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{{	int val = cctrue (regs.ccrflags, 7) ? 0xff : 0;
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2 + count_cycles;
}

/* Scc.B (An) (EQ) */
uae_u32 REGPARAM2 CPUFUNC(op_57d0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 7) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (EQ) */
uae_u32 REGPARAM2 CPUFUNC(op_57d8_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (regs.ccrflags, 7) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (EQ) */
uae_u32 REGPARAM2 CPUFUNC(op_57e0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (regs.ccrflags, 7) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (EQ) */
uae_u32 REGPARAM2 CPUFUNC(op_57e8_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 7) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (EQ) */
uae_u32 REGPARAM2 CPUFUNC(op_57f0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 7) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (EQ) */
uae_u32 REGPARAM2 CPUFUNC(op_57f8_5)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 7) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (EQ) */
uae_u32 REGPARAM2 CPUFUNC(op_57f9_5)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 7) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (VC) */
uae_u32 REGPARAM2 CPUFUNC(op_58c0_5)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{{	int val = cctrue (regs.ccrflags, 8) ? 0xff : 0;
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2 + count_cycles;
}

/* Scc.B (An) (VC) */
uae_u32 REGPARAM2 CPUFUNC(op_58d0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 8) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (VC) */
uae_u32 REGPARAM2 CPUFUNC(op_58d8_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (regs.ccrflags, 8) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (VC) */
uae_u32 REGPARAM2 CPUFUNC(op_58e0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (regs.ccrflags, 8) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (VC) */
uae_u32 REGPARAM2 CPUFUNC(op_58e8_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 8) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (VC) */
uae_u32 REGPARAM2 CPUFUNC(op_58f0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 8) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (VC) */
uae_u32 REGPARAM2 CPUFUNC(op_58f8_5)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 8) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (VC) */
uae_u32 REGPARAM2 CPUFUNC(op_58f9_5)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 8) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (VS) */
uae_u32 REGPARAM2 CPUFUNC(op_59c0_5)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{{	int val = cctrue (regs.ccrflags, 9) ? 0xff : 0;
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2 + count_cycles;
}

/* Scc.B (An) (VS) */
uae_u32 REGPARAM2 CPUFUNC(op_59d0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 9) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (VS) */
uae_u32 REGPARAM2 CPUFUNC(op_59d8_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (regs.ccrflags, 9) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (VS) */
uae_u32 REGPARAM2 CPUFUNC(op_59e0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (regs.ccrflags, 9) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (VS) */
uae_u32 REGPARAM2 CPUFUNC(op_59e8_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 9) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (VS) */
uae_u32 REGPARAM2 CPUFUNC(op_59f0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 9) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (VS) */
uae_u32 REGPARAM2 CPUFUNC(op_59f8_5)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 9) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (VS) */
uae_u32 REGPARAM2 CPUFUNC(op_59f9_5)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 9) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (PL) */
uae_u32 REGPARAM2 CPUFUNC(op_5ac0_5)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{{	int val = cctrue (regs.ccrflags, 10) ? 0xff : 0;
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2 + count_cycles;
}

/* Scc.B (An) (PL) */
uae_u32 REGPARAM2 CPUFUNC(op_5ad0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 10) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (PL) */
uae_u32 REGPARAM2 CPUFUNC(op_5ad8_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (regs.ccrflags, 10) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (PL) */
uae_u32 REGPARAM2 CPUFUNC(op_5ae0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (regs.ccrflags, 10) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (PL) */
uae_u32 REGPARAM2 CPUFUNC(op_5ae8_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 10) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (PL) */
uae_u32 REGPARAM2 CPUFUNC(op_5af0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 10) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (PL) */
uae_u32 REGPARAM2 CPUFUNC(op_5af8_5)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 10) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (PL) */
uae_u32 REGPARAM2 CPUFUNC(op_5af9_5)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 10) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (MI) */
uae_u32 REGPARAM2 CPUFUNC(op_5bc0_5)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{{	int val = cctrue (regs.ccrflags, 11) ? 0xff : 0;
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2 + count_cycles;
}

/* Scc.B (An) (MI) */
uae_u32 REGPARAM2 CPUFUNC(op_5bd0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 11) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (MI) */
uae_u32 REGPARAM2 CPUFUNC(op_5bd8_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (regs.ccrflags, 11) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (MI) */
uae_u32 REGPARAM2 CPUFUNC(op_5be0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (regs.ccrflags, 11) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (MI) */
uae_u32 REGPARAM2 CPUFUNC(op_5be8_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 11) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (MI) */
uae_u32 REGPARAM2 CPUFUNC(op_5bf0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 11) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (MI) */
uae_u32 REGPARAM2 CPUFUNC(op_5bf8_5)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 11) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (MI) */
uae_u32 REGPARAM2 CPUFUNC(op_5bf9_5)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 11) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (GE) */
uae_u32 REGPARAM2 CPUFUNC(op_5cc0_5)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{{	int val = cctrue (regs.ccrflags, 12) ? 0xff : 0;
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2 + count_cycles;
}

/* Scc.B (An) (GE) */
uae_u32 REGPARAM2 CPUFUNC(op_5cd0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 12) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (GE) */
uae_u32 REGPARAM2 CPUFUNC(op_5cd8_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (regs.ccrflags, 12) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (GE) */
uae_u32 REGPARAM2 CPUFUNC(op_5ce0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (regs.ccrflags, 12) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (GE) */
uae_u32 REGPARAM2 CPUFUNC(op_5ce8_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 12) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (GE) */
uae_u32 REGPARAM2 CPUFUNC(op_5cf0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 12) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (GE) */
uae_u32 REGPARAM2 CPUFUNC(op_5cf8_5)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 12) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (GE) */
uae_u32 REGPARAM2 CPUFUNC(op_5cf9_5)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 12) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (LT) */
uae_u32 REGPARAM2 CPUFUNC(op_5dc0_5)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{{	int val = cctrue (regs.ccrflags, 13) ? 0xff : 0;
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2 + count_cycles;
}

/* Scc.B (An) (LT) */
uae_u32 REGPARAM2 CPUFUNC(op_5dd0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 13) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (LT) */
uae_u32 REGPARAM2 CPUFUNC(op_5dd8_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (regs.ccrflags, 13) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (LT) */
uae_u32 REGPARAM2 CPUFUNC(op_5de0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (regs.ccrflags, 13) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (LT) */
uae_u32 REGPARAM2 CPUFUNC(op_5de8_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 13) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (LT) */
uae_u32 REGPARAM2 CPUFUNC(op_5df0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 13) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (LT) */
uae_u32 REGPARAM2 CPUFUNC(op_5df8_5)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 13) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (LT) */
uae_u32 REGPARAM2 CPUFUNC(op_5df9_5)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 13) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (GT) */
uae_u32 REGPARAM2 CPUFUNC(op_5ec0_5)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{{	int val = cctrue (regs.ccrflags, 14) ? 0xff : 0;
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2 + count_cycles;
}

/* Scc.B (An) (GT) */
uae_u32 REGPARAM2 CPUFUNC(op_5ed0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 14) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (GT) */
uae_u32 REGPARAM2 CPUFUNC(op_5ed8_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (regs.ccrflags, 14) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (GT) */
uae_u32 REGPARAM2 CPUFUNC(op_5ee0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (regs.ccrflags, 14) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (GT) */
uae_u32 REGPARAM2 CPUFUNC(op_5ee8_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 14) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (GT) */
uae_u32 REGPARAM2 CPUFUNC(op_5ef0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 14) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (GT) */
uae_u32 REGPARAM2 CPUFUNC(op_5ef8_5)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 14) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (GT) */
uae_u32 REGPARAM2 CPUFUNC(op_5ef9_5)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 14) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (LE) */
uae_u32 REGPARAM2 CPUFUNC(op_5fc0_5)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{{	int val = cctrue (regs.ccrflags, 15) ? 0xff : 0;
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2 + count_cycles;
}

/* Scc.B (An) (LE) */
uae_u32 REGPARAM2 CPUFUNC(op_5fd0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 15) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (LE) */
uae_u32 REGPARAM2 CPUFUNC(op_5fd8_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (regs.ccrflags, 15) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (LE) */
uae_u32 REGPARAM2 CPUFUNC(op_5fe0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (regs.ccrflags, 15) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (LE) */
uae_u32 REGPARAM2 CPUFUNC(op_5fe8_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 15) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (LE) */
uae_u32 REGPARAM2 CPUFUNC(op_5ff0_5)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000 (m68k_areg (regs, srcreg), get_diword (2));
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 15) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (LE) */
uae_u32 REGPARAM2 CPUFUNC(op_5ff8_5)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 15) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (LE) */
uae_u32 REGPARAM2 CPUFUNC(op_5ff9_5)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte (srca);
{{	int val = cctrue (regs.ccrflags, 15) ? 0xff : 0;
	put_byte (srca, val);
}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

#endif

#ifdef PART_6
#endif

#ifdef PART_7
#endif

#ifdef PART_8
#endif

