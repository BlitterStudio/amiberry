#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "memory.h"
#include "custom.h"
#include "events.h"
#include "newcpu.h"
#include "cpu_prefetch.h"
#include "cputbl.h"
#define CPUFUNC(x) x##_ff
#define SET_CFLG_ALWAYS(flags, x) SET_CFLG(flags, x)
#define SET_NFLG_ALWAYS(flags, x) SET_NFLG(flags, x)
#ifdef NOFLAGS
#include "noflags.h"
#endif

/* OR.B #<data>.B,Dn */
unsigned long REGPARAM2 CPUFUNC(op_0000_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}	m68k_incpc (regs, 4);
return 8 * CYCLE_UNIT / 2;
}
/* OR.B #<data>.B,(An) */
unsigned long REGPARAM2 CPUFUNC(op_0010_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* OR.B #<data>.B,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_0018_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* OR.B #<data>.B,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_0020_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* OR.B #<data>.B,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_0028_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 4);
{	uae_s8 dst = get_byte (dsta);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* OR.B #<data>.B,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_0030_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 4));
{	uae_s8 dst = get_byte (dsta);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 6);
return 22 * CYCLE_UNIT / 2;
}
/* OR.B #<data>.B,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_0038_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 4);
{	uae_s8 dst = get_byte (dsta);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* OR.B #<data>.B,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_0039_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta = get_ilong (regs, 4);
{	uae_s8 dst = get_byte (dsta);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 8);
return 24 * CYCLE_UNIT / 2;
}
/* ORSR.B #<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_003c_4)(uae_u32 opcode, struct regstruct *regs)
{
{	MakeSR(regs);
{	uae_s16 src = get_iword (regs, 2);
	src &= 0xFF;
	regs->sr |= src;
	MakeFromSR(regs);
}}	m68k_incpc (regs, 4);
return 20 * CYCLE_UNIT / 2;
}
/* OR.W #<data>.W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_0040_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}	m68k_incpc (regs, 4);
return 8 * CYCLE_UNIT / 2;
}
/* OR.W #<data>.W,(An) */
unsigned long REGPARAM2 CPUFUNC(op_0050_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s16 dst = get_word (dsta);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* OR.W #<data>.W,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_0058_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s16 dst = get_word (dsta);
	m68k_areg(regs, dstreg) += 2;
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* OR.W #<data>.W,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_0060_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* OR.W #<data>.W,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_0068_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 4);
{	uae_s16 dst = get_word (dsta);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* OR.W #<data>.W,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_0070_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 4));
{	uae_s16 dst = get_word (dsta);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 6);
return 22 * CYCLE_UNIT / 2;
}
/* OR.W #<data>.W,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_0078_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 4);
{	uae_s16 dst = get_word (dsta);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* OR.W #<data>.W,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_0079_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = get_ilong (regs, 4);
{	uae_s16 dst = get_word (dsta);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 8);
return 24 * CYCLE_UNIT / 2;
}
/* ORSR.W #<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_007c_4)(uae_u32 opcode, struct regstruct *regs)
{
{if (!regs->s) { Exception(8, regs, 0); return 34 * CYCLE_UNIT / 2; }
{	MakeSR(regs);
{	uae_s16 src = get_iword (regs, 2);
	regs->sr |= src;
	MakeFromSR(regs);
}}}	m68k_incpc (regs, 4);
return 20 * CYCLE_UNIT / 2;
}
/* OR.L #<data>.L,Dn */
unsigned long REGPARAM2 CPUFUNC(op_0080_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
}}}	m68k_incpc (regs, 6);
return 16 * CYCLE_UNIT / 2;
}
/* OR.L #<data>.L,(An) */
unsigned long REGPARAM2 CPUFUNC(op_0090_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s32 dst = get_long (dsta);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 6);
return 28 * CYCLE_UNIT / 2;
}
/* OR.L #<data>.L,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_0098_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s32 dst = get_long (dsta);
	m68k_areg(regs, dstreg) += 4;
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 6);
return 28 * CYCLE_UNIT / 2;
}
/* OR.L #<data>.L,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_00a0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 6);
return 30 * CYCLE_UNIT / 2;
}
/* OR.L #<data>.L,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_00a8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 6);
{	uae_s32 dst = get_long (dsta);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 8);
return 32 * CYCLE_UNIT / 2;
}
/* OR.L #<data>.L,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_00b0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 6));
{	uae_s32 dst = get_long (dsta);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 8);
return 34 * CYCLE_UNIT / 2;
}
/* OR.L #<data>.L,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_00b8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 6);
{	uae_s32 dst = get_long (dsta);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 8);
return 32 * CYCLE_UNIT / 2;
}
/* OR.L #<data>.L,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_00b9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta = get_ilong (regs, 6);
{	uae_s32 dst = get_long (dsta);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 10);
return 36 * CYCLE_UNIT / 2;
}
/* BTST.L Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_0100_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= 31;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
}}}	m68k_incpc (regs, 2);
return 6 * CYCLE_UNIT / 2;
}
/* MVPMR.W (d16,An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_0108_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr memp = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_u16 val = (get_byte (memp) << 8) + get_byte (memp + 2);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* BTST.B Dn,(An) */
unsigned long REGPARAM2 CPUFUNC(op_0110_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* BTST.B Dn,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_0118_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* BTST.B Dn,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_0120_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (regs, 2);
return 10 * CYCLE_UNIT / 2;
}
/* BTST.B Dn,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_0128_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* BTST.B Dn,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_0130_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 2));
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (regs, 4);
return 14 * CYCLE_UNIT / 2;
}
/* BTST.B Dn,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_0138_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* BTST.B Dn,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_0139_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = get_ilong (regs, 2);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (regs, 6);
return 16 * CYCLE_UNIT / 2;
}
/* BTST.B Dn,(d16,PC) */
unsigned long REGPARAM2 CPUFUNC(op_013a_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = 2;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = m68k_getpc (regs) + 2;
	dsta += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* BTST.B Dn,(d8,PC,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_013b_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = 3;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr tmppc;
	uaecptr dsta;
	tmppc = m68k_getpc(regs) + 2;
	dsta = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (regs, 4);
return 14 * CYCLE_UNIT / 2;
}
/* BTST.B Dn,#<data>.B */
unsigned long REGPARAM2 CPUFUNC(op_013c_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uae_s8 dst = get_ibyte (regs, 2);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
}}}	m68k_incpc (regs, 4);
return 8 * CYCLE_UNIT / 2;
}
/* BCHG.L Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_0140_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= 31;
	dst ^= (1 << src);
	SET_ZFLG (&regs->ccrflags, ((uae_u32)dst & (1 << src)) >> src);
	m68k_dreg(regs, dstreg) = (dst);
}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* MVPMR.L (d16,An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_0148_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr memp = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_u32 val = (get_byte (memp) << 24) + (get_byte (memp + 2) << 16)
              + (get_byte (memp + 4) << 8) + get_byte (memp + 6);
	m68k_dreg(regs, dstreg) = (val);
}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* BCHG.B Dn,(An) */
unsigned long REGPARAM2 CPUFUNC(op_0150_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (&regs->ccrflags, ((uae_u32)dst & (1 << src)) >> src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* BCHG.B Dn,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_0158_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (&regs->ccrflags, ((uae_u32)dst & (1 << src)) >> src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* BCHG.B Dn,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_0160_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (&regs->ccrflags, ((uae_u32)dst & (1 << src)) >> src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* BCHG.B Dn,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_0168_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (&regs->ccrflags, ((uae_u32)dst & (1 << src)) >> src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* BCHG.B Dn,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_0170_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 2));
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (&regs->ccrflags, ((uae_u32)dst & (1 << src)) >> src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* BCHG.B Dn,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_0178_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (&regs->ccrflags, ((uae_u32)dst & (1 << src)) >> src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* BCHG.B Dn,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_0179_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = get_ilong (regs, 2);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (&regs->ccrflags, ((uae_u32)dst & (1 << src)) >> src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* BCHG.B Dn,(d16,PC) */
unsigned long REGPARAM2 CPUFUNC(op_017a_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = 2;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = m68k_getpc (regs) + 2;
	dsta += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (&regs->ccrflags, ((uae_u32)dst & (1 << src)) >> src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* BCHG.B Dn,(d8,PC,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_017b_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = 3;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr tmppc;
	uaecptr dsta;
	tmppc = m68k_getpc(regs) + 2;
	dsta = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (&regs->ccrflags, ((uae_u32)dst & (1 << src)) >> src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* BCLR.L Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_0180_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= 31;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	m68k_dreg(regs, dstreg) = (dst);
}}}	m68k_incpc (regs, 2);
return 10 * CYCLE_UNIT / 2;
}
/* MVPRM.W Dn,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_0188_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr memp = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
	put_byte (memp, src >> 8); put_byte (memp + 2, src);
}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* BCLR.B Dn,(An) */
unsigned long REGPARAM2 CPUFUNC(op_0190_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* BCLR.B Dn,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_0198_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* BCLR.B Dn,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_01a0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* BCLR.B Dn,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_01a8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* BCLR.B Dn,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_01b0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 2));
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* BCLR.B Dn,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_01b8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* BCLR.B Dn,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_01b9_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = get_ilong (regs, 2);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* BCLR.B Dn,(d16,PC) */
unsigned long REGPARAM2 CPUFUNC(op_01ba_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = 2;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = m68k_getpc (regs) + 2;
	dsta += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* BCLR.B Dn,(d8,PC,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_01bb_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = 3;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr tmppc;
	uaecptr dsta;
	tmppc = m68k_getpc(regs) + 2;
	dsta = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* BSET.L Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_01c0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= 31;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	m68k_dreg(regs, dstreg) = (dst);
}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* MVPRM.L Dn,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_01c8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr memp = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
	put_byte (memp, src >> 24); put_byte (memp + 2, src >> 16);
	put_byte (memp + 4, src >> 8); put_byte (memp + 6, src);
}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* BSET.B Dn,(An) */
unsigned long REGPARAM2 CPUFUNC(op_01d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* BSET.B Dn,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_01d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* BSET.B Dn,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_01e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* BSET.B Dn,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_01e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* BSET.B Dn,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_01f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 2));
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* BSET.B Dn,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_01f8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* BSET.B Dn,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_01f9_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = get_ilong (regs, 2);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* BSET.B Dn,(d16,PC) */
unsigned long REGPARAM2 CPUFUNC(op_01fa_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = 2;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = m68k_getpc (regs) + 2;
	dsta += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* BSET.B Dn,(d8,PC,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_01fb_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = 3;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr tmppc;
	uaecptr dsta;
	tmppc = m68k_getpc(regs) + 2;
	dsta = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* AND.B #<data>.B,Dn */
unsigned long REGPARAM2 CPUFUNC(op_0200_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}	m68k_incpc (regs, 4);
return 8 * CYCLE_UNIT / 2;
}
/* AND.B #<data>.B,(An) */
unsigned long REGPARAM2 CPUFUNC(op_0210_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* AND.B #<data>.B,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_0218_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* AND.B #<data>.B,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_0220_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* AND.B #<data>.B,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_0228_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 4);
{	uae_s8 dst = get_byte (dsta);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* AND.B #<data>.B,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_0230_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 4));
{	uae_s8 dst = get_byte (dsta);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 6);
return 22 * CYCLE_UNIT / 2;
}
/* AND.B #<data>.B,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_0238_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 4);
{	uae_s8 dst = get_byte (dsta);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* AND.B #<data>.B,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_0239_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta = get_ilong (regs, 4);
{	uae_s8 dst = get_byte (dsta);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 8);
return 24 * CYCLE_UNIT / 2;
}
/* ANDSR.B #<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_023c_4)(uae_u32 opcode, struct regstruct *regs)
{
{	MakeSR(regs);
{	uae_s16 src = get_iword (regs, 2);
	src |= 0xFF00;
	regs->sr &= src;
	MakeFromSR(regs);
}}	m68k_incpc (regs, 4);
return 20 * CYCLE_UNIT / 2;
}
/* AND.W #<data>.W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_0240_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}	m68k_incpc (regs, 4);
return 8 * CYCLE_UNIT / 2;
}
/* AND.W #<data>.W,(An) */
unsigned long REGPARAM2 CPUFUNC(op_0250_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s16 dst = get_word (dsta);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* AND.W #<data>.W,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_0258_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s16 dst = get_word (dsta);
	m68k_areg(regs, dstreg) += 2;
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* AND.W #<data>.W,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_0260_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* AND.W #<data>.W,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_0268_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 4);
{	uae_s16 dst = get_word (dsta);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* AND.W #<data>.W,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_0270_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 4));
{	uae_s16 dst = get_word (dsta);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 6);
return 22 * CYCLE_UNIT / 2;
}
/* AND.W #<data>.W,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_0278_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 4);
{	uae_s16 dst = get_word (dsta);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* AND.W #<data>.W,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_0279_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = get_ilong (regs, 4);
{	uae_s16 dst = get_word (dsta);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 8);
return 24 * CYCLE_UNIT / 2;
}
/* ANDSR.W #<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_027c_4)(uae_u32 opcode, struct regstruct *regs)
{
{if (!regs->s) { Exception(8, regs, 0); return 34 * CYCLE_UNIT / 2; }
{	MakeSR(regs);
{	uae_s16 src = get_iword (regs, 2);
	regs->sr &= src;
	MakeFromSR(regs);
}}}	m68k_incpc (regs, 4);
return 20 * CYCLE_UNIT / 2;
}
/* AND.L #<data>.L,Dn */
unsigned long REGPARAM2 CPUFUNC(op_0280_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
}}}	m68k_incpc (regs, 6);
return 14 * CYCLE_UNIT / 2;
}
/* AND.L #<data>.L,(An) */
unsigned long REGPARAM2 CPUFUNC(op_0290_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s32 dst = get_long (dsta);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 6);
return 28 * CYCLE_UNIT / 2;
}
/* AND.L #<data>.L,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_0298_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s32 dst = get_long (dsta);
	m68k_areg(regs, dstreg) += 4;
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 6);
return 28 * CYCLE_UNIT / 2;
}
/* AND.L #<data>.L,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_02a0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 6);
return 30 * CYCLE_UNIT / 2;
}
/* AND.L #<data>.L,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_02a8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 6);
{	uae_s32 dst = get_long (dsta);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 8);
return 32 * CYCLE_UNIT / 2;
}
/* AND.L #<data>.L,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_02b0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 6));
{	uae_s32 dst = get_long (dsta);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 8);
return 34 * CYCLE_UNIT / 2;
}
/* AND.L #<data>.L,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_02b8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 6);
{	uae_s32 dst = get_long (dsta);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 8);
return 32 * CYCLE_UNIT / 2;
}
/* AND.L #<data>.L,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_02b9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta = get_ilong (regs, 6);
{	uae_s32 dst = get_long (dsta);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 10);
return 36 * CYCLE_UNIT / 2;
}
/* SUB.B #<data>.B,Dn */
unsigned long REGPARAM2 CPUFUNC(op_0400_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}	m68k_incpc (regs, 4);
return 8 * CYCLE_UNIT / 2;
}
/* SUB.B #<data>.B,(An) */
unsigned long REGPARAM2 CPUFUNC(op_0410_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* SUB.B #<data>.B,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_0418_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* SUB.B #<data>.B,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_0420_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* SUB.B #<data>.B,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_0428_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 4);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* SUB.B #<data>.B,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_0430_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 4));
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 6);
return 22 * CYCLE_UNIT / 2;
}
/* SUB.B #<data>.B,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_0438_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 4);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* SUB.B #<data>.B,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_0439_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta = get_ilong (regs, 4);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 8);
return 24 * CYCLE_UNIT / 2;
}
/* SUB.W #<data>.W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_0440_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}	m68k_incpc (regs, 4);
return 8 * CYCLE_UNIT / 2;
}
/* SUB.W #<data>.W,(An) */
unsigned long REGPARAM2 CPUFUNC(op_0450_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* SUB.W #<data>.W,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_0458_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s16 dst = get_word (dsta);
	m68k_areg(regs, dstreg) += 2;
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* SUB.W #<data>.W,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_0460_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* SUB.W #<data>.W,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_0468_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 4);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* SUB.W #<data>.W,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_0470_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 4));
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 6);
return 22 * CYCLE_UNIT / 2;
}
/* SUB.W #<data>.W,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_0478_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 4);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* SUB.W #<data>.W,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_0479_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = get_ilong (regs, 4);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 8);
return 24 * CYCLE_UNIT / 2;
}
/* SUB.L #<data>.L,Dn */
unsigned long REGPARAM2 CPUFUNC(op_0480_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
}}}}}}	m68k_incpc (regs, 6);
return 16 * CYCLE_UNIT / 2;
}
/* SUB.L #<data>.L,(An) */
unsigned long REGPARAM2 CPUFUNC(op_0490_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 6);
return 28 * CYCLE_UNIT / 2;
}
/* SUB.L #<data>.L,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_0498_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s32 dst = get_long (dsta);
	m68k_areg(regs, dstreg) += 4;
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 6);
return 28 * CYCLE_UNIT / 2;
}
/* SUB.L #<data>.L,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_04a0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 6);
return 30 * CYCLE_UNIT / 2;
}
/* SUB.L #<data>.L,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_04a8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 6);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 8);
return 32 * CYCLE_UNIT / 2;
}
/* SUB.L #<data>.L,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_04b0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 6));
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 8);
return 34 * CYCLE_UNIT / 2;
}
/* SUB.L #<data>.L,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_04b8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 6);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 8);
return 32 * CYCLE_UNIT / 2;
}
/* SUB.L #<data>.L,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_04b9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta = get_ilong (regs, 6);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 10);
return 36 * CYCLE_UNIT / 2;
}
/* ADD.B #<data>.B,Dn */
unsigned long REGPARAM2 CPUFUNC(op_0600_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}	m68k_incpc (regs, 4);
return 8 * CYCLE_UNIT / 2;
}
/* ADD.B #<data>.B,(An) */
unsigned long REGPARAM2 CPUFUNC(op_0610_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* ADD.B #<data>.B,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_0618_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* ADD.B #<data>.B,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_0620_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* ADD.B #<data>.B,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_0628_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 4);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* ADD.B #<data>.B,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_0630_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 4));
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 6);
return 22 * CYCLE_UNIT / 2;
}
/* ADD.B #<data>.B,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_0638_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 4);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* ADD.B #<data>.B,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_0639_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta = get_ilong (regs, 4);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 8);
return 24 * CYCLE_UNIT / 2;
}
/* ADD.W #<data>.W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_0640_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}	m68k_incpc (regs, 4);
return 8 * CYCLE_UNIT / 2;
}
/* ADD.W #<data>.W,(An) */
unsigned long REGPARAM2 CPUFUNC(op_0650_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* ADD.W #<data>.W,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_0658_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s16 dst = get_word (dsta);
	m68k_areg(regs, dstreg) += 2;
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* ADD.W #<data>.W,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_0660_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* ADD.W #<data>.W,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_0668_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 4);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* ADD.W #<data>.W,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_0670_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 4));
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 6);
return 22 * CYCLE_UNIT / 2;
}
/* ADD.W #<data>.W,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_0678_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 4);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* ADD.W #<data>.W,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_0679_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = get_ilong (regs, 4);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 8);
return 24 * CYCLE_UNIT / 2;
}
/* ADD.L #<data>.L,Dn */
unsigned long REGPARAM2 CPUFUNC(op_0680_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
}}}}}}	m68k_incpc (regs, 6);
return 16 * CYCLE_UNIT / 2;
}
/* ADD.L #<data>.L,(An) */
unsigned long REGPARAM2 CPUFUNC(op_0690_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 6);
return 28 * CYCLE_UNIT / 2;
}
/* ADD.L #<data>.L,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_0698_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s32 dst = get_long (dsta);
	m68k_areg(regs, dstreg) += 4;
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 6);
return 28 * CYCLE_UNIT / 2;
}
/* ADD.L #<data>.L,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_06a0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 6);
return 30 * CYCLE_UNIT / 2;
}
/* ADD.L #<data>.L,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_06a8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 6);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 8);
return 32 * CYCLE_UNIT / 2;
}
/* ADD.L #<data>.L,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_06b0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 6));
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 8);
return 34 * CYCLE_UNIT / 2;
}
/* ADD.L #<data>.L,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_06b8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 6);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 8);
return 32 * CYCLE_UNIT / 2;
}
/* ADD.L #<data>.L,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_06b9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta = get_ilong (regs, 6);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 10);
return 36 * CYCLE_UNIT / 2;
}
/* BTST.L #<data>.W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_0800_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= 31;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
}}}	m68k_incpc (regs, 4);
return 10 * CYCLE_UNIT / 2;
}
/* BTST.B #<data>.W,(An) */
unsigned long REGPARAM2 CPUFUNC(op_0810_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* BTST.B #<data>.W,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_0818_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* BTST.B #<data>.W,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_0820_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (regs, 4);
return 14 * CYCLE_UNIT / 2;
}
/* BTST.B #<data>.W,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_0828_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 4);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (regs, 6);
return 16 * CYCLE_UNIT / 2;
}
/* BTST.B #<data>.W,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_0830_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 4));
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (regs, 6);
return 18 * CYCLE_UNIT / 2;
}
/* BTST.B #<data>.W,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_0838_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 4);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (regs, 6);
return 16 * CYCLE_UNIT / 2;
}
/* BTST.B #<data>.W,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_0839_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = get_ilong (regs, 4);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (regs, 8);
return 20 * CYCLE_UNIT / 2;
}
/* BTST.B #<data>.W,(d16,PC) */
unsigned long REGPARAM2 CPUFUNC(op_083a_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = 2;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = m68k_getpc (regs) + 4;
	dsta += (uae_s32)(uae_s16)get_iword (regs, 4);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (regs, 6);
return 16 * CYCLE_UNIT / 2;
}
/* BTST.B #<data>.W,(d8,PC,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_083b_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = 3;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr tmppc;
	uaecptr dsta;
	tmppc = m68k_getpc(regs) + 4;
	dsta = get_disp_ea_000(regs, tmppc, get_iword (regs, 4));
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (regs, 6);
return 18 * CYCLE_UNIT / 2;
}
/* BTST.B #<data>.W,#<data>.B */
unsigned long REGPARAM2 CPUFUNC(op_083c_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s16 src = get_iword (regs, 2);
{	uae_s8 dst = get_ibyte (regs, 4);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
}}}	m68k_incpc (regs, 6);
return 12 * CYCLE_UNIT / 2;
}
/* BCHG.L #<data>.W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_0840_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= 31;
	dst ^= (1 << src);
	SET_ZFLG (&regs->ccrflags, ((uae_u32)dst & (1 << src)) >> src);
	m68k_dreg(regs, dstreg) = (dst);
}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* BCHG.B #<data>.W,(An) */
unsigned long REGPARAM2 CPUFUNC(op_0850_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (&regs->ccrflags, ((uae_u32)dst & (1 << src)) >> src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* BCHG.B #<data>.W,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_0858_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (&regs->ccrflags, ((uae_u32)dst & (1 << src)) >> src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* BCHG.B #<data>.W,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_0860_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (&regs->ccrflags, ((uae_u32)dst & (1 << src)) >> src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* BCHG.B #<data>.W,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_0868_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 4);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (&regs->ccrflags, ((uae_u32)dst & (1 << src)) >> src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* BCHG.B #<data>.W,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_0870_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 4));
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (&regs->ccrflags, ((uae_u32)dst & (1 << src)) >> src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 6);
return 22 * CYCLE_UNIT / 2;
}
/* BCHG.B #<data>.W,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_0878_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 4);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (&regs->ccrflags, ((uae_u32)dst & (1 << src)) >> src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* BCHG.B #<data>.W,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_0879_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = get_ilong (regs, 4);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (&regs->ccrflags, ((uae_u32)dst & (1 << src)) >> src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 8);
return 24 * CYCLE_UNIT / 2;
}
/* BCHG.B #<data>.W,(d16,PC) */
unsigned long REGPARAM2 CPUFUNC(op_087a_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = 2;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = m68k_getpc (regs) + 4;
	dsta += (uae_s32)(uae_s16)get_iword (regs, 4);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (&regs->ccrflags, ((uae_u32)dst & (1 << src)) >> src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* BCHG.B #<data>.W,(d8,PC,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_087b_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = 3;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr tmppc;
	uaecptr dsta;
	tmppc = m68k_getpc(regs) + 4;
	dsta = get_disp_ea_000(regs, tmppc, get_iword (regs, 4));
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (&regs->ccrflags, ((uae_u32)dst & (1 << src)) >> src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 6);
return 22 * CYCLE_UNIT / 2;
}
/* BCLR.L #<data>.W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_0880_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= 31;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	m68k_dreg(regs, dstreg) = (dst);
}}}	m68k_incpc (regs, 4);
return 14 * CYCLE_UNIT / 2;
}
/* BCLR.B #<data>.W,(An) */
unsigned long REGPARAM2 CPUFUNC(op_0890_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* BCLR.B #<data>.W,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_0898_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* BCLR.B #<data>.W,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_08a0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* BCLR.B #<data>.W,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_08a8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 4);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* BCLR.B #<data>.W,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_08b0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 4));
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 6);
return 22 * CYCLE_UNIT / 2;
}
/* BCLR.B #<data>.W,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_08b8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 4);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* BCLR.B #<data>.W,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_08b9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = get_ilong (regs, 4);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 8);
return 24 * CYCLE_UNIT / 2;
}
/* BCLR.B #<data>.W,(d16,PC) */
unsigned long REGPARAM2 CPUFUNC(op_08ba_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = 2;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = m68k_getpc (regs) + 4;
	dsta += (uae_s32)(uae_s16)get_iword (regs, 4);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* BCLR.B #<data>.W,(d8,PC,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_08bb_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = 3;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr tmppc;
	uaecptr dsta;
	tmppc = m68k_getpc(regs) + 4;
	dsta = get_disp_ea_000(regs, tmppc, get_iword (regs, 4));
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 6);
return 22 * CYCLE_UNIT / 2;
}
/* BSET.L #<data>.W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_08c0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= 31;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	m68k_dreg(regs, dstreg) = (dst);
}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* BSET.B #<data>.W,(An) */
unsigned long REGPARAM2 CPUFUNC(op_08d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* BSET.B #<data>.W,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_08d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* BSET.B #<data>.W,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_08e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* BSET.B #<data>.W,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_08e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 4);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* BSET.B #<data>.W,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_08f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 4));
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 6);
return 22 * CYCLE_UNIT / 2;
}
/* BSET.B #<data>.W,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_08f8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 4);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* BSET.B #<data>.W,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_08f9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = get_ilong (regs, 4);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 8);
return 24 * CYCLE_UNIT / 2;
}
/* BSET.B #<data>.W,(d16,PC) */
unsigned long REGPARAM2 CPUFUNC(op_08fa_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = 2;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = m68k_getpc (regs) + 4;
	dsta += (uae_s32)(uae_s16)get_iword (regs, 4);
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* BSET.B #<data>.W,(d8,PC,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_08fb_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = 3;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr tmppc;
	uaecptr dsta;
	tmppc = m68k_getpc(regs) + 4;
	dsta = get_disp_ea_000(regs, tmppc, get_iword (regs, 4));
{	uae_s8 dst = get_byte (dsta);
	src &= 7;
	SET_ZFLG (&regs->ccrflags, 1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte (dsta,dst);
}}}}	m68k_incpc (regs, 6);
return 22 * CYCLE_UNIT / 2;
}
/* EOR.B #<data>.B,Dn */
unsigned long REGPARAM2 CPUFUNC(op_0a00_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}	m68k_incpc (regs, 4);
return 8 * CYCLE_UNIT / 2;
}
/* EOR.B #<data>.B,(An) */
unsigned long REGPARAM2 CPUFUNC(op_0a10_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* EOR.B #<data>.B,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_0a18_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* EOR.B #<data>.B,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_0a20_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* EOR.B #<data>.B,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_0a28_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 4);
{	uae_s8 dst = get_byte (dsta);
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* EOR.B #<data>.B,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_0a30_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 4));
{	uae_s8 dst = get_byte (dsta);
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 6);
return 22 * CYCLE_UNIT / 2;
}
/* EOR.B #<data>.B,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_0a38_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 4);
{	uae_s8 dst = get_byte (dsta);
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* EOR.B #<data>.B,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_0a39_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta = get_ilong (regs, 4);
{	uae_s8 dst = get_byte (dsta);
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 8);
return 24 * CYCLE_UNIT / 2;
}
/* EORSR.B #<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_0a3c_4)(uae_u32 opcode, struct regstruct *regs)
{
{	MakeSR(regs);
{	uae_s16 src = get_iword (regs, 2);
	src &= 0xFF;
	regs->sr ^= src;
	MakeFromSR(regs);
}}	m68k_incpc (regs, 4);
return 20 * CYCLE_UNIT / 2;
}
/* EOR.W #<data>.W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_0a40_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}	m68k_incpc (regs, 4);
return 8 * CYCLE_UNIT / 2;
}
/* EOR.W #<data>.W,(An) */
unsigned long REGPARAM2 CPUFUNC(op_0a50_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s16 dst = get_word (dsta);
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* EOR.W #<data>.W,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_0a58_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s16 dst = get_word (dsta);
	m68k_areg(regs, dstreg) += 2;
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* EOR.W #<data>.W,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_0a60_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* EOR.W #<data>.W,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_0a68_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 4);
{	uae_s16 dst = get_word (dsta);
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* EOR.W #<data>.W,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_0a70_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 4));
{	uae_s16 dst = get_word (dsta);
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 6);
return 22 * CYCLE_UNIT / 2;
}
/* EOR.W #<data>.W,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_0a78_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 4);
{	uae_s16 dst = get_word (dsta);
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* EOR.W #<data>.W,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_0a79_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = get_ilong (regs, 4);
{	uae_s16 dst = get_word (dsta);
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 8);
return 24 * CYCLE_UNIT / 2;
}
/* EORSR.W #<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_0a7c_4)(uae_u32 opcode, struct regstruct *regs)
{
{if (!regs->s) { Exception(8, regs, 0);return 34 * CYCLE_UNIT / 2; }
{	MakeSR(regs);
{	uae_s16 src = get_iword (regs, 2);
	regs->sr ^= src;
	MakeFromSR(regs);
}}}	m68k_incpc (regs, 4);
return 20 * CYCLE_UNIT / 2;
}
/* EOR.L #<data>.L,Dn */
unsigned long REGPARAM2 CPUFUNC(op_0a80_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
}}}	m68k_incpc (regs, 6);
return 16 * CYCLE_UNIT / 2;
}
/* EOR.L #<data>.L,(An) */
unsigned long REGPARAM2 CPUFUNC(op_0a90_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s32 dst = get_long (dsta);
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 6);
return 28 * CYCLE_UNIT / 2;
}
/* EOR.L #<data>.L,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_0a98_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s32 dst = get_long (dsta);
	m68k_areg(regs, dstreg) += 4;
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 6);
return 28 * CYCLE_UNIT / 2;
}
/* EOR.L #<data>.L,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_0aa0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 6);
return 30 * CYCLE_UNIT / 2;
}
/* EOR.L #<data>.L,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_0aa8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 6);
{	uae_s32 dst = get_long (dsta);
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 8);
return 32 * CYCLE_UNIT / 2;
}
/* EOR.L #<data>.L,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_0ab0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 6));
{	uae_s32 dst = get_long (dsta);
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 8);
return 34 * CYCLE_UNIT / 2;
}
/* EOR.L #<data>.L,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_0ab8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 6);
{	uae_s32 dst = get_long (dsta);
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 8);
return 32 * CYCLE_UNIT / 2;
}
/* EOR.L #<data>.L,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_0ab9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta = get_ilong (regs, 6);
{	uae_s32 dst = get_long (dsta);
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 10);
return 36 * CYCLE_UNIT / 2;
}
/* CMP.B #<data>.B,Dn */
unsigned long REGPARAM2 CPUFUNC(op_0c00_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}	m68k_incpc (regs, 4);
return 8 * CYCLE_UNIT / 2;
}
/* CMP.B #<data>.B,(An) */
unsigned long REGPARAM2 CPUFUNC(op_0c10_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* CMP.B #<data>.B,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_0c18_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* CMP.B #<data>.B,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_0c20_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 4);
return 14 * CYCLE_UNIT / 2;
}
/* CMP.B #<data>.B,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_0c28_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 4);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 6);
return 16 * CYCLE_UNIT / 2;
}
/* CMP.B #<data>.B,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_0c30_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 4));
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 6);
return 18 * CYCLE_UNIT / 2;
}
/* CMP.B #<data>.B,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_0c38_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 4);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 6);
return 16 * CYCLE_UNIT / 2;
}
/* CMP.B #<data>.B,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_0c39_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta = get_ilong (regs, 4);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 8);
return 20 * CYCLE_UNIT / 2;
}
/* CMP.W #<data>.W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_0c40_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}	m68k_incpc (regs, 4);
return 8 * CYCLE_UNIT / 2;
}
/* CMP.W #<data>.W,(An) */
unsigned long REGPARAM2 CPUFUNC(op_0c50_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* CMP.W #<data>.W,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_0c58_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s16 dst = get_word (dsta);
	m68k_areg(regs, dstreg) += 2;
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* CMP.W #<data>.W,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_0c60_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 4);
return 14 * CYCLE_UNIT / 2;
}
/* CMP.W #<data>.W,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_0c68_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 4);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 6);
return 16 * CYCLE_UNIT / 2;
}
/* CMP.W #<data>.W,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_0c70_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 4));
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 6);
return 18 * CYCLE_UNIT / 2;
}
/* CMP.W #<data>.W,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_0c78_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 4);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 6);
return 16 * CYCLE_UNIT / 2;
}
/* CMP.W #<data>.W,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_0c79_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = get_ilong (regs, 4);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 8);
return 20 * CYCLE_UNIT / 2;
}
/* CMP.L #<data>.L,Dn */
unsigned long REGPARAM2 CPUFUNC(op_0c80_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}	m68k_incpc (regs, 6);
return 14 * CYCLE_UNIT / 2;
}
/* CMP.L #<data>.L,(An) */
unsigned long REGPARAM2 CPUFUNC(op_0c90_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* CMP.L #<data>.L,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_0c98_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s32 dst = get_long (dsta);
	m68k_areg(regs, dstreg) += 4;
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* CMP.L #<data>.L,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_0ca0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 6);
return 22 * CYCLE_UNIT / 2;
}
/* CMP.L #<data>.L,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_0ca8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 6);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 8);
return 24 * CYCLE_UNIT / 2;
}
/* CMP.L #<data>.L,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_0cb0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 6));
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 8);
return 26 * CYCLE_UNIT / 2;
}
/* CMP.L #<data>.L,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_0cb8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 6);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 8);
return 24 * CYCLE_UNIT / 2;
}
/* CMP.L #<data>.L,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_0cb9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta = get_ilong (regs, 6);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 10);
return 28 * CYCLE_UNIT / 2;
}
/* MOVE.B Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_1000_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 2);
}}}return 4 * CYCLE_UNIT / 2;
}
/* MOVE.B (An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_1010_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s8 src = get_byte (srca);
{	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 2);
}}}}return 8 * CYCLE_UNIT / 2;
}
/* MOVE.B (An)+,Dn */
unsigned long REGPARAM2 CPUFUNC(op_1018_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
{	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 2);
}}}}return 8 * CYCLE_UNIT / 2;
}
/* MOVE.B -(An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_1020_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 2);
}}}}return 10 * CYCLE_UNIT / 2;
}
/* MOVE.B (d16,An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_1028_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVE.B (d8,An,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_1030_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s8 src = get_byte (srca);
{	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 14 * CYCLE_UNIT / 2;
}
/* MOVE.B (xxx).W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_1038_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVE.B (xxx).L,Dn */
unsigned long REGPARAM2 CPUFUNC(op_1039_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s8 src = get_byte (srca);
{	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.B (d16,PC),Dn */
unsigned long REGPARAM2 CPUFUNC(op_103a_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVE.B (d8,PC,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_103b_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s8 src = get_byte (srca);
{	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 14 * CYCLE_UNIT / 2;
}
/* MOVE.B #<data>.B,Dn */
unsigned long REGPARAM2 CPUFUNC(op_103c_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 4);
}}}return 8 * CYCLE_UNIT / 2;
}
/* MOVE.B Dn,(An) */
unsigned long REGPARAM2 CPUFUNC(op_1080_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 2);
}}}return 8 * CYCLE_UNIT / 2;
}
/* MOVE.B (An),(An) */
unsigned long REGPARAM2 CPUFUNC(op_1090_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 2);
}}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVE.B (An)+,(An) */
unsigned long REGPARAM2 CPUFUNC(op_1098_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
{	uaecptr dsta = m68k_areg(regs, dstreg);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 2);
}}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVE.B -(An),(An) */
unsigned long REGPARAM2 CPUFUNC(op_10a0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta = m68k_areg(regs, dstreg);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 2);
}}}}return 14 * CYCLE_UNIT / 2;
}
/* MOVE.B (d16,An),(An) */
unsigned long REGPARAM2 CPUFUNC(op_10a8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.B (d8,An,Xn),(An) */
unsigned long REGPARAM2 CPUFUNC(op_10b0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 18 * CYCLE_UNIT / 2;
}
/* MOVE.B (xxx).W,(An) */
unsigned long REGPARAM2 CPUFUNC(op_10b8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.B (xxx).L,(An) */
unsigned long REGPARAM2 CPUFUNC(op_10b9_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 20 * CYCLE_UNIT / 2;
}
/* MOVE.B (d16,PC),(An) */
unsigned long REGPARAM2 CPUFUNC(op_10ba_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.B (d8,PC,Xn),(An) */
unsigned long REGPARAM2 CPUFUNC(op_10bb_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 18 * CYCLE_UNIT / 2;
}
/* MOVE.B #<data>.B,(An) */
unsigned long REGPARAM2 CPUFUNC(op_10bc_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 4);
}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVE.B Dn,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_10c0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 2);
}}}return 8 * CYCLE_UNIT / 2;
}
/* MOVE.B (An),(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_10d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 2);
}}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVE.B (An)+,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_10d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
{	uaecptr dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 2);
}}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVE.B -(An),(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_10e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 2);
}}}}return 14 * CYCLE_UNIT / 2;
}
/* MOVE.B (d16,An),(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_10e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.B (d8,An,Xn),(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_10f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 18 * CYCLE_UNIT / 2;
}
/* MOVE.B (xxx).W,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_10f8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.B (xxx).L,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_10f9_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 20 * CYCLE_UNIT / 2;
}
/* MOVE.B (d16,PC),(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_10fa_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.B (d8,PC,Xn),(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_10fb_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 18 * CYCLE_UNIT / 2;
}
/* MOVE.B #<data>.B,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_10fc_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 4);
}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVE.B Dn,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_1100_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg (regs, dstreg) = dsta;
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 2);
}}}return 8 * CYCLE_UNIT / 2;
}
/* MOVE.B (An),-(An) */
unsigned long REGPARAM2 CPUFUNC(op_1110_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg (regs, dstreg) = dsta;
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 2);
}}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVE.B (An)+,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_1118_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg (regs, dstreg) = dsta;
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 2);
}}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVE.B -(An),-(An) */
unsigned long REGPARAM2 CPUFUNC(op_1120_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg (regs, dstreg) = dsta;
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 2);
}}}}return 14 * CYCLE_UNIT / 2;
}
/* MOVE.B (d16,An),-(An) */
unsigned long REGPARAM2 CPUFUNC(op_1128_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg (regs, dstreg) = dsta;
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.B (d8,An,Xn),-(An) */
unsigned long REGPARAM2 CPUFUNC(op_1130_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg (regs, dstreg) = dsta;
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 18 * CYCLE_UNIT / 2;
}
/* MOVE.B (xxx).W,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_1138_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg (regs, dstreg) = dsta;
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.B (xxx).L,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_1139_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg (regs, dstreg) = dsta;
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 20 * CYCLE_UNIT / 2;
}
/* MOVE.B (d16,PC),-(An) */
unsigned long REGPARAM2 CPUFUNC(op_113a_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg (regs, dstreg) = dsta;
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.B (d8,PC,Xn),-(An) */
unsigned long REGPARAM2 CPUFUNC(op_113b_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg (regs, dstreg) = dsta;
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 18 * CYCLE_UNIT / 2;
}
/* MOVE.B #<data>.B,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_113c_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg (regs, dstreg) = dsta;
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 4);
}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVE.B Dn,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_1140_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 4);
}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVE.B (An),(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_1150_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.B (An)+,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_1158_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.B -(An),(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_1160_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 18 * CYCLE_UNIT / 2;
}
/* MOVE.B (d16,An),(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_1168_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 4);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 20 * CYCLE_UNIT / 2;
}
/* MOVE.B (d8,An,Xn),(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_1170_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 4);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 22 * CYCLE_UNIT / 2;
}
/* MOVE.B (xxx).W,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_1178_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 4);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 20 * CYCLE_UNIT / 2;
}
/* MOVE.B (xxx).L,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_1179_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 6);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 8);
}}}}return 24 * CYCLE_UNIT / 2;
}
/* MOVE.B (d16,PC),(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_117a_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 4);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 20 * CYCLE_UNIT / 2;
}
/* MOVE.B (d8,PC,Xn),(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_117b_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 4);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 22 * CYCLE_UNIT / 2;
}
/* MOVE.B #<data>.B,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_117c_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 4);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 6);
}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.B Dn,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_1180_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 2));
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 4);
}}}return 14 * CYCLE_UNIT / 2;
}
/* MOVE.B (An),(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_1190_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 2));
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 18 * CYCLE_UNIT / 2;
}
/* MOVE.B (An)+,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_1198_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 2));
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 18 * CYCLE_UNIT / 2;
}
/* MOVE.B -(An),(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_11a0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 2));
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 20 * CYCLE_UNIT / 2;
}
/* MOVE.B (d16,An),(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_11a8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 4));
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 22 * CYCLE_UNIT / 2;
}
/* MOVE.B (d8,An,Xn),(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_11b0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 4));
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 24 * CYCLE_UNIT / 2;
}
/* MOVE.B (xxx).W,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_11b8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 4));
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 22 * CYCLE_UNIT / 2;
}
/* MOVE.B (xxx).L,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_11b9_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 6));
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 8);
}}}}return 26 * CYCLE_UNIT / 2;
}
/* MOVE.B (d16,PC),(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_11ba_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 4));
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 22 * CYCLE_UNIT / 2;
}
/* MOVE.B (d8,PC,Xn),(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_11bb_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 4));
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 24 * CYCLE_UNIT / 2;
}
/* MOVE.B #<data>.B,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_11bc_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 4));
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 6);
}}}return 18 * CYCLE_UNIT / 2;
}
/* MOVE.B Dn,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_11c0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 2);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 4);
}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVE.B (An),(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_11d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 2);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.B (An)+,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_11d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 2);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.B -(An),(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_11e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 2);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 18 * CYCLE_UNIT / 2;
}
/* MOVE.B (d16,An),(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_11e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 4);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 20 * CYCLE_UNIT / 2;
}
/* MOVE.B (d8,An,Xn),(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_11f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 4);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 22 * CYCLE_UNIT / 2;
}
/* MOVE.B (xxx).W,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_11f8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 4);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 20 * CYCLE_UNIT / 2;
}
/* MOVE.B (xxx).L,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_11f9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 6);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 8);
}}}}return 24 * CYCLE_UNIT / 2;
}
/* MOVE.B (d16,PC),(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_11fa_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 4);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 20 * CYCLE_UNIT / 2;
}
/* MOVE.B (d8,PC,Xn),(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_11fb_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 4);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 22 * CYCLE_UNIT / 2;
}
/* MOVE.B #<data>.B,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_11fc_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 4);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 6);
}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.B Dn,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_13c0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = get_ilong (regs, 2);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 6);
}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.B (An),(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_13d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta = get_ilong (regs, 2);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 20 * CYCLE_UNIT / 2;
}
/* MOVE.B (An)+,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_13d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
{	uaecptr dsta = get_ilong (regs, 2);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 20 * CYCLE_UNIT / 2;
}
/* MOVE.B -(An),(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_13e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta = get_ilong (regs, 2);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 22 * CYCLE_UNIT / 2;
}
/* MOVE.B (d16,An),(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_13e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta = get_ilong (regs, 4);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 8);
}}}}return 24 * CYCLE_UNIT / 2;
}
/* MOVE.B (d8,An,Xn),(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_13f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta = get_ilong (regs, 4);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 8);
}}}}return 26 * CYCLE_UNIT / 2;
}
/* MOVE.B (xxx).W,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_13f8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta = get_ilong (regs, 4);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 8);
}}}}return 24 * CYCLE_UNIT / 2;
}
/* MOVE.B (xxx).L,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_13f9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta = get_ilong (regs, 6);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 10);
}}}}return 28 * CYCLE_UNIT / 2;
}
/* MOVE.B (d16,PC),(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_13fa_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta = get_ilong (regs, 4);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 8);
}}}}return 24 * CYCLE_UNIT / 2;
}
/* MOVE.B (d8,PC,Xn),(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_13fb_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s8 src = get_byte (srca);
{	uaecptr dsta = get_ilong (regs, 4);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 8);
}}}}return 26 * CYCLE_UNIT / 2;
}
/* MOVE.B #<data>.B,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_13fc_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s8 src = get_ibyte (regs, 2);
{	uaecptr dsta = get_ilong (regs, 4);
	put_byte (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_incpc (regs, 8);
}}}return 20 * CYCLE_UNIT / 2;
}
/* MOVE.L Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_2000_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	m68k_dreg(regs, dstreg) = (src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 2);
}}}return 4 * CYCLE_UNIT / 2;
}
/* MOVE.L An,Dn */
unsigned long REGPARAM2 CPUFUNC(op_2008_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_areg(regs, srcreg);
{	m68k_dreg(regs, dstreg) = (src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 2);
}}}return 4 * CYCLE_UNIT / 2;
}
/* MOVE.L (An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_2010_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
{	m68k_dreg(regs, dstreg) = (src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 2);
}}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVE.L (An)+,Dn */
unsigned long REGPARAM2 CPUFUNC(op_2018_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg(regs, srcreg) += 4;
{	m68k_dreg(regs, dstreg) = (src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 2);
}}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVE.L -(An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_2020_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	m68k_dreg(regs, dstreg) = (src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 2);
}}}}return 14 * CYCLE_UNIT / 2;
}
/* MOVE.L (d16,An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_2028_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	m68k_dreg(regs, dstreg) = (src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.L (d8,An,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_2030_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s32 src = get_long (srca);
{	m68k_dreg(regs, dstreg) = (src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 18 * CYCLE_UNIT / 2;
}
/* MOVE.L (xxx).W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_2038_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	m68k_dreg(regs, dstreg) = (src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.L (xxx).L,Dn */
unsigned long REGPARAM2 CPUFUNC(op_2039_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s32 src = get_long (srca);
{	m68k_dreg(regs, dstreg) = (src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 20 * CYCLE_UNIT / 2;
}
/* MOVE.L (d16,PC),Dn */
unsigned long REGPARAM2 CPUFUNC(op_203a_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	m68k_dreg(regs, dstreg) = (src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.L (d8,PC,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_203b_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s32 src = get_long (srca);
{	m68k_dreg(regs, dstreg) = (src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 18 * CYCLE_UNIT / 2;
}
/* MOVE.L #<data>.L,Dn */
unsigned long REGPARAM2 CPUFUNC(op_203c_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	m68k_dreg(regs, dstreg) = (src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 6);
}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVEA.L Dn,An */
unsigned long REGPARAM2 CPUFUNC(op_2040_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	m68k_areg(regs, dstreg) = (src);
	m68k_incpc (regs, 2);
}}}return 4 * CYCLE_UNIT / 2;
}
/* MOVEA.L An,An */
unsigned long REGPARAM2 CPUFUNC(op_2048_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_areg(regs, srcreg);
{	m68k_areg(regs, dstreg) = (src);
	m68k_incpc (regs, 2);
}}}return 4 * CYCLE_UNIT / 2;
}
/* MOVEA.L (An),An */
unsigned long REGPARAM2 CPUFUNC(op_2050_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
{	m68k_areg(regs, dstreg) = (src);
	m68k_incpc (regs, 2);
}}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVEA.L (An)+,An */
unsigned long REGPARAM2 CPUFUNC(op_2058_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg(regs, srcreg) += 4;
{	m68k_areg(regs, dstreg) = (src);
	m68k_incpc (regs, 2);
}}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVEA.L -(An),An */
unsigned long REGPARAM2 CPUFUNC(op_2060_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	m68k_areg(regs, dstreg) = (src);
	m68k_incpc (regs, 2);
}}}}return 14 * CYCLE_UNIT / 2;
}
/* MOVEA.L (d16,An),An */
unsigned long REGPARAM2 CPUFUNC(op_2068_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	m68k_areg(regs, dstreg) = (src);
	m68k_incpc (regs, 4);
}}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVEA.L (d8,An,Xn),An */
unsigned long REGPARAM2 CPUFUNC(op_2070_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s32 src = get_long (srca);
{	m68k_areg(regs, dstreg) = (src);
	m68k_incpc (regs, 4);
}}}}return 18 * CYCLE_UNIT / 2;
}
/* MOVEA.L (xxx).W,An */
unsigned long REGPARAM2 CPUFUNC(op_2078_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	m68k_areg(regs, dstreg) = (src);
	m68k_incpc (regs, 4);
}}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVEA.L (xxx).L,An */
unsigned long REGPARAM2 CPUFUNC(op_2079_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s32 src = get_long (srca);
{	m68k_areg(regs, dstreg) = (src);
	m68k_incpc (regs, 6);
}}}}return 20 * CYCLE_UNIT / 2;
}
/* MOVEA.L (d16,PC),An */
unsigned long REGPARAM2 CPUFUNC(op_207a_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	m68k_areg(regs, dstreg) = (src);
	m68k_incpc (regs, 4);
}}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVEA.L (d8,PC,Xn),An */
unsigned long REGPARAM2 CPUFUNC(op_207b_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s32 src = get_long (srca);
{	m68k_areg(regs, dstreg) = (src);
	m68k_incpc (regs, 4);
}}}}return 18 * CYCLE_UNIT / 2;
}
/* MOVEA.L #<data>.L,An */
unsigned long REGPARAM2 CPUFUNC(op_207c_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	m68k_areg(regs, dstreg) = (src);
	m68k_incpc (regs, 6);
}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVE.L Dn,(An) */
unsigned long REGPARAM2 CPUFUNC(op_2080_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 2);
}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVE.L An,(An) */
unsigned long REGPARAM2 CPUFUNC(op_2088_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_areg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 2);
}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVE.L (An),(An) */
unsigned long REGPARAM2 CPUFUNC(op_2090_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 2);
}}}}return 20 * CYCLE_UNIT / 2;
}
/* MOVE.L (An)+,(An) */
unsigned long REGPARAM2 CPUFUNC(op_2098_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg(regs, srcreg) += 4;
{	uaecptr dsta = m68k_areg(regs, dstreg);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 2);
}}}}return 20 * CYCLE_UNIT / 2;
}
/* MOVE.L -(An),(An) */
unsigned long REGPARAM2 CPUFUNC(op_20a0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta = m68k_areg(regs, dstreg);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 2);
}}}}return 22 * CYCLE_UNIT / 2;
}
/* MOVE.L (d16,An),(An) */
unsigned long REGPARAM2 CPUFUNC(op_20a8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 24 * CYCLE_UNIT / 2;
}
/* MOVE.L (d8,An,Xn),(An) */
unsigned long REGPARAM2 CPUFUNC(op_20b0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s32 src = get_long (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 26 * CYCLE_UNIT / 2;
}
/* MOVE.L (xxx).W,(An) */
unsigned long REGPARAM2 CPUFUNC(op_20b8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 24 * CYCLE_UNIT / 2;
}
/* MOVE.L (xxx).L,(An) */
unsigned long REGPARAM2 CPUFUNC(op_20b9_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 28 * CYCLE_UNIT / 2;
}
/* MOVE.L (d16,PC),(An) */
unsigned long REGPARAM2 CPUFUNC(op_20ba_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 24 * CYCLE_UNIT / 2;
}
/* MOVE.L (d8,PC,Xn),(An) */
unsigned long REGPARAM2 CPUFUNC(op_20bb_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s32 src = get_long (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 26 * CYCLE_UNIT / 2;
}
/* MOVE.L #<data>.L,(An) */
unsigned long REGPARAM2 CPUFUNC(op_20bc_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 6);
}}}return 20 * CYCLE_UNIT / 2;
}
/* MOVE.L Dn,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_20c0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 4;
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 2);
}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVE.L An,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_20c8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_areg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 4;
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 2);
}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVE.L (An),(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_20d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 4;
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 2);
}}}}return 20 * CYCLE_UNIT / 2;
}
/* MOVE.L (An)+,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_20d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg(regs, srcreg) += 4;
{	uaecptr dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 4;
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 2);
}}}}return 20 * CYCLE_UNIT / 2;
}
/* MOVE.L -(An),(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_20e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 4;
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 2);
}}}}return 22 * CYCLE_UNIT / 2;
}
/* MOVE.L (d16,An),(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_20e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 4;
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 24 * CYCLE_UNIT / 2;
}
/* MOVE.L (d8,An,Xn),(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_20f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s32 src = get_long (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 4;
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 26 * CYCLE_UNIT / 2;
}
/* MOVE.L (xxx).W,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_20f8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 4;
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 24 * CYCLE_UNIT / 2;
}
/* MOVE.L (xxx).L,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_20f9_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 4;
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 28 * CYCLE_UNIT / 2;
}
/* MOVE.L (d16,PC),(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_20fa_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 4;
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 24 * CYCLE_UNIT / 2;
}
/* MOVE.L (d8,PC,Xn),(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_20fb_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s32 src = get_long (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 4;
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 26 * CYCLE_UNIT / 2;
}
/* MOVE.L #<data>.L,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_20fc_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 4;
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 6);
}}}return 20 * CYCLE_UNIT / 2;
}
/* MOVE.L Dn,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_2100_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	m68k_areg (regs, dstreg) = dsta;
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 2);
}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVE.L An,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_2108_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_areg(regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	m68k_areg (regs, dstreg) = dsta;
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 2);
}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVE.L (An),-(An) */
unsigned long REGPARAM2 CPUFUNC(op_2110_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	m68k_areg (regs, dstreg) = dsta;
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 2);
}}}}return 20 * CYCLE_UNIT / 2;
}
/* MOVE.L (An)+,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_2118_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg(regs, srcreg) += 4;
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	m68k_areg (regs, dstreg) = dsta;
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 2);
}}}}return 20 * CYCLE_UNIT / 2;
}
/* MOVE.L -(An),-(An) */
unsigned long REGPARAM2 CPUFUNC(op_2120_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	m68k_areg (regs, dstreg) = dsta;
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 2);
}}}}return 22 * CYCLE_UNIT / 2;
}
/* MOVE.L (d16,An),-(An) */
unsigned long REGPARAM2 CPUFUNC(op_2128_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	m68k_areg (regs, dstreg) = dsta;
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 24 * CYCLE_UNIT / 2;
}
/* MOVE.L (d8,An,Xn),-(An) */
unsigned long REGPARAM2 CPUFUNC(op_2130_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	m68k_areg (regs, dstreg) = dsta;
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 26 * CYCLE_UNIT / 2;
}
/* MOVE.L (xxx).W,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_2138_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	m68k_areg (regs, dstreg) = dsta;
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 24 * CYCLE_UNIT / 2;
}
/* MOVE.L (xxx).L,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_2139_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	m68k_areg (regs, dstreg) = dsta;
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 28 * CYCLE_UNIT / 2;
}
/* MOVE.L (d16,PC),-(An) */
unsigned long REGPARAM2 CPUFUNC(op_213a_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	m68k_areg (regs, dstreg) = dsta;
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 24 * CYCLE_UNIT / 2;
}
/* MOVE.L (d8,PC,Xn),-(An) */
unsigned long REGPARAM2 CPUFUNC(op_213b_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	m68k_areg (regs, dstreg) = dsta;
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 26 * CYCLE_UNIT / 2;
}
/* MOVE.L #<data>.L,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_213c_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	m68k_areg (regs, dstreg) = dsta;
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 6);
}}}return 20 * CYCLE_UNIT / 2;
}
/* MOVE.L Dn,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_2140_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 4);
}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.L An,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_2148_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_areg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 4);
}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.L (An),(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_2150_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 24 * CYCLE_UNIT / 2;
}
/* MOVE.L (An)+,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_2158_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg(regs, srcreg) += 4;
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 24 * CYCLE_UNIT / 2;
}
/* MOVE.L -(An),(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_2160_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 26 * CYCLE_UNIT / 2;
}
/* MOVE.L (d16,An),(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_2168_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 4);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 28 * CYCLE_UNIT / 2;
}
/* MOVE.L (d8,An,Xn),(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_2170_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s32 src = get_long (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 4);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 30 * CYCLE_UNIT / 2;
}
/* MOVE.L (xxx).W,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_2178_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 4);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 28 * CYCLE_UNIT / 2;
}
/* MOVE.L (xxx).L,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_2179_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 6);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 8);
}}}}return 32 * CYCLE_UNIT / 2;
}
/* MOVE.L (d16,PC),(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_217a_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 4);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 28 * CYCLE_UNIT / 2;
}
/* MOVE.L (d8,PC,Xn),(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_217b_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s32 src = get_long (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 4);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 30 * CYCLE_UNIT / 2;
}
/* MOVE.L #<data>.L,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_217c_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 6);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 8);
}}}return 24 * CYCLE_UNIT / 2;
}
/* MOVE.L Dn,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_2180_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 2));
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 4);
}}}return 18 * CYCLE_UNIT / 2;
}
/* MOVE.L An,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_2188_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_areg(regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 2));
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 4);
}}}return 18 * CYCLE_UNIT / 2;
}
/* MOVE.L (An),(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_2190_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 2));
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 26 * CYCLE_UNIT / 2;
}
/* MOVE.L (An)+,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_2198_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg(regs, srcreg) += 4;
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 2));
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 26 * CYCLE_UNIT / 2;
}
/* MOVE.L -(An),(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_21a0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 2));
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 28 * CYCLE_UNIT / 2;
}
/* MOVE.L (d16,An),(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_21a8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 4));
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 30 * CYCLE_UNIT / 2;
}
/* MOVE.L (d8,An,Xn),(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_21b0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 4));
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 32 * CYCLE_UNIT / 2;
}
/* MOVE.L (xxx).W,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_21b8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 4));
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 30 * CYCLE_UNIT / 2;
}
/* MOVE.L (xxx).L,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_21b9_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 6));
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 8);
}}}}return 34 * CYCLE_UNIT / 2;
}
/* MOVE.L (d16,PC),(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_21ba_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 4));
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 30 * CYCLE_UNIT / 2;
}
/* MOVE.L (d8,PC,Xn),(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_21bb_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s32 src = get_long (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 4));
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 32 * CYCLE_UNIT / 2;
}
/* MOVE.L #<data>.L,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_21bc_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 6));
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 8);
}}}return 26 * CYCLE_UNIT / 2;
}
/* MOVE.L Dn,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_21c0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 2);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 4);
}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.L An,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_21c8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = m68k_areg(regs, srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 2);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 4);
}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.L (An),(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_21d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 2);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 24 * CYCLE_UNIT / 2;
}
/* MOVE.L (An)+,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_21d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg(regs, srcreg) += 4;
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 2);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 24 * CYCLE_UNIT / 2;
}
/* MOVE.L -(An),(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_21e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 2);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 26 * CYCLE_UNIT / 2;
}
/* MOVE.L (d16,An),(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_21e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 4);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 28 * CYCLE_UNIT / 2;
}
/* MOVE.L (d8,An,Xn),(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_21f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s32 src = get_long (srca);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 4);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 30 * CYCLE_UNIT / 2;
}
/* MOVE.L (xxx).W,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_21f8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 4);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 28 * CYCLE_UNIT / 2;
}
/* MOVE.L (xxx).L,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_21f9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 6);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 8);
}}}}return 32 * CYCLE_UNIT / 2;
}
/* MOVE.L (d16,PC),(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_21fa_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 4);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 28 * CYCLE_UNIT / 2;
}
/* MOVE.L (d8,PC,Xn),(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_21fb_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s32 src = get_long (srca);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 4);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 30 * CYCLE_UNIT / 2;
}
/* MOVE.L #<data>.L,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_21fc_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 6);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 8);
}}}return 24 * CYCLE_UNIT / 2;
}
/* MOVE.L Dn,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_23c0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = get_ilong (regs, 2);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 6);
}}}return 20 * CYCLE_UNIT / 2;
}
/* MOVE.L An,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_23c8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = m68k_areg(regs, srcreg);
{	uaecptr dsta = get_ilong (regs, 2);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 6);
}}}return 20 * CYCLE_UNIT / 2;
}
/* MOVE.L (An),(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_23d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta = get_ilong (regs, 2);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 28 * CYCLE_UNIT / 2;
}
/* MOVE.L (An)+,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_23d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg(regs, srcreg) += 4;
{	uaecptr dsta = get_ilong (regs, 2);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 28 * CYCLE_UNIT / 2;
}
/* MOVE.L -(An),(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_23e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta = get_ilong (regs, 2);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 30 * CYCLE_UNIT / 2;
}
/* MOVE.L (d16,An),(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_23e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta = get_ilong (regs, 4);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 8);
}}}}return 32 * CYCLE_UNIT / 2;
}
/* MOVE.L (d8,An,Xn),(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_23f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s32 src = get_long (srca);
{	uaecptr dsta = get_ilong (regs, 4);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 8);
}}}}return 34 * CYCLE_UNIT / 2;
}
/* MOVE.L (xxx).W,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_23f8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta = get_ilong (regs, 4);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 8);
}}}}return 32 * CYCLE_UNIT / 2;
}
/* MOVE.L (xxx).L,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_23f9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta = get_ilong (regs, 6);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 10);
}}}}return 36 * CYCLE_UNIT / 2;
}
/* MOVE.L (d16,PC),(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_23fa_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uaecptr dsta = get_ilong (regs, 4);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 8);
}}}}return 32 * CYCLE_UNIT / 2;
}
/* MOVE.L (d8,PC,Xn),(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_23fb_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s32 src = get_long (srca);
{	uaecptr dsta = get_ilong (regs, 4);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 8);
}}}}return 34 * CYCLE_UNIT / 2;
}
/* MOVE.L #<data>.L,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_23fc_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s32 src = get_ilong (regs, 2);
{	uaecptr dsta = get_ilong (regs, 6);
	put_long (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 10);
}}}return 28 * CYCLE_UNIT / 2;
}
/* MOVE.W Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_3000_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 2);
}}}return 4 * CYCLE_UNIT / 2;
}
/* MOVE.W An,Dn */
unsigned long REGPARAM2 CPUFUNC(op_3008_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_areg(regs, srcreg);
{	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 2);
}}}return 4 * CYCLE_UNIT / 2;
}
/* MOVE.W (An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_3010_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
{	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 2);
}}}}return 8 * CYCLE_UNIT / 2;
}
/* MOVE.W (An)+,Dn */
unsigned long REGPARAM2 CPUFUNC(op_3018_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg(regs, srcreg) += 2;
{	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 2);
}}}}return 8 * CYCLE_UNIT / 2;
}
/* MOVE.W -(An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_3020_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 2);
}}}}return 10 * CYCLE_UNIT / 2;
}
/* MOVE.W (d16,An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_3028_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVE.W (d8,An,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_3030_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
{	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 14 * CYCLE_UNIT / 2;
}
/* MOVE.W (xxx).W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_3038_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVE.W (xxx).L,Dn */
unsigned long REGPARAM2 CPUFUNC(op_3039_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s16 src = get_word (srca);
{	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.W (d16,PC),Dn */
unsigned long REGPARAM2 CPUFUNC(op_303a_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVE.W (d8,PC,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_303b_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
{	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 14 * CYCLE_UNIT / 2;
}
/* MOVE.W #<data>.W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_303c_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 4);
}}}return 8 * CYCLE_UNIT / 2;
}
/* MOVEA.W Dn,An */
unsigned long REGPARAM2 CPUFUNC(op_3040_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	src = (uae_s32)(uae_s16)src;
	m68k_areg(regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc (regs, 2);
}}}return 4 * CYCLE_UNIT / 2;
}
/* MOVEA.W An,An */
unsigned long REGPARAM2 CPUFUNC(op_3048_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_areg(regs, srcreg);
{	src = (uae_s32)(uae_s16)src;
	m68k_areg(regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc (regs, 2);
}}}return 4 * CYCLE_UNIT / 2;
}
/* MOVEA.W (An),An */
unsigned long REGPARAM2 CPUFUNC(op_3050_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
{	src = (uae_s32)(uae_s16)src;
	m68k_areg(regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc (regs, 2);
}}}}return 8 * CYCLE_UNIT / 2;
}
/* MOVEA.W (An)+,An */
unsigned long REGPARAM2 CPUFUNC(op_3058_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg(regs, srcreg) += 2;
{	src = (uae_s32)(uae_s16)src;
	m68k_areg(regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc (regs, 2);
}}}}return 8 * CYCLE_UNIT / 2;
}
/* MOVEA.W -(An),An */
unsigned long REGPARAM2 CPUFUNC(op_3060_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	src = (uae_s32)(uae_s16)src;
	m68k_areg(regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc (regs, 2);
}}}}return 10 * CYCLE_UNIT / 2;
}
/* MOVEA.W (d16,An),An */
unsigned long REGPARAM2 CPUFUNC(op_3068_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	src = (uae_s32)(uae_s16)src;
	m68k_areg(regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc (regs, 4);
}}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVEA.W (d8,An,Xn),An */
unsigned long REGPARAM2 CPUFUNC(op_3070_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
{	src = (uae_s32)(uae_s16)src;
	m68k_areg(regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc (regs, 4);
}}}}return 14 * CYCLE_UNIT / 2;
}
/* MOVEA.W (xxx).W,An */
unsigned long REGPARAM2 CPUFUNC(op_3078_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	src = (uae_s32)(uae_s16)src;
	m68k_areg(regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc (regs, 4);
}}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVEA.W (xxx).L,An */
unsigned long REGPARAM2 CPUFUNC(op_3079_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s16 src = get_word (srca);
{	src = (uae_s32)(uae_s16)src;
	m68k_areg(regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc (regs, 6);
}}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVEA.W (d16,PC),An */
unsigned long REGPARAM2 CPUFUNC(op_307a_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	src = (uae_s32)(uae_s16)src;
	m68k_areg(regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc (regs, 4);
}}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVEA.W (d8,PC,Xn),An */
unsigned long REGPARAM2 CPUFUNC(op_307b_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
{	src = (uae_s32)(uae_s16)src;
	m68k_areg(regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc (regs, 4);
}}}}return 14 * CYCLE_UNIT / 2;
}
/* MOVEA.W #<data>.W,An */
unsigned long REGPARAM2 CPUFUNC(op_307c_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	src = (uae_s32)(uae_s16)src;
	m68k_areg(regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc (regs, 4);
}}}return 8 * CYCLE_UNIT / 2;
}
/* MOVE.W Dn,(An) */
unsigned long REGPARAM2 CPUFUNC(op_3080_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 2);
}}}return 8 * CYCLE_UNIT / 2;
}
/* MOVE.W An,(An) */
unsigned long REGPARAM2 CPUFUNC(op_3088_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_areg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 2);
}}}return 8 * CYCLE_UNIT / 2;
}
/* MOVE.W (An),(An) */
unsigned long REGPARAM2 CPUFUNC(op_3090_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 2);
}}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVE.W (An)+,(An) */
unsigned long REGPARAM2 CPUFUNC(op_3098_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg(regs, srcreg) += 2;
{	uaecptr dsta = m68k_areg(regs, dstreg);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 2);
}}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVE.W -(An),(An) */
unsigned long REGPARAM2 CPUFUNC(op_30a0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta = m68k_areg(regs, dstreg);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 2);
}}}}return 14 * CYCLE_UNIT / 2;
}
/* MOVE.W (d16,An),(An) */
unsigned long REGPARAM2 CPUFUNC(op_30a8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.W (d8,An,Xn),(An) */
unsigned long REGPARAM2 CPUFUNC(op_30b0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 18 * CYCLE_UNIT / 2;
}
/* MOVE.W (xxx).W,(An) */
unsigned long REGPARAM2 CPUFUNC(op_30b8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.W (xxx).L,(An) */
unsigned long REGPARAM2 CPUFUNC(op_30b9_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 20 * CYCLE_UNIT / 2;
}
/* MOVE.W (d16,PC),(An) */
unsigned long REGPARAM2 CPUFUNC(op_30ba_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.W (d8,PC,Xn),(An) */
unsigned long REGPARAM2 CPUFUNC(op_30bb_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 18 * CYCLE_UNIT / 2;
}
/* MOVE.W #<data>.W,(An) */
unsigned long REGPARAM2 CPUFUNC(op_30bc_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 4);
}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVE.W Dn,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_30c0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 2;
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 2);
}}}return 8 * CYCLE_UNIT / 2;
}
/* MOVE.W An,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_30c8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_areg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 2;
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 2);
}}}return 8 * CYCLE_UNIT / 2;
}
/* MOVE.W (An),(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_30d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 2;
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 2);
}}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVE.W (An)+,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_30d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg(regs, srcreg) += 2;
{	uaecptr dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 2;
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 2);
}}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVE.W -(An),(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_30e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 2;
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 2);
}}}}return 14 * CYCLE_UNIT / 2;
}
/* MOVE.W (d16,An),(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_30e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 2;
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.W (d8,An,Xn),(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_30f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 2;
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 18 * CYCLE_UNIT / 2;
}
/* MOVE.W (xxx).W,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_30f8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 2;
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.W (xxx).L,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_30f9_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 2;
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 20 * CYCLE_UNIT / 2;
}
/* MOVE.W (d16,PC),(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_30fa_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 2;
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.W (d8,PC,Xn),(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_30fb_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 2;
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 18 * CYCLE_UNIT / 2;
}
/* MOVE.W #<data>.W,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_30fc_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 2;
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 4);
}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVE.W Dn,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_3100_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	m68k_areg (regs, dstreg) = dsta;
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 2);
}}}return 8 * CYCLE_UNIT / 2;
}
/* MOVE.W An,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_3108_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_areg(regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	m68k_areg (regs, dstreg) = dsta;
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 2);
}}}return 8 * CYCLE_UNIT / 2;
}
/* MOVE.W (An),-(An) */
unsigned long REGPARAM2 CPUFUNC(op_3110_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	m68k_areg (regs, dstreg) = dsta;
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 2);
}}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVE.W (An)+,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_3118_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg(regs, srcreg) += 2;
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	m68k_areg (regs, dstreg) = dsta;
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 2);
}}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVE.W -(An),-(An) */
unsigned long REGPARAM2 CPUFUNC(op_3120_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	m68k_areg (regs, dstreg) = dsta;
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 2);
}}}}return 14 * CYCLE_UNIT / 2;
}
/* MOVE.W (d16,An),-(An) */
unsigned long REGPARAM2 CPUFUNC(op_3128_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	m68k_areg (regs, dstreg) = dsta;
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.W (d8,An,Xn),-(An) */
unsigned long REGPARAM2 CPUFUNC(op_3130_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	m68k_areg (regs, dstreg) = dsta;
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 18 * CYCLE_UNIT / 2;
}
/* MOVE.W (xxx).W,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_3138_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	m68k_areg (regs, dstreg) = dsta;
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.W (xxx).L,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_3139_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	m68k_areg (regs, dstreg) = dsta;
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 20 * CYCLE_UNIT / 2;
}
/* MOVE.W (d16,PC),-(An) */
unsigned long REGPARAM2 CPUFUNC(op_313a_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	m68k_areg (regs, dstreg) = dsta;
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.W (d8,PC,Xn),-(An) */
unsigned long REGPARAM2 CPUFUNC(op_313b_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	m68k_areg (regs, dstreg) = dsta;
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 18 * CYCLE_UNIT / 2;
}
/* MOVE.W #<data>.W,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_313c_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	m68k_areg (regs, dstreg) = dsta;
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 4);
}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVE.W Dn,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_3140_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 4);
}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVE.W An,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_3148_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_areg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 4);
}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVE.W (An),(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_3150_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.W (An)+,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_3158_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg(regs, srcreg) += 2;
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.W -(An),(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_3160_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 18 * CYCLE_UNIT / 2;
}
/* MOVE.W (d16,An),(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_3168_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 4);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 20 * CYCLE_UNIT / 2;
}
/* MOVE.W (d8,An,Xn),(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_3170_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 4);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 22 * CYCLE_UNIT / 2;
}
/* MOVE.W (xxx).W,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_3178_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 4);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 20 * CYCLE_UNIT / 2;
}
/* MOVE.W (xxx).L,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_3179_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 6);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 8);
}}}}return 24 * CYCLE_UNIT / 2;
}
/* MOVE.W (d16,PC),(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_317a_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 4);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 20 * CYCLE_UNIT / 2;
}
/* MOVE.W (d8,PC,Xn),(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_317b_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 4);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 22 * CYCLE_UNIT / 2;
}
/* MOVE.W #<data>.W,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_317c_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 4);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 6);
}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.W Dn,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_3180_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 2));
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 4);
}}}return 14 * CYCLE_UNIT / 2;
}
/* MOVE.W An,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_3188_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_areg(regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 2));
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 4);
}}}return 14 * CYCLE_UNIT / 2;
}
/* MOVE.W (An),(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_3190_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 2));
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 18 * CYCLE_UNIT / 2;
}
/* MOVE.W (An)+,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_3198_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg(regs, srcreg) += 2;
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 2));
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 18 * CYCLE_UNIT / 2;
}
/* MOVE.W -(An),(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_31a0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 2));
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 20 * CYCLE_UNIT / 2;
}
/* MOVE.W (d16,An),(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_31a8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 4));
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 22 * CYCLE_UNIT / 2;
}
/* MOVE.W (d8,An,Xn),(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_31b0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 4));
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 24 * CYCLE_UNIT / 2;
}
/* MOVE.W (xxx).W,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_31b8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 4));
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 22 * CYCLE_UNIT / 2;
}
/* MOVE.W (xxx).L,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_31b9_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 6));
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 8);
}}}}return 26 * CYCLE_UNIT / 2;
}
/* MOVE.W (d16,PC),(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_31ba_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 4));
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 22 * CYCLE_UNIT / 2;
}
/* MOVE.W (d8,PC,Xn),(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_31bb_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 4));
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 24 * CYCLE_UNIT / 2;
}
/* MOVE.W #<data>.W,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_31bc_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 4));
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 6);
}}}return 18 * CYCLE_UNIT / 2;
}
/* MOVE.W Dn,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_31c0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 2);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 4);
}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVE.W An,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_31c8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_areg(regs, srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 2);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 4);
}}}return 12 * CYCLE_UNIT / 2;
}
/* MOVE.W (An),(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_31d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 2);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.W (An)+,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_31d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg(regs, srcreg) += 2;
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 2);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.W -(An),(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_31e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 2);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 4);
}}}}return 18 * CYCLE_UNIT / 2;
}
/* MOVE.W (d16,An),(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_31e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 4);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 20 * CYCLE_UNIT / 2;
}
/* MOVE.W (d8,An,Xn),(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_31f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 4);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 22 * CYCLE_UNIT / 2;
}
/* MOVE.W (xxx).W,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_31f8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 4);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 20 * CYCLE_UNIT / 2;
}
/* MOVE.W (xxx).L,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_31f9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 6);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 8);
}}}}return 24 * CYCLE_UNIT / 2;
}
/* MOVE.W (d16,PC),(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_31fa_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 4);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 20 * CYCLE_UNIT / 2;
}
/* MOVE.W (d8,PC,Xn),(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_31fb_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 4);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 22 * CYCLE_UNIT / 2;
}
/* MOVE.W #<data>.W,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_31fc_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 4);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 6);
}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.W Dn,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_33c0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = get_ilong (regs, 2);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 6);
}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.W An,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_33c8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_areg(regs, srcreg);
{	uaecptr dsta = get_ilong (regs, 2);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 6);
}}}return 16 * CYCLE_UNIT / 2;
}
/* MOVE.W (An),(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_33d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta = get_ilong (regs, 2);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 20 * CYCLE_UNIT / 2;
}
/* MOVE.W (An)+,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_33d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg(regs, srcreg) += 2;
{	uaecptr dsta = get_ilong (regs, 2);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 20 * CYCLE_UNIT / 2;
}
/* MOVE.W -(An),(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_33e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta = get_ilong (regs, 2);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 6);
}}}}return 22 * CYCLE_UNIT / 2;
}
/* MOVE.W (d16,An),(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_33e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta = get_ilong (regs, 4);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 8);
}}}}return 24 * CYCLE_UNIT / 2;
}
/* MOVE.W (d8,An,Xn),(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_33f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
{	uaecptr dsta = get_ilong (regs, 4);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 8);
}}}}return 26 * CYCLE_UNIT / 2;
}
/* MOVE.W (xxx).W,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_33f8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta = get_ilong (regs, 4);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 8);
}}}}return 24 * CYCLE_UNIT / 2;
}
/* MOVE.W (xxx).L,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_33f9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta = get_ilong (regs, 6);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 10);
}}}}return 28 * CYCLE_UNIT / 2;
}
/* MOVE.W (d16,PC),(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_33fa_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uaecptr dsta = get_ilong (regs, 4);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 8);
}}}}return 24 * CYCLE_UNIT / 2;
}
/* MOVE.W (d8,PC,Xn),(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_33fb_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
{	uaecptr dsta = get_ilong (regs, 4);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 8);
}}}}return 26 * CYCLE_UNIT / 2;
}
/* MOVE.W #<data>.W,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_33fc_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s16 src = get_iword (regs, 2);
{	uaecptr dsta = get_ilong (regs, 4);
	put_word (dsta,src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_incpc (regs, 8);
}}}return 20 * CYCLE_UNIT / 2;
}
/* NEGX.B Dn */
unsigned long REGPARAM2 CPUFUNC(op_4000_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uae_u32 newv = 0 - src - (GET_XFLG (&regs->ccrflags) ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s8)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s8)(newv)) < 0);
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((newv) & 0xff);
}}}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* NEGX.B (An) */
unsigned long REGPARAM2 CPUFUNC(op_4010_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s8 src = get_byte (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG (&regs->ccrflags) ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s8)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s8)(newv)) < 0);
	put_byte (srca,newv);
}}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* NEGX.B (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_4018_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
{	uae_u32 newv = 0 - src - (GET_XFLG (&regs->ccrflags) ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s8)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s8)(newv)) < 0);
	put_byte (srca,newv);
}}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* NEGX.B -(An) */
unsigned long REGPARAM2 CPUFUNC(op_4020_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_u32 newv = 0 - src - (GET_XFLG (&regs->ccrflags) ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s8)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s8)(newv)) < 0);
	put_byte (srca,newv);
}}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* NEGX.B (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_4028_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG (&regs->ccrflags) ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s8)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s8)(newv)) < 0);
	put_byte (srca,newv);
}}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* NEGX.B (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_4030_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s8 src = get_byte (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG (&regs->ccrflags) ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s8)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s8)(newv)) < 0);
	put_byte (srca,newv);
}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* NEGX.B (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_4038_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG (&regs->ccrflags) ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s8)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s8)(newv)) < 0);
	put_byte (srca,newv);
}}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* NEGX.B (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_4039_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG (&regs->ccrflags) ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s8)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s8)(newv)) < 0);
	put_byte (srca,newv);
}}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* NEGX.W Dn */
unsigned long REGPARAM2 CPUFUNC(op_4040_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uae_u32 newv = 0 - src - (GET_XFLG (&regs->ccrflags) ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s16)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s16)(newv)) < 0);
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | ((newv) & 0xffff);
}}}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* NEGX.W (An) */
unsigned long REGPARAM2 CPUFUNC(op_4050_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG (&regs->ccrflags) ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s16)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s16)(newv)) < 0);
	put_word (srca,newv);
}}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* NEGX.W (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_4058_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg(regs, srcreg) += 2;
{	uae_u32 newv = 0 - src - (GET_XFLG (&regs->ccrflags) ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s16)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s16)(newv)) < 0);
	put_word (srca,newv);
}}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* NEGX.W -(An) */
unsigned long REGPARAM2 CPUFUNC(op_4060_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_u32 newv = 0 - src - (GET_XFLG (&regs->ccrflags) ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s16)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s16)(newv)) < 0);
	put_word (srca,newv);
}}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* NEGX.W (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_4068_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG (&regs->ccrflags) ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s16)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s16)(newv)) < 0);
	put_word (srca,newv);
}}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* NEGX.W (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_4070_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG (&regs->ccrflags) ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s16)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s16)(newv)) < 0);
	put_word (srca,newv);
}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* NEGX.W (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_4078_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG (&regs->ccrflags) ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s16)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s16)(newv)) < 0);
	put_word (srca,newv);
}}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* NEGX.W (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_4079_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG (&regs->ccrflags) ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s16)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s16)(newv)) < 0);
	put_word (srca,newv);
}}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* NEGX.L Dn */
unsigned long REGPARAM2 CPUFUNC(op_4080_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uae_u32 newv = 0 - src - (GET_XFLG (&regs->ccrflags) ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s32)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s32)(newv)) < 0);
	m68k_dreg(regs, srcreg) = (newv);
}}}}	m68k_incpc (regs, 2);
return 6 * CYCLE_UNIT / 2;
}
/* NEGX.L (An) */
unsigned long REGPARAM2 CPUFUNC(op_4090_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG (&regs->ccrflags) ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s32)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s32)(newv)) < 0);
	put_long (srca,newv);
}}}}}	m68k_incpc (regs, 2);
return 20 * CYCLE_UNIT / 2;
}
/* NEGX.L (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_4098_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg(regs, srcreg) += 4;
{	uae_u32 newv = 0 - src - (GET_XFLG (&regs->ccrflags) ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s32)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s32)(newv)) < 0);
	put_long (srca,newv);
}}}}}	m68k_incpc (regs, 2);
return 20 * CYCLE_UNIT / 2;
}
/* NEGX.L -(An) */
unsigned long REGPARAM2 CPUFUNC(op_40a0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_u32 newv = 0 - src - (GET_XFLG (&regs->ccrflags) ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s32)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s32)(newv)) < 0);
	put_long (srca,newv);
}}}}}	m68k_incpc (regs, 2);
return 22 * CYCLE_UNIT / 2;
}
/* NEGX.L (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_40a8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG (&regs->ccrflags) ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s32)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s32)(newv)) < 0);
	put_long (srca,newv);
}}}}}	m68k_incpc (regs, 4);
return 24 * CYCLE_UNIT / 2;
}
/* NEGX.L (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_40b0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s32 src = get_long (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG (&regs->ccrflags) ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s32)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s32)(newv)) < 0);
	put_long (srca,newv);
}}}}}	m68k_incpc (regs, 4);
return 26 * CYCLE_UNIT / 2;
}
/* NEGX.L (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_40b8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG (&regs->ccrflags) ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s32)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s32)(newv)) < 0);
	put_long (srca,newv);
}}}}}	m68k_incpc (regs, 4);
return 24 * CYCLE_UNIT / 2;
}
/* NEGX.L (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_40b9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s32 src = get_long (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG (&regs->ccrflags) ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s32)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s32)(newv)) < 0);
	put_long (srca,newv);
}}}}}	m68k_incpc (regs, 6);
return 28 * CYCLE_UNIT / 2;
}
/* MVSR2.W Dn */
unsigned long REGPARAM2 CPUFUNC(op_40c0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs->s) { Exception(8, regs, 0); return 34 * CYCLE_UNIT / 2; }
{{	MakeSR(regs);
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | ((regs->sr) & 0xffff);
}}}	m68k_incpc (regs, 2);
return 6 * CYCLE_UNIT / 2;
}
/* MVSR2.W (An) */
unsigned long REGPARAM2 CPUFUNC(op_40d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs->s) { Exception(8, regs, 0); return 34 * CYCLE_UNIT / 2; }
{{	uaecptr srca = m68k_areg(regs, srcreg);
	MakeSR(regs);
	put_word (srca,regs->sr);
}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* MVSR2.W (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_40d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs->s) { Exception(8, regs, 0); return 34 * CYCLE_UNIT / 2; }
{{	uaecptr srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += 2;
	MakeSR(regs);
	put_word (srca,regs->sr);
}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* MVSR2.W -(An) */
unsigned long REGPARAM2 CPUFUNC(op_40e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs->s) { Exception(8, regs, 0); return 34 * CYCLE_UNIT / 2; }
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	m68k_areg (regs, srcreg) = srca;
	MakeSR(regs);
	put_word (srca,regs->sr);
}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* MVSR2.W (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_40e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs->s) { Exception(8, regs, 0); return 34 * CYCLE_UNIT / 2; }
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
	MakeSR(regs);
	put_word (srca,regs->sr);
}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* MVSR2.W (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_40f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs->s) { Exception(8, regs, 0); return 34 * CYCLE_UNIT / 2; }
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
	MakeSR(regs);
	put_word (srca,regs->sr);
}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* MVSR2.W (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_40f8_4)(uae_u32 opcode, struct regstruct *regs)
{
{if (!regs->s) { Exception(8, regs, 0); return 34 * CYCLE_UNIT / 2; }
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
	MakeSR(regs);
	put_word (srca,regs->sr);
}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* MVSR2.W (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_40f9_4)(uae_u32 opcode, struct regstruct *regs)
{
{if (!regs->s) { Exception(8, regs, 0); return 34 * CYCLE_UNIT / 2; }
{{	uaecptr srca = get_ilong (regs, 2);
	MakeSR(regs);
	put_word (srca,regs->sr);
}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* CHK.W Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_4180_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr oldpc = m68k_getpc(regs);
{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
	m68k_incpc (regs, 2);
	if ((uae_s32)dst < 0) {
		SET_NFLG (&regs->ccrflags, 1);
		Exception (6, regs, oldpc);
		return 40 * CYCLE_UNIT / 2;
	} else if (dst > src) {
		SET_NFLG (&regs->ccrflags, 0);
		Exception (6, regs, oldpc);
		return 40 * CYCLE_UNIT / 2;
	}
}}}
return 10 * CYCLE_UNIT / 2;
}
/* CHK.W (An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_4190_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr oldpc = m68k_getpc(regs);
{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
	m68k_incpc (regs, 2);
	if ((uae_s32)dst < 0) {
		SET_NFLG (&regs->ccrflags, 1);
		Exception (6, regs, oldpc);
		return 44 * CYCLE_UNIT / 2;
	} else if (dst > src) {
		SET_NFLG (&regs->ccrflags, 0);
		Exception (6, regs, oldpc);
		return 44 * CYCLE_UNIT / 2;
	}
}}}}
return 14 * CYCLE_UNIT / 2;
}
/* CHK.W (An)+,Dn */
unsigned long REGPARAM2 CPUFUNC(op_4198_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr oldpc = m68k_getpc(regs);
{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg(regs, srcreg) += 2;
{	uae_s16 dst = m68k_dreg(regs, dstreg);
	m68k_incpc (regs, 2);
	if ((uae_s32)dst < 0) {
		SET_NFLG (&regs->ccrflags, 1);
		Exception (6, regs, oldpc);
		return 44 * CYCLE_UNIT / 2;
	} else if (dst > src) {
		SET_NFLG (&regs->ccrflags, 0);
		Exception (6, regs, oldpc);
		return 44 * CYCLE_UNIT / 2;
	}
}}}}
return 14 * CYCLE_UNIT / 2;
}
/* CHK.W -(An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_41a0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr oldpc = m68k_getpc(regs);
{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s16 dst = m68k_dreg(regs, dstreg);
	m68k_incpc (regs, 2);
	if ((uae_s32)dst < 0) {
		SET_NFLG (&regs->ccrflags, 1);
		Exception (6, regs, oldpc);
		return 46 * CYCLE_UNIT / 2;
	} else if (dst > src) {
		SET_NFLG (&regs->ccrflags, 0);
		Exception (6, regs, oldpc);
		return 46 * CYCLE_UNIT / 2;
	}
}}}}
return 16 * CYCLE_UNIT / 2;
}
/* CHK.W (d16,An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_41a8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr oldpc = m68k_getpc(regs);
{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
	m68k_incpc (regs, 4);
	if ((uae_s32)dst < 0) {
		SET_NFLG (&regs->ccrflags, 1);
		Exception (6, regs, oldpc);
		return 48 * CYCLE_UNIT / 2;
	} else if (dst > src) {
		SET_NFLG (&regs->ccrflags, 0);
		Exception (6, regs, oldpc);
		return 48 * CYCLE_UNIT / 2;
	}
}}}}
return 18 * CYCLE_UNIT / 2;
}
/* CHK.W (d8,An,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_41b0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr oldpc = m68k_getpc(regs);
{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
	m68k_incpc (regs, 4);
	if ((uae_s32)dst < 0) {
		SET_NFLG (&regs->ccrflags, 1);
		Exception (6, regs, oldpc);
		return 50 * CYCLE_UNIT / 2;
	} else if (dst > src) {
		SET_NFLG (&regs->ccrflags, 0);
		Exception (6, regs, oldpc);
		return 50 * CYCLE_UNIT / 2;
	}
}}}}
return 20 * CYCLE_UNIT / 2;
}
/* CHK.W (xxx).W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_41b8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr oldpc = m68k_getpc(regs);
{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
	m68k_incpc (regs, 4);
	if ((uae_s32)dst < 0) {
		SET_NFLG (&regs->ccrflags, 1);
		Exception (6, regs, oldpc);
		return 48 * CYCLE_UNIT / 2;
	} else if (dst > src) {
		SET_NFLG (&regs->ccrflags, 0);
		Exception (6, regs, oldpc);
		return 48 * CYCLE_UNIT / 2;
	}
}}}}
return 18 * CYCLE_UNIT / 2;
}
/* CHK.W (xxx).L,Dn */
unsigned long REGPARAM2 CPUFUNC(op_41b9_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr oldpc = m68k_getpc(regs);
{	uaecptr srca = get_ilong (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
	m68k_incpc (regs, 6);
	if ((uae_s32)dst < 0) {
		SET_NFLG (&regs->ccrflags, 1);
		Exception (6, regs, oldpc);
		return 52 * CYCLE_UNIT / 2;
	} else if (dst > src) {
		SET_NFLG (&regs->ccrflags, 0);
		Exception (6, regs, oldpc);
		return 52 * CYCLE_UNIT / 2;
	}
}}}}
return 22 * CYCLE_UNIT / 2;
}
/* CHK.W (d16,PC),Dn */
unsigned long REGPARAM2 CPUFUNC(op_41ba_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr oldpc = m68k_getpc(regs);
{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
	m68k_incpc (regs, 4);
	if ((uae_s32)dst < 0) {
		SET_NFLG (&regs->ccrflags, 1);
		Exception (6, regs, oldpc);
		return 48 * CYCLE_UNIT / 2;
	} else if (dst > src) {
		SET_NFLG (&regs->ccrflags, 0);
		Exception (6, regs, oldpc);
		return 48 * CYCLE_UNIT / 2;
	}
}}}}
return 18 * CYCLE_UNIT / 2;
}
/* CHK.W (d8,PC,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_41bb_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr oldpc = m68k_getpc(regs);
{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
	m68k_incpc (regs, 4);
	if ((uae_s32)dst < 0) {
		SET_NFLG (&regs->ccrflags, 1);
		Exception (6, regs, oldpc);
		return 50 * CYCLE_UNIT / 2;
	} else if (dst > src) {
		SET_NFLG (&regs->ccrflags, 0);
		Exception (6, regs, oldpc);
		return 50 * CYCLE_UNIT / 2;
	}
}}}}
return 20 * CYCLE_UNIT / 2;
}
/* CHK.W #<data>.W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_41bc_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr oldpc = m68k_getpc(regs);
{	uae_s16 src = get_iword (regs, 2);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
	m68k_incpc (regs, 4);
	if ((uae_s32)dst < 0) {
		SET_NFLG (&regs->ccrflags, 1);
		Exception (6, regs, oldpc);
		return 44 * CYCLE_UNIT / 2;
	} else if (dst > src) {
		SET_NFLG (&regs->ccrflags, 0);
		Exception (6, regs, oldpc);
		return 44 * CYCLE_UNIT / 2;
	}
}}}
return 14 * CYCLE_UNIT / 2;
}
/* LEA.L (An),An */
unsigned long REGPARAM2 CPUFUNC(op_41d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	m68k_areg(regs, dstreg) = (srca);
}}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* LEA.L (d16,An),An */
unsigned long REGPARAM2 CPUFUNC(op_41e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	m68k_areg(regs, dstreg) = (srca);
}}}	m68k_incpc (regs, 4);
return 8 * CYCLE_UNIT / 2;
}
/* LEA.L (d8,An,Xn),An */
unsigned long REGPARAM2 CPUFUNC(op_41f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	m68k_areg(regs, dstreg) = (srca);
}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* LEA.L (xxx).W,An */
unsigned long REGPARAM2 CPUFUNC(op_41f8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	m68k_areg(regs, dstreg) = (srca);
}}}	m68k_incpc (regs, 4);
return 8 * CYCLE_UNIT / 2;
}
/* LEA.L (xxx).L,An */
unsigned long REGPARAM2 CPUFUNC(op_41f9_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	m68k_areg(regs, dstreg) = (srca);
}}}	m68k_incpc (regs, 6);
return 12 * CYCLE_UNIT / 2;
}
/* LEA.L (d16,PC),An */
unsigned long REGPARAM2 CPUFUNC(op_41fa_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	m68k_areg(regs, dstreg) = (srca);
}}}	m68k_incpc (regs, 4);
return 8 * CYCLE_UNIT / 2;
}
/* LEA.L (d8,PC,Xn),An */
unsigned long REGPARAM2 CPUFUNC(op_41fb_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	m68k_areg(regs, dstreg) = (srca);
}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* CLR.B Dn */
unsigned long REGPARAM2 CPUFUNC(op_4200_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(0)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(0)) < 0);
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((0) & 0xff);
}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* CLR.B (An) */
unsigned long REGPARAM2 CPUFUNC(op_4210_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(0)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(0)) < 0);
	put_byte (srca,0);
}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* CLR.B (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_4218_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(0)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(0)) < 0);
	put_byte (srca,0);
}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* CLR.B -(An) */
unsigned long REGPARAM2 CPUFUNC(op_4220_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(0)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(0)) < 0);
	put_byte (srca,0);
}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* CLR.B (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_4228_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(0)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(0)) < 0);
	put_byte (srca,0);
}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* CLR.B (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_4230_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(0)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(0)) < 0);
	put_byte (srca,0);
}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* CLR.B (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_4238_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(0)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(0)) < 0);
	put_byte (srca,0);
}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* CLR.B (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_4239_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(0)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(0)) < 0);
	put_byte (srca,0);
}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* CLR.W Dn */
unsigned long REGPARAM2 CPUFUNC(op_4240_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(0)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(0)) < 0);
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | ((0) & 0xffff);
}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* CLR.W (An) */
unsigned long REGPARAM2 CPUFUNC(op_4250_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(0)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(0)) < 0);
	put_word (srca,0);
}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* CLR.W (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_4258_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += 2;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(0)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(0)) < 0);
	put_word (srca,0);
}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* CLR.W -(An) */
unsigned long REGPARAM2 CPUFUNC(op_4260_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	m68k_areg (regs, srcreg) = srca;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(0)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(0)) < 0);
	put_word (srca,0);
}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* CLR.W (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_4268_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(0)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(0)) < 0);
	put_word (srca,0);
}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* CLR.W (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_4270_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(0)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(0)) < 0);
	put_word (srca,0);
}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* CLR.W (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_4278_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(0)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(0)) < 0);
	put_word (srca,0);
}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* CLR.W (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_4279_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(0)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(0)) < 0);
	put_word (srca,0);
}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* CLR.L Dn */
unsigned long REGPARAM2 CPUFUNC(op_4280_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(0)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(0)) < 0);
	m68k_dreg(regs, srcreg) = (0);
}}	m68k_incpc (regs, 2);
return 6 * CYCLE_UNIT / 2;
}
/* CLR.L (An) */
unsigned long REGPARAM2 CPUFUNC(op_4290_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(0)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(0)) < 0);
	put_long (srca,0);
}}	m68k_incpc (regs, 2);
return 20 * CYCLE_UNIT / 2;
}
/* CLR.L (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_4298_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += 4;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(0)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(0)) < 0);
	put_long (srca,0);
}}	m68k_incpc (regs, 2);
return 20 * CYCLE_UNIT / 2;
}
/* CLR.L -(An) */
unsigned long REGPARAM2 CPUFUNC(op_42a0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
	m68k_areg (regs, srcreg) = srca;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(0)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(0)) < 0);
	put_long (srca,0);
}}	m68k_incpc (regs, 2);
return 22 * CYCLE_UNIT / 2;
}
/* CLR.L (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_42a8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(0)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(0)) < 0);
	put_long (srca,0);
}}	m68k_incpc (regs, 4);
return 24 * CYCLE_UNIT / 2;
}
/* CLR.L (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_42b0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(0)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(0)) < 0);
	put_long (srca,0);
}}	m68k_incpc (regs, 4);
return 26 * CYCLE_UNIT / 2;
}
/* CLR.L (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_42b8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(0)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(0)) < 0);
	put_long (srca,0);
}}	m68k_incpc (regs, 4);
return 24 * CYCLE_UNIT / 2;
}
/* CLR.L (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_42b9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(0)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(0)) < 0);
	put_long (srca,0);
}}	m68k_incpc (regs, 6);
return 28 * CYCLE_UNIT / 2;
}
/* MVSR2.B Dn */
unsigned long REGPARAM2 CPUFUNC(op_42c0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	MakeSR(regs);
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | ((regs->sr & 0xff) & 0xffff);
}}	m68k_incpc (regs, 2);
return 6 * CYCLE_UNIT / 2;
}
/* MVSR2.B (An) */
unsigned long REGPARAM2 CPUFUNC(op_42d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
	MakeSR(regs);
	put_word (srca,regs->sr & 0xff);
}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* MVSR2.B (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_42d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += 2;
	MakeSR(regs);
	put_word (srca,regs->sr & 0xff);
}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* MVSR2.B -(An) */
unsigned long REGPARAM2 CPUFUNC(op_42e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	m68k_areg (regs, srcreg) = srca;
	MakeSR(regs);
	put_word (srca,regs->sr & 0xff);
}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* MVSR2.B (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_42e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
	MakeSR(regs);
	put_word (srca,regs->sr & 0xff);
}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* MVSR2.B (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_42f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
	MakeSR(regs);
	put_word (srca,regs->sr & 0xff);
}}	m68k_incpc (regs, 4);
return 14 * CYCLE_UNIT / 2;
}
/* MVSR2.B (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_42f8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
	MakeSR(regs);
	put_word (srca,regs->sr & 0xff);
}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* MVSR2.B (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_42f9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
	MakeSR(regs);
	put_word (srca,regs->sr & 0xff);
}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* NEG.B Dn */
unsigned long REGPARAM2 CPUFUNC(op_4400_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{{uae_u32 dst = ((uae_s8)(0)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(dst)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(dst)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(0)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((dst) & 0xff);
}}}}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* NEG.B (An) */
unsigned long REGPARAM2 CPUFUNC(op_4410_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s8 src = get_byte (srca);
{{uae_u32 dst = ((uae_s8)(0)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(dst)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(dst)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(0)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (srca,dst);
}}}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* NEG.B (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_4418_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
{{uae_u32 dst = ((uae_s8)(0)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(dst)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(dst)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(0)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (srca,dst);
}}}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* NEG.B -(An) */
unsigned long REGPARAM2 CPUFUNC(op_4420_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{{uae_u32 dst = ((uae_s8)(0)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(dst)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(dst)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(0)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (srca,dst);
}}}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* NEG.B (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_4428_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{{uae_u32 dst = ((uae_s8)(0)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(dst)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(dst)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(0)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (srca,dst);
}}}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* NEG.B (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_4430_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s8 src = get_byte (srca);
{{uae_u32 dst = ((uae_s8)(0)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(dst)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(dst)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(0)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (srca,dst);
}}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* NEG.B (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_4438_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{{uae_u32 dst = ((uae_s8)(0)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(dst)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(dst)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(0)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (srca,dst);
}}}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* NEG.B (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_4439_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s8 src = get_byte (srca);
{{uae_u32 dst = ((uae_s8)(0)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(dst)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(dst)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(0)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (srca,dst);
}}}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* NEG.W Dn */
unsigned long REGPARAM2 CPUFUNC(op_4440_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{{uae_u32 dst = ((uae_s16)(0)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(dst)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(dst)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(0)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | ((dst) & 0xffff);
}}}}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* NEG.W (An) */
unsigned long REGPARAM2 CPUFUNC(op_4450_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
{{uae_u32 dst = ((uae_s16)(0)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(dst)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(dst)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(0)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (srca,dst);
}}}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* NEG.W (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_4458_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg(regs, srcreg) += 2;
{{uae_u32 dst = ((uae_s16)(0)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(dst)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(dst)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(0)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (srca,dst);
}}}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* NEG.W -(An) */
unsigned long REGPARAM2 CPUFUNC(op_4460_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{{uae_u32 dst = ((uae_s16)(0)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(dst)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(dst)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(0)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (srca,dst);
}}}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* NEG.W (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_4468_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{{uae_u32 dst = ((uae_s16)(0)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(dst)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(dst)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(0)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (srca,dst);
}}}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* NEG.W (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_4470_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
{{uae_u32 dst = ((uae_s16)(0)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(dst)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(dst)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(0)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (srca,dst);
}}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* NEG.W (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_4478_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{{uae_u32 dst = ((uae_s16)(0)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(dst)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(dst)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(0)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (srca,dst);
}}}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* NEG.W (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_4479_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s16 src = get_word (srca);
{{uae_u32 dst = ((uae_s16)(0)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(dst)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(dst)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(0)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (srca,dst);
}}}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* NEG.L Dn */
unsigned long REGPARAM2 CPUFUNC(op_4480_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{{uae_u32 dst = ((uae_s32)(0)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(dst)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(dst)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(0)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, srcreg) = (dst);
}}}}}	m68k_incpc (regs, 2);
return 6 * CYCLE_UNIT / 2;
}
/* NEG.L (An) */
unsigned long REGPARAM2 CPUFUNC(op_4490_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
{{uae_u32 dst = ((uae_s32)(0)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(dst)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(dst)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(0)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (srca,dst);
}}}}}}	m68k_incpc (regs, 2);
return 20 * CYCLE_UNIT / 2;
}
/* NEG.L (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_4498_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg(regs, srcreg) += 4;
{{uae_u32 dst = ((uae_s32)(0)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(dst)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(dst)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(0)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (srca,dst);
}}}}}}	m68k_incpc (regs, 2);
return 20 * CYCLE_UNIT / 2;
}
/* NEG.L -(An) */
unsigned long REGPARAM2 CPUFUNC(op_44a0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{{uae_u32 dst = ((uae_s32)(0)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(dst)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(dst)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(0)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (srca,dst);
}}}}}}	m68k_incpc (regs, 2);
return 22 * CYCLE_UNIT / 2;
}
/* NEG.L (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_44a8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{{uae_u32 dst = ((uae_s32)(0)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(dst)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(dst)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(0)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (srca,dst);
}}}}}}	m68k_incpc (regs, 4);
return 24 * CYCLE_UNIT / 2;
}
/* NEG.L (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_44b0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s32 src = get_long (srca);
{{uae_u32 dst = ((uae_s32)(0)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(dst)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(dst)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(0)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (srca,dst);
}}}}}}	m68k_incpc (regs, 4);
return 26 * CYCLE_UNIT / 2;
}
/* NEG.L (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_44b8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{{uae_u32 dst = ((uae_s32)(0)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(dst)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(dst)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(0)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (srca,dst);
}}}}}}	m68k_incpc (regs, 4);
return 24 * CYCLE_UNIT / 2;
}
/* NEG.L (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_44b9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s32 src = get_long (srca);
{{uae_u32 dst = ((uae_s32)(0)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(dst)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(dst)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(0)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (srca,dst);
}}}}}}	m68k_incpc (regs, 6);
return 28 * CYCLE_UNIT / 2;
}
/* MV2SR.B Dn */
unsigned long REGPARAM2 CPUFUNC(op_44c0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg(regs, srcreg);
	MakeSR(regs);
	regs->sr &= 0xFF00;
	regs->sr |= src & 0xFF;
	MakeFromSR(regs);
}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* MV2SR.B (An) */
unsigned long REGPARAM2 CPUFUNC(op_44d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
	MakeSR(regs);
	regs->sr &= 0xFF00;
	regs->sr |= src & 0xFF;
	MakeFromSR(regs);
}}}	m68k_incpc (regs, 2);
return 16 * CYCLE_UNIT / 2;
}
/* MV2SR.B (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_44d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg(regs, srcreg) += 2;
	MakeSR(regs);
	regs->sr &= 0xFF00;
	regs->sr |= src & 0xFF;
	MakeFromSR(regs);
}}}	m68k_incpc (regs, 2);
return 16 * CYCLE_UNIT / 2;
}
/* MV2SR.B -(An) */
unsigned long REGPARAM2 CPUFUNC(op_44e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
	MakeSR(regs);
	regs->sr &= 0xFF00;
	regs->sr |= src & 0xFF;
	MakeFromSR(regs);
}}}	m68k_incpc (regs, 2);
return 18 * CYCLE_UNIT / 2;
}
/* MV2SR.B (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_44e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
	MakeSR(regs);
	regs->sr &= 0xFF00;
	regs->sr |= src & 0xFF;
	MakeFromSR(regs);
}}}	m68k_incpc (regs, 4);
return 20 * CYCLE_UNIT / 2;
}
/* MV2SR.B (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_44f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
	MakeSR(regs);
	regs->sr &= 0xFF00;
	regs->sr |= src & 0xFF;
	MakeFromSR(regs);
}}}	m68k_incpc (regs, 4);
return 22 * CYCLE_UNIT / 2;
}
/* MV2SR.B (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_44f8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
	MakeSR(regs);
	regs->sr &= 0xFF00;
	regs->sr |= src & 0xFF;
	MakeFromSR(regs);
}}}	m68k_incpc (regs, 4);
return 20 * CYCLE_UNIT / 2;
}
/* MV2SR.B (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_44f9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s16 src = get_word (srca);
	MakeSR(regs);
	regs->sr &= 0xFF00;
	regs->sr |= src & 0xFF;
	MakeFromSR(regs);
}}}	m68k_incpc (regs, 6);
return 24 * CYCLE_UNIT / 2;
}
/* MV2SR.B (d16,PC) */
unsigned long REGPARAM2 CPUFUNC(op_44fa_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
	MakeSR(regs);
	regs->sr &= 0xFF00;
	regs->sr |= src & 0xFF;
	MakeFromSR(regs);
}}}	m68k_incpc (regs, 4);
return 20 * CYCLE_UNIT / 2;
}
/* MV2SR.B (d8,PC,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_44fb_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
	MakeSR(regs);
	regs->sr &= 0xFF00;
	regs->sr |= src & 0xFF;
	MakeFromSR(regs);
}}}	m68k_incpc (regs, 4);
return 22 * CYCLE_UNIT / 2;
}
/* MV2SR.B #<data>.B */
unsigned long REGPARAM2 CPUFUNC(op_44fc_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s16 src = get_iword (regs, 2);
	MakeSR(regs);
	regs->sr &= 0xFF00;
	regs->sr |= src & 0xFF;
	MakeFromSR(regs);
}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* NOT.B Dn */
unsigned long REGPARAM2 CPUFUNC(op_4600_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uae_u32 dst = ~src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(dst)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(dst)) < 0);
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((dst) & 0xff);
}}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* NOT.B (An) */
unsigned long REGPARAM2 CPUFUNC(op_4610_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s8 src = get_byte (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(dst)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(dst)) < 0);
	put_byte (srca,dst);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* NOT.B (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_4618_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
{	uae_u32 dst = ~src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(dst)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(dst)) < 0);
	put_byte (srca,dst);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* NOT.B -(An) */
unsigned long REGPARAM2 CPUFUNC(op_4620_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_u32 dst = ~src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(dst)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(dst)) < 0);
	put_byte (srca,dst);
}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* NOT.B (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_4628_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(dst)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(dst)) < 0);
	put_byte (srca,dst);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* NOT.B (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_4630_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s8 src = get_byte (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(dst)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(dst)) < 0);
	put_byte (srca,dst);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* NOT.B (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_4638_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(dst)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(dst)) < 0);
	put_byte (srca,dst);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* NOT.B (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_4639_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(dst)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(dst)) < 0);
	put_byte (srca,dst);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* NOT.W Dn */
unsigned long REGPARAM2 CPUFUNC(op_4640_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uae_u32 dst = ~src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(dst)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(dst)) < 0);
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | ((dst) & 0xffff);
}}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* NOT.W (An) */
unsigned long REGPARAM2 CPUFUNC(op_4650_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(dst)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(dst)) < 0);
	put_word (srca,dst);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* NOT.W (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_4658_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg(regs, srcreg) += 2;
{	uae_u32 dst = ~src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(dst)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(dst)) < 0);
	put_word (srca,dst);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* NOT.W -(An) */
unsigned long REGPARAM2 CPUFUNC(op_4660_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_u32 dst = ~src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(dst)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(dst)) < 0);
	put_word (srca,dst);
}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* NOT.W (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_4668_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(dst)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(dst)) < 0);
	put_word (srca,dst);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* NOT.W (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_4670_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(dst)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(dst)) < 0);
	put_word (srca,dst);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* NOT.W (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_4678_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(dst)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(dst)) < 0);
	put_word (srca,dst);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* NOT.W (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_4679_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(dst)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(dst)) < 0);
	put_word (srca,dst);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* NOT.L Dn */
unsigned long REGPARAM2 CPUFUNC(op_4680_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uae_u32 dst = ~src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(dst)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(dst)) < 0);
	m68k_dreg(regs, srcreg) = (dst);
}}}	m68k_incpc (regs, 2);
return 6 * CYCLE_UNIT / 2;
}
/* NOT.L (An) */
unsigned long REGPARAM2 CPUFUNC(op_4690_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(dst)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(dst)) < 0);
	put_long (srca,dst);
}}}}	m68k_incpc (regs, 2);
return 20 * CYCLE_UNIT / 2;
}
/* NOT.L (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_4698_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg(regs, srcreg) += 4;
{	uae_u32 dst = ~src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(dst)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(dst)) < 0);
	put_long (srca,dst);
}}}}	m68k_incpc (regs, 2);
return 20 * CYCLE_UNIT / 2;
}
/* NOT.L -(An) */
unsigned long REGPARAM2 CPUFUNC(op_46a0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_u32 dst = ~src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(dst)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(dst)) < 0);
	put_long (srca,dst);
}}}}	m68k_incpc (regs, 2);
return 22 * CYCLE_UNIT / 2;
}
/* NOT.L (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_46a8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(dst)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(dst)) < 0);
	put_long (srca,dst);
}}}}	m68k_incpc (regs, 4);
return 24 * CYCLE_UNIT / 2;
}
/* NOT.L (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_46b0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s32 src = get_long (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(dst)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(dst)) < 0);
	put_long (srca,dst);
}}}}	m68k_incpc (regs, 4);
return 26 * CYCLE_UNIT / 2;
}
/* NOT.L (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_46b8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(dst)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(dst)) < 0);
	put_long (srca,dst);
}}}}	m68k_incpc (regs, 4);
return 24 * CYCLE_UNIT / 2;
}
/* NOT.L (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_46b9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s32 src = get_long (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(dst)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(dst)) < 0);
	put_long (srca,dst);
}}}}	m68k_incpc (regs, 6);
return 28 * CYCLE_UNIT / 2;
}
/* MV2SR.W Dn */
unsigned long REGPARAM2 CPUFUNC(op_46c0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs->s) { Exception(8, regs, 0); return 34 * CYCLE_UNIT / 2; }
{{	uae_s16 src = m68k_dreg(regs, srcreg);
	regs->sr = src;
	MakeFromSR(regs);
}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* MV2SR.W (An) */
unsigned long REGPARAM2 CPUFUNC(op_46d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs->s) { Exception(8, regs, 0); return 34 * CYCLE_UNIT / 2; }
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
	regs->sr = src;
	MakeFromSR(regs);
}}}}	m68k_incpc (regs, 2);
return 16 * CYCLE_UNIT / 2;
}
/* MV2SR.W (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_46d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs->s) { Exception(8, regs, 0); return 34 * CYCLE_UNIT / 2; }
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg(regs, srcreg) += 2;
	regs->sr = src;
	MakeFromSR(regs);
}}}}	m68k_incpc (regs, 2);
return 16 * CYCLE_UNIT / 2;
}
/* MV2SR.W -(An) */
unsigned long REGPARAM2 CPUFUNC(op_46e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs->s) { Exception(8, regs, 0); return 34 * CYCLE_UNIT / 2; }
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
	regs->sr = src;
	MakeFromSR(regs);
}}}}	m68k_incpc (regs, 2);
return 18 * CYCLE_UNIT / 2;
}
/* MV2SR.W (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_46e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs->s) { Exception(8, regs, 0); return 34 * CYCLE_UNIT / 2; }
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
	regs->sr = src;
	MakeFromSR(regs);
}}}}	m68k_incpc (regs, 4);
return 20 * CYCLE_UNIT / 2;
}
/* MV2SR.W (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_46f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs->s) { Exception(8, regs, 0); return 34 * CYCLE_UNIT / 2; }
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
	regs->sr = src;
	MakeFromSR(regs);
}}}}	m68k_incpc (regs, 4);
return 22 * CYCLE_UNIT / 2;
}
/* MV2SR.W (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_46f8_4)(uae_u32 opcode, struct regstruct *regs)
{
{if (!regs->s) { Exception(8, regs, 0); return 34 * CYCLE_UNIT / 2; }
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
	regs->sr = src;
	MakeFromSR(regs);
}}}}	m68k_incpc (regs, 4);
return 20 * CYCLE_UNIT / 2;
}
/* MV2SR.W (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_46f9_4)(uae_u32 opcode, struct regstruct *regs)
{
{if (!regs->s) { Exception(8, regs, 0); return 34 * CYCLE_UNIT / 2; }
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s16 src = get_word (srca);
	regs->sr = src;
	MakeFromSR(regs);
}}}}	m68k_incpc (regs, 6);
return 24 * CYCLE_UNIT / 2;
}
/* MV2SR.W (d16,PC) */
unsigned long REGPARAM2 CPUFUNC(op_46fa_4)(uae_u32 opcode, struct regstruct *regs)
{
{if (!regs->s) { Exception(8, regs, 0); return 34 * CYCLE_UNIT / 2; }
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
	regs->sr = src;
	MakeFromSR(regs);
}}}}	m68k_incpc (regs, 4);
return 20 * CYCLE_UNIT / 2;
}
/* MV2SR.W (d8,PC,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_46fb_4)(uae_u32 opcode, struct regstruct *regs)
{
{if (!regs->s) { Exception(8, regs, 0); return 34 * CYCLE_UNIT / 2; }
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
	regs->sr = src;
	MakeFromSR(regs);
}}}}	m68k_incpc (regs, 4);
return 22 * CYCLE_UNIT / 2;
}
/* MV2SR.W #<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_46fc_4)(uae_u32 opcode, struct regstruct *regs)
{
{if (!regs->s) { Exception(8, regs, 0); return 34 * CYCLE_UNIT / 2; }
{{	uae_s16 src = get_iword (regs, 2);
	regs->sr = src;
	MakeFromSR(regs);
}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* NBCD.B Dn */
unsigned long REGPARAM2 CPUFUNC(op_4800_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uae_u16 newv_lo = - (src & 0xF) - (GET_XFLG (&regs->ccrflags) ? 1 : 0);
	uae_u16 newv_hi = - (src & 0xF0);
	uae_u16 newv;
	int cflg;
	if (newv_lo > 9) { newv_lo -= 6; }
	newv = newv_hi + newv_lo;
	cflg = (newv & 0x1F0) > 0x90;
	if (cflg) newv -= 0x60;
	SET_CFLG (&regs->ccrflags, cflg);
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s8)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s8)(newv)) < 0);
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((newv) & 0xff);
}}}	m68k_incpc (regs, 2);
return 6 * CYCLE_UNIT / 2;
}
/* NBCD.B (An) */
unsigned long REGPARAM2 CPUFUNC(op_4810_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s8 src = get_byte (srca);
{	uae_u16 newv_lo = - (src & 0xF) - (GET_XFLG (&regs->ccrflags) ? 1 : 0);
	uae_u16 newv_hi = - (src & 0xF0);
	uae_u16 newv;
	int cflg;
	if (newv_lo > 9) { newv_lo -= 6; }
	newv = newv_hi + newv_lo;
	cflg = (newv & 0x1F0) > 0x90;
	if (cflg) newv -= 0x60;
	SET_CFLG (&regs->ccrflags, cflg);
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s8)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s8)(newv)) < 0);
	put_byte (srca,newv);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* NBCD.B (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_4818_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
{	uae_u16 newv_lo = - (src & 0xF) - (GET_XFLG (&regs->ccrflags) ? 1 : 0);
	uae_u16 newv_hi = - (src & 0xF0);
	uae_u16 newv;
	int cflg;
	if (newv_lo > 9) { newv_lo -= 6; }
	newv = newv_hi + newv_lo;
	cflg = (newv & 0x1F0) > 0x90;
	if (cflg) newv -= 0x60;
	SET_CFLG (&regs->ccrflags, cflg);
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s8)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s8)(newv)) < 0);
	put_byte (srca,newv);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* NBCD.B -(An) */
unsigned long REGPARAM2 CPUFUNC(op_4820_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_u16 newv_lo = - (src & 0xF) - (GET_XFLG (&regs->ccrflags) ? 1 : 0);
	uae_u16 newv_hi = - (src & 0xF0);
	uae_u16 newv;
	int cflg;
	if (newv_lo > 9) { newv_lo -= 6; }
	newv = newv_hi + newv_lo;
	cflg = (newv & 0x1F0) > 0x90;
	if (cflg) newv -= 0x60;
	SET_CFLG (&regs->ccrflags, cflg);
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s8)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s8)(newv)) < 0);
	put_byte (srca,newv);
}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* NBCD.B (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_4828_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uae_u16 newv_lo = - (src & 0xF) - (GET_XFLG (&regs->ccrflags) ? 1 : 0);
	uae_u16 newv_hi = - (src & 0xF0);
	uae_u16 newv;
	int cflg;
	if (newv_lo > 9) { newv_lo -= 6; }
	newv = newv_hi + newv_lo;
	cflg = (newv & 0x1F0) > 0x90;
	if (cflg) newv -= 0x60;
	SET_CFLG (&regs->ccrflags, cflg);
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s8)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s8)(newv)) < 0);
	put_byte (srca,newv);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* NBCD.B (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_4830_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s8 src = get_byte (srca);
{	uae_u16 newv_lo = - (src & 0xF) - (GET_XFLG (&regs->ccrflags) ? 1 : 0);
	uae_u16 newv_hi = - (src & 0xF0);
	uae_u16 newv;
	int cflg;
	if (newv_lo > 9) { newv_lo -= 6; }
	newv = newv_hi + newv_lo;
	cflg = (newv & 0x1F0) > 0x90;
	if (cflg) newv -= 0x60;
	SET_CFLG (&regs->ccrflags, cflg);
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s8)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s8)(newv)) < 0);
	put_byte (srca,newv);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* NBCD.B (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_4838_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uae_u16 newv_lo = - (src & 0xF) - (GET_XFLG (&regs->ccrflags) ? 1 : 0);
	uae_u16 newv_hi = - (src & 0xF0);
	uae_u16 newv;
	int cflg;
	if (newv_lo > 9) { newv_lo -= 6; }
	newv = newv_hi + newv_lo;
	cflg = (newv & 0x1F0) > 0x90;
	if (cflg) newv -= 0x60;
	SET_CFLG (&regs->ccrflags, cflg);
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s8)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s8)(newv)) < 0);
	put_byte (srca,newv);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* NBCD.B (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_4839_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uae_u16 newv_lo = - (src & 0xF) - (GET_XFLG (&regs->ccrflags) ? 1 : 0);
	uae_u16 newv_hi = - (src & 0xF0);
	uae_u16 newv;
	int cflg;
	if (newv_lo > 9) { newv_lo -= 6; }
	newv = newv_hi + newv_lo;
	cflg = (newv & 0x1F0) > 0x90;
	if (cflg) newv -= 0x60;
	SET_CFLG (&regs->ccrflags, cflg);
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s8)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s8)(newv)) < 0);
	put_byte (srca,newv);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* SWAP.W Dn */
unsigned long REGPARAM2 CPUFUNC(op_4840_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uae_u32 dst = ((src >> 16)&0xFFFF) | ((src&0xFFFF)<<16);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(dst)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(dst)) < 0);
	m68k_dreg(regs, srcreg) = (dst);
}}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* PEA.L (An) */
unsigned long REGPARAM2 CPUFUNC(op_4850_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg(regs, 7) - 4;
	m68k_areg (regs, 7) = dsta;
	put_long (dsta,srca);
}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* PEA.L (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_4868_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uaecptr dsta;
	dsta = m68k_areg(regs, 7) - 4;
	m68k_areg (regs, 7) = dsta;
	put_long (dsta,srca);
}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* PEA.L (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_4870_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uaecptr dsta;
	dsta = m68k_areg(regs, 7) - 4;
	m68k_areg (regs, 7) = dsta;
	put_long (dsta,srca);
}}}	m68k_incpc (regs, 4);
return 20 * CYCLE_UNIT / 2;
}
/* PEA.L (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_4878_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uaecptr dsta;
	dsta = m68k_areg(regs, 7) - 4;
	m68k_areg (regs, 7) = dsta;
	put_long (dsta,srca);
}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* PEA.L (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_4879_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
{	uaecptr dsta;
	dsta = m68k_areg(regs, 7) - 4;
	m68k_areg (regs, 7) = dsta;
	put_long (dsta,srca);
}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* PEA.L (d16,PC) */
unsigned long REGPARAM2 CPUFUNC(op_487a_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uaecptr dsta;
	dsta = m68k_areg(regs, 7) - 4;
	m68k_areg (regs, 7) = dsta;
	put_long (dsta,srca);
}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* PEA.L (d8,PC,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_487b_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uaecptr dsta;
	dsta = m68k_areg(regs, 7) - 4;
	m68k_areg (regs, 7) = dsta;
	put_long (dsta,srca);
}}}	m68k_incpc (regs, 4);
return 20 * CYCLE_UNIT / 2;
}
/* EXT.W Dn */
unsigned long REGPARAM2 CPUFUNC(op_4880_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uae_u16 dst = (uae_s16)(uae_s8)src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(dst)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(dst)) < 0);
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | ((dst) & 0xffff);
}}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* MVMLE.W #<data>.W,(An) */
unsigned long REGPARAM2 CPUFUNC(op_4890_4)(uae_u32 opcode, struct regstruct *regs)
{
  uae_u32 writecycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_iword (regs, 2);
{	uaecptr srca = m68k_areg(regs, dstreg);
{	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) { put_word(srca, m68k_dreg(regs, movem_index1[dmask])); srca += 2; dmask = movem_next[dmask]; writecycles += 4; }
	while (amask) { put_word(srca, m68k_areg(regs, movem_index1[amask])); srca += 2; amask = movem_next[amask]; writecycles += 4; }
}}}	m68k_incpc (regs, 4);
return (writecycles + 8) * CYCLE_UNIT / 2;
}
/* MVMLE.W #<data>.W,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_48a0_4)(uae_u32 opcode, struct regstruct *regs)
{
  uae_u32 writecycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_iword (regs, 2);
{	uaecptr srca;
	srca = m68k_areg(regs, dstreg) - 0;
{	uae_u16 amask = mask & 0xff, dmask = (mask >> 8) & 0xff;
	int type = get_cpu_model() >= 68020;
	while (amask) {
		srca -= 2;
		if (type) m68k_areg(regs, dstreg) = srca;
		put_word(srca, m68k_areg(regs, movem_index2[amask]));
		amask = movem_next[amask];
		writecycles += 4;
	}
	while (dmask) { srca -= 2; put_word(srca, m68k_dreg(regs, movem_index2[dmask])); dmask = movem_next[dmask]; writecycles += 4; }
	m68k_areg(regs, dstreg) = srca;
}}}	m68k_incpc (regs, 4);
return (writecycles + 8) * CYCLE_UNIT / 2;
}
/* MVMLE.W #<data>.W,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_48a8_4)(uae_u32 opcode, struct regstruct *regs)
{
  uae_u32 writecycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_iword (regs, 2);
{	uaecptr srca = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 4);
{	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) { put_word(srca, m68k_dreg(regs, movem_index1[dmask])); srca += 2; dmask = movem_next[dmask]; writecycles += 4; }
	while (amask) { put_word(srca, m68k_areg(regs, movem_index1[amask])); srca += 2; amask = movem_next[amask]; writecycles += 4; }
}}}	m68k_incpc (regs, 6);
return (writecycles + 12) * CYCLE_UNIT / 2;
}
/* MVMLE.W #<data>.W,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_48b0_4)(uae_u32 opcode, struct regstruct *regs)
{
  uae_u32 writecycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_iword (regs, 2);
{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 4));
{	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) { put_word(srca, m68k_dreg(regs, movem_index1[dmask])); srca += 2; dmask = movem_next[dmask]; writecycles += 4; }
	while (amask) { put_word(srca, m68k_areg(regs, movem_index1[amask])); srca += 2; amask = movem_next[amask]; writecycles += 4; }
}}}	m68k_incpc (regs, 6);
return (writecycles + 14) * CYCLE_UNIT / 2;
}
/* MVMLE.W #<data>.W,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_48b8_4)(uae_u32 opcode, struct regstruct *regs)
{
  uae_u32 writecycles = 0;
{	uae_u16 mask = get_iword (regs, 2);
{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 4);
{	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) { put_word(srca, m68k_dreg(regs, movem_index1[dmask])); srca += 2; dmask = movem_next[dmask]; writecycles += 4; }
	while (amask) { put_word(srca, m68k_areg(regs, movem_index1[amask])); srca += 2; amask = movem_next[amask]; writecycles += 4; }
}}}	m68k_incpc (regs, 6);
return (writecycles + 12) * CYCLE_UNIT / 2;
}
/* MVMLE.W #<data>.W,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_48b9_4)(uae_u32 opcode, struct regstruct *regs)
{
  uae_u32 writecycles = 0;
{	uae_u16 mask = get_iword (regs, 2);
{	uaecptr srca = get_ilong (regs, 4);
{	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) { put_word(srca, m68k_dreg(regs, movem_index1[dmask])); srca += 2; dmask = movem_next[dmask]; writecycles += 4; }
	while (amask) { put_word(srca, m68k_areg(regs, movem_index1[amask])); srca += 2; amask = movem_next[amask]; writecycles += 4; }
}}}	m68k_incpc (regs, 8);
return (writecycles + 16) * CYCLE_UNIT / 2;
}
/* EXT.L Dn */
unsigned long REGPARAM2 CPUFUNC(op_48c0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uae_u32 dst = (uae_s32)(uae_s16)src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(dst)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(dst)) < 0);
	m68k_dreg(regs, srcreg) = (dst);
}}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* MVMLE.L #<data>.W,(An) */
unsigned long REGPARAM2 CPUFUNC(op_48d0_4)(uae_u32 opcode, struct regstruct *regs)
{
  uae_u32 writecycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_iword (regs, 2);
{	uaecptr srca = m68k_areg(regs, dstreg);
{	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) { put_long(srca, m68k_dreg(regs, movem_index1[dmask])); srca += 4; dmask = movem_next[dmask]; writecycles += 8; }
	while (amask) { put_long(srca, m68k_areg(regs, movem_index1[amask])); srca += 4; amask = movem_next[amask]; writecycles += 8; }
}}}	m68k_incpc (regs, 4);
return (writecycles + 8) * CYCLE_UNIT / 2;
}
/* MVMLE.L #<data>.W,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_48e0_4)(uae_u32 opcode, struct regstruct *regs)
{
  uae_u32 writecycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_iword (regs, 2);
{	uaecptr srca;
	srca = m68k_areg(regs, dstreg) - 0;
{	uae_u16 amask = mask & 0xff, dmask = (mask >> 8) & 0xff;
	int type = get_cpu_model() >= 68020;
	while (amask) {
		srca -= 4;
		if (type) m68k_areg(regs, dstreg) = srca;
		put_long(srca, m68k_areg(regs, movem_index2[amask]));
		amask = movem_next[amask];
		writecycles += 8;
	}
	while (dmask) { srca -= 4; put_long(srca, m68k_dreg(regs, movem_index2[dmask])); dmask = movem_next[dmask]; writecycles += 8; }
	m68k_areg(regs, dstreg) = srca;
}}}	m68k_incpc (regs, 4);
return (writecycles + 8) * CYCLE_UNIT / 2;
}
/* MVMLE.L #<data>.W,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_48e8_4)(uae_u32 opcode, struct regstruct *regs)
{
  uae_u32 writecycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_iword (regs, 2);
{	uaecptr srca = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 4);
{	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) { put_long(srca, m68k_dreg(regs, movem_index1[dmask])); srca += 4; dmask = movem_next[dmask]; writecycles += 8; }
	while (amask) { put_long(srca, m68k_areg(regs, movem_index1[amask])); srca += 4; amask = movem_next[amask]; writecycles += 8; }
}}}	m68k_incpc (regs, 6);
return (writecycles + 12) * CYCLE_UNIT / 2;
}
/* MVMLE.L #<data>.W,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_48f0_4)(uae_u32 opcode, struct regstruct *regs)
{
  uae_u32 writecycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_iword (regs, 2);
{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 4));
{	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) { put_long(srca, m68k_dreg(regs, movem_index1[dmask])); srca += 4; dmask = movem_next[dmask]; writecycles += 8; }
	while (amask) { put_long(srca, m68k_areg(regs, movem_index1[amask])); srca += 4; amask = movem_next[amask]; writecycles += 8; }
}}}	m68k_incpc (regs, 6);
return (writecycles + 14) * CYCLE_UNIT / 2;
}
/* MVMLE.L #<data>.W,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_48f8_4)(uae_u32 opcode, struct regstruct *regs)
{
  uae_u32 writecycles = 0;
{	uae_u16 mask = get_iword (regs, 2);
{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 4);
{	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) { put_long(srca, m68k_dreg(regs, movem_index1[dmask])); srca += 4; dmask = movem_next[dmask]; writecycles += 8; }
	while (amask) { put_long(srca, m68k_areg(regs, movem_index1[amask])); srca += 4; amask = movem_next[amask]; writecycles += 8; }
}}}	m68k_incpc (regs, 6);
return (writecycles + 12) * CYCLE_UNIT / 2;
}
/* MVMLE.L #<data>.W,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_48f9_4)(uae_u32 opcode, struct regstruct *regs)
{
  uae_u32 writecycles = 0;
{	uae_u16 mask = get_iword (regs, 2);
{	uaecptr srca = get_ilong (regs, 4);
{	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) { put_long(srca, m68k_dreg(regs, movem_index1[dmask])); srca += 4; dmask = movem_next[dmask]; writecycles += 8; }
	while (amask) { put_long(srca, m68k_areg(regs, movem_index1[amask])); srca += 4; amask = movem_next[amask]; writecycles += 8; }
}}}	m68k_incpc (regs, 8);
return (writecycles + 16) * CYCLE_UNIT / 2;
}
/* TST.B Dn */
unsigned long REGPARAM2 CPUFUNC(op_4a00_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* TST.B (An) */
unsigned long REGPARAM2 CPUFUNC(op_4a10_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s8 src = get_byte (srca);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* TST.B (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_4a18_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* TST.B -(An) */
unsigned long REGPARAM2 CPUFUNC(op_4a20_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
}}}	m68k_incpc (regs, 2);
return 10 * CYCLE_UNIT / 2;
}
/* TST.B (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_4a28_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* TST.B (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_4a30_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s8 src = get_byte (srca);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
}}}	m68k_incpc (regs, 4);
return 14 * CYCLE_UNIT / 2;
}
/* TST.B (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_4a38_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* TST.B (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_4a39_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s8 src = get_byte (srca);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
}}}	m68k_incpc (regs, 6);
return 16 * CYCLE_UNIT / 2;
}
/* TST.W Dn */
unsigned long REGPARAM2 CPUFUNC(op_4a40_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg(regs, srcreg);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* TST.W (An) */
unsigned long REGPARAM2 CPUFUNC(op_4a50_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* TST.W (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_4a58_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg(regs, srcreg) += 2;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* TST.W -(An) */
unsigned long REGPARAM2 CPUFUNC(op_4a60_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
}}}	m68k_incpc (regs, 2);
return 10 * CYCLE_UNIT / 2;
}
/* TST.W (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_4a68_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* TST.W (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_4a70_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
}}}	m68k_incpc (regs, 4);
return 14 * CYCLE_UNIT / 2;
}
/* TST.W (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_4a78_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* TST.W (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_4a79_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s16 src = get_word (srca);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
}}}	m68k_incpc (regs, 6);
return 16 * CYCLE_UNIT / 2;
}
/* TST.L Dn */
unsigned long REGPARAM2 CPUFUNC(op_4a80_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = m68k_dreg(regs, srcreg);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* TST.L (An) */
unsigned long REGPARAM2 CPUFUNC(op_4a90_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* TST.L (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_4a98_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg(regs, srcreg) += 4;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* TST.L -(An) */
unsigned long REGPARAM2 CPUFUNC(op_4aa0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* TST.L (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_4aa8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* TST.L (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_4ab0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s32 src = get_long (srca);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* TST.L (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_4ab8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* TST.L (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_4ab9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s32 src = get_long (srca);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* TAS.B Dn */
unsigned long REGPARAM2 CPUFUNC(op_4ac0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	src |= 0x80;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((src) & 0xff);
}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* TAS.B (An) */
unsigned long REGPARAM2 CPUFUNC(op_4ad0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s8 src = get_byte (srca);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	src |= 0x80;
	put_byte (srca,src);
}}}	m68k_incpc (regs, 2);
return 18 * CYCLE_UNIT / 2;
}
/* TAS.B (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_4ad8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	src |= 0x80;
	put_byte (srca,src);
}}}	m68k_incpc (regs, 2);
return 18 * CYCLE_UNIT / 2;
}
/* TAS.B -(An) */
unsigned long REGPARAM2 CPUFUNC(op_4ae0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	src |= 0x80;
	put_byte (srca,src);
}}}	m68k_incpc (regs, 2);
return 20 * CYCLE_UNIT / 2;
}
/* TAS.B (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_4ae8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	src |= 0x80;
	put_byte (srca,src);
}}}	m68k_incpc (regs, 4);
return 22 * CYCLE_UNIT / 2;
}
/* TAS.B (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_4af0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s8 src = get_byte (srca);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	src |= 0x80;
	put_byte (srca,src);
}}}	m68k_incpc (regs, 4);
return 24 * CYCLE_UNIT / 2;
}
/* TAS.B (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_4af8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	src |= 0x80;
	put_byte (srca,src);
}}}	m68k_incpc (regs, 4);
return 22 * CYCLE_UNIT / 2;
}
/* TAS.B (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_4af9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s8 src = get_byte (srca);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	src |= 0x80;
	put_byte (srca,src);
}}}	m68k_incpc (regs, 6);
return 26 * CYCLE_UNIT / 2;
}
/* MVMEL.W #<data>.W,(An) */
unsigned long REGPARAM2 CPUFUNC(op_4c90_4)(uae_u32 opcode, struct regstruct *regs)
{
  uae_u32 readcycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_iword (regs, 2);
	unsigned int dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca = m68k_areg(regs, dstreg);
{	while (dmask) { m68k_dreg(regs, movem_index1[dmask]) = (uae_s32)(uae_s16)get_word(srca); srca += 2; dmask = movem_next[dmask]; readcycles += 4; }
	while (amask) { m68k_areg(regs, movem_index1[amask]) = (uae_s32)(uae_s16)get_word(srca); srca += 2; amask = movem_next[amask]; readcycles += 4; }
}}}	m68k_incpc (regs, 4);
return (readcycles + 12) * CYCLE_UNIT / 2;
}
/* MVMEL.W #<data>.W,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_4c98_4)(uae_u32 opcode, struct regstruct *regs)
{
  uae_u32 readcycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_iword (regs, 2);
	unsigned int dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca = m68k_areg(regs, dstreg);
{	while (dmask) { m68k_dreg(regs, movem_index1[dmask]) = (uae_s32)(uae_s16)get_word(srca); srca += 2; dmask = movem_next[dmask]; readcycles += 4; }
	while (amask) { m68k_areg(regs, movem_index1[amask]) = (uae_s32)(uae_s16)get_word(srca); srca += 2; amask = movem_next[amask]; readcycles += 4; }
	m68k_areg(regs, dstreg) = srca;
}}}	m68k_incpc (regs, 4);
return (readcycles + 12) * CYCLE_UNIT / 2;
}
/* MVMEL.W #<data>.W,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_4ca8_4)(uae_u32 opcode, struct regstruct *regs)
{
  uae_u32 readcycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_iword (regs, 2);
	unsigned int dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 4);
{	while (dmask) { m68k_dreg(regs, movem_index1[dmask]) = (uae_s32)(uae_s16)get_word(srca); srca += 2; dmask = movem_next[dmask]; readcycles += 4; }
	while (amask) { m68k_areg(regs, movem_index1[amask]) = (uae_s32)(uae_s16)get_word(srca); srca += 2; amask = movem_next[amask]; readcycles += 4; }
}}}	m68k_incpc (regs, 6);
return (readcycles + 16) * CYCLE_UNIT / 2;
}
/* MVMEL.W #<data>.W,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_4cb0_4)(uae_u32 opcode, struct regstruct *regs)
{
  uae_u32 readcycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_iword (regs, 2);
	unsigned int dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 4));
{	while (dmask) { m68k_dreg(regs, movem_index1[dmask]) = (uae_s32)(uae_s16)get_word(srca); srca += 2; dmask = movem_next[dmask]; readcycles += 4; }
	while (amask) { m68k_areg(regs, movem_index1[amask]) = (uae_s32)(uae_s16)get_word(srca); srca += 2; amask = movem_next[amask]; readcycles += 4; }
}}}	m68k_incpc (regs, 6);
return (readcycles + 18) * CYCLE_UNIT / 2;
}
/* MVMEL.W #<data>.W,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_4cb8_4)(uae_u32 opcode, struct regstruct *regs)
{
  uae_u32 readcycles = 0;
{	uae_u16 mask = get_iword (regs, 2);
	unsigned int dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 4);
{	while (dmask) { m68k_dreg(regs, movem_index1[dmask]) = (uae_s32)(uae_s16)get_word(srca); srca += 2; dmask = movem_next[dmask]; readcycles += 4; }
	while (amask) { m68k_areg(regs, movem_index1[amask]) = (uae_s32)(uae_s16)get_word(srca); srca += 2; amask = movem_next[amask]; readcycles += 4; }
}}}	m68k_incpc (regs, 6);
return (readcycles + 16) * CYCLE_UNIT / 2;
}
/* MVMEL.W #<data>.W,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_4cb9_4)(uae_u32 opcode, struct regstruct *regs)
{
  uae_u32 readcycles = 0;
{	uae_u16 mask = get_iword (regs, 2);
	unsigned int dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca = get_ilong (regs, 4);
{	while (dmask) { m68k_dreg(regs, movem_index1[dmask]) = (uae_s32)(uae_s16)get_word(srca); srca += 2; dmask = movem_next[dmask]; readcycles += 4; }
	while (amask) { m68k_areg(regs, movem_index1[amask]) = (uae_s32)(uae_s16)get_word(srca); srca += 2; amask = movem_next[amask]; readcycles += 4; }
}}}	m68k_incpc (regs, 8);
return (readcycles + 20) * CYCLE_UNIT / 2;
}
/* MVMEL.W #<data>.W,(d16,PC) */
unsigned long REGPARAM2 CPUFUNC(op_4cba_4)(uae_u32 opcode, struct regstruct *regs)
{
  uae_u32 readcycles = 0;
	uae_u32 dstreg = 2;
{	uae_u16 mask = get_iword (regs, 2);
	unsigned int dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca = m68k_getpc (regs) + 4;
	srca += (uae_s32)(uae_s16)get_iword (regs, 4);
{	while (dmask) { m68k_dreg(regs, movem_index1[dmask]) = (uae_s32)(uae_s16)get_word(srca); srca += 2; dmask = movem_next[dmask]; readcycles += 4; }
	while (amask) { m68k_areg(regs, movem_index1[amask]) = (uae_s32)(uae_s16)get_word(srca); srca += 2; amask = movem_next[amask]; readcycles += 4; }
}}}	m68k_incpc (regs, 6);
return (readcycles + 16) * CYCLE_UNIT / 2;
}
/* MVMEL.W #<data>.W,(d8,PC,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_4cbb_4)(uae_u32 opcode, struct regstruct *regs)
{
  uae_u32 readcycles = 0;
	uae_u32 dstreg = 3;
{	uae_u16 mask = get_iword (regs, 2);
	unsigned int dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 4;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 4));
{	while (dmask) { m68k_dreg(regs, movem_index1[dmask]) = (uae_s32)(uae_s16)get_word(srca); srca += 2; dmask = movem_next[dmask]; readcycles += 4; }
	while (amask) { m68k_areg(regs, movem_index1[amask]) = (uae_s32)(uae_s16)get_word(srca); srca += 2; amask = movem_next[amask]; readcycles += 4; }
}}}	m68k_incpc (regs, 6);
return (readcycles + 18) * CYCLE_UNIT / 2;
}
/* MVMEL.L #<data>.W,(An) */
unsigned long REGPARAM2 CPUFUNC(op_4cd0_4)(uae_u32 opcode, struct regstruct *regs)
{
  uae_u32 readcycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_iword (regs, 2);
	unsigned int dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca = m68k_areg(regs, dstreg);
{	while (dmask) { m68k_dreg(regs, movem_index1[dmask]) = get_long(srca); srca += 4; dmask = movem_next[dmask]; readcycles += 8; }
	while (amask) { m68k_areg(regs, movem_index1[amask]) = get_long(srca); srca += 4; amask = movem_next[amask]; readcycles += 8; }
}}}	m68k_incpc (regs, 4);
return (readcycles + 12) * CYCLE_UNIT / 2;
}
/* MVMEL.L #<data>.W,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_4cd8_4)(uae_u32 opcode, struct regstruct *regs)
{
  uae_u32 readcycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_iword (regs, 2);
	unsigned int dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca = m68k_areg(regs, dstreg);
{	while (dmask) { m68k_dreg(regs, movem_index1[dmask]) = get_long(srca); srca += 4; dmask = movem_next[dmask]; readcycles += 8; }
	while (amask) { m68k_areg(regs, movem_index1[amask]) = get_long(srca); srca += 4; amask = movem_next[amask]; readcycles += 8; }
	m68k_areg(regs, dstreg) = srca;
}}}	m68k_incpc (regs, 4);
return (readcycles + 12) * CYCLE_UNIT / 2;
}
/* MVMEL.L #<data>.W,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_4ce8_4)(uae_u32 opcode, struct regstruct *regs)
{
  uae_u32 readcycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_iword (regs, 2);
	unsigned int dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 4);
{	while (dmask) { m68k_dreg(regs, movem_index1[dmask]) = get_long(srca); srca += 4; dmask = movem_next[dmask]; readcycles += 8; }
	while (amask) { m68k_areg(regs, movem_index1[amask]) = get_long(srca); srca += 4; amask = movem_next[amask]; readcycles += 8; }
}}}	m68k_incpc (regs, 6);
return (readcycles + 16) * CYCLE_UNIT / 2;
}
/* MVMEL.L #<data>.W,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_4cf0_4)(uae_u32 opcode, struct regstruct *regs)
{
  uae_u32 readcycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_iword (regs, 2);
	unsigned int dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 4));
{	while (dmask) { m68k_dreg(regs, movem_index1[dmask]) = get_long(srca); srca += 4; dmask = movem_next[dmask]; readcycles += 8; }
	while (amask) { m68k_areg(regs, movem_index1[amask]) = get_long(srca); srca += 4; amask = movem_next[amask]; readcycles += 8; }
}}}	m68k_incpc (regs, 6);
return (readcycles + 18) * CYCLE_UNIT / 2;
}
/* MVMEL.L #<data>.W,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_4cf8_4)(uae_u32 opcode, struct regstruct *regs)
{
  uae_u32 readcycles = 0;
{	uae_u16 mask = get_iword (regs, 2);
	unsigned int dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 4);
{	while (dmask) { m68k_dreg(regs, movem_index1[dmask]) = get_long(srca); srca += 4; dmask = movem_next[dmask]; readcycles += 8; }
	while (amask) { m68k_areg(regs, movem_index1[amask]) = get_long(srca); srca += 4; amask = movem_next[amask]; readcycles += 8; }
}}}	m68k_incpc (regs, 6);
return (readcycles + 16) * CYCLE_UNIT / 2;
}
/* MVMEL.L #<data>.W,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_4cf9_4)(uae_u32 opcode, struct regstruct *regs)
{
  uae_u32 readcycles = 0;
{	uae_u16 mask = get_iword (regs, 2);
	unsigned int dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca = get_ilong (regs, 4);
{	while (dmask) { m68k_dreg(regs, movem_index1[dmask]) = get_long(srca); srca += 4; dmask = movem_next[dmask]; readcycles += 8; }
	while (amask) { m68k_areg(regs, movem_index1[amask]) = get_long(srca); srca += 4; amask = movem_next[amask]; readcycles += 8; }
}}}	m68k_incpc (regs, 8);
return (readcycles + 20) * CYCLE_UNIT / 2;
}
/* MVMEL.L #<data>.W,(d16,PC) */
unsigned long REGPARAM2 CPUFUNC(op_4cfa_4)(uae_u32 opcode, struct regstruct *regs)
{
  uae_u32 readcycles = 0;
	uae_u32 dstreg = 2;
{	uae_u16 mask = get_iword (regs, 2);
	unsigned int dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca = m68k_getpc (regs) + 4;
	srca += (uae_s32)(uae_s16)get_iword (regs, 4);
{	while (dmask) { m68k_dreg(regs, movem_index1[dmask]) = get_long(srca); srca += 4; dmask = movem_next[dmask]; readcycles += 8; }
	while (amask) { m68k_areg(regs, movem_index1[amask]) = get_long(srca); srca += 4; amask = movem_next[amask]; readcycles += 8; }
}}}	m68k_incpc (regs, 6);
return (readcycles + 16) * CYCLE_UNIT / 2;
}
/* MVMEL.L #<data>.W,(d8,PC,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_4cfb_4)(uae_u32 opcode, struct regstruct *regs)
{
  uae_u32 readcycles = 0;
	uae_u32 dstreg = 3;
{	uae_u16 mask = get_iword (regs, 2);
	unsigned int dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 4;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 4));
{	while (dmask) { m68k_dreg(regs, movem_index1[dmask]) = get_long(srca); srca += 4; dmask = movem_next[dmask]; readcycles += 8; }
	while (amask) { m68k_areg(regs, movem_index1[amask]) = get_long(srca); srca += 4; amask = movem_next[amask]; readcycles += 8; }
}}}	m68k_incpc (regs, 6);
return (readcycles + 18) * CYCLE_UNIT / 2;
}
/* TRAP.L #<data> */
unsigned long REGPARAM2 CPUFUNC(op_4e40_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 15);
{{	uae_u32 src = srcreg;
	m68k_incpc (regs, 2);
	Exception (src + 32, regs, 0);
}}return 34 * CYCLE_UNIT / 2;
}
/* LINK.W An,#<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_4e50_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr olda;
	olda = m68k_areg(regs, 7) - 4;
	m68k_areg (regs, 7) = olda;
{	uae_s32 src = m68k_areg(regs, srcreg);
	put_long (olda,src);
	m68k_areg(regs, srcreg) = (m68k_areg(regs, 7));
{	uae_s16 offs = get_iword (regs, 2);
	m68k_areg(regs, 7) += offs;
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* UNLK.L An */
unsigned long REGPARAM2 CPUFUNC(op_4e58_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = m68k_areg(regs, srcreg);
	m68k_areg(regs, 7) = src;
{	uaecptr olda = m68k_areg(regs, 7);
{	uae_s32 old = get_long (olda);
	m68k_areg(regs, 7) += 4;
	m68k_areg(regs, srcreg) = (old);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* MVR2USP.L An */
unsigned long REGPARAM2 CPUFUNC(op_4e60_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs->s) { Exception(8, regs, 0); return 34 * CYCLE_UNIT / 2; }
{{	uae_s32 src = m68k_areg(regs, srcreg);
	regs->usp = src;
}}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* MVUSP2R.L An */
unsigned long REGPARAM2 CPUFUNC(op_4e68_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs->s) { Exception(8, regs, 0); return 34 * CYCLE_UNIT / 2; }
{{	m68k_areg(regs, srcreg) = (regs->usp);
}}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* RESET.L  */
unsigned long REGPARAM2 CPUFUNC(op_4e70_4)(uae_u32 opcode, struct regstruct *regs)
{
{if (!regs->s) { Exception(8, regs, 0); return 34 * CYCLE_UNIT / 2; }
{	cpureset();
}}	m68k_incpc (regs, 2);
return 132 * CYCLE_UNIT / 2;
}
/* NOP.L  */
unsigned long REGPARAM2 CPUFUNC(op_4e71_4)(uae_u32 opcode, struct regstruct *regs)
{
{}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* STOP.L #<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_4e72_4)(uae_u32 opcode, struct regstruct *regs)
{
{if (!regs->s) { Exception(8, regs, 0); return 34 * CYCLE_UNIT / 2; }
{{	uae_s16 src = get_iword (regs, 2);
	regs->sr = src;
	MakeFromSR(regs);
	m68k_setstopped(regs, 1);
	m68k_incpc (regs, 4);
}}}
return 4 * CYCLE_UNIT / 2;
}
/* RTE.L  */
unsigned long REGPARAM2 CPUFUNC(op_4e73_4)(uae_u32 opcode, struct regstruct *regs)
{
{if (!regs->s) { Exception(8, regs, 0); return 34 * CYCLE_UNIT / 2; }
{	uae_u16 newsr; uae_u32 newpc; for (;;) {
{	uaecptr sra = m68k_areg(regs, 7);
{	uae_s16 sr = get_word (sra);
	m68k_areg(regs, 7) += 2;
{	uaecptr pca = m68k_areg(regs, 7);
{	uae_s32 pc = get_long (pca);
	m68k_areg(regs, 7) += 4;
{	uaecptr formata = m68k_areg(regs, 7);
{	uae_s16 format = get_word (formata);
	m68k_areg(regs, 7) += 2;
	newsr = sr; newpc = pc;
	if ((format & 0xF000) == 0x0000) { break; }
	else if ((format & 0xF000) == 0x1000) { ; }
	else if ((format & 0xF000) == 0x2000) { m68k_areg(regs, 7) += 4; break; }
  else if ((format & 0xF000) == 0x4000) { m68k_areg(regs, 7) += 8; break; }
	else if ((format & 0xF000) == 0x7000) { m68k_areg(regs, 7) += 52; break; }
	else if ((format & 0xF000) == 0x8000) { m68k_areg(regs, 7) += 50; break; }
	else if ((format & 0xF000) == 0x9000) { m68k_areg(regs, 7) += 12; break; }
	else if ((format & 0xF000) == 0xa000) { m68k_areg(regs, 7) += 24; break; }
	else if ((format & 0xF000) == 0xb000) { m68k_areg(regs, 7) += 84; break; }
	else { Exception(14, regs, 0); return 50 * CYCLE_UNIT / 2; }
	regs->sr = newsr; MakeFromSR(regs);
}
}}}}}}	regs->sr = newsr; MakeFromSR(regs);
	if (newpc & 1)
	{
		exception3 (0x4E73, m68k_getpc(regs), newpc);
		return 50 * CYCLE_UNIT / 2;
	}
	else
		m68k_setpc(regs, newpc);
}}
return 20 * CYCLE_UNIT / 2;
}
/* RTD.L #<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_4e74_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr pca = m68k_areg(regs, 7);
{	uae_s32 pc = get_long (pca);
	m68k_areg(regs, 7) += 4;
{	uae_s16 offs = get_iword (regs, 2);
	m68k_areg(regs, 7) += offs;
	if (pc & 1)
	{
		exception3 (0x4E74, m68k_getpc(regs), pc);
		return 50 * CYCLE_UNIT / 2;
	}
	else
		m68k_setpc(regs, pc);
}}}}return 10 * CYCLE_UNIT / 2;
}
/* RTS.L  */
unsigned long REGPARAM2 CPUFUNC(op_4e75_4)(uae_u32 opcode, struct regstruct *regs)
{
{	m68k_do_rts(regs);
}return 16 * CYCLE_UNIT / 2;
}
/* TRAPV.L  */
unsigned long REGPARAM2 CPUFUNC(op_4e76_4)(uae_u32 opcode, struct regstruct *regs)
{
{	m68k_incpc (regs, 2);
	if (GET_VFLG (&regs->ccrflags)) {
		Exception (7, regs, m68k_getpc (regs));
		return 34 * CYCLE_UNIT / 2;
	}
}
return 4 * CYCLE_UNIT / 2;
}
/* RTR.L  */
unsigned long REGPARAM2 CPUFUNC(op_4e77_4)(uae_u32 opcode, struct regstruct *regs)
{
{	MakeSR(regs);
{	uaecptr sra = m68k_areg(regs, 7);
{	uae_s16 sr = get_word (sra);
	m68k_areg(regs, 7) += 2;
{	uaecptr pca = m68k_areg(regs, 7);
{	uae_s32 pc = get_long (pca);
	m68k_areg(regs, 7) += 4;
	regs->sr &= 0xFF00; sr &= 0xFF;
	regs->sr |= sr; m68k_setpc(regs, pc);
	MakeFromSR(regs);
}}}}}return 20 * CYCLE_UNIT / 2;
}
/* MOVEC2.L #<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_4e7a_4)(uae_u32 opcode, struct regstruct *regs)
{
{if (!regs->s) { Exception(8, regs, 0); return 34 * CYCLE_UNIT / 2; }
{{	uae_s16 src = get_iword (regs, 2);
{	int regno = (src >> 12) & 15;
	uae_u32 *regp = regs->regs + regno;
	if (! m68k_movec2(src & 0xFFF, regp)) goto endlabel911;
}}}}	m68k_incpc (regs, 4);
endlabel911: ;
return 6 * CYCLE_UNIT / 2;
}
/* MOVE2C.L #<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_4e7b_4)(uae_u32 opcode, struct regstruct *regs)
{
{if (!regs->s) { Exception(8, regs, 0); return 34 * CYCLE_UNIT / 2; }
{{	uae_s16 src = get_iword (regs, 2);
{	int regno = (src >> 12) & 15;
	uae_u32 *regp = regs->regs + regno;
	if (! m68k_move2c(src & 0xFFF, regp)) goto endlabel912;
}}}}	m68k_incpc (regs, 4);
endlabel912: ;
return 12 * CYCLE_UNIT / 2;
}
/* JSR.L (An) */
unsigned long REGPARAM2 CPUFUNC(op_4e90_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uaecptr oldpc = m68k_getpc(regs) + 2;
	if (srca & 1) {
		exception3i (opcode, oldpc, srca);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_setpc (regs, srca);
	m68k_areg (regs, 7) -= 4;
	put_long (m68k_areg (regs, 7), oldpc);
}}}
return 16 * CYCLE_UNIT / 2;
}
/* JSR.L (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_4ea8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uaecptr oldpc = m68k_getpc(regs) + 4;
	if (srca & 1) {
		exception3i (opcode, oldpc, srca);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_setpc (regs, srca);
	m68k_areg (regs, 7) -= 4;
	put_long (m68k_areg (regs, 7), oldpc);
}}}
return 18 * CYCLE_UNIT / 2;
}
/* JSR.L (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_4eb0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uaecptr oldpc = m68k_getpc(regs) + 4;
	if (srca & 1) {
		exception3i (opcode, oldpc, srca);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_setpc (regs, srca);
	m68k_areg (regs, 7) -= 4;
	put_long (m68k_areg (regs, 7), oldpc);
}}}
return 22 * CYCLE_UNIT / 2;
}
/* JSR.L (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_4eb8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uaecptr oldpc = m68k_getpc(regs) + 4;
	if (srca & 1) {
		exception3i (opcode, oldpc, srca);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_setpc (regs, srca);
	m68k_areg (regs, 7) -= 4;
	put_long (m68k_areg (regs, 7), oldpc);
}}}
return 18 * CYCLE_UNIT / 2;
}
/* JSR.L (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_4eb9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
{	uaecptr oldpc = m68k_getpc(regs) + 6;
	if (srca & 1) {
		exception3i (opcode, oldpc, srca);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_setpc (regs, srca);
	m68k_areg (regs, 7) -= 4;
	put_long (m68k_areg (regs, 7), oldpc);
}}}
return 20 * CYCLE_UNIT / 2;
}
/* JSR.L (d16,PC) */
unsigned long REGPARAM2 CPUFUNC(op_4eba_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uaecptr oldpc = m68k_getpc(regs) + 4;
	if (srca & 1) {
		exception3i (opcode, oldpc, srca);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_setpc (regs, srca);
	m68k_areg (regs, 7) -= 4;
	put_long (m68k_areg (regs, 7), oldpc);
}}}
return 18 * CYCLE_UNIT / 2;
}
/* JSR.L (d8,PC,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_4ebb_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uaecptr oldpc = m68k_getpc(regs) + 4;
	if (srca & 1) {
		exception3i (opcode, oldpc, srca);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_setpc (regs, srca);
	m68k_areg (regs, 7) -= 4;
	put_long (m68k_areg (regs, 7), oldpc);
}}}
return 22 * CYCLE_UNIT / 2;
}
/* JMP.L (An) */
unsigned long REGPARAM2 CPUFUNC(op_4ed0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
	if (srca & 1) {
		exception3i (opcode, m68k_getpc(regs) + 6, srca);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_setpc(regs, srca);
}}
return 8 * CYCLE_UNIT / 2;
}
/* JMP.L (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_4ee8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
	if (srca & 1) {
		exception3i (opcode, m68k_getpc(regs) + 6, srca);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_setpc(regs, srca);
}}
return 10 * CYCLE_UNIT / 2;
}
/* JMP.L (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_4ef0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
	if (srca & 1) {
		exception3i (opcode, m68k_getpc(regs) + 6, srca);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_setpc(regs, srca);
}}
return 14 * CYCLE_UNIT / 2;
}
/* JMP.L (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_4ef8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
	if (srca & 1) {
		exception3i (opcode, m68k_getpc(regs) + 6, srca);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_setpc(regs, srca);
}}
return 10 * CYCLE_UNIT / 2;
}
/* JMP.L (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_4ef9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
	if (srca & 1) {
		exception3i (opcode, m68k_getpc(regs) + 6, srca);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_setpc(regs, srca);
}}
return 12 * CYCLE_UNIT / 2;
}
/* JMP.L (d16,PC) */
unsigned long REGPARAM2 CPUFUNC(op_4efa_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
	if (srca & 1) {
		exception3i (opcode, m68k_getpc(regs) + 6, srca);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_setpc(regs, srca);
}}
return 10 * CYCLE_UNIT / 2;
}
/* JMP.L (d8,PC,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_4efb_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
	if (srca & 1) {
		exception3i (opcode, m68k_getpc(regs) + 6, srca);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_setpc(regs, srca);
}}
return 14 * CYCLE_UNIT / 2;
}
/* ADD.B #<data>,Dn */
unsigned long REGPARAM2 CPUFUNC(op_5000_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* ADD.B #<data>,(An) */
unsigned long REGPARAM2 CPUFUNC(op_5010_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* ADD.B #<data>,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_5018_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* ADD.B #<data>,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_5020_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* ADD.B #<data>,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_5028_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* ADD.B #<data>,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_5030_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 2));
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* ADD.B #<data>,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_5038_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* ADD.B #<data>,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_5039_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta = get_ilong (regs, 2);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* ADD.W #<data>,Dn */
unsigned long REGPARAM2 CPUFUNC(op_5040_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* ADDA.W #<data>,An */
unsigned long REGPARAM2 CPUFUNC(op_5048_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
}}}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* ADD.W #<data>,(An) */
unsigned long REGPARAM2 CPUFUNC(op_5050_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* ADD.W #<data>,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_5058_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s16 dst = get_word (dsta);
	m68k_areg(regs, dstreg) += 2;
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* ADD.W #<data>,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_5060_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* ADD.W #<data>,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_5068_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* ADD.W #<data>,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_5070_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 2));
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* ADD.W #<data>,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_5078_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* ADD.W #<data>,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_5079_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta = get_ilong (regs, 2);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* ADD.L #<data>,Dn */
unsigned long REGPARAM2 CPUFUNC(op_5080_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
}}}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* ADDA.L #<data>,An */
unsigned long REGPARAM2 CPUFUNC(op_5088_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* ADD.L #<data>,(An) */
unsigned long REGPARAM2 CPUFUNC(op_5090_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 20 * CYCLE_UNIT / 2;
}
/* ADD.L #<data>,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_5098_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s32 dst = get_long (dsta);
	m68k_areg(regs, dstreg) += 4;
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 20 * CYCLE_UNIT / 2;
}
/* ADD.L #<data>,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_50a0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 22 * CYCLE_UNIT / 2;
}
/* ADD.L #<data>,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_50a8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 24 * CYCLE_UNIT / 2;
}
/* ADD.L #<data>,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_50b0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 2));
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 26 * CYCLE_UNIT / 2;
}
/* ADD.L #<data>,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_50b8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 24 * CYCLE_UNIT / 2;
}
/* ADD.L #<data>,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_50b9_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta = get_ilong (regs, 2);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 6);
return 28 * CYCLE_UNIT / 2;
}
/* Scc.B Dn */
unsigned long REGPARAM2 CPUFUNC(op_50c0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue(&regs->ccrflags, 0) ? 0xff : 0;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (regs, 2);
return 6 * CYCLE_UNIT / 2;
}
/* DBcc.W Dn,#<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_50c8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uae_s16 offs = get_iword (regs, 2);
	uaecptr oldpc = m68k_getpc(regs);
	if (!cctrue(&regs->ccrflags, 0)) {
		m68k_incpc(regs, (uae_s32)offs + 2);
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src-1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)offs + 2);
				return 50 * CYCLE_UNIT / 2;
			}
			return 10 * CYCLE_UNIT / 2;
		}
	} else {
	}
	m68k_setpc (regs, oldpc + 4);
}}}
return 14 * CYCLE_UNIT / 2;
}
/* Scc.B (An) */
unsigned long REGPARAM2 CPUFUNC(op_50d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{{	int val = cctrue(&regs->ccrflags, 0) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* Scc.B (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_50d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue(&regs->ccrflags, 0) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* Scc.B -(An) */
unsigned long REGPARAM2 CPUFUNC(op_50e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue(&regs->ccrflags, 0) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* Scc.B (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_50e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 0) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* Scc.B (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_50f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{{	int val = cctrue(&regs->ccrflags, 0) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* Scc.B (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_50f8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 0) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* Scc.B (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_50f9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 0) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* SUB.B #<data>,Dn */
unsigned long REGPARAM2 CPUFUNC(op_5100_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* SUB.B #<data>,(An) */
unsigned long REGPARAM2 CPUFUNC(op_5110_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* SUB.B #<data>,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_5118_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* SUB.B #<data>,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_5120_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* SUB.B #<data>,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_5128_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* SUB.B #<data>,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_5130_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 2));
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* SUB.B #<data>,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_5138_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* SUB.B #<data>,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_5139_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta = get_ilong (regs, 2);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* SUB.W #<data>,Dn */
unsigned long REGPARAM2 CPUFUNC(op_5140_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* SUBA.W #<data>,An */
unsigned long REGPARAM2 CPUFUNC(op_5148_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* SUB.W #<data>,(An) */
unsigned long REGPARAM2 CPUFUNC(op_5150_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* SUB.W #<data>,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_5158_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s16 dst = get_word (dsta);
	m68k_areg(regs, dstreg) += 2;
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* SUB.W #<data>,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_5160_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* SUB.W #<data>,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_5168_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* SUB.W #<data>,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_5170_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 2));
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* SUB.W #<data>,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_5178_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* SUB.W #<data>,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_5179_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta = get_ilong (regs, 2);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* SUB.L #<data>,Dn */
unsigned long REGPARAM2 CPUFUNC(op_5180_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
}}}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* SUBA.L #<data>,An */
unsigned long REGPARAM2 CPUFUNC(op_5188_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* SUB.L #<data>,(An) */
unsigned long REGPARAM2 CPUFUNC(op_5190_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 20 * CYCLE_UNIT / 2;
}
/* SUB.L #<data>,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_5198_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s32 dst = get_long (dsta);
	m68k_areg(regs, dstreg) += 4;
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 20 * CYCLE_UNIT / 2;
}
/* SUB.L #<data>,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_51a0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 22 * CYCLE_UNIT / 2;
}
/* SUB.L #<data>,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_51a8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 24 * CYCLE_UNIT / 2;
}
/* SUB.L #<data>,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_51b0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 2));
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 26 * CYCLE_UNIT / 2;
}
/* SUB.L #<data>,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_51b8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 24 * CYCLE_UNIT / 2;
}
/* SUB.L #<data>,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_51b9_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta = get_ilong (regs, 2);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 6);
return 28 * CYCLE_UNIT / 2;
}
/* Scc.B Dn */
unsigned long REGPARAM2 CPUFUNC(op_51c0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue(&regs->ccrflags, 1) ? 0xff : 0;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* DBcc.W Dn,#<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_51c8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uae_s16 offs = get_iword (regs, 2);
	uaecptr oldpc = m68k_getpc(regs);
	if (!cctrue(&regs->ccrflags, 1)) {
		m68k_incpc(regs, (uae_s32)offs + 2);
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src-1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)offs + 2);
				return 50 * CYCLE_UNIT / 2;
			}
			return 10 * CYCLE_UNIT / 2;
		}
	} else {
	}
	m68k_setpc (regs, oldpc + 4);
}}}
return 14 * CYCLE_UNIT / 2;
}
/* Scc.B (An) */
unsigned long REGPARAM2 CPUFUNC(op_51d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{{	int val = cctrue(&regs->ccrflags, 1) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* Scc.B (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_51d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue(&regs->ccrflags, 1) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* Scc.B -(An) */
unsigned long REGPARAM2 CPUFUNC(op_51e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue(&regs->ccrflags, 1) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* Scc.B (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_51e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 1) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* Scc.B (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_51f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{{	int val = cctrue(&regs->ccrflags, 1) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* Scc.B (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_51f8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 1) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* Scc.B (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_51f9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 1) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* Scc.B Dn */
unsigned long REGPARAM2 CPUFUNC(op_52c0_4)(uae_u32 opcode, struct regstruct *regs)
{
  uae_u32 cyc = 4;
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue(&regs->ccrflags, 2) ? 0xff : 0;
  if(val) cyc = 6;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (regs, 2);
return cyc * CYCLE_UNIT / 2;
}
/* DBcc.W Dn,#<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_52c8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uae_s16 offs = get_iword (regs, 2);
	uaecptr oldpc = m68k_getpc(regs);
	if (!cctrue(&regs->ccrflags, 2)) {
		m68k_incpc(regs, (uae_s32)offs + 2);
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src-1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)offs + 2);
				return 50 * CYCLE_UNIT / 2;
			}
			return 10 * CYCLE_UNIT / 2;
		}
	} else {
	}
	m68k_setpc (regs, oldpc + 4);
}}}
return 14 * CYCLE_UNIT / 2;
}
/* Scc.B (An) */
unsigned long REGPARAM2 CPUFUNC(op_52d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{{	int val = cctrue(&regs->ccrflags, 2) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* Scc.B (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_52d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue(&regs->ccrflags, 2) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* Scc.B -(An) */
unsigned long REGPARAM2 CPUFUNC(op_52e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue(&regs->ccrflags, 2) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* Scc.B (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_52e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 2) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* Scc.B (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_52f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{{	int val = cctrue(&regs->ccrflags, 2) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* Scc.B (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_52f8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 2) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* Scc.B (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_52f9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 2) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* Scc.B Dn */
unsigned long REGPARAM2 CPUFUNC(op_53c0_4)(uae_u32 opcode, struct regstruct *regs)
{
  uae_u32 cyc = 4;
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue(&regs->ccrflags, 3) ? 0xff : 0;
  if(val) cyc = 6;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (regs, 2);
return cyc * CYCLE_UNIT / 2;
}
/* DBcc.W Dn,#<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_53c8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uae_s16 offs = get_iword (regs, 2);
	uaecptr oldpc = m68k_getpc(regs);
	if (!cctrue(&regs->ccrflags, 3)) {
		m68k_incpc(regs, (uae_s32)offs + 2);
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src-1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)offs + 2);
				return 50 * CYCLE_UNIT / 2;
			}
			return 10 * CYCLE_UNIT / 2;
		}
	} else {
	}
	m68k_setpc (regs, oldpc + 4);
}}}
return 14 * CYCLE_UNIT / 2;
}
/* Scc.B (An) */
unsigned long REGPARAM2 CPUFUNC(op_53d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{{	int val = cctrue(&regs->ccrflags, 3) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* Scc.B (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_53d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue(&regs->ccrflags, 3) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* Scc.B -(An) */
unsigned long REGPARAM2 CPUFUNC(op_53e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue(&regs->ccrflags, 3) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* Scc.B (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_53e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 3) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* Scc.B (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_53f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{{	int val = cctrue(&regs->ccrflags, 3) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* Scc.B (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_53f8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 3) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* Scc.B (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_53f9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 3) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* Scc.B Dn */
unsigned long REGPARAM2 CPUFUNC(op_54c0_4)(uae_u32 opcode, struct regstruct *regs)
{
  uae_u32 cyc = 4;
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue(&regs->ccrflags, 4) ? 0xff : 0;
  if(val) cyc = 6;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (regs, 2);
return cyc * CYCLE_UNIT / 2;
}
/* DBcc.W Dn,#<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_54c8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uae_s16 offs = get_iword (regs, 2);
	uaecptr oldpc = m68k_getpc(regs);
	if (!cctrue(&regs->ccrflags, 4)) {
		m68k_incpc(regs, (uae_s32)offs + 2);
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src-1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)offs + 2);
				return 50 * CYCLE_UNIT / 2;
			}
			return 10 * CYCLE_UNIT / 2;
		}
	} else {
	}
	m68k_setpc (regs, oldpc + 4);
}}}
return 14 * CYCLE_UNIT / 2;
}
/* Scc.B (An) */
unsigned long REGPARAM2 CPUFUNC(op_54d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{{	int val = cctrue(&regs->ccrflags, 4) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* Scc.B (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_54d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue(&regs->ccrflags, 4) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* Scc.B -(An) */
unsigned long REGPARAM2 CPUFUNC(op_54e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue(&regs->ccrflags, 4) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* Scc.B (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_54e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 4) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* Scc.B (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_54f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{{	int val = cctrue(&regs->ccrflags, 4) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* Scc.B (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_54f8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 4) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* Scc.B (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_54f9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 4) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* Scc.B Dn */
unsigned long REGPARAM2 CPUFUNC(op_55c0_4)(uae_u32 opcode, struct regstruct *regs)
{
  uae_u32 cyc = 4;
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue(&regs->ccrflags, 5) ? 0xff : 0;
  if(val) cyc = 6;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (regs, 2);
return cyc * CYCLE_UNIT / 2;
}
/* DBcc.W Dn,#<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_55c8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uae_s16 offs = get_iword (regs, 2);
	uaecptr oldpc = m68k_getpc(regs);
	if (!cctrue(&regs->ccrflags, 5)) {
		m68k_incpc(regs, (uae_s32)offs + 2);
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src-1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)offs + 2);
				return 50 * CYCLE_UNIT / 2;
			}
			return 10 * CYCLE_UNIT / 2;
		}
	} else {
	}
	m68k_setpc (regs, oldpc + 4);
}}}
return 14 * CYCLE_UNIT / 2;
}
/* Scc.B (An) */
unsigned long REGPARAM2 CPUFUNC(op_55d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{{	int val = cctrue(&regs->ccrflags, 5) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* Scc.B (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_55d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue(&regs->ccrflags, 5) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* Scc.B -(An) */
unsigned long REGPARAM2 CPUFUNC(op_55e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue(&regs->ccrflags, 5) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* Scc.B (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_55e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 5) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* Scc.B (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_55f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{{	int val = cctrue(&regs->ccrflags, 5) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* Scc.B (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_55f8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 5) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* Scc.B (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_55f9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 5) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* Scc.B Dn */
unsigned long REGPARAM2 CPUFUNC(op_56c0_4)(uae_u32 opcode, struct regstruct *regs)
{
  uae_u32 cyc = 4;
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue(&regs->ccrflags, 6) ? 0xff : 0;
  if(val) cyc = 6;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (regs, 2);
return cyc * CYCLE_UNIT / 2;
}
/* DBcc.W Dn,#<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_56c8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uae_s16 offs = get_iword (regs, 2);
	uaecptr oldpc = m68k_getpc(regs);
	if (!cctrue(&regs->ccrflags, 6)) {
		m68k_incpc(regs, (uae_s32)offs + 2);
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src-1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)offs + 2);
				return 50 * CYCLE_UNIT / 2;
			}
			return 10 * CYCLE_UNIT / 2;
		}
	} else {
	}
	m68k_setpc (regs, oldpc + 4);
}}}
return 14 * CYCLE_UNIT / 2;
}
/* Scc.B (An) */
unsigned long REGPARAM2 CPUFUNC(op_56d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{{	int val = cctrue(&regs->ccrflags, 6) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* Scc.B (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_56d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue(&regs->ccrflags, 6) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* Scc.B -(An) */
unsigned long REGPARAM2 CPUFUNC(op_56e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue(&regs->ccrflags, 6) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* Scc.B (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_56e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 6) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* Scc.B (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_56f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{{	int val = cctrue(&regs->ccrflags, 6) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* Scc.B (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_56f8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 6) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* Scc.B (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_56f9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 6) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* Scc.B Dn */
unsigned long REGPARAM2 CPUFUNC(op_57c0_4)(uae_u32 opcode, struct regstruct *regs)
{
  uae_u32 cyc = 4;
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue(&regs->ccrflags, 7) ? 0xff : 0;
  if(val) cyc = 6;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (regs, 2);
return cyc * CYCLE_UNIT / 2;
}
/* DBcc.W Dn,#<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_57c8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uae_s16 offs = get_iword (regs, 2);
	uaecptr oldpc = m68k_getpc(regs);
	if (!cctrue(&regs->ccrflags, 7)) {
		m68k_incpc(regs, (uae_s32)offs + 2);
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src-1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)offs + 2);
				return 50 * CYCLE_UNIT / 2;
			}
			return 10 * CYCLE_UNIT / 2;
		}
	} else {
	}
	m68k_setpc (regs, oldpc + 4);
}}}
return 14 * CYCLE_UNIT / 2;
}
/* Scc.B (An) */
unsigned long REGPARAM2 CPUFUNC(op_57d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{{	int val = cctrue(&regs->ccrflags, 7) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* Scc.B (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_57d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue(&regs->ccrflags, 7) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* Scc.B -(An) */
unsigned long REGPARAM2 CPUFUNC(op_57e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue(&regs->ccrflags, 7) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* Scc.B (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_57e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 7) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* Scc.B (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_57f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{{	int val = cctrue(&regs->ccrflags, 7) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* Scc.B (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_57f8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 7) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* Scc.B (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_57f9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 7) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* Scc.B Dn */
unsigned long REGPARAM2 CPUFUNC(op_58c0_4)(uae_u32 opcode, struct regstruct *regs)
{
  uae_u32 cyc = 4;
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue(&regs->ccrflags, 8) ? 0xff : 0;
  if(val) cyc = 6;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (regs, 2);
return cyc * CYCLE_UNIT / 2;
}
/* DBcc.W Dn,#<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_58c8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uae_s16 offs = get_iword (regs, 2);
	uaecptr oldpc = m68k_getpc(regs);
	if (!cctrue(&regs->ccrflags, 8)) {
		m68k_incpc(regs, (uae_s32)offs + 2);
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src-1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)offs + 2);
				return 50 * CYCLE_UNIT / 2;
			}
			return 10 * CYCLE_UNIT / 2;
		}
	} else {
	}
	m68k_setpc (regs, oldpc + 4);
}}}
return 14 * CYCLE_UNIT / 2;
}
/* Scc.B (An) */
unsigned long REGPARAM2 CPUFUNC(op_58d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{{	int val = cctrue(&regs->ccrflags, 8) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* Scc.B (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_58d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue(&regs->ccrflags, 8) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* Scc.B -(An) */
unsigned long REGPARAM2 CPUFUNC(op_58e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue(&regs->ccrflags, 8) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* Scc.B (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_58e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 8) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* Scc.B (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_58f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{{	int val = cctrue(&regs->ccrflags, 8) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* Scc.B (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_58f8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 8) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* Scc.B (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_58f9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 8) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* Scc.B Dn */
unsigned long REGPARAM2 CPUFUNC(op_59c0_4)(uae_u32 opcode, struct regstruct *regs)
{
  uae_u32 cyc = 4;
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue(&regs->ccrflags, 9) ? 0xff : 0;
  if(val) cyc = 6;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (regs, 2);
return cyc * CYCLE_UNIT / 2;
}
/* DBcc.W Dn,#<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_59c8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uae_s16 offs = get_iword (regs, 2);
	uaecptr oldpc = m68k_getpc(regs);
	if (!cctrue(&regs->ccrflags, 9)) {
		m68k_incpc(regs, (uae_s32)offs + 2);
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src-1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)offs + 2);
				return 50 * CYCLE_UNIT / 2;
			}
			return 10 * CYCLE_UNIT / 2;
		}
	} else {
	}
	m68k_setpc (regs, oldpc + 4);
}}}
return 14 * CYCLE_UNIT / 2;
}
/* Scc.B (An) */
unsigned long REGPARAM2 CPUFUNC(op_59d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{{	int val = cctrue(&regs->ccrflags, 9) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* Scc.B (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_59d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue(&regs->ccrflags, 9) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* Scc.B -(An) */
unsigned long REGPARAM2 CPUFUNC(op_59e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue(&regs->ccrflags, 9) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* Scc.B (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_59e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 9) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* Scc.B (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_59f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{{	int val = cctrue(&regs->ccrflags, 9) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* Scc.B (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_59f8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 9) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* Scc.B (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_59f9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 9) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* Scc.B Dn */
unsigned long REGPARAM2 CPUFUNC(op_5ac0_4)(uae_u32 opcode, struct regstruct *regs)
{
  uae_u32 cyc = 4;
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue(&regs->ccrflags, 10) ? 0xff : 0;
  if(val) cyc = 6;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (regs, 2);
return cyc * CYCLE_UNIT / 2;
}
/* DBcc.W Dn,#<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_5ac8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uae_s16 offs = get_iword (regs, 2);
	uaecptr oldpc = m68k_getpc(regs);
	if (!cctrue(&regs->ccrflags, 10)) {
		m68k_incpc(regs, (uae_s32)offs + 2);
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src-1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)offs + 2);
				return 50 * CYCLE_UNIT / 2;
			}
			return 10 * CYCLE_UNIT / 2;
		}
	} else {
	}
	m68k_setpc (regs, oldpc + 4);
}}}
return 14 * CYCLE_UNIT / 2;
}
/* Scc.B (An) */
unsigned long REGPARAM2 CPUFUNC(op_5ad0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{{	int val = cctrue(&regs->ccrflags, 10) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* Scc.B (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_5ad8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue(&regs->ccrflags, 10) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* Scc.B -(An) */
unsigned long REGPARAM2 CPUFUNC(op_5ae0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue(&regs->ccrflags, 10) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* Scc.B (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_5ae8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 10) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* Scc.B (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_5af0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{{	int val = cctrue(&regs->ccrflags, 10) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* Scc.B (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_5af8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 10) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* Scc.B (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_5af9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 10) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* Scc.B Dn */
unsigned long REGPARAM2 CPUFUNC(op_5bc0_4)(uae_u32 opcode, struct regstruct *regs)
{
  uae_u32 cyc = 4;
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue(&regs->ccrflags, 11) ? 0xff : 0;
  if(val) cyc = 6;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (regs, 2);
return cyc * CYCLE_UNIT / 2;
}
/* DBcc.W Dn,#<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_5bc8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uae_s16 offs = get_iword (regs, 2);
	uaecptr oldpc = m68k_getpc(regs);
	if (!cctrue(&regs->ccrflags, 11)) {
		m68k_incpc(regs, (uae_s32)offs + 2);
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src-1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)offs + 2);
				return 50 * CYCLE_UNIT / 2;
			}
			return 10 * CYCLE_UNIT / 2;
		}
	} else {
	}
	m68k_setpc (regs, oldpc + 4);
}}}
return 14 * CYCLE_UNIT / 2;
}
/* Scc.B (An) */
unsigned long REGPARAM2 CPUFUNC(op_5bd0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{{	int val = cctrue(&regs->ccrflags, 11) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* Scc.B (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_5bd8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue(&regs->ccrflags, 11) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* Scc.B -(An) */
unsigned long REGPARAM2 CPUFUNC(op_5be0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue(&regs->ccrflags, 11) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* Scc.B (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_5be8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 11) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* Scc.B (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_5bf0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{{	int val = cctrue(&regs->ccrflags, 11) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* Scc.B (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_5bf8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 11) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* Scc.B (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_5bf9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 11) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* Scc.B Dn */
unsigned long REGPARAM2 CPUFUNC(op_5cc0_4)(uae_u32 opcode, struct regstruct *regs)
{
  uae_u32 cyc = 4;
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue(&regs->ccrflags, 12) ? 0xff : 0;
  if(val) cyc = 6;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (regs, 2);
return cyc * CYCLE_UNIT / 2;
}
/* DBcc.W Dn,#<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_5cc8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uae_s16 offs = get_iword (regs, 2);
	uaecptr oldpc = m68k_getpc(regs);
	if (!cctrue(&regs->ccrflags, 12)) {
		m68k_incpc(regs, (uae_s32)offs + 2);
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src-1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)offs + 2);
				return 50 * CYCLE_UNIT / 2;
			}
			return 10 * CYCLE_UNIT / 2;
		}
	} else {
	}
	m68k_setpc (regs, oldpc + 4);
}}}
return 14 * CYCLE_UNIT / 2;
}
/* Scc.B (An) */
unsigned long REGPARAM2 CPUFUNC(op_5cd0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{{	int val = cctrue(&regs->ccrflags, 12) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* Scc.B (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_5cd8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue(&regs->ccrflags, 12) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* Scc.B -(An) */
unsigned long REGPARAM2 CPUFUNC(op_5ce0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue(&regs->ccrflags, 12) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* Scc.B (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_5ce8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 12) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* Scc.B (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_5cf0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{{	int val = cctrue(&regs->ccrflags, 12) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* Scc.B (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_5cf8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 12) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* Scc.B (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_5cf9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 12) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* Scc.B Dn */
unsigned long REGPARAM2 CPUFUNC(op_5dc0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 cyc = 4;
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue(&regs->ccrflags, 13) ? 0xff : 0;
  if(val) cyc = 6;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (regs, 2);
return cyc * CYCLE_UNIT / 2;
}
/* DBcc.W Dn,#<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_5dc8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uae_s16 offs = get_iword (regs, 2);
	uaecptr oldpc = m68k_getpc(regs);
	if (!cctrue(&regs->ccrflags, 13)) {
		m68k_incpc(regs, (uae_s32)offs + 2);
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src-1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)offs + 2);
				return 50 * CYCLE_UNIT / 2;
			}
			return 10 * CYCLE_UNIT / 2;
		}
	} else {
	}
	m68k_setpc (regs, oldpc + 4);
}}}
return 14 * CYCLE_UNIT / 2;
}
/* Scc.B (An) */
unsigned long REGPARAM2 CPUFUNC(op_5dd0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{{	int val = cctrue(&regs->ccrflags, 13) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* Scc.B (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_5dd8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue(&regs->ccrflags, 13) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* Scc.B -(An) */
unsigned long REGPARAM2 CPUFUNC(op_5de0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue(&regs->ccrflags, 13) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* Scc.B (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_5de8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 13) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* Scc.B (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_5df0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{{	int val = cctrue(&regs->ccrflags, 13) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* Scc.B (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_5df8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 13) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* Scc.B (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_5df9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 13) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* Scc.B Dn */
unsigned long REGPARAM2 CPUFUNC(op_5ec0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 cyc = 4;
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue(&regs->ccrflags, 14) ? 0xff : 0;
  if(val) cyc = 6;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (regs, 2);
return cyc * CYCLE_UNIT / 2;
}
/* DBcc.W Dn,#<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_5ec8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uae_s16 offs = get_iword (regs, 2);
	uaecptr oldpc = m68k_getpc(regs);
	if (!cctrue(&regs->ccrflags, 14)) {
		m68k_incpc(regs, (uae_s32)offs + 2);
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src-1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)offs + 2);
				return 50 * CYCLE_UNIT / 2;
			}
			return 10 * CYCLE_UNIT / 2;
		}
	} else {
	}
	m68k_setpc (regs, oldpc + 4);
}}}
return 14 * CYCLE_UNIT / 2;
}
/* Scc.B (An) */
unsigned long REGPARAM2 CPUFUNC(op_5ed0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{{	int val = cctrue(&regs->ccrflags, 14) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* Scc.B (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_5ed8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue(&regs->ccrflags, 14) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* Scc.B -(An) */
unsigned long REGPARAM2 CPUFUNC(op_5ee0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue(&regs->ccrflags, 14) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* Scc.B (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_5ee8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 14) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* Scc.B (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_5ef0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{{	int val = cctrue(&regs->ccrflags, 14) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* Scc.B (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_5ef8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 14) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* Scc.B (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_5ef9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 14) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* Scc.B Dn */
unsigned long REGPARAM2 CPUFUNC(op_5fc0_4)(uae_u32 opcode, struct regstruct *regs)
{
  uae_u32 cyc = 4;
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue(&regs->ccrflags, 15) ? 0xff : 0;
  if(val) cyc = 6;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (regs, 2);
return cyc * CYCLE_UNIT / 2;
}
/* DBcc.W Dn,#<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_5fc8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uae_s16 offs = get_iword (regs, 2);
	uaecptr oldpc = m68k_getpc(regs);
	if (!cctrue(&regs->ccrflags, 15)) {
		m68k_incpc(regs, (uae_s32)offs + 2);
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src-1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)offs + 2);
				return 50 * CYCLE_UNIT / 2;
			}
			return 10 * CYCLE_UNIT / 2;
		}
	} else {
	}
	m68k_setpc (regs, oldpc + 4);
}}}
return 14 * CYCLE_UNIT / 2;
}
/* Scc.B (An) */
unsigned long REGPARAM2 CPUFUNC(op_5fd0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
{{	int val = cctrue(&regs->ccrflags, 15) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* Scc.B (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_5fd8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue(&regs->ccrflags, 15) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* Scc.B -(An) */
unsigned long REGPARAM2 CPUFUNC(op_5fe0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue(&regs->ccrflags, 15) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* Scc.B (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_5fe8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 15) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* Scc.B (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_5ff0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{{	int val = cctrue(&regs->ccrflags, 15) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* Scc.B (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_5ff8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 15) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* Scc.B (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_5ff9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
{{	int val = cctrue(&regs->ccrflags, 15) ? 0xff : 0;
	put_byte (srca,val);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* Bcc.W #<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_6000_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s16 src = get_iword (regs, 2);
	if (!cctrue(&regs->ccrflags, 0)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)src);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_incpc (regs, (uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:
	m68k_incpc (regs, 4);
}}
return 12 * CYCLE_UNIT / 2;
}
/* Bcc.B #<data> */
unsigned long REGPARAM2 CPUFUNC(op_6001_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue(&regs->ccrflags, 0)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)src);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_incpc (regs, (uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:
	m68k_incpc (regs, 2);
}}
return 8 * CYCLE_UNIT / 2;
}
/* BSR.W #<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_6100_4)(uae_u32 opcode, struct regstruct *regs)
{
{	uae_s32 s;
{	uae_s16 src = get_iword (regs, 2);
	s = (uae_s32)src + 2;
	if (src & 1) {
		exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + s);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_do_bsr (regs, m68k_getpc(regs) + 4, s);
}}
return 18 * CYCLE_UNIT / 2;
}
/* BSR.B #<data> */
unsigned long REGPARAM2 CPUFUNC(op_6101_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{	uae_s32 s;
{	uae_u32 src = srcreg;
	s = (uae_s32)src + 2;
	if (src & 1) {
		exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + s);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_do_bsr (regs, m68k_getpc(regs) + 2, s);
}}
return 18 * CYCLE_UNIT / 2;
}
/* Bcc.W #<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_6200_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s16 src = get_iword (regs, 2);
	if (!cctrue(&regs->ccrflags, 2)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)src);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_incpc (regs, (uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:
	m68k_incpc (regs, 4);
}}
return 12 * CYCLE_UNIT / 2;
}
/* Bcc.B #<data> */
unsigned long REGPARAM2 CPUFUNC(op_6201_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue(&regs->ccrflags, 2)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)src);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_incpc (regs, (uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:
	m68k_incpc (regs, 2);
}}
return 8 * CYCLE_UNIT / 2;
}
/* Bcc.W #<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_6300_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s16 src = get_iword (regs, 2);
	if (!cctrue(&regs->ccrflags, 3)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)src);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_incpc (regs, (uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:
	m68k_incpc (regs, 4);
}}
return 12 * CYCLE_UNIT / 2;
}
/* Bcc.B #<data> */
unsigned long REGPARAM2 CPUFUNC(op_6301_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue(&regs->ccrflags, 3)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)src);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_incpc (regs, (uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:
	m68k_incpc (regs, 2);
}}
return 8 * CYCLE_UNIT / 2;
}
/* Bcc.W #<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_6400_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s16 src = get_iword (regs, 2);
	if (!cctrue(&regs->ccrflags, 4)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)src);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_incpc (regs, (uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:
	m68k_incpc (regs, 4);
}}
return 12 * CYCLE_UNIT / 2;
}
/* Bcc.B #<data> */
unsigned long REGPARAM2 CPUFUNC(op_6401_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue(&regs->ccrflags, 4)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)src);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_incpc (regs, (uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:
	m68k_incpc (regs, 2);
}}
return 8 * CYCLE_UNIT / 2;
}
/* Bcc.W #<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_6500_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s16 src = get_iword (regs, 2);
	if (!cctrue(&regs->ccrflags, 5)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)src);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_incpc (regs, (uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:
	m68k_incpc (regs, 4);
}}
return 12 * CYCLE_UNIT / 2;
}
/* Bcc.B #<data> */
unsigned long REGPARAM2 CPUFUNC(op_6501_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue(&regs->ccrflags, 5)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)src);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_incpc (regs, (uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:
	m68k_incpc (regs, 2);
}}
return 8 * CYCLE_UNIT / 2;
}
/* Bcc.W #<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_6600_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s16 src = get_iword (regs, 2);
	if (!cctrue(&regs->ccrflags, 6)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)src);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_incpc (regs, (uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:
	m68k_incpc (regs, 4);
}}
return 12 * CYCLE_UNIT / 2;
}
/* Bcc.B #<data> */
unsigned long REGPARAM2 CPUFUNC(op_6601_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue(&regs->ccrflags, 6)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)src);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_incpc (regs, (uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:
	m68k_incpc (regs, 2);
}}
return 8 * CYCLE_UNIT / 2;
}
/* Bcc.W #<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_6700_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s16 src = get_iword (regs, 2);
	if (!cctrue(&regs->ccrflags, 7)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)src);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_incpc (regs, (uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:
	m68k_incpc (regs, 4);
}}
return 12 * CYCLE_UNIT / 2;
}
/* Bcc.B #<data> */
unsigned long REGPARAM2 CPUFUNC(op_6701_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue(&regs->ccrflags, 7)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)src);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_incpc (regs, (uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:
	m68k_incpc (regs, 2);
}}
return 8 * CYCLE_UNIT / 2;
}
/* Bcc.W #<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_6800_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s16 src = get_iword (regs, 2);
	if (!cctrue(&regs->ccrflags, 8)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)src);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_incpc (regs, (uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:
	m68k_incpc (regs, 4);
}}
return 12 * CYCLE_UNIT / 2;
}
/* Bcc.B #<data> */
unsigned long REGPARAM2 CPUFUNC(op_6801_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue(&regs->ccrflags, 8)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)src);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_incpc (regs, (uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:
	m68k_incpc (regs, 2);
}}
return 8 * CYCLE_UNIT / 2;
}
/* Bcc.W #<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_6900_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s16 src = get_iword (regs, 2);
	if (!cctrue(&regs->ccrflags, 9)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)src);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_incpc (regs, (uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:
	m68k_incpc (regs, 4);
}}
return 12 * CYCLE_UNIT / 2;
}
/* Bcc.B #<data> */
unsigned long REGPARAM2 CPUFUNC(op_6901_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue(&regs->ccrflags, 9)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)src);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_incpc (regs, (uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:
	m68k_incpc (regs, 2);
}}
return 8 * CYCLE_UNIT / 2;
}
/* Bcc.W #<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_6a00_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s16 src = get_iword (regs, 2);
	if (!cctrue(&regs->ccrflags, 10)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)src);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_incpc (regs, (uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:
	m68k_incpc (regs, 4);
}}
return 12 * CYCLE_UNIT / 2;
}
/* Bcc.B #<data> */
unsigned long REGPARAM2 CPUFUNC(op_6a01_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue(&regs->ccrflags, 10)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)src);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_incpc (regs, (uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:
	m68k_incpc (regs, 2);
}}
return 8 * CYCLE_UNIT / 2;
}
/* Bcc.W #<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_6b00_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s16 src = get_iword (regs, 2);
	if (!cctrue(&regs->ccrflags, 11)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)src);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_incpc (regs, (uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:
	m68k_incpc (regs, 4);
}}
return 12 * CYCLE_UNIT / 2;
}
/* Bcc.B #<data> */
unsigned long REGPARAM2 CPUFUNC(op_6b01_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue(&regs->ccrflags, 11)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)src);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_incpc (regs, (uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:
	m68k_incpc (regs, 2);
}}
return 8 * CYCLE_UNIT / 2;
}
/* Bcc.W #<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_6c00_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s16 src = get_iword (regs, 2);
	if (!cctrue(&regs->ccrflags, 12)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)src);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_incpc (regs, (uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:
	m68k_incpc (regs, 4);
}}
return 12 * CYCLE_UNIT / 2;
}
/* Bcc.B #<data> */
unsigned long REGPARAM2 CPUFUNC(op_6c01_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue(&regs->ccrflags, 12)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)src);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_incpc (regs, (uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:
	m68k_incpc (regs, 2);
}}
return 8 * CYCLE_UNIT / 2;
}
/* Bcc.W #<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_6d00_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s16 src = get_iword (regs, 2);
	if (!cctrue(&regs->ccrflags, 13)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)src);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_incpc (regs, (uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:
	m68k_incpc (regs, 4);
}}
return 12 * CYCLE_UNIT / 2;
}
/* Bcc.B #<data> */
unsigned long REGPARAM2 CPUFUNC(op_6d01_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue(&regs->ccrflags, 13)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)src);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_incpc (regs, (uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:
	m68k_incpc (regs, 2);
}}
return 8 * CYCLE_UNIT / 2;
}
/* Bcc.W #<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_6e00_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s16 src = get_iword (regs, 2);
	if (!cctrue(&regs->ccrflags, 14)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)src);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_incpc (regs, (uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:
	m68k_incpc (regs, 4);
}}
return 12 * CYCLE_UNIT / 2;
}
/* Bcc.B #<data> */
unsigned long REGPARAM2 CPUFUNC(op_6e01_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue(&regs->ccrflags, 14)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)src);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_incpc (regs, (uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:
	m68k_incpc (regs, 2);
}}
return 8 * CYCLE_UNIT / 2;
}
/* Bcc.W #<data>.W */
unsigned long REGPARAM2 CPUFUNC(op_6f00_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uae_s16 src = get_iword (regs, 2);
	if (!cctrue(&regs->ccrflags, 15)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)src);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_incpc (regs, (uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:
	m68k_incpc (regs, 4);
}}
return 12 * CYCLE_UNIT / 2;
}
/* Bcc.B #<data> */
unsigned long REGPARAM2 CPUFUNC(op_6f01_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue(&regs->ccrflags, 15)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc(regs) + 2, m68k_getpc(regs) + 2 + (uae_s32)src);
		return 50 * CYCLE_UNIT / 2;
	}
	m68k_incpc (regs, (uae_s32)src + 2);
	return 10 * CYCLE_UNIT / 2;
didnt_jump:
	m68k_incpc (regs, 2);
}}
return 8 * CYCLE_UNIT / 2;
}
/* MOVE.L #<data>,Dn */
unsigned long REGPARAM2 CPUFUNC(op_7000_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_u32 src = srcreg;
{	m68k_dreg(regs, dstreg) = (src);
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_incpc (regs, 2);
}}}return 4 * CYCLE_UNIT / 2;
}
/* OR.B Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_8000_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* OR.B (An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_8010_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* OR.B (An)+,Dn */
unsigned long REGPARAM2 CPUFUNC(op_8018_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* OR.B -(An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_8020_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (regs, 2);
return 10 * CYCLE_UNIT / 2;
}
/* OR.B (d16,An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_8028_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* OR.B (d8,An,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_8030_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (regs, 4);
return 14 * CYCLE_UNIT / 2;
}
/* OR.B (xxx).W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_8038_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* OR.B (xxx).L,Dn */
unsigned long REGPARAM2 CPUFUNC(op_8039_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (regs, 6);
return 16 * CYCLE_UNIT / 2;
}
/* OR.B (d16,PC),Dn */
unsigned long REGPARAM2 CPUFUNC(op_803a_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* OR.B (d8,PC,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_803b_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (regs, 4);
return 14 * CYCLE_UNIT / 2;
}
/* OR.B #<data>.B,Dn */
unsigned long REGPARAM2 CPUFUNC(op_803c_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}	m68k_incpc (regs, 4);
return 8 * CYCLE_UNIT / 2;
}
/* OR.W Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_8040_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* OR.W (An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_8050_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* OR.W (An)+,Dn */
unsigned long REGPARAM2 CPUFUNC(op_8058_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg(regs, srcreg) += 2;
{	uae_s16 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* OR.W -(An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_8060_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s16 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (regs, 2);
return 10 * CYCLE_UNIT / 2;
}
/* OR.W (d16,An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_8068_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* OR.W (d8,An,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_8070_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (regs, 4);
return 14 * CYCLE_UNIT / 2;
}
/* OR.W (xxx).W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_8078_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* OR.W (xxx).L,Dn */
unsigned long REGPARAM2 CPUFUNC(op_8079_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (regs, 6);
return 16 * CYCLE_UNIT / 2;
}
/* OR.W (d16,PC),Dn */
unsigned long REGPARAM2 CPUFUNC(op_807a_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* OR.W (d8,PC,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_807b_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (regs, 4);
return 14 * CYCLE_UNIT / 2;
}
/* OR.W #<data>.W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_807c_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}	m68k_incpc (regs, 4);
return 8 * CYCLE_UNIT / 2;
}
/* OR.L Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_8080_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* OR.L (An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_8090_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* OR.L (An)+,Dn */
unsigned long REGPARAM2 CPUFUNC(op_8098_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg(regs, srcreg) += 4;
{	uae_s32 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* OR.L -(An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_80a0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s32 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
}}}}	m68k_incpc (regs, 2);
return 16 * CYCLE_UNIT / 2;
}
/* OR.L (d16,An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_80a8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* OR.L (d8,An,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_80b0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* OR.L (xxx).W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_80b8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* OR.L (xxx).L,Dn */
unsigned long REGPARAM2 CPUFUNC(op_80b9_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
}}}}	m68k_incpc (regs, 6);
return 22 * CYCLE_UNIT / 2;
}
/* OR.L (d16,PC),Dn */
unsigned long REGPARAM2 CPUFUNC(op_80ba_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* OR.L (d8,PC,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_80bb_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* OR.L #<data>.L,Dn */
unsigned long REGPARAM2 CPUFUNC(op_80bc_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
}}}	m68k_incpc (regs, 6);
return 16 * CYCLE_UNIT / 2;
}
/* DIVU.W Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_80c0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc(regs);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	CLEAR_CZNV (&regs->ccrflags);
	if (src == 0) {
		SET_VFLG (&regs->ccrflags, 1);
		if (dst < 0) SET_NFLG (&regs->ccrflags, 1);
		m68k_incpc (regs, 2);
		Exception (5, regs, oldpc);
		return 38 * CYCLE_UNIT / 2;
	} else {
		uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
		uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
		if (newv > 0xffff) {
			SET_VFLG (&regs->ccrflags, 1);
			SET_NFLG (&regs->ccrflags, 1);
		} else {
			CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg(regs, dstreg) = (newv);
		}
	m68k_incpc (regs, 2);
	}
return getDivu68kCycles(dst, src) * CYCLE_UNIT / 2;
}
/* DIVU.W (An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_80d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc(regs);
	uaecptr srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word (srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	CLEAR_CZNV (&regs->ccrflags);
	if (src == 0) {
		SET_VFLG (&regs->ccrflags, 1);
		if (dst < 0) SET_NFLG (&regs->ccrflags, 1);
		m68k_incpc (regs, 2);
		Exception (5, regs, oldpc);
		return 42 * CYCLE_UNIT / 2;
	} else {
		uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
		uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
		if (newv > 0xffff) {
			SET_VFLG (&regs->ccrflags, 1);
			SET_NFLG (&regs->ccrflags, 1);
		} else {
			CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg(regs, dstreg) = (newv);
		}
	m68k_incpc (regs, 2);
	}
return (getDivu68kCycles(dst, src) + 4) * CYCLE_UNIT / 2;
}
/* DIVU.W (An)+,Dn */
unsigned long REGPARAM2 CPUFUNC(op_80d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc(regs);
	uaecptr srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word (srca);
	m68k_areg(regs, srcreg) += 2;
	uae_s32 dst = m68k_dreg(regs, dstreg);
	CLEAR_CZNV (&regs->ccrflags);
	if (src == 0) {
		SET_VFLG (&regs->ccrflags, 1);
		if (dst < 0) SET_NFLG (&regs->ccrflags, 1);
		m68k_incpc (regs, 2);
		Exception (5, regs, oldpc);
		return 42 * CYCLE_UNIT / 2;
	} else {
		uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
		uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
		if (newv > 0xffff) {
			SET_VFLG (&regs->ccrflags, 1);
			SET_NFLG (&regs->ccrflags, 1);
		} else {
			CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg(regs, dstreg) = (newv);
		}
	m68k_incpc (regs, 2);
	}
return (getDivu68kCycles(dst, src) + 4) * CYCLE_UNIT / 2;
}
/* DIVU.W -(An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_80e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc(regs);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
	uae_s32 dst = m68k_dreg(regs, dstreg);
	CLEAR_CZNV (&regs->ccrflags);
	if (src == 0) {
		SET_VFLG (&regs->ccrflags, 1);
		if (dst < 0) SET_NFLG (&regs->ccrflags, 1);
		m68k_incpc (regs, 2);
		Exception (5, regs, oldpc);
		return 44 * CYCLE_UNIT / 2;
	} else {
		uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
		uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
		if (newv > 0xffff) {
			SET_VFLG (&regs->ccrflags, 1);
			SET_NFLG (&regs->ccrflags, 1);
		} else {
			CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg(regs, dstreg) = (newv);
		}
	m68k_incpc (regs, 2);
	}
return (getDivu68kCycles(dst, src) + 6) * CYCLE_UNIT / 2;
}
/* DIVU.W (d16,An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_80e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc(regs);
	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
	uae_s16 src = get_word (srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	CLEAR_CZNV (&regs->ccrflags);
	if (src == 0) {
		SET_VFLG (&regs->ccrflags, 1);
		if (dst < 0) SET_NFLG (&regs->ccrflags, 1);
		m68k_incpc (regs, 4);
		Exception (5, regs, oldpc);
		return 46 * CYCLE_UNIT / 2;
	} else {
		uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
		uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
		if (newv > 0xffff) {
			SET_VFLG (&regs->ccrflags, 1);
			SET_NFLG (&regs->ccrflags, 1);
		} else {
			CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg(regs, dstreg) = (newv);
		}
	m68k_incpc (regs, 4);
	}
return (getDivu68kCycles(dst, src) + 8) * CYCLE_UNIT / 2;
}
/* DIVU.W (d8,An,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_80f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc(regs);
	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
	uae_s16 src = get_word (srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	CLEAR_CZNV (&regs->ccrflags);
	if (src == 0) {
		SET_VFLG (&regs->ccrflags, 1);
		if (dst < 0) SET_NFLG (&regs->ccrflags, 1);
		m68k_incpc (regs, 4);
		Exception (5, regs, oldpc);
		return 48 * CYCLE_UNIT / 2;
	} else {
		uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
		uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
		if (newv > 0xffff) {
			SET_VFLG (&regs->ccrflags, 1);
			SET_NFLG (&regs->ccrflags, 1);
		} else {
			CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg(regs, dstreg) = (newv);
		}
	m68k_incpc (regs, 4);
	}
return (getDivu68kCycles(dst, src) + 10) * CYCLE_UNIT / 2;
}
/* DIVU.W (xxx).W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_80f8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc(regs);
	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
	uae_s16 src = get_word (srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	CLEAR_CZNV (&regs->ccrflags);
	if (src == 0) {
		SET_VFLG (&regs->ccrflags, 1);
		if (dst < 0) SET_NFLG (&regs->ccrflags, 1);
		m68k_incpc (regs, 4);
		Exception (5, regs, oldpc);
		return 46 * CYCLE_UNIT / 2;
	} else {
		uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
		uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
		if (newv > 0xffff) {
			SET_VFLG (&regs->ccrflags, 1);
			SET_NFLG (&regs->ccrflags, 1);
		} else {
			CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg(regs, dstreg) = (newv);
		}
	m68k_incpc (regs, 4);
	}
return (getDivu68kCycles(dst, src) + 8) * CYCLE_UNIT / 2;
}
/* DIVU.W (xxx).L,Dn */
unsigned long REGPARAM2 CPUFUNC(op_80f9_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc(regs);
	uaecptr srca = get_ilong (regs, 2);
	uae_s16 src = get_word (srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	CLEAR_CZNV (&regs->ccrflags);
	if (src == 0) {
		SET_VFLG (&regs->ccrflags, 1);
		if (dst < 0) SET_NFLG (&regs->ccrflags, 1);
		m68k_incpc (regs, 6);
		Exception (5, regs, oldpc);
		return 50 * CYCLE_UNIT / 2;
	} else {
		uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
		uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
		if (newv > 0xffff) {
			SET_VFLG (&regs->ccrflags, 1);
			SET_NFLG (&regs->ccrflags, 1);
		} else {
			CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg(regs, dstreg) = (newv);
		}
	m68k_incpc (regs, 6);
	}
return (getDivu68kCycles(dst, src) + 12) * CYCLE_UNIT / 2;
}
/* DIVU.W (d16,PC),Dn */
unsigned long REGPARAM2 CPUFUNC(op_80fa_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc(regs);
	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
	uae_s16 src = get_word (srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	CLEAR_CZNV (&regs->ccrflags);
	if (src == 0) {
		SET_VFLG (&regs->ccrflags, 1);
		if (dst < 0) SET_NFLG (&regs->ccrflags, 1);
		m68k_incpc (regs, 4);
		Exception (5, regs, oldpc);
		return 46 * CYCLE_UNIT / 2;
	} else {
		uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
		uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
		if (newv > 0xffff) {
			SET_VFLG (&regs->ccrflags, 1);
			SET_NFLG (&regs->ccrflags, 1);
		} else {
			CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg(regs, dstreg) = (newv);
		}
	m68k_incpc (regs, 4);
	}
return (getDivu68kCycles(dst, src) + 8) * CYCLE_UNIT / 2;
}
/* DIVU.W (d8,PC,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_80fb_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc(regs);
	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
	uae_s16 src = get_word (srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	CLEAR_CZNV (&regs->ccrflags);
	if (src == 0) {
		SET_VFLG (&regs->ccrflags, 1);
		if (dst < 0) SET_NFLG (&regs->ccrflags, 1);
		m68k_incpc (regs, 4);
		Exception (5, regs, oldpc);
		return 48 * CYCLE_UNIT / 2;
	} else {
		uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
		uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
		if (newv > 0xffff) {
			SET_VFLG (&regs->ccrflags, 1);
			SET_NFLG (&regs->ccrflags, 1);
		} else {
			CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg(regs, dstreg) = (newv);
		}
	m68k_incpc (regs, 4);
	}
return (getDivu68kCycles(dst, src) + 10) * CYCLE_UNIT / 2;
}
/* DIVU.W #<data>.W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_80fc_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc(regs);
	uae_s16 src = get_iword (regs, 2);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	CLEAR_CZNV (&regs->ccrflags);
	if (src == 0) {
		SET_VFLG (&regs->ccrflags, 1);
		if (dst < 0) SET_NFLG (&regs->ccrflags, 1);
		m68k_incpc (regs, 4);
		Exception (5, regs, oldpc);
		return 42 * CYCLE_UNIT / 2;
	} else {
		uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
		uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
		if (newv > 0xffff) {
			SET_VFLG (&regs->ccrflags, 1);
			SET_NFLG (&regs->ccrflags, 1);
		} else {
			CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg(regs, dstreg) = (newv);
		}
	m68k_incpc (regs, 4);
	}
return (getDivu68kCycles(dst, src) + 4) * CYCLE_UNIT / 2;
}
/* SBCD.B Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_8100_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{	uae_u16 newv_lo = (dst & 0xF) - (src & 0xF) - (GET_XFLG (&regs->ccrflags) ? 1 : 0);
	uae_u16 newv_hi = (dst & 0xF0) - (src & 0xF0);
	uae_u16 newv, tmp_newv;
	int bcd = 0;
	newv = tmp_newv = newv_hi + newv_lo;
	if (newv_lo & 0xF0) { newv -= 6; bcd = 6; };
	if ((((dst & 0xFF) - (src & 0xFF) - (GET_XFLG (&regs->ccrflags) ? 1 : 0)) & 0x100) > 0xFF) { newv -= 0x60; }
	SET_CFLG (&regs->ccrflags, (((dst & 0xFF) - (src & 0xFF) - bcd - (GET_XFLG (&regs->ccrflags) ? 1 : 0)) & 0x300) > 0xFF);
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s8)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s8)(newv)) < 0);
	SET_VFLG (&regs->ccrflags, (tmp_newv & 0x80) != 0 && (newv & 0x80) == 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}	m68k_incpc (regs, 2);
return 6 * CYCLE_UNIT / 2;
}
/* SBCD.B -(An),-(An) */
unsigned long REGPARAM2 CPUFUNC(op_8108_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
{	uae_u16 newv_lo = (dst & 0xF) - (src & 0xF) - (GET_XFLG (&regs->ccrflags) ? 1 : 0);
	uae_u16 newv_hi = (dst & 0xF0) - (src & 0xF0);
	uae_u16 newv, tmp_newv;
	int bcd = 0;
	newv = tmp_newv = newv_hi + newv_lo;
	if (newv_lo & 0xF0) { newv -= 6; bcd = 6; };
	if ((((dst & 0xFF) - (src & 0xFF) - (GET_XFLG (&regs->ccrflags) ? 1 : 0)) & 0x100) > 0xFF) { newv -= 0x60; }
	SET_CFLG (&regs->ccrflags, (((dst & 0xFF) - (src & 0xFF) - bcd - (GET_XFLG (&regs->ccrflags) ? 1 : 0)) & 0x300) > 0xFF);
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s8)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s8)(newv)) < 0);
	SET_VFLG (&regs->ccrflags, (tmp_newv & 0x80) != 0 && (newv & 0x80) == 0);
	put_byte (dsta,newv);
}}}}}}	m68k_incpc (regs, 2);
return 18 * CYCLE_UNIT / 2;
}
/* OR.B Dn,(An) */
unsigned long REGPARAM2 CPUFUNC(op_8110_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* OR.B Dn,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_8118_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* OR.B Dn,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_8120_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* OR.B Dn,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_8128_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 dst = get_byte (dsta);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* OR.B Dn,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_8130_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 2));
{	uae_s8 dst = get_byte (dsta);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* OR.B Dn,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_8138_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 dst = get_byte (dsta);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* OR.B Dn,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_8139_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = get_ilong (regs, 2);
{	uae_s8 dst = get_byte (dsta);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* OR.W Dn,(An) */
unsigned long REGPARAM2 CPUFUNC(op_8150_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s16 dst = get_word (dsta);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* OR.W Dn,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_8158_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s16 dst = get_word (dsta);
	m68k_areg(regs, dstreg) += 2;
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* OR.W Dn,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_8160_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* OR.W Dn,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_8168_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 dst = get_word (dsta);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* OR.W Dn,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_8170_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 2));
{	uae_s16 dst = get_word (dsta);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* OR.W Dn,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_8178_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 dst = get_word (dsta);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* OR.W Dn,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_8179_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = get_ilong (regs, 2);
{	uae_s16 dst = get_word (dsta);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* OR.L Dn,(An) */
unsigned long REGPARAM2 CPUFUNC(op_8190_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s32 dst = get_long (dsta);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 2);
return 20 * CYCLE_UNIT / 2;
}
/* OR.L Dn,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_8198_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s32 dst = get_long (dsta);
	m68k_areg(regs, dstreg) += 4;
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 2);
return 20 * CYCLE_UNIT / 2;
}
/* OR.L Dn,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_81a0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 2);
return 22 * CYCLE_UNIT / 2;
}
/* OR.L Dn,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_81a8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 dst = get_long (dsta);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 24 * CYCLE_UNIT / 2;
}
/* OR.L Dn,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_81b0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 2));
{	uae_s32 dst = get_long (dsta);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 26 * CYCLE_UNIT / 2;
}
/* OR.L Dn,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_81b8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 dst = get_long (dsta);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 24 * CYCLE_UNIT / 2;
}
/* OR.L Dn,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_81b9_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = get_ilong (regs, 2);
{	uae_s32 dst = get_long (dsta);
	src |= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 6);
return 28 * CYCLE_UNIT / 2;
}
/* DIVS.W Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_81c0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc(regs);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	CLEAR_CZNV (&regs->ccrflags);
	if (src == 0) {
		SET_VFLG (&regs->ccrflags, 1);
		SET_ZFLG (&regs->ccrflags, 1);
		m68k_incpc (regs, 2);
		Exception (5, regs, oldpc);
		return 38 * CYCLE_UNIT / 2;
	} else {
		uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
		uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
		if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) {
			SET_VFLG (&regs->ccrflags, 1);
			SET_NFLG (&regs->ccrflags, 1);
		} else {
			if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg(regs, dstreg) = (newv);
		}
	m68k_incpc (regs, 2);
	}
return (getDivs68kCycles(dst, src) + 0) * CYCLE_UNIT / 2;
}
/* DIVS.W (An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_81d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc(regs);
	uaecptr srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word (srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	CLEAR_CZNV (&regs->ccrflags);
	if (src == 0) {
		SET_VFLG (&regs->ccrflags, 1);
		SET_ZFLG (&regs->ccrflags, 1);
		m68k_incpc (regs, 2);
		Exception (5, regs, oldpc);
		return 42 * CYCLE_UNIT / 2;
	} else {
		uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
		uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
		if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) {
			SET_VFLG (&regs->ccrflags, 1);
			SET_NFLG (&regs->ccrflags, 1);
		} else {
			if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg(regs, dstreg) = (newv);
		}
	m68k_incpc (regs, 2);
	}
return (getDivs68kCycles(dst, src) + 4) * CYCLE_UNIT / 2;
}
/* DIVS.W (An)+,Dn */
unsigned long REGPARAM2 CPUFUNC(op_81d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc(regs);
	uaecptr srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word (srca);
	m68k_areg(regs, srcreg) += 2;
	uae_s32 dst = m68k_dreg(regs, dstreg);
	CLEAR_CZNV (&regs->ccrflags);
	if (src == 0) {
		SET_VFLG (&regs->ccrflags, 1);
		SET_ZFLG (&regs->ccrflags, 1);
		m68k_incpc (regs, 2);
		Exception (5, regs, oldpc);
		return 42 * CYCLE_UNIT / 2;
	} else {
		uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
		uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
		if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) {
			SET_VFLG (&regs->ccrflags, 1);
			SET_NFLG (&regs->ccrflags, 1);
		} else {
			if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg(regs, dstreg) = (newv);
		}
	m68k_incpc (regs, 2);
	}
return (getDivs68kCycles(dst, src) + 4) * CYCLE_UNIT / 2;
}
/* DIVS.W -(An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_81e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc(regs);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
	uae_s32 dst = m68k_dreg(regs, dstreg);
	CLEAR_CZNV (&regs->ccrflags);
	if (src == 0) {
		SET_VFLG (&regs->ccrflags, 1);
		SET_ZFLG (&regs->ccrflags, 1);
		m68k_incpc (regs, 2);
		Exception (5, regs, oldpc);
		return 44 * CYCLE_UNIT / 2;
	} else {
		uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
		uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
		if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) {
			SET_VFLG (&regs->ccrflags, 1);
			SET_NFLG (&regs->ccrflags, 1);
		} else {
			if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg(regs, dstreg) = (newv);
		}
	m68k_incpc (regs, 2);
	}
return (getDivs68kCycles(dst, src) + 6) * CYCLE_UNIT / 2;
}
/* DIVS.W (d16,An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_81e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc(regs);
	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
	uae_s16 src = get_word (srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	CLEAR_CZNV (&regs->ccrflags);
	if (src == 0) {
		SET_VFLG (&regs->ccrflags, 1);
		SET_ZFLG (&regs->ccrflags, 1);
		m68k_incpc (regs, 4);
		Exception (5, regs, oldpc);
		return 46 * CYCLE_UNIT / 2;
	} else {
		uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
		uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
		if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) {
			SET_VFLG (&regs->ccrflags, 1);
			SET_NFLG (&regs->ccrflags, 1);
		} else {
			if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg(regs, dstreg) = (newv);
		}
	m68k_incpc (regs, 4);
	}
return (getDivs68kCycles(dst, src) + 8) * CYCLE_UNIT / 2;
}
/* DIVS.W (d8,An,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_81f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc(regs);
	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
	uae_s16 src = get_word (srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	CLEAR_CZNV (&regs->ccrflags);
	if (src == 0) {
		SET_VFLG (&regs->ccrflags, 1);
		SET_ZFLG (&regs->ccrflags, 1);
		m68k_incpc (regs, 4);
		Exception (5, regs, oldpc);
		return 48 * CYCLE_UNIT / 2;
	} else {
		uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
		uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
		if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) {
			SET_VFLG (&regs->ccrflags, 1);
			SET_NFLG (&regs->ccrflags, 1);
		} else {
			if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg(regs, dstreg) = (newv);
		}
	m68k_incpc (regs, 4);
	}
return (getDivs68kCycles(dst, src) + 10) * CYCLE_UNIT / 2;
}
/* DIVS.W (xxx).W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_81f8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc(regs);
	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
	uae_s16 src = get_word (srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	CLEAR_CZNV (&regs->ccrflags);
	if (src == 0) {
		SET_VFLG (&regs->ccrflags, 1);
		SET_ZFLG (&regs->ccrflags, 1);
		m68k_incpc (regs, 4);
		Exception (5, regs, oldpc);
		return 46 * CYCLE_UNIT / 2;
	} else {
		uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
		uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
		if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) {
			SET_VFLG (&regs->ccrflags, 1);
			SET_NFLG (&regs->ccrflags, 1);
		} else {
			if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg(regs, dstreg) = (newv);
		}
	m68k_incpc (regs, 4);
	}
return (getDivs68kCycles(dst, src) + 8) * CYCLE_UNIT / 2;
}
/* DIVS.W (xxx).L,Dn */
unsigned long REGPARAM2 CPUFUNC(op_81f9_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc(regs);
	uaecptr srca = get_ilong (regs, 2);
	uae_s16 src = get_word (srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	CLEAR_CZNV (&regs->ccrflags);
	if (src == 0) {
		SET_VFLG (&regs->ccrflags, 1);
		SET_ZFLG (&regs->ccrflags, 1);
		m68k_incpc (regs, 6);
		Exception (5, regs, oldpc);
		return 50 * CYCLE_UNIT / 2;
	} else {
		uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
		uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
		if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) {
			SET_VFLG (&regs->ccrflags, 1);
			SET_NFLG (&regs->ccrflags, 1);
		} else {
			if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg(regs, dstreg) = (newv);
		}
	m68k_incpc (regs, 6);
	}
return (getDivs68kCycles(dst, src) + 12) * CYCLE_UNIT / 2;
}
/* DIVS.W (d16,PC),Dn */
unsigned long REGPARAM2 CPUFUNC(op_81fa_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc(regs);
	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
	uae_s16 src = get_word (srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	CLEAR_CZNV (&regs->ccrflags);
	if (src == 0) {
		SET_VFLG (&regs->ccrflags, 1);
		SET_ZFLG (&regs->ccrflags, 1);
		m68k_incpc (regs, 4);
		Exception (5, regs, oldpc);
		return 46 * CYCLE_UNIT / 2;
	} else {
		uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
		uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
		if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) {
			SET_VFLG (&regs->ccrflags, 1);
			SET_NFLG (&regs->ccrflags, 1);
		} else {
			if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg(regs, dstreg) = (newv);
		}
	m68k_incpc (regs, 4);
	}
return (getDivs68kCycles(dst, src) + 8) * CYCLE_UNIT / 2;
}
/* DIVS.W (d8,PC,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_81fb_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc(regs);
	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
	uae_s16 src = get_word (srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	CLEAR_CZNV (&regs->ccrflags);
	if (src == 0) {
		SET_VFLG (&regs->ccrflags, 1);
		SET_ZFLG (&regs->ccrflags, 1);
		m68k_incpc (regs, 4);
		Exception (5, regs, oldpc);
		return 48 * CYCLE_UNIT / 2;
	} else {
		uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
		uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
		if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) {
			SET_VFLG (&regs->ccrflags, 1);
			SET_NFLG (&regs->ccrflags, 1);
		} else {
			if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg(regs, dstreg) = (newv);
		}
	m68k_incpc (regs, 4);
	}
return (getDivs68kCycles(dst, src) + 10) * CYCLE_UNIT / 2;
}
/* DIVS.W #<data>.W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_81fc_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc(regs);
	uae_s16 src = get_iword (regs, 2);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	CLEAR_CZNV (&regs->ccrflags);
	if (src == 0) {
		SET_VFLG (&regs->ccrflags, 1);
		SET_ZFLG (&regs->ccrflags, 1);
		m68k_incpc (regs, 4);
		Exception (5, regs, oldpc);
		return 42 * CYCLE_UNIT / 2;
	} else {
		uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
		uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
		if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) {
			SET_VFLG (&regs->ccrflags, 1);
			SET_NFLG (&regs->ccrflags, 1);
		} else {
			if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg(regs, dstreg) = (newv);
		}
	m68k_incpc (regs, 4);
	}
return (getDivs68kCycles(dst, src) + 4) * CYCLE_UNIT / 2;
}
/* SUB.B Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_9000_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* SUB.B (An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_9010_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* SUB.B (An)+,Dn */
unsigned long REGPARAM2 CPUFUNC(op_9018_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* SUB.B -(An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_9020_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}	m68k_incpc (regs, 2);
return 10 * CYCLE_UNIT / 2;
}
/* SUB.B (d16,An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_9028_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* SUB.B (d8,An,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_9030_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}	m68k_incpc (regs, 4);
return 14 * CYCLE_UNIT / 2;
}
/* SUB.B (xxx).W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_9038_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* SUB.B (xxx).L,Dn */
unsigned long REGPARAM2 CPUFUNC(op_9039_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}	m68k_incpc (regs, 6);
return 16 * CYCLE_UNIT / 2;
}
/* SUB.B (d16,PC),Dn */
unsigned long REGPARAM2 CPUFUNC(op_903a_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* SUB.B (d8,PC,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_903b_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}	m68k_incpc (regs, 4);
return 14 * CYCLE_UNIT / 2;
}
/* SUB.B #<data>.B,Dn */
unsigned long REGPARAM2 CPUFUNC(op_903c_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}	m68k_incpc (regs, 4);
return 8 * CYCLE_UNIT / 2;
}
/* SUB.W Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_9040_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* SUB.W An,Dn */
unsigned long REGPARAM2 CPUFUNC(op_9048_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_areg(regs, srcreg);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* SUB.W (An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_9050_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* SUB.W (An)+,Dn */
unsigned long REGPARAM2 CPUFUNC(op_9058_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg(regs, srcreg) += 2;
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* SUB.W -(An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_9060_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}	m68k_incpc (regs, 2);
return 10 * CYCLE_UNIT / 2;
}
/* SUB.W (d16,An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_9068_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* SUB.W (d8,An,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_9070_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}	m68k_incpc (regs, 4);
return 14 * CYCLE_UNIT / 2;
}
/* SUB.W (xxx).W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_9078_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* SUB.W (xxx).L,Dn */
unsigned long REGPARAM2 CPUFUNC(op_9079_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}	m68k_incpc (regs, 6);
return 16 * CYCLE_UNIT / 2;
}
/* SUB.W (d16,PC),Dn */
unsigned long REGPARAM2 CPUFUNC(op_907a_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* SUB.W (d8,PC,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_907b_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}	m68k_incpc (regs, 4);
return 14 * CYCLE_UNIT / 2;
}
/* SUB.W #<data>.W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_907c_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}	m68k_incpc (regs, 4);
return 8 * CYCLE_UNIT / 2;
}
/* SUB.L Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_9080_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
}}}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* SUB.L An,Dn */
unsigned long REGPARAM2 CPUFUNC(op_9088_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_areg(regs, srcreg);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
}}}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* SUB.L (An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_9090_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
}}}}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* SUB.L (An)+,Dn */
unsigned long REGPARAM2 CPUFUNC(op_9098_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg(regs, srcreg) += 4;
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
}}}}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* SUB.L -(An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_90a0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
}}}}}}}	m68k_incpc (regs, 2);
return 16 * CYCLE_UNIT / 2;
}
/* SUB.L (d16,An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_90a8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
}}}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* SUB.L (d8,An,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_90b0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
}}}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* SUB.L (xxx).W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_90b8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
}}}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* SUB.L (xxx).L,Dn */
unsigned long REGPARAM2 CPUFUNC(op_90b9_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
}}}}}}}	m68k_incpc (regs, 6);
return 22 * CYCLE_UNIT / 2;
}
/* SUB.L (d16,PC),Dn */
unsigned long REGPARAM2 CPUFUNC(op_90ba_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
}}}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* SUB.L (d8,PC,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_90bb_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
}}}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* SUB.L #<data>.L,Dn */
unsigned long REGPARAM2 CPUFUNC(op_90bc_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
}}}}}}	m68k_incpc (regs, 6);
return 16 * CYCLE_UNIT / 2;
}
/* SUBA.W Dn,An */
unsigned long REGPARAM2 CPUFUNC(op_90c0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* SUBA.W An,An */
unsigned long REGPARAM2 CPUFUNC(op_90c8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_areg(regs, srcreg);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* SUBA.W (An),An */
unsigned long REGPARAM2 CPUFUNC(op_90d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
}}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* SUBA.W (An)+,An */
unsigned long REGPARAM2 CPUFUNC(op_90d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg(regs, srcreg) += 2;
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
}}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* SUBA.W -(An),An */
unsigned long REGPARAM2 CPUFUNC(op_90e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
}}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* SUBA.W (d16,An),An */
unsigned long REGPARAM2 CPUFUNC(op_90e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
}}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* SUBA.W (d8,An,Xn),An */
unsigned long REGPARAM2 CPUFUNC(op_90f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* SUBA.W (xxx).W,An */
unsigned long REGPARAM2 CPUFUNC(op_90f8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
}}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* SUBA.W (xxx).L,An */
unsigned long REGPARAM2 CPUFUNC(op_90f9_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
}}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* SUBA.W (d16,PC),An */
unsigned long REGPARAM2 CPUFUNC(op_90fa_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
}}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* SUBA.W (d8,PC,Xn),An */
unsigned long REGPARAM2 CPUFUNC(op_90fb_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* SUBA.W #<data>.W,An */
unsigned long REGPARAM2 CPUFUNC(op_90fc_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* SUBX.B Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_9100_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = dst - src - (GET_XFLG (&regs->ccrflags) ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s8)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s8)(newv)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* SUBX.B -(An),-(An) */
unsigned long REGPARAM2 CPUFUNC(op_9108_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
{	uae_u32 newv = dst - src - (GET_XFLG (&regs->ccrflags) ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s8)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s8)(newv)) < 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 18 * CYCLE_UNIT / 2;
}
/* SUB.B Dn,(An) */
unsigned long REGPARAM2 CPUFUNC(op_9110_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* SUB.B Dn,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_9118_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* SUB.B Dn,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_9120_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* SUB.B Dn,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_9128_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* SUB.B Dn,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_9130_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 2));
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* SUB.B Dn,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_9138_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* SUB.B Dn,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_9139_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = get_ilong (regs, 2);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* SUBX.W Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_9140_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{	uae_u32 newv = dst - src - (GET_XFLG (&regs->ccrflags) ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s16)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s16)(newv)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* SUBX.W -(An),-(An) */
unsigned long REGPARAM2 CPUFUNC(op_9148_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) = dsta;
{	uae_u32 newv = dst - src - (GET_XFLG (&regs->ccrflags) ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s16)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s16)(newv)) < 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 18 * CYCLE_UNIT / 2;
}
/* SUB.W Dn,(An) */
unsigned long REGPARAM2 CPUFUNC(op_9150_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* SUB.W Dn,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_9158_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s16 dst = get_word (dsta);
	m68k_areg(regs, dstreg) += 2;
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* SUB.W Dn,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_9160_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* SUB.W Dn,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_9168_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* SUB.W Dn,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_9170_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 2));
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* SUB.W Dn,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_9178_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* SUB.W Dn,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_9179_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = get_ilong (regs, 2);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* SUBX.L Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_9180_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{	uae_u32 newv = dst - src - (GET_XFLG (&regs->ccrflags) ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s32)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s32)(newv)) < 0);
	m68k_dreg(regs, dstreg) = (newv);
}}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* SUBX.L -(An),-(An) */
unsigned long REGPARAM2 CPUFUNC(op_9188_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) = dsta;
{	uae_u32 newv = dst - src - (GET_XFLG (&regs->ccrflags) ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s32)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s32)(newv)) < 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 30 * CYCLE_UNIT / 2;
}
/* SUB.L Dn,(An) */
unsigned long REGPARAM2 CPUFUNC(op_9190_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 20 * CYCLE_UNIT / 2;
}
/* SUB.L Dn,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_9198_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s32 dst = get_long (dsta);
	m68k_areg(regs, dstreg) += 4;
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 20 * CYCLE_UNIT / 2;
}
/* SUB.L Dn,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_91a0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 22 * CYCLE_UNIT / 2;
}
/* SUB.L Dn,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_91a8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 24 * CYCLE_UNIT / 2;
}
/* SUB.L Dn,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_91b0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 2));
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 26 * CYCLE_UNIT / 2;
}
/* SUB.L Dn,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_91b8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 24 * CYCLE_UNIT / 2;
}
/* SUB.L Dn,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_91b9_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = get_ilong (regs, 2);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 6);
return 28 * CYCLE_UNIT / 2;
}
/* SUBA.L Dn,An */
unsigned long REGPARAM2 CPUFUNC(op_91c0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* SUBA.L An,An */
unsigned long REGPARAM2 CPUFUNC(op_91c8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_areg(regs, srcreg);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* SUBA.L (An),An */
unsigned long REGPARAM2 CPUFUNC(op_91d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
}}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* SUBA.L (An)+,An */
unsigned long REGPARAM2 CPUFUNC(op_91d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg(regs, srcreg) += 4;
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
}}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* SUBA.L -(An),An */
unsigned long REGPARAM2 CPUFUNC(op_91e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
}}}}}	m68k_incpc (regs, 2);
return 16 * CYCLE_UNIT / 2;
}
/* SUBA.L (d16,An),An */
unsigned long REGPARAM2 CPUFUNC(op_91e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* SUBA.L (d8,An,Xn),An */
unsigned long REGPARAM2 CPUFUNC(op_91f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
}}}}}	m68k_incpc (regs, 4);
return 20 * CYCLE_UNIT / 2;
}
/* SUBA.L (xxx).W,An */
unsigned long REGPARAM2 CPUFUNC(op_91f8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* SUBA.L (xxx).L,An */
unsigned long REGPARAM2 CPUFUNC(op_91f9_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
}}}}}	m68k_incpc (regs, 6);
return 22 * CYCLE_UNIT / 2;
}
/* SUBA.L (d16,PC),An */
unsigned long REGPARAM2 CPUFUNC(op_91fa_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* SUBA.L (d8,PC,Xn),An */
unsigned long REGPARAM2 CPUFUNC(op_91fb_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
}}}}}	m68k_incpc (regs, 4);
return 20 * CYCLE_UNIT / 2;
}
/* SUBA.L #<data>.L,An */
unsigned long REGPARAM2 CPUFUNC(op_91fc_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
}}}}	m68k_incpc (regs, 6);
return 16 * CYCLE_UNIT / 2;
}
/* CMP.B Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_b000_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* CMP.B (An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_b010_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* CMP.B (An)+,Dn */
unsigned long REGPARAM2 CPUFUNC(op_b018_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* CMP.B -(An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_b020_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 2);
return 10 * CYCLE_UNIT / 2;
}
/* CMP.B (d16,An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_b028_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* CMP.B (d8,An,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_b030_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 4);
return 14 * CYCLE_UNIT / 2;
}
/* CMP.B (xxx).W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_b038_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* CMP.B (xxx).L,Dn */
unsigned long REGPARAM2 CPUFUNC(op_b039_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 6);
return 16 * CYCLE_UNIT / 2;
}
/* CMP.B (d16,PC),Dn */
unsigned long REGPARAM2 CPUFUNC(op_b03a_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* CMP.B (d8,PC,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_b03b_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 4);
return 14 * CYCLE_UNIT / 2;
}
/* CMP.B #<data>.B,Dn */
unsigned long REGPARAM2 CPUFUNC(op_b03c_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}	m68k_incpc (regs, 4);
return 8 * CYCLE_UNIT / 2;
}
/* CMP.W Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_b040_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* CMP.W An,Dn */
unsigned long REGPARAM2 CPUFUNC(op_b048_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_areg(regs, srcreg);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* CMP.W (An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_b050_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* CMP.W (An)+,Dn */
unsigned long REGPARAM2 CPUFUNC(op_b058_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg(regs, srcreg) += 2;
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* CMP.W -(An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_b060_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 2);
return 10 * CYCLE_UNIT / 2;
}
/* CMP.W (d16,An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_b068_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* CMP.W (d8,An,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_b070_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 4);
return 14 * CYCLE_UNIT / 2;
}
/* CMP.W (xxx).W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_b078_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* CMP.W (xxx).L,Dn */
unsigned long REGPARAM2 CPUFUNC(op_b079_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 6);
return 16 * CYCLE_UNIT / 2;
}
/* CMP.W (d16,PC),Dn */
unsigned long REGPARAM2 CPUFUNC(op_b07a_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* CMP.W (d8,PC,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_b07b_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 4);
return 14 * CYCLE_UNIT / 2;
}
/* CMP.W #<data>.W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_b07c_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}	m68k_incpc (regs, 4);
return 8 * CYCLE_UNIT / 2;
}
/* CMP.L Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_b080_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}	m68k_incpc (regs, 2);
return 6 * CYCLE_UNIT / 2;
}
/* CMP.L An,Dn */
unsigned long REGPARAM2 CPUFUNC(op_b088_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_areg(regs, srcreg);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}	m68k_incpc (regs, 2);
return 6 * CYCLE_UNIT / 2;
}
/* CMP.L (An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_b090_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* CMP.L (An)+,Dn */
unsigned long REGPARAM2 CPUFUNC(op_b098_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg(regs, srcreg) += 4;
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* CMP.L -(An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_b0a0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 2);
return 16 * CYCLE_UNIT / 2;
}
/* CMP.L (d16,An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_b0a8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* CMP.L (d8,An,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_b0b0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* CMP.L (xxx).W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_b0b8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* CMP.L (xxx).L,Dn */
unsigned long REGPARAM2 CPUFUNC(op_b0b9_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 6);
return 22 * CYCLE_UNIT / 2;
}
/* CMP.L (d16,PC),Dn */
unsigned long REGPARAM2 CPUFUNC(op_b0ba_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* CMP.L (d8,PC,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_b0bb_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* CMP.L #<data>.L,Dn */
unsigned long REGPARAM2 CPUFUNC(op_b0bc_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}	m68k_incpc (regs, 6);
return 14 * CYCLE_UNIT / 2;
}
/* CMPA.W Dn,An */
unsigned long REGPARAM2 CPUFUNC(op_b0c0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}	m68k_incpc (regs, 2);
return 6 * CYCLE_UNIT / 2;
}
/* CMPA.W An,An */
unsigned long REGPARAM2 CPUFUNC(op_b0c8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_areg(regs, srcreg);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}	m68k_incpc (regs, 2);
return 6 * CYCLE_UNIT / 2;
}
/* CMPA.W (An),An */
unsigned long REGPARAM2 CPUFUNC(op_b0d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 2);
return 10 * CYCLE_UNIT / 2;
}
/* CMPA.W (An)+,An */
unsigned long REGPARAM2 CPUFUNC(op_b0d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg(regs, srcreg) += 2;
{	uae_s32 dst = m68k_areg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 2);
return 10 * CYCLE_UNIT / 2;
}
/* CMPA.W -(An),An */
unsigned long REGPARAM2 CPUFUNC(op_b0e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s32 dst = m68k_areg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* CMPA.W (d16,An),An */
unsigned long REGPARAM2 CPUFUNC(op_b0e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 4);
return 14 * CYCLE_UNIT / 2;
}
/* CMPA.W (d8,An,Xn),An */
unsigned long REGPARAM2 CPUFUNC(op_b0f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* CMPA.W (xxx).W,An */
unsigned long REGPARAM2 CPUFUNC(op_b0f8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 4);
return 14 * CYCLE_UNIT / 2;
}
/* CMPA.W (xxx).L,An */
unsigned long REGPARAM2 CPUFUNC(op_b0f9_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 6);
return 18 * CYCLE_UNIT / 2;
}
/* CMPA.W (d16,PC),An */
unsigned long REGPARAM2 CPUFUNC(op_b0fa_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 4);
return 14 * CYCLE_UNIT / 2;
}
/* CMPA.W (d8,PC,Xn),An */
unsigned long REGPARAM2 CPUFUNC(op_b0fb_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* CMPA.W #<data>.W,An */
unsigned long REGPARAM2 CPUFUNC(op_b0fc_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}	m68k_incpc (regs, 4);
return 10 * CYCLE_UNIT / 2;
}
/* EOR.B Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_b100_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* CMPM.B (An)+,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_b108_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* EOR.B Dn,(An) */
unsigned long REGPARAM2 CPUFUNC(op_b110_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* EOR.B Dn,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_b118_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* EOR.B Dn,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_b120_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* EOR.B Dn,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_b128_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 dst = get_byte (dsta);
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* EOR.B Dn,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_b130_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 2));
{	uae_s8 dst = get_byte (dsta);
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* EOR.B Dn,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_b138_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 dst = get_byte (dsta);
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* EOR.B Dn,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_b139_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = get_ilong (regs, 2);
{	uae_s8 dst = get_byte (dsta);
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* EOR.W Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_b140_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* CMPM.W (An)+,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_b148_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg(regs, srcreg) += 2;
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s16 dst = get_word (dsta);
	m68k_areg(regs, dstreg) += 2;
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* EOR.W Dn,(An) */
unsigned long REGPARAM2 CPUFUNC(op_b150_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s16 dst = get_word (dsta);
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* EOR.W Dn,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_b158_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s16 dst = get_word (dsta);
	m68k_areg(regs, dstreg) += 2;
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* EOR.W Dn,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_b160_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* EOR.W Dn,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_b168_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 dst = get_word (dsta);
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* EOR.W Dn,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_b170_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 2));
{	uae_s16 dst = get_word (dsta);
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* EOR.W Dn,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_b178_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 dst = get_word (dsta);
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* EOR.W Dn,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_b179_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = get_ilong (regs, 2);
{	uae_s16 dst = get_word (dsta);
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* EOR.L Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_b180_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* CMPM.L (An)+,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_b188_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg(regs, srcreg) += 4;
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s32 dst = get_long (dsta);
	m68k_areg(regs, dstreg) += 4;
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}}	m68k_incpc (regs, 2);
return 20 * CYCLE_UNIT / 2;
}
/* EOR.L Dn,(An) */
unsigned long REGPARAM2 CPUFUNC(op_b190_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s32 dst = get_long (dsta);
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 2);
return 20 * CYCLE_UNIT / 2;
}
/* EOR.L Dn,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_b198_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s32 dst = get_long (dsta);
	m68k_areg(regs, dstreg) += 4;
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 2);
return 20 * CYCLE_UNIT / 2;
}
/* EOR.L Dn,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_b1a0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 2);
return 22 * CYCLE_UNIT / 2;
}
/* EOR.L Dn,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_b1a8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 dst = get_long (dsta);
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 24 * CYCLE_UNIT / 2;
}
/* EOR.L Dn,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_b1b0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 2));
{	uae_s32 dst = get_long (dsta);
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 26 * CYCLE_UNIT / 2;
}
/* EOR.L Dn,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_b1b8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 dst = get_long (dsta);
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 24 * CYCLE_UNIT / 2;
}
/* EOR.L Dn,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_b1b9_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = get_ilong (regs, 2);
{	uae_s32 dst = get_long (dsta);
	src ^= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 6);
return 28 * CYCLE_UNIT / 2;
}
/* CMPA.L Dn,An */
unsigned long REGPARAM2 CPUFUNC(op_b1c0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}	m68k_incpc (regs, 2);
return 6 * CYCLE_UNIT / 2;
}
/* CMPA.L An,An */
unsigned long REGPARAM2 CPUFUNC(op_b1c8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_areg(regs, srcreg);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}	m68k_incpc (regs, 2);
return 6 * CYCLE_UNIT / 2;
}
/* CMPA.L (An),An */
unsigned long REGPARAM2 CPUFUNC(op_b1d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* CMPA.L (An)+,An */
unsigned long REGPARAM2 CPUFUNC(op_b1d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg(regs, srcreg) += 4;
{	uae_s32 dst = m68k_areg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* CMPA.L -(An),An */
unsigned long REGPARAM2 CPUFUNC(op_b1e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s32 dst = m68k_areg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 2);
return 16 * CYCLE_UNIT / 2;
}
/* CMPA.L (d16,An),An */
unsigned long REGPARAM2 CPUFUNC(op_b1e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* CMPA.L (d8,An,Xn),An */
unsigned long REGPARAM2 CPUFUNC(op_b1f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 4);
return 20 * CYCLE_UNIT / 2;
}
/* CMPA.L (xxx).W,An */
unsigned long REGPARAM2 CPUFUNC(op_b1f8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* CMPA.L (xxx).L,An */
unsigned long REGPARAM2 CPUFUNC(op_b1f9_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 6);
return 22 * CYCLE_UNIT / 2;
}
/* CMPA.L (d16,PC),An */
unsigned long REGPARAM2 CPUFUNC(op_b1fa_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* CMPA.L (d8,PC,Xn),An */
unsigned long REGPARAM2 CPUFUNC(op_b1fb_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}}	m68k_incpc (regs, 4);
return 20 * CYCLE_UNIT / 2;
}
/* CMPA.L #<data>.L,An */
unsigned long REGPARAM2 CPUFUNC(op_b1fc_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs != flgo) && (flgn != flgo));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (&regs->ccrflags, flgn != 0);
}}}}}}	m68k_incpc (regs, 6);
return 14 * CYCLE_UNIT / 2;
}
/* AND.B Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_c000_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* AND.B (An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_c010_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* AND.B (An)+,Dn */
unsigned long REGPARAM2 CPUFUNC(op_c018_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* AND.B -(An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_c020_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (regs, 2);
return 10 * CYCLE_UNIT / 2;
}
/* AND.B (d16,An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_c028_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* AND.B (d8,An,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_c030_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (regs, 4);
return 14 * CYCLE_UNIT / 2;
}
/* AND.B (xxx).W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_c038_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* AND.B (xxx).L,Dn */
unsigned long REGPARAM2 CPUFUNC(op_c039_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (regs, 6);
return 16 * CYCLE_UNIT / 2;
}
/* AND.B (d16,PC),Dn */
unsigned long REGPARAM2 CPUFUNC(op_c03a_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* AND.B (d8,PC,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_c03b_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (regs, 4);
return 14 * CYCLE_UNIT / 2;
}
/* AND.B #<data>.B,Dn */
unsigned long REGPARAM2 CPUFUNC(op_c03c_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}	m68k_incpc (regs, 4);
return 8 * CYCLE_UNIT / 2;
}
/* AND.W Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_c040_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* AND.W (An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_c050_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* AND.W (An)+,Dn */
unsigned long REGPARAM2 CPUFUNC(op_c058_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg(regs, srcreg) += 2;
{	uae_s16 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* AND.W -(An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_c060_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s16 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (regs, 2);
return 10 * CYCLE_UNIT / 2;
}
/* AND.W (d16,An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_c068_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* AND.W (d8,An,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_c070_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (regs, 4);
return 14 * CYCLE_UNIT / 2;
}
/* AND.W (xxx).W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_c078_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* AND.W (xxx).L,Dn */
unsigned long REGPARAM2 CPUFUNC(op_c079_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (regs, 6);
return 16 * CYCLE_UNIT / 2;
}
/* AND.W (d16,PC),Dn */
unsigned long REGPARAM2 CPUFUNC(op_c07a_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* AND.W (d8,PC,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_c07b_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (regs, 4);
return 14 * CYCLE_UNIT / 2;
}
/* AND.W #<data>.W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_c07c_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}	m68k_incpc (regs, 4);
return 8 * CYCLE_UNIT / 2;
}
/* AND.L Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_c080_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* AND.L (An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_c090_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* AND.L (An)+,Dn */
unsigned long REGPARAM2 CPUFUNC(op_c098_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg(regs, srcreg) += 4;
{	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* AND.L -(An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_c0a0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
}}}}	m68k_incpc (regs, 2);
return 16 * CYCLE_UNIT / 2;
}
/* AND.L (d16,An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_c0a8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* AND.L (d8,An,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_c0b0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* AND.L (xxx).W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_c0b8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* AND.L (xxx).L,Dn */
unsigned long REGPARAM2 CPUFUNC(op_c0b9_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
}}}}	m68k_incpc (regs, 6);
return 22 * CYCLE_UNIT / 2;
}
/* AND.L (d16,PC),Dn */
unsigned long REGPARAM2 CPUFUNC(op_c0ba_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* AND.L (d8,PC,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_c0bb_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* AND.L #<data>.L,Dn */
unsigned long REGPARAM2 CPUFUNC(op_c0bc_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
}}}	m68k_incpc (regs, 6);
return 16 * CYCLE_UNIT / 2;
}
/* MULU.W Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_c0c0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(newv)) < 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc (regs, 2);
}}return (38 + bitset_count(src & 0xFFFF) * 2) * CYCLE_UNIT / 2;
}
/* MULU.W (An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_c0d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(newv)) < 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc (regs, 2);
}}return (42 + bitset_count(src & 0xFFFF) * 2) * CYCLE_UNIT / 2;
}
/* MULU.W (An)+,Dn */
unsigned long REGPARAM2 CPUFUNC(op_c0d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word (srca);
	m68k_areg(regs, srcreg) += 2;
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(newv)) < 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc (regs, 2);
}}return (42 + bitset_count(src & 0xFFFF) * 2) * CYCLE_UNIT / 2;
}
/* MULU.W -(An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_c0e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(newv)) < 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc (regs, 2);
}}return (44 + bitset_count(src & 0xFFFF) * 2) * CYCLE_UNIT / 2;
}
/* MULU.W (d16,An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_c0e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(newv)) < 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc (regs, 4);
}}return (46 + bitset_count(src & 0xFFFF) * 2) * CYCLE_UNIT / 2;
}
/* MULU.W (d8,An,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_c0f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(newv)) < 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc (regs, 4);
}}return (48 + bitset_count(src & 0xFFFF) * 2) * CYCLE_UNIT / 2;
}
/* MULU.W (xxx).W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_c0f8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(newv)) < 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc (regs, 4);
}}return (46 + bitset_count(src & 0xFFFF) * 2) * CYCLE_UNIT / 2;
}
/* MULU.W (xxx).L,Dn */
unsigned long REGPARAM2 CPUFUNC(op_c0f9_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca = get_ilong (regs, 2);
	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(newv)) < 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc (regs, 6);
}}return (50 + bitset_count(src & 0xFFFF) * 2) * CYCLE_UNIT / 2;
}
/* MULU.W (d16,PC),Dn */
unsigned long REGPARAM2 CPUFUNC(op_c0fa_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(newv)) < 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc (regs, 4);
}}return (46 + bitset_count(src & 0xFFFF) * 2) * CYCLE_UNIT / 2;
}
/* MULU.W (d8,PC,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_c0fb_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(newv)) < 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc (regs, 4);
}}return (48 + bitset_count(src & 0xFFFF) * 2) * CYCLE_UNIT / 2;
}
/* MULU.W #<data>.W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_c0fc_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = get_iword (regs, 2);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(newv)) < 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc (regs, 4);
}}return (42 + bitset_count(src & 0xFFFF) * 2) * CYCLE_UNIT / 2;
}
/* ABCD.B Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_c100_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{	uae_u16 newv_lo = (src & 0xF) + (dst & 0xF) + (GET_XFLG (&regs->ccrflags) ? 1 : 0);
	uae_u16 newv_hi = (src & 0xF0) + (dst & 0xF0);
	uae_u16 newv, tmp_newv;
	int cflg;
	newv = tmp_newv = newv_hi + newv_lo;
	if (newv_lo > 9) { newv += 6; }
	cflg = (newv & 0x3F0) > 0x90;
	if (cflg) newv += 0x60;
	SET_CFLG (&regs->ccrflags, cflg);
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s8)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s8)(newv)) < 0);
	SET_VFLG (&regs->ccrflags, (tmp_newv & 0x80) == 0 && (newv & 0x80) != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}	m68k_incpc (regs, 2);
return 6 * CYCLE_UNIT / 2;
}
/* ABCD.B -(An),-(An) */
unsigned long REGPARAM2 CPUFUNC(op_c108_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
{	uae_u16 newv_lo = (src & 0xF) + (dst & 0xF) + (GET_XFLG (&regs->ccrflags) ? 1 : 0);
	uae_u16 newv_hi = (src & 0xF0) + (dst & 0xF0);
	uae_u16 newv, tmp_newv;
	int cflg;
	newv = tmp_newv = newv_hi + newv_lo;
	if (newv_lo > 9) { newv += 6; }
	cflg = (newv & 0x3F0) > 0x90;
	if (cflg) newv += 0x60;
	SET_CFLG (&regs->ccrflags, cflg);
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s8)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s8)(newv)) < 0);
	SET_VFLG (&regs->ccrflags, (tmp_newv & 0x80) == 0 && (newv & 0x80) != 0);
	put_byte (dsta,newv);
}}}}}}	m68k_incpc (regs, 2);
return 18 * CYCLE_UNIT / 2;
}
/* AND.B Dn,(An) */
unsigned long REGPARAM2 CPUFUNC(op_c110_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* AND.B Dn,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_c118_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* AND.B Dn,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_c120_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* AND.B Dn,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_c128_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 dst = get_byte (dsta);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* AND.B Dn,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_c130_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 2));
{	uae_s8 dst = get_byte (dsta);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* AND.B Dn,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_c138_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 dst = get_byte (dsta);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* AND.B Dn,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_c139_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = get_ilong (regs, 2);
{	uae_s8 dst = get_byte (dsta);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s8)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s8)(src)) < 0);
	put_byte (dsta,src);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* EXG.L Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_c140_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
	m68k_dreg(regs, srcreg) = (dst);
	m68k_dreg(regs, dstreg) = (src);
}}}	m68k_incpc (regs, 2);
return 6 * CYCLE_UNIT / 2;
}
/* EXG.L An,An */
unsigned long REGPARAM2 CPUFUNC(op_c148_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_areg(regs, srcreg);
{	uae_s32 dst = m68k_areg(regs, dstreg);
	m68k_areg(regs, srcreg) = (dst);
	m68k_areg(regs, dstreg) = (src);
}}}	m68k_incpc (regs, 2);
return 6 * CYCLE_UNIT / 2;
}
/* AND.W Dn,(An) */
unsigned long REGPARAM2 CPUFUNC(op_c150_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s16 dst = get_word (dsta);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* AND.W Dn,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_c158_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s16 dst = get_word (dsta);
	m68k_areg(regs, dstreg) += 2;
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* AND.W Dn,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_c160_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* AND.W Dn,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_c168_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 dst = get_word (dsta);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* AND.W Dn,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_c170_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 2));
{	uae_s16 dst = get_word (dsta);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* AND.W Dn,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_c178_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 dst = get_word (dsta);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* AND.W Dn,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_c179_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = get_ilong (regs, 2);
{	uae_s16 dst = get_word (dsta);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(src)) < 0);
	put_word (dsta,src);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* EXG.L Dn,An */
unsigned long REGPARAM2 CPUFUNC(op_c188_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uae_s32 dst = m68k_areg(regs, dstreg);
	m68k_dreg(regs, srcreg) = (dst);
	m68k_areg(regs, dstreg) = (src);
}}}	m68k_incpc (regs, 2);
return 6 * CYCLE_UNIT / 2;
}
/* AND.L Dn,(An) */
unsigned long REGPARAM2 CPUFUNC(op_c190_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s32 dst = get_long (dsta);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 2);
return 20 * CYCLE_UNIT / 2;
}
/* AND.L Dn,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_c198_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s32 dst = get_long (dsta);
	m68k_areg(regs, dstreg) += 4;
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 2);
return 20 * CYCLE_UNIT / 2;
}
/* AND.L Dn,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_c1a0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 2);
return 22 * CYCLE_UNIT / 2;
}
/* AND.L Dn,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_c1a8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 dst = get_long (dsta);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 24 * CYCLE_UNIT / 2;
}
/* AND.L Dn,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_c1b0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 2));
{	uae_s32 dst = get_long (dsta);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 26 * CYCLE_UNIT / 2;
}
/* AND.L Dn,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_c1b8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 dst = get_long (dsta);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 4);
return 24 * CYCLE_UNIT / 2;
}
/* AND.L Dn,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_c1b9_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = get_ilong (regs, 2);
{	uae_s32 dst = get_long (dsta);
	src &= dst;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(src)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(src)) < 0);
	put_long (dsta,src);
}}}}	m68k_incpc (regs, 6);
return 28 * CYCLE_UNIT / 2;
}
/* MULS.W Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_c1c0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(newv)) < 0);
	m68k_dreg(regs, dstreg) = (newv);
}}	m68k_incpc (regs, 2);
return (38 + bitset_count(src ^ (src << 1)) * 2) * CYCLE_UNIT / 2;
}
/* MULS.W (An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_c1d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(newv)) < 0);
	m68k_dreg(regs, dstreg) = (newv);
}}	m68k_incpc (regs, 2);
return (42 + bitset_count(src ^ (src << 1)) * 2) * CYCLE_UNIT / 2;
}
/* MULS.W (An)+,Dn */
unsigned long REGPARAM2 CPUFUNC(op_c1d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word (srca);
	m68k_areg(regs, srcreg) += 2;
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(newv)) < 0);
	m68k_dreg(regs, dstreg) = (newv);
}}	m68k_incpc (regs, 2);
return (42 + bitset_count(src ^ (src << 1)) * 2) * CYCLE_UNIT / 2;
}
/* MULS.W -(An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_c1e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(newv)) < 0);
	m68k_dreg(regs, dstreg) = (newv);
}}	m68k_incpc (regs, 2);
return (44 + bitset_count(src ^ (src << 1)) * 2) * CYCLE_UNIT / 2;
}
/* MULS.W (d16,An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_c1e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(newv)) < 0);
	m68k_dreg(regs, dstreg) = (newv);
}}	m68k_incpc (regs, 4);
return (46 + bitset_count(src ^ (src << 1)) * 2) * CYCLE_UNIT / 2;
}
/* MULS.W (d8,An,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_c1f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(newv)) < 0);
	m68k_dreg(regs, dstreg) = (newv);
}}	m68k_incpc (regs, 4);
return (48 + bitset_count(src ^ (src << 1)) * 2) * CYCLE_UNIT / 2;
}
/* MULS.W (xxx).W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_c1f8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(newv)) < 0);
	m68k_dreg(regs, dstreg) = (newv);
}}	m68k_incpc (regs, 4);
return (46 + bitset_count(src ^ (src << 1)) * 2) * CYCLE_UNIT / 2;
}
/* MULS.W (xxx).L,Dn */
unsigned long REGPARAM2 CPUFUNC(op_c1f9_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca = get_ilong (regs, 2);
	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(newv)) < 0);
	m68k_dreg(regs, dstreg) = (newv);
}}	m68k_incpc (regs, 6);
return (50 + bitset_count(src ^ (src << 1)) * 2) * CYCLE_UNIT / 2;
}
/* MULS.W (d16,PC),Dn */
unsigned long REGPARAM2 CPUFUNC(op_c1fa_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(newv)) < 0);
	m68k_dreg(regs, dstreg) = (newv);
}}	m68k_incpc (regs, 4);
return (46 + bitset_count(src ^ (src << 1)) * 2) * CYCLE_UNIT / 2;
}
/* MULS.W (d8,PC,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_c1fb_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(newv)) < 0);
	m68k_dreg(regs, dstreg) = (newv);
}}	m68k_incpc (regs, 4);
return (48 + bitset_count(src ^ (src << 1)) * 2) * CYCLE_UNIT / 2;
}
/* MULS.W #<data>.W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_c1fc_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = get_iword (regs, 2);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s32)(newv)) < 0);
	m68k_dreg(regs, dstreg) = (newv);
}}	m68k_incpc (regs, 4);
return (42 + bitset_count(src ^ (src << 1)) * 2) * CYCLE_UNIT / 2;
}
/* ADD.B Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_d000_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* ADD.B (An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_d010_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* ADD.B (An)+,Dn */
unsigned long REGPARAM2 CPUFUNC(op_d018_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s8 src = get_byte (srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* ADD.B -(An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_d020_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}	m68k_incpc (regs, 2);
return 10 * CYCLE_UNIT / 2;
}
/* ADD.B (d16,An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_d028_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* ADD.B (d8,An,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_d030_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}	m68k_incpc (regs, 4);
return 14 * CYCLE_UNIT / 2;
}
/* ADD.B (xxx).W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_d038_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* ADD.B (xxx).L,Dn */
unsigned long REGPARAM2 CPUFUNC(op_d039_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}	m68k_incpc (regs, 6);
return 16 * CYCLE_UNIT / 2;
}
/* ADD.B (d16,PC),Dn */
unsigned long REGPARAM2 CPUFUNC(op_d03a_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* ADD.B (d8,PC,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_d03b_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s8 src = get_byte (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}	m68k_incpc (regs, 4);
return 14 * CYCLE_UNIT / 2;
}
/* ADD.B #<data>.B,Dn */
unsigned long REGPARAM2 CPUFUNC(op_d03c_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = get_ibyte (regs, 2);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}	m68k_incpc (regs, 4);
return 8 * CYCLE_UNIT / 2;
}
/* ADD.W Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_d040_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* ADD.W An,Dn */
unsigned long REGPARAM2 CPUFUNC(op_d048_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_areg(regs, srcreg);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* ADD.W (An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_d050_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* ADD.W (An)+,Dn */
unsigned long REGPARAM2 CPUFUNC(op_d058_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg(regs, srcreg) += 2;
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* ADD.W -(An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_d060_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}	m68k_incpc (regs, 2);
return 10 * CYCLE_UNIT / 2;
}
/* ADD.W (d16,An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_d068_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* ADD.W (d8,An,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_d070_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}	m68k_incpc (regs, 4);
return 14 * CYCLE_UNIT / 2;
}
/* ADD.W (xxx).W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_d078_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* ADD.W (xxx).L,Dn */
unsigned long REGPARAM2 CPUFUNC(op_d079_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}	m68k_incpc (regs, 6);
return 16 * CYCLE_UNIT / 2;
}
/* ADD.W (d16,PC),Dn */
unsigned long REGPARAM2 CPUFUNC(op_d07a_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* ADD.W (d8,PC,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_d07b_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}	m68k_incpc (regs, 4);
return 14 * CYCLE_UNIT / 2;
}
/* ADD.W #<data>.W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_d07c_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}	m68k_incpc (regs, 4);
return 8 * CYCLE_UNIT / 2;
}
/* ADD.L Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_d080_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
}}}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* ADD.L An,Dn */
unsigned long REGPARAM2 CPUFUNC(op_d088_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_areg(regs, srcreg);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
}}}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* ADD.L (An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_d090_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
}}}}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* ADD.L (An)+,Dn */
unsigned long REGPARAM2 CPUFUNC(op_d098_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg(regs, srcreg) += 4;
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
}}}}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* ADD.L -(An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_d0a0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
}}}}}}}	m68k_incpc (regs, 2);
return 16 * CYCLE_UNIT / 2;
}
/* ADD.L (d16,An),Dn */
unsigned long REGPARAM2 CPUFUNC(op_d0a8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
}}}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* ADD.L (d8,An,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_d0b0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
}}}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* ADD.L (xxx).W,Dn */
unsigned long REGPARAM2 CPUFUNC(op_d0b8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
}}}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* ADD.L (xxx).L,Dn */
unsigned long REGPARAM2 CPUFUNC(op_d0b9_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
}}}}}}}	m68k_incpc (regs, 6);
return 22 * CYCLE_UNIT / 2;
}
/* ADD.L (d16,PC),Dn */
unsigned long REGPARAM2 CPUFUNC(op_d0ba_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
}}}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* ADD.L (d8,PC,Xn),Dn */
unsigned long REGPARAM2 CPUFUNC(op_d0bb_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
}}}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* ADD.L #<data>.L,Dn */
unsigned long REGPARAM2 CPUFUNC(op_d0bc_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
}}}}}}	m68k_incpc (regs, 6);
return 16 * CYCLE_UNIT / 2;
}
/* ADDA.W Dn,An */
unsigned long REGPARAM2 CPUFUNC(op_d0c0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* ADDA.W An,An */
unsigned long REGPARAM2 CPUFUNC(op_d0c8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_areg(regs, srcreg);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* ADDA.W (An),An */
unsigned long REGPARAM2 CPUFUNC(op_d0d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
}}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* ADDA.W (An)+,An */
unsigned long REGPARAM2 CPUFUNC(op_d0d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s16 src = get_word (srca);
	m68k_areg(regs, srcreg) += 2;
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
}}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* ADDA.W -(An),An */
unsigned long REGPARAM2 CPUFUNC(op_d0e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
}}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* ADDA.W (d16,An),An */
unsigned long REGPARAM2 CPUFUNC(op_d0e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
}}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* ADDA.W (d8,An,Xn),An */
unsigned long REGPARAM2 CPUFUNC(op_d0f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* ADDA.W (xxx).W,An */
unsigned long REGPARAM2 CPUFUNC(op_d0f8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
}}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* ADDA.W (xxx).L,An */
unsigned long REGPARAM2 CPUFUNC(op_d0f9_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
}}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* ADDA.W (d16,PC),An */
unsigned long REGPARAM2 CPUFUNC(op_d0fa_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
}}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* ADDA.W (d8,PC,Xn),An */
unsigned long REGPARAM2 CPUFUNC(op_d0fb_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s16 src = get_word (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* ADDA.W #<data>.W,An */
unsigned long REGPARAM2 CPUFUNC(op_d0fc_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_iword (regs, 2);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
}}}}	m68k_incpc (regs, 4);
return 12 * CYCLE_UNIT / 2;
}
/* ADDX.B Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_d100_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = dst + src + (GET_XFLG (&regs->ccrflags) ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, flgs ^ ((flgs ^ flgo) & (flgo ^ flgn)));
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s8)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s8)(newv)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* ADDX.B -(An),-(An) */
unsigned long REGPARAM2 CPUFUNC(op_d108_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
{	uae_u32 newv = dst + src + (GET_XFLG (&regs->ccrflags) ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, flgs ^ ((flgs ^ flgo) & (flgo ^ flgn)));
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s8)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s8)(newv)) < 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 18 * CYCLE_UNIT / 2;
}
/* ADD.B Dn,(An) */
unsigned long REGPARAM2 CPUFUNC(op_d110_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* ADD.B Dn,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_d118_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s8 dst = get_byte (dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* ADD.B Dn,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_d120_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* ADD.B Dn,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_d128_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* ADD.B Dn,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_d130_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 2));
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* ADD.B Dn,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_d138_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* ADD.B Dn,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_d139_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta = get_ilong (regs, 2);
{	uae_s8 dst = get_byte (dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_byte (dsta,newv);
}}}}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* ADDX.W Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_d140_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uae_s16 dst = m68k_dreg(regs, dstreg);
{	uae_u32 newv = dst + src + (GET_XFLG (&regs->ccrflags) ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, flgs ^ ((flgs ^ flgo) & (flgo ^ flgn)));
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s16)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s16)(newv)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}	m68k_incpc (regs, 2);
return 4 * CYCLE_UNIT / 2;
}
/* ADDX.W -(An),-(An) */
unsigned long REGPARAM2 CPUFUNC(op_d148_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
{	uae_s16 src = get_word (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) = dsta;
{	uae_u32 newv = dst + src + (GET_XFLG (&regs->ccrflags) ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, flgs ^ ((flgs ^ flgo) & (flgo ^ flgn)));
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s16)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s16)(newv)) < 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 18 * CYCLE_UNIT / 2;
}
/* ADD.W Dn,(An) */
unsigned long REGPARAM2 CPUFUNC(op_d150_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* ADD.W Dn,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_d158_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s16 dst = get_word (dsta);
	m68k_areg(regs, dstreg) += 2;
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* ADD.W Dn,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_d160_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
{	uae_s16 dst = get_word (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* ADD.W Dn,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_d168_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* ADD.W Dn,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_d170_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 2));
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* ADD.W Dn,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_d178_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* ADD.W Dn,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_d179_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s16 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = get_ilong (regs, 2);
{	uae_s16 dst = get_word (dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_word (dsta,newv);
}}}}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* ADDX.L Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_d180_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uae_s32 dst = m68k_dreg(regs, dstreg);
{	uae_u32 newv = dst + src + (GET_XFLG (&regs->ccrflags) ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, flgs ^ ((flgs ^ flgo) & (flgo ^ flgn)));
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s32)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s32)(newv)) < 0);
	m68k_dreg(regs, dstreg) = (newv);
}}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* ADDX.L -(An),-(An) */
unsigned long REGPARAM2 CPUFUNC(op_d188_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) = dsta;
{	uae_u32 newv = dst + src + (GET_XFLG (&regs->ccrflags) ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, flgs ^ ((flgs ^ flgo) & (flgo ^ flgn)));
	COPY_CARRY (&regs->ccrflags);
	SET_ZFLG (&regs->ccrflags, GET_ZFLG (&(regs->ccrflags)) & (((uae_s32)(newv)) == 0));
	SET_NFLG (&regs->ccrflags, ((uae_s32)(newv)) < 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 30 * CYCLE_UNIT / 2;
}
/* ADD.L Dn,(An) */
unsigned long REGPARAM2 CPUFUNC(op_d190_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 20 * CYCLE_UNIT / 2;
}
/* ADD.L Dn,(An)+ */
unsigned long REGPARAM2 CPUFUNC(op_d198_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg);
{	uae_s32 dst = get_long (dsta);
	m68k_areg(regs, dstreg) += 4;
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 20 * CYCLE_UNIT / 2;
}
/* ADD.L Dn,-(An) */
unsigned long REGPARAM2 CPUFUNC(op_d1a0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
{	uae_s32 dst = get_long (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 2);
return 22 * CYCLE_UNIT / 2;
}
/* ADD.L Dn,(d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_d1a8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 24 * CYCLE_UNIT / 2;
}
/* ADD.L Dn,(d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_d1b0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta;
	dsta = get_disp_ea_000(regs, m68k_areg(regs, dstreg), get_iword (regs, 2));
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 26 * CYCLE_UNIT / 2;
}
/* ADD.L Dn,(xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_d1b8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 4);
return 24 * CYCLE_UNIT / 2;
}
/* ADD.L Dn,(xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_d1b9_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uaecptr dsta = get_ilong (regs, 2);
{	uae_s32 dst = get_long (dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(newv)) == 0);
	SET_VFLG (&regs->ccrflags, (flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (&regs->ccrflags, ((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY (&regs->ccrflags);
	SET_NFLG (&regs->ccrflags, flgn != 0);
	put_long (dsta,newv);
}}}}}}}	m68k_incpc (regs, 6);
return 28 * CYCLE_UNIT / 2;
}
/* ADDA.L Dn,An */
unsigned long REGPARAM2 CPUFUNC(op_d1c0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg(regs, srcreg);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* ADDA.L An,An */
unsigned long REGPARAM2 CPUFUNC(op_d1c8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_areg(regs, srcreg);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
}}}}	m68k_incpc (regs, 2);
return 8 * CYCLE_UNIT / 2;
}
/* ADDA.L (An),An */
unsigned long REGPARAM2 CPUFUNC(op_d1d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
}}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* ADDA.L (An)+,An */
unsigned long REGPARAM2 CPUFUNC(op_d1d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg);
{	uae_s32 src = get_long (srca);
	m68k_areg(regs, srcreg) += 4;
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
}}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* ADDA.L -(An),An */
unsigned long REGPARAM2 CPUFUNC(op_d1e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
{	uae_s32 src = get_long (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
}}}}}	m68k_incpc (regs, 2);
return 16 * CYCLE_UNIT / 2;
}
/* ADDA.L (d16,An),An */
unsigned long REGPARAM2 CPUFUNC(op_d1e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* ADDA.L (d8,An,Xn),An */
unsigned long REGPARAM2 CPUFUNC(op_d1f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
}}}}}	m68k_incpc (regs, 4);
return 20 * CYCLE_UNIT / 2;
}
/* ADDA.L (xxx).W,An */
unsigned long REGPARAM2 CPUFUNC(op_d1f8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* ADDA.L (xxx).L,An */
unsigned long REGPARAM2 CPUFUNC(op_d1f9_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = get_ilong (regs, 2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
}}}}}	m68k_incpc (regs, 6);
return 22 * CYCLE_UNIT / 2;
}
/* ADDA.L (d16,PC),An */
unsigned long REGPARAM2 CPUFUNC(op_d1fa_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = m68k_getpc (regs) + 2;
	srca += (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
}}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* ADDA.L (d8,PC,Xn),An */
unsigned long REGPARAM2 CPUFUNC(op_d1fb_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	tmppc = m68k_getpc(regs) + 2;
	srca = get_disp_ea_000(regs, tmppc, get_iword (regs, 2));
{	uae_s32 src = get_long (srca);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
}}}}}	m68k_incpc (regs, 4);
return 20 * CYCLE_UNIT / 2;
}
/* ADDA.L #<data>.L,An */
unsigned long REGPARAM2 CPUFUNC(op_d1fc_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = get_ilong (regs, 2);
{	uae_s32 dst = m68k_areg(regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
}}}}	m68k_incpc (regs, 6);
return 16 * CYCLE_UNIT / 2;
}
/* ASR.B #<data>,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e000_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
{	uae_s8 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u8)data;
	uae_u32 sign = (0x80 & val) >> 7;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
	if (cnt >= 8) {
		val = 0xff & (uae_u32)-sign;
		SET_CFLG (&regs->ccrflags, sign);
		COPY_CARRY (&regs->ccrflags);
	} else {
		val >>= cnt - 1;
		SET_CFLG (&regs->ccrflags, val & 1);
		COPY_CARRY (&regs->ccrflags);
		val >>= 1;
		val |= (0xff << (8 - cnt)) & (uae_u32)-sign;
		val &= 0xff;
	}
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s8)(val)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((val) & 0xff);
}}	m68k_incpc (regs, 2);
return (6 + cnt * 2) * CYCLE_UNIT / 2;
}
/* LSR.B #<data>,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e008_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
{	uae_s8 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
	if (cnt >= 8) {
		SET_CFLG (&regs->ccrflags, (cnt == 8) & (val >> 7));
		COPY_CARRY (&regs->ccrflags);
		val = 0;
	} else {
		val >>= cnt - 1;
		SET_CFLG (&regs->ccrflags, val & 1);
		COPY_CARRY (&regs->ccrflags);
		val >>= 1;
	}
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s8)(val)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((val) & 0xff);
}}	m68k_incpc (regs, 2);
return (6 + cnt * 2) * CYCLE_UNIT / 2;
}
/* ROXR.B #<data>,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e010_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
{	uae_s8 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
{	cnt--;
	{
	uae_u32 carry;
	uae_u32 hival = (val << 1) | GET_XFLG (&regs->ccrflags);
	hival <<= (7 - cnt);
	val >>= cnt;
	carry = val & 1;
	val >>= 1;
	val |= hival;
	SET_XFLG (&regs->ccrflags, carry);
	val &= 0xff;
	} }
	SET_CFLG (&regs->ccrflags, GET_XFLG (&regs->ccrflags));
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s8)(val)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((val) & 0xff);
}}	m68k_incpc (regs, 2);
return (6 + cnt * 2) * CYCLE_UNIT / 2;
}
/* ROR.B #<data>,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e018_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
{	uae_s8 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
{	uae_u32 hival;
	cnt &= 7;
	hival = val << (8 - cnt);
	val >>= cnt;
	val |= hival;
	val &= 0xff;
	SET_CFLG (&regs->ccrflags, (val & 0x80) >> 7);
	}
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s8)(val)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((val) & 0xff);
}}	m68k_incpc (regs, 2);
return (6 + cnt * 2) * CYCLE_UNIT / 2;
}
/* ASR.B Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e020_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 cnt = m68k_dreg (regs, srcreg);
{	uae_s8 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u8)data;
	uae_u32 sign = (0x80 & val) >> 7;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
	if (cnt >= 8) {
		val = 0xff & (uae_u32)-sign;
		SET_CFLG (&regs->ccrflags, sign);
		COPY_CARRY (&regs->ccrflags);
	} else if (cnt > 0) {
		val >>= cnt - 1;
		SET_CFLG (&regs->ccrflags, val & 1);
		COPY_CARRY (&regs->ccrflags);
		val >>= 1;
		val |= (0xff << (8 - cnt)) & (uae_u32)-sign;
		val &= 0xff;
	}
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s8)(val)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((val) & 0xff);
}}	m68k_incpc (regs, 2);
return (6 + cnt * 2) * CYCLE_UNIT / 2;
}
/* LSR.B Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e028_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 cnt = m68k_dreg (regs, srcreg);
{	uae_s8 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
	if (cnt >= 8) {
		SET_CFLG (&regs->ccrflags, (cnt == 8) & (val >> 7));
		COPY_CARRY (&regs->ccrflags);
		val = 0;
	} else if (cnt > 0) {
		val >>= cnt - 1;
		SET_CFLG (&regs->ccrflags, val & 1);
		COPY_CARRY (&regs->ccrflags);
		val >>= 1;
	}
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s8)(val)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((val) & 0xff);
}}	m68k_incpc (regs, 2);
return (6 + cnt * 2) * CYCLE_UNIT / 2;
}
/* ROXR.B Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e030_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 cnt = m68k_dreg (regs, srcreg);
{	uae_s8 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
	if (cnt >= 36) cnt -= 36;
	if (cnt >= 18) cnt -= 18;
	if (cnt >= 9) cnt -= 9;
	if (cnt > 0) {
	cnt--;
	{
	uae_u32 carry;
	uae_u32 hival = (val << 1) | GET_XFLG (&regs->ccrflags);
	hival <<= (7 - cnt);
	val >>= cnt;
	carry = val & 1;
	val >>= 1;
	val |= hival;
	SET_XFLG (&regs->ccrflags, carry);
	val &= 0xff;
	} }
	SET_CFLG (&regs->ccrflags, GET_XFLG (&regs->ccrflags));
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s8)(val)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((val) & 0xff);
}}	m68k_incpc (regs, 2);
return (6 + cnt * 2) * CYCLE_UNIT / 2;
}
/* ROR.B Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e038_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 cnt = m68k_dreg (regs, srcreg);
{	uae_s8 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
	if (cnt > 0) {	uae_u32 hival;
	cnt &= 7;
	hival = val << (8 - cnt);
	val >>= cnt;
	val |= hival;
	val &= 0xff;
	SET_CFLG (&regs->ccrflags, (val & 0x80) >> 7);
	}
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s8)(val)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((val) & 0xff);
}}	m68k_incpc (regs, 2);
return (6 + cnt * 2) * CYCLE_UNIT / 2;
}
/* ASR.W #<data>,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e040_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
{	uae_s16 data = m68k_dreg(regs, dstreg);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = (0x8000 & val) >> 15;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
	if (cnt >= 16) {
		val = 0xffff & (uae_u32)-sign;
		SET_CFLG (&regs->ccrflags, sign);
		COPY_CARRY (&regs->ccrflags);
	} else {
		val >>= cnt - 1;
		SET_CFLG (&regs->ccrflags, val & 1);
		COPY_CARRY (&regs->ccrflags);
		val >>= 1;
		val |= (0xffff << (16 - cnt)) & (uae_u32)-sign;
		val &= 0xffff;
	}
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s16)(val)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}	m68k_incpc (regs, 2);
return (6 + cnt * 2) * CYCLE_UNIT / 2;
}
/* LSR.W #<data>,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e048_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
{	uae_s16 data = m68k_dreg(regs, dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
	if (cnt >= 16) {
		SET_CFLG (&regs->ccrflags, (cnt == 16) & (val >> 15));
		COPY_CARRY (&regs->ccrflags);
		val = 0;
	} else {
		val >>= cnt - 1;
		SET_CFLG (&regs->ccrflags, val & 1);
		COPY_CARRY (&regs->ccrflags);
		val >>= 1;
	}
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s16)(val)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}	m68k_incpc (regs, 2);
return (6 + cnt * 2) * CYCLE_UNIT / 2;
}
/* ROXR.W #<data>,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e050_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
{	uae_s16 data = m68k_dreg(regs, dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
{	cnt--;
	{
	uae_u32 carry;
	uae_u32 hival = (val << 1) | GET_XFLG (&regs->ccrflags);
	hival <<= (15 - cnt);
	val >>= cnt;
	carry = val & 1;
	val >>= 1;
	val |= hival;
	SET_XFLG (&regs->ccrflags, carry);
	val &= 0xffff;
	} }
	SET_CFLG (&regs->ccrflags, GET_XFLG (&regs->ccrflags));
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s16)(val)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}	m68k_incpc (regs, 2);
return (6 + cnt * 2) * CYCLE_UNIT / 2;
}
/* ROR.W #<data>,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e058_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
{	uae_s16 data = m68k_dreg(regs, dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
{	uae_u32 hival;
	cnt &= 15;
	hival = val << (16 - cnt);
	val >>= cnt;
	val |= hival;
	val &= 0xffff;
	SET_CFLG (&regs->ccrflags, (val & 0x8000) >> 15);
	}
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s16)(val)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}	m68k_incpc (regs, 2);
return (6 + cnt * 2) * CYCLE_UNIT / 2;
}
/* ASR.W Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e060_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 cnt = m68k_dreg(regs, srcreg);
{	uae_s16 data = m68k_dreg(regs, dstreg);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = (0x8000 & val) >> 15;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
	if (cnt >= 16) {
		val = 0xffff & (uae_u32)-sign;
		SET_CFLG (&regs->ccrflags, sign);
		COPY_CARRY (&regs->ccrflags);
	} else if (cnt > 0) {
		val >>= cnt - 1;
		SET_CFLG (&regs->ccrflags, val & 1);
		COPY_CARRY (&regs->ccrflags);
		val >>= 1;
		val |= (0xffff << (16 - cnt)) & (uae_u32)-sign;
		val &= 0xffff;
	}
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s16)(val)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}	m68k_incpc (regs, 2);
return (6 + cnt * 2) * CYCLE_UNIT / 2;
}
/* LSR.W Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e068_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 cnt = m68k_dreg(regs, srcreg);
{	uae_s16 data = m68k_dreg(regs, dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
	if (cnt >= 16) {
		SET_CFLG (&regs->ccrflags, (cnt == 16) & (val >> 15));
		COPY_CARRY (&regs->ccrflags);
		val = 0;
	} else if (cnt > 0) {
		val >>= cnt - 1;
		SET_CFLG (&regs->ccrflags, val & 1);
		COPY_CARRY (&regs->ccrflags);
		val >>= 1;
	}
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s16)(val)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}	m68k_incpc (regs, 2);
return (6 + cnt * 2) * CYCLE_UNIT / 2;
}
/* ROXR.W Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e070_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 cnt = m68k_dreg(regs, srcreg);
{	uae_s16 data = m68k_dreg(regs, dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
	if (cnt >= 34) cnt -= 34;
	if (cnt >= 17) cnt -= 17;
	if (cnt > 0) {
	cnt--;
	{
	uae_u32 carry;
	uae_u32 hival = (val << 1) | GET_XFLG (&regs->ccrflags);
	hival <<= (15 - cnt);
	val >>= cnt;
	carry = val & 1;
	val >>= 1;
	val |= hival;
	SET_XFLG (&regs->ccrflags, carry);
	val &= 0xffff;
	} }
	SET_CFLG (&regs->ccrflags, GET_XFLG (&regs->ccrflags));
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s16)(val)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}	m68k_incpc (regs, 2);
return (6 + cnt * 2) * CYCLE_UNIT / 2;
}
/* ROR.W Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e078_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 cnt = m68k_dreg(regs, srcreg);
{	uae_s16 data = m68k_dreg(regs, dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
	if (cnt > 0) {	uae_u32 hival;
	cnt &= 15;
	hival = val << (16 - cnt);
	val >>= cnt;
	val |= hival;
	val &= 0xffff;
	SET_CFLG (&regs->ccrflags, (val & 0x8000) >> 15);
	}
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s16)(val)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}	m68k_incpc (regs, 2);
return (6 + cnt * 2) * CYCLE_UNIT / 2;
}
/* ASR.L #<data>,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e080_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
{	uae_s32 data = m68k_dreg(regs, dstreg);
{	uae_u32 val = data;
	uae_u32 sign = (0x80000000 & val) >> 31;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
	if (cnt >= 32) {
		val = 0xffffffff & (uae_u32)-sign;
		SET_CFLG (&regs->ccrflags, sign);
		COPY_CARRY (&regs->ccrflags);
	} else {
		val >>= cnt - 1;
		SET_CFLG (&regs->ccrflags, val & 1);
		COPY_CARRY (&regs->ccrflags);
		val >>= 1;
		val |= (0xffffffff << (32 - cnt)) & (uae_u32)-sign;
		val &= 0xffffffff;
	}
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s32)(val)) < 0);
	m68k_dreg(regs, dstreg) = (val);
}}	m68k_incpc (regs, 2);
return (8 + cnt * 2) * CYCLE_UNIT / 2;
}
/* LSR.L #<data>,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e088_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
{	uae_s32 data = m68k_dreg(regs, dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
	if (cnt >= 32) {
		SET_CFLG (&regs->ccrflags, (cnt == 32) & (val >> 31));
		COPY_CARRY (&regs->ccrflags);
		val = 0;
	} else {
		val >>= cnt - 1;
		SET_CFLG (&regs->ccrflags, val & 1);
		COPY_CARRY (&regs->ccrflags);
		val >>= 1;
	}
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s32)(val)) < 0);
	m68k_dreg(regs, dstreg) = (val);
}}	m68k_incpc (regs, 2);
return (8 + cnt * 2) * CYCLE_UNIT / 2;
}
/* ROXR.L #<data>,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e090_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
{	uae_s32 data = m68k_dreg(regs, dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
{	cnt--;
	{
	uae_u32 carry;
	uae_u32 hival = (val << 1) | GET_XFLG (&regs->ccrflags);
	hival <<= (31 - cnt);
	val >>= cnt;
	carry = val & 1;
	val >>= 1;
	val |= hival;
	SET_XFLG (&regs->ccrflags, carry);
	val &= 0xffffffff;
	} }
	SET_CFLG (&regs->ccrflags, GET_XFLG (&regs->ccrflags));
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s32)(val)) < 0);
	m68k_dreg(regs, dstreg) = (val);
}}	m68k_incpc (regs, 2);
return (8 + cnt * 2) * CYCLE_UNIT / 2;
}
/* ROR.L #<data>,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e098_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
{	uae_s32 data = m68k_dreg(regs, dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
{	uae_u32 hival;
	cnt &= 31;
	hival = val << (32 - cnt);
	val >>= cnt;
	val |= hival;
	val &= 0xffffffff;
	SET_CFLG (&regs->ccrflags, (val & 0x80000000) >> 31);
	}
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s32)(val)) < 0);
	m68k_dreg(regs, dstreg) = (val);
}}	m68k_incpc (regs, 2);
return (8 + cnt * 2) * CYCLE_UNIT / 2;
}
/* ASR.L Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e0a0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 cnt = m68k_dreg(regs, srcreg);
{	uae_s32 data = m68k_dreg(regs, dstreg);
{	uae_u32 val = data;
	uae_u32 sign = (0x80000000 & val) >> 31;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
	if (cnt >= 32) {
		val = 0xffffffff & (uae_u32)-sign;
		SET_CFLG (&regs->ccrflags, sign);
		COPY_CARRY (&regs->ccrflags);
	} else if (cnt > 0) {
		val >>= cnt - 1;
		SET_CFLG (&regs->ccrflags, val & 1);
		COPY_CARRY (&regs->ccrflags);
		val >>= 1;
		val |= (0xffffffff << (32 - cnt)) & (uae_u32)-sign;
		val &= 0xffffffff;
	}
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s32)(val)) < 0);
	m68k_dreg(regs, dstreg) = (val);
}}	m68k_incpc (regs, 2);
return (8 + cnt * 2) * CYCLE_UNIT / 2;
}
/* LSR.L Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e0a8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 cnt = m68k_dreg(regs, srcreg);
{	uae_s32 data = m68k_dreg(regs, dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
	if (cnt >= 32) {
		SET_CFLG (&regs->ccrflags, (cnt == 32) & (val >> 31));
		COPY_CARRY (&regs->ccrflags);
		val = 0;
	} else if (cnt > 0) {
		val >>= cnt - 1;
		SET_CFLG (&regs->ccrflags, val & 1);
		COPY_CARRY (&regs->ccrflags);
		val >>= 1;
	}
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s32)(val)) < 0);
	m68k_dreg(regs, dstreg) = (val);
}}	m68k_incpc (regs, 2);
return (8 + cnt * 2) * CYCLE_UNIT / 2;
}
/* ROXR.L Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e0b0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 cnt = m68k_dreg(regs, srcreg);
{	uae_s32 data = m68k_dreg(regs, dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
	if (cnt >= 33) cnt -= 33;
	if (cnt > 0) {
	cnt--;
	{
	uae_u32 carry;
	uae_u32 hival = (val << 1) | GET_XFLG (&regs->ccrflags);
	hival <<= (31 - cnt);
	val >>= cnt;
	carry = val & 1;
	val >>= 1;
	val |= hival;
	SET_XFLG (&regs->ccrflags, carry);
	val &= 0xffffffff;
	} }
	SET_CFLG (&regs->ccrflags, GET_XFLG (&regs->ccrflags));
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s32)(val)) < 0);
	m68k_dreg(regs, dstreg) = (val);
}}	m68k_incpc (regs, 2);
return (8 + cnt * 2) * CYCLE_UNIT / 2;
}
/* ROR.L Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e0b8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 cnt = m68k_dreg(regs, srcreg);
{	uae_s32 data = m68k_dreg(regs, dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
	if (cnt > 0) {	uae_u32 hival;
	cnt &= 31;
	hival = val << (32 - cnt);
	val >>= cnt;
	val |= hival;
	val &= 0xffffffff;
	SET_CFLG (&regs->ccrflags, (val & 0x80000000) >> 31);
	}
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s32)(val)) < 0);
	m68k_dreg(regs, dstreg) = (val);
}}	m68k_incpc (regs, 2);
return (8 + cnt * 2) * CYCLE_UNIT / 2;
}
/* ASRW.W (An) */
unsigned long REGPARAM2 CPUFUNC(op_e0d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = m68k_areg(regs, srcreg);
{	uae_s16 data = get_word (dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 cflg = val & 1;
	val = (val >> 1) | sign;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
	SET_CFLG (&regs->ccrflags, cflg);
	COPY_CARRY (&regs->ccrflags);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* ASRW.W (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_e0d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = m68k_areg(regs, srcreg);
{	uae_s16 data = get_word (dataa);
	m68k_areg(regs, srcreg) += 2;
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 cflg = val & 1;
	val = (val >> 1) | sign;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
	SET_CFLG (&regs->ccrflags, cflg);
	COPY_CARRY (&regs->ccrflags);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* ASRW.W -(An) */
unsigned long REGPARAM2 CPUFUNC(op_e0e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg(regs, srcreg) - 2;
{	uae_s16 data = get_word (dataa);
	m68k_areg (regs, srcreg) = dataa;
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 cflg = val & 1;
	val = (val >> 1) | sign;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
	SET_CFLG (&regs->ccrflags, cflg);
	COPY_CARRY (&regs->ccrflags);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* ASRW.W (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_e0e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 data = get_word (dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 cflg = val & 1;
	val = (val >> 1) | sign;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
	SET_CFLG (&regs->ccrflags, cflg);
	COPY_CARRY (&regs->ccrflags);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* ASRW.W (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_e0f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s16 data = get_word (dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 cflg = val & 1;
	val = (val >> 1) | sign;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
	SET_CFLG (&regs->ccrflags, cflg);
	COPY_CARRY (&regs->ccrflags);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* ASRW.W (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_e0f8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr dataa = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 data = get_word (dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 cflg = val & 1;
	val = (val >> 1) | sign;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
	SET_CFLG (&regs->ccrflags, cflg);
	COPY_CARRY (&regs->ccrflags);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* ASRW.W (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_e0f9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr dataa = get_ilong (regs, 2);
{	uae_s16 data = get_word (dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 cflg = val & 1;
	val = (val >> 1) | sign;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
	SET_CFLG (&regs->ccrflags, cflg);
	COPY_CARRY (&regs->ccrflags);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* ASL.B #<data>,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e100_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
{	uae_s8 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
	if (cnt >= 8) {
		SET_VFLG (&regs->ccrflags, val != 0);
		SET_CFLG (&regs->ccrflags, cnt == 8 ? val & 1 : 0);
		COPY_CARRY (&regs->ccrflags);
		val = 0;
	} else {
		uae_u32 mask = (0xff << (7 - cnt)) & 0xff;
		SET_VFLG (&regs->ccrflags, (val & mask) != mask && (val & mask) != 0);
		val <<= cnt - 1;
		SET_CFLG (&regs->ccrflags, (val & 0x80) >> 7);
		COPY_CARRY (&regs->ccrflags);
		val <<= 1;
		val &= 0xff;
	}
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s8)(val)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((val) & 0xff);
}}	m68k_incpc (regs, 2);
return (6 + cnt * 2) * CYCLE_UNIT / 2;
}
/* LSL.B #<data>,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e108_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
{	uae_s8 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
	if (cnt >= 8) {
		SET_CFLG (&regs->ccrflags, cnt == 8 ? val & 1 : 0);
		COPY_CARRY (&regs->ccrflags);
		val = 0;
	} else {
		val <<= (cnt - 1);
		SET_CFLG (&regs->ccrflags, (val & 0x80) >> 7);
		COPY_CARRY (&regs->ccrflags);
		val <<= 1;
	val &= 0xff;
	}
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s8)(val)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((val) & 0xff);
}}	m68k_incpc (regs, 2);
return (6 + cnt * 2) * CYCLE_UNIT / 2;
}
/* ROXL.B #<data>,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e110_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
{	uae_s8 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
{	cnt--;
	{
	uae_u32 carry;
	uae_u32 loval = val >> (7 - cnt);
	carry = loval & 1;
	val = (((val << 1) | GET_XFLG (&regs->ccrflags)) << cnt) | (loval >> 1);
	SET_XFLG (&regs->ccrflags, carry);
	val &= 0xff;
	} }
	SET_CFLG (&regs->ccrflags, GET_XFLG (&regs->ccrflags));
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s8)(val)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((val) & 0xff);
}}	m68k_incpc (regs, 2);
return (6 + cnt * 2) * CYCLE_UNIT / 2;
}
/* ROL.B #<data>,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e118_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
{	uae_s8 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
{	uae_u32 loval;
	cnt &= 7;
	loval = val >> (8 - cnt);
	val <<= cnt;
	val |= loval;
	val &= 0xff;
	SET_CFLG (&regs->ccrflags, val & 1);
}
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s8)(val)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((val) & 0xff);
}}	m68k_incpc (regs, 2);
return (6 + cnt * 2) * CYCLE_UNIT / 2;
}
/* ASL.B Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e120_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 cnt = m68k_dreg (regs, srcreg);
{	uae_s8 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
	if (cnt >= 8) {
		SET_VFLG (&regs->ccrflags, val != 0);
		SET_CFLG (&regs->ccrflags, cnt == 8 ? val & 1 : 0);
		COPY_CARRY (&regs->ccrflags);
		val = 0;
	} else if (cnt > 0) {
		uae_u32 mask = (0xff << (7 - cnt)) & 0xff;
		SET_VFLG (&regs->ccrflags, (val & mask) != mask && (val & mask) != 0);
		val <<= cnt - 1;
		SET_CFLG (&regs->ccrflags, (val & 0x80) >> 7);
		COPY_CARRY (&regs->ccrflags);
		val <<= 1;
		val &= 0xff;
	}
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s8)(val)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((val) & 0xff);
}}	m68k_incpc (regs, 2);
return (6 + cnt * 2) * CYCLE_UNIT / 2;
}
/* LSL.B Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e128_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 cnt = m68k_dreg (regs, srcreg);
{	uae_s8 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
	if (cnt >= 8) {
		SET_CFLG (&regs->ccrflags, cnt == 8 ? val & 1 : 0);
		COPY_CARRY (&regs->ccrflags);
		val = 0;
	} else if (cnt > 0) {
		val <<= (cnt - 1);
		SET_CFLG (&regs->ccrflags, (val & 0x80) >> 7);
		COPY_CARRY (&regs->ccrflags);
		val <<= 1;
	val &= 0xff;
	}
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s8)(val)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((val) & 0xff);
}}	m68k_incpc (regs, 2);
return (6 + cnt * 2) * CYCLE_UNIT / 2;
}
/* ROXL.B Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e130_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 cnt = m68k_dreg (regs, srcreg);
{	uae_s8 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
	if (cnt >= 36) cnt -= 36;
	if (cnt >= 18) cnt -= 18;
	if (cnt >= 9) cnt -= 9;
	if (cnt > 0) {
	cnt--;
	{
	uae_u32 carry;
	uae_u32 loval = val >> (7 - cnt);
	carry = loval & 1;
	val = (((val << 1) | GET_XFLG (&regs->ccrflags)) << cnt) | (loval >> 1);
	SET_XFLG (&regs->ccrflags, carry);
	val &= 0xff;
	} }
	SET_CFLG (&regs->ccrflags, GET_XFLG (&regs->ccrflags));
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s8)(val)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((val) & 0xff);
}}	m68k_incpc (regs, 2);
return (6 + cnt * 2) * CYCLE_UNIT / 2;
}
/* ROL.B Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e138_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 cnt = m68k_dreg (regs, srcreg);
{	uae_s8 data = m68k_dreg (regs, dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
	if (cnt > 0) {
	uae_u32 loval;
	cnt &= 7;
	loval = val >> (8 - cnt);
	val <<= cnt;
	val |= loval;
	val &= 0xff;
	SET_CFLG (&regs->ccrflags, val & 1);
}
	SET_ZFLG (&regs->ccrflags, ((uae_s8)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s8)(val)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((val) & 0xff);
}}	m68k_incpc (regs, 2);
return (6 + cnt * 2) * CYCLE_UNIT / 2;
}
/* ASL.W #<data>,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e140_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
{	uae_s16 data = m68k_dreg(regs, dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
	if (cnt >= 16) {
		SET_VFLG (&regs->ccrflags, val != 0);
		SET_CFLG (&regs->ccrflags, cnt == 16 ? val & 1 : 0);
		COPY_CARRY (&regs->ccrflags);
		val = 0;
	} else {
		uae_u32 mask = (0xffff << (15 - cnt)) & 0xffff;
		SET_VFLG (&regs->ccrflags, (val & mask) != mask && (val & mask) != 0);
		val <<= cnt - 1;
		SET_CFLG (&regs->ccrflags, (val & 0x8000) >> 15);
		COPY_CARRY (&regs->ccrflags);
		val <<= 1;
		val &= 0xffff;
	}
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s16)(val)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}	m68k_incpc (regs, 2);
return (6 + cnt * 2) * CYCLE_UNIT / 2;
}
/* LSL.W #<data>,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e148_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
{	uae_s16 data = m68k_dreg(regs, dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
	if (cnt >= 16) {
		SET_CFLG (&regs->ccrflags, cnt == 16 ? val & 1 : 0);
		COPY_CARRY (&regs->ccrflags);
		val = 0;
	} else {
		val <<= (cnt - 1);
		SET_CFLG (&regs->ccrflags, (val & 0x8000) >> 15);
		COPY_CARRY (&regs->ccrflags);
		val <<= 1;
	val &= 0xffff;
	}
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s16)(val)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}	m68k_incpc (regs, 2);
return (6 + cnt * 2) * CYCLE_UNIT / 2;
}
/* ROXL.W #<data>,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e150_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
{	uae_s16 data = m68k_dreg(regs, dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
{	cnt--;
	{
	uae_u32 carry;
	uae_u32 loval = val >> (15 - cnt);
	carry = loval & 1;
	val = (((val << 1) | GET_XFLG (&regs->ccrflags)) << cnt) | (loval >> 1);
	SET_XFLG (&regs->ccrflags, carry);
	val &= 0xffff;
	} }
	SET_CFLG (&regs->ccrflags, GET_XFLG (&regs->ccrflags));
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s16)(val)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}	m68k_incpc (regs, 2);
return (6 + cnt * 2) * CYCLE_UNIT / 2;
}
/* ROL.W #<data>,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e158_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
{	uae_s16 data = m68k_dreg(regs, dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
{	uae_u32 loval;
	cnt &= 15;
	loval = val >> (16 - cnt);
	val <<= cnt;
	val |= loval;
	val &= 0xffff;
	SET_CFLG (&regs->ccrflags, val & 1);
}
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s16)(val)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}	m68k_incpc (regs, 2);
return (6 + cnt * 2) * CYCLE_UNIT / 2;
}
/* ASL.W Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e160_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 cnt = m68k_dreg(regs, srcreg);
{	uae_s16 data = m68k_dreg(regs, dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
	if (cnt >= 16) {
		SET_VFLG (&regs->ccrflags, val != 0);
		SET_CFLG (&regs->ccrflags, cnt == 16 ? val & 1 : 0);
		COPY_CARRY (&regs->ccrflags);
		val = 0;
	} else if (cnt > 0) {
		uae_u32 mask = (0xffff << (15 - cnt)) & 0xffff;
		SET_VFLG (&regs->ccrflags, (val & mask) != mask && (val & mask) != 0);
		val <<= cnt - 1;
		SET_CFLG (&regs->ccrflags, (val & 0x8000) >> 15);
		COPY_CARRY (&regs->ccrflags);
		val <<= 1;
		val &= 0xffff;
	}
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s16)(val)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}	m68k_incpc (regs, 2);
return (6 + cnt * 2) * CYCLE_UNIT / 2;
}
/* LSL.W Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e168_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 cnt = m68k_dreg(regs, srcreg);
{	uae_s16 data = m68k_dreg(regs, dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
	if (cnt >= 16) {
		SET_CFLG (&regs->ccrflags, cnt == 16 ? val & 1 : 0);
		COPY_CARRY (&regs->ccrflags);
		val = 0;
	} else if (cnt > 0) {
		val <<= (cnt - 1);
		SET_CFLG (&regs->ccrflags, (val & 0x8000) >> 15);
		COPY_CARRY (&regs->ccrflags);
		val <<= 1;
	val &= 0xffff;
	}
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s16)(val)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}	m68k_incpc (regs, 2);
return (6 + cnt * 2) * CYCLE_UNIT / 2;
}
/* ROXL.W Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e170_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 cnt = m68k_dreg(regs, srcreg);
{	uae_s16 data = m68k_dreg(regs, dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
	if (cnt >= 34) cnt -= 34;
	if (cnt >= 17) cnt -= 17;
	if (cnt > 0) {
	cnt--;
	{
	uae_u32 carry;
	uae_u32 loval = val >> (15 - cnt);
	carry = loval & 1;
	val = (((val << 1) | GET_XFLG (&regs->ccrflags)) << cnt) | (loval >> 1);
	SET_XFLG (&regs->ccrflags, carry);
	val &= 0xffff;
	} }
	SET_CFLG (&regs->ccrflags, GET_XFLG (&regs->ccrflags));
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s16)(val)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}	m68k_incpc (regs, 2);
return (6 + cnt * 2) * CYCLE_UNIT / 2;
}
/* ROL.W Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e178_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 cnt = m68k_dreg(regs, srcreg);
{	uae_s16 data = m68k_dreg(regs, dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
	if (cnt > 0) {
	uae_u32 loval;
	cnt &= 15;
	loval = val >> (16 - cnt);
	val <<= cnt;
	val |= loval;
	val &= 0xffff;
	SET_CFLG (&regs->ccrflags, val & 1);
}
	SET_ZFLG (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s16)(val)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}	m68k_incpc (regs, 2);
return (6 + cnt * 2) * CYCLE_UNIT / 2;
}
/* ASL.L #<data>,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e180_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
{	uae_s32 data = m68k_dreg(regs, dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
	if (cnt >= 32) {
		SET_VFLG (&regs->ccrflags, val != 0);
		SET_CFLG (&regs->ccrflags, cnt == 32 ? val & 1 : 0);
		COPY_CARRY (&regs->ccrflags);
		val = 0;
	} else {
		uae_u32 mask = (0xffffffff << (31 - cnt)) & 0xffffffff;
		SET_VFLG (&regs->ccrflags, (val & mask) != mask && (val & mask) != 0);
		val <<= cnt - 1;
		SET_CFLG (&regs->ccrflags, (val & 0x80000000) >> 31);
		COPY_CARRY (&regs->ccrflags);
		val <<= 1;
		val &= 0xffffffff;
	}
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s32)(val)) < 0);
	m68k_dreg(regs, dstreg) = (val);
}}	m68k_incpc (regs, 2);
return (8 + cnt * 2) * CYCLE_UNIT / 2;
}
/* LSL.L #<data>,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e188_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
{	uae_s32 data = m68k_dreg(regs, dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
	if (cnt >= 32) {
		SET_CFLG (&regs->ccrflags, cnt == 32 ? val & 1 : 0);
		COPY_CARRY (&regs->ccrflags);
		val = 0;
	} else {
		val <<= (cnt - 1);
		SET_CFLG (&regs->ccrflags, (val & 0x80000000) >> 31);
		COPY_CARRY (&regs->ccrflags);
		val <<= 1;
	val &= 0xffffffff;
	}
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s32)(val)) < 0);
	m68k_dreg(regs, dstreg) = (val);
}}	m68k_incpc (regs, 2);
return (8 + cnt * 2) * CYCLE_UNIT / 2;
}
/* ROXL.L #<data>,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e190_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
{	uae_s32 data = m68k_dreg(regs, dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
{	cnt--;
	{
	uae_u32 carry;
	uae_u32 loval = val >> (31 - cnt);
	carry = loval & 1;
	val = (((val << 1) | GET_XFLG (&regs->ccrflags)) << cnt) | (loval >> 1);
	SET_XFLG (&regs->ccrflags, carry);
	val &= 0xffffffff;
	} }
	SET_CFLG (&regs->ccrflags, GET_XFLG (&regs->ccrflags));
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s32)(val)) < 0);
	m68k_dreg(regs, dstreg) = (val);
}}	m68k_incpc (regs, 2);
return (8 + cnt * 2) * CYCLE_UNIT / 2;
}
/* ROL.L #<data>,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e198_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
{	uae_s32 data = m68k_dreg(regs, dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
{	uae_u32 loval;
	cnt &= 31;
	loval = val >> (32 - cnt);
	val <<= cnt;
	val |= loval;
	val &= 0xffffffff;
	SET_CFLG (&regs->ccrflags, val & 1);
}
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s32)(val)) < 0);
	m68k_dreg(regs, dstreg) = (val);
}}	m68k_incpc (regs, 2);
return (8 + cnt * 2) * CYCLE_UNIT / 2;
}
/* ASL.L Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e1a0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 cnt = m68k_dreg(regs, srcreg);
{	uae_s32 data = m68k_dreg(regs, dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
	if (cnt >= 32) {
		SET_VFLG (&regs->ccrflags, val != 0);
		SET_CFLG (&regs->ccrflags, cnt == 32 ? val & 1 : 0);
		COPY_CARRY (&regs->ccrflags);
		val = 0;
	} else if (cnt > 0) {
		uae_u32 mask = (0xffffffff << (31 - cnt)) & 0xffffffff;
		SET_VFLG (&regs->ccrflags, (val & mask) != mask && (val & mask) != 0);
		val <<= cnt - 1;
		SET_CFLG (&regs->ccrflags, (val & 0x80000000) >> 31);
		COPY_CARRY (&regs->ccrflags);
		val <<= 1;
		val &= 0xffffffff;
	}
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s32)(val)) < 0);
	m68k_dreg(regs, dstreg) = (val);
}}	m68k_incpc (regs, 2);
return (8 + cnt * 2) * CYCLE_UNIT / 2;
}
/* LSL.L Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e1a8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 cnt = m68k_dreg(regs, srcreg);
{	uae_s32 data = m68k_dreg(regs, dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
	if (cnt >= 32) {
		SET_CFLG (&regs->ccrflags, cnt == 32 ? val & 1 : 0);
		COPY_CARRY (&regs->ccrflags);
		val = 0;
	} else if (cnt > 0) {
		val <<= (cnt - 1);
		SET_CFLG (&regs->ccrflags, (val & 0x80000000) >> 31);
		COPY_CARRY (&regs->ccrflags);
		val <<= 1;
	val &= 0xffffffff;
	}
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s32)(val)) < 0);
	m68k_dreg(regs, dstreg) = (val);
}}	m68k_incpc (regs, 2);
return (8 + cnt * 2) * CYCLE_UNIT / 2;
}
/* ROXL.L Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e1b0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 cnt = m68k_dreg(regs, srcreg);
{	uae_s32 data = m68k_dreg(regs, dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
	if (cnt >= 33) cnt -= 33;
	if (cnt > 0) {
	cnt--;
	{
	uae_u32 carry;
	uae_u32 loval = val >> (31 - cnt);
	carry = loval & 1;
	val = (((val << 1) | GET_XFLG (&regs->ccrflags)) << cnt) | (loval >> 1);
	SET_XFLG (&regs->ccrflags, carry);
	val &= 0xffffffff;
	} }
	SET_CFLG (&regs->ccrflags, GET_XFLG (&regs->ccrflags));
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s32)(val)) < 0);
	m68k_dreg(regs, dstreg) = (val);
}}	m68k_incpc (regs, 2);
return (8 + cnt * 2) * CYCLE_UNIT / 2;
}
/* ROL.L Dn,Dn */
unsigned long REGPARAM2 CPUFUNC(op_e1b8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 cnt = m68k_dreg(regs, srcreg);
{	uae_s32 data = m68k_dreg(regs, dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	CLEAR_CZNV (&regs->ccrflags);
	if (cnt > 0) {
	uae_u32 loval;
	cnt &= 31;
	loval = val >> (32 - cnt);
	val <<= cnt;
	val |= loval;
	val &= 0xffffffff;
	SET_CFLG (&regs->ccrflags, val & 1);
}
	SET_ZFLG (&regs->ccrflags, ((uae_s32)(val)) == 0);
	SET_NFLG (&regs->ccrflags, ((uae_s32)(val)) < 0);
	m68k_dreg(regs, dstreg) = (val);
}}	m68k_incpc (regs, 2);
return (8 + cnt * 2) * CYCLE_UNIT / 2;
}
/* ASLW.W (An) */
unsigned long REGPARAM2 CPUFUNC(op_e1d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = m68k_areg(regs, srcreg);
{	uae_s16 data = get_word (dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 sign2;
	val <<= 1;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
	sign2 = 0x8000 & val;
	SET_CFLG (&regs->ccrflags, sign != 0);
	COPY_CARRY (&regs->ccrflags);
	SET_VFLG (&regs->ccrflags, GET_VFLG (&regs->ccrflags) | (sign2 != sign));
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* ASLW.W (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_e1d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = m68k_areg(regs, srcreg);
{	uae_s16 data = get_word (dataa);
	m68k_areg(regs, srcreg) += 2;
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 sign2;
	val <<= 1;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
	sign2 = 0x8000 & val;
	SET_CFLG (&regs->ccrflags, sign != 0);
	COPY_CARRY (&regs->ccrflags);
	SET_VFLG (&regs->ccrflags, GET_VFLG (&regs->ccrflags) | (sign2 != sign));
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* ASLW.W -(An) */
unsigned long REGPARAM2 CPUFUNC(op_e1e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg(regs, srcreg) - 2;
{	uae_s16 data = get_word (dataa);
	m68k_areg (regs, srcreg) = dataa;
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 sign2;
	val <<= 1;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
	sign2 = 0x8000 & val;
	SET_CFLG (&regs->ccrflags, sign != 0);
	COPY_CARRY (&regs->ccrflags);
	SET_VFLG (&regs->ccrflags, GET_VFLG (&regs->ccrflags) | (sign2 != sign));
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* ASLW.W (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_e1e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 data = get_word (dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 sign2;
	val <<= 1;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
	sign2 = 0x8000 & val;
	SET_CFLG (&regs->ccrflags, sign != 0);
	COPY_CARRY (&regs->ccrflags);
	SET_VFLG (&regs->ccrflags, GET_VFLG (&regs->ccrflags) | (sign2 != sign));
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* ASLW.W (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_e1f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s16 data = get_word (dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 sign2;
	val <<= 1;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
	sign2 = 0x8000 & val;
	SET_CFLG (&regs->ccrflags, sign != 0);
	COPY_CARRY (&regs->ccrflags);
	SET_VFLG (&regs->ccrflags, GET_VFLG (&regs->ccrflags) | (sign2 != sign));
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* ASLW.W (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_e1f8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr dataa = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 data = get_word (dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 sign2;
	val <<= 1;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
	sign2 = 0x8000 & val;
	SET_CFLG (&regs->ccrflags, sign != 0);
	COPY_CARRY (&regs->ccrflags);
	SET_VFLG (&regs->ccrflags, GET_VFLG (&regs->ccrflags) | (sign2 != sign));
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* ASLW.W (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_e1f9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr dataa = get_ilong (regs, 2);
{	uae_s16 data = get_word (dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 sign2;
	val <<= 1;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
	sign2 = 0x8000 & val;
	SET_CFLG (&regs->ccrflags, sign != 0);
	COPY_CARRY (&regs->ccrflags);
	SET_VFLG (&regs->ccrflags, GET_VFLG (&regs->ccrflags) | (sign2 != sign));
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* LSRW.W (An) */
unsigned long REGPARAM2 CPUFUNC(op_e2d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = m68k_areg(regs, srcreg);
{	uae_s16 data = get_word (dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 carry = val & 1;
	val >>= 1;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry);
	COPY_CARRY (&regs->ccrflags);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* LSRW.W (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_e2d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = m68k_areg(regs, srcreg);
{	uae_s16 data = get_word (dataa);
	m68k_areg(regs, srcreg) += 2;
{	uae_u32 val = (uae_u16)data;
	uae_u32 carry = val & 1;
	val >>= 1;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry);
	COPY_CARRY (&regs->ccrflags);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* LSRW.W -(An) */
unsigned long REGPARAM2 CPUFUNC(op_e2e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg(regs, srcreg) - 2;
{	uae_s16 data = get_word (dataa);
	m68k_areg (regs, srcreg) = dataa;
{	uae_u32 val = (uae_u16)data;
	uae_u32 carry = val & 1;
	val >>= 1;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry);
	COPY_CARRY (&regs->ccrflags);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* LSRW.W (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_e2e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 data = get_word (dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 carry = val & 1;
	val >>= 1;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry);
	COPY_CARRY (&regs->ccrflags);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* LSRW.W (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_e2f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s16 data = get_word (dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 carry = val & 1;
	val >>= 1;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry);
	COPY_CARRY (&regs->ccrflags);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* LSRW.W (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_e2f8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr dataa = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 data = get_word (dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 carry = val & 1;
	val >>= 1;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry);
	COPY_CARRY (&regs->ccrflags);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* LSRW.W (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_e2f9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr dataa = get_ilong (regs, 2);
{	uae_s16 data = get_word (dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 carry = val & 1;
	val >>= 1;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry);
	COPY_CARRY (&regs->ccrflags);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* LSLW.W (An) */
unsigned long REGPARAM2 CPUFUNC(op_e3d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = m68k_areg(regs, srcreg);
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry >> 15);
	COPY_CARRY (&regs->ccrflags);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* LSLW.W (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_e3d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = m68k_areg(regs, srcreg);
{	uae_s16 data = get_word (dataa);
	m68k_areg(regs, srcreg) += 2;
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry >> 15);
	COPY_CARRY (&regs->ccrflags);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* LSLW.W -(An) */
unsigned long REGPARAM2 CPUFUNC(op_e3e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg(regs, srcreg) - 2;
{	uae_s16 data = get_word (dataa);
	m68k_areg (regs, srcreg) = dataa;
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry >> 15);
	COPY_CARRY (&regs->ccrflags);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* LSLW.W (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_e3e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry >> 15);
	COPY_CARRY (&regs->ccrflags);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* LSLW.W (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_e3f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry >> 15);
	COPY_CARRY (&regs->ccrflags);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* LSLW.W (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_e3f8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr dataa = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry >> 15);
	COPY_CARRY (&regs->ccrflags);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* LSLW.W (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_e3f9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr dataa = get_ilong (regs, 2);
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry >> 15);
	COPY_CARRY (&regs->ccrflags);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* ROXRW.W (An) */
unsigned long REGPARAM2 CPUFUNC(op_e4d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = m68k_areg(regs, srcreg);
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (GET_XFLG (&regs->ccrflags)) val |= 0x8000;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry);
	COPY_CARRY (&regs->ccrflags);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* ROXRW.W (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_e4d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = m68k_areg(regs, srcreg);
{	uae_s16 data = get_word (dataa);
	m68k_areg(regs, srcreg) += 2;
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (GET_XFLG (&regs->ccrflags)) val |= 0x8000;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry);
	COPY_CARRY (&regs->ccrflags);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* ROXRW.W -(An) */
unsigned long REGPARAM2 CPUFUNC(op_e4e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg(regs, srcreg) - 2;
{	uae_s16 data = get_word (dataa);
	m68k_areg (regs, srcreg) = dataa;
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (GET_XFLG (&regs->ccrflags)) val |= 0x8000;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry);
	COPY_CARRY (&regs->ccrflags);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* ROXRW.W (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_e4e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (GET_XFLG (&regs->ccrflags)) val |= 0x8000;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry);
	COPY_CARRY (&regs->ccrflags);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* ROXRW.W (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_e4f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (GET_XFLG (&regs->ccrflags)) val |= 0x8000;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry);
	COPY_CARRY (&regs->ccrflags);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* ROXRW.W (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_e4f8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr dataa = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (GET_XFLG (&regs->ccrflags)) val |= 0x8000;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry);
	COPY_CARRY (&regs->ccrflags);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* ROXRW.W (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_e4f9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr dataa = get_ilong (regs, 2);
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (GET_XFLG (&regs->ccrflags)) val |= 0x8000;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry);
	COPY_CARRY (&regs->ccrflags);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* ROXLW.W (An) */
unsigned long REGPARAM2 CPUFUNC(op_e5d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = m68k_areg(regs, srcreg);
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (GET_XFLG (&regs->ccrflags)) val |= 1;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry >> 15);
	COPY_CARRY (&regs->ccrflags);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* ROXLW.W (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_e5d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = m68k_areg(regs, srcreg);
{	uae_s16 data = get_word (dataa);
	m68k_areg(regs, srcreg) += 2;
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (GET_XFLG (&regs->ccrflags)) val |= 1;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry >> 15);
	COPY_CARRY (&regs->ccrflags);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* ROXLW.W -(An) */
unsigned long REGPARAM2 CPUFUNC(op_e5e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg(regs, srcreg) - 2;
{	uae_s16 data = get_word (dataa);
	m68k_areg (regs, srcreg) = dataa;
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (GET_XFLG (&regs->ccrflags)) val |= 1;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry >> 15);
	COPY_CARRY (&regs->ccrflags);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* ROXLW.W (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_e5e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (GET_XFLG (&regs->ccrflags)) val |= 1;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry >> 15);
	COPY_CARRY (&regs->ccrflags);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* ROXLW.W (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_e5f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (GET_XFLG (&regs->ccrflags)) val |= 1;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry >> 15);
	COPY_CARRY (&regs->ccrflags);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* ROXLW.W (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_e5f8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr dataa = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (GET_XFLG (&regs->ccrflags)) val |= 1;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry >> 15);
	COPY_CARRY (&regs->ccrflags);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* ROXLW.W (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_e5f9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr dataa = get_ilong (regs, 2);
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (GET_XFLG (&regs->ccrflags)) val |= 1;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry >> 15);
	COPY_CARRY (&regs->ccrflags);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* RORW.W (An) */
unsigned long REGPARAM2 CPUFUNC(op_e6d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = m68k_areg(regs, srcreg);
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (carry) val |= 0x8000;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* RORW.W (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_e6d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = m68k_areg(regs, srcreg);
{	uae_s16 data = get_word (dataa);
	m68k_areg(regs, srcreg) += 2;
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (carry) val |= 0x8000;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* RORW.W -(An) */
unsigned long REGPARAM2 CPUFUNC(op_e6e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg(regs, srcreg) - 2;
{	uae_s16 data = get_word (dataa);
	m68k_areg (regs, srcreg) = dataa;
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (carry) val |= 0x8000;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* RORW.W (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_e6e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (carry) val |= 0x8000;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* RORW.W (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_e6f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (carry) val |= 0x8000;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* RORW.W (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_e6f8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr dataa = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (carry) val |= 0x8000;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* RORW.W (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_e6f9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr dataa = get_ilong (regs, 2);
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (carry) val |= 0x8000;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* ROLW.W (An) */
unsigned long REGPARAM2 CPUFUNC(op_e7d0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = m68k_areg(regs, srcreg);
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (carry)  val |= 1;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry >> 15);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* ROLW.W (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_e7d8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = m68k_areg(regs, srcreg);
{	uae_s16 data = get_word (dataa);
	m68k_areg(regs, srcreg) += 2;
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (carry)  val |= 1;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry >> 15);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* ROLW.W -(An) */
unsigned long REGPARAM2 CPUFUNC(op_e7e0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg(regs, srcreg) - 2;
{	uae_s16 data = get_word (dataa);
	m68k_areg (regs, srcreg) = dataa;
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (carry)  val |= 1;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry >> 15);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* ROLW.W (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_e7e8_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (carry)  val |= 1;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry >> 15);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* ROLW.W (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_e7f0_4)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (carry)  val |= 1;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry >> 15);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* ROLW.W (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_e7f8_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr dataa = (uae_s32)(uae_s16)get_iword (regs, 2);
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (carry)  val |= 1;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry >> 15);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* ROLW.W (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_e7f9_4)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr dataa = get_ilong (regs, 2);
{	uae_s16 data = get_word (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (carry)  val |= 1;
	CLEAR_CZNV (&regs->ccrflags);
	SET_ZFLG   (&regs->ccrflags, ((uae_s16)(val)) == 0);
	SET_NFLG   (&regs->ccrflags, ((uae_s16)(val)) < 0);
SET_CFLG (&regs->ccrflags, carry >> 15);
	put_word (dataa,val);
}}}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}


/* MVSR2.W Dn */
unsigned long REGPARAM2 CPUFUNC(op_40c0_5)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	MakeSR(regs);
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | ((regs->sr) & 0xffff);
}}	m68k_incpc (regs, 2);
return 6 * CYCLE_UNIT / 2;
}
/* MVSR2.W (An) */
unsigned long REGPARAM2 CPUFUNC(op_40d0_5)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
	MakeSR(regs);
	put_word (srca,regs->sr);
}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* MVSR2.W (An)+ */
unsigned long REGPARAM2 CPUFUNC(op_40d8_5)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += 2;
	MakeSR(regs);
	put_word (srca,regs->sr);
}}	m68k_incpc (regs, 2);
return 12 * CYCLE_UNIT / 2;
}
/* MVSR2.W -(An) */
unsigned long REGPARAM2 CPUFUNC(op_40e0_5)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	m68k_areg (regs, srcreg) = srca;
	MakeSR(regs);
	put_word (srca,regs->sr);
}}	m68k_incpc (regs, 2);
return 14 * CYCLE_UNIT / 2;
}
/* MVSR2.W (d16,An) */
unsigned long REGPARAM2 CPUFUNC(op_40e8_5)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_iword (regs, 2);
	MakeSR(regs);
	put_word (srca,regs->sr);
}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* MVSR2.W (d8,An,Xn) */
unsigned long REGPARAM2 CPUFUNC(op_40f0_5)(uae_u32 opcode, struct regstruct *regs)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = get_disp_ea_000(regs, m68k_areg(regs, srcreg), get_iword (regs, 2));
	MakeSR(regs);
	put_word (srca,regs->sr);
}}	m68k_incpc (regs, 4);
return 18 * CYCLE_UNIT / 2;
}
/* MVSR2.W (xxx).W */
unsigned long REGPARAM2 CPUFUNC(op_40f8_5)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = (uae_s32)(uae_s16)get_iword (regs, 2);
	MakeSR(regs);
	put_word (srca,regs->sr);
}}	m68k_incpc (regs, 4);
return 16 * CYCLE_UNIT / 2;
}
/* MVSR2.W (xxx).L */
unsigned long REGPARAM2 CPUFUNC(op_40f9_5)(uae_u32 opcode, struct regstruct *regs)
{
{{	uaecptr srca = get_ilong (regs, 2);
	MakeSR(regs);
	put_word (srca,regs->sr);
}}	m68k_incpc (regs, 6);
return 20 * CYCLE_UNIT / 2;
}
/* RTE.L  */
unsigned long REGPARAM2 CPUFUNC(op_4e73_5)(uae_u32 opcode, struct regstruct *regs)
{
{if (!regs->s) { Exception(8, regs, 0); return 34 * CYCLE_UNIT / 2; }
{{	uaecptr sra = m68k_areg(regs, 7);
{	uae_s16 sr = get_word (sra);
	m68k_areg(regs, 7) += 2;
{	uaecptr pca = m68k_areg(regs, 7);
{	uae_s32 pc = get_long (pca);
	m68k_areg(regs, 7) += 4;
	regs->sr = sr; m68k_setpc(regs, pc);
	MakeFromSR(regs);
}}}}}}
return 20 * CYCLE_UNIT / 2;
}
