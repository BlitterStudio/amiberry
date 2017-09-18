#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "memory.h"
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
uae_u32 REGPARAM2 CPUFUNC(op_0000_40)(uae_u32 opcode)
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
return 6 * CYCLE_UNIT / 2;
}

/* OR.B #<data>.B,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0010_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}	m68k_incpc (4);
return 10 * CYCLE_UNIT / 2;
}

/* OR.B #<data>.B,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0018_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}	m68k_incpc (4);
return 13 * CYCLE_UNIT / 2;
}

/* OR.B #<data>.B,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0020_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* OR.B #<data>.B,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0028_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte_jit (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}	m68k_incpc (6);
return 21 * CYCLE_UNIT / 2;
}

/* OR.B #<data>.B,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0030_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s8 dst = get_byte_jit (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}}return 17 * CYCLE_UNIT / 2;
}

/* OR.B #<data>.B,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0038_40)(uae_u32 opcode)
{
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte_jit (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* OR.B #<data>.B,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0039_40)(uae_u32 opcode)
{
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_s8 dst = get_byte_jit (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}	m68k_incpc (8);
return 17 * CYCLE_UNIT / 2;
}

/* ORSR.B #<data>.W */
uae_u32 REGPARAM2 CPUFUNC(op_003c_40)(uae_u32 opcode)
{
{	MakeSR ();
{	uae_s16 src = get_diword (2);
	src &= 0xFF;
	regs.sr |= src;
	MakeFromSR_T0();
	m68k_incpc (4);
}}return 15 * CYCLE_UNIT / 2;
}

/* OR.W #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0040_40)(uae_u32 opcode)
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
return 6 * CYCLE_UNIT / 2;
}

/* OR.W #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0050_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word_jit (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}	m68k_incpc (4);
return 10 * CYCLE_UNIT / 2;
}

/* OR.W #<data>.W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0058_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word_jit (dsta);
	m68k_areg (regs, dstreg) += 2;
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}	m68k_incpc (4);
return 13 * CYCLE_UNIT / 2;
}

/* OR.W #<data>.W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0060_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
{	uae_s16 dst = get_word_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* OR.W #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0068_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_s16 dst = get_word_jit (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}	m68k_incpc (6);
return 21 * CYCLE_UNIT / 2;
}

/* OR.W #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0070_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s16 dst = get_word_jit (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}}return 17 * CYCLE_UNIT / 2;
}

/* OR.W #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0078_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_s16 dst = get_word_jit (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* OR.W #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0079_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_s16 dst = get_word_jit (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}	m68k_incpc (8);
return 16 * CYCLE_UNIT / 2;
}

/* ORSR.W #<data>.W */
uae_u32 REGPARAM2 CPUFUNC(op_007c_40)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	MakeSR ();
{	uae_s16 src = get_diword (2);
	regs.sr |= src;
	MakeFromSR_T0();
	m68k_incpc (4);
}}}return 15 * CYCLE_UNIT / 2;
}

/* OR.L #<data>.L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0080_40)(uae_u32 opcode)
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
return 8 * CYCLE_UNIT / 2;
}

/* OR.L #<data>.L,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0090_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long_jit (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* OR.L #<data>.L,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0098_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long_jit (dsta);
	m68k_areg (regs, dstreg) += 4;
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}	m68k_incpc (6);
return 15 * CYCLE_UNIT / 2;
}

/* OR.L #<data>.L,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_00a0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
{	uae_s32 dst = get_long_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}	m68k_incpc (6);
return 14 * CYCLE_UNIT / 2;
}

/* OR.L #<data>.L,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_00a8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (6);
{	uae_s32 dst = get_long_jit (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}	m68k_incpc (8);
return 23 * CYCLE_UNIT / 2;
}

/* OR.L #<data>.L,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_00b0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	m68k_incpc (6);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s32 dst = get_long_jit (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}}return 21 * CYCLE_UNIT / 2;
}

/* OR.L #<data>.L,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_00b8_40)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (6);
{	uae_s32 dst = get_long_jit (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}	m68k_incpc (8);
return 16 * CYCLE_UNIT / 2;
}

/* OR.L #<data>.L,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_00b9_40)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = get_dilong (6);
{	uae_s32 dst = get_long_jit (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}	m68k_incpc (10);
return 18 * CYCLE_UNIT / 2;
}

/* CHK2.B #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_00d0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	{uae_s32 upper,lower,reg = regs.regs[(extra >> 12) & 15];
	lower = (uae_s32)(uae_s8)get_byte_jit (dsta); upper = (uae_s32)(uae_s8)get_byte_jit (dsta + 1);
	if ((extra & 0x8000) == 0) reg = (uae_s32)(uae_s8)reg;
	SET_ZFLG (upper == reg || lower == reg);
	SET_CFLG_ALWAYS (lower <= upper ? reg < lower || reg > upper : reg > upper || reg < lower);
	if ((extra & 0x800) && GET_CFLG ()) { Exception_cpu(6); 
		return 8 * CYCLE_UNIT / 2;
	}
}
}}}	m68k_incpc (4);
return 22 * CYCLE_UNIT / 2;
}

/* CHK2.B #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_00e8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	{uae_s32 upper,lower,reg = regs.regs[(extra >> 12) & 15];
	lower = (uae_s32)(uae_s8)get_byte_jit (dsta); upper = (uae_s32)(uae_s8)get_byte_jit (dsta + 1);
	if ((extra & 0x8000) == 0) reg = (uae_s32)(uae_s8)reg;
	SET_ZFLG (upper == reg || lower == reg);
	SET_CFLG_ALWAYS (lower <= upper ? reg < lower || reg > upper : reg > upper || reg < lower);
	if ((extra & 0x800) && GET_CFLG ()) { Exception_cpu(6); 
		return 12 * CYCLE_UNIT / 2;
	}
}
}}}	m68k_incpc (6);
return 33 * CYCLE_UNIT / 2;
}

/* CHK2.B #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_00f0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
	{uae_s32 upper,lower,reg = regs.regs[(extra >> 12) & 15];
	lower = (uae_s32)(uae_s8)get_byte_jit (dsta); upper = (uae_s32)(uae_s8)get_byte_jit (dsta + 1);
	if ((extra & 0x8000) == 0) reg = (uae_s32)(uae_s8)reg;
	SET_ZFLG (upper == reg || lower == reg);
	SET_CFLG_ALWAYS (lower <= upper ? reg < lower || reg > upper : reg > upper || reg < lower);
	if ((extra & 0x800) && GET_CFLG ()) { Exception_cpu(6); 
		return 8 * CYCLE_UNIT / 2;
  }
}
}}}}return 29 * CYCLE_UNIT / 2;
}

/* CHK2.B #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_00f8_40)(uae_u32 opcode)
{
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	{uae_s32 upper,lower,reg = regs.regs[(extra >> 12) & 15];
	lower = (uae_s32)(uae_s8)get_byte_jit (dsta); upper = (uae_s32)(uae_s8)get_byte_jit (dsta + 1);
	if ((extra & 0x8000) == 0) reg = (uae_s32)(uae_s8)reg;
	SET_ZFLG (upper == reg || lower == reg);
	SET_CFLG_ALWAYS (lower <= upper ? reg < lower || reg > upper : reg > upper || reg < lower);
	if ((extra & 0x800) && GET_CFLG ()) { Exception_cpu(6); 
		return 12 * CYCLE_UNIT / 2;
	}
}
}}}	m68k_incpc (6);
return 25 * CYCLE_UNIT / 2;
}

/* CHK2.B #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_00f9_40)(uae_u32 opcode)
{
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
	{uae_s32 upper,lower,reg = regs.regs[(extra >> 12) & 15];
	lower = (uae_s32)(uae_s8)get_byte_jit (dsta); upper = (uae_s32)(uae_s8)get_byte_jit (dsta + 1);
	if ((extra & 0x8000) == 0) reg = (uae_s32)(uae_s8)reg;
	SET_ZFLG (upper == reg || lower == reg);
	SET_CFLG_ALWAYS (lower <= upper ? reg < lower || reg > upper : reg > upper || reg < lower);
	if ((extra & 0x800) && GET_CFLG ()) { Exception_cpu(6); 
		return 16 * CYCLE_UNIT / 2;
	}
}
}}}	m68k_incpc (8);
return 28 * CYCLE_UNIT / 2;
}

/* CHK2.B #<data>.W,(d16,PC) */
uae_u32 REGPARAM2 CPUFUNC(op_00fa_40)(uae_u32 opcode)
{
	uae_u32 dstreg = 2;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_getpc () + 4;
	dsta += (uae_s32)(uae_s16)get_diword (4);
	{uae_s32 upper,lower,reg = regs.regs[(extra >> 12) & 15];
	lower = (uae_s32)(uae_s8)get_byte_jit (dsta); upper = (uae_s32)(uae_s8)get_byte_jit (dsta + 1);
	if ((extra & 0x8000) == 0) reg = (uae_s32)(uae_s8)reg;
	SET_ZFLG (upper == reg || lower == reg);
	SET_CFLG_ALWAYS (lower <= upper ? reg < lower || reg > upper : reg > upper || reg < lower);
	if ((extra & 0x800) && GET_CFLG ()) { Exception_cpu(6); 
		return 12 * CYCLE_UNIT / 2;
	}
}
}}}	m68k_incpc (6);
return 33 * CYCLE_UNIT / 2;
}

/* CHK2.B #<data>.W,(d8,PC,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_00fb_40)(uae_u32 opcode)
{
	uae_u32 dstreg = 3;
{{	uae_s16 extra = get_diword (2);
{	uaecptr tmppc;
	uaecptr dsta;
	m68k_incpc (4);
{	tmppc = m68k_getpc ();
	dsta = get_disp_ea_020 (tmppc, 0);
	{uae_s32 upper,lower,reg = regs.regs[(extra >> 12) & 15];
	lower = (uae_s32)(uae_s8)get_byte_jit (dsta); upper = (uae_s32)(uae_s8)get_byte_jit (dsta + 1);
	if ((extra & 0x8000) == 0) reg = (uae_s32)(uae_s8)reg;
	SET_ZFLG (upper == reg || lower == reg);
	SET_CFLG_ALWAYS (lower <= upper ? reg < lower || reg > upper : reg > upper || reg < lower);
	if ((extra & 0x800) && GET_CFLG ()) { Exception_cpu(6); 
		return 8 * CYCLE_UNIT / 2;
	}
}
}}}}return 29 * CYCLE_UNIT / 2;
}

/* BTST.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0100_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src &= 31;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}	m68k_incpc (2);
return 5 * CYCLE_UNIT / 2;
}

/* MVPMR.W (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0108_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr memp = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_u16 val  = (get_byte_jit (memp) & 0xff) << 8;
	        val |= (get_byte_jit (memp + 2) & 0xff);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* BTST.B Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0110_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (2);
return 9 * CYCLE_UNIT / 2;
}

/* BTST.B Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0118_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (2);
return 9 * CYCLE_UNIT / 2;
}

/* BTST.B Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0120_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* BTST.B Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0128_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (4);
return 15 * CYCLE_UNIT / 2;
}

/* BTST.B Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0130_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	m68k_incpc (2);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}}return 13 * CYCLE_UNIT / 2;
}

/* BTST.B Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0138_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (4);
return 11 * CYCLE_UNIT / 2;
}

/* BTST.B Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0139_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (6);
return 12 * CYCLE_UNIT / 2;
}

/* BTST.B Dn,(d16,PC) */
uae_u32 REGPARAM2 CPUFUNC(op_013a_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = 2;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_getpc () + 2;
	dsta += (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (4);
return 11 * CYCLE_UNIT / 2;
}

/* BTST.B Dn,(d8,PC,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_013b_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = 3;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr tmppc;
	uaecptr dsta;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	dsta = get_disp_ea_020 (tmppc, 0);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}}return 13 * CYCLE_UNIT / 2;
}

/* BTST.B Dn,#<data>.B */
uae_u32 REGPARAM2 CPUFUNC(op_013c_40)(uae_u32 opcode)
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
uae_u32 REGPARAM2 CPUFUNC(op_0140_40)(uae_u32 opcode)
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
return 5 * CYCLE_UNIT / 2;
}

/* MVPMR.L (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0148_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr memp = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_u32 val  = (get_byte_jit (memp) & 0xff) << 24;
	        val |= (get_byte_jit (memp + 2) & 0xff) << 16;
	        val |= (get_byte_jit (memp + 4) & 0xff) << 8;
	        val |= (get_byte_jit (memp + 6) & 0xff);
	m68k_dreg (regs, dstreg) = (val);
}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* BCHG.B Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0150_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	put_byte_jit (dsta,dst);
}}}}	m68k_incpc (2);
return 9 * CYCLE_UNIT / 2;
}

/* BCHG.B Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0158_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	put_byte_jit (dsta,dst);
}}}}	m68k_incpc (2);
return 9 * CYCLE_UNIT / 2;
}

/* BCHG.B Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0160_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	put_byte_jit (dsta,dst);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* BCHG.B Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0168_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	put_byte_jit (dsta,dst);
}}}}	m68k_incpc (4);
return 15 * CYCLE_UNIT / 2;
}

/* BCHG.B Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0170_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	m68k_incpc (2);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	put_byte_jit (dsta,dst);
}}}}}return 13 * CYCLE_UNIT / 2;
}

/* BCHG.B Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0178_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	put_byte_jit (dsta,dst);
}}}}	m68k_incpc (4);
return 11 * CYCLE_UNIT / 2;
}

/* BCHG.B Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0179_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	put_byte_jit (dsta,dst);
}}}}	m68k_incpc (6);
return 12 * CYCLE_UNIT / 2;
}

/* BCLR.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0180_40)(uae_u32 opcode)
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
return 5 * CYCLE_UNIT / 2;
}

/* MVPRM.W Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0188_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
	uaecptr memp = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	put_byte_jit (memp, src >> 8);
	put_byte_jit (memp + 2, src);
}}	m68k_incpc (4);
return 11 * CYCLE_UNIT / 2;
}

/* BCLR.B Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0190_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit (dsta,dst);
}}}}	m68k_incpc (2);
return 9 * CYCLE_UNIT / 2;
}

/* BCLR.B Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0198_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit (dsta,dst);
}}}}	m68k_incpc (2);
return 9 * CYCLE_UNIT / 2;
}

/* BCLR.B Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_01a0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit (dsta,dst);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* BCLR.B Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_01a8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit (dsta,dst);
}}}}	m68k_incpc (4);
return 15 * CYCLE_UNIT / 2;
}

/* BCLR.B Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_01b0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	m68k_incpc (2);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit (dsta,dst);
}}}}}return 13 * CYCLE_UNIT / 2;
}

/* BCLR.B Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_01b8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit (dsta,dst);
}}}}	m68k_incpc (4);
return 11 * CYCLE_UNIT / 2;
}

/* BCLR.B Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_01b9_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit (dsta,dst);
}}}}	m68k_incpc (6);
return 12 * CYCLE_UNIT / 2;
}

/* BSET.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_01c0_40)(uae_u32 opcode)
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
return 5 * CYCLE_UNIT / 2;
}

/* MVPRM.L Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_01c8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
	uaecptr memp = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	put_byte_jit (memp, src >> 24);
	put_byte_jit (memp + 2, src >> 16);
	put_byte_jit (memp + 4, src >> 8);
	put_byte_jit (memp + 6, src);
}}	m68k_incpc (4);
return 17 * CYCLE_UNIT / 2;
}

/* BSET.B Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_01d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte_jit (dsta,dst);
}}}}	m68k_incpc (2);
return 9 * CYCLE_UNIT / 2;
}

/* BSET.B Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_01d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte_jit (dsta,dst);
}}}}	m68k_incpc (2);
return 9 * CYCLE_UNIT / 2;
}

/* BSET.B Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_01e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte_jit (dsta,dst);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* BSET.B Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_01e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte_jit (dsta,dst);
}}}}	m68k_incpc (4);
return 15 * CYCLE_UNIT / 2;
}

/* BSET.B Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_01f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	m68k_incpc (2);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte_jit (dsta,dst);
}}}}}return 13 * CYCLE_UNIT / 2;
}

/* BSET.B Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_01f8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte_jit (dsta,dst);
}}}}	m68k_incpc (4);
return 11 * CYCLE_UNIT / 2;
}

/* BSET.B Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_01f9_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte_jit (dsta,dst);
}}}}	m68k_incpc (6);
return 12 * CYCLE_UNIT / 2;
}

/* AND.B #<data>.B,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0200_40)(uae_u32 opcode)
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
return 6 * CYCLE_UNIT / 2;
}

/* AND.B #<data>.B,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0210_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}	m68k_incpc (4);
return 10 * CYCLE_UNIT / 2;
}

/* AND.B #<data>.B,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0218_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}	m68k_incpc (4);
return 13 * CYCLE_UNIT / 2;
}

/* AND.B #<data>.B,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0220_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* AND.B #<data>.B,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0228_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}	m68k_incpc (6);
return 21 * CYCLE_UNIT / 2;
}

/* AND.B #<data>.B,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0230_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}}return 17 * CYCLE_UNIT / 2;
}

/* AND.B #<data>.B,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0238_40)(uae_u32 opcode)
{
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* AND.B #<data>.B,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0239_40)(uae_u32 opcode)
{
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}	m68k_incpc (8);
return 16 * CYCLE_UNIT / 2;
}

/* ANDSR.B #<data>.W */
uae_u32 REGPARAM2 CPUFUNC(op_023c_40)(uae_u32 opcode)
{
{	MakeSR ();
{	uae_s16 src = get_diword (2);
	src |= 0xFF00;
	regs.sr &= src;
	MakeFromSR_T0();
	m68k_incpc (4);
}}return 15 * CYCLE_UNIT / 2;
}

/* AND.W #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0240_40)(uae_u32 opcode)
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
return 6 * CYCLE_UNIT / 2;
}

/* AND.W #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0250_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word_jit (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}	m68k_incpc (4);
return 10 * CYCLE_UNIT / 2;
}

/* AND.W #<data>.W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0258_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word_jit (dsta);
	m68k_areg (regs, dstreg) += 2;
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}	m68k_incpc (4);
return 13 * CYCLE_UNIT / 2;
}

/* AND.W #<data>.W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0260_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
{	uae_s16 dst = get_word_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* AND.W #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0268_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_s16 dst = get_word_jit (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}	m68k_incpc (6);
return 21 * CYCLE_UNIT / 2;
}

/* AND.W #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0270_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s16 dst = get_word_jit (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}}return 17 * CYCLE_UNIT / 2;
}

/* AND.W #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0278_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_s16 dst = get_word_jit (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* AND.W #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0279_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_s16 dst = get_word_jit (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}	m68k_incpc (8);
return 16 * CYCLE_UNIT / 2;
}

/* ANDSR.W #<data>.W */
uae_u32 REGPARAM2 CPUFUNC(op_027c_40)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	MakeSR ();
{	uae_s16 src = get_diword (2);
	regs.sr &= src;
	MakeFromSR_T0();
	m68k_incpc (4);
}}}return 15 * CYCLE_UNIT / 2;
}

/* AND.L #<data>.L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0280_40)(uae_u32 opcode)
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
return 8 * CYCLE_UNIT / 2;
}

/* AND.L #<data>.L,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0290_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long_jit (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}	m68k_incpc (6);
return 10 * CYCLE_UNIT / 2;
}

/* AND.L #<data>.L,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0298_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long_jit (dsta);
	m68k_areg (regs, dstreg) += 4;
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}	m68k_incpc (6);
return 15 * CYCLE_UNIT / 2;
}

/* AND.L #<data>.L,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_02a0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
{	uae_s32 dst = get_long_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}	m68k_incpc (6);
return 14 * CYCLE_UNIT / 2;
}

/* AND.L #<data>.L,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_02a8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (6);
{	uae_s32 dst = get_long_jit (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}	m68k_incpc (8);
return 23 * CYCLE_UNIT / 2;
}

/* AND.L #<data>.L,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_02b0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	m68k_incpc (6);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s32 dst = get_long_jit (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}}return 19 * CYCLE_UNIT / 2;
}

/* AND.L #<data>.L,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_02b8_40)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (6);
{	uae_s32 dst = get_long_jit (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}	m68k_incpc (8);
return 16 * CYCLE_UNIT / 2;
}

/* AND.L #<data>.L,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_02b9_40)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = get_dilong (6);
{	uae_s32 dst = get_long_jit (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}	m68k_incpc (10);
return 18 * CYCLE_UNIT / 2;
}

/* CHK2.W #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_02d0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	{uae_s32 upper,lower,reg = regs.regs[(extra >> 12) & 15];
	lower = (uae_s32)(uae_s16)get_word_jit (dsta); upper = (uae_s32)(uae_s16)get_word_jit (dsta + 2);
	if ((extra & 0x8000) == 0) reg = (uae_s32)(uae_s16)reg;
	SET_ZFLG (upper == reg || lower == reg);
	SET_CFLG_ALWAYS (lower <= upper ? reg < lower || reg > upper : reg > upper || reg < lower);
	if ((extra & 0x800) && GET_CFLG ()) { Exception_cpu(6); 
		return 8 * CYCLE_UNIT / 2;
	}
}
}}}	m68k_incpc (4);
return 22 * CYCLE_UNIT / 2;
}

/* CHK2.W #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_02e8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	{uae_s32 upper,lower,reg = regs.regs[(extra >> 12) & 15];
	lower = (uae_s32)(uae_s16)get_word_jit (dsta); upper = (uae_s32)(uae_s16)get_word_jit (dsta + 2);
	if ((extra & 0x8000) == 0) reg = (uae_s32)(uae_s16)reg;
	SET_ZFLG (upper == reg || lower == reg);
	SET_CFLG_ALWAYS (lower <= upper ? reg < lower || reg > upper : reg > upper || reg < lower);
	if ((extra & 0x800) && GET_CFLG ()) { Exception_cpu(6); 
		return 12 * CYCLE_UNIT / 2;
	}
}
}}}	m68k_incpc (6);
return 33 * CYCLE_UNIT / 2;
}

/* CHK2.W #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_02f0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
	{uae_s32 upper,lower,reg = regs.regs[(extra >> 12) & 15];
	lower = (uae_s32)(uae_s16)get_word_jit (dsta); upper = (uae_s32)(uae_s16)get_word_jit (dsta + 2);
	if ((extra & 0x8000) == 0) reg = (uae_s32)(uae_s16)reg;
	SET_ZFLG (upper == reg || lower == reg);
	SET_CFLG_ALWAYS (lower <= upper ? reg < lower || reg > upper : reg > upper || reg < lower);
	if ((extra & 0x800) && GET_CFLG ()) { Exception_cpu(6); 
		return 8 * CYCLE_UNIT / 2;
	}
}
}}}}return 29 * CYCLE_UNIT / 2;
}

/* CHK2.W #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_02f8_40)(uae_u32 opcode)
{
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	{uae_s32 upper,lower,reg = regs.regs[(extra >> 12) & 15];
	lower = (uae_s32)(uae_s16)get_word_jit (dsta); upper = (uae_s32)(uae_s16)get_word_jit (dsta + 2);
	if ((extra & 0x8000) == 0) reg = (uae_s32)(uae_s16)reg;
	SET_ZFLG (upper == reg || lower == reg);
	SET_CFLG_ALWAYS (lower <= upper ? reg < lower || reg > upper : reg > upper || reg < lower);
	if ((extra & 0x800) && GET_CFLG ()) { Exception_cpu(6); 
		return 12 * CYCLE_UNIT / 2;
	}
}
}}}	m68k_incpc (6);
return 25 * CYCLE_UNIT / 2;
}

/* CHK2.W #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_02f9_40)(uae_u32 opcode)
{
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
	{uae_s32 upper,lower,reg = regs.regs[(extra >> 12) & 15];
	lower = (uae_s32)(uae_s16)get_word_jit (dsta); upper = (uae_s32)(uae_s16)get_word_jit (dsta + 2);
	if ((extra & 0x8000) == 0) reg = (uae_s32)(uae_s16)reg;
	SET_ZFLG (upper == reg || lower == reg);
	SET_CFLG_ALWAYS (lower <= upper ? reg < lower || reg > upper : reg > upper || reg < lower);
	if ((extra & 0x800) && GET_CFLG ()) { Exception_cpu(6); 
		return 16 * CYCLE_UNIT / 2;
	}
}
}}}	m68k_incpc (8);
return 28 * CYCLE_UNIT / 2;
}

/* CHK2.W #<data>.W,(d16,PC) */
uae_u32 REGPARAM2 CPUFUNC(op_02fa_40)(uae_u32 opcode)
{
	uae_u32 dstreg = 2;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_getpc () + 4;
	dsta += (uae_s32)(uae_s16)get_diword (4);
	{uae_s32 upper,lower,reg = regs.regs[(extra >> 12) & 15];
	lower = (uae_s32)(uae_s16)get_word_jit (dsta); upper = (uae_s32)(uae_s16)get_word_jit (dsta + 2);
	if ((extra & 0x8000) == 0) reg = (uae_s32)(uae_s16)reg;
	SET_ZFLG (upper == reg || lower == reg);
	SET_CFLG_ALWAYS (lower <= upper ? reg < lower || reg > upper : reg > upper || reg < lower);
	if ((extra & 0x800) && GET_CFLG ()) { Exception_cpu(6); 
		return 12 * CYCLE_UNIT / 2;
	}
}
}}}	m68k_incpc (6);
return 33 * CYCLE_UNIT / 2;
}

/* CHK2.W #<data>.W,(d8,PC,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_02fb_40)(uae_u32 opcode)
{
	uae_u32 dstreg = 3;
{{	uae_s16 extra = get_diword (2);
{	uaecptr tmppc;
	uaecptr dsta;
	m68k_incpc (4);
{	tmppc = m68k_getpc ();
	dsta = get_disp_ea_020 (tmppc, 0);
	{uae_s32 upper,lower,reg = regs.regs[(extra >> 12) & 15];
	lower = (uae_s32)(uae_s16)get_word_jit (dsta); upper = (uae_s32)(uae_s16)get_word_jit (dsta + 2);
	if ((extra & 0x8000) == 0) reg = (uae_s32)(uae_s16)reg;
	SET_ZFLG (upper == reg || lower == reg);
	SET_CFLG_ALWAYS (lower <= upper ? reg < lower || reg > upper : reg > upper || reg < lower);
	if ((extra & 0x800) && GET_CFLG ()) { Exception_cpu(6); 
		return 8 * CYCLE_UNIT / 2;
  }
}
}}}}return 29 * CYCLE_UNIT / 2;
}

/* SUB.B #<data>.B,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0400_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
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
return 6 * CYCLE_UNIT / 2;
}

/* SUB.B #<data>.B,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0410_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}	m68k_incpc (4);
return 10 * CYCLE_UNIT / 2;
}

/* SUB.B #<data>.B,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0418_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}	m68k_incpc (4);
return 13 * CYCLE_UNIT / 2;
}

/* SUB.B #<data>.B,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0420_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* SUB.B #<data>.B,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0428_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte_jit (dsta);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}	m68k_incpc (6);
return 21 * CYCLE_UNIT / 2;
}

/* SUB.B #<data>.B,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0430_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s8 dst = get_byte_jit (dsta);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}}return 17 * CYCLE_UNIT / 2;
}

/* SUB.B #<data>.B,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0438_40)(uae_u32 opcode)
{
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte_jit (dsta);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* SUB.B #<data>.B,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0439_40)(uae_u32 opcode)
{
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_s8 dst = get_byte_jit (dsta);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}	m68k_incpc (8);
return 16 * CYCLE_UNIT / 2;
}

/* SUB.W #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0440_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
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
return 6 * CYCLE_UNIT / 2;
}

/* SUB.W #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0450_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word_jit (dsta);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}	m68k_incpc (4);
return 10 * CYCLE_UNIT / 2;
}

/* SUB.W #<data>.W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0458_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word_jit (dsta);
	m68k_areg (regs, dstreg) += 2;
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}	m68k_incpc (4);
return 13 * CYCLE_UNIT / 2;
}

/* SUB.W #<data>.W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0460_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
{	uae_s16 dst = get_word_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* SUB.W #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0468_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_s16 dst = get_word_jit (dsta);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}	m68k_incpc (6);
return 21 * CYCLE_UNIT / 2;
}

/* SUB.W #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0470_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s16 dst = get_word_jit (dsta);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}}return 17 * CYCLE_UNIT / 2;
}

/* SUB.W #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0478_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_s16 dst = get_word_jit (dsta);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* SUB.W #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0479_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_s16 dst = get_word_jit (dsta);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}	m68k_incpc (8);
return 16 * CYCLE_UNIT / 2;
}

/* SUB.L #<data>.L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0480_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
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
return 8 * CYCLE_UNIT / 2;
}

/* SUB.L #<data>.L,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0490_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long_jit (dsta);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* SUB.L #<data>.L,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0498_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long_jit (dsta);
	m68k_areg (regs, dstreg) += 4;
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}	m68k_incpc (6);
return 15 * CYCLE_UNIT / 2;
}

/* SUB.L #<data>.L,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_04a0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
{	uae_s32 dst = get_long_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}	m68k_incpc (6);
return 14 * CYCLE_UNIT / 2;
}

/* SUB.L #<data>.L,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_04a8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (6);
{	uae_s32 dst = get_long_jit (dsta);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}	m68k_incpc (8);
return 23 * CYCLE_UNIT / 2;
}

/* SUB.L #<data>.L,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_04b0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	m68k_incpc (6);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s32 dst = get_long_jit (dsta);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}}return 19 * CYCLE_UNIT / 2;
}

/* SUB.L #<data>.L,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_04b8_40)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (6);
{	uae_s32 dst = get_long_jit (dsta);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}	m68k_incpc (8);
return 16 * CYCLE_UNIT / 2;
}

/* SUB.L #<data>.L,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_04b9_40)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = get_dilong (6);
{	uae_s32 dst = get_long_jit (dsta);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}	m68k_incpc (10);
return 18 * CYCLE_UNIT / 2;
}

/* CHK2.L #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_04d0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	{uae_s32 upper,lower,reg = regs.regs[(extra >> 12) & 15];
	lower = get_long_jit (dsta); upper = get_long_jit (dsta + 4);
	SET_ZFLG (upper == reg || lower == reg);
	SET_CFLG_ALWAYS (lower <= upper ? reg < lower || reg > upper : reg > upper || reg < lower);
	if ((extra & 0x800) && GET_CFLG ()) { Exception_cpu(6); 
		return 8 * CYCLE_UNIT / 2;
	}
}
}}}	m68k_incpc (4);
return 22 * CYCLE_UNIT / 2;
}

/* CHK2.L #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_04e8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	{uae_s32 upper,lower,reg = regs.regs[(extra >> 12) & 15];
	lower = get_long_jit (dsta); upper = get_long_jit (dsta + 4);
	SET_ZFLG (upper == reg || lower == reg);
	SET_CFLG_ALWAYS (lower <= upper ? reg < lower || reg > upper : reg > upper || reg < lower);
	if ((extra & 0x800) && GET_CFLG ()) { Exception_cpu(6); 
		return 12 * CYCLE_UNIT / 2;
	}
}
}}}	m68k_incpc (6);
return 33 * CYCLE_UNIT / 2;
}

/* CHK2.L #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_04f0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
	{uae_s32 upper,lower,reg = regs.regs[(extra >> 12) & 15];
	lower = get_long_jit (dsta); upper = get_long_jit (dsta + 4);
	SET_ZFLG (upper == reg || lower == reg);
	SET_CFLG_ALWAYS (lower <= upper ? reg < lower || reg > upper : reg > upper || reg < lower);
	if ((extra & 0x800) && GET_CFLG ()) { Exception_cpu(6); 
		return 8 * CYCLE_UNIT / 2;
	}
}
}}}}return 29 * CYCLE_UNIT / 2;
}

/* CHK2.L #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_04f8_40)(uae_u32 opcode)
{
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	{uae_s32 upper,lower,reg = regs.regs[(extra >> 12) & 15];
	lower = get_long_jit (dsta); upper = get_long_jit (dsta + 4);
	SET_ZFLG (upper == reg || lower == reg);
	SET_CFLG_ALWAYS (lower <= upper ? reg < lower || reg > upper : reg > upper || reg < lower);
	if ((extra & 0x800) && GET_CFLG ()) { Exception_cpu(6); 
		return 12 * CYCLE_UNIT / 2;
	}
}
}}}	m68k_incpc (6);
return 25 * CYCLE_UNIT / 2;
}

/* CHK2.L #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_04f9_40)(uae_u32 opcode)
{
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
	{uae_s32 upper,lower,reg = regs.regs[(extra >> 12) & 15];
	lower = get_long_jit (dsta); upper = get_long_jit (dsta + 4);
	SET_ZFLG (upper == reg || lower == reg);
	SET_CFLG_ALWAYS (lower <= upper ? reg < lower || reg > upper : reg > upper || reg < lower);
	if ((extra & 0x800) && GET_CFLG ()) { Exception_cpu(6); 
		return 16 * CYCLE_UNIT / 2;
	}
}
}}}	m68k_incpc (8);
return 28 * CYCLE_UNIT / 2;
}

/* CHK2.L #<data>.W,(d16,PC) */
uae_u32 REGPARAM2 CPUFUNC(op_04fa_40)(uae_u32 opcode)
{
	uae_u32 dstreg = 2;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_getpc () + 4;
	dsta += (uae_s32)(uae_s16)get_diword (4);
	{uae_s32 upper,lower,reg = regs.regs[(extra >> 12) & 15];
	lower = get_long_jit (dsta); upper = get_long_jit (dsta + 4);
	SET_ZFLG (upper == reg || lower == reg);
	SET_CFLG_ALWAYS (lower <= upper ? reg < lower || reg > upper : reg > upper || reg < lower);
	if ((extra & 0x800) && GET_CFLG ()) { Exception_cpu(6); 
		return 12 * CYCLE_UNIT / 2;
	}
}
}}}	m68k_incpc (6);
return 33 * CYCLE_UNIT / 2;
}

/* CHK2.L #<data>.W,(d8,PC,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_04fb_40)(uae_u32 opcode)
{
	uae_u32 dstreg = 3;
{{	uae_s16 extra = get_diword (2);
{	uaecptr tmppc;
	uaecptr dsta;
	m68k_incpc (4);
{	tmppc = m68k_getpc ();
	dsta = get_disp_ea_020 (tmppc, 0);
	{uae_s32 upper,lower,reg = regs.regs[(extra >> 12) & 15];
	lower = get_long_jit (dsta); upper = get_long_jit (dsta + 4);
	SET_ZFLG (upper == reg || lower == reg);
	SET_CFLG_ALWAYS (lower <= upper ? reg < lower || reg > upper : reg > upper || reg < lower);
	if ((extra & 0x800) && GET_CFLG ()) { Exception_cpu(6); 
		return 8 * CYCLE_UNIT / 2;
	}
}
}}}}return 29 * CYCLE_UNIT / 2;
}

/* ADD.B #<data>.B,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0600_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
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
return 6 * CYCLE_UNIT / 2;
}

/* ADD.B #<data>.B,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0610_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
{{uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}	m68k_incpc (4);
return 10 * CYCLE_UNIT / 2;
}

/* ADD.B #<data>.B,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0618_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
{{uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}	m68k_incpc (4);
return 13 * CYCLE_UNIT / 2;
}

/* ADD.B #<data>.B,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0620_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* ADD.B #<data>.B,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0628_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte_jit (dsta);
{{uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}	m68k_incpc (6);
return 21 * CYCLE_UNIT / 2;
}

/* ADD.B #<data>.B,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0630_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s8 dst = get_byte_jit (dsta);
{{uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}}return 17 * CYCLE_UNIT / 2;
}

/* ADD.B #<data>.B,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0638_40)(uae_u32 opcode)
{
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte_jit (dsta);
{{uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* ADD.B #<data>.B,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0639_40)(uae_u32 opcode)
{
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_s8 dst = get_byte_jit (dsta);
{{uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}	m68k_incpc (8);
return 16 * CYCLE_UNIT / 2;
}

/* ADD.W #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0640_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
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
return 6 * CYCLE_UNIT / 2;
}

/* ADD.W #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0650_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word_jit (dsta);
{{uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}	m68k_incpc (4);
return 10 * CYCLE_UNIT / 2;
}

/* ADD.W #<data>.W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0658_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word_jit (dsta);
	m68k_areg (regs, dstreg) += 2;
{{uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}	m68k_incpc (4);
return 13 * CYCLE_UNIT / 2;
}

/* ADD.W #<data>.W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0660_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
{	uae_s16 dst = get_word_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* ADD.W #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0668_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_s16 dst = get_word_jit (dsta);
{{uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}	m68k_incpc (6);
return 21 * CYCLE_UNIT / 2;
}

/* ADD.W #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0670_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s16 dst = get_word_jit (dsta);
{{uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}}return 17 * CYCLE_UNIT / 2;
}

/* ADD.W #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0678_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_s16 dst = get_word_jit (dsta);
{{uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* ADD.W #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0679_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_s16 dst = get_word_jit (dsta);
{{uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}	m68k_incpc (8);
return 16 * CYCLE_UNIT / 2;
}

/* ADD.L #<data>.L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0680_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
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
return 8 * CYCLE_UNIT / 2;
}

/* ADD.L #<data>.L,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0690_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long_jit (dsta);
{{uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* ADD.L #<data>.L,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0698_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long_jit (dsta);
	m68k_areg (regs, dstreg) += 4;
{{uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}	m68k_incpc (6);
return 15 * CYCLE_UNIT / 2;
}

/* ADD.L #<data>.L,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_06a0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
{	uae_s32 dst = get_long_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}	m68k_incpc (6);
return 14 * CYCLE_UNIT / 2;
}

/* ADD.L #<data>.L,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_06a8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (6);
{	uae_s32 dst = get_long_jit (dsta);
{{uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}	m68k_incpc (8);
return 23 * CYCLE_UNIT / 2;
}

/* ADD.L #<data>.L,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_06b0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	m68k_incpc (6);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s32 dst = get_long_jit (dsta);
{{uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}}return 19 * CYCLE_UNIT / 2;
}

/* ADD.L #<data>.L,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_06b8_40)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (6);
{	uae_s32 dst = get_long_jit (dsta);
{{uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}	m68k_incpc (8);
return 16 * CYCLE_UNIT / 2;
}

/* ADD.L #<data>.L,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_06b9_40)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = get_dilong (6);
{	uae_s32 dst = get_long_jit (dsta);
{{uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}	m68k_incpc (10);
return 18 * CYCLE_UNIT / 2;
}

/* RTM.L Dn */
uae_u32 REGPARAM2 CPUFUNC(op_06c0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{	m68k_incpc (2);
	op_illg (opcode);
}return 4 * CYCLE_UNIT / 2;
}

/* RTM.L An */
uae_u32 REGPARAM2 CPUFUNC(op_06c8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{	m68k_incpc (2);
	op_illg (opcode);
}return 4 * CYCLE_UNIT / 2;
}

/* CALLM.L (An) */
uae_u32 REGPARAM2 CPUFUNC(op_06d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{	m68k_incpc (2);
	op_illg (opcode);
}return 4 * CYCLE_UNIT / 2;
}

/* CALLM.L (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_06e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{	m68k_incpc (2);
	op_illg (opcode);
}return 4 * CYCLE_UNIT / 2;
}

/* CALLM.L (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_06f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{	m68k_incpc (2);
	op_illg (opcode);
}return 4 * CYCLE_UNIT / 2;
}

/* CALLM.L (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_06f8_40)(uae_u32 opcode)
{
{	m68k_incpc (2);
	op_illg (opcode);
}return 4 * CYCLE_UNIT / 2;
}

/* CALLM.L (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_06f9_40)(uae_u32 opcode)
{
{	m68k_incpc (2);
	op_illg (opcode);
}return 4 * CYCLE_UNIT / 2;
}

/* CALLM.L (d16,PC) */
uae_u32 REGPARAM2 CPUFUNC(op_06fa_40)(uae_u32 opcode)
{
{	m68k_incpc (2);
	op_illg (opcode);
}return 4 * CYCLE_UNIT / 2;
}

/* CALLM.L (d8,PC,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_06fb_40)(uae_u32 opcode)
{
{	m68k_incpc (2);
	op_illg (opcode);
}return 4 * CYCLE_UNIT / 2;
}

/* BTST.L #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0800_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src &= 31;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}	m68k_incpc (4);
return 5 * CYCLE_UNIT / 2;
}

/* BTST.B #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0810_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* BTST.B #<data>.W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0818_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* BTST.B #<data>.W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0820_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (4);
return 11 * CYCLE_UNIT / 2;
}

/* BTST.B #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0828_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* BTST.B #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0830_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}}return 16 * CYCLE_UNIT / 2;
}

/* BTST.B #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0838_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (6);
return 12 * CYCLE_UNIT / 2;
}

/* BTST.B #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0839_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (8);
return 15 * CYCLE_UNIT / 2;
}

/* BTST.B #<data>.W,(d16,PC) */
uae_u32 REGPARAM2 CPUFUNC(op_083a_40)(uae_u32 opcode)
{
	uae_u32 dstreg = 2;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_getpc () + 4;
	dsta += (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* BTST.B #<data>.W,(d8,PC,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_083b_40)(uae_u32 opcode)
{
	uae_u32 dstreg = 3;
{{	uae_s16 src = get_diword (2);
{	uaecptr tmppc;
	uaecptr dsta;
	m68k_incpc (4);
{	tmppc = m68k_getpc ();
	dsta = get_disp_ea_020 (tmppc, 0);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}}return 16 * CYCLE_UNIT / 2;
}

/* BCHG.L #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0840_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src &= 31;
	dst ^= (1 << src);
	SET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	m68k_dreg (regs, dstreg) = (dst);
}}}	m68k_incpc (4);
return 6 * CYCLE_UNIT / 2;
}

/* BCHG.B #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0850_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	put_byte_jit (dsta,dst);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* BCHG.B #<data>.W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0858_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	put_byte_jit (dsta,dst);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* BCHG.B #<data>.W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0860_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	put_byte_jit (dsta,dst);
}}}}	m68k_incpc (4);
return 11 * CYCLE_UNIT / 2;
}

/* BCHG.B #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0868_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	put_byte_jit (dsta,dst);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* BCHG.B #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0870_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	put_byte_jit (dsta,dst);
}}}}}return 16 * CYCLE_UNIT / 2;
}

/* BCHG.B #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0878_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	put_byte_jit (dsta,dst);
}}}}	m68k_incpc (6);
return 12 * CYCLE_UNIT / 2;
}

/* BCHG.B #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0879_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	put_byte_jit (dsta,dst);
}}}}	m68k_incpc (8);
return 15 * CYCLE_UNIT / 2;
}

/* BCLR.L #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0880_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src &= 31;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	m68k_dreg (regs, dstreg) = (dst);
}}}	m68k_incpc (4);
return 6 * CYCLE_UNIT / 2;
}

/* BCLR.B #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0890_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit (dsta,dst);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* BCLR.B #<data>.W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0898_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit (dsta,dst);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* BCLR.B #<data>.W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_08a0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit (dsta,dst);
}}}}	m68k_incpc (4);
return 11 * CYCLE_UNIT / 2;
}

/* BCLR.B #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_08a8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit (dsta,dst);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* BCLR.B #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_08b0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit (dsta,dst);
}}}}}return 16 * CYCLE_UNIT / 2;
}

/* BCLR.B #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_08b8_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit (dsta,dst);
}}}}	m68k_incpc (6);
return 12 * CYCLE_UNIT / 2;
}

/* BCLR.B #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_08b9_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit (dsta,dst);
}}}}	m68k_incpc (8);
return 15 * CYCLE_UNIT / 2;
}

/* BSET.L #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_08c0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src &= 31;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	m68k_dreg (regs, dstreg) = (dst);
}}}	m68k_incpc (4);
return 6 * CYCLE_UNIT / 2;
}

/* BSET.B #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_08d0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte_jit (dsta,dst);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* BSET.B #<data>.W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_08d8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte_jit (dsta,dst);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* BSET.B #<data>.W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_08e0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte_jit (dsta,dst);
}}}}	m68k_incpc (4);
return 11 * CYCLE_UNIT / 2;
}

/* BSET.B #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_08e8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte_jit (dsta,dst);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* BSET.B #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_08f0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte_jit (dsta,dst);
}}}}}return 16 * CYCLE_UNIT / 2;
}

/* BSET.B #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_08f8_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte_jit (dsta,dst);
}}}}	m68k_incpc (6);
return 12 * CYCLE_UNIT / 2;
}

/* BSET.B #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_08f9_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= 7;
	SET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte_jit (dsta,dst);
}}}}	m68k_incpc (8);
return 15 * CYCLE_UNIT / 2;
}

/* EOR.B #<data>.B,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0a00_40)(uae_u32 opcode)
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
return 6 * CYCLE_UNIT / 2;
}

/* EOR.B #<data>.B,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0a10_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}	m68k_incpc (4);
return 10 * CYCLE_UNIT / 2;
}

/* EOR.B #<data>.B,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0a18_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}	m68k_incpc (4);
return 13 * CYCLE_UNIT / 2;
}

/* EOR.B #<data>.B,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0a20_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* EOR.B #<data>.B,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0a28_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte_jit (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}	m68k_incpc (6);
return 21 * CYCLE_UNIT / 2;
}

/* EOR.B #<data>.B,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0a30_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s8 dst = get_byte_jit (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}}return 17 * CYCLE_UNIT / 2;
}

/* EOR.B #<data>.B,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0a38_40)(uae_u32 opcode)
{
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte_jit (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* EOR.B #<data>.B,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0a39_40)(uae_u32 opcode)
{
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_s8 dst = get_byte_jit (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}	m68k_incpc (8);
return 16 * CYCLE_UNIT / 2;
}

/* EORSR.B #<data>.W */
uae_u32 REGPARAM2 CPUFUNC(op_0a3c_40)(uae_u32 opcode)
{
{	MakeSR ();
{	uae_s16 src = get_diword (2);
	src &= 0xFF;
	regs.sr ^= src;
	MakeFromSR_T0();
	m68k_incpc (4);
}}return 15 * CYCLE_UNIT / 2;
}

/* EOR.W #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0a40_40)(uae_u32 opcode)
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
return 6 * CYCLE_UNIT / 2;
}

/* EOR.W #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0a50_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word_jit (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}	m68k_incpc (4);
return 10 * CYCLE_UNIT / 2;
}

/* EOR.W #<data>.W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0a58_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word_jit (dsta);
	m68k_areg (regs, dstreg) += 2;
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}	m68k_incpc (4);
return 13 * CYCLE_UNIT / 2;
}

/* EOR.W #<data>.W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0a60_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
{	uae_s16 dst = get_word_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* EOR.W #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0a68_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_s16 dst = get_word_jit (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}	m68k_incpc (6);
return 21 * CYCLE_UNIT / 2;
}

/* EOR.W #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0a70_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s16 dst = get_word_jit (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}}return 17 * CYCLE_UNIT / 2;
}

/* EOR.W #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0a78_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_s16 dst = get_word_jit (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* EOR.W #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0a79_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_s16 dst = get_word_jit (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}	m68k_incpc (8);
return 16 * CYCLE_UNIT / 2;
}

/* EORSR.W #<data>.W */
uae_u32 REGPARAM2 CPUFUNC(op_0a7c_40)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	MakeSR ();
{	uae_s16 src = get_diword (2);
	regs.sr ^= src;
	MakeFromSR_T0();
	m68k_incpc (4);
}}}return 15 * CYCLE_UNIT / 2;
}

/* EOR.L #<data>.L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0a80_40)(uae_u32 opcode)
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
return 8 * CYCLE_UNIT / 2;
}

/* EOR.L #<data>.L,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0a90_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long_jit (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* EOR.L #<data>.L,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0a98_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long_jit (dsta);
	m68k_areg (regs, dstreg) += 4;
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}	m68k_incpc (6);
return 15 * CYCLE_UNIT / 2;
}

/* EOR.L #<data>.L,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0aa0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
{	uae_s32 dst = get_long_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}	m68k_incpc (6);
return 14 * CYCLE_UNIT / 2;
}

/* EOR.L #<data>.L,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0aa8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (6);
{	uae_s32 dst = get_long_jit (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}	m68k_incpc (8);
return 23 * CYCLE_UNIT / 2;
}

/* EOR.L #<data>.L,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0ab0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	m68k_incpc (6);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s32 dst = get_long_jit (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}}return 19 * CYCLE_UNIT / 2;
}

/* EOR.L #<data>.L,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0ab8_40)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (6);
{	uae_s32 dst = get_long_jit (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}	m68k_incpc (8);
return 16 * CYCLE_UNIT / 2;
}

/* EOR.L #<data>.L,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0ab9_40)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = get_dilong (6);
{	uae_s32 dst = get_long_jit (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}	m68k_incpc (10);
return 18 * CYCLE_UNIT / 2;
}

/* CAS.B #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0ad0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
{	int ru = (src >> 6) & 7;
	int rc = src & 7;
{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(m68k_dreg (regs, rc)));
{	int flgs = ((uae_s8)(m68k_dreg (regs, rc))) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(m68k_dreg (regs, rc))) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
	if (GET_ZFLG ()){
		put_byte_jit (dsta, (m68k_dreg (regs, ru)));
	}else{
		put_byte_jit (dsta, dst);
		m68k_dreg(regs, rc) = (m68k_dreg(regs, rc) & ~0xff) | (dst & 0xff);
}}}}}}}}m68k_incpc (4);
	return 16 * CYCLE_UNIT / 2;
}

/* CAS.B #<data>.W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0ad8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
{	int ru = (src >> 6) & 7;
	int rc = src & 7;
{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(m68k_dreg (regs, rc)));
{	int flgs = ((uae_s8)(m68k_dreg (regs, rc))) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(m68k_dreg (regs, rc))) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
	if (GET_ZFLG ()){
		put_byte_jit (dsta, (m68k_dreg (regs, ru)));
	}else{
		put_byte_jit (dsta, dst);
		m68k_dreg(regs, rc) = (m68k_dreg(regs, rc) & ~0xff) | (dst & 0xff);
}}}}}}}}m68k_incpc (4);
	return 18 * CYCLE_UNIT / 2;
}

/* CAS.B #<data>.W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0ae0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
{	int ru = (src >> 6) & 7;
	int rc = src & 7;
{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(m68k_dreg (regs, rc)));
{	int flgs = ((uae_s8)(m68k_dreg (regs, rc))) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(m68k_dreg (regs, rc))) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
	if (GET_ZFLG ()){
		put_byte_jit (dsta, (m68k_dreg (regs, ru)));
	}else{
		put_byte_jit (dsta, dst);
		m68k_dreg(regs, rc) = (m68k_dreg(regs, rc) & ~0xff) | (dst & 0xff);
}}}}}}}}m68k_incpc (4);
	return 19 * CYCLE_UNIT / 2;
}

/* CAS.B #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0ae8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte_jit (dsta);
{	int ru = (src >> 6) & 7;
	int rc = src & 7;
{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(m68k_dreg (regs, rc)));
{	int flgs = ((uae_s8)(m68k_dreg (regs, rc))) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(m68k_dreg (regs, rc))) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
	if (GET_ZFLG ()){
		put_byte_jit (dsta, (m68k_dreg (regs, ru)));
	}else{
		put_byte_jit (dsta, dst);
		m68k_dreg(regs, rc) = (m68k_dreg(regs, rc) & ~0xff) | (dst & 0xff);
}}}}}}}}m68k_incpc (6);
	return 26 * CYCLE_UNIT / 2;
}

#endif

#ifdef PART_2
/* CAS.B #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0af0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s8 dst = get_byte_jit (dsta);
{	int ru = (src >> 6) & 7;
	int rc = src & 7;
{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(m68k_dreg (regs, rc)));
{	int flgs = ((uae_s8)(m68k_dreg (regs, rc))) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(m68k_dreg (regs, rc))) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
	if (GET_ZFLG ()){
		put_byte_jit (dsta, (m68k_dreg (regs, ru)));
	}else{
		put_byte_jit (dsta, dst);
		m68k_dreg(regs, rc) = (m68k_dreg(regs, rc) & ~0xff) | (dst & 0xff);
}}}}}}}}}return 21 * CYCLE_UNIT / 2;
}

/* CAS.B #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0af8_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte_jit (dsta);
{	int ru = (src >> 6) & 7;
	int rc = src & 7;
{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(m68k_dreg (regs, rc)));
{	int flgs = ((uae_s8)(m68k_dreg (regs, rc))) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(m68k_dreg (regs, rc))) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
	if (GET_ZFLG ()){
		put_byte_jit (dsta, (m68k_dreg (regs, ru)));
	}else{
		put_byte_jit (dsta, dst);
		m68k_dreg(regs, rc) = (m68k_dreg(regs, rc) & ~0xff) | (dst & 0xff);
}}}}}}}}m68k_incpc (6);
	return 18 * CYCLE_UNIT / 2;
}

/* CAS.B #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0af9_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_s8 dst = get_byte_jit (dsta);
{	int ru = (src >> 6) & 7;
	int rc = src & 7;
{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(m68k_dreg (regs, rc)));
{	int flgs = ((uae_s8)(m68k_dreg (regs, rc))) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(m68k_dreg (regs, rc))) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
	if (GET_ZFLG ()){
		put_byte_jit (dsta, (m68k_dreg (regs, ru)));
	}else{
		put_byte_jit (dsta, dst);
		m68k_dreg(regs, rc) = (m68k_dreg(regs, rc) & ~0xff) | (dst & 0xff);
}}}}}}}}m68k_incpc (8);
	return 19 * CYCLE_UNIT / 2;
}

/* CMP.B #<data>.B,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0c00_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}	m68k_incpc (4);
return 6 * CYCLE_UNIT / 2;
}

/* CMP.B #<data>.B,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0c10_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 7 * CYCLE_UNIT / 2;
}

/* CMP.B #<data>.B,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0c18_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 10 * CYCLE_UNIT / 2;
}

/* CMP.B #<data>.B,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0c20_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* CMP.B #<data>.B,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0c28_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte_jit (dsta);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
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

/* CMP.B #<data>.B,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0c30_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s8 dst = get_byte_jit (dsta);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}}return 14 * CYCLE_UNIT / 2;
}

/* CMP.B #<data>.B,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0c38_40)(uae_u32 opcode)
{
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte_jit (dsta);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (6);
return 10 * CYCLE_UNIT / 2;
}

/* CMP.B #<data>.B,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0c39_40)(uae_u32 opcode)
{
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_s8 dst = get_byte_jit (dsta);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (8);
return 13 * CYCLE_UNIT / 2;
}

/* CMP.B #<data>.B,(d16,PC) */
uae_u32 REGPARAM2 CPUFUNC(op_0c3a_40)(uae_u32 opcode)
{
	uae_u32 dstreg = 2;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_getpc () + 4;
	dsta += (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 dst = get_byte_jit (dsta);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
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

/* CMP.B #<data>.B,(d8,PC,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0c3b_40)(uae_u32 opcode)
{
	uae_u32 dstreg = 3;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr tmppc;
	uaecptr dsta;
	m68k_incpc (4);
{	tmppc = m68k_getpc ();
	dsta = get_disp_ea_020 (tmppc, 0);
{	uae_s8 dst = get_byte_jit (dsta);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}}return 14 * CYCLE_UNIT / 2;
}

/* CMP.W #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0c40_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}	m68k_incpc (4);
return 6 * CYCLE_UNIT / 2;
}

/* CMP.W #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0c50_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word_jit (dsta);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 7 * CYCLE_UNIT / 2;
}

/* CMP.W #<data>.W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0c58_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word_jit (dsta);
	m68k_areg (regs, dstreg) += 2;
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 10 * CYCLE_UNIT / 2;
}

/* CMP.W #<data>.W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0c60_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
{	uae_s16 dst = get_word_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* CMP.W #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0c68_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_s16 dst = get_word_jit (dsta);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
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

/* CMP.W #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0c70_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s16 dst = get_word_jit (dsta);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}}return 14 * CYCLE_UNIT / 2;
}

/* CMP.W #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0c78_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_s16 dst = get_word_jit (dsta);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (6);
return 10 * CYCLE_UNIT / 2;
}

/* CMP.W #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0c79_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_s16 dst = get_word_jit (dsta);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (8);
return 13 * CYCLE_UNIT / 2;
}

/* CMP.W #<data>.W,(d16,PC) */
uae_u32 REGPARAM2 CPUFUNC(op_0c7a_40)(uae_u32 opcode)
{
	uae_u32 dstreg = 2;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_getpc () + 4;
	dsta += (uae_s32)(uae_s16)get_diword (4);
{	uae_s16 dst = get_word_jit (dsta);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
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

/* CMP.W #<data>.W,(d8,PC,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0c7b_40)(uae_u32 opcode)
{
	uae_u32 dstreg = 3;
{{	uae_s16 src = get_diword (2);
{	uaecptr tmppc;
	uaecptr dsta;
	m68k_incpc (4);
{	tmppc = m68k_getpc ();
	dsta = get_disp_ea_020 (tmppc, 0);
{	uae_s16 dst = get_word_jit (dsta);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}}return 14 * CYCLE_UNIT / 2;
}

/* CMP.L #<data>.L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_0c80_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}	m68k_incpc (6);
return 8 * CYCLE_UNIT / 2;
}

/* CMP.L #<data>.L,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0c90_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long_jit (dsta);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (6);
return 10 * CYCLE_UNIT / 2;
}

/* CMP.L #<data>.L,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0c98_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long_jit (dsta);
	m68k_areg (regs, dstreg) += 4;
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (6);
return 12 * CYCLE_UNIT / 2;
}

/* CMP.L #<data>.L,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0ca0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
{	uae_s32 dst = get_long_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (6);
return 11 * CYCLE_UNIT / 2;
}

/* CMP.L #<data>.L,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0ca8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (6);
{	uae_s32 dst = get_long_jit (dsta);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (8);
return 20 * CYCLE_UNIT / 2;
}

/* CMP.L #<data>.L,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0cb0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	m68k_incpc (6);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s32 dst = get_long_jit (dsta);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}}return 16 * CYCLE_UNIT / 2;
}

/* CMP.L #<data>.L,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0cb8_40)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (6);
{	uae_s32 dst = get_long_jit (dsta);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (8);
return 13 * CYCLE_UNIT / 2;
}

/* CMP.L #<data>.L,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0cb9_40)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = get_dilong (6);
{	uae_s32 dst = get_long_jit (dsta);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (10);
return 15 * CYCLE_UNIT / 2;
}

/* CMP.L #<data>.L,(d16,PC) */
uae_u32 REGPARAM2 CPUFUNC(op_0cba_40)(uae_u32 opcode)
{
	uae_u32 dstreg = 2;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_getpc () + 6;
	dsta += (uae_s32)(uae_s16)get_diword (6);
{	uae_s32 dst = get_long_jit (dsta);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (8);
return 20 * CYCLE_UNIT / 2;
}

/* CMP.L #<data>.L,(d8,PC,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0cbb_40)(uae_u32 opcode)
{
	uae_u32 dstreg = 3;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr tmppc;
	uaecptr dsta;
	m68k_incpc (6);
{	tmppc = m68k_getpc ();
	dsta = get_disp_ea_020 (tmppc, 0);
{	uae_s32 dst = get_long_jit (dsta);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}}return 16 * CYCLE_UNIT / 2;
}

/* CAS.W #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0cd0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word_jit (dsta);
{	int ru = (src >> 6) & 7;
	int rc = src & 7;
{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(m68k_dreg (regs, rc)));
{	int flgs = ((uae_s16)(m68k_dreg (regs, rc))) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(m68k_dreg (regs, rc))) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
	if (GET_ZFLG ()){
		put_word_jit (dsta, (m68k_dreg (regs, ru)));
	}else{
		put_word_jit (dsta, dst);
		m68k_dreg(regs, rc) = (m68k_dreg(regs, rc) & ~0xffff) | (dst & 0xffff);
}}}}}}}}m68k_incpc (4);
	return 16 * CYCLE_UNIT / 2;
}

/* CAS.W #<data>.W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0cd8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word_jit (dsta);
	m68k_areg (regs, dstreg) += 2;
{	int ru = (src >> 6) & 7;
	int rc = src & 7;
{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(m68k_dreg (regs, rc)));
{	int flgs = ((uae_s16)(m68k_dreg (regs, rc))) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(m68k_dreg (regs, rc))) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
	if (GET_ZFLG ()){
		put_word_jit (dsta, (m68k_dreg (regs, ru)));
	}else{
		put_word_jit (dsta, dst);
		m68k_dreg(regs, rc) = (m68k_dreg(regs, rc) & ~0xffff) | (dst & 0xffff);
}}}}}}}}m68k_incpc (4);
	return 18 * CYCLE_UNIT / 2;
}

/* CAS.W #<data>.W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0ce0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
{	uae_s16 dst = get_word_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
{	int ru = (src >> 6) & 7;
	int rc = src & 7;
{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(m68k_dreg (regs, rc)));
{	int flgs = ((uae_s16)(m68k_dreg (regs, rc))) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(m68k_dreg (regs, rc))) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
	if (GET_ZFLG ()){
		put_word_jit (dsta, (m68k_dreg (regs, ru)));
	}else{
		put_word_jit (dsta, dst);
		m68k_dreg(regs, rc) = (m68k_dreg(regs, rc) & ~0xffff) | (dst & 0xffff);
}}}}}}}}m68k_incpc (4);
	return 19 * CYCLE_UNIT / 2;
}

/* CAS.W #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0ce8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_s16 dst = get_word_jit (dsta);
{	int ru = (src >> 6) & 7;
	int rc = src & 7;
{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(m68k_dreg (regs, rc)));
{	int flgs = ((uae_s16)(m68k_dreg (regs, rc))) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(m68k_dreg (regs, rc))) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
	if (GET_ZFLG ()){
		put_word_jit (dsta, (m68k_dreg (regs, ru)));
	}else{
		put_word_jit (dsta, dst);
		m68k_dreg(regs, rc) = (m68k_dreg(regs, rc) & ~0xffff) | (dst & 0xffff);
}}}}}}}}m68k_incpc (6);
	return 26 * CYCLE_UNIT / 2;
}

/* CAS.W #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0cf0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s16 dst = get_word_jit (dsta);
{	int ru = (src >> 6) & 7;
	int rc = src & 7;
{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(m68k_dreg (regs, rc)));
{	int flgs = ((uae_s16)(m68k_dreg (regs, rc))) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(m68k_dreg (regs, rc))) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
	if (GET_ZFLG ()){
		put_word_jit (dsta, (m68k_dreg (regs, ru)));
	}else{
		put_word_jit (dsta, dst);
		m68k_dreg(regs, rc) = (m68k_dreg(regs, rc) & ~0xffff) | (dst & 0xffff);
}}}}}}}}}return 21 * CYCLE_UNIT / 2;
}

/* CAS.W #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0cf8_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_s16 dst = get_word_jit (dsta);
{	int ru = (src >> 6) & 7;
	int rc = src & 7;
{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(m68k_dreg (regs, rc)));
{	int flgs = ((uae_s16)(m68k_dreg (regs, rc))) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(m68k_dreg (regs, rc))) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
	if (GET_ZFLG ()){
		put_word_jit (dsta, (m68k_dreg (regs, ru)));
	}else{
		put_word_jit (dsta, dst);
		m68k_dreg(regs, rc) = (m68k_dreg(regs, rc) & ~0xffff) | (dst & 0xffff);
}}}}}}}}m68k_incpc (6);
	return 18 * CYCLE_UNIT / 2;
}

/* CAS.W #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0cf9_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_s16 dst = get_word_jit (dsta);
{	int ru = (src >> 6) & 7;
	int rc = src & 7;
{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(m68k_dreg (regs, rc)));
{	int flgs = ((uae_s16)(m68k_dreg (regs, rc))) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(m68k_dreg (regs, rc))) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
	if (GET_ZFLG ()){
		put_word_jit (dsta, (m68k_dreg (regs, ru)));
	}else{
		put_word_jit (dsta, dst);
		m68k_dreg(regs, rc) = (m68k_dreg(regs, rc) & ~0xffff) | (dst & 0xffff);
}}}}}}}}m68k_incpc (8);
	return 19 * CYCLE_UNIT / 2;
}

/* CAS2.W #<data>.L */
uae_u32 REGPARAM2 CPUFUNC(op_0cfc_40)(uae_u32 opcode)
{
{{	uae_s32 extra;
	extra = get_dilong (2);
	uae_u32 rn1 = regs.regs[(extra >> 28) & 15];
	uae_u32 rn2 = regs.regs[(extra >> 12) & 15];
	uae_u16 dst1 = get_word_jit (rn1), dst2 = get_word_jit (rn2);
{uae_u32 newv = ((uae_u16)(dst1)) - ((uae_u16)(m68k_dreg (regs, (extra >> 16) & 7)));
{	int flgs = ((uae_s16)(m68k_dreg (regs, (extra >> 16) & 7))) < 0;
	int flgo = ((uae_s16)(dst1)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(m68k_dreg (regs, (extra >> 16) & 7))) > ((uae_u16)(dst1)));
	SET_NFLG (flgn != 0);
	if (GET_ZFLG ()) {
{uae_u32 newv = ((uae_u16)(dst2)) - ((uae_u16)(m68k_dreg (regs, extra & 7)));
{	int flgs = ((uae_s16)(m68k_dreg (regs, extra & 7))) < 0;
	int flgo = ((uae_s16)(dst2)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(m68k_dreg (regs, extra & 7))) > ((uae_u16)(dst2)));
	SET_NFLG (flgn != 0);
	if (GET_ZFLG ()) {
	put_word_jit (rn1, m68k_dreg (regs, (extra >> 22) & 7));
	put_word_jit (rn2, m68k_dreg (regs, (extra >> 6) & 7));
	}}
}}}}	if (! GET_ZFLG ()) {
	m68k_dreg (regs, (extra >> 0) & 7) = (m68k_dreg (regs, (extra >> 6) & 7) & ~0xffff) | (dst2 & 0xffff);
	m68k_dreg (regs, (extra >> 16) & 7) = (m68k_dreg (regs, (extra >> 22) & 7) & ~0xffff) | (dst1 & 0xffff);
	}
}}	m68k_incpc (6);
return 25 * CYCLE_UNIT / 2;
}

/* MOVES.B #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0e10_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uae_s16 extra = get_diword (2);
	if (extra & 0x800)
{	uae_u32 src = regs.regs[(extra >> 12) & 15];
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	put_byte_jit (dsta,src);
}}else{{	uaecptr srca;
	srca = m68k_areg (regs, dstreg);
{	uae_s8 src = get_byte_jit (srca);
	if (extra & 0x8000) {
	m68k_areg (regs, (extra >> 12) & 7) = (uae_s32)(uae_s8)src;
	} else {
	m68k_dreg (regs, (extra >> 12) & 7) = (m68k_dreg (regs, (extra >> 12) & 7) & ~0xff) | ((src) & 0xff);
	}
}}}}}}	m68k_incpc (4);
return 10 * CYCLE_UNIT / 2;
}

/* MOVES.B #<data>.W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0e18_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uae_s16 extra = get_diword (2);
	if (extra & 0x800)
{	uae_u32 src = regs.regs[(extra >> 12) & 15];
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	put_byte_jit (dsta,src);
}}else{{	uaecptr srca;
	srca = m68k_areg (regs, dstreg);
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	if (extra & 0x8000) {
	m68k_areg (regs, (extra >> 12) & 7) = (uae_s32)(uae_s8)src;
	} else {
	m68k_dreg (regs, (extra >> 12) & 7) = (m68k_dreg (regs, (extra >> 12) & 7) & ~0xff) | ((src) & 0xff);
	}
}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* MOVES.B #<data>.W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0e20_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uae_s16 extra = get_diword (2);
	if (extra & 0x800)
{	uae_u32 src = regs.regs[(extra >> 12) & 15];
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg (regs, dstreg) = dsta;
	put_byte_jit (dsta,src);
}}else{{	uaecptr srca;
	srca = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, dstreg) = srca;
	if (extra & 0x8000) {
	m68k_areg (regs, (extra >> 12) & 7) = (uae_s32)(uae_s8)src;
	} else {
	m68k_dreg (regs, (extra >> 12) & 7) = (m68k_dreg (regs, (extra >> 12) & 7) & ~0xff) | ((src) & 0xff);
	}
}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* MOVES.B #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0e28_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uae_s16 extra = get_diword (2);
	if (extra & 0x800)
{	uae_u32 src = regs.regs[(extra >> 12) & 15];
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	put_byte_jit (dsta,src);
}}else{{	uaecptr srca;
	srca = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 src = get_byte_jit (srca);
	if (extra & 0x8000) {
	m68k_areg (regs, (extra >> 12) & 7) = (uae_s32)(uae_s8)src;
	} else {
	m68k_dreg (regs, (extra >> 12) & 7) = (m68k_dreg (regs, (extra >> 12) & 7) & ~0xff) | ((src) & 0xff);
	}
}}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* MOVES.B #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0e30_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uae_s16 extra = get_diword (2);
	if (extra & 0x800)
{	uae_u32 src = regs.regs[(extra >> 12) & 15];
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
	put_byte_jit (dsta,src);
}}}else{{	uaecptr srca;
	m68k_incpc (4);
{	srca = get_disp_ea_020 (m68k_areg (regs, dstreg), 1);
{	uae_s8 src = get_byte_jit (srca);
	if (extra & 0x8000) {
	m68k_areg (regs, (extra >> 12) & 7) = (uae_s32)(uae_s8)src;
	} else {
	m68k_dreg (regs, (extra >> 12) & 7) = (m68k_dreg (regs, (extra >> 12) & 7) & ~0xff) | ((src) & 0xff);
	}
}}}}}}}return 15 * CYCLE_UNIT / 2;
}

/* MOVES.B #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0e38_40)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uae_s16 extra = get_diword (2);
	if (extra & 0x800)
{	uae_u32 src = regs.regs[(extra >> 12) & 15];
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	put_byte_jit (dsta,src);
}}else{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (4);
{	uae_s8 src = get_byte_jit (srca);
	if (extra & 0x8000) {
	m68k_areg (regs, (extra >> 12) & 7) = (uae_s32)(uae_s8)src;
	} else {
	m68k_dreg (regs, (extra >> 12) & 7) = (m68k_dreg (regs, (extra >> 12) & 7) & ~0xff) | ((src) & 0xff);
	}
}}}}}}	m68k_incpc (6);
return 12 * CYCLE_UNIT / 2;
}

/* MOVES.B #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0e39_40)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uae_s16 extra = get_diword (2);
	if (extra & 0x800)
{	uae_u32 src = regs.regs[(extra >> 12) & 15];
{	uaecptr dsta;
	dsta = get_dilong (4);
	put_byte_jit (dsta,src);
}}else{{	uaecptr srca;
	srca = get_dilong (4);
{	uae_s8 src = get_byte_jit (srca);
	if (extra & 0x8000) {
	m68k_areg (regs, (extra >> 12) & 7) = (uae_s32)(uae_s8)src;
	} else {
	m68k_dreg (regs, (extra >> 12) & 7) = (m68k_dreg (regs, (extra >> 12) & 7) & ~0xff) | ((src) & 0xff);
	}
}}}}}}	m68k_incpc (8);
return 13 * CYCLE_UNIT / 2;
}

/* MOVES.W #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0e50_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uae_s16 extra = get_diword (2);
	if (extra & 0x800)
{	uae_u32 src = regs.regs[(extra >> 12) & 15];
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	put_word_jit (dsta,src);
}}else{{	uaecptr srca;
	srca = m68k_areg (regs, dstreg);
{	uae_s16 src = get_word_jit (srca);
	if (extra & 0x8000) {
	m68k_areg (regs, (extra >> 12) & 7) = (uae_s32)(uae_s16)src;
	} else {
	m68k_dreg (regs, (extra >> 12) & 7) = (m68k_dreg (regs, (extra >> 12) & 7) & ~0xffff) | ((src) & 0xffff);
	}
}}}}}}	m68k_incpc (4);
return 10 * CYCLE_UNIT / 2;
}

/* MOVES.W #<data>.W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0e58_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uae_s16 extra = get_diword (2);
	if (extra & 0x800)
{	uae_u32 src = regs.regs[(extra >> 12) & 15];
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 2;
	put_word_jit (dsta,src);
}}else{{	uaecptr srca;
	srca = m68k_areg (regs, dstreg);
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, dstreg) += 2;
	if (extra & 0x8000) {
	m68k_areg (regs, (extra >> 12) & 7) = (uae_s32)(uae_s16)src;
	} else {
	m68k_dreg (regs, (extra >> 12) & 7) = (m68k_dreg (regs, (extra >> 12) & 7) & ~0xffff) | ((src) & 0xffff);
	}
}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* MOVES.W #<data>.W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0e60_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uae_s16 extra = get_diword (2);
	if (extra & 0x800)
{	uae_u32 src = regs.regs[(extra >> 12) & 15];
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
	m68k_areg (regs, dstreg) = dsta;
	put_word_jit (dsta,src);
}}else{{	uaecptr srca;
	srca = m68k_areg (regs, dstreg) - 2;
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, dstreg) = srca;
	if (extra & 0x8000) {
	m68k_areg (regs, (extra >> 12) & 7) = (uae_s32)(uae_s16)src;
	} else {
	m68k_dreg (regs, (extra >> 12) & 7) = (m68k_dreg (regs, (extra >> 12) & 7) & ~0xffff) | ((src) & 0xffff);
	}
}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* MOVES.W #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0e68_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uae_s16 extra = get_diword (2);
	if (extra & 0x800)
{	uae_u32 src = regs.regs[(extra >> 12) & 15];
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	put_word_jit (dsta,src);
}}else{{	uaecptr srca;
	srca = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_s16 src = get_word_jit (srca);
	if (extra & 0x8000) {
	m68k_areg (regs, (extra >> 12) & 7) = (uae_s32)(uae_s16)src;
	} else {
	m68k_dreg (regs, (extra >> 12) & 7) = (m68k_dreg (regs, (extra >> 12) & 7) & ~0xffff) | ((src) & 0xffff);
	}
}}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* MOVES.W #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0e70_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uae_s16 extra = get_diword (2);
	if (extra & 0x800)
{	uae_u32 src = regs.regs[(extra >> 12) & 15];
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
	put_word_jit (dsta,src);
}}}else{{	uaecptr srca;
	m68k_incpc (4);
{	srca = get_disp_ea_020 (m68k_areg (regs, dstreg), 1);
{	uae_s16 src = get_word_jit (srca);
	if (extra & 0x8000) {
	m68k_areg (regs, (extra >> 12) & 7) = (uae_s32)(uae_s16)src;
	} else {
	m68k_dreg (regs, (extra >> 12) & 7) = (m68k_dreg (regs, (extra >> 12) & 7) & ~0xffff) | ((src) & 0xffff);
	}
}}}}}}}return 15 * CYCLE_UNIT / 2;
}

/* MOVES.W #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0e78_40)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uae_s16 extra = get_diword (2);
	if (extra & 0x800)
{	uae_u32 src = regs.regs[(extra >> 12) & 15];
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	put_word_jit (dsta,src);
}}else{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (4);
{	uae_s16 src = get_word_jit (srca);
	if (extra & 0x8000) {
	m68k_areg (regs, (extra >> 12) & 7) = (uae_s32)(uae_s16)src;
	} else {
	m68k_dreg (regs, (extra >> 12) & 7) = (m68k_dreg (regs, (extra >> 12) & 7) & ~0xffff) | ((src) & 0xffff);
	}
}}}}}}	m68k_incpc (6);
return 12 * CYCLE_UNIT / 2;
}

/* MOVES.W #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0e79_40)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uae_s16 extra = get_diword (2);
	if (extra & 0x800)
{	uae_u32 src = regs.regs[(extra >> 12) & 15];
{	uaecptr dsta;
	dsta = get_dilong (4);
	put_word_jit (dsta,src);
}}else{{	uaecptr srca;
	srca = get_dilong (4);
{	uae_s16 src = get_word_jit (srca);
	if (extra & 0x8000) {
	m68k_areg (regs, (extra >> 12) & 7) = (uae_s32)(uae_s16)src;
	} else {
	m68k_dreg (regs, (extra >> 12) & 7) = (m68k_dreg (regs, (extra >> 12) & 7) & ~0xffff) | ((src) & 0xffff);
	}
}}}}}}	m68k_incpc (8);
return 13 * CYCLE_UNIT / 2;
}

/* MOVES.L #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0e90_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uae_s16 extra = get_diword (2);
	if (extra & 0x800)
{	uae_u32 src = regs.regs[(extra >> 12) & 15];
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	put_long_jit (dsta,src);
}}else{{	uaecptr srca;
	srca = m68k_areg (regs, dstreg);
{	uae_s32 src = get_long_jit (srca);
	if (extra & 0x8000) {
	m68k_areg (regs, (extra >> 12) & 7) = src;
	} else {
	m68k_dreg (regs, (extra >> 12) & 7) = (src);
	}
}}}}}}	m68k_incpc (4);
return 10 * CYCLE_UNIT / 2;
}

/* MOVES.L #<data>.W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0e98_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uae_s16 extra = get_diword (2);
	if (extra & 0x800)
{	uae_u32 src = regs.regs[(extra >> 12) & 15];
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 4;
	put_long_jit (dsta,src);
}}else{{	uaecptr srca;
	srca = m68k_areg (regs, dstreg);
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, dstreg) += 4;
	if (extra & 0x8000) {
	m68k_areg (regs, (extra >> 12) & 7) = src;
	} else {
	m68k_dreg (regs, (extra >> 12) & 7) = (src);
	}
}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* MOVES.L #<data>.W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0ea0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uae_s16 extra = get_diword (2);
	if (extra & 0x800)
{	uae_u32 src = regs.regs[(extra >> 12) & 15];
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
	m68k_areg (regs, dstreg) = dsta;
	put_long_jit (dsta,src);
}}else{{	uaecptr srca;
	srca = m68k_areg (regs, dstreg) - 4;
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, dstreg) = srca;
	if (extra & 0x8000) {
	m68k_areg (regs, (extra >> 12) & 7) = src;
	} else {
	m68k_dreg (regs, (extra >> 12) & 7) = (src);
	}
}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* MOVES.L #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0ea8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uae_s16 extra = get_diword (2);
	if (extra & 0x800)
{	uae_u32 src = regs.regs[(extra >> 12) & 15];
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	put_long_jit (dsta,src);
}}else{{	uaecptr srca;
	srca = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_s32 src = get_long_jit (srca);
	if (extra & 0x8000) {
	m68k_areg (regs, (extra >> 12) & 7) = src;
	} else {
	m68k_dreg (regs, (extra >> 12) & 7) = (src);
	}
}}}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* MOVES.L #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0eb0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uae_s16 extra = get_diword (2);
	if (extra & 0x800)
{	uae_u32 src = regs.regs[(extra >> 12) & 15];
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
	put_long_jit (dsta,src);
}}}else{{	uaecptr srca;
	m68k_incpc (4);
{	srca = get_disp_ea_020 (m68k_areg (regs, dstreg), 1);
{	uae_s32 src = get_long_jit (srca);
	if (extra & 0x8000) {
	m68k_areg (regs, (extra >> 12) & 7) = src;
	} else {
	m68k_dreg (regs, (extra >> 12) & 7) = (src);
	}
}}}}}}}return 15 * CYCLE_UNIT / 2;
}

/* MOVES.L #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0eb8_40)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uae_s16 extra = get_diword (2);
	if (extra & 0x800)
{	uae_u32 src = regs.regs[(extra >> 12) & 15];
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	put_long_jit (dsta,src);
}}else{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (4);
{	uae_s32 src = get_long_jit (srca);
	if (extra & 0x8000) {
	m68k_areg (regs, (extra >> 12) & 7) = src;
	} else {
	m68k_dreg (regs, (extra >> 12) & 7) = (src);
	}
}}}}}}	m68k_incpc (6);
return 12 * CYCLE_UNIT / 2;
}

/* MOVES.L #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0eb9_40)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uae_s16 extra = get_diword (2);
	if (extra & 0x800)
{	uae_u32 src = regs.regs[(extra >> 12) & 15];
{	uaecptr dsta;
	dsta = get_dilong (4);
	put_long_jit (dsta,src);
}}else{{	uaecptr srca;
	srca = get_dilong (4);
{	uae_s32 src = get_long_jit (srca);
	if (extra & 0x8000) {
	m68k_areg (regs, (extra >> 12) & 7) = src;
	} else {
	m68k_dreg (regs, (extra >> 12) & 7) = (src);
	}
}}}}}}	m68k_incpc (8);
return 13 * CYCLE_UNIT / 2;
}

/* CAS.L #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0ed0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long_jit (dsta);
{	int ru = (src >> 6) & 7;
	int rc = src & 7;
{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(m68k_dreg (regs, rc)));
{	int flgs = ((uae_s32)(m68k_dreg (regs, rc))) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(m68k_dreg (regs, rc))) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
	if (GET_ZFLG ()){
		put_long_jit (dsta, (m68k_dreg (regs, ru)));
	}else{
		put_long_jit (dsta, dst);
		m68k_dreg(regs, rc) = dst;
}}}}}}}}m68k_incpc (4);
	return 16 * CYCLE_UNIT / 2;
}

/* CAS.L #<data>.W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_0ed8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long_jit (dsta);
	m68k_areg (regs, dstreg) += 4;
{	int ru = (src >> 6) & 7;
	int rc = src & 7;
{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(m68k_dreg (regs, rc)));
{	int flgs = ((uae_s32)(m68k_dreg (regs, rc))) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(m68k_dreg (regs, rc))) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
	if (GET_ZFLG ()){
		put_long_jit (dsta, (m68k_dreg (regs, ru)));
	}else{
		put_long_jit (dsta, dst);
		m68k_dreg(regs, rc) = dst;
}}}}}}}}m68k_incpc (4);
	return 18 * CYCLE_UNIT / 2;
}

/* CAS.L #<data>.W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_0ee0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
{	uae_s32 dst = get_long_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
{	int ru = (src >> 6) & 7;
	int rc = src & 7;
{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(m68k_dreg (regs, rc)));
{	int flgs = ((uae_s32)(m68k_dreg (regs, rc))) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(m68k_dreg (regs, rc))) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
	if (GET_ZFLG ()){
		put_long_jit (dsta, (m68k_dreg (regs, ru)));
	}else{
		put_long_jit (dsta, dst);
		m68k_dreg(regs, rc) = dst;
}}}}}}}}m68k_incpc (4);
	return 19 * CYCLE_UNIT / 2;
}

/* CAS.L #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_0ee8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_s32 dst = get_long_jit (dsta);
{	int ru = (src >> 6) & 7;
	int rc = src & 7;
{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(m68k_dreg (regs, rc)));
{	int flgs = ((uae_s32)(m68k_dreg (regs, rc))) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(m68k_dreg (regs, rc))) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
	if (GET_ZFLG ()){
		put_long_jit (dsta, (m68k_dreg (regs, ru)));
	}else{
		put_long_jit (dsta, dst);
		m68k_dreg(regs, rc) = dst;
}}}}}}}}m68k_incpc (6);
	return 26 * CYCLE_UNIT / 2;
}

/* CAS.L #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_0ef0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s32 dst = get_long_jit (dsta);
{	int ru = (src >> 6) & 7;
	int rc = src & 7;
{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(m68k_dreg (regs, rc)));
{	int flgs = ((uae_s32)(m68k_dreg (regs, rc))) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(m68k_dreg (regs, rc))) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
	if (GET_ZFLG ()){
		put_long_jit (dsta, (m68k_dreg (regs, ru)));
	}else{
		put_long_jit (dsta, dst);
		m68k_dreg(regs, rc) = dst;
}}}}}}}}}return 21 * CYCLE_UNIT / 2;
}

/* CAS.L #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_0ef8_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_s32 dst = get_long_jit (dsta);
{	int ru = (src >> 6) & 7;
	int rc = src & 7;
{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(m68k_dreg (regs, rc)));
{	int flgs = ((uae_s32)(m68k_dreg (regs, rc))) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(m68k_dreg (regs, rc))) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
	if (GET_ZFLG ()){
		put_long_jit (dsta, (m68k_dreg (regs, ru)));
	}else{
		put_long_jit (dsta, dst);
		m68k_dreg(regs, rc) = dst;
}}}}}}}}m68k_incpc (6);
	return 18 * CYCLE_UNIT / 2;
}

/* CAS.L #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_0ef9_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_s32 dst = get_long_jit (dsta);
{	int ru = (src >> 6) & 7;
	int rc = src & 7;
{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(m68k_dreg (regs, rc)));
{	int flgs = ((uae_s32)(m68k_dreg (regs, rc))) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(m68k_dreg (regs, rc))) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
	if (GET_ZFLG ()){
		put_long_jit (dsta, (m68k_dreg (regs, ru)));
	}else{
		put_long_jit (dsta, dst);
		m68k_dreg(regs, rc) = dst;
}}}}}}}}m68k_incpc (8);
	return 19 * CYCLE_UNIT / 2;
}

/* CAS2.L #<data>.L */
uae_u32 REGPARAM2 CPUFUNC(op_0efc_40)(uae_u32 opcode)
{
{{	uae_s32 extra;
	extra = get_dilong (2);
	uae_u32 rn1 = regs.regs[(extra >> 28) & 15];
	uae_u32 rn2 = regs.regs[(extra >> 12) & 15];
	uae_u32 dst1 = get_long_jit (rn1), dst2 = get_long_jit (rn2);
{uae_u32 newv = ((uae_u32)(dst1)) - ((uae_u32)(m68k_dreg (regs, (extra >> 16) & 7)));
{	int flgs = ((uae_s32)(m68k_dreg (regs, (extra >> 16) & 7))) < 0;
	int flgo = ((uae_s32)(dst1)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(m68k_dreg (regs, (extra >> 16) & 7))) > ((uae_u32)(dst1)));
	SET_NFLG (flgn != 0);
	if (GET_ZFLG ()) {
{uae_u32 newv = ((uae_u32)(dst2)) - ((uae_u32)(m68k_dreg (regs, extra & 7)));
{	int flgs = ((uae_s32)(m68k_dreg (regs, extra & 7))) < 0;
	int flgo = ((uae_s32)(dst2)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(m68k_dreg (regs, extra & 7))) > ((uae_u32)(dst2)));
	SET_NFLG (flgn != 0);
	if (GET_ZFLG ()) {
	put_long_jit (rn1, m68k_dreg (regs, (extra >> 22) & 7));
	put_long_jit (rn2, m68k_dreg (regs, (extra >> 6) & 7));
	}}
}}}}	if (! GET_ZFLG ()) {
	m68k_dreg (regs, (extra >> 0) & 7) = dst2;
	m68k_dreg (regs, (extra >> 16) & 7) = dst1;
	}
}}	m68k_incpc (6);
return 25 * CYCLE_UNIT / 2;
}

/* MOVE.B Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_1000_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc (2);
}}}return 3 * CYCLE_UNIT / 2;
}

/* MOVE.B (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_1010_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc (2);
}}}}return 7 * CYCLE_UNIT / 2;
}

/* MOVE.B (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_1018_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc (2);
}}}}return 7 * CYCLE_UNIT / 2;
}

/* MOVE.B -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_1020_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc (2);
}}}}return 8 * CYCLE_UNIT / 2;
}

/* MOVE.B (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_1028_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc (4);
}}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.B (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_1030_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s8 src = get_byte_jit (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.B (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_1038_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc (4);
}}}}return 8 * CYCLE_UNIT / 2;
}

/* MOVE.B (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_1039_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte_jit (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc (6);
}}}}return 10 * CYCLE_UNIT / 2;
}

/* MOVE.B (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_103a_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc (4);
}}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.B (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_103b_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s8 src = get_byte_jit (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.B #<data>.B,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_103c_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = get_dibyte (2);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc (4);
}}}return 4 * CYCLE_UNIT / 2;
}

/* MOVE.B Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_1080_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (2);
}}}return 5 * CYCLE_UNIT / 2;
}

/* MOVE.B (An),(An) */
uae_u32 REGPARAM2 CPUFUNC(op_1090_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (2);
}}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.B (An)+,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_1098_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (2);
}}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.B -(An),(An) */
uae_u32 REGPARAM2 CPUFUNC(op_10a0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (2);
}}}}return 10 * CYCLE_UNIT / 2;
}

/* MOVE.B (d16,An),(An) */
uae_u32 REGPARAM2 CPUFUNC(op_10a8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (4);
}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.B (d8,An,Xn),(An) */
uae_u32 REGPARAM2 CPUFUNC(op_10b0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.B (xxx).W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_10b8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (4);
}}}}return 10 * CYCLE_UNIT / 2;
}

/* MOVE.B (xxx).L,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_10b9_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (6);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.B (d16,PC),(An) */
uae_u32 REGPARAM2 CPUFUNC(op_10ba_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (4);
}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.B (d8,PC,Xn),(An) */
uae_u32 REGPARAM2 CPUFUNC(op_10bb_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.B #<data>.B,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_10bc_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (4);
}}}return 6 * CYCLE_UNIT / 2;
}

/* MOVE.B Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_10c0_40)(uae_u32 opcode)
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
	put_byte_jit (dsta,src);
	m68k_incpc (2);
}}}return 5 * CYCLE_UNIT / 2;
}

/* MOVE.B (An),(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_10d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (2);
}}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.B (An)+,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_10d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (2);
}}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.B -(An),(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_10e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (2);
}}}}return 10 * CYCLE_UNIT / 2;
}

/* MOVE.B (d16,An),(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_10e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (4);
}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.B (d8,An,Xn),(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_10f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.B (xxx).W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_10f8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (4);
}}}}return 10 * CYCLE_UNIT / 2;
}

/* MOVE.B (xxx).L,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_10f9_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (6);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.B (d16,PC),(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_10fa_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (4);
}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.B (d8,PC,Xn),(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_10fb_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.B #<data>.B,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_10fc_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (4);
}}}return 8 * CYCLE_UNIT / 2;
}

/* MOVE.B Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_1100_40)(uae_u32 opcode)
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
	put_byte_jit (dsta,src);
	m68k_incpc (2);
}}}return 6 * CYCLE_UNIT / 2;
}

/* MOVE.B (An),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_1110_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (2);
}}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.B (An)+,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_1118_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (2);
}}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.B -(An),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_1120_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (2);
}}}}return 10 * CYCLE_UNIT / 2;
}

/* MOVE.B (d16,An),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_1128_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (4);
}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.B (d8,An,Xn),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_1130_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.B (xxx).W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_1138_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (4);
}}}}return 10 * CYCLE_UNIT / 2;
}

/* MOVE.B (xxx).L,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_1139_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (6);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.B (d16,PC),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_113a_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (4);
}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.B (d8,PC,Xn),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_113b_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.B #<data>.B,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_113c_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (4);
}}}return 7 * CYCLE_UNIT / 2;
}

/* MOVE.B Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_1140_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (4);
}}}return 7 * CYCLE_UNIT / 2;
}

/* MOVE.B (An),(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_1150_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (4);
}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.B (An)+,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_1158_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (4);
}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.B -(An),(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_1160_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (4);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.B (d16,An),(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_1168_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (6);
}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.B (d8,An,Xn),(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_1170_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (2);
}}}}}return 15 * CYCLE_UNIT / 2;
}

/* MOVE.B (xxx).W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_1178_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (6);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.B (xxx).L,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_1179_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (6);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (8);
}}}}return 14 * CYCLE_UNIT / 2;
}

/* MOVE.B (d16,PC),(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_117a_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (6);
}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.B (d8,PC,Xn),(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_117b_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (2);
}}}}}return 15 * CYCLE_UNIT / 2;
}

/* MOVE.B #<data>.B,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_117c_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (6);
}}}return 7 * CYCLE_UNIT / 2;
}

/* MOVE.B Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_1180_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	m68k_incpc (2);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.B (An),(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_1190_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	m68k_incpc (2);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.B (An)+,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_1198_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{	uaecptr dsta;
	m68k_incpc (2);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.B -(An),(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_11a0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	m68k_incpc (2);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.B (d16,An),(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_11a8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.B (d8,An,Xn),(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_11b0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 1);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}}}return 15 * CYCLE_UNIT / 2;
}

/* MOVE.B (xxx).W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_11b8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.B (xxx).L,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_11b9_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	m68k_incpc (6);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}}return 14 * CYCLE_UNIT / 2;
}

/* MOVE.B (d16,PC),(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_11ba_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.B (d8,PC,Xn),(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_11bb_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 1);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}}}return 15 * CYCLE_UNIT / 2;
}

/* MOVE.B #<data>.B,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_11bc_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.B Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_11c0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (4);
}}}return 7 * CYCLE_UNIT / 2;
}

/* MOVE.B (An),(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_11d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (4);
}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.B (An)+,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_11d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (4);
}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.B -(An),(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_11e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (4);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.B (d16,An),(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_11e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (6);
}}}}return 8 * CYCLE_UNIT / 2;
}

/* MOVE.B (d8,An,Xn),(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_11f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (2);
}}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.B (xxx).W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_11f8_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (6);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.B (xxx).L,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_11f9_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (6);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (8);
}}}}return 14 * CYCLE_UNIT / 2;
}

/* MOVE.B (d16,PC),(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_11fa_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (6);
}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.B (d8,PC,Xn),(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_11fb_40)(uae_u32 opcode)
{
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (2);
}}}}}return 15 * CYCLE_UNIT / 2;
}

/* MOVE.B #<data>.B,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_11fc_40)(uae_u32 opcode)
{
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (6);
}}}return 7 * CYCLE_UNIT / 2;
}

/* MOVE.B Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_13c0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (6);
}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.B (An),(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_13d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = get_dilong (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (6);
}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.B (An)+,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_13d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{	uaecptr dsta;
	dsta = get_dilong (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (6);
}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.B -(An),(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_13e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = get_dilong (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (6);
}}}}return 14 * CYCLE_UNIT / 2;
}

/* MOVE.B (d16,An),(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_13e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = get_dilong (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (8);
}}}}return 15 * CYCLE_UNIT / 2;
}

/* MOVE.B (d8,An,Xn),(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_13f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = get_dilong (0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (4);
}}}}}return 17 * CYCLE_UNIT / 2;
}

/* MOVE.B (xxx).W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_13f8_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = get_dilong (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (8);
}}}}return 14 * CYCLE_UNIT / 2;
}

/* MOVE.B (xxx).L,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_13f9_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = get_dilong (6);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (10);
}}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.B (d16,PC),(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_13fa_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = get_dilong (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (8);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.B (d8,PC,Xn),(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_13fb_40)(uae_u32 opcode)
{
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s8 src = get_byte_jit (srca);
{	uaecptr dsta;
	dsta = get_dilong (0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (4);
}}}}}return 17 * CYCLE_UNIT / 2;
}

/* MOVE.B #<data>.B,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_13fc_40)(uae_u32 opcode)
{
{{	uae_s8 src = get_dibyte (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
	m68k_incpc (8);
}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_2000_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
	m68k_incpc (2);
}}}return 3 * CYCLE_UNIT / 2;
}

/* MOVE.L An,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_2008_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_areg (regs, srcreg);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
	m68k_incpc (2);
}}}return 3 * CYCLE_UNIT / 2;
}

/* MOVE.L (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_2010_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
	m68k_incpc (2);
}}}}return 7 * CYCLE_UNIT / 2;
}

/* MOVE.L (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_2018_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) += 4;
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
	m68k_incpc (2);
}}}}return 7 * CYCLE_UNIT / 2;
}

/* MOVE.L -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_2020_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
	m68k_incpc (2);
}}}}return 8 * CYCLE_UNIT / 2;
}

/* MOVE.L (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_2028_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
	m68k_incpc (4);
}}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.L (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_2030_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s32 src = get_long_jit (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.L (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_2038_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
	m68k_incpc (4);
}}}}return 8 * CYCLE_UNIT / 2;
}

/* MOVE.L (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_2039_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long_jit (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
	m68k_incpc (6);
}}}}return 10 * CYCLE_UNIT / 2;
}

/* MOVE.L (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_203a_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
	m68k_incpc (4);
}}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.L (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_203b_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s32 src = get_long_jit (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.L #<data>.L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_203c_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
	m68k_incpc (6);
}}}return 6 * CYCLE_UNIT / 2;
}

/* MOVEA.L Dn,An */
uae_u32 REGPARAM2 CPUFUNC(op_2040_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	m68k_areg (regs, dstreg) = (src);
	m68k_incpc (2);
}}}return 3 * CYCLE_UNIT / 2;
}

/* MOVEA.L An,An */
uae_u32 REGPARAM2 CPUFUNC(op_2048_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_areg (regs, srcreg);
{	m68k_areg (regs, dstreg) = (src);
	m68k_incpc (2);
}}}return 3 * CYCLE_UNIT / 2;
}

/* MOVEA.L (An),An */
uae_u32 REGPARAM2 CPUFUNC(op_2050_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
{	m68k_areg (regs, dstreg) = (src);
	m68k_incpc (2);
}}}}return 7 * CYCLE_UNIT / 2;
}

/* MOVEA.L (An)+,An */
uae_u32 REGPARAM2 CPUFUNC(op_2058_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) += 4;
{	m68k_areg (regs, dstreg) = (src);
	m68k_incpc (2);
}}}}return 7 * CYCLE_UNIT / 2;
}

/* MOVEA.L -(An),An */
uae_u32 REGPARAM2 CPUFUNC(op_2060_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	m68k_areg (regs, dstreg) = (src);
	m68k_incpc (2);
}}}}return 8 * CYCLE_UNIT / 2;
}

/* MOVEA.L (d16,An),An */
uae_u32 REGPARAM2 CPUFUNC(op_2068_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	m68k_areg (regs, dstreg) = (src);
	m68k_incpc (4);
}}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVEA.L (d8,An,Xn),An */
uae_u32 REGPARAM2 CPUFUNC(op_2070_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s32 src = get_long_jit (srca);
{	m68k_areg (regs, dstreg) = (src);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVEA.L (xxx).W,An */
uae_u32 REGPARAM2 CPUFUNC(op_2078_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	m68k_areg (regs, dstreg) = (src);
	m68k_incpc (4);
}}}}return 8 * CYCLE_UNIT / 2;
}

/* MOVEA.L (xxx).L,An */
uae_u32 REGPARAM2 CPUFUNC(op_2079_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long_jit (srca);
{	m68k_areg (regs, dstreg) = (src);
	m68k_incpc (6);
}}}}return 10 * CYCLE_UNIT / 2;
}

/* MOVEA.L (d16,PC),An */
uae_u32 REGPARAM2 CPUFUNC(op_207a_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	m68k_areg (regs, dstreg) = (src);
	m68k_incpc (4);
}}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVEA.L (d8,PC,Xn),An */
uae_u32 REGPARAM2 CPUFUNC(op_207b_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s32 src = get_long_jit (srca);
{	m68k_areg (regs, dstreg) = (src);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVEA.L #<data>.L,An */
uae_u32 REGPARAM2 CPUFUNC(op_207c_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	m68k_areg (regs, dstreg) = (src);
	m68k_incpc (6);
}}}return 6 * CYCLE_UNIT / 2;
}

/* MOVE.L Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_2080_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (2);
}}}return 5 * CYCLE_UNIT / 2;
}

/* MOVE.L An,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_2088_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_areg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (2);
}}}return 5 * CYCLE_UNIT / 2;
}

/* MOVE.L (An),(An) */
uae_u32 REGPARAM2 CPUFUNC(op_2090_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (2);
}}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.L (An)+,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_2098_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) += 4;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (2);
}}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.L -(An),(An) */
uae_u32 REGPARAM2 CPUFUNC(op_20a0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (2);
}}}}return 10 * CYCLE_UNIT / 2;
}

/* MOVE.L (d16,An),(An) */
uae_u32 REGPARAM2 CPUFUNC(op_20a8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (4);
}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.L (d8,An,Xn),(An) */
uae_u32 REGPARAM2 CPUFUNC(op_20b0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.L (xxx).W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_20b8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (4);
}}}}return 10 * CYCLE_UNIT / 2;
}

/* MOVE.L (xxx).L,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_20b9_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (6);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.L (d16,PC),(An) */
uae_u32 REGPARAM2 CPUFUNC(op_20ba_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (4);
}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.L (d8,PC,Xn),(An) */
uae_u32 REGPARAM2 CPUFUNC(op_20bb_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.L #<data>.L,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_20bc_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (6);
}}}return 8 * CYCLE_UNIT / 2;
}

/* MOVE.L Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_20c0_40)(uae_u32 opcode)
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
	put_long_jit (dsta,src);
	m68k_incpc (2);
}}}return 5 * CYCLE_UNIT / 2;
}

/* MOVE.L An,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_20c8_40)(uae_u32 opcode)
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
	put_long_jit (dsta,src);
	m68k_incpc (2);
}}}return 5 * CYCLE_UNIT / 2;
}

/* MOVE.L (An),(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_20d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 4;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (2);
}}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.L (An)+,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_20d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) += 4;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 4;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (2);
}}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.L -(An),(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_20e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 4;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (2);
}}}}return 10 * CYCLE_UNIT / 2;
}

/* MOVE.L (d16,An),(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_20e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 4;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (4);
}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.L (d8,An,Xn),(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_20f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 4;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.L (xxx).W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_20f8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 4;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (4);
}}}}return 10 * CYCLE_UNIT / 2;
}

/* MOVE.L (xxx).L,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_20f9_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 4;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (6);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.L (d16,PC),(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_20fa_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 4;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (4);
}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.L (d8,PC,Xn),(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_20fb_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 4;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.L #<data>.L,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_20fc_40)(uae_u32 opcode)
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
	put_long_jit (dsta,src);
	m68k_incpc (6);
}}}return 8 * CYCLE_UNIT / 2;
}

/* MOVE.L Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_2100_40)(uae_u32 opcode)
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
	put_long_jit (dsta,src);
	m68k_incpc (2);
}}}return 6 * CYCLE_UNIT / 2;
}

/* MOVE.L An,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_2108_40)(uae_u32 opcode)
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
	put_long_jit (dsta,src);
	m68k_incpc (2);
}}}return 6 * CYCLE_UNIT / 2;
}

/* MOVE.L (An),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_2110_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (2);
}}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.L (An)+,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_2118_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) += 4;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (2);
}}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.L -(An),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_2120_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (2);
}}}}return 10 * CYCLE_UNIT / 2;
}

/* MOVE.L (d16,An),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_2128_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (4);
}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.L (d8,An,Xn),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_2130_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.L (xxx).W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_2138_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (4);
}}}}return 10 * CYCLE_UNIT / 2;
}

/* MOVE.L (xxx).L,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_2139_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (6);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.L (d16,PC),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_213a_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (4);
}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.L (d8,PC,Xn),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_213b_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.L #<data>.L,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_213c_40)(uae_u32 opcode)
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
	put_long_jit (dsta,src);
	m68k_incpc (6);
}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.L Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_2140_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (4);
}}}return 7 * CYCLE_UNIT / 2;
}

/* MOVE.L An,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_2148_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_areg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (4);
}}}return 7 * CYCLE_UNIT / 2;
}

/* MOVE.L (An),(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_2150_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (4);
}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.L (An)+,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_2158_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) += 4;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (4);
}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.L -(An),(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_2160_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (4);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.L (d16,An),(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_2168_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (6);
}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.L (d8,An,Xn),(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_2170_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (2);
}}}}}return 15 * CYCLE_UNIT / 2;
}

/* MOVE.L (xxx).W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_2178_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (6);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.L (xxx).L,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_2179_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (6);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (8);
}}}}return 14 * CYCLE_UNIT / 2;
}

/* MOVE.L (d16,PC),(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_217a_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (6);
}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.L (d8,PC,Xn),(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_217b_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (2);
}}}}}return 15 * CYCLE_UNIT / 2;
}

/* MOVE.L #<data>.L,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_217c_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (6);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (8);
}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.L Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_2180_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	m68k_incpc (2);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.L An,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_2188_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_areg (regs, srcreg);
{	uaecptr dsta;
	m68k_incpc (2);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.L (An),(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_2190_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	m68k_incpc (2);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.L (An)+,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_2198_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) += 4;
{	uaecptr dsta;
	m68k_incpc (2);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}}return 11 * CYCLE_UNIT / 2;
}

#endif

#ifdef PART_3
/* MOVE.L -(An),(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_21a0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	m68k_incpc (2);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.L (d16,An),(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_21a8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.L (d8,An,Xn),(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_21b0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 1);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}}}return 15 * CYCLE_UNIT / 2;
}

/* MOVE.L (xxx).W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_21b8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.L (xxx).L,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_21b9_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	m68k_incpc (6);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}}return 14 * CYCLE_UNIT / 2;
}

/* MOVE.L (d16,PC),(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_21ba_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.L (d8,PC,Xn),(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_21bb_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 1);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}}}return 15 * CYCLE_UNIT / 2;
}

/* MOVE.L #<data>.L,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_21bc_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	m68k_incpc (6);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.L Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_21c0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (4);
}}}return 7 * CYCLE_UNIT / 2;
}

/* MOVE.L An,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_21c8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = m68k_areg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (4);
}}}return 7 * CYCLE_UNIT / 2;
}

/* MOVE.L (An),(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_21d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (4);
}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.L (An)+,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_21d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) += 4;
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (4);
}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.L -(An),(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_21e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (4);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.L (d16,An),(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_21e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (6);
}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.L (d8,An,Xn),(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_21f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (2);
}}}}}return 15 * CYCLE_UNIT / 2;
}

/* MOVE.L (xxx).W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_21f8_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (6);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.L (xxx).L,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_21f9_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (6);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (8);
}}}}return 14 * CYCLE_UNIT / 2;
}

/* MOVE.L (d16,PC),(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_21fa_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (6);
}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.L (d8,PC,Xn),(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_21fb_40)(uae_u32 opcode)
{
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (2);
}}}}}return 15 * CYCLE_UNIT / 2;
}

/* MOVE.L #<data>.L,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_21fc_40)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (6);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (8);
}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.L Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_23c0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (6);
}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.L An,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_23c8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = m68k_areg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (6);
}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.L (An),(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_23d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = get_dilong (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (6);
}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.L (An)+,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_23d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) += 4;
{	uaecptr dsta;
	dsta = get_dilong (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (6);
}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.L -(An),(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_23e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = get_dilong (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (6);
}}}}return 14 * CYCLE_UNIT / 2;
}

/* MOVE.L (d16,An),(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_23e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = get_dilong (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (8);
}}}}return 15 * CYCLE_UNIT / 2;
}

/* MOVE.L (d8,An,Xn),(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_23f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = get_dilong (0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (4);
}}}}}return 17 * CYCLE_UNIT / 2;
}

/* MOVE.L (xxx).W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_23f8_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = get_dilong (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (8);
}}}}return 14 * CYCLE_UNIT / 2;
}

/* MOVE.L (xxx).L,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_23f9_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = get_dilong (6);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (10);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.L (d16,PC),(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_23fa_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = get_dilong (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (8);
}}}}return 15 * CYCLE_UNIT / 2;
}

/* MOVE.L (d8,PC,Xn),(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_23fb_40)(uae_u32 opcode)
{
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s32 src = get_long_jit (srca);
{	uaecptr dsta;
	dsta = get_dilong (0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (4);
}}}}}return 17 * CYCLE_UNIT / 2;
}

/* MOVE.L #<data>.L,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_23fc_40)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
{	uaecptr dsta;
	dsta = get_dilong (6);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
	m68k_incpc (10);
}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_3000_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc (2);
}}}return 3 * CYCLE_UNIT / 2;
}

/* MOVE.W An,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_3008_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_areg (regs, srcreg);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc (2);
}}}return 3 * CYCLE_UNIT / 2;
}

/* MOVE.W (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_3010_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc (2);
}}}}return 7 * CYCLE_UNIT / 2;
}

/* MOVE.W (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_3018_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) += 2;
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc (2);
}}}}return 7 * CYCLE_UNIT / 2;
}

/* MOVE.W -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_3020_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc (2);
}}}}return 8 * CYCLE_UNIT / 2;
}

/* MOVE.W (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_3028_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc (4);
}}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.W (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_3030_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s16 src = get_word_jit (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.W (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_3038_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc (4);
}}}}return 8 * CYCLE_UNIT / 2;
}

/* MOVE.W (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_3039_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word_jit (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc (6);
}}}}return 10 * CYCLE_UNIT / 2;
}

/* MOVE.W (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_303a_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc (4);
}}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.W (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_303b_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s16 src = get_word_jit (srca);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.W #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_303c_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_diword (2);
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc (4);
}}}return 4 * CYCLE_UNIT / 2;
}

/* MOVEA.W Dn,An */
uae_u32 REGPARAM2 CPUFUNC(op_3040_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	src = (uae_s32)(uae_s16)src;
	m68k_areg (regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc (2);
}}}return 3 * CYCLE_UNIT / 2;
}

/* MOVEA.W An,An */
uae_u32 REGPARAM2 CPUFUNC(op_3048_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_areg (regs, srcreg);
{	src = (uae_s32)(uae_s16)src;
	m68k_areg (regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc (2);
}}}return 3 * CYCLE_UNIT / 2;
}

/* MOVEA.W (An),An */
uae_u32 REGPARAM2 CPUFUNC(op_3050_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
{	src = (uae_s32)(uae_s16)src;
	m68k_areg (regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc (2);
}}}}return 7 * CYCLE_UNIT / 2;
}

/* MOVEA.W (An)+,An */
uae_u32 REGPARAM2 CPUFUNC(op_3058_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) += 2;
{	src = (uae_s32)(uae_s16)src;
	m68k_areg (regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc (2);
}}}}return 7 * CYCLE_UNIT / 2;
}

/* MOVEA.W -(An),An */
uae_u32 REGPARAM2 CPUFUNC(op_3060_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	src = (uae_s32)(uae_s16)src;
	m68k_areg (regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc (2);
}}}}return 8 * CYCLE_UNIT / 2;
}

/* MOVEA.W (d16,An),An */
uae_u32 REGPARAM2 CPUFUNC(op_3068_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	src = (uae_s32)(uae_s16)src;
	m68k_areg (regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc (4);
}}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVEA.W (d8,An,Xn),An */
uae_u32 REGPARAM2 CPUFUNC(op_3070_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s16 src = get_word_jit (srca);
{	src = (uae_s32)(uae_s16)src;
	m68k_areg (regs, dstreg) = (uae_s32)(uae_s16)(src);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVEA.W (xxx).W,An */
uae_u32 REGPARAM2 CPUFUNC(op_3078_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	src = (uae_s32)(uae_s16)src;
	m68k_areg (regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc (4);
}}}}return 8 * CYCLE_UNIT / 2;
}

/* MOVEA.W (xxx).L,An */
uae_u32 REGPARAM2 CPUFUNC(op_3079_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word_jit (srca);
{	src = (uae_s32)(uae_s16)src;
	m68k_areg (regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc (6);
}}}}return 10 * CYCLE_UNIT / 2;
}

/* MOVEA.W (d16,PC),An */
uae_u32 REGPARAM2 CPUFUNC(op_307a_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	src = (uae_s32)(uae_s16)src;
	m68k_areg (regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc (4);
}}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVEA.W (d8,PC,Xn),An */
uae_u32 REGPARAM2 CPUFUNC(op_307b_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s16 src = get_word_jit (srca);
{	src = (uae_s32)(uae_s16)src;
	m68k_areg (regs, dstreg) = (uae_s32)(uae_s16)(src);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVEA.W #<data>.W,An */
uae_u32 REGPARAM2 CPUFUNC(op_307c_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_diword (2);
{	src = (uae_s32)(uae_s16)src;
	m68k_areg (regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc (4);
}}}return 4 * CYCLE_UNIT / 2;
}

/* MOVE.W Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_3080_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (2);
}}}return 5 * CYCLE_UNIT / 2;
}

/* MOVE.W An,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_3088_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_areg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (2);
}}}return 5 * CYCLE_UNIT / 2;
}

/* MOVE.W (An),(An) */
uae_u32 REGPARAM2 CPUFUNC(op_3090_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (2);
}}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.W (An)+,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_3098_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) += 2;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (2);
}}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.W -(An),(An) */
uae_u32 REGPARAM2 CPUFUNC(op_30a0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (2);
}}}}return 10 * CYCLE_UNIT / 2;
}

/* MOVE.W (d16,An),(An) */
uae_u32 REGPARAM2 CPUFUNC(op_30a8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (4);
}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.W (d8,An,Xn),(An) */
uae_u32 REGPARAM2 CPUFUNC(op_30b0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.W (xxx).W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_30b8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (4);
}}}}return 10 * CYCLE_UNIT / 2;
}

/* MOVE.W (xxx).L,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_30b9_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (6);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.W (d16,PC),(An) */
uae_u32 REGPARAM2 CPUFUNC(op_30ba_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (4);
}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.W (d8,PC,Xn),(An) */
uae_u32 REGPARAM2 CPUFUNC(op_30bb_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.W #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_30bc_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (4);
}}}return 6 * CYCLE_UNIT / 2;
}

/* MOVE.W Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_30c0_40)(uae_u32 opcode)
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
	put_word_jit (dsta,src);
	m68k_incpc (2);
}}}return 5 * CYCLE_UNIT / 2;
}

/* MOVE.W An,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_30c8_40)(uae_u32 opcode)
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
	put_word_jit (dsta,src);
	m68k_incpc (2);
}}}return 5 * CYCLE_UNIT / 2;
}

/* MOVE.W (An),(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_30d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 2;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (2);
}}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.W (An)+,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_30d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) += 2;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 2;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (2);
}}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.W -(An),(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_30e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 2;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (2);
}}}}return 10 * CYCLE_UNIT / 2;
}

/* MOVE.W (d16,An),(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_30e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 2;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (4);
}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.W (d8,An,Xn),(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_30f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 2;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.W (xxx).W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_30f8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 2;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (4);
}}}}return 10 * CYCLE_UNIT / 2;
}

/* MOVE.W (xxx).L,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_30f9_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 2;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (6);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.W (d16,PC),(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_30fa_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 2;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (4);
}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.W (d8,PC,Xn),(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_30fb_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 2;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.W #<data>.W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_30fc_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
	m68k_areg (regs, dstreg) += 2;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (4);
}}}return 8 * CYCLE_UNIT / 2;
}

/* MOVE.W Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_3100_40)(uae_u32 opcode)
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
	put_word_jit (dsta,src);
	m68k_incpc (2);
}}}return 6 * CYCLE_UNIT / 2;
}

/* MOVE.W An,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_3108_40)(uae_u32 opcode)
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
	put_word_jit (dsta,src);
	m68k_incpc (2);
}}}return 6 * CYCLE_UNIT / 2;
}

/* MOVE.W (An),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_3110_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (2);
}}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.W (An)+,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_3118_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) += 2;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (2);
}}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.W -(An),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_3120_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (2);
}}}}return 10 * CYCLE_UNIT / 2;
}

/* MOVE.W (d16,An),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_3128_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (4);
}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.W (d8,An,Xn),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_3130_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.W (xxx).W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_3138_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (4);
}}}}return 10 * CYCLE_UNIT / 2;
}

/* MOVE.W (xxx).L,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_3139_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (6);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.W (d16,PC),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_313a_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (4);
}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.W (d8,PC,Xn),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_313b_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.W #<data>.W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_313c_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
	m68k_areg (regs, dstreg) = dsta;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (4);
}}}return 7 * CYCLE_UNIT / 2;
}

/* MOVE.W Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_3140_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (4);
}}}return 6 * CYCLE_UNIT / 2;
}

/* MOVE.W An,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_3148_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_areg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (4);
}}}return 6 * CYCLE_UNIT / 2;
}

/* MOVE.W (An),(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_3150_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (4);
}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.W (An)+,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_3158_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) += 2;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (4);
}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.W -(An),(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_3160_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (4);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.W (d16,An),(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_3168_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (6);
}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.W (d8,An,Xn),(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_3170_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (2);
}}}}}return 15 * CYCLE_UNIT / 2;
}

/* MOVE.W (xxx).W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_3178_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (6);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.W (xxx).L,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_3179_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (6);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (8);
}}}}return 14 * CYCLE_UNIT / 2;
}

/* MOVE.W (d16,PC),(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_317a_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (6);
}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.W (d8,PC,Xn),(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_317b_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (2);
}}}}}return 15 * CYCLE_UNIT / 2;
}

/* MOVE.W #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_317c_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (6);
}}}return 7 * CYCLE_UNIT / 2;
}

/* MOVE.W Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_3180_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	m68k_incpc (2);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.W An,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_3188_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_areg (regs, srcreg);
{	uaecptr dsta;
	m68k_incpc (2);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.W (An),(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_3190_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	m68k_incpc (2);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.W (An)+,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_3198_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) += 2;
{	uaecptr dsta;
	m68k_incpc (2);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.W -(An),(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_31a0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	m68k_incpc (2);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.W (d16,An),(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_31a8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.W (d8,An,Xn),(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_31b0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 1);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}}}return 15 * CYCLE_UNIT / 2;
}

/* MOVE.W (xxx).W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_31b8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.W (xxx).L,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_31b9_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	m68k_incpc (6);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}}return 14 * CYCLE_UNIT / 2;
}

/* MOVE.W (d16,PC),(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_31ba_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.W (d8,PC,Xn),(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_31bb_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 1);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}}}return 15 * CYCLE_UNIT / 2;
}

/* MOVE.W #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_31bc_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.W Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_31c0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (4);
}}}return 7 * CYCLE_UNIT / 2;
}

/* MOVE.W An,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_31c8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_areg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (4);
}}}return 7 * CYCLE_UNIT / 2;
}

/* MOVE.W (An),(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_31d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (4);
}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.W (An)+,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_31d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) += 2;
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (4);
}}}}return 11 * CYCLE_UNIT / 2;
}

/* MOVE.W -(An),(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_31e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (4);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.W (d16,An),(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_31e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (6);
}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.W (d8,An,Xn),(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_31f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (2);
}}}}}return 15 * CYCLE_UNIT / 2;
}

/* MOVE.W (xxx).W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_31f8_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (6);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE.W (xxx).L,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_31f9_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (6);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (8);
}}}}return 14 * CYCLE_UNIT / 2;
}

/* MOVE.W (d16,PC),(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_31fa_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (6);
}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.W (d8,PC,Xn),(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_31fb_40)(uae_u32 opcode)
{
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (2);
}}}}}return 15 * CYCLE_UNIT / 2;
}

/* MOVE.W #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_31fc_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (6);
}}}return 7 * CYCLE_UNIT / 2;
}

/* MOVE.W Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_33c0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (6);
}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.W An,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_33c8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_areg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (6);
}}}return 9 * CYCLE_UNIT / 2;
}

/* MOVE.W (An),(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_33d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = get_dilong (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (6);
}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.W (An)+,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_33d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) += 2;
{	uaecptr dsta;
	dsta = get_dilong (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (6);
}}}}return 13 * CYCLE_UNIT / 2;
}

/* MOVE.W -(An),(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_33e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = get_dilong (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (6);
}}}}return 14 * CYCLE_UNIT / 2;
}

/* MOVE.W (d16,An),(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_33e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = get_dilong (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (8);
}}}}return 15 * CYCLE_UNIT / 2;
}

/* MOVE.W (d8,An,Xn),(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_33f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = get_dilong (0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (4);
}}}}}return 17 * CYCLE_UNIT / 2;
}

/* MOVE.W (xxx).W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_33f8_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = get_dilong (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (8);
}}}}return 14 * CYCLE_UNIT / 2;
}

/* MOVE.W (xxx).L,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_33f9_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = get_dilong (6);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (10);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MOVE.W (d16,PC),(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_33fa_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = get_dilong (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (8);
}}}}return 15 * CYCLE_UNIT / 2;
}

/* MOVE.W (d8,PC,Xn),(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_33fb_40)(uae_u32 opcode)
{
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s16 src = get_word_jit (srca);
{	uaecptr dsta;
	dsta = get_dilong (0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (4);
}}}}}return 17 * CYCLE_UNIT / 2;
}

/* MOVE.W #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_33fc_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
	m68k_incpc (8);
}}}return 9 * CYCLE_UNIT / 2;
}

/* NEGX.B Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4000_40)(uae_u32 opcode)
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
return 3 * CYCLE_UNIT / 2;
}

/* NEGX.B (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4010_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s8)(newv)) == 0));
	SET_NFLG (((uae_s8)(newv)) < 0);
	put_byte_jit (srca,newv);
}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* NEGX.B (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4018_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
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
	put_byte_jit (srca,newv);
}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* NEGX.B -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_4020_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte_jit (srca);
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
	put_byte_jit (srca,newv);
}}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

/* NEGX.B (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4028_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s8)(newv)) == 0));
	SET_NFLG (((uae_s8)(newv)) < 0);
	put_byte_jit (srca,newv);
}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* NEGX.B (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4030_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s8 src = get_byte_jit (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s8)(newv)) == 0));
	SET_NFLG (((uae_s8)(newv)) < 0);
	put_byte_jit (srca,newv);
}}}}}}return 14 * CYCLE_UNIT / 2;
}

/* NEGX.B (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4038_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s8)(newv)) == 0));
	SET_NFLG (((uae_s8)(newv)) < 0);
	put_byte_jit (srca,newv);
}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* NEGX.B (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4039_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte_jit (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s8)(newv)) == 0));
	SET_NFLG (((uae_s8)(newv)) < 0);
	put_byte_jit (srca,newv);
}}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* NEGX.W Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4040_40)(uae_u32 opcode)
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
return 3 * CYCLE_UNIT / 2;
}

/* NEGX.W (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4050_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s16)(newv)) == 0));
	SET_NFLG (((uae_s16)(newv)) < 0);
	put_word_jit (srca,newv);
}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* NEGX.W (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4058_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
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
	put_word_jit (srca,newv);
}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* NEGX.W -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_4060_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word_jit (srca);
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
	put_word_jit (srca,newv);
}}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

/* NEGX.W (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4068_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s16)(newv)) == 0));
	SET_NFLG (((uae_s16)(newv)) < 0);
	put_word_jit (srca,newv);
}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* NEGX.W (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4070_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s16 src = get_word_jit (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s16)(newv)) == 0));
	SET_NFLG (((uae_s16)(newv)) < 0);
	put_word_jit (srca,newv);
}}}}}}return 14 * CYCLE_UNIT / 2;
}

/* NEGX.W (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4078_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s16)(newv)) == 0));
	SET_NFLG (((uae_s16)(newv)) < 0);
	put_word_jit (srca,newv);
}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* NEGX.W (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4079_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s16)(newv)) == 0));
	SET_NFLG (((uae_s16)(newv)) < 0);
	put_word_jit (srca,newv);
}}}}}	m68k_incpc (6);
return 15 * CYCLE_UNIT / 2;
}

/* NEGX.L Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4080_40)(uae_u32 opcode)
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
return 3 * CYCLE_UNIT / 2;
}

/* NEGX.L (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4090_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s32)(newv)) == 0));
	SET_NFLG (((uae_s32)(newv)) < 0);
	put_long_jit (srca,newv);
}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* NEGX.L (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4098_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
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
	put_long_jit (srca,newv);
}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* NEGX.L -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_40a0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long_jit (srca);
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
	put_long_jit (srca,newv);
}}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

/* NEGX.L (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_40a8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s32)(newv)) == 0));
	SET_NFLG (((uae_s32)(newv)) < 0);
	put_long_jit (srca,newv);
}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* NEGX.L (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_40b0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s32 src = get_long_jit (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s32)(newv)) == 0));
	SET_NFLG (((uae_s32)(newv)) < 0);
	put_long_jit (srca,newv);
}}}}}}return 14 * CYCLE_UNIT / 2;
}

/* NEGX.L (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_40b8_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s32)(newv)) == 0));
	SET_NFLG (((uae_s32)(newv)) < 0);
	put_long_jit (srca,newv);
}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* NEGX.L (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_40b9_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_u32 newv = 0 - src - (GET_XFLG () ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s32)(newv)) == 0));
	SET_NFLG (((uae_s32)(newv)) < 0);
	put_long_jit (srca,newv);
}}}}}	m68k_incpc (6);
return 15 * CYCLE_UNIT / 2;
}

/* MVSR2.W Dn */
uae_u32 REGPARAM2 CPUFUNC(op_40c0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	MakeSR ();
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | ((regs.sr) & 0xffff);
}}}	m68k_incpc (2);
return 5 * CYCLE_UNIT / 2;
}

/* MVSR2.W (An) */
uae_u32 REGPARAM2 CPUFUNC(op_40d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	MakeSR ();
	put_word_jit (srca,regs.sr);
}}}	m68k_incpc (2);
return 9 * CYCLE_UNIT / 2;
}

/* MVSR2.W (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_40d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += 2;
	MakeSR ();
	put_word_jit (srca,regs.sr);
}}}	m68k_incpc (2);
return 9 * CYCLE_UNIT / 2;
}

/* MVSR2.W -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_40e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
	m68k_areg (regs, srcreg) = srca;
	MakeSR ();
	put_word_jit (srca,regs.sr);
}}}	m68k_incpc (2);
return 9 * CYCLE_UNIT / 2;
}

/* MVSR2.W (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_40e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	MakeSR ();
	put_word_jit (srca,regs.sr);
}}}	m68k_incpc (4);
return 10 * CYCLE_UNIT / 2;
}

/* MVSR2.W (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_40f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
	MakeSR ();
	put_word_jit (srca,regs.sr);
}}}}return 12 * CYCLE_UNIT / 2;
}

/* MVSR2.W (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_40f8_40)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	MakeSR ();
	put_word_jit (srca,regs.sr);
}}}	m68k_incpc (4);
return 10 * CYCLE_UNIT / 2;
}

/* MVSR2.W (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_40f9_40)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uaecptr srca;
	srca = get_dilong (2);
	MakeSR ();
	put_word_jit (srca,regs.sr);
}}}	m68k_incpc (6);
return 12 * CYCLE_UNIT / 2;
}

/* CHK.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4100_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	m68k_incpc (2);
	if (dst > src) {
		SET_NFLG (0);
		Exception_cpu(6);
		return 6 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		SET_NFLG (1);
		Exception_cpu(6);
		return 8 * CYCLE_UNIT / 2;
	}
}}}return 8 * CYCLE_UNIT / 2;
}

/* CHK.L (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4110_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	m68k_incpc (2);
	if (dst > src) {
		SET_NFLG (0);
		Exception_cpu(6);
		return 10 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		SET_NFLG (1);
		Exception_cpu(6);
		return 12 * CYCLE_UNIT / 2;
	}
}}}}return 12 * CYCLE_UNIT / 2;
}

/* CHK.L (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4118_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) += 4;
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	m68k_incpc (2);
	if (dst > src) {
		SET_NFLG (0);
		Exception_cpu(6);
		return 10 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		SET_NFLG (1);
		Exception_cpu(6);
		return 12 * CYCLE_UNIT / 2;
	}
}}}}return 12 * CYCLE_UNIT / 2;
}

/* CHK.L -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4120_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	m68k_incpc (2);
	if (dst > src) {
		SET_NFLG (0);
		Exception_cpu(6);
		return 11 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		SET_NFLG (1);
		Exception_cpu(6);
		return 13 * CYCLE_UNIT / 2;
	}
}}}}return 13 * CYCLE_UNIT / 2;
}

/* CHK.L (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4128_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	m68k_incpc (4);
	if (dst > src) {
		SET_NFLG (0);
		Exception_cpu(6);
		return 12 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		SET_NFLG (1);
		Exception_cpu(6);
		return 14 * CYCLE_UNIT / 2;
	}
}}}}return 14 * CYCLE_UNIT / 2;
}

/* CHK.L (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4130_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	if (dst > src) {
		SET_NFLG (0);
		Exception_cpu(6);
		return 14 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		SET_NFLG (1);
		Exception_cpu(6);
		return 16 * CYCLE_UNIT / 2;
	}
}}}}}return 16 * CYCLE_UNIT / 2;
}

/* CHK.L (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4138_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	m68k_incpc (4);
	if (dst > src) {
		SET_NFLG (0);
		Exception_cpu(6);
		return 12 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		SET_NFLG (1);
		Exception_cpu(6);
		return 14 * CYCLE_UNIT / 2;
	}
}}}}return 14 * CYCLE_UNIT / 2;
}

/* CHK.L (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4139_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	m68k_incpc (6);
	if (dst > src) {
		SET_NFLG (0);
		Exception_cpu(6);
		return 13 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		SET_NFLG (1);
		Exception_cpu(6);
		return 15 * CYCLE_UNIT / 2;
	}
}}}}return 15 * CYCLE_UNIT / 2;
}

/* CHK.L (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_413a_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	m68k_incpc (4);
	if (dst > src) {
		SET_NFLG (0);
		Exception_cpu(6);
		return 12 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		SET_NFLG (1);
		Exception_cpu(6);
		return 14 * CYCLE_UNIT / 2;
	}
}}}}return 14 * CYCLE_UNIT / 2;
}

/* CHK.L (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_413b_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	if (dst > src) {
		SET_NFLG (0);
		Exception_cpu(6);
		return 14 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		SET_NFLG (1);
		Exception_cpu(6);
		return 16 * CYCLE_UNIT / 2;
	}
}}}}}return 16 * CYCLE_UNIT / 2;
}

/* CHK.L #<data>.L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_413c_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	m68k_incpc (6);
	if (dst > src) {
		SET_NFLG (0);
		Exception_cpu(6);
		return 11 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		SET_NFLG (1);
		Exception_cpu(6);
		return 13 * CYCLE_UNIT / 2;
	}
}}}return 13 * CYCLE_UNIT / 2;
}

/* CHK.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4180_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	m68k_incpc (2);
	if (dst > src) {
		SET_NFLG (0);
		Exception_cpu(6);
		return 6 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		SET_NFLG (1);
		Exception_cpu(6);
		return 8 * CYCLE_UNIT / 2;
	}
}}}return 8 * CYCLE_UNIT / 2;
}

/* CHK.W (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4190_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	m68k_incpc (2);
	if (dst > src) {
		SET_NFLG (0);
		Exception_cpu(6);
		return 10 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		SET_NFLG (1);
		Exception_cpu(6);
		return 12 * CYCLE_UNIT / 2;
	}
}}}}return 12 * CYCLE_UNIT / 2;
}

/* CHK.W (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4198_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) += 2;
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	m68k_incpc (2);
	if (dst > src) {
		SET_NFLG (0);
		Exception_cpu(6);
		return 10 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		SET_NFLG (1);
		Exception_cpu(6);
		return 12 * CYCLE_UNIT / 2;
	}
}}}}return 12 * CYCLE_UNIT / 2;
}

/* CHK.W -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_41a0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	m68k_incpc (2);
	if (dst > src) {
		SET_NFLG (0);
		Exception_cpu(6);
		return 11 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		SET_NFLG (1);
		Exception_cpu(6);
		return 13 * CYCLE_UNIT / 2;
	}
}}}}return 13 * CYCLE_UNIT / 2;
}

/* CHK.W (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_41a8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	m68k_incpc (4);
	if (dst > src) {
		SET_NFLG (0);
		Exception_cpu(6);
		return 12 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		SET_NFLG (1);
		Exception_cpu(6);
		return 14 * CYCLE_UNIT / 2;
	}
}}}}return 14 * CYCLE_UNIT / 2;
}

/* CHK.W (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_41b0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	if (dst > src) {
		SET_NFLG (0);
		Exception_cpu(6);
		return 14 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		SET_NFLG (1);
		Exception_cpu(6);
		return 16 * CYCLE_UNIT / 2;
	}
}}}}}return 16 * CYCLE_UNIT / 2;
}

/* CHK.W (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_41b8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	m68k_incpc (4);
	if (dst > src) {
		SET_NFLG (0);
		Exception_cpu(6);
		return 12 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		SET_NFLG (1);
		Exception_cpu(6);
		return 14 * CYCLE_UNIT / 2;
	}
}}}}return 14 * CYCLE_UNIT / 2;
}

/* CHK.W (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_41b9_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	m68k_incpc (6);
	if (dst > src) {
		SET_NFLG (0);
		Exception_cpu(6);
		return 13 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		SET_NFLG (1);
		Exception_cpu(6);
		return 15 * CYCLE_UNIT / 2;
	}
}}}}return 15 * CYCLE_UNIT / 2;
}

/* CHK.W (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_41ba_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	m68k_incpc (4);
	if (dst > src) {
		SET_NFLG (0);
		Exception_cpu(6);
		return 12 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		SET_NFLG (1);
		Exception_cpu(6);
		return 14 * CYCLE_UNIT / 2;
	}
}}}}return 14 * CYCLE_UNIT / 2;
}

/* CHK.W (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_41bb_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	if (dst > src) {
		SET_NFLG (0);
		Exception_cpu(6);
		return 14 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		SET_NFLG (1);
		Exception_cpu(6);
		return 16 * CYCLE_UNIT / 2;
	}
}}}}}return 16 * CYCLE_UNIT / 2;
}

/* CHK.W #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_41bc_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	m68k_incpc (4);
	if (dst > src) {
		SET_NFLG (0);
		Exception_cpu(6);
		return 9 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		SET_NFLG (1);
		Exception_cpu(6);
		return 11 * CYCLE_UNIT / 2;
	}
}}}return 11 * CYCLE_UNIT / 2;
}

/* LEA.L (An),An */
uae_u32 REGPARAM2 CPUFUNC(op_41d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	m68k_areg (regs, dstreg) = (srca);
}}}	m68k_incpc (2);
return 3 * CYCLE_UNIT / 2;
}

/* LEA.L (d16,An),An */
uae_u32 REGPARAM2 CPUFUNC(op_41e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	m68k_areg (regs, dstreg) = (srca);
}}}	m68k_incpc (4);
return 6 * CYCLE_UNIT / 2;
}

/* LEA.L (d8,An,Xn),An */
uae_u32 REGPARAM2 CPUFUNC(op_41f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	m68k_areg (regs, dstreg) = (srca);
}}}}return 8 * CYCLE_UNIT / 2;
}

/* LEA.L (xxx).W,An */
uae_u32 REGPARAM2 CPUFUNC(op_41f8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	m68k_areg (regs, dstreg) = (srca);
}}}	m68k_incpc (4);
return 6 * CYCLE_UNIT / 2;
}

/* LEA.L (xxx).L,An */
uae_u32 REGPARAM2 CPUFUNC(op_41f9_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	m68k_areg (regs, dstreg) = (srca);
}}}	m68k_incpc (6);
return 8 * CYCLE_UNIT / 2;
}

/* LEA.L (d16,PC),An */
uae_u32 REGPARAM2 CPUFUNC(op_41fa_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	m68k_areg (regs, dstreg) = (srca);
}}}	m68k_incpc (4);
return 6 * CYCLE_UNIT / 2;
}

/* LEA.L (d8,PC,Xn),An */
uae_u32 REGPARAM2 CPUFUNC(op_41fb_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	m68k_areg (regs, dstreg) = (srca);
}}}}return 8 * CYCLE_UNIT / 2;
}

/* CLR.B Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4200_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(0)) == 0);
	SET_NFLG   (((uae_s8)(0)) < 0);
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((0) & 0xff);
}}	m68k_incpc (2);
return 3 * CYCLE_UNIT / 2;
}

/* CLR.B (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4210_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(0)) == 0);
	SET_NFLG   (((uae_s8)(0)) < 0);
	put_byte_jit (srca,0);
}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* CLR.B (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4218_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(0)) == 0);
	SET_NFLG   (((uae_s8)(0)) < 0);
	put_byte_jit (srca,0);
}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* CLR.B -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_4220_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(0)) == 0);
	SET_NFLG   (((uae_s8)(0)) < 0);
	put_byte_jit (srca,0);
}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* CLR.B (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4228_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(0)) == 0);
	SET_NFLG   (((uae_s8)(0)) < 0);
	put_byte_jit (srca,0);
}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* CLR.B (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4230_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(0)) == 0);
	SET_NFLG   (((uae_s8)(0)) < 0);
	put_byte_jit (srca,0);
}}}return 11 * CYCLE_UNIT / 2;
}

/* CLR.B (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4238_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(0)) == 0);
	SET_NFLG   (((uae_s8)(0)) < 0);
	put_byte_jit (srca,0);
}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* CLR.B (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4239_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(0)) == 0);
	SET_NFLG   (((uae_s8)(0)) < 0);
	put_byte_jit (srca,0);
}}	m68k_incpc (6);
return 11 * CYCLE_UNIT / 2;
}

/* CLR.W Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4240_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(0)) == 0);
	SET_NFLG   (((uae_s16)(0)) < 0);
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | ((0) & 0xffff);
}}	m68k_incpc (2);
return 3 * CYCLE_UNIT / 2;
}

/* CLR.W (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4250_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(0)) == 0);
	SET_NFLG   (((uae_s16)(0)) < 0);
	put_word_jit (srca,0);
}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* CLR.W (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4258_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += 2;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(0)) == 0);
	SET_NFLG   (((uae_s16)(0)) < 0);
	put_word_jit (srca,0);
}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* CLR.W -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_4260_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
	m68k_areg (regs, srcreg) = srca;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(0)) == 0);
	SET_NFLG   (((uae_s16)(0)) < 0);
	put_word_jit (srca,0);
}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* CLR.W (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4268_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(0)) == 0);
	SET_NFLG   (((uae_s16)(0)) < 0);
	put_word_jit (srca,0);
}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* CLR.W (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4270_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(0)) == 0);
	SET_NFLG   (((uae_s16)(0)) < 0);
	put_word_jit (srca,0);
}}}return 11 * CYCLE_UNIT / 2;
}

/* CLR.W (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4278_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(0)) == 0);
	SET_NFLG   (((uae_s16)(0)) < 0);
	put_word_jit (srca,0);
}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* CLR.W (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4279_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(0)) == 0);
	SET_NFLG   (((uae_s16)(0)) < 0);
	put_word_jit (srca,0);
}}	m68k_incpc (6);
return 11 * CYCLE_UNIT / 2;
}

/* CLR.L Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4280_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(0)) == 0);
	SET_NFLG   (((uae_s32)(0)) < 0);
	m68k_dreg (regs, srcreg) = (0);
}}	m68k_incpc (2);
return 3 * CYCLE_UNIT / 2;
}

/* CLR.L (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4290_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(0)) == 0);
	SET_NFLG   (((uae_s32)(0)) < 0);
	put_long_jit (srca,0);
}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* CLR.L (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4298_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += 4;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(0)) == 0);
	SET_NFLG   (((uae_s32)(0)) < 0);
	put_long_jit (srca,0);
}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* CLR.L -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_42a0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
	m68k_areg (regs, srcreg) = srca;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(0)) == 0);
	SET_NFLG   (((uae_s32)(0)) < 0);
	put_long_jit (srca,0);
}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* CLR.L (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_42a8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(0)) == 0);
	SET_NFLG   (((uae_s32)(0)) < 0);
	put_long_jit (srca,0);
}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* CLR.L (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_42b0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(0)) == 0);
	SET_NFLG   (((uae_s32)(0)) < 0);
	put_long_jit (srca,0);
}}}return 11 * CYCLE_UNIT / 2;
}

/* CLR.L (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_42b8_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(0)) == 0);
	SET_NFLG   (((uae_s32)(0)) < 0);
	put_long_jit (srca,0);
}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* CLR.L (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_42b9_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(0)) == 0);
	SET_NFLG   (((uae_s32)(0)) < 0);
	put_long_jit (srca,0);
}}	m68k_incpc (6);
return 11 * CYCLE_UNIT / 2;
}

/* MVSR2.B Dn */
uae_u32 REGPARAM2 CPUFUNC(op_42c0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	MakeSR ();
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | ((regs.sr & 0xff) & 0xffff);
}}	m68k_incpc (2);
return 5 * CYCLE_UNIT / 2;
}

/* MVSR2.B (An) */
uae_u32 REGPARAM2 CPUFUNC(op_42d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	MakeSR ();
	put_word_jit (srca,regs.sr & 0xff);
}}	m68k_incpc (2);
return 9 * CYCLE_UNIT / 2;
}

/* MVSR2.B (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_42d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += 2;
	MakeSR ();
	put_word_jit (srca,regs.sr & 0xff);
}}	m68k_incpc (2);
return 9 * CYCLE_UNIT / 2;
}

/* MVSR2.B -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_42e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
	m68k_areg (regs, srcreg) = srca;
	MakeSR ();
	put_word_jit (srca,regs.sr & 0xff);
}}	m68k_incpc (2);
return 9 * CYCLE_UNIT / 2;
}

/* MVSR2.B (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_42e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	MakeSR ();
	put_word_jit (srca,regs.sr & 0xff);
}}	m68k_incpc (4);
return 10 * CYCLE_UNIT / 2;
}

/* MVSR2.B (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_42f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
	MakeSR ();
	put_word_jit (srca,regs.sr & 0xff);
}}}return 12 * CYCLE_UNIT / 2;
}

/* MVSR2.B (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_42f8_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	MakeSR ();
	put_word_jit (srca,regs.sr & 0xff);
}}	m68k_incpc (4);
return 10 * CYCLE_UNIT / 2;
}

/* MVSR2.B (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_42f9_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
	MakeSR ();
	put_word_jit (srca,regs.sr & 0xff);
}}	m68k_incpc (6);
return 12 * CYCLE_UNIT / 2;
}

/* NEG.B Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4400_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{{uae_u32 dst = ((uae_u8)(0)) - ((uae_u8)(src));
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
return 3 * CYCLE_UNIT / 2;
}

#endif

#ifdef PART_4
/* NEG.B (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4410_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
{{uae_u32 dst = ((uae_u8)(0)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(dst)) < 0;
	SET_ZFLG (((uae_s8)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (srca,dst);
}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* NEG.B (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4418_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{uae_u32 dst = ((uae_u8)(0)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(dst)) < 0;
	SET_ZFLG (((uae_s8)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (srca,dst);
}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* NEG.B -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_4420_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{{uae_u32 dst = ((uae_u8)(0)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(dst)) < 0;
	SET_ZFLG (((uae_s8)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (srca,dst);
}}}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

/* NEG.B (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4428_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{{uae_u32 dst = ((uae_u8)(0)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(dst)) < 0;
	SET_ZFLG (((uae_s8)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (srca,dst);
}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* NEG.B (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4430_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s8 src = get_byte_jit (srca);
{{uae_u32 dst = ((uae_u8)(0)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(dst)) < 0;
	SET_ZFLG (((uae_s8)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (srca,dst);
}}}}}}}return 14 * CYCLE_UNIT / 2;
}

/* NEG.B (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4438_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{{uae_u32 dst = ((uae_u8)(0)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(dst)) < 0;
	SET_ZFLG (((uae_s8)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (srca,dst);
}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* NEG.B (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4439_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte_jit (srca);
{{uae_u32 dst = ((uae_u8)(0)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(dst)) < 0;
	SET_ZFLG (((uae_s8)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (srca,dst);
}}}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* NEG.W Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4440_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{{uae_u32 dst = ((uae_u16)(0)) - ((uae_u16)(src));
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
return 3 * CYCLE_UNIT / 2;
}

/* NEG.W (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4450_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
{{uae_u32 dst = ((uae_u16)(0)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(dst)) < 0;
	SET_ZFLG (((uae_s16)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (srca,dst);
}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* NEG.W (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4458_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) += 2;
{{uae_u32 dst = ((uae_u16)(0)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(dst)) < 0;
	SET_ZFLG (((uae_s16)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (srca,dst);
}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* NEG.W -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_4460_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{{uae_u32 dst = ((uae_u16)(0)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(dst)) < 0;
	SET_ZFLG (((uae_s16)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (srca,dst);
}}}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

/* NEG.W (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4468_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{{uae_u32 dst = ((uae_u16)(0)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(dst)) < 0;
	SET_ZFLG (((uae_s16)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (srca,dst);
}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* NEG.W (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4470_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s16 src = get_word_jit (srca);
{{uae_u32 dst = ((uae_u16)(0)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(dst)) < 0;
	SET_ZFLG (((uae_s16)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (srca,dst);
}}}}}}}return 14 * CYCLE_UNIT / 2;
}

/* NEG.W (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4478_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{{uae_u32 dst = ((uae_u16)(0)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(dst)) < 0;
	SET_ZFLG (((uae_s16)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (srca,dst);
}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* NEG.W (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4479_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word_jit (srca);
{{uae_u32 dst = ((uae_u16)(0)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(dst)) < 0;
	SET_ZFLG (((uae_s16)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (srca,dst);
}}}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* NEG.L Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4480_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{{uae_u32 dst = ((uae_u32)(0)) - ((uae_u32)(src));
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
return 3 * CYCLE_UNIT / 2;
}

/* NEG.L (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4490_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
{{uae_u32 dst = ((uae_u32)(0)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(dst)) < 0;
	SET_ZFLG (((uae_s32)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (srca,dst);
}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* NEG.L (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4498_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) += 4;
{{uae_u32 dst = ((uae_u32)(0)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(dst)) < 0;
	SET_ZFLG (((uae_s32)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (srca,dst);
}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* NEG.L -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_44a0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{{uae_u32 dst = ((uae_u32)(0)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(dst)) < 0;
	SET_ZFLG (((uae_s32)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (srca,dst);
}}}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

/* NEG.L (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_44a8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{{uae_u32 dst = ((uae_u32)(0)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(dst)) < 0;
	SET_ZFLG (((uae_s32)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (srca,dst);
}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* NEG.L (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_44b0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s32 src = get_long_jit (srca);
{{uae_u32 dst = ((uae_u32)(0)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(dst)) < 0;
	SET_ZFLG (((uae_s32)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (srca,dst);
}}}}}}}return 14 * CYCLE_UNIT / 2;
}

/* NEG.L (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_44b8_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{{uae_u32 dst = ((uae_u32)(0)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(dst)) < 0;
	SET_ZFLG (((uae_s32)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (srca,dst);
}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* NEG.L (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_44b9_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long_jit (srca);
{{uae_u32 dst = ((uae_u32)(0)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(dst)) < 0;
	SET_ZFLG (((uae_s32)(dst)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(0)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (srca,dst);
}}}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* MV2SR.B Dn */
uae_u32 REGPARAM2 CPUFUNC(op_44c0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
	MakeSR ();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR_T0();
	m68k_incpc (2);
}}return 5 * CYCLE_UNIT / 2;
}

/* MV2SR.B (An) */
uae_u32 REGPARAM2 CPUFUNC(op_44d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
	MakeSR ();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR_T0();
	m68k_incpc (2);
}}}return 9 * CYCLE_UNIT / 2;
}

/* MV2SR.B (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_44d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) += 2;
	MakeSR ();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR_T0();
	m68k_incpc (2);
}}}return 9 * CYCLE_UNIT / 2;
}

/* MV2SR.B -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_44e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) = srca;
	MakeSR ();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR_T0();
	m68k_incpc (2);
}}}return 10 * CYCLE_UNIT / 2;
}

/* MV2SR.B (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_44e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
	MakeSR ();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR_T0();
	m68k_incpc (4);
}}}return 11 * CYCLE_UNIT / 2;
}

/* MV2SR.B (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_44f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s16 src = get_word_jit (srca);
	MakeSR ();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR_T0();
}}}}return 13 * CYCLE_UNIT / 2;
}

/* MV2SR.B (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_44f8_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
	MakeSR ();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR_T0();
	m68k_incpc (4);
}}}return 11 * CYCLE_UNIT / 2;
}

/* MV2SR.B (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_44f9_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word_jit (srca);
	MakeSR ();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR_T0();
	m68k_incpc (6);
}}}return 12 * CYCLE_UNIT / 2;
}

/* MV2SR.B (d16,PC) */
uae_u32 REGPARAM2 CPUFUNC(op_44fa_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
	MakeSR ();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR_T0();
	m68k_incpc (4);
}}}return 11 * CYCLE_UNIT / 2;
}

/* MV2SR.B (d8,PC,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_44fb_40)(uae_u32 opcode)
{
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s16 src = get_word_jit (srca);
	MakeSR ();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR_T0();
}}}}return 13 * CYCLE_UNIT / 2;
}

/* MV2SR.B #<data>.B */
uae_u32 REGPARAM2 CPUFUNC(op_44fc_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
	MakeSR ();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR_T0();
	m68k_incpc (4);
}}return 8 * CYCLE_UNIT / 2;
}

/* NOT.B Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4600_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(dst)) == 0);
	SET_NFLG   (((uae_s8)(dst)) < 0);
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((dst) & 0xff);
}}}	m68k_incpc (2);
return 3 * CYCLE_UNIT / 2;
}

/* NOT.B (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4610_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(dst)) == 0);
	SET_NFLG   (((uae_s8)(dst)) < 0);
	put_byte_jit (srca,dst);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* NOT.B (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4618_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(dst)) == 0);
	SET_NFLG   (((uae_s8)(dst)) < 0);
	put_byte_jit (srca,dst);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* NOT.B -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_4620_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(dst)) == 0);
	SET_NFLG   (((uae_s8)(dst)) < 0);
	put_byte_jit (srca,dst);
}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

/* NOT.B (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4628_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(dst)) == 0);
	SET_NFLG   (((uae_s8)(dst)) < 0);
	put_byte_jit (srca,dst);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* NOT.B (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4630_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s8 src = get_byte_jit (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(dst)) == 0);
	SET_NFLG   (((uae_s8)(dst)) < 0);
	put_byte_jit (srca,dst);
}}}}}return 14 * CYCLE_UNIT / 2;
}

/* NOT.B (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4638_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(dst)) == 0);
	SET_NFLG   (((uae_s8)(dst)) < 0);
	put_byte_jit (srca,dst);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* NOT.B (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4639_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte_jit (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(dst)) == 0);
	SET_NFLG   (((uae_s8)(dst)) < 0);
	put_byte_jit (srca,dst);
}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* NOT.W Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4640_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(dst)) == 0);
	SET_NFLG   (((uae_s16)(dst)) < 0);
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | ((dst) & 0xffff);
}}}	m68k_incpc (2);
return 3 * CYCLE_UNIT / 2;
}

/* NOT.W (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4650_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(dst)) == 0);
	SET_NFLG   (((uae_s16)(dst)) < 0);
	put_word_jit (srca,dst);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* NOT.W (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4658_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) += 2;
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(dst)) == 0);
	SET_NFLG   (((uae_s16)(dst)) < 0);
	put_word_jit (srca,dst);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* NOT.W -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_4660_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(dst)) == 0);
	SET_NFLG   (((uae_s16)(dst)) < 0);
	put_word_jit (srca,dst);
}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

/* NOT.W (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4668_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(dst)) == 0);
	SET_NFLG   (((uae_s16)(dst)) < 0);
	put_word_jit (srca,dst);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* NOT.W (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4670_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s16 src = get_word_jit (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(dst)) == 0);
	SET_NFLG   (((uae_s16)(dst)) < 0);
	put_word_jit (srca,dst);
}}}}}return 14 * CYCLE_UNIT / 2;
}

/* NOT.W (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4678_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(dst)) == 0);
	SET_NFLG   (((uae_s16)(dst)) < 0);
	put_word_jit (srca,dst);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* NOT.W (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4679_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(dst)) == 0);
	SET_NFLG   (((uae_s16)(dst)) < 0);
	put_word_jit (srca,dst);
}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* NOT.L Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4680_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(dst)) == 0);
	SET_NFLG   (((uae_s32)(dst)) < 0);
	m68k_dreg (regs, srcreg) = (dst);
}}}	m68k_incpc (2);
return 3 * CYCLE_UNIT / 2;
}

/* NOT.L (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4690_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(dst)) == 0);
	SET_NFLG   (((uae_s32)(dst)) < 0);
	put_long_jit (srca,dst);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* NOT.L (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4698_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) += 4;
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(dst)) == 0);
	SET_NFLG   (((uae_s32)(dst)) < 0);
	put_long_jit (srca,dst);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* NOT.L -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_46a0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(dst)) == 0);
	SET_NFLG   (((uae_s32)(dst)) < 0);
	put_long_jit (srca,dst);
}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

/* NOT.L (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_46a8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(dst)) == 0);
	SET_NFLG   (((uae_s32)(dst)) < 0);
	put_long_jit (srca,dst);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* NOT.L (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_46b0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s32 src = get_long_jit (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(dst)) == 0);
	SET_NFLG   (((uae_s32)(dst)) < 0);
	put_long_jit (srca,dst);
}}}}}return 14 * CYCLE_UNIT / 2;
}

/* NOT.L (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_46b8_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(dst)) == 0);
	SET_NFLG   (((uae_s32)(dst)) < 0);
	put_long_jit (srca,dst);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* NOT.L (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_46b9_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_u32 dst = ~src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(dst)) == 0);
	SET_NFLG   (((uae_s32)(dst)) < 0);
	put_long_jit (srca,dst);
}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* MV2SR.W Dn */
uae_u32 REGPARAM2 CPUFUNC(op_46c0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uae_s16 src = m68k_dreg (regs, srcreg);
	regs.sr = src;
	MakeFromSR_T0();
	m68k_incpc (2);
}}}return 11 * CYCLE_UNIT / 2;
}

/* MV2SR.W (An) */
uae_u32 REGPARAM2 CPUFUNC(op_46d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
	regs.sr = src;
	MakeFromSR_T0();
	m68k_incpc (2);
}}}}return 15 * CYCLE_UNIT / 2;
}

/* MV2SR.W (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_46d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) += 2;
	regs.sr = src;
	MakeFromSR_T0();
	m68k_incpc (2);
}}}}return 15 * CYCLE_UNIT / 2;
}

/* MV2SR.W -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_46e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) = srca;
	regs.sr = src;
	MakeFromSR_T0();
	m68k_incpc (2);
}}}}return 16 * CYCLE_UNIT / 2;
}

/* MV2SR.W (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_46e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
	regs.sr = src;
	MakeFromSR_T0();
	m68k_incpc (4);
}}}}return 17 * CYCLE_UNIT / 2;
}

/* MV2SR.W (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_46f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s16 src = get_word_jit (srca);
	regs.sr = src;
	MakeFromSR_T0();
}}}}}return 19 * CYCLE_UNIT / 2;
}

/* MV2SR.W (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_46f8_40)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
	regs.sr = src;
	MakeFromSR_T0();
	m68k_incpc (4);
}}}}return 17 * CYCLE_UNIT / 2;
}

/* MV2SR.W (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_46f9_40)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word_jit (srca);
	regs.sr = src;
	MakeFromSR_T0();
	m68k_incpc (6);
}}}}return 18 * CYCLE_UNIT / 2;
}

/* MV2SR.W (d16,PC) */
uae_u32 REGPARAM2 CPUFUNC(op_46fa_40)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
	regs.sr = src;
	MakeFromSR_T0();
	m68k_incpc (4);
}}}}return 17 * CYCLE_UNIT / 2;
}

/* MV2SR.W (d8,PC,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_46fb_40)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s16 src = get_word_jit (srca);
	regs.sr = src;
	MakeFromSR_T0();
}}}}}return 19 * CYCLE_UNIT / 2;
}

/* MV2SR.W #<data>.W */
uae_u32 REGPARAM2 CPUFUNC(op_46fc_40)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uae_s16 src = get_diword (2);
	regs.sr = src;
	MakeFromSR_T0();
	m68k_incpc (4);
}}}return 14 * CYCLE_UNIT / 2;
}

/* NBCD.B Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4800_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uae_u16 newv_lo = - (src & 0xF) - (GET_XFLG () ? 1 : 0);
	uae_u16 newv_hi = - (src & 0xF0);
	uae_u16 newv;
	int cflg;
	if (newv_lo > 9) { newv_lo -= 6; }
	newv = newv_hi + newv_lo;
	cflg = (newv & 0x1F0) > 0x90;
	if (cflg) newv -= 0x60;
	SET_CFLG (cflg);
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s8)(newv)) == 0));
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((newv) & 0xff);
}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* LINK.L An,#<data>.L */
uae_u32 REGPARAM2 CPUFUNC(op_4808_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr olda;
	olda = m68k_areg (regs, 7) - 4;
	m68k_areg (regs, 7) = olda;
{	uae_s32 src = m68k_areg (regs, srcreg);
{	uae_s32 offs;
	offs = get_dilong (2);
	put_long_jit (olda,src);
	m68k_areg (regs, srcreg) = (m68k_areg(regs, 7));
	m68k_areg(regs, 7) += offs;
}}}}	m68k_incpc (6);
return 10 * CYCLE_UNIT / 2;
}

/* NBCD.B (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4810_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
{	uae_u16 newv_lo = - (src & 0xF) - (GET_XFLG () ? 1 : 0);
	uae_u16 newv_hi = - (src & 0xF0);
	uae_u16 newv;
	int cflg;
	if (newv_lo > 9) { newv_lo -= 6; }
	newv = newv_hi + newv_lo;
	cflg = (newv & 0x1F0) > 0x90;
	if (cflg) newv -= 0x60;
	SET_CFLG (cflg);
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s8)(newv)) == 0));
	put_byte_jit (srca,newv);
}}}}	m68k_incpc (2);
return 13 * CYCLE_UNIT / 2;
}

/* NBCD.B (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4818_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{	uae_u16 newv_lo = - (src & 0xF) - (GET_XFLG () ? 1 : 0);
	uae_u16 newv_hi = - (src & 0xF0);
	uae_u16 newv;
	int cflg;
	if (newv_lo > 9) { newv_lo -= 6; }
	newv = newv_hi + newv_lo;
	cflg = (newv & 0x1F0) > 0x90;
	if (cflg) newv -= 0x60;
	SET_CFLG (cflg);
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s8)(newv)) == 0));
	put_byte_jit (srca,newv);
}}}}	m68k_incpc (2);
return 13 * CYCLE_UNIT / 2;
}

/* NBCD.B -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_4820_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_u16 newv_lo = - (src & 0xF) - (GET_XFLG () ? 1 : 0);
	uae_u16 newv_hi = - (src & 0xF0);
	uae_u16 newv;
	int cflg;
	if (newv_lo > 9) { newv_lo -= 6; }
	newv = newv_hi + newv_lo;
	cflg = (newv & 0x1F0) > 0x90;
	if (cflg) newv -= 0x60;
	SET_CFLG (cflg);
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s8)(newv)) == 0));
	put_byte_jit (srca,newv);
}}}}	m68k_incpc (2);
return 14 * CYCLE_UNIT / 2;
}

/* NBCD.B (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4828_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uae_u16 newv_lo = - (src & 0xF) - (GET_XFLG () ? 1 : 0);
	uae_u16 newv_hi = - (src & 0xF0);
	uae_u16 newv;
	int cflg;
	if (newv_lo > 9) { newv_lo -= 6; }
	newv = newv_hi + newv_lo;
	cflg = (newv & 0x1F0) > 0x90;
	if (cflg) newv -= 0x60;
	SET_CFLG (cflg);
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s8)(newv)) == 0));
	put_byte_jit (srca,newv);
}}}}	m68k_incpc (4);
return 15 * CYCLE_UNIT / 2;
}

/* NBCD.B (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4830_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s8 src = get_byte_jit (srca);
{	uae_u16 newv_lo = - (src & 0xF) - (GET_XFLG () ? 1 : 0);
	uae_u16 newv_hi = - (src & 0xF0);
	uae_u16 newv;
	int cflg;
	if (newv_lo > 9) { newv_lo -= 6; }
	newv = newv_hi + newv_lo;
	cflg = (newv & 0x1F0) > 0x90;
	if (cflg) newv -= 0x60;
	SET_CFLG (cflg);
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s8)(newv)) == 0));
	put_byte_jit (srca,newv);
}}}}}return 17 * CYCLE_UNIT / 2;
}

/* NBCD.B (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4838_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uae_u16 newv_lo = - (src & 0xF) - (GET_XFLG () ? 1 : 0);
	uae_u16 newv_hi = - (src & 0xF0);
	uae_u16 newv;
	int cflg;
	if (newv_lo > 9) { newv_lo -= 6; }
	newv = newv_hi + newv_lo;
	cflg = (newv & 0x1F0) > 0x90;
	if (cflg) newv -= 0x60;
	SET_CFLG (cflg);
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s8)(newv)) == 0));
	put_byte_jit (srca,newv);
}}}}	m68k_incpc (4);
return 15 * CYCLE_UNIT / 2;
}

/* NBCD.B (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4839_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte_jit (srca);
{	uae_u16 newv_lo = - (src & 0xF) - (GET_XFLG () ? 1 : 0);
	uae_u16 newv_hi = - (src & 0xF0);
	uae_u16 newv;
	int cflg;
	if (newv_lo > 9) { newv_lo -= 6; }
	newv = newv_hi + newv_lo;
	cflg = (newv & 0x1F0) > 0x90;
	if (cflg) newv -= 0x60;
	SET_CFLG (cflg);
	COPY_CARRY ();
	SET_ZFLG (GET_ZFLG () & (((uae_s8)(newv)) == 0));
	put_byte_jit (srca,newv);
}}}}	m68k_incpc (6);
return 16 * CYCLE_UNIT / 2;
}

/* SWAP.W Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4840_40)(uae_u32 opcode)
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

/* BKPTQ.L #<data> */
uae_u32 REGPARAM2 CPUFUNC(op_4848_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{	m68k_incpc (2);
	op_illg (opcode);
}return 4 * CYCLE_UNIT / 2;
}

/* PEA.L (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4850_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, 7) - 4;
	m68k_areg (regs, 7) = dsta;
	put_long_jit (dsta,srca);
}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* PEA.L (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4868_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, 7) - 4;
	m68k_areg (regs, 7) = dsta;
	put_long_jit (dsta,srca);
}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* PEA.L (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4870_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uaecptr dsta;
	dsta = m68k_areg (regs, 7) - 4;
	m68k_areg (regs, 7) = dsta;
	put_long_jit (dsta,srca);
}}}}return 11 * CYCLE_UNIT / 2;
}

/* PEA.L (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4878_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, 7) - 4;
	m68k_areg (regs, 7) = dsta;
	put_long_jit (dsta,srca);
}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* PEA.L (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4879_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, 7) - 4;
	m68k_areg (regs, 7) = dsta;
	put_long_jit (dsta,srca);
}}}	m68k_incpc (6);
return 11 * CYCLE_UNIT / 2;
}

/* PEA.L (d16,PC) */
uae_u32 REGPARAM2 CPUFUNC(op_487a_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, 7) - 4;
	m68k_areg (regs, 7) = dsta;
	put_long_jit (dsta,srca);
}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* PEA.L (d8,PC,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_487b_40)(uae_u32 opcode)
{
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uaecptr dsta;
	dsta = m68k_areg (regs, 7) - 4;
	m68k_areg (regs, 7) = dsta;
	put_long_jit (dsta,srca);
}}}}return 11 * CYCLE_UNIT / 2;
}

/* EXT.W Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4880_40)(uae_u32 opcode)
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
uae_u32 REGPARAM2 CPUFUNC(op_4890_40)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_diword (2);
{	uaecptr srca;
	srca = m68k_areg (regs, dstreg);
{	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) {
		put_word_jit (srca, m68k_dreg (regs, movem_index1[dmask]));
		srca += 2;
		dmask = movem_next[dmask];
		count_cycles += 3 * CYCLE_UNIT / 2;
	}
	while (amask) {
		put_word_jit (srca, m68k_areg (regs, movem_index1[amask]));
		srca += 2;
		amask = movem_next[amask];
		count_cycles += 3 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (4);
return 8 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMLE.W #<data>.W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_48a0_40)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_diword (2);
{	uaecptr srca;
	srca = m68k_areg (regs, dstreg) - 0;
{	uae_u16 amask = mask & 0xff, dmask = (mask >> 8) & 0xff;
	while (amask) {
		srca -= 2;
		m68k_areg (regs, dstreg) = srca;
		put_word_jit (srca, m68k_areg (regs, movem_index2[amask]));
		amask = movem_next[amask];
		count_cycles += 3 * CYCLE_UNIT / 2;
	}
	while (dmask) {
		srca -= 2;
		put_word_jit (srca, m68k_dreg (regs, movem_index2[dmask]));
		dmask = movem_next[dmask];
		count_cycles += 3 * CYCLE_UNIT / 2;
	}
	m68k_areg (regs, dstreg) = srca;
}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMLE.W #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_48a8_40)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_diword (2);
{	uaecptr srca;
	srca = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) {
		put_word_jit (srca, m68k_dreg (regs, movem_index1[dmask]));
		srca += 2;
		dmask = movem_next[dmask];
		count_cycles += 3 * CYCLE_UNIT / 2;
	}
	while (amask) {
		put_word_jit (srca, m68k_areg (regs, movem_index1[amask]));
		srca += 2;
		amask = movem_next[amask];
		count_cycles += 3 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (6);
return 18 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMLE.W #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_48b0_40)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_diword (2);
{	uaecptr srca;
	m68k_incpc (4);
{	srca = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) {
		put_word_jit (srca, m68k_dreg (regs, movem_index1[dmask]));
		srca += 2;
		dmask = movem_next[dmask];
		count_cycles += 3 * CYCLE_UNIT / 2;
	}
	while (amask) {
		put_word_jit (srca, m68k_areg (regs, movem_index1[amask]));
		srca += 2;
		amask = movem_next[amask];
		count_cycles += 3 * CYCLE_UNIT / 2;
	}
}}}}return 14 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMLE.W #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_48b8_40)(uae_u32 opcode)
{
	int count_cycles = 0;
{	uae_u16 mask = get_diword (2);
{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (4);
{	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) {
		put_word_jit (srca, m68k_dreg (regs, movem_index1[dmask]));
		srca += 2;
		dmask = movem_next[dmask];
		count_cycles += 3 * CYCLE_UNIT / 2;
	}
	while (amask) {
		put_word_jit (srca, m68k_areg (regs, movem_index1[amask]));
		srca += 2;
		amask = movem_next[amask];
		count_cycles += 3 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (6);
return 10 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMLE.W #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_48b9_40)(uae_u32 opcode)
{
	int count_cycles = 0;
{	uae_u16 mask = get_diword (2);
{	uaecptr srca;
	srca = get_dilong (4);
{	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) {
		put_word_jit (srca, m68k_dreg (regs, movem_index1[dmask]));
		srca += 2;
		dmask = movem_next[dmask];
		count_cycles += 3 * CYCLE_UNIT / 2;
	}
	while (amask) {
		put_word_jit (srca, m68k_areg (regs, movem_index1[amask]));
		srca += 2;
		amask = movem_next[amask];
		count_cycles += 3 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (8);
return 11 * CYCLE_UNIT / 2 + count_cycles;
}

/* EXT.L Dn */
uae_u32 REGPARAM2 CPUFUNC(op_48c0_40)(uae_u32 opcode)
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
uae_u32 REGPARAM2 CPUFUNC(op_48d0_40)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_diword (2);
{	uaecptr srca;
	srca = m68k_areg (regs, dstreg);
{	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) {
		put_long_jit (srca, m68k_dreg (regs, movem_index1[dmask]));
		srca += 4;
		dmask = movem_next[dmask];
		count_cycles += 3 * CYCLE_UNIT / 2;
	}
	while (amask) {
		put_long_jit (srca, m68k_areg (regs, movem_index1[amask]));
		srca += 4;
		amask = movem_next[amask];
		count_cycles += 3 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (4);
return 8 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMLE.L #<data>.W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_48e0_40)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_diword (2);
{	uaecptr srca;
	srca = m68k_areg (regs, dstreg) - 0;
{	uae_u16 amask = mask & 0xff, dmask = (mask >> 8) & 0xff;
	while (amask) {
		srca -= 4;
		m68k_areg (regs, dstreg) = srca;
		put_long_jit (srca, m68k_areg (regs, movem_index2[amask]));
		amask = movem_next[amask];
		count_cycles += 3 * CYCLE_UNIT / 2;
	}
	while (dmask) {
		srca -= 4;
		put_long_jit (srca, m68k_dreg (regs, movem_index2[dmask]));
		dmask = movem_next[dmask];
		count_cycles += 3 * CYCLE_UNIT / 2;
	}
	m68k_areg (regs, dstreg) = srca;
}}}	m68k_incpc (4);
return 10 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMLE.L #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_48e8_40)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_diword (2);
{	uaecptr srca;
	srca = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) {
		put_long_jit (srca, m68k_dreg (regs, movem_index1[dmask]));
		srca += 4;
		dmask = movem_next[dmask];
		count_cycles += 3 * CYCLE_UNIT / 2;
	}
	while (amask) {
		put_long_jit (srca, m68k_areg (regs, movem_index1[amask]));
		srca += 4;
		amask = movem_next[amask];
		count_cycles += 3 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (6);
return 18 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMLE.L #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_48f0_40)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_diword (2);
{	uaecptr srca;
	m68k_incpc (4);
{	srca = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) {
		put_long_jit (srca, m68k_dreg (regs, movem_index1[dmask]));
		srca += 4;
		dmask = movem_next[dmask];
		count_cycles += 3 * CYCLE_UNIT / 2;
	}
	while (amask) {
		put_long_jit (srca, m68k_areg (regs, movem_index1[amask]));
		srca += 4;
		amask = movem_next[amask];
		count_cycles += 3 * CYCLE_UNIT / 2;
	}
}}}}return 13 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMLE.L #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_48f8_40)(uae_u32 opcode)
{
	int count_cycles = 0;
{	uae_u16 mask = get_diword (2);
{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (4);
{	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) {
		put_long_jit (srca, m68k_dreg (regs, movem_index1[dmask]));
		srca += 4;
		dmask = movem_next[dmask];
		count_cycles += 3 * CYCLE_UNIT / 2;
	}
	while (amask) {
		put_long_jit (srca, m68k_areg (regs, movem_index1[amask]));
		srca += 4;
		amask = movem_next[amask];
		count_cycles += 3 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (6);
return 10 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMLE.L #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_48f9_40)(uae_u32 opcode)
{
	int count_cycles = 0;
{	uae_u16 mask = get_diword (2);
{	uaecptr srca;
	srca = get_dilong (4);
{	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) {
		put_long_jit (srca, m68k_dreg (regs, movem_index1[dmask]));
		srca += 4;
		dmask = movem_next[dmask];
		count_cycles += 3 * CYCLE_UNIT / 2;
	}
	while (amask) {
		put_long_jit (srca, m68k_areg (regs, movem_index1[amask]));
		srca += 4;
		amask = movem_next[amask];
		count_cycles += 3 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (8);
return 11 * CYCLE_UNIT / 2 + count_cycles;
}

/* EXT.B Dn */
uae_u32 REGPARAM2 CPUFUNC(op_49c0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uae_u32 dst = (uae_s32)(uae_s8)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(dst)) == 0);
	SET_NFLG   (((uae_s32)(dst)) < 0);
	m68k_dreg (regs, srcreg) = (dst);
}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* TST.B Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4a00_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
}}	m68k_incpc (2);
return 3 * CYCLE_UNIT / 2;
}

/* TST.B (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4a10_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
}}}	m68k_incpc (2);
return 7 * CYCLE_UNIT / 2;
}

/* TST.B (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4a18_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
}}}	m68k_incpc (2);
return 7 * CYCLE_UNIT / 2;
}

/* TST.B -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_4a20_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) = srca;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* TST.B (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4a28_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* TST.B (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4a30_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s8 src = get_byte_jit (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
}}}}return 11 * CYCLE_UNIT / 2;
}

/* TST.B (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4a38_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* TST.B (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4a39_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte_jit (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
}}}	m68k_incpc (6);
return 10 * CYCLE_UNIT / 2;
}

/* TST.B (d16,PC) */
uae_u32 REGPARAM2 CPUFUNC(op_4a3a_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* TST.B (d8,PC,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4a3b_40)(uae_u32 opcode)
{
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s8 src = get_byte_jit (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
}}}}return 11 * CYCLE_UNIT / 2;
}

/* TST.B #<data>.B */
uae_u32 REGPARAM2 CPUFUNC(op_4a3c_40)(uae_u32 opcode)
{
{{	uae_s8 src = get_dibyte (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
}}	m68k_incpc (4);
return 4 * CYCLE_UNIT / 2;
}

/* TST.W Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4a40_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
}}	m68k_incpc (2);
return 3 * CYCLE_UNIT / 2;
}

/* TST.W An */
uae_u32 REGPARAM2 CPUFUNC(op_4a48_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_areg (regs, srcreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
}}	m68k_incpc (2);
return 3 * CYCLE_UNIT / 2;
}

/* TST.W (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4a50_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
}}}	m68k_incpc (2);
return 7 * CYCLE_UNIT / 2;
}

/* TST.W (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4a58_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) += 2;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
}}}	m68k_incpc (2);
return 7 * CYCLE_UNIT / 2;
}

/* TST.W -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_4a60_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) = srca;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* TST.W (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4a68_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* TST.W (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4a70_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s16 src = get_word_jit (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
}}}}return 11 * CYCLE_UNIT / 2;
}

/* TST.W (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4a78_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* TST.W (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4a79_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word_jit (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
}}}	m68k_incpc (6);
return 10 * CYCLE_UNIT / 2;
}

/* TST.W (d16,PC) */
uae_u32 REGPARAM2 CPUFUNC(op_4a7a_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* TST.W (d8,PC,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4a7b_40)(uae_u32 opcode)
{
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s16 src = get_word_jit (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
}}}}return 11 * CYCLE_UNIT / 2;
}

/* TST.W #<data>.W */
uae_u32 REGPARAM2 CPUFUNC(op_4a7c_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
}}	m68k_incpc (4);
return 4 * CYCLE_UNIT / 2;
}

/* TST.L Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4a80_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = m68k_dreg (regs, srcreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
}}	m68k_incpc (2);
return 3 * CYCLE_UNIT / 2;
}

/* TST.L An */
uae_u32 REGPARAM2 CPUFUNC(op_4a88_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = m68k_areg (regs, srcreg);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
}}	m68k_incpc (2);
return 3 * CYCLE_UNIT / 2;
}

/* TST.L (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4a90_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
}}}	m68k_incpc (2);
return 7 * CYCLE_UNIT / 2;
}

/* TST.L (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4a98_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) += 4;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
}}}	m68k_incpc (2);
return 7 * CYCLE_UNIT / 2;
}

/* TST.L -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_4aa0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) = srca;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* TST.L (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4aa8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* TST.L (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4ab0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s32 src = get_long_jit (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
}}}}return 11 * CYCLE_UNIT / 2;
}

/* TST.L (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4ab8_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* TST.L (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4ab9_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long_jit (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
}}}	m68k_incpc (6);
return 10 * CYCLE_UNIT / 2;
}

/* TST.L (d16,PC) */
uae_u32 REGPARAM2 CPUFUNC(op_4aba_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* TST.L (d8,PC,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4abb_40)(uae_u32 opcode)
{
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s32 src = get_long_jit (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
}}}}return 11 * CYCLE_UNIT / 2;
}

/* TST.L #<data>.L */
uae_u32 REGPARAM2 CPUFUNC(op_4abc_40)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
}}	m68k_incpc (6);
return 6 * CYCLE_UNIT / 2;
}

/* TAS.B Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4ac0_40)(uae_u32 opcode)
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
uae_u32 REGPARAM2 CPUFUNC(op_4ad0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	src |= 0x80;
	put_byte_jit (srca,src);
}}}	m68k_incpc (2);
return 15 * CYCLE_UNIT / 2;
}

/* TAS.B (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4ad8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	src |= 0x80;
	put_byte_jit (srca,src);
}}}	m68k_incpc (2);
return 15 * CYCLE_UNIT / 2;
}

/* TAS.B -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_4ae0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) = srca;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	src |= 0x80;
	put_byte_jit (srca,src);
}}}	m68k_incpc (2);
return 15 * CYCLE_UNIT / 2;
}

/* TAS.B (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4ae8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	src |= 0x80;
	put_byte_jit (srca,src);
}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* TAS.B (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4af0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s8 src = get_byte_jit (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	src |= 0x80;
	put_byte_jit (srca,src);
}}}}return 18 * CYCLE_UNIT / 2;
}

/* TAS.B (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4af8_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	src |= 0x80;
	put_byte_jit (srca,src);
}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2;
}

/* TAS.B (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4af9_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte_jit (srca);
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	src |= 0x80;
	put_byte_jit (srca,src);
}}}	m68k_incpc (6);
return 18 * CYCLE_UNIT / 2;
}

/* MULL.L #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4c00_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	m68k_incpc (4);
	m68k_mull(opcode, dst, extra);
}}}return 47 * CYCLE_UNIT / 2;
}

/* MULL.L #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_4c10_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long_jit (dsta);
	m68k_incpc (4);
	m68k_mull(opcode, dst, extra);
}}}}return 48 * CYCLE_UNIT / 2;
}

/* MULL.L #<data>.W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4c18_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long_jit (dsta);
	m68k_areg (regs, dstreg) += 4;
	m68k_incpc (4);
	m68k_mull(opcode, dst, extra);
}}}}return 51 * CYCLE_UNIT / 2;
}

/* MULL.L #<data>.W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_4c20_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
{	uae_s32 dst = get_long_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
	m68k_incpc (4);
	m68k_mull(opcode, dst, extra);
}}}}return 50 * CYCLE_UNIT / 2;
}

/* MULL.L #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4c28_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_s32 dst = get_long_jit (dsta);
	m68k_incpc (6);
	m68k_mull(opcode, dst, extra);
}}}}return 59 * CYCLE_UNIT / 2;
}

/* MULL.L #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4c30_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s32 dst = get_long_jit (dsta);
	m68k_mull(opcode, dst, extra);
}}}}}return 55 * CYCLE_UNIT / 2;
}

/* MULL.L #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4c38_40)(uae_u32 opcode)
{
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_s32 dst = get_long_jit (dsta);
	m68k_incpc (6);
	m68k_mull(opcode, dst, extra);
}}}}return 51 * CYCLE_UNIT / 2;
}

/* MULL.L #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4c39_40)(uae_u32 opcode)
{
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_s32 dst = get_long_jit (dsta);
	m68k_incpc (8);
	m68k_mull(opcode, dst, extra);
}}}}return 54 * CYCLE_UNIT / 2;
}

/* MULL.L #<data>.W,(d16,PC) */
uae_u32 REGPARAM2 CPUFUNC(op_4c3a_40)(uae_u32 opcode)
{
	uae_u32 dstreg = 2;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_getpc () + 4;
	dsta += (uae_s32)(uae_s16)get_diword (4);
{	uae_s32 dst = get_long_jit (dsta);
	m68k_incpc (6);
	m68k_mull(opcode, dst, extra);
}}}}return 59 * CYCLE_UNIT / 2;
}

/* MULL.L #<data>.W,(d8,PC,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4c3b_40)(uae_u32 opcode)
{
	uae_u32 dstreg = 3;
{{	uae_s16 extra = get_diword (2);
{	uaecptr tmppc;
	uaecptr dsta;
	m68k_incpc (4);
{	tmppc = m68k_getpc ();
	dsta = get_disp_ea_020 (tmppc, 0);
{	uae_s32 dst = get_long_jit (dsta);
	m68k_mull(opcode, dst, extra);
}}}}}return 55 * CYCLE_UNIT / 2;
}

/* MULL.L #<data>.W,#<data>.L */
uae_u32 REGPARAM2 CPUFUNC(op_4c3c_40)(uae_u32 opcode)
{
{{	uae_s16 extra = get_diword (2);
{	uae_s32 dst;
	dst = get_dilong (4);
	m68k_incpc (8);
	m68k_mull(opcode, dst, extra);
}}}return 52 * CYCLE_UNIT / 2;
}

/* DIVL.L #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4c40_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_u32 cyc = 0;
{{	uae_s16 extra = get_diword (2);
  if (extra & 0x800) cyc = 12 * CYCLE_UNIT / 2;
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	m68k_incpc (4);
  if (dst == 0) {
		Exception_cpu(5);
	  return 4 * CYCLE_UNIT / 2;
  }
	m68k_divl(opcode, dst, extra);
}}}return 82 * CYCLE_UNIT / 2 + cyc;
}

/* DIVL.L #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_4c50_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_u32 cyc = 0;
{{	uae_s16 extra = get_diword (2);
  if (extra & 0x800) cyc = 12 * CYCLE_UNIT / 2;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long_jit (dsta);
	m68k_incpc (4);
  if (dst == 0) {
		Exception_cpu(5);
	  return 4 * CYCLE_UNIT / 2;
  }
	m68k_divl(opcode, dst, extra);
}}}}return 83 * CYCLE_UNIT / 2 + cyc;
}

/* DIVL.L #<data>.W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4c58_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_u32 cyc = 0;
{{	uae_s16 extra = get_diword (2);
  if (extra & 0x800) cyc = 12 * CYCLE_UNIT / 2;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long_jit (dsta);
	m68k_areg (regs, dstreg) += 4;
	m68k_incpc (4);
  if (dst == 0) {
		Exception_cpu(5);
	  return 4 * CYCLE_UNIT / 2;
  }
	m68k_divl(opcode, dst, extra);
}}}}return 86 * CYCLE_UNIT / 2 + cyc;
}

/* DIVL.L #<data>.W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_4c60_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_u32 cyc = 0;
{{	uae_s16 extra = get_diword (2);
  if (extra & 0x800) cyc = 12 * CYCLE_UNIT / 2;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
{	uae_s32 dst = get_long_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
	m68k_incpc (4);
  if (dst == 0) {
		Exception_cpu(5);
	  return 4 * CYCLE_UNIT / 2;
  }
	m68k_divl(opcode, dst, extra);
}}}}return 85 * CYCLE_UNIT / 2 + cyc;
}

/* DIVL.L #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4c68_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_u32 cyc = 0;
{{	uae_s16 extra = get_diword (2);
  if (extra & 0x800) cyc = 12 * CYCLE_UNIT / 2;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_s32 dst = get_long_jit (dsta);
	m68k_incpc (6);
  if (dst == 0) {
		Exception_cpu(5);
	  return 4 * CYCLE_UNIT / 2;
  }
	m68k_divl(opcode, dst, extra);
}}}}return 94 * CYCLE_UNIT / 2 + cyc;
}

/* DIVL.L #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4c70_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_u32 cyc = 0;
{{	uae_s16 extra = get_diword (2);
  if (extra & 0x800) cyc = 12 * CYCLE_UNIT / 2;
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s32 dst = get_long_jit (dsta);
  if (dst == 0) {
		Exception_cpu(5);
	  return 4 * CYCLE_UNIT / 2;
  }
	m68k_divl(opcode, dst, extra);
}}}}}return 90 * CYCLE_UNIT / 2 + cyc;
}

/* DIVL.L #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4c78_40)(uae_u32 opcode)
{
	uae_u32 cyc = 0;
{{	uae_s16 extra = get_diword (2);
  if (extra & 0x800) cyc = 12 * CYCLE_UNIT / 2;
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_s32 dst = get_long_jit (dsta);
	m68k_incpc (6);
  if (dst == 0) {
		Exception_cpu(5);
	  return 4 * CYCLE_UNIT / 2;
  }
	m68k_divl(opcode, dst, extra);
}}}}return 86 * CYCLE_UNIT / 2 + cyc;
}

/* DIVL.L #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4c79_40)(uae_u32 opcode)
{
	uae_u32 cyc = 0;
{{	uae_s16 extra = get_diword (2);
  if (extra & 0x800) cyc = 12 * CYCLE_UNIT / 2;
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_s32 dst = get_long_jit (dsta);
	m68k_incpc (8);
  if (dst == 0) {
		Exception_cpu(5);
	  return 4 * CYCLE_UNIT / 2;
  }
	m68k_divl(opcode, dst, extra);
}}}}return 89 * CYCLE_UNIT / 2 + cyc;
}

/* DIVL.L #<data>.W,(d16,PC) */
uae_u32 REGPARAM2 CPUFUNC(op_4c7a_40)(uae_u32 opcode)
{
	uae_u32 dstreg = 2;
	uae_u32 cyc = 0;
{{	uae_s16 extra = get_diword (2);
  if (extra & 0x800) cyc = 12 * CYCLE_UNIT / 2;
{	uaecptr dsta;
	dsta = m68k_getpc () + 4;
	dsta += (uae_s32)(uae_s16)get_diword (4);
{	uae_s32 dst = get_long_jit (dsta);
	m68k_incpc (6);
  if (dst == 0) {
		Exception_cpu(5);
	  return 4 * CYCLE_UNIT / 2;
  }
	m68k_divl(opcode, dst, extra);
}}}}return 94 * CYCLE_UNIT / 2 + cyc;
}

/* DIVL.L #<data>.W,(d8,PC,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4c7b_40)(uae_u32 opcode)
{
	uae_u32 dstreg = 3;
	uae_u32 cyc = 0;
{{	uae_s16 extra = get_diword (2);
  if (extra & 0x800) cyc = 12 * CYCLE_UNIT / 2;
{	uaecptr tmppc;
	uaecptr dsta;
	m68k_incpc (4);
{	tmppc = m68k_getpc ();
	dsta = get_disp_ea_020 (tmppc, 0);
{	uae_s32 dst = get_long_jit (dsta);
  if (dst == 0) {
		Exception_cpu(5);
	  return 4 * CYCLE_UNIT / 2;
  }
	m68k_divl(opcode, dst, extra);
}}}}}return 90 * CYCLE_UNIT / 2 + cyc;
}

/* DIVL.L #<data>.W,#<data>.L */
uae_u32 REGPARAM2 CPUFUNC(op_4c7c_40)(uae_u32 opcode)
{
	uae_u32 cyc = 0;
{{	uae_s16 extra = get_diword (2);
  if (extra & 0x800) cyc = 12 * CYCLE_UNIT / 2;
{	uae_s32 dst;
	dst = get_dilong (4);
	m68k_incpc (8);
  if (dst == 0) {
		Exception_cpu(5);
	  return 4 * CYCLE_UNIT / 2;
  }
	m68k_divl(opcode, dst, extra);
}}}return 87 * CYCLE_UNIT / 2 + cyc;
}

/* MVMEL.W #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_4c90_40)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca;
	srca = m68k_areg (regs, dstreg);
{	while (dmask) {
		m68k_dreg (regs, movem_index1[dmask]) = (uae_s32)(uae_s16)get_word_jit (srca);
		srca += 2;
		dmask = movem_next[dmask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
	while (amask) {
		m68k_areg (regs, movem_index1[amask]) = (uae_s32)(uae_s16)get_word_jit (srca);
		srca += 2;
		amask = movem_next[amask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMEL.W #<data>.W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4c98_40)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca;
	srca = m68k_areg (regs, dstreg);
{	while (dmask) {
		m68k_dreg (regs, movem_index1[dmask]) = (uae_s32)(uae_s16)get_word_jit (srca);
		srca += 2;
		dmask = movem_next[dmask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
	while (amask) {
		m68k_areg (regs, movem_index1[amask]) = (uae_s32)(uae_s16)get_word_jit (srca);
		srca += 2;
		amask = movem_next[amask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
	m68k_areg (regs, dstreg) = srca;
}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMEL.W #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4ca8_40)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca;
	srca = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	while (dmask) {
		m68k_dreg (regs, movem_index1[dmask]) = (uae_s32)(uae_s16)get_word_jit (srca);
		srca += 2;
		dmask = movem_next[dmask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
	while (amask) {
		m68k_areg (regs, movem_index1[amask]) = (uae_s32)(uae_s16)get_word_jit (srca);
		srca += 2;
		amask = movem_next[amask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (6);
return 22 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMEL.W #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4cb0_40)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca;
	m68k_incpc (4);
{	srca = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	while (dmask) {
		m68k_dreg (regs, movem_index1[dmask]) = (uae_s32)(uae_s16)get_word_jit (srca);
		srca += 2;
		dmask = movem_next[dmask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
	while (amask) {
		m68k_areg (regs, movem_index1[amask]) = (uae_s32)(uae_s16)get_word_jit (srca);
		srca += 2;
		amask = movem_next[amask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
}}}}return 17 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMEL.W #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4cb8_40)(uae_u32 opcode)
{
	int count_cycles = 0;
{	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (4);
{	while (dmask) {
		m68k_dreg (regs, movem_index1[dmask]) = (uae_s32)(uae_s16)get_word_jit (srca);
		srca += 2;
		dmask = movem_next[dmask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
	while (amask) {
		m68k_areg (regs, movem_index1[amask]) = (uae_s32)(uae_s16)get_word_jit (srca);
		srca += 2;
		amask = movem_next[amask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (6);
return 14 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMEL.W #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4cb9_40)(uae_u32 opcode)
{
	int count_cycles = 0;
{	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca;
	srca = get_dilong (4);
{	while (dmask) {
		m68k_dreg (regs, movem_index1[dmask]) = (uae_s32)(uae_s16)get_word_jit (srca);
		srca += 2;
		dmask = movem_next[dmask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
	while (amask) {
		m68k_areg (regs, movem_index1[amask]) = (uae_s32)(uae_s16)get_word_jit (srca);
		srca += 2;
		amask = movem_next[amask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (8);
return 15 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMEL.W #<data>.W,(d16,PC) */
uae_u32 REGPARAM2 CPUFUNC(op_4cba_40)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = 2;
{	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca;
	srca = m68k_getpc () + 4;
	srca += (uae_s32)(uae_s16)get_diword (4);
{	while (dmask) {
		m68k_dreg (regs, movem_index1[dmask]) = (uae_s32)(uae_s16)get_word_jit (srca);
		srca += 2;
		dmask = movem_next[dmask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
	while (amask) {
		m68k_areg (regs, movem_index1[amask]) = (uae_s32)(uae_s16)get_word_jit (srca);
		srca += 2;
		amask = movem_next[amask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (6);
return 22 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMEL.W #<data>.W,(d8,PC,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4cbb_40)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = 3;
{	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (4);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	while (dmask) {
		m68k_dreg (regs, movem_index1[dmask]) = (uae_s32)(uae_s16)get_word_jit (srca);
		srca += 2;
		dmask = movem_next[dmask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
	while (amask) {
		m68k_areg (regs, movem_index1[amask]) = (uae_s32)(uae_s16)get_word_jit (srca);
		srca += 2;
		amask = movem_next[amask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
}}}}return 17 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMEL.L #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_4cd0_40)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca;
	srca = m68k_areg (regs, dstreg);
{	while (dmask) {
		m68k_dreg (regs, movem_index1[dmask]) = get_long_jit (srca);
		srca += 4;
		dmask = movem_next[dmask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
	while (amask) {
		m68k_areg (regs, movem_index1[amask]) = get_long_jit (srca);
		srca += 4;
		amask = movem_next[amask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (4);
return 14 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMEL.L #<data>.W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4cd8_40)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca;
	srca = m68k_areg (regs, dstreg);
{	while (dmask) {
		m68k_dreg (regs, movem_index1[dmask]) = get_long_jit (srca);
		srca += 4;
		dmask = movem_next[dmask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
	while (amask) {
		m68k_areg (regs, movem_index1[amask]) = get_long_jit (srca);
		srca += 4;
		amask = movem_next[amask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
	m68k_areg (regs, dstreg) = srca;
}}}	m68k_incpc (4);
return 16 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMEL.L #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4ce8_40)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca;
	srca = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	while (dmask) {
		m68k_dreg (regs, movem_index1[dmask]) = get_long_jit (srca);
		srca += 4;
		dmask = movem_next[dmask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
	while (amask) {
		m68k_areg (regs, movem_index1[amask]) = get_long_jit (srca);
		srca += 4;
		amask = movem_next[amask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (6);
return 24 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMEL.L #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4cf0_40)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca;
	m68k_incpc (4);
{	srca = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	while (dmask) {
		m68k_dreg (regs, movem_index1[dmask]) = get_long_jit (srca);
		srca += 4;
		dmask = movem_next[dmask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
	while (amask) {
		m68k_areg (regs, movem_index1[amask]) = get_long_jit (srca);
		srca += 4;
		amask = movem_next[amask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
}}}}return 19 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMEL.L #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4cf8_40)(uae_u32 opcode)
{
	int count_cycles = 0;
{	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (4);
{	while (dmask) {
		m68k_dreg (regs, movem_index1[dmask]) = get_long_jit (srca);
		srca += 4;
		dmask = movem_next[dmask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
	while (amask) {
		m68k_areg (regs, movem_index1[amask]) = get_long_jit (srca);
		srca += 4;
		amask = movem_next[amask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (6);
return 16 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMEL.L #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4cf9_40)(uae_u32 opcode)
{
	int count_cycles = 0;
{	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca;
	srca = get_dilong (4);
{	while (dmask) {
		m68k_dreg (regs, movem_index1[dmask]) = get_long_jit (srca);
		srca += 4;
		dmask = movem_next[dmask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
	while (amask) {
		m68k_areg (regs, movem_index1[amask]) = get_long_jit (srca);
		srca += 4;
		amask = movem_next[amask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (8);
return 19 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMEL.L #<data>.W,(d16,PC) */
uae_u32 REGPARAM2 CPUFUNC(op_4cfa_40)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = 2;
{	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca;
	srca = m68k_getpc () + 4;
	srca += (uae_s32)(uae_s16)get_diword (4);
{	while (dmask) {
		m68k_dreg (regs, movem_index1[dmask]) = get_long_jit (srca);
		srca += 4;
		dmask = movem_next[dmask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
	while (amask) {
		m68k_areg (regs, movem_index1[amask]) = get_long_jit (srca);
		srca += 4;
		amask = movem_next[amask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
}}}	m68k_incpc (6);
return 24 * CYCLE_UNIT / 2 + count_cycles;
}

/* MVMEL.L #<data>.W,(d8,PC,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4cfb_40)(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = 3;
{	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (4);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	while (dmask) {
		m68k_dreg (regs, movem_index1[dmask]) = get_long_jit (srca);
		srca += 4;
		dmask = movem_next[dmask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
	while (amask) {
		m68k_areg (regs, movem_index1[amask]) = get_long_jit (srca);
		srca += 4;
		amask = movem_next[amask];
		count_cycles += 4 * CYCLE_UNIT / 2;
	}
}}}}return 19 * CYCLE_UNIT / 2 + count_cycles;
}

/* TRAPQ.L #<data> */
uae_u32 REGPARAM2 CPUFUNC(op_4e40_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 15);
{{	uae_u32 src = srcreg;
	m68k_incpc (2);
	Exception_cpu(src + 32);
}}return 27 * CYCLE_UNIT / 2;
}

/* LINK.W An,#<data>.W */
uae_u32 REGPARAM2 CPUFUNC(op_4e50_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr olda;
	olda = m68k_areg (regs, 7) - 4;
	m68k_areg (regs, 7) = olda;
{	uae_s32 src = m68k_areg (regs, srcreg);
{	uae_s16 offs = get_diword (2);
	put_long_jit (olda,src);
	m68k_areg (regs, srcreg) = (m68k_areg(regs, 7));
	m68k_areg(regs, 7) += offs;
}}}}	m68k_incpc (4);
return 7 * CYCLE_UNIT / 2;
}

/* UNLK.L An */
uae_u32 REGPARAM2 CPUFUNC(op_4e58_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = m68k_areg (regs, srcreg);
	m68k_areg (regs, 7) = src;
{	uaecptr olda;
	olda = m68k_areg (regs, 7);
{	uae_s32 old = get_long_jit (olda);
	m68k_areg (regs, 7) += 4;
	m68k_areg (regs, srcreg) = (old);
}}}}	m68k_incpc (2);
return 7 * CYCLE_UNIT / 2;
}

/* MVR2USP.L An */
uae_u32 REGPARAM2 CPUFUNC(op_4e60_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uae_s32 src = m68k_areg (regs, srcreg);
	regs.usp = src;
}}}	m68k_incpc (2);
return 3 * CYCLE_UNIT / 2;
}

/* MVUSP2R.L An */
uae_u32 REGPARAM2 CPUFUNC(op_4e68_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	m68k_areg (regs, srcreg) = (regs.usp);
}}}	m68k_incpc (2);
return 3 * CYCLE_UNIT / 2;
}

/* RESET.L  */
uae_u32 REGPARAM2 CPUFUNC(op_4e70_40)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	cpureset ();
	m68k_incpc (2);
}}return 519 * CYCLE_UNIT / 2;
}

/* NOP.L  */
uae_u32 REGPARAM2 CPUFUNC(op_4e71_40)(uae_u32 opcode)
{
{}	m68k_incpc (2);
return 3 * CYCLE_UNIT / 2;
}

/* STOP.L #<data>.W */
uae_u32 REGPARAM2 CPUFUNC(op_4e72_40)(uae_u32 opcode)
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
uae_u32 REGPARAM2 CPUFUNC(op_4e73_40)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	uae_u16 newsr; uae_u32 newpc;
	for (;;) {
		uaecptr a = m68k_areg (regs, 7);
		uae_u16 sr = get_word_jit (a);
		uae_u32 pc = get_long_jit (a + 2);
		uae_u16 format = get_word_jit (a + 2 + 4);
		int frame = format >> 12;
		int offset = 8;
		newsr = sr; newpc = pc;
		if (frame == 0x0) { m68k_areg (regs, 7) += offset; break; }
		else if (frame == 0x1) { m68k_areg (regs, 7) += offset; }
		else if (frame == 0x2) { m68k_areg (regs, 7) += offset + 4; break; }
		else if (frame == 0x3) { m68k_areg (regs, 7) += offset + 4; break; }
		else if (frame == 0x4) { m68k_areg (regs, 7) += offset + 8; break; }
		else if (frame == 0x7) { m68k_areg (regs, 7) += offset + 52; break; }
		else { m68k_areg (regs, 7) += offset; Exception_cpu(14); return 4 * CYCLE_UNIT / 2; }
		regs.sr = newsr;
	MakeFromSR_T0();
}
	regs.sr = newsr;
	MakeFromSR_T0();
	if (newpc & 1) {
		exception3i (0x4E73, newpc);
		return 4 * CYCLE_UNIT / 2;
  }
	m68k_setpc (newpc);
}}return 24 * CYCLE_UNIT / 2;
}

/* RTD.L #<data>.W */
uae_u32 REGPARAM2 CPUFUNC(op_4e74_40)(uae_u32 opcode)
{
{{	uaecptr pca;
	pca = m68k_areg (regs, 7);
{	uae_s32 pc = get_long_jit (pca);
	m68k_areg (regs, 7) += 4;
{	uae_s16 offs = get_diword (2);
	m68k_areg(regs, 7) += offs;
	if (pc & 1) {
		exception3i (0x4E74, pc);
		return 4 * CYCLE_UNIT / 2;
	}
		m68k_setpc (pc);
	if(regs.t0) check_t0_trace();
}}}}return 12 * CYCLE_UNIT / 2;
}

/* RTS.L  */
uae_u32 REGPARAM2 CPUFUNC(op_4e75_40)(uae_u32 opcode)
{
{	uaecptr pc = m68k_getpc ();
	m68k_do_rts ();
	if (m68k_getpc () & 1) {
	  uaecptr faultpc = m68k_getpc ();
	  m68k_setpc (pc);
	  exception3i (0x4E75, faultpc);
		return 8 * CYCLE_UNIT / 2;
	}
	if(regs.t0) check_t0_trace();
}return 12 * CYCLE_UNIT / 2;
}

/* TRAPV.L  */
uae_u32 REGPARAM2 CPUFUNC(op_4e76_40)(uae_u32 opcode)
{
{	m68k_incpc (2);
	if (GET_VFLG ()) {
		Exception_cpu(7);
		return 4 * CYCLE_UNIT / 2;
	}
}return 5 * CYCLE_UNIT / 2;
}

/* RTR.L  */
uae_u32 REGPARAM2 CPUFUNC(op_4e77_40)(uae_u32 opcode)
{
{ uaecptr oldpc = m68k_getpc ();
	MakeSR ();
{	uaecptr sra;
	sra = m68k_areg (regs, 7);
{	uae_s16 sr = get_word_jit (sra);
	m68k_areg (regs, 7) += 2;
{	uaecptr pca;
	pca = m68k_areg (regs, 7);
{	uae_s32 pc = get_long_jit (pca);
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
	if(regs.t0) check_t0_trace();
}}}}}return 15 * CYCLE_UNIT / 2;
}

/* MOVEC2.L #<data>.W */
uae_u32 REGPARAM2 CPUFUNC(op_4e7a_40)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uae_s16 src = get_diword (2);
{	int regno = (src >> 12) & 15;
	uae_u32 *regp = regs.regs + regno;
	if (! m68k_movec2(src & 0xFFF, regp)) goto l_899;
}}}}	m68k_incpc (4);
l_899: ;
return 7 * CYCLE_UNIT / 2;
}

/* MOVE2C.L #<data>.W */
uae_u32 REGPARAM2 CPUFUNC(op_4e7b_40)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{{	uae_s16 src = get_diword (2);
{	int regno = (src >> 12) & 15;
	uae_u32 *regp = regs.regs + regno;
	if (! m68k_move2c(src & 0xFFF, regp)) goto l_900;
}}}}	m68k_incpc (4);
l_900: ;
return 13 * CYCLE_UNIT / 2;
}

/* JSR.L (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4e90_40)(uae_u32 opcode)
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
	put_long_jit (m68k_areg (regs, 7), oldpc);
}}}return 13 * CYCLE_UNIT / 2;
}

/* JSR.L (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4ea8_40)(uae_u32 opcode)
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
	put_long_jit (m68k_areg (regs, 7), oldpc);
}}}return 15 * CYCLE_UNIT / 2;
}

/* JSR.L (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4eb0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uaecptr oldpc = m68k_getpc () + 0;
	if (srca & 1) {
		exception3i (opcode, srca);
		return 4 * CYCLE_UNIT / 2;
	}
	m68k_setpc (srca);
	m68k_areg (regs, 7) -= 4;
	put_long_jit (m68k_areg (regs, 7), oldpc);
}}}}return 17 * CYCLE_UNIT / 2;
}

/* JSR.L (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4eb8_40)(uae_u32 opcode)
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
	put_long_jit (m68k_areg (regs, 7), oldpc);
}}}return 13 * CYCLE_UNIT / 2;
}

/* JSR.L (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4eb9_40)(uae_u32 opcode)
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
	put_long_jit (m68k_areg (regs, 7), oldpc);
}}}return 13 * CYCLE_UNIT / 2;
}

/* JSR.L (d16,PC) */
uae_u32 REGPARAM2 CPUFUNC(op_4eba_40)(uae_u32 opcode)
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
	put_long_jit (m68k_areg (regs, 7), oldpc);
}}}return 15 * CYCLE_UNIT / 2;
}

/* JSR.L (d8,PC,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4ebb_40)(uae_u32 opcode)
{
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uaecptr oldpc = m68k_getpc () + 0;
	if (srca & 1) {
		exception3i (opcode, srca);
		return 4 * CYCLE_UNIT / 2;
	}
	m68k_setpc (srca);
	m68k_areg (regs, 7) -= 4;
	put_long_jit (m68k_areg (regs, 7), oldpc);
}}}}return 17 * CYCLE_UNIT / 2;
}

/* JMP.L (An) */
uae_u32 REGPARAM2 CPUFUNC(op_4ed0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	if (srca & 1) {
		exception3i (opcode, srca);
		return 4 * CYCLE_UNIT / 2;
	}
	m68k_setpc (srca);
	if(regs.t0) check_t0_trace();
}}return 9 * CYCLE_UNIT / 2;
}

/* JMP.L (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4ee8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	if (srca & 1) {
		exception3i (opcode, srca);
		return 8 * CYCLE_UNIT / 2;
	}
	m68k_setpc (srca);
	if(regs.t0) check_t0_trace();
}}return 11 * CYCLE_UNIT / 2;
}

/* JMP.L (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4ef0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
	if (srca & 1) {
		exception3i (opcode, srca);
		return 4 * CYCLE_UNIT / 2;
	}
	m68k_setpc (srca);
	if(regs.t0) check_t0_trace();
}}}return 13 * CYCLE_UNIT / 2;
}

/* JMP.L (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4ef8_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	if (srca & 1) {
		exception3i (opcode, srca);
		return 8 * CYCLE_UNIT / 2;
	}
	m68k_setpc (srca);
	if(regs.t0) check_t0_trace();
}}return 9 * CYCLE_UNIT / 2;
}

/* JMP.L (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4ef9_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
	if (srca & 1) {
		exception3i (opcode, srca);
		return 12 * CYCLE_UNIT / 2;
	}
	m68k_setpc (srca);
	if(regs.t0) check_t0_trace();
}}return 9 * CYCLE_UNIT / 2;
}

/* JMP.L (d16,PC) */
uae_u32 REGPARAM2 CPUFUNC(op_4efa_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	if (srca & 1) {
		exception3i (opcode, srca);
		return 8 * CYCLE_UNIT / 2;
	}
	m68k_setpc (srca);
	if(regs.t0) check_t0_trace();
}}return 11 * CYCLE_UNIT / 2;
}

/* JMP.L (d8,PC,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4efb_40)(uae_u32 opcode)
{
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
	if (srca & 1) {
		exception3i (opcode, srca);
		return 4 * CYCLE_UNIT / 2;
	}
	m68k_setpc (srca);
	if(regs.t0) check_t0_trace();
}}}return 13 * CYCLE_UNIT / 2;
}

/* ADDQ.B #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_5000_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
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
return 3 * CYCLE_UNIT / 2;
}

/* ADDQ.B #<data>,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_5010_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
{{uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* ADDQ.B #<data>,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_5018_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
{{uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* ADDQ.B #<data>,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_5020_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

/* ADDQ.B #<data>,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_5028_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte_jit (dsta);
{{uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* ADDQ.B #<data>,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_5030_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	m68k_incpc (2);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s8 dst = get_byte_jit (dsta);
{{uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}}return 14 * CYCLE_UNIT / 2;
}

/* ADDQ.B #<data>,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_5038_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte_jit (dsta);
{{uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* ADDQ.B #<data>,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_5039_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s8 dst = get_byte_jit (dsta);
{{uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* ADDQ.W #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_5040_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
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
return 3 * CYCLE_UNIT / 2;
}

/* ADDAQ.W #<data>,An */
uae_u32 REGPARAM2 CPUFUNC(op_5048_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}	m68k_incpc (2);
return 3 * CYCLE_UNIT / 2;
}

/* ADDQ.W #<data>,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_5050_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word_jit (dsta);
{{uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* ADDQ.W #<data>,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_5058_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word_jit (dsta);
	m68k_areg (regs, dstreg) += 2;
{{uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* ADDQ.W #<data>,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_5060_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
{	uae_s16 dst = get_word_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

/* ADDQ.W #<data>,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_5068_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 dst = get_word_jit (dsta);
{{uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* ADDQ.W #<data>,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_5070_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	m68k_incpc (2);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s16 dst = get_word_jit (dsta);
{{uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}}return 14 * CYCLE_UNIT / 2;
}

/* ADDQ.W #<data>,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_5078_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 dst = get_word_jit (dsta);
{{uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* ADDQ.W #<data>,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_5079_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s16 dst = get_word_jit (dsta);
{{uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* ADDQ.L #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_5080_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
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
return 3 * CYCLE_UNIT / 2;
}

/* ADDAQ.L #<data>,An */
uae_u32 REGPARAM2 CPUFUNC(op_5088_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}	m68k_incpc (2);
return 3 * CYCLE_UNIT / 2;
}

/* ADDQ.L #<data>,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_5090_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long_jit (dsta);
{{uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* ADDQ.L #<data>,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_5098_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long_jit (dsta);
	m68k_areg (regs, dstreg) += 4;
{{uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* ADDQ.L #<data>,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_50a0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
{	uae_s32 dst = get_long_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

#endif

#ifdef PART_5
/* ADDQ.L #<data>,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_50a8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 dst = get_long_jit (dsta);
{{uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* ADDQ.L #<data>,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_50b0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	m68k_incpc (2);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s32 dst = get_long_jit (dsta);
{{uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}}return 14 * CYCLE_UNIT / 2;
}

/* ADDQ.L #<data>,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_50b8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 dst = get_long_jit (dsta);
{{uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* ADDQ.L #<data>,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_50b9_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s32 dst = get_long_jit (dsta);
{{uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (T) */
uae_u32 REGPARAM2 CPUFUNC(op_50c0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue (0) ? 0xff : 0;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* DBcc.W Dn,#<data>.W (T) */
uae_u32 REGPARAM2 CPUFUNC(op_50c8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue (0)) {
	m68k_incpc ((uae_s32)offs + 2);
			m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc () + 2 + (uae_s32)offs + 2);
		return 10 * CYCLE_UNIT / 2;
			}
			return 7 * CYCLE_UNIT / 2;
		}
	} else {
	}
	m68k_setpc (oldpc + 4);
}}}return 10 * CYCLE_UNIT / 2;
}

/* Scc.B (An) (T) */
uae_u32 REGPARAM2 CPUFUNC(op_50d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{{	int val = cctrue (0) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (T) */
uae_u32 REGPARAM2 CPUFUNC(op_50d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (0) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (T) */
uae_u32 REGPARAM2 CPUFUNC(op_50e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (0) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (T) */
uae_u32 REGPARAM2 CPUFUNC(op_50e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (0) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (T) */
uae_u32 REGPARAM2 CPUFUNC(op_50f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{{	int val = cctrue (0) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (T) */
uae_u32 REGPARAM2 CPUFUNC(op_50f8_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (0) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (T) */
uae_u32 REGPARAM2 CPUFUNC(op_50f9_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{{	int val = cctrue (0) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (6);
return 11 * CYCLE_UNIT / 2;
}

/* TRAPcc.L #<data>.W (T) */
uae_u32 REGPARAM2 CPUFUNC(op_50fa_40)(uae_u32 opcode)
{
{{	uae_s16 dummy = get_diword (2);
	if (cctrue (0)) { Exception_cpu(7); goto l_950; }
}}	m68k_incpc (4);
l_950: ;
return 10 * CYCLE_UNIT / 2;
}

/* TRAPcc.L #<data>.L (T) */
uae_u32 REGPARAM2 CPUFUNC(op_50fb_40)(uae_u32 opcode)
{
{{	uae_s32 dummy;
	dummy = get_dilong (2);
	if (cctrue (0)) { Exception_cpu(7); goto l_951; }
}}	m68k_incpc (6);
l_951: ;
return 10 * CYCLE_UNIT / 2;
}

/* TRAPcc.L  (T) */
uae_u32 REGPARAM2 CPUFUNC(op_50fc_40)(uae_u32 opcode)
{
{	if (cctrue (0)) { Exception_cpu(7); goto l_952; }
}	m68k_incpc (2);
l_952: ;
return 10 * CYCLE_UNIT / 2;
}

/* SUBQ.B #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_5100_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
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
return 3 * CYCLE_UNIT / 2;
}

/* SUBQ.B #<data>,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_5110_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* SUBQ.B #<data>,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_5118_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* SUBQ.B #<data>,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_5120_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

/* SUBQ.B #<data>,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_5128_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte_jit (dsta);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* SUBQ.B #<data>,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_5130_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	m68k_incpc (2);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s8 dst = get_byte_jit (dsta);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}}return 14 * CYCLE_UNIT / 2;
}

/* SUBQ.B #<data>,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_5138_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte_jit (dsta);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* SUBQ.B #<data>,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_5139_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s8 dst = get_byte_jit (dsta);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* SUBQ.W #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_5140_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
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
return 3 * CYCLE_UNIT / 2;
}

/* SUBAQ.W #<data>,An */
uae_u32 REGPARAM2 CPUFUNC(op_5148_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}	m68k_incpc (2);
return 3 * CYCLE_UNIT / 2;
}

/* SUBQ.W #<data>,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_5150_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word_jit (dsta);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* SUBQ.W #<data>,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_5158_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word_jit (dsta);
	m68k_areg (regs, dstreg) += 2;
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* SUBQ.W #<data>,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_5160_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
{	uae_s16 dst = get_word_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

/* SUBQ.W #<data>,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_5168_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 dst = get_word_jit (dsta);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* SUBQ.W #<data>,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_5170_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	m68k_incpc (2);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s16 dst = get_word_jit (dsta);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}}return 14 * CYCLE_UNIT / 2;
}

/* SUBQ.W #<data>,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_5178_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 dst = get_word_jit (dsta);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* SUBQ.W #<data>,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_5179_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s16 dst = get_word_jit (dsta);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* SUBQ.L #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_5180_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
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
return 3 * CYCLE_UNIT / 2;
}

/* SUBAQ.L #<data>,An */
uae_u32 REGPARAM2 CPUFUNC(op_5188_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}	m68k_incpc (2);
return 3 * CYCLE_UNIT / 2;
}

/* SUBQ.L #<data>,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_5190_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long_jit (dsta);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* SUBQ.L #<data>,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_5198_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long_jit (dsta);
	m68k_areg (regs, dstreg) += 4;
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* SUBQ.L #<data>,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_51a0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
{	uae_s32 dst = get_long_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

/* SUBQ.L #<data>,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_51a8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 dst = get_long_jit (dsta);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* SUBQ.L #<data>,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_51b0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	m68k_incpc (2);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s32 dst = get_long_jit (dsta);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}}return 14 * CYCLE_UNIT / 2;
}

/* SUBQ.L #<data>,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_51b8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 dst = get_long_jit (dsta);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* SUBQ.L #<data>,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_51b9_40)(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s32 dst = get_long_jit (dsta);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (F) */
uae_u32 REGPARAM2 CPUFUNC(op_51c0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue (1) ? 0xff : 0;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* DBcc.W Dn,#<data>.W (F) */
uae_u32 REGPARAM2 CPUFUNC(op_51c8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue (1)) {
	m68k_incpc ((uae_s32)offs + 2);
			m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc () + 2 + (uae_s32)offs + 2);
		return 10 * CYCLE_UNIT / 2;
			}
			return 7 * CYCLE_UNIT / 2;
		}
	} else {
	}
	m68k_setpc (oldpc + 4);
}}}return 10 * CYCLE_UNIT / 2;
}

/* Scc.B (An) (F) */
uae_u32 REGPARAM2 CPUFUNC(op_51d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{{	int val = cctrue (1) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (F) */
uae_u32 REGPARAM2 CPUFUNC(op_51d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (1) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (F) */
uae_u32 REGPARAM2 CPUFUNC(op_51e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (1) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (F) */
uae_u32 REGPARAM2 CPUFUNC(op_51e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (1) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (F) */
uae_u32 REGPARAM2 CPUFUNC(op_51f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{{	int val = cctrue (1) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (F) */
uae_u32 REGPARAM2 CPUFUNC(op_51f8_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (1) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (F) */
uae_u32 REGPARAM2 CPUFUNC(op_51f9_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{{	int val = cctrue (1) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (6);
return 11 * CYCLE_UNIT / 2;
}

/* TRAPcc.L #<data>.W (F) */
uae_u32 REGPARAM2 CPUFUNC(op_51fa_40)(uae_u32 opcode)
{
{{	uae_s16 dummy = get_diword (2);
	if (cctrue (1)) { Exception_cpu(7); goto l_988; }
}}	m68k_incpc (4);
l_988: ;
return 10 * CYCLE_UNIT / 2;
}

/* TRAPcc.L #<data>.L (F) */
uae_u32 REGPARAM2 CPUFUNC(op_51fb_40)(uae_u32 opcode)
{
{{	uae_s32 dummy;
	dummy = get_dilong (2);
	if (cctrue (1)) { Exception_cpu(7); goto l_989; }
}}	m68k_incpc (6);
l_989: ;
return 10 * CYCLE_UNIT / 2;
}

/* TRAPcc.L  (F) */
uae_u32 REGPARAM2 CPUFUNC(op_51fc_40)(uae_u32 opcode)
{
{	if (cctrue (1)) { Exception_cpu(7); goto l_990; }
}	m68k_incpc (2);
l_990: ;
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (HI) */
uae_u32 REGPARAM2 CPUFUNC(op_52c0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue (2) ? 0xff : 0;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* DBcc.W Dn,#<data>.W (HI) */
uae_u32 REGPARAM2 CPUFUNC(op_52c8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue (2)) {
	m68k_incpc ((uae_s32)offs + 2);
			m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc () + 2 + (uae_s32)offs + 2);
		return 10 * CYCLE_UNIT / 2;
			}
			return 7 * CYCLE_UNIT / 2;
		}
	} else {
	}
	m68k_setpc (oldpc + 4);
}}}return 10 * CYCLE_UNIT / 2;
}

/* Scc.B (An) (HI) */
uae_u32 REGPARAM2 CPUFUNC(op_52d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{{	int val = cctrue (2) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (HI) */
uae_u32 REGPARAM2 CPUFUNC(op_52d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (2) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (HI) */
uae_u32 REGPARAM2 CPUFUNC(op_52e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (2) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (HI) */
uae_u32 REGPARAM2 CPUFUNC(op_52e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (2) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (HI) */
uae_u32 REGPARAM2 CPUFUNC(op_52f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{{	int val = cctrue (2) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (HI) */
uae_u32 REGPARAM2 CPUFUNC(op_52f8_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (2) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (HI) */
uae_u32 REGPARAM2 CPUFUNC(op_52f9_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{{	int val = cctrue (2) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (6);
return 11 * CYCLE_UNIT / 2;
}

/* TRAPcc.L #<data>.W (HI) */
uae_u32 REGPARAM2 CPUFUNC(op_52fa_40)(uae_u32 opcode)
{
{{	uae_s16 dummy = get_diword (2);
	if (cctrue (2)) { Exception_cpu(7); goto l_1000; }
}}	m68k_incpc (4);
l_1000: ;
return 10 * CYCLE_UNIT / 2;
}

/* TRAPcc.L #<data>.L (HI) */
uae_u32 REGPARAM2 CPUFUNC(op_52fb_40)(uae_u32 opcode)
{
{{	uae_s32 dummy;
	dummy = get_dilong (2);
	if (cctrue (2)) { Exception_cpu(7); goto l_1001; }
}}	m68k_incpc (6);
l_1001: ;
return 10 * CYCLE_UNIT / 2;
}

/* TRAPcc.L  (HI) */
uae_u32 REGPARAM2 CPUFUNC(op_52fc_40)(uae_u32 opcode)
{
{	if (cctrue (2)) { Exception_cpu(7); goto l_1002; }
}	m68k_incpc (2);
l_1002: ;
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (LS) */
uae_u32 REGPARAM2 CPUFUNC(op_53c0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue (3) ? 0xff : 0;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* DBcc.W Dn,#<data>.W (LS) */
uae_u32 REGPARAM2 CPUFUNC(op_53c8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue (3)) {
	m68k_incpc ((uae_s32)offs + 2);
			m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc () + 2 + (uae_s32)offs + 2);
		return 10 * CYCLE_UNIT / 2;
			}
			return 7 * CYCLE_UNIT / 2;
		}
	} else {
	}
	m68k_setpc (oldpc + 4);
}}}return 10 * CYCLE_UNIT / 2;
}

/* Scc.B (An) (LS) */
uae_u32 REGPARAM2 CPUFUNC(op_53d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{{	int val = cctrue (3) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (LS) */
uae_u32 REGPARAM2 CPUFUNC(op_53d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (3) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (LS) */
uae_u32 REGPARAM2 CPUFUNC(op_53e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (3) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (LS) */
uae_u32 REGPARAM2 CPUFUNC(op_53e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (3) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (LS) */
uae_u32 REGPARAM2 CPUFUNC(op_53f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{{	int val = cctrue (3) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (LS) */
uae_u32 REGPARAM2 CPUFUNC(op_53f8_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (3) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (LS) */
uae_u32 REGPARAM2 CPUFUNC(op_53f9_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{{	int val = cctrue (3) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (6);
return 11 * CYCLE_UNIT / 2;
}

/* TRAPcc.L #<data>.W (LS) */
uae_u32 REGPARAM2 CPUFUNC(op_53fa_40)(uae_u32 opcode)
{
{{	uae_s16 dummy = get_diword (2);
	if (cctrue (3)) { Exception_cpu(7); goto l_1012; }
}}	m68k_incpc (4);
l_1012: ;
return 10 * CYCLE_UNIT / 2;
}

/* TRAPcc.L #<data>.L (LS) */
uae_u32 REGPARAM2 CPUFUNC(op_53fb_40)(uae_u32 opcode)
{
{{	uae_s32 dummy;
	dummy = get_dilong (2);
	if (cctrue (3)) { Exception_cpu(7); goto l_1013; }
}}	m68k_incpc (6);
l_1013: ;
return 10 * CYCLE_UNIT / 2;
}

/* TRAPcc.L  (LS) */
uae_u32 REGPARAM2 CPUFUNC(op_53fc_40)(uae_u32 opcode)
{
{	if (cctrue (3)) { Exception_cpu(7); goto l_1014; }
}	m68k_incpc (2);
l_1014: ;
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (CC) */
uae_u32 REGPARAM2 CPUFUNC(op_54c0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue (4) ? 0xff : 0;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* DBcc.W Dn,#<data>.W (CC) */
uae_u32 REGPARAM2 CPUFUNC(op_54c8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue (4)) {
	m68k_incpc ((uae_s32)offs + 2);
			m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc () + 2 + (uae_s32)offs + 2);
		return 10 * CYCLE_UNIT / 2;
			}
			return 7 * CYCLE_UNIT / 2;
		}
	} else {
	}
	m68k_setpc (oldpc + 4);
}}}return 10 * CYCLE_UNIT / 2;
}

/* Scc.B (An) (CC) */
uae_u32 REGPARAM2 CPUFUNC(op_54d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{{	int val = cctrue (4) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (CC) */
uae_u32 REGPARAM2 CPUFUNC(op_54d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (4) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (CC) */
uae_u32 REGPARAM2 CPUFUNC(op_54e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (4) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (CC) */
uae_u32 REGPARAM2 CPUFUNC(op_54e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (4) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (CC) */
uae_u32 REGPARAM2 CPUFUNC(op_54f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{{	int val = cctrue (4) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (CC) */
uae_u32 REGPARAM2 CPUFUNC(op_54f8_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (4) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (CC) */
uae_u32 REGPARAM2 CPUFUNC(op_54f9_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{{	int val = cctrue (4) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (6);
return 11 * CYCLE_UNIT / 2;
}

/* TRAPcc.L #<data>.W (CC) */
uae_u32 REGPARAM2 CPUFUNC(op_54fa_40)(uae_u32 opcode)
{
{{	uae_s16 dummy = get_diword (2);
	if (cctrue (4)) { Exception_cpu(7); goto l_1024; }
}}	m68k_incpc (4);
l_1024: ;
return 10 * CYCLE_UNIT / 2;
}

/* TRAPcc.L #<data>.L (CC) */
uae_u32 REGPARAM2 CPUFUNC(op_54fb_40)(uae_u32 opcode)
{
{{	uae_s32 dummy;
	dummy = get_dilong (2);
	if (cctrue (4)) { Exception_cpu(7); goto l_1025; }
}}	m68k_incpc (6);
l_1025: ;
return 10 * CYCLE_UNIT / 2;
}

/* TRAPcc.L  (CC) */
uae_u32 REGPARAM2 CPUFUNC(op_54fc_40)(uae_u32 opcode)
{
{	if (cctrue (4)) { Exception_cpu(7); goto l_1026; }
}	m68k_incpc (2);
l_1026: ;
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (CS) */
uae_u32 REGPARAM2 CPUFUNC(op_55c0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue (5) ? 0xff : 0;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* DBcc.W Dn,#<data>.W (CS) */
uae_u32 REGPARAM2 CPUFUNC(op_55c8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue (5)) {
	m68k_incpc ((uae_s32)offs + 2);
			m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc () + 2 + (uae_s32)offs + 2);
		return 10 * CYCLE_UNIT / 2;
			}
			return 7 * CYCLE_UNIT / 2;
		}
	} else {
	}
	m68k_setpc (oldpc + 4);
}}}return 10 * CYCLE_UNIT / 2;
}

/* Scc.B (An) (CS) */
uae_u32 REGPARAM2 CPUFUNC(op_55d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{{	int val = cctrue (5) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (CS) */
uae_u32 REGPARAM2 CPUFUNC(op_55d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (5) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (CS) */
uae_u32 REGPARAM2 CPUFUNC(op_55e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (5) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (CS) */
uae_u32 REGPARAM2 CPUFUNC(op_55e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (5) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (CS) */
uae_u32 REGPARAM2 CPUFUNC(op_55f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{{	int val = cctrue (5) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (CS) */
uae_u32 REGPARAM2 CPUFUNC(op_55f8_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (5) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (CS) */
uae_u32 REGPARAM2 CPUFUNC(op_55f9_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{{	int val = cctrue (5) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (6);
return 11 * CYCLE_UNIT / 2;
}

/* TRAPcc.L #<data>.W (CS) */
uae_u32 REGPARAM2 CPUFUNC(op_55fa_40)(uae_u32 opcode)
{
{{	uae_s16 dummy = get_diword (2);
	if (cctrue (5)) { Exception_cpu(7); goto l_1036; }
}}	m68k_incpc (4);
l_1036: ;
return 10 * CYCLE_UNIT / 2;
}

/* TRAPcc.L #<data>.L (CS) */
uae_u32 REGPARAM2 CPUFUNC(op_55fb_40)(uae_u32 opcode)
{
{{	uae_s32 dummy;
	dummy = get_dilong (2);
	if (cctrue (5)) { Exception_cpu(7); goto l_1037; }
}}	m68k_incpc (6);
l_1037: ;
return 10 * CYCLE_UNIT / 2;
}

/* TRAPcc.L  (CS) */
uae_u32 REGPARAM2 CPUFUNC(op_55fc_40)(uae_u32 opcode)
{
{	if (cctrue (5)) { Exception_cpu(7); goto l_1038; }
}	m68k_incpc (2);
l_1038: ;
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (NE) */
uae_u32 REGPARAM2 CPUFUNC(op_56c0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue (6) ? 0xff : 0;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* DBcc.W Dn,#<data>.W (NE) */
uae_u32 REGPARAM2 CPUFUNC(op_56c8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue (6)) {
	m68k_incpc ((uae_s32)offs + 2);
			m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc () + 2 + (uae_s32)offs + 2);
		return 10 * CYCLE_UNIT / 2;
			}
			return 7 * CYCLE_UNIT / 2;
		}
	} else {
	}
	m68k_setpc (oldpc + 4);
}}}return 10 * CYCLE_UNIT / 2;
}

/* Scc.B (An) (NE) */
uae_u32 REGPARAM2 CPUFUNC(op_56d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{{	int val = cctrue (6) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (NE) */
uae_u32 REGPARAM2 CPUFUNC(op_56d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (6) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (NE) */
uae_u32 REGPARAM2 CPUFUNC(op_56e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (6) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (NE) */
uae_u32 REGPARAM2 CPUFUNC(op_56e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (6) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (NE) */
uae_u32 REGPARAM2 CPUFUNC(op_56f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{{	int val = cctrue (6) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (NE) */
uae_u32 REGPARAM2 CPUFUNC(op_56f8_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (6) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (NE) */
uae_u32 REGPARAM2 CPUFUNC(op_56f9_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{{	int val = cctrue (6) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (6);
return 11 * CYCLE_UNIT / 2;
}

/* TRAPcc.L #<data>.W (NE) */
uae_u32 REGPARAM2 CPUFUNC(op_56fa_40)(uae_u32 opcode)
{
{{	uae_s16 dummy = get_diword (2);
	if (cctrue (6)) { Exception_cpu(7); goto l_1048; }
}}	m68k_incpc (4);
l_1048: ;
return 10 * CYCLE_UNIT / 2;
}

/* TRAPcc.L #<data>.L (NE) */
uae_u32 REGPARAM2 CPUFUNC(op_56fb_40)(uae_u32 opcode)
{
{{	uae_s32 dummy;
	dummy = get_dilong (2);
	if (cctrue (6)) { Exception_cpu(7); goto l_1049; }
}}	m68k_incpc (6);
l_1049: ;
return 10 * CYCLE_UNIT / 2;
}

/* TRAPcc.L  (NE) */
uae_u32 REGPARAM2 CPUFUNC(op_56fc_40)(uae_u32 opcode)
{
{	if (cctrue (6)) { Exception_cpu(7); goto l_1050; }
}	m68k_incpc (2);
l_1050: ;
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (EQ) */
uae_u32 REGPARAM2 CPUFUNC(op_57c0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue (7) ? 0xff : 0;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* DBcc.W Dn,#<data>.W (EQ) */
uae_u32 REGPARAM2 CPUFUNC(op_57c8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue (7)) {
	m68k_incpc ((uae_s32)offs + 2);
			m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc () + 2 + (uae_s32)offs + 2);
		return 10 * CYCLE_UNIT / 2;
			}
			return 7 * CYCLE_UNIT / 2;
		}
	} else {
	}
	m68k_setpc (oldpc + 4);
}}}return 10 * CYCLE_UNIT / 2;
}

/* Scc.B (An) (EQ) */
uae_u32 REGPARAM2 CPUFUNC(op_57d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{{	int val = cctrue (7) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (EQ) */
uae_u32 REGPARAM2 CPUFUNC(op_57d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (7) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (EQ) */
uae_u32 REGPARAM2 CPUFUNC(op_57e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (7) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (EQ) */
uae_u32 REGPARAM2 CPUFUNC(op_57e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (7) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (EQ) */
uae_u32 REGPARAM2 CPUFUNC(op_57f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{{	int val = cctrue (7) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (EQ) */
uae_u32 REGPARAM2 CPUFUNC(op_57f8_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (7) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (EQ) */
uae_u32 REGPARAM2 CPUFUNC(op_57f9_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{{	int val = cctrue (7) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (6);
return 11 * CYCLE_UNIT / 2;
}

/* TRAPcc.L #<data>.W (EQ) */
uae_u32 REGPARAM2 CPUFUNC(op_57fa_40)(uae_u32 opcode)
{
{{	uae_s16 dummy = get_diword (2);
	if (cctrue (7)) { Exception_cpu(7); goto l_1060; }
}}	m68k_incpc (4);
l_1060: ;
return 10 * CYCLE_UNIT / 2;
}

/* TRAPcc.L #<data>.L (EQ) */
uae_u32 REGPARAM2 CPUFUNC(op_57fb_40)(uae_u32 opcode)
{
{{	uae_s32 dummy;
	dummy = get_dilong (2);
	if (cctrue (7)) { Exception_cpu(7); goto l_1061; }
}}	m68k_incpc (6);
l_1061: ;
return 10 * CYCLE_UNIT / 2;
}

/* TRAPcc.L  (EQ) */
uae_u32 REGPARAM2 CPUFUNC(op_57fc_40)(uae_u32 opcode)
{
{	if (cctrue (7)) { Exception_cpu(7); goto l_1062; }
}	m68k_incpc (2);
l_1062: ;
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (VC) */
uae_u32 REGPARAM2 CPUFUNC(op_58c0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue (8) ? 0xff : 0;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* DBcc.W Dn,#<data>.W (VC) */
uae_u32 REGPARAM2 CPUFUNC(op_58c8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue (8)) {
	m68k_incpc ((uae_s32)offs + 2);
			m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc () + 2 + (uae_s32)offs + 2);
		return 10 * CYCLE_UNIT / 2;
			}
			return 7 * CYCLE_UNIT / 2;
		}
	} else {
	}
	m68k_setpc (oldpc + 4);
}}}return 10 * CYCLE_UNIT / 2;
}

/* Scc.B (An) (VC) */
uae_u32 REGPARAM2 CPUFUNC(op_58d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{{	int val = cctrue (8) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (VC) */
uae_u32 REGPARAM2 CPUFUNC(op_58d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (8) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (VC) */
uae_u32 REGPARAM2 CPUFUNC(op_58e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (8) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (VC) */
uae_u32 REGPARAM2 CPUFUNC(op_58e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (8) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (VC) */
uae_u32 REGPARAM2 CPUFUNC(op_58f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{{	int val = cctrue (8) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (VC) */
uae_u32 REGPARAM2 CPUFUNC(op_58f8_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (8) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (VC) */
uae_u32 REGPARAM2 CPUFUNC(op_58f9_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{{	int val = cctrue (8) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (6);
return 11 * CYCLE_UNIT / 2;
}

/* TRAPcc.L #<data>.W (VC) */
uae_u32 REGPARAM2 CPUFUNC(op_58fa_40)(uae_u32 opcode)
{
{{	uae_s16 dummy = get_diword (2);
	if (cctrue (8)) { Exception_cpu(7); goto l_1072; }
}}	m68k_incpc (4);
l_1072: ;
return 10 * CYCLE_UNIT / 2;
}

/* TRAPcc.L #<data>.L (VC) */
uae_u32 REGPARAM2 CPUFUNC(op_58fb_40)(uae_u32 opcode)
{
{{	uae_s32 dummy;
	dummy = get_dilong (2);
	if (cctrue (8)) { Exception_cpu(7); goto l_1073; }
}}	m68k_incpc (6);
l_1073: ;
return 10 * CYCLE_UNIT / 2;
}

/* TRAPcc.L  (VC) */
uae_u32 REGPARAM2 CPUFUNC(op_58fc_40)(uae_u32 opcode)
{
{	if (cctrue (8)) { Exception_cpu(7); goto l_1074; }
}	m68k_incpc (2);
l_1074: ;
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (VS) */
uae_u32 REGPARAM2 CPUFUNC(op_59c0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue (9) ? 0xff : 0;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* DBcc.W Dn,#<data>.W (VS) */
uae_u32 REGPARAM2 CPUFUNC(op_59c8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue (9)) {
	m68k_incpc ((uae_s32)offs + 2);
			m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc () + 2 + (uae_s32)offs + 2);
		return 10 * CYCLE_UNIT / 2;
			}
			return 7 * CYCLE_UNIT / 2;
		}
	} else {
	}
	m68k_setpc (oldpc + 4);
}}}return 10 * CYCLE_UNIT / 2;
}

/* Scc.B (An) (VS) */
uae_u32 REGPARAM2 CPUFUNC(op_59d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{{	int val = cctrue (9) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (VS) */
uae_u32 REGPARAM2 CPUFUNC(op_59d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (9) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (VS) */
uae_u32 REGPARAM2 CPUFUNC(op_59e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (9) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (VS) */
uae_u32 REGPARAM2 CPUFUNC(op_59e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (9) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (VS) */
uae_u32 REGPARAM2 CPUFUNC(op_59f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{{	int val = cctrue (9) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (VS) */
uae_u32 REGPARAM2 CPUFUNC(op_59f8_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (9) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (VS) */
uae_u32 REGPARAM2 CPUFUNC(op_59f9_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{{	int val = cctrue (9) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (6);
return 11 * CYCLE_UNIT / 2;
}

/* TRAPcc.L #<data>.W (VS) */
uae_u32 REGPARAM2 CPUFUNC(op_59fa_40)(uae_u32 opcode)
{
{{	uae_s16 dummy = get_diword (2);
	if (cctrue (9)) { Exception_cpu(7); goto l_1084; }
}}	m68k_incpc (4);
l_1084: ;
return 10 * CYCLE_UNIT / 2;
}

/* TRAPcc.L #<data>.L (VS) */
uae_u32 REGPARAM2 CPUFUNC(op_59fb_40)(uae_u32 opcode)
{
{{	uae_s32 dummy;
	dummy = get_dilong (2);
	if (cctrue (9)) { Exception_cpu(7); goto l_1085; }
}}	m68k_incpc (6);
l_1085: ;
return 10 * CYCLE_UNIT / 2;
}

/* TRAPcc.L  (VS) */
uae_u32 REGPARAM2 CPUFUNC(op_59fc_40)(uae_u32 opcode)
{
{	if (cctrue (9)) { Exception_cpu(7); goto l_1086; }
}	m68k_incpc (2);
l_1086: ;
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (PL) */
uae_u32 REGPARAM2 CPUFUNC(op_5ac0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue (10) ? 0xff : 0;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* DBcc.W Dn,#<data>.W (PL) */
uae_u32 REGPARAM2 CPUFUNC(op_5ac8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue (10)) {
	m68k_incpc ((uae_s32)offs + 2);
			m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc () + 2 + (uae_s32)offs + 2);
		return 10 * CYCLE_UNIT / 2;
			}
			return 7 * CYCLE_UNIT / 2;
		}
	} else {
	}
	m68k_setpc (oldpc + 4);
}}}return 10 * CYCLE_UNIT / 2;
}

/* Scc.B (An) (PL) */
uae_u32 REGPARAM2 CPUFUNC(op_5ad0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{{	int val = cctrue (10) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (PL) */
uae_u32 REGPARAM2 CPUFUNC(op_5ad8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (10) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (PL) */
uae_u32 REGPARAM2 CPUFUNC(op_5ae0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (10) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (PL) */
uae_u32 REGPARAM2 CPUFUNC(op_5ae8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (10) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (PL) */
uae_u32 REGPARAM2 CPUFUNC(op_5af0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{{	int val = cctrue (10) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (PL) */
uae_u32 REGPARAM2 CPUFUNC(op_5af8_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (10) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (PL) */
uae_u32 REGPARAM2 CPUFUNC(op_5af9_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{{	int val = cctrue (10) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (6);
return 11 * CYCLE_UNIT / 2;
}

/* TRAPcc.L #<data>.W (PL) */
uae_u32 REGPARAM2 CPUFUNC(op_5afa_40)(uae_u32 opcode)
{
{{	uae_s16 dummy = get_diword (2);
	if (cctrue (10)) { Exception_cpu(7); goto l_1096; }
}}	m68k_incpc (4);
l_1096: ;
return 10 * CYCLE_UNIT / 2;
}

/* TRAPcc.L #<data>.L (PL) */
uae_u32 REGPARAM2 CPUFUNC(op_5afb_40)(uae_u32 opcode)
{
{{	uae_s32 dummy;
	dummy = get_dilong (2);
	if (cctrue (10)) { Exception_cpu(7); goto l_1097; }
}}	m68k_incpc (6);
l_1097: ;
return 10 * CYCLE_UNIT / 2;
}

/* TRAPcc.L  (PL) */
uae_u32 REGPARAM2 CPUFUNC(op_5afc_40)(uae_u32 opcode)
{
{	if (cctrue (10)) { Exception_cpu(7); goto l_1098; }
}	m68k_incpc (2);
l_1098: ;
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (MI) */
uae_u32 REGPARAM2 CPUFUNC(op_5bc0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue (11) ? 0xff : 0;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* DBcc.W Dn,#<data>.W (MI) */
uae_u32 REGPARAM2 CPUFUNC(op_5bc8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue (11)) {
	m68k_incpc ((uae_s32)offs + 2);
			m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc () + 2 + (uae_s32)offs + 2);
		return 10 * CYCLE_UNIT / 2;
			}
			return 7 * CYCLE_UNIT / 2;
		}
	} else {
	}
	m68k_setpc (oldpc + 4);
}}}return 10 * CYCLE_UNIT / 2;
}

/* Scc.B (An) (MI) */
uae_u32 REGPARAM2 CPUFUNC(op_5bd0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{{	int val = cctrue (11) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (MI) */
uae_u32 REGPARAM2 CPUFUNC(op_5bd8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (11) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (MI) */
uae_u32 REGPARAM2 CPUFUNC(op_5be0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (11) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (MI) */
uae_u32 REGPARAM2 CPUFUNC(op_5be8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (11) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (MI) */
uae_u32 REGPARAM2 CPUFUNC(op_5bf0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{{	int val = cctrue (11) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (MI) */
uae_u32 REGPARAM2 CPUFUNC(op_5bf8_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (11) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (MI) */
uae_u32 REGPARAM2 CPUFUNC(op_5bf9_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{{	int val = cctrue (11) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (6);
return 11 * CYCLE_UNIT / 2;
}

/* TRAPcc.L #<data>.W (MI) */
uae_u32 REGPARAM2 CPUFUNC(op_5bfa_40)(uae_u32 opcode)
{
{{	uae_s16 dummy = get_diword (2);
	if (cctrue (11)) { Exception_cpu(7); goto l_1108; }
}}	m68k_incpc (4);
l_1108: ;
return 10 * CYCLE_UNIT / 2;
}

/* TRAPcc.L #<data>.L (MI) */
uae_u32 REGPARAM2 CPUFUNC(op_5bfb_40)(uae_u32 opcode)
{
{{	uae_s32 dummy;
	dummy = get_dilong (2);
	if (cctrue (11)) { Exception_cpu(7); goto l_1109; }
}}	m68k_incpc (6);
l_1109: ;
return 10 * CYCLE_UNIT / 2;
}

/* TRAPcc.L  (MI) */
uae_u32 REGPARAM2 CPUFUNC(op_5bfc_40)(uae_u32 opcode)
{
{	if (cctrue (11)) { Exception_cpu(7); goto l_1110; }
}	m68k_incpc (2);
l_1110: ;
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (GE) */
uae_u32 REGPARAM2 CPUFUNC(op_5cc0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue (12) ? 0xff : 0;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* DBcc.W Dn,#<data>.W (GE) */
uae_u32 REGPARAM2 CPUFUNC(op_5cc8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue (12)) {
	m68k_incpc ((uae_s32)offs + 2);
			m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc () + 2 + (uae_s32)offs + 2);
		return 10 * CYCLE_UNIT / 2;
			}
			return 7 * CYCLE_UNIT / 2;
		}
	} else {
	}
	m68k_setpc (oldpc + 4);
}}}return 10 * CYCLE_UNIT / 2;
}

/* Scc.B (An) (GE) */
uae_u32 REGPARAM2 CPUFUNC(op_5cd0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{{	int val = cctrue (12) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (GE) */
uae_u32 REGPARAM2 CPUFUNC(op_5cd8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (12) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (GE) */
uae_u32 REGPARAM2 CPUFUNC(op_5ce0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (12) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (GE) */
uae_u32 REGPARAM2 CPUFUNC(op_5ce8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (12) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (GE) */
uae_u32 REGPARAM2 CPUFUNC(op_5cf0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{{	int val = cctrue (12) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (GE) */
uae_u32 REGPARAM2 CPUFUNC(op_5cf8_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (12) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (GE) */
uae_u32 REGPARAM2 CPUFUNC(op_5cf9_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{{	int val = cctrue (12) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (6);
return 11 * CYCLE_UNIT / 2;
}

/* TRAPcc.L #<data>.W (GE) */
uae_u32 REGPARAM2 CPUFUNC(op_5cfa_40)(uae_u32 opcode)
{
{{	uae_s16 dummy = get_diword (2);
	if (cctrue (12)) { Exception_cpu(7); goto l_1120; }
}}	m68k_incpc (4);
l_1120: ;
return 10 * CYCLE_UNIT / 2;
}

/* TRAPcc.L #<data>.L (GE) */
uae_u32 REGPARAM2 CPUFUNC(op_5cfb_40)(uae_u32 opcode)
{
{{	uae_s32 dummy;
	dummy = get_dilong (2);
	if (cctrue (12)) { Exception_cpu(7); goto l_1121; }
}}	m68k_incpc (6);
l_1121: ;
return 10 * CYCLE_UNIT / 2;
}

/* TRAPcc.L  (GE) */
uae_u32 REGPARAM2 CPUFUNC(op_5cfc_40)(uae_u32 opcode)
{
{	if (cctrue (12)) { Exception_cpu(7); goto l_1122; }
}	m68k_incpc (2);
l_1122: ;
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (LT) */
uae_u32 REGPARAM2 CPUFUNC(op_5dc0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue (13) ? 0xff : 0;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* DBcc.W Dn,#<data>.W (LT) */
uae_u32 REGPARAM2 CPUFUNC(op_5dc8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue (13)) {
	m68k_incpc ((uae_s32)offs + 2);
			m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc () + 2 + (uae_s32)offs + 2);
		return 10 * CYCLE_UNIT / 2;
			}
			return 7 * CYCLE_UNIT / 2;
		}
	} else {
	}
	m68k_setpc (oldpc + 4);
}}}return 10 * CYCLE_UNIT / 2;
}

/* Scc.B (An) (LT) */
uae_u32 REGPARAM2 CPUFUNC(op_5dd0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{{	int val = cctrue (13) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (LT) */
uae_u32 REGPARAM2 CPUFUNC(op_5dd8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (13) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (LT) */
uae_u32 REGPARAM2 CPUFUNC(op_5de0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (13) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (LT) */
uae_u32 REGPARAM2 CPUFUNC(op_5de8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (13) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (LT) */
uae_u32 REGPARAM2 CPUFUNC(op_5df0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{{	int val = cctrue (13) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (LT) */
uae_u32 REGPARAM2 CPUFUNC(op_5df8_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (13) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (LT) */
uae_u32 REGPARAM2 CPUFUNC(op_5df9_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{{	int val = cctrue (13) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (6);
return 11 * CYCLE_UNIT / 2;
}

/* TRAPcc.L #<data>.W (LT) */
uae_u32 REGPARAM2 CPUFUNC(op_5dfa_40)(uae_u32 opcode)
{
{{	uae_s16 dummy = get_diword (2);
	if (cctrue (13)) { Exception_cpu(7); goto l_1132; }
}}	m68k_incpc (4);
l_1132: ;
return 10 * CYCLE_UNIT / 2;
}

/* TRAPcc.L #<data>.L (LT) */
uae_u32 REGPARAM2 CPUFUNC(op_5dfb_40)(uae_u32 opcode)
{
{{	uae_s32 dummy;
	dummy = get_dilong (2);
	if (cctrue (13)) { Exception_cpu(7); goto l_1133; }
}}	m68k_incpc (6);
l_1133: ;
return 10 * CYCLE_UNIT / 2;
}

/* TRAPcc.L  (LT) */
uae_u32 REGPARAM2 CPUFUNC(op_5dfc_40)(uae_u32 opcode)
{
{	if (cctrue (13)) { Exception_cpu(7); goto l_1134; }
}	m68k_incpc (2);
l_1134: ;
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (GT) */
uae_u32 REGPARAM2 CPUFUNC(op_5ec0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue (14) ? 0xff : 0;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* DBcc.W Dn,#<data>.W (GT) */
uae_u32 REGPARAM2 CPUFUNC(op_5ec8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue (14)) {
	m68k_incpc ((uae_s32)offs + 2);
			m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc () + 2 + (uae_s32)offs + 2);
		return 10 * CYCLE_UNIT / 2;
			}
			return 7 * CYCLE_UNIT / 2;
		}
	} else {
	}
	m68k_setpc (oldpc + 4);
}}}return 10 * CYCLE_UNIT / 2;
}

/* Scc.B (An) (GT) */
uae_u32 REGPARAM2 CPUFUNC(op_5ed0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{{	int val = cctrue (14) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (GT) */
uae_u32 REGPARAM2 CPUFUNC(op_5ed8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (14) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (GT) */
uae_u32 REGPARAM2 CPUFUNC(op_5ee0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (14) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (GT) */
uae_u32 REGPARAM2 CPUFUNC(op_5ee8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (14) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (GT) */
uae_u32 REGPARAM2 CPUFUNC(op_5ef0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{{	int val = cctrue (14) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (GT) */
uae_u32 REGPARAM2 CPUFUNC(op_5ef8_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (14) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (GT) */
uae_u32 REGPARAM2 CPUFUNC(op_5ef9_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{{	int val = cctrue (14) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (6);
return 11 * CYCLE_UNIT / 2;
}

/* TRAPcc.L #<data>.W (GT) */
uae_u32 REGPARAM2 CPUFUNC(op_5efa_40)(uae_u32 opcode)
{
{{	uae_s16 dummy = get_diword (2);
	if (cctrue (14)) { Exception_cpu(7); goto l_1144; }
}}	m68k_incpc (4);
l_1144: ;
return 10 * CYCLE_UNIT / 2;
}

/* TRAPcc.L #<data>.L (GT) */
uae_u32 REGPARAM2 CPUFUNC(op_5efb_40)(uae_u32 opcode)
{
{{	uae_s32 dummy;
	dummy = get_dilong (2);
	if (cctrue (14)) { Exception_cpu(7); goto l_1145; }
}}	m68k_incpc (6);
l_1145: ;
return 10 * CYCLE_UNIT / 2;
}

/* TRAPcc.L  (GT) */
uae_u32 REGPARAM2 CPUFUNC(op_5efc_40)(uae_u32 opcode)
{
{	if (cctrue (14)) { Exception_cpu(7); goto l_1146; }
}	m68k_incpc (2);
l_1146: ;
return 10 * CYCLE_UNIT / 2;
}

/* Scc.B Dn (LE) */
uae_u32 REGPARAM2 CPUFUNC(op_5fc0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{{{	int val = cctrue (15) ? 0xff : 0;
	m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* DBcc.W Dn,#<data>.W (LE) */
uae_u32 REGPARAM2 CPUFUNC(op_5fc8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue (15)) {
	m68k_incpc ((uae_s32)offs + 2);
			m68k_dreg (regs, srcreg) = (m68k_dreg (regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
		if (src) {
			if (offs & 1) {
				exception3i (opcode, m68k_getpc () + 2 + (uae_s32)offs + 2);
		return 10 * CYCLE_UNIT / 2;
			}
			return 7 * CYCLE_UNIT / 2;
		}
	} else {
	}
	m68k_setpc (oldpc + 4);
}}}return 10 * CYCLE_UNIT / 2;
}

/* Scc.B (An) (LE) */
uae_u32 REGPARAM2 CPUFUNC(op_5fd0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{{	int val = cctrue (15) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B (An)+ (LE) */
uae_u32 REGPARAM2 CPUFUNC(op_5fd8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{{	int val = cctrue (15) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B -(An) (LE) */
uae_u32 REGPARAM2 CPUFUNC(op_5fe0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg (regs, srcreg) = srca;
{{	int val = cctrue (15) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* Scc.B (d16,An) (LE) */
uae_u32 REGPARAM2 CPUFUNC(op_5fe8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (15) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* Scc.B (d8,An,Xn) (LE) */
uae_u32 REGPARAM2 CPUFUNC(op_5ff0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{{	int val = cctrue (15) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).W (LE) */
uae_u32 REGPARAM2 CPUFUNC(op_5ff8_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{{	int val = cctrue (15) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* Scc.B (xxx).L (LE) */
uae_u32 REGPARAM2 CPUFUNC(op_5ff9_40)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{{	int val = cctrue (15) ? 0xff : 0;
	put_byte_jit (srca,val);
}}}}	m68k_incpc (6);
return 11 * CYCLE_UNIT / 2;
}

/* TRAPcc.L #<data>.W (LE) */
uae_u32 REGPARAM2 CPUFUNC(op_5ffa_40)(uae_u32 opcode)
{
{{	uae_s16 dummy = get_diword (2);
	if (cctrue (15)) { Exception_cpu(7); goto l_1156; }
}}	m68k_incpc (4);
l_1156: ;
return 10 * CYCLE_UNIT / 2;
}

/* TRAPcc.L #<data>.L (LE) */
uae_u32 REGPARAM2 CPUFUNC(op_5ffb_40)(uae_u32 opcode)
{
{{	uae_s32 dummy;
	dummy = get_dilong (2);
	if (cctrue (15)) { Exception_cpu(7); goto l_1157; }
}}	m68k_incpc (6);
l_1157: ;
return 10 * CYCLE_UNIT / 2;
}

/* TRAPcc.L  (LE) */
uae_u32 REGPARAM2 CPUFUNC(op_5ffc_40)(uae_u32 opcode)
{
{	if (cctrue (15)) { Exception_cpu(7); goto l_1158; }
}	m68k_incpc (2);
l_1158: ;
return 10 * CYCLE_UNIT / 2;
}

/* Bcc.W #<data>.W (T) */
uae_u32 REGPARAM2 CPUFUNC(op_6000_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
	if (!cctrue (0)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (4);
}}return 7 * CYCLE_UNIT / 2;
}

/* BccQ.B #<data> (T) */
uae_u32 REGPARAM2 CPUFUNC(op_6001_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue (0)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (2);
}}return 5 * CYCLE_UNIT / 2;
}

/* Bcc.L #<data>.L (T) */
uae_u32 REGPARAM2 CPUFUNC(op_60ff_40)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
	if (!cctrue (0)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 14 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (6);
}}return 9 * CYCLE_UNIT / 2;
}

/* BSR.W #<data>.W */
uae_u32 REGPARAM2 CPUFUNC(op_6100_40)(uae_u32 opcode)
{
{	uae_s32 s;
{	uae_s16 src = get_diword (2);
	s = (uae_s32)src + 2;
	if (src & 1) {
		exception3b (opcode, m68k_getpc () + s, 0, 1, m68k_getpc () + s);
		return 8 * CYCLE_UNIT / 2;
	}
	m68k_do_bsr (m68k_getpc () + 4, s);
	if(regs.t0) check_t0_trace();
}}return 13 * CYCLE_UNIT / 2;
}

/* BSRQ.B #<data> */
uae_u32 REGPARAM2 CPUFUNC(op_6101_40)(uae_u32 opcode)
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
	if(regs.t0) check_t0_trace();
}}return 13 * CYCLE_UNIT / 2;
}

/* BSR.L #<data>.L */
uae_u32 REGPARAM2 CPUFUNC(op_61ff_40)(uae_u32 opcode)
{
{	uae_s32 s;
{	uae_s32 src;
	src = get_dilong (2);
	s = (uae_s32)src + 2;
	if (src & 1) {
		exception3b (opcode, m68k_getpc () + s, 0, 1, m68k_getpc () + s);
		return 12 * CYCLE_UNIT / 2;
	}
	m68k_do_bsr (m68k_getpc () + 6, s);
	if(regs.t0) check_t0_trace();
}}return 13 * CYCLE_UNIT / 2;
}

/* Bcc.W #<data>.W (HI) */
uae_u32 REGPARAM2 CPUFUNC(op_6200_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
	if (!cctrue (2)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (4);
}}return 7 * CYCLE_UNIT / 2;
}

/* BccQ.B #<data> (HI) */
uae_u32 REGPARAM2 CPUFUNC(op_6201_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue (2)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (2);
}}return 5 * CYCLE_UNIT / 2;
}

/* Bcc.L #<data>.L (HI) */
uae_u32 REGPARAM2 CPUFUNC(op_62ff_40)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
	if (!cctrue (2)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (6);
}}return 9 * CYCLE_UNIT / 2;
}

/* Bcc.W #<data>.W (LS) */
uae_u32 REGPARAM2 CPUFUNC(op_6300_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
	if (!cctrue (3)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (4);
}}return 7 * CYCLE_UNIT / 2;
}

/* BccQ.B #<data> (LS) */
uae_u32 REGPARAM2 CPUFUNC(op_6301_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue (3)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (2);
}}return 5 * CYCLE_UNIT / 2;
}

#endif

#ifdef PART_6
/* Bcc.L #<data>.L (LS) */
uae_u32 REGPARAM2 CPUFUNC(op_63ff_40)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
	if (!cctrue (3)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (6);
}}return 9 * CYCLE_UNIT / 2;
}

/* Bcc.W #<data>.W (CC) */
uae_u32 REGPARAM2 CPUFUNC(op_6400_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
	if (!cctrue (4)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (4);
}}return 7 * CYCLE_UNIT / 2;
}

/* BccQ.B #<data> (CC) */
uae_u32 REGPARAM2 CPUFUNC(op_6401_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue (4)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (2);
}}return 5 * CYCLE_UNIT / 2;
}

/* Bcc.L #<data>.L (CC) */
uae_u32 REGPARAM2 CPUFUNC(op_64ff_40)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
	if (!cctrue (4)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (6);
}}return 9 * CYCLE_UNIT / 2;
}

/* Bcc.W #<data>.W (CS) */
uae_u32 REGPARAM2 CPUFUNC(op_6500_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
	if (!cctrue (5)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (4);
}}return 7 * CYCLE_UNIT / 2;
}

/* BccQ.B #<data> (CS) */
uae_u32 REGPARAM2 CPUFUNC(op_6501_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue (5)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (2);
}}return 5 * CYCLE_UNIT / 2;
}

/* Bcc.L #<data>.L (CS) */
uae_u32 REGPARAM2 CPUFUNC(op_65ff_40)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
	if (!cctrue (5)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (6);
}}return 9 * CYCLE_UNIT / 2;
}

/* Bcc.W #<data>.W (NE) */
uae_u32 REGPARAM2 CPUFUNC(op_6600_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
	if (!cctrue (6)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (4);
}}return 7 * CYCLE_UNIT / 2;
}

/* BccQ.B #<data> (NE) */
uae_u32 REGPARAM2 CPUFUNC(op_6601_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue (6)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (2);
}}return 5 * CYCLE_UNIT / 2;
}

/* Bcc.L #<data>.L (NE) */
uae_u32 REGPARAM2 CPUFUNC(op_66ff_40)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
	if (!cctrue (6)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (6);
}}return 9 * CYCLE_UNIT / 2;
}

/* Bcc.W #<data>.W (EQ) */
uae_u32 REGPARAM2 CPUFUNC(op_6700_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
	if (!cctrue (7)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (4);
}}return 7 * CYCLE_UNIT / 2;
}

/* BccQ.B #<data> (EQ) */
uae_u32 REGPARAM2 CPUFUNC(op_6701_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue (7)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (2);
}}return 5 * CYCLE_UNIT / 2;
}

/* Bcc.L #<data>.L (EQ) */
uae_u32 REGPARAM2 CPUFUNC(op_67ff_40)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
	if (!cctrue (7)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (6);
}}return 9 * CYCLE_UNIT / 2;
}

/* Bcc.W #<data>.W (VC) */
uae_u32 REGPARAM2 CPUFUNC(op_6800_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
	if (!cctrue (8)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (4);
}}return 7 * CYCLE_UNIT / 2;
}

/* BccQ.B #<data> (VC) */
uae_u32 REGPARAM2 CPUFUNC(op_6801_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue (8)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (2);
}}return 5 * CYCLE_UNIT / 2;
}

/* Bcc.L #<data>.L (VC) */
uae_u32 REGPARAM2 CPUFUNC(op_68ff_40)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
	if (!cctrue (8)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (6);
}}return 9 * CYCLE_UNIT / 2;
}

/* Bcc.W #<data>.W (VS) */
uae_u32 REGPARAM2 CPUFUNC(op_6900_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
	if (!cctrue (9)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (4);
}}return 7 * CYCLE_UNIT / 2;
}

/* BccQ.B #<data> (VS) */
uae_u32 REGPARAM2 CPUFUNC(op_6901_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue (9)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (2);
}}return 5 * CYCLE_UNIT / 2;
}

/* Bcc.L #<data>.L (VS) */
uae_u32 REGPARAM2 CPUFUNC(op_69ff_40)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
	if (!cctrue (9)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (6);
}}return 9 * CYCLE_UNIT / 2;
}

/* Bcc.W #<data>.W (PL) */
uae_u32 REGPARAM2 CPUFUNC(op_6a00_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
	if (!cctrue (10)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (4);
}}return 7 * CYCLE_UNIT / 2;
}

/* BccQ.B #<data> (PL) */
uae_u32 REGPARAM2 CPUFUNC(op_6a01_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue (10)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (2);
}}return 5 * CYCLE_UNIT / 2;
}

/* Bcc.L #<data>.L (PL) */
uae_u32 REGPARAM2 CPUFUNC(op_6aff_40)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
	if (!cctrue (10)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (6);
}}return 9 * CYCLE_UNIT / 2;
}

/* Bcc.W #<data>.W (MI) */
uae_u32 REGPARAM2 CPUFUNC(op_6b00_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
	if (!cctrue (11)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (4);
}}return 7 * CYCLE_UNIT / 2;
}

/* BccQ.B #<data> (MI) */
uae_u32 REGPARAM2 CPUFUNC(op_6b01_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue (11)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (2);
}}return 5 * CYCLE_UNIT / 2;
}

/* Bcc.L #<data>.L (MI) */
uae_u32 REGPARAM2 CPUFUNC(op_6bff_40)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
	if (!cctrue (11)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (6);
}}return 9 * CYCLE_UNIT / 2;
}

/* Bcc.W #<data>.W (GE) */
uae_u32 REGPARAM2 CPUFUNC(op_6c00_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
	if (!cctrue (12)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (4);
}}return 7 * CYCLE_UNIT / 2;
}

/* BccQ.B #<data> (GE) */
uae_u32 REGPARAM2 CPUFUNC(op_6c01_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue (12)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (2);
}}return 5 * CYCLE_UNIT / 2;
}

/* Bcc.L #<data>.L (GE) */
uae_u32 REGPARAM2 CPUFUNC(op_6cff_40)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
	if (!cctrue (12)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (6);
}}return 9 * CYCLE_UNIT / 2;
}

/* Bcc.W #<data>.W (LT) */
uae_u32 REGPARAM2 CPUFUNC(op_6d00_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
	if (!cctrue (13)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (4);
}}return 7 * CYCLE_UNIT / 2;
}

/* BccQ.B #<data> (LT) */
uae_u32 REGPARAM2 CPUFUNC(op_6d01_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue (13)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (2);
}}return 5 * CYCLE_UNIT / 2;
}

/* Bcc.L #<data>.L (LT) */
uae_u32 REGPARAM2 CPUFUNC(op_6dff_40)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
	if (!cctrue (13)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (6);
}}return 9 * CYCLE_UNIT / 2;
}

/* Bcc.W #<data>.W (GT) */
uae_u32 REGPARAM2 CPUFUNC(op_6e00_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
	if (!cctrue (14)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (4);
}}return 7 * CYCLE_UNIT / 2;
}

/* BccQ.B #<data> (GT) */
uae_u32 REGPARAM2 CPUFUNC(op_6e01_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue (14)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (2);
}}return 5 * CYCLE_UNIT / 2;
}

/* Bcc.L #<data>.L (GT) */
uae_u32 REGPARAM2 CPUFUNC(op_6eff_40)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
	if (!cctrue (14)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (6);
}}return 9 * CYCLE_UNIT / 2;
}

/* Bcc.W #<data>.W (LE) */
uae_u32 REGPARAM2 CPUFUNC(op_6f00_40)(uae_u32 opcode)
{
{{	uae_s16 src = get_diword (2);
	if (!cctrue (15)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (4);
}}return 7 * CYCLE_UNIT / 2;
}

/* BccQ.B #<data> (LE) */
uae_u32 REGPARAM2 CPUFUNC(op_6f01_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!cctrue (15)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (2);
}}return 5 * CYCLE_UNIT / 2;
}

/* Bcc.L #<data>.L (LE) */
uae_u32 REGPARAM2 CPUFUNC(op_6fff_40)(uae_u32 opcode)
{
{{	uae_s32 src;
	src = get_dilong (2);
	if (!cctrue (15)) goto didnt_jump;
	if (src & 1) {
		exception3i (opcode, m68k_getpc () + 2 + (uae_s32)src);
		return 7 * CYCLE_UNIT / 2;
	}
	m68k_incpc ((uae_s32)src + 2);
	return 9 * CYCLE_UNIT / 2;
didnt_jump:;
	m68k_incpc (6);
}}return 9 * CYCLE_UNIT / 2;
}

/* MOVEQ.L #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_7000_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_u32 src = srcreg;
{	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
	m68k_incpc (2);
}}}return 3 * CYCLE_UNIT / 2;
}

/* OR.B Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8000_40)(uae_u32 opcode)
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
return 3 * CYCLE_UNIT / 2;
}

/* OR.B (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8010_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (2);
return 7 * CYCLE_UNIT / 2;
}

/* OR.B (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8018_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (2);
return 7 * CYCLE_UNIT / 2;
}

/* OR.B -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8020_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* OR.B (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8028_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* OR.B (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8030_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s8 src = get_byte_jit (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* OR.B (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8038_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* OR.B (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8039_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte_jit (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (6);
return 10 * CYCLE_UNIT / 2;
}

/* OR.B (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_803a_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* OR.B (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_803b_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s8 src = get_byte_jit (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* OR.B #<data>.B,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_803c_40)(uae_u32 opcode)
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
return 6 * CYCLE_UNIT / 2;
}

/* OR.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8040_40)(uae_u32 opcode)
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
return 3 * CYCLE_UNIT / 2;
}

/* OR.W (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8050_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (2);
return 7 * CYCLE_UNIT / 2;
}

/* OR.W (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8058_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) += 2;
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (2);
return 7 * CYCLE_UNIT / 2;
}

/* OR.W -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8060_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* OR.W (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8068_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* OR.W (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8070_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* OR.W (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8078_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* OR.W (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8079_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (6);
return 10 * CYCLE_UNIT / 2;
}

/* OR.W (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_807a_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* OR.W (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_807b_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* OR.W #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_807c_40)(uae_u32 opcode)
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
return 6 * CYCLE_UNIT / 2;
}

/* OR.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8080_40)(uae_u32 opcode)
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
return 3 * CYCLE_UNIT / 2;
}

/* OR.L (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8090_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}}	m68k_incpc (2);
return 7 * CYCLE_UNIT / 2;
}

/* OR.L (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8098_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) += 4;
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}}	m68k_incpc (2);
return 7 * CYCLE_UNIT / 2;
}

/* OR.L -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_80a0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* OR.L (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_80a8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* OR.L (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_80b0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* OR.L (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_80b8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* OR.L (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_80b9_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}}	m68k_incpc (6);
return 10 * CYCLE_UNIT / 2;
}

/* OR.L (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_80ba_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* OR.L (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_80bb_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* OR.L #<data>.L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_80bc_40)(uae_u32 opcode)
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
return 8 * CYCLE_UNIT / 2;
}

/* DIVU.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_80c0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	CLEAR_CZNV ();
	if (src == 0) {
		divbyzero_special (0, dst);
	m68k_incpc (2);
		Exception_cpu(5);
		return 4 * CYCLE_UNIT / 2;
	} else {
		uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
		uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
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
}}}return 44 * CYCLE_UNIT / 2;
}

/* DIVU.W (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_80d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	CLEAR_CZNV ();
	if (src == 0) {
		divbyzero_special (0, dst);
	m68k_incpc (2);
		Exception_cpu(5);
		return 8 * CYCLE_UNIT / 2;
	} else {
		uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
		uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
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
}}}}return 48 * CYCLE_UNIT / 2;
}

/* DIVU.W (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_80d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) += 2;
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	CLEAR_CZNV ();
	if (src == 0) {
		divbyzero_special (0, dst);
	m68k_incpc (2);
		Exception_cpu(5);
		return 8 * CYCLE_UNIT / 2;
	} else {
		uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
		uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
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
}}}}return 48 * CYCLE_UNIT / 2;
}

/* DIVU.W -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_80e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	CLEAR_CZNV ();
	if (src == 0) {
		divbyzero_special (0, dst);
	m68k_incpc (2);
		Exception_cpu(5);
		return 10 * CYCLE_UNIT / 2;
	} else {
		uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
		uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
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
}}}}return 49 * CYCLE_UNIT / 2;
}

/* DIVU.W (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_80e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	CLEAR_CZNV ();
	if (src == 0) {
		divbyzero_special (0, dst);
	m68k_incpc (4);
		Exception_cpu(5);
		return 12 * CYCLE_UNIT / 2;
	} else {
		uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
		uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
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
}}}}return 50 * CYCLE_UNIT / 2;
}

/* DIVU.W (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_80f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s16 src = get_word_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	CLEAR_CZNV ();
	if (src == 0) {
		divbyzero_special (0, dst);
	m68k_incpc (0);
		Exception_cpu(5);
		return 8 * CYCLE_UNIT / 2;
	} else {
		uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
		uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
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
	}
}}}}}return 52 * CYCLE_UNIT / 2;
}

/* DIVU.W (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_80f8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	CLEAR_CZNV ();
	if (src == 0) {
		divbyzero_special (0, dst);
	m68k_incpc (4);
		Exception_cpu(5);
		return 12 * CYCLE_UNIT / 2;
	} else {
		uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
		uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
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
}}}}return 50 * CYCLE_UNIT / 2;
}

/* DIVU.W (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_80f9_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	CLEAR_CZNV ();
	if (src == 0) {
		divbyzero_special (0, dst);
	m68k_incpc (6);
		Exception_cpu(5);
		return 16 * CYCLE_UNIT / 2;
	} else {
		uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
		uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
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
}}}}return 51 * CYCLE_UNIT / 2;
}

/* DIVU.W (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_80fa_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	CLEAR_CZNV ();
	if (src == 0) {
		divbyzero_special (0, dst);
	m68k_incpc (4);
		Exception_cpu(5);
		return 12 * CYCLE_UNIT / 2;
	} else {
		uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
		uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
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
}}}}return 50 * CYCLE_UNIT / 2;
}

/* DIVU.W (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_80fb_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s16 src = get_word_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	CLEAR_CZNV ();
	if (src == 0) {
		divbyzero_special (0, dst);
	m68k_incpc (0);
		Exception_cpu(5);
		return 8 * CYCLE_UNIT / 2;
	} else {
		uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
		uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
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
	}
}}}}}return 52 * CYCLE_UNIT / 2;
}

/* DIVU.W #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_80fc_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	CLEAR_CZNV ();
	if (src == 0) {
		divbyzero_special (0, dst);
	m68k_incpc (4);
		Exception_cpu(5);
		return 8 * CYCLE_UNIT / 2;
	} else {
		uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
		uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
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
}}}return 47 * CYCLE_UNIT / 2;
}

/* SBCD.B Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8100_40)(uae_u32 opcode)
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
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}	m68k_incpc (2);
return 5 * CYCLE_UNIT / 2;
}

/* SBCD.B -(An),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_8108_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte_jit (dsta);
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
	put_byte_jit (dsta,newv);
}}}}}}	m68k_incpc (2);
return 17 * CYCLE_UNIT / 2;
}

/* OR.B Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_8110_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* OR.B Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_8118_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* OR.B Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_8120_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

/* OR.B Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_8128_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte_jit (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* OR.B Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_8130_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	m68k_incpc (2);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s8 dst = get_byte_jit (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}}return 14 * CYCLE_UNIT / 2;
}

/* OR.B Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_8138_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte_jit (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* OR.B Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_8139_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s8 dst = get_byte_jit (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* PACK.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8140_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uae_u16 val = m68k_dreg (regs, srcreg) + get_diword (2);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & 0xffffff00) | ((val >> 4) & 0xf0) | (val & 0xf);
}	m68k_incpc (4);
return 7 * CYCLE_UNIT / 2;
}

/* PACK.L -(An),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_8148_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uae_u16 val;
	m68k_areg (regs, srcreg) -= areg_byteinc[srcreg];
	val = (uae_u16)(get_byte_jit (m68k_areg (regs, srcreg)) & 0xff);
	m68k_areg (regs, srcreg) -= areg_byteinc[srcreg];
	val = (val | ((uae_u16)(get_byte_jit (m68k_areg (regs, srcreg)) & 0xff) << 8)) + get_diword (2);
	m68k_areg (regs, dstreg) -= areg_byteinc[dstreg];
	put_byte_jit (m68k_areg (regs, dstreg),((val >> 4) & 0xf0) | (val & 0xf));
}	m68k_incpc (4);
return 13 * CYCLE_UNIT / 2;
}

/* OR.W Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_8150_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word_jit (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* OR.W Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_8158_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word_jit (dsta);
	m68k_areg (regs, dstreg) += 2;
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* OR.W Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_8160_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
{	uae_s16 dst = get_word_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

/* OR.W Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_8168_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 dst = get_word_jit (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* OR.W Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_8170_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	m68k_incpc (2);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s16 dst = get_word_jit (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}}return 14 * CYCLE_UNIT / 2;
}

/* OR.W Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_8178_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 dst = get_word_jit (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* OR.W Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_8179_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s16 dst = get_word_jit (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* UNPK.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8180_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uae_u16 val = m68k_dreg (regs, srcreg);
	val = (((val << 4) & 0xf00) | (val & 0xf)) + get_diword (2);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & 0xffff0000) | (val & 0xffff);
}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* UNPK.L -(An),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_8188_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uae_u16 val;
	m68k_areg (regs, srcreg) -= areg_byteinc[srcreg];
	val = (uae_u16)(get_byte_jit (m68k_areg (regs, srcreg)) & 0xff);
	val = (((val << 4) & 0xf00) | (val & 0xf)) + get_diword (2);
	m68k_areg (regs, dstreg) -= 2 * areg_byteinc[dstreg];
	put_byte_jit (m68k_areg (regs, dstreg) + areg_byteinc[dstreg], val);
	put_byte_jit (m68k_areg (regs, dstreg), val >> 8);
}	m68k_incpc (4);
return 13 * CYCLE_UNIT / 2;
}

/* OR.L Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_8190_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long_jit (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* OR.L Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_8198_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long_jit (dsta);
	m68k_areg (regs, dstreg) += 4;
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* OR.L Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_81a0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
{	uae_s32 dst = get_long_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

/* OR.L Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_81a8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 dst = get_long_jit (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* OR.L Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_81b0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	m68k_incpc (2);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s32 dst = get_long_jit (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}}return 14 * CYCLE_UNIT / 2;
}

/* OR.L Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_81b8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 dst = get_long_jit (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* OR.L Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_81b9_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s32 dst = get_long_jit (dsta);
	src |= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* DIVS.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_81c0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	if (src == 0) {
		divbyzero_special (1, dst);
	m68k_incpc (2);
		Exception_cpu(5);
		return 4 * CYCLE_UNIT / 2;
	}
	CLEAR_CZNV ();
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
}}}return 57 * CYCLE_UNIT / 2;
}

/* DIVS.W (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_81d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	if (src == 0) {
		divbyzero_special (1, dst);
	m68k_incpc (2);
		Exception_cpu(5);
		return 8 * CYCLE_UNIT / 2;
	}
	CLEAR_CZNV ();
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
}}}}return 61 * CYCLE_UNIT / 2;
}

/* DIVS.W (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_81d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) += 2;
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	if (src == 0) {
		divbyzero_special (1, dst);
	m68k_incpc (2);
		Exception_cpu(5);
		return 8 * CYCLE_UNIT / 2;
	}
	CLEAR_CZNV ();
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
}}}}return 61 * CYCLE_UNIT / 2;
}

/* DIVS.W -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_81e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	if (src == 0) {
		divbyzero_special (1, dst);
	m68k_incpc (2);
		Exception_cpu(5);
		return 10 * CYCLE_UNIT / 2;
	}
	CLEAR_CZNV ();
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
}}}}return 62 * CYCLE_UNIT / 2;
}

/* DIVS.W (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_81e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	if (src == 0) {
		divbyzero_special (1, dst);
	m68k_incpc (4);
		Exception_cpu(5);
		return 12 * CYCLE_UNIT / 2;
	}
	CLEAR_CZNV ();
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
}}}}return 63 * CYCLE_UNIT / 2;
}

/* DIVS.W (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_81f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s16 src = get_word_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	if (src == 0) {
		divbyzero_special (1, dst);
	m68k_incpc (0);
		Exception_cpu(5);
		return 8 * CYCLE_UNIT / 2;
	}
	CLEAR_CZNV ();
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
}}}}}return 65 * CYCLE_UNIT / 2;
}

/* DIVS.W (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_81f8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	if (src == 0) {
		divbyzero_special (1, dst);
	m68k_incpc (4);
		Exception_cpu(5);
		return 12 * CYCLE_UNIT / 2;
	}
	CLEAR_CZNV ();
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
}}}}return 63 * CYCLE_UNIT / 2;
}

/* DIVS.W (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_81f9_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	if (src == 0) {
		divbyzero_special (1, dst);
	m68k_incpc (6);
		Exception_cpu(5);
		return 16 * CYCLE_UNIT / 2;
	}
	CLEAR_CZNV ();
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
}}}}return 64 * CYCLE_UNIT / 2;
}

/* DIVS.W (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_81fa_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	if (src == 0) {
		divbyzero_special (1, dst);
	m68k_incpc (4);
		Exception_cpu(5);
		return 12 * CYCLE_UNIT / 2;
	}
	CLEAR_CZNV ();
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
}}}}return 63 * CYCLE_UNIT / 2;
}

/* DIVS.W (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_81fb_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s16 src = get_word_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	if (src == 0) {
		divbyzero_special (1, dst);
	m68k_incpc (0);
		Exception_cpu(5);
		return 8 * CYCLE_UNIT / 2;
	}
	CLEAR_CZNV ();
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
}}}}}return 65 * CYCLE_UNIT / 2;
}

/* DIVS.W #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_81fc_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	if (src == 0) {
		divbyzero_special (1, dst);
	m68k_incpc (4);
		Exception_cpu(5);
		return 8 * CYCLE_UNIT / 2;
	}
	CLEAR_CZNV ();
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
}}}return 60 * CYCLE_UNIT / 2;
}

/* SUB.B Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9000_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
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
return 3 * CYCLE_UNIT / 2;
}

/* SUB.B (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9010_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
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
return 7 * CYCLE_UNIT / 2;
}

/* SUB.B (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9018_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
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
return 7 * CYCLE_UNIT / 2;
}

/* SUB.B -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9020_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
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

/* SUB.B (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9028_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
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
return 9 * CYCLE_UNIT / 2;
}

/* SUB.B (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9030_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s8 src = get_byte_jit (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}}return 11 * CYCLE_UNIT / 2;
}

/* SUB.B (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9038_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
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
return 9 * CYCLE_UNIT / 2;
}

/* SUB.B (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9039_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte_jit (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
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
return 10 * CYCLE_UNIT / 2;
}

/* SUB.B (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_903a_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
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
return 9 * CYCLE_UNIT / 2;
}

/* SUB.B (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_903b_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s8 src = get_byte_jit (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}}return 11 * CYCLE_UNIT / 2;
}

/* SUB.B #<data>.B,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_903c_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = get_dibyte (2);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
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
return 6 * CYCLE_UNIT / 2;
}

/* SUB.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9040_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
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
return 3 * CYCLE_UNIT / 2;
}

/* SUB.W An,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9048_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_areg (regs, srcreg);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
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
return 3 * CYCLE_UNIT / 2;
}

/* SUB.W (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9050_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
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
return 7 * CYCLE_UNIT / 2;
}

/* SUB.W (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9058_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) += 2;
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
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
return 7 * CYCLE_UNIT / 2;
}

/* SUB.W -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9060_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
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

/* SUB.W (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9068_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
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
return 9 * CYCLE_UNIT / 2;
}

/* SUB.W (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9070_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}}return 11 * CYCLE_UNIT / 2;
}

/* SUB.W (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9078_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
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
return 9 * CYCLE_UNIT / 2;
}

/* SUB.W (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9079_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
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
return 10 * CYCLE_UNIT / 2;
}

/* SUB.W (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_907a_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
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
return 9 * CYCLE_UNIT / 2;
}

/* SUB.W (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_907b_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}}return 11 * CYCLE_UNIT / 2;
}

/* SUB.W #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_907c_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
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
return 6 * CYCLE_UNIT / 2;
}

/* SUB.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9080_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
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
return 3 * CYCLE_UNIT / 2;
}

/* SUB.L An,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9088_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_areg (regs, srcreg);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
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
return 3 * CYCLE_UNIT / 2;
}

/* SUB.L (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9090_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
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
return 7 * CYCLE_UNIT / 2;
}

/* SUB.L (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9098_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) += 4;
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
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
return 7 * CYCLE_UNIT / 2;
}

/* SUB.L -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_90a0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
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
return 8 * CYCLE_UNIT / 2;
}

/* SUB.L (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_90a8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
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
return 9 * CYCLE_UNIT / 2;
}

/* SUB.L (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_90b0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}}}}return 11 * CYCLE_UNIT / 2;
}

/* SUB.L (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_90b8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
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
return 9 * CYCLE_UNIT / 2;
}

/* SUB.L (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_90b9_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
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
return 10 * CYCLE_UNIT / 2;
}

/* SUB.L (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_90ba_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
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
return 9 * CYCLE_UNIT / 2;
}

/* SUB.L (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_90bb_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}}}}return 11 * CYCLE_UNIT / 2;
}

/* SUB.L #<data>.L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_90bc_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
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
return 8 * CYCLE_UNIT / 2;
}

/* SUBA.W Dn,An */
uae_u32 REGPARAM2 CPUFUNC(op_90c0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}	m68k_incpc (2);
return 3 * CYCLE_UNIT / 2;
}

/* SUBA.W An,An */
uae_u32 REGPARAM2 CPUFUNC(op_90c8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_areg (regs, srcreg);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}	m68k_incpc (2);
return 3 * CYCLE_UNIT / 2;
}

/* SUBA.W (An),An */
uae_u32 REGPARAM2 CPUFUNC(op_90d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (2);
return 7 * CYCLE_UNIT / 2;
}

/* SUBA.W (An)+,An */
uae_u32 REGPARAM2 CPUFUNC(op_90d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) += 2;
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (2);
return 7 * CYCLE_UNIT / 2;
}

/* SUBA.W -(An),An */
uae_u32 REGPARAM2 CPUFUNC(op_90e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* SUBA.W (d16,An),An */
uae_u32 REGPARAM2 CPUFUNC(op_90e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* SUBA.W (d8,An,Xn),An */
uae_u32 REGPARAM2 CPUFUNC(op_90f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s16 src = get_word_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}}return 11 * CYCLE_UNIT / 2;
}

/* SUBA.W (xxx).W,An */
uae_u32 REGPARAM2 CPUFUNC(op_90f8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* SUBA.W (xxx).L,An */
uae_u32 REGPARAM2 CPUFUNC(op_90f9_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (6);
return 10 * CYCLE_UNIT / 2;
}

/* SUBA.W (d16,PC),An */
uae_u32 REGPARAM2 CPUFUNC(op_90fa_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* SUBA.W (d8,PC,Xn),An */
uae_u32 REGPARAM2 CPUFUNC(op_90fb_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s16 src = get_word_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}}return 11 * CYCLE_UNIT / 2;
}

/* SUBA.W #<data>.W,An */
uae_u32 REGPARAM2 CPUFUNC(op_90fc_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}	m68k_incpc (4);
return 6 * CYCLE_UNIT / 2;
}

/* SUBX.B Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9100_40)(uae_u32 opcode)
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
return 3 * CYCLE_UNIT / 2;
}

/* SUBX.B -(An),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_9108_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte_jit (dsta);
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
	put_byte_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 13 * CYCLE_UNIT / 2;
}

/* SUB.B Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_9110_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* SUB.B Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_9118_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* SUB.B Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_9120_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

/* SUB.B Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_9128_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte_jit (dsta);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* SUB.B Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_9130_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	m68k_incpc (2);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s8 dst = get_byte_jit (dsta);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}}return 14 * CYCLE_UNIT / 2;
}

/* SUB.B Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_9138_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte_jit (dsta);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* SUB.B Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_9139_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s8 dst = get_byte_jit (dsta);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* SUBX.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9140_40)(uae_u32 opcode)
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
return 3 * CYCLE_UNIT / 2;
}

/* SUBX.W -(An),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_9148_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
{	uae_s16 dst = get_word_jit (dsta);
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
	put_word_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 13 * CYCLE_UNIT / 2;
}

/* SUB.W Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_9150_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word_jit (dsta);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* SUB.W Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_9158_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word_jit (dsta);
	m68k_areg (regs, dstreg) += 2;
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* SUB.W Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_9160_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
{	uae_s16 dst = get_word_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

/* SUB.W Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_9168_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 dst = get_word_jit (dsta);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* SUB.W Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_9170_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	m68k_incpc (2);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s16 dst = get_word_jit (dsta);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}}return 14 * CYCLE_UNIT / 2;
}

/* SUB.W Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_9178_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 dst = get_word_jit (dsta);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* SUB.W Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_9179_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s16 dst = get_word_jit (dsta);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* SUBX.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_9180_40)(uae_u32 opcode)
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
return 3 * CYCLE_UNIT / 2;
}

/* SUBX.L -(An),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_9188_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
{	uae_s32 dst = get_long_jit (dsta);
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
	put_long_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 13 * CYCLE_UNIT / 2;
}

/* SUB.L Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_9190_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long_jit (dsta);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* SUB.L Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_9198_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long_jit (dsta);
	m68k_areg (regs, dstreg) += 4;
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* SUB.L Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_91a0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
{	uae_s32 dst = get_long_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

/* SUB.L Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_91a8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 dst = get_long_jit (dsta);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* SUB.L Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_91b0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	m68k_incpc (2);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s32 dst = get_long_jit (dsta);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}}return 14 * CYCLE_UNIT / 2;
}

/* SUB.L Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_91b8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 dst = get_long_jit (dsta);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* SUB.L Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_91b9_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s32 dst = get_long_jit (dsta);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* SUBA.L Dn,An */
uae_u32 REGPARAM2 CPUFUNC(op_91c0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}	m68k_incpc (2);
return 3 * CYCLE_UNIT / 2;
}

/* SUBA.L An,An */
uae_u32 REGPARAM2 CPUFUNC(op_91c8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_areg (regs, srcreg);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}	m68k_incpc (2);
return 3 * CYCLE_UNIT / 2;
}

/* SUBA.L (An),An */
uae_u32 REGPARAM2 CPUFUNC(op_91d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (2);
return 7 * CYCLE_UNIT / 2;
}

/* SUBA.L (An)+,An */
uae_u32 REGPARAM2 CPUFUNC(op_91d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) += 4;
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (2);
return 7 * CYCLE_UNIT / 2;
}

/* SUBA.L -(An),An */
uae_u32 REGPARAM2 CPUFUNC(op_91e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* SUBA.L (d16,An),An */
uae_u32 REGPARAM2 CPUFUNC(op_91e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* SUBA.L (d8,An,Xn),An */
uae_u32 REGPARAM2 CPUFUNC(op_91f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}}return 11 * CYCLE_UNIT / 2;
}

/* SUBA.L (xxx).W,An */
uae_u32 REGPARAM2 CPUFUNC(op_91f8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* SUBA.L (xxx).L,An */
uae_u32 REGPARAM2 CPUFUNC(op_91f9_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (6);
return 10 * CYCLE_UNIT / 2;
}

/* SUBA.L (d16,PC),An */
uae_u32 REGPARAM2 CPUFUNC(op_91fa_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* SUBA.L (d8,PC,Xn),An */
uae_u32 REGPARAM2 CPUFUNC(op_91fb_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}}return 11 * CYCLE_UNIT / 2;
}

/* SUBA.L #<data>.L,An */
uae_u32 REGPARAM2 CPUFUNC(op_91fc_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst - src;
	m68k_areg (regs, dstreg) = (newv);
}}}}	m68k_incpc (6);
return 8 * CYCLE_UNIT / 2;
}

/* CMP.B Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b000_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}	m68k_incpc (2);
return 3 * CYCLE_UNIT / 2;
}

/* CMP.B (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b010_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (2);
return 7 * CYCLE_UNIT / 2;
}

/* CMP.B (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b018_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (2);
return 7 * CYCLE_UNIT / 2;
}

/* CMP.B -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b020_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
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

/* CMP.B (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b028_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* CMP.B (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b030_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s8 src = get_byte_jit (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}}return 11 * CYCLE_UNIT / 2;
}

/* CMP.B (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b038_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* CMP.B (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b039_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte_jit (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (6);
return 10 * CYCLE_UNIT / 2;
}

/* CMP.B (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b03a_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* CMP.B (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b03b_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s8 src = get_byte_jit (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}}return 11 * CYCLE_UNIT / 2;
}

/* CMP.B #<data>.B,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b03c_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = get_dibyte (2);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}	m68k_incpc (4);
return 6 * CYCLE_UNIT / 2;
}

/* CMP.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b040_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}	m68k_incpc (2);
return 3 * CYCLE_UNIT / 2;
}

/* CMP.W An,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b048_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_areg (regs, srcreg);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}	m68k_incpc (2);
return 3 * CYCLE_UNIT / 2;
}

/* CMP.W (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b050_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (2);
return 7 * CYCLE_UNIT / 2;
}

/* CMP.W (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b058_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) += 2;
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (2);
return 7 * CYCLE_UNIT / 2;
}

/* CMP.W -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b060_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
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

/* CMP.W (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b068_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* CMP.W (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b070_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}}return 11 * CYCLE_UNIT / 2;
}

/* CMP.W (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b078_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* CMP.W (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b079_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (6);
return 10 * CYCLE_UNIT / 2;
}

/* CMP.W (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b07a_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* CMP.W (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b07b_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}}return 11 * CYCLE_UNIT / 2;
}

/* CMP.W #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b07c_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}	m68k_incpc (4);
return 6 * CYCLE_UNIT / 2;
}

/* CMP.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b080_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}	m68k_incpc (2);
return 3 * CYCLE_UNIT / 2;
}

/* CMP.L An,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b088_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_areg (regs, srcreg);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}	m68k_incpc (2);
return 3 * CYCLE_UNIT / 2;
}

/* CMP.L (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b090_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (2);
return 7 * CYCLE_UNIT / 2;
}

/* CMP.L (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b098_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) += 4;
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (2);
return 7 * CYCLE_UNIT / 2;
}

/* CMP.L -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b0a0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

#endif

#ifdef PART_7
/* CMP.L (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b0a8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* CMP.L (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b0b0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}}return 11 * CYCLE_UNIT / 2;
}

/* CMP.L (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b0b8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* CMP.L (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b0b9_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (6);
return 10 * CYCLE_UNIT / 2;
}

/* CMP.L (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b0ba_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* CMP.L (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b0bb_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}}return 11 * CYCLE_UNIT / 2;
}

/* CMP.L #<data>.L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b0bc_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}	m68k_incpc (6);
return 8 * CYCLE_UNIT / 2;
}

/* CMPA.W Dn,An */
uae_u32 REGPARAM2 CPUFUNC(op_b0c0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* CMPA.W An,An */
uae_u32 REGPARAM2 CPUFUNC(op_b0c8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_areg (regs, srcreg);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* CMPA.W (An),An */
uae_u32 REGPARAM2 CPUFUNC(op_b0d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* CMPA.W (An)+,An */
uae_u32 REGPARAM2 CPUFUNC(op_b0d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) += 2;
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* CMPA.W -(An),An */
uae_u32 REGPARAM2 CPUFUNC(op_b0e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (2);
return 9 * CYCLE_UNIT / 2;
}

/* CMPA.W (d16,An),An */
uae_u32 REGPARAM2 CPUFUNC(op_b0e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 10 * CYCLE_UNIT / 2;
}

/* CMPA.W (d8,An,Xn),An */
uae_u32 REGPARAM2 CPUFUNC(op_b0f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s16 src = get_word_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}}return 12 * CYCLE_UNIT / 2;
}

/* CMPA.W (xxx).W,An */
uae_u32 REGPARAM2 CPUFUNC(op_b0f8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 10 * CYCLE_UNIT / 2;
}

/* CMPA.W (xxx).L,An */
uae_u32 REGPARAM2 CPUFUNC(op_b0f9_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (6);
return 11 * CYCLE_UNIT / 2;
}

/* CMPA.W (d16,PC),An */
uae_u32 REGPARAM2 CPUFUNC(op_b0fa_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 10 * CYCLE_UNIT / 2;
}

/* CMPA.W (d8,PC,Xn),An */
uae_u32 REGPARAM2 CPUFUNC(op_b0fb_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s16 src = get_word_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}}return 12 * CYCLE_UNIT / 2;
}

/* CMPA.W #<data>.W,An */
uae_u32 REGPARAM2 CPUFUNC(op_b0fc_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}	m68k_incpc (4);
return 7 * CYCLE_UNIT / 2;
}

/* EOR.B Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b100_40)(uae_u32 opcode)
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
return 3 * CYCLE_UNIT / 2;
}

/* CMPM.B (An)+,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_b108_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
{{uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* EOR.B Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_b110_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* EOR.B Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_b118_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* EOR.B Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_b120_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

/* EOR.B Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_b128_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte_jit (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* EOR.B Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_b130_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	m68k_incpc (2);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s8 dst = get_byte_jit (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}}return 14 * CYCLE_UNIT / 2;
}

/* EOR.B Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_b138_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte_jit (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* EOR.B Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_b139_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s8 dst = get_byte_jit (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* EOR.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b140_40)(uae_u32 opcode)
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
return 3 * CYCLE_UNIT / 2;
}

/* CMPM.W (An)+,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_b148_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) += 2;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word_jit (dsta);
	m68k_areg (regs, dstreg) += 2;
{{uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* EOR.W Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_b150_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word_jit (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* EOR.W Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_b158_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word_jit (dsta);
	m68k_areg (regs, dstreg) += 2;
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* EOR.W Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_b160_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
{	uae_s16 dst = get_word_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

/* EOR.W Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_b168_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 dst = get_word_jit (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* EOR.W Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_b170_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	m68k_incpc (2);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s16 dst = get_word_jit (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}}return 14 * CYCLE_UNIT / 2;
}

/* EOR.W Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_b178_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 dst = get_word_jit (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* EOR.W Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_b179_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s16 dst = get_word_jit (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* EOR.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_b180_40)(uae_u32 opcode)
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
return 3 * CYCLE_UNIT / 2;
}

/* CMPM.L (An)+,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_b188_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) += 4;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long_jit (dsta);
	m68k_areg (regs, dstreg) += 4;
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* EOR.L Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_b190_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long_jit (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* EOR.L Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_b198_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long_jit (dsta);
	m68k_areg (regs, dstreg) += 4;
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* EOR.L Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_b1a0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
{	uae_s32 dst = get_long_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

/* EOR.L Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_b1a8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 dst = get_long_jit (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* EOR.L Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_b1b0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	m68k_incpc (2);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s32 dst = get_long_jit (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}}return 14 * CYCLE_UNIT / 2;
}

/* EOR.L Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_b1b8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 dst = get_long_jit (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* EOR.L Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_b1b9_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s32 dst = get_long_jit (dsta);
	src ^= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* CMPA.L Dn,An */
uae_u32 REGPARAM2 CPUFUNC(op_b1c0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* CMPA.L An,An */
uae_u32 REGPARAM2 CPUFUNC(op_b1c8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_areg (regs, srcreg);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* CMPA.L (An),An */
uae_u32 REGPARAM2 CPUFUNC(op_b1d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* CMPA.L (An)+,An */
uae_u32 REGPARAM2 CPUFUNC(op_b1d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) += 4;
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* CMPA.L -(An),An */
uae_u32 REGPARAM2 CPUFUNC(op_b1e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (2);
return 9 * CYCLE_UNIT / 2;
}

/* CMPA.L (d16,An),An */
uae_u32 REGPARAM2 CPUFUNC(op_b1e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 10 * CYCLE_UNIT / 2;
}

/* CMPA.L (d8,An,Xn),An */
uae_u32 REGPARAM2 CPUFUNC(op_b1f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}}return 12 * CYCLE_UNIT / 2;
}

/* CMPA.L (xxx).W,An */
uae_u32 REGPARAM2 CPUFUNC(op_b1f8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 10 * CYCLE_UNIT / 2;
}

/* CMPA.L (xxx).L,An */
uae_u32 REGPARAM2 CPUFUNC(op_b1f9_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (6);
return 11 * CYCLE_UNIT / 2;
}

/* CMPA.L (d16,PC),An */
uae_u32 REGPARAM2 CPUFUNC(op_b1fa_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}	m68k_incpc (4);
return 10 * CYCLE_UNIT / 2;
}

/* CMPA.L (d8,PC,Xn),An */
uae_u32 REGPARAM2 CPUFUNC(op_b1fb_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}}}return 12 * CYCLE_UNIT / 2;
}

/* CMPA.L #<data>.L,An */
uae_u32 REGPARAM2 CPUFUNC(op_b1fc_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs != flgo) && (flgn != flgo));
	SET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG (flgn != 0);
}}}}}}	m68k_incpc (6);
return 9 * CYCLE_UNIT / 2;
}

/* AND.B Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c000_40)(uae_u32 opcode)
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
return 3 * CYCLE_UNIT / 2;
}

/* AND.B (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c010_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (2);
return 7 * CYCLE_UNIT / 2;
}

/* AND.B (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c018_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (2);
return 7 * CYCLE_UNIT / 2;
}

/* AND.B -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c020_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* AND.B (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c028_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* AND.B (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c030_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s8 src = get_byte_jit (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* AND.B (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c038_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* AND.B (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c039_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte_jit (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (6);
return 10 * CYCLE_UNIT / 2;
}

/* AND.B (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c03a_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* AND.B (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c03b_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s8 src = get_byte_jit (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((src) & 0xff);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* AND.B #<data>.B,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c03c_40)(uae_u32 opcode)
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
return 6 * CYCLE_UNIT / 2;
}

/* AND.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c040_40)(uae_u32 opcode)
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
return 3 * CYCLE_UNIT / 2;
}

/* AND.W (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c050_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (2);
return 7 * CYCLE_UNIT / 2;
}

/* AND.W (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c058_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) += 2;
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (2);
return 7 * CYCLE_UNIT / 2;
}

/* AND.W -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c060_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* AND.W (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c068_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* AND.W (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c070_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* AND.W (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c078_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* AND.W (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c079_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (6);
return 10 * CYCLE_UNIT / 2;
}

/* AND.W (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c07a_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* AND.W (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c07b_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* AND.W #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c07c_40)(uae_u32 opcode)
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
return 6 * CYCLE_UNIT / 2;
}

/* AND.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c080_40)(uae_u32 opcode)
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
return 3 * CYCLE_UNIT / 2;
}

/* AND.L (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c090_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}}	m68k_incpc (2);
return 7 * CYCLE_UNIT / 2;
}

/* AND.L (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c098_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) += 4;
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}}	m68k_incpc (2);
return 7 * CYCLE_UNIT / 2;
}

/* AND.L -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c0a0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* AND.L (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c0a8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* AND.L (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c0b0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* AND.L (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c0b8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* AND.L (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c0b9_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}}	m68k_incpc (6);
return 10 * CYCLE_UNIT / 2;
}

/* AND.L (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c0ba_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* AND.L (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c0bb_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	m68k_dreg (regs, dstreg) = (src);
}}}}}return 11 * CYCLE_UNIT / 2;
}

/* AND.L #<data>.L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c0bc_40)(uae_u32 opcode)
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
return 8 * CYCLE_UNIT / 2;
}

/* MULU.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c0c0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	m68k_dreg (regs, dstreg) = (newv);
	m68k_incpc (2);
}}}}return 28 * CYCLE_UNIT / 2;
}

/* MULU.W (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c0d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	m68k_dreg (regs, dstreg) = (newv);
	m68k_incpc (2);
}}}}}return 32 * CYCLE_UNIT / 2;
}

/* MULU.W (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c0d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) += 2;
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	m68k_dreg (regs, dstreg) = (newv);
	m68k_incpc (2);
}}}}}return 32 * CYCLE_UNIT / 2;
}

/* MULU.W -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c0e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	m68k_dreg (regs, dstreg) = (newv);
	m68k_incpc (2);
}}}}}return 33 * CYCLE_UNIT / 2;
}

/* MULU.W (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c0e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	m68k_dreg (regs, dstreg) = (newv);
	m68k_incpc (4);
}}}}}return 34 * CYCLE_UNIT / 2;
}

/* MULU.W (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c0f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}}return 36 * CYCLE_UNIT / 2;
}

/* MULU.W (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c0f8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	m68k_dreg (regs, dstreg) = (newv);
	m68k_incpc (4);
}}}}}return 34 * CYCLE_UNIT / 2;
}

/* MULU.W (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c0f9_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	m68k_dreg (regs, dstreg) = (newv);
	m68k_incpc (6);
}}}}}return 35 * CYCLE_UNIT / 2;
}

/* MULU.W (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c0fa_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	m68k_dreg (regs, dstreg) = (newv);
	m68k_incpc (4);
}}}}}return 34 * CYCLE_UNIT / 2;
}

/* MULU.W (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c0fb_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}}return 36 * CYCLE_UNIT / 2;
}

/* MULU.W #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c0fc_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	m68k_dreg (regs, dstreg) = (newv);
	m68k_incpc (4);
}}}}return 31 * CYCLE_UNIT / 2;
}

/* ABCD.B Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c100_40)(uae_u32 opcode)
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
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}	m68k_incpc (2);
return 5 * CYCLE_UNIT / 2;
}

/* ABCD.B -(An),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_c108_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte_jit (dsta);
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
	put_byte_jit (dsta,newv);
}}}}}}	m68k_incpc (2);
return 17 * CYCLE_UNIT / 2;
}

/* AND.B Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_c110_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* AND.B Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_c118_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* AND.B Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_c120_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

/* AND.B Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_c128_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* AND.B Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_c130_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	m68k_incpc (2);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}}return 14 * CYCLE_UNIT / 2;
}

/* AND.B Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_c138_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* AND.B Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_c139_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s8 dst = get_byte_jit (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s8)(src)) == 0);
	SET_NFLG   (((uae_s8)(src)) < 0);
	put_byte_jit (dsta,src);
}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* EXG.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c140_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
	m68k_dreg (regs, srcreg) = (dst);
	m68k_dreg (regs, dstreg) = (src);
}}}	m68k_incpc (2);
return 3 * CYCLE_UNIT / 2;
}

/* EXG.L An,An */
uae_u32 REGPARAM2 CPUFUNC(op_c148_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_areg (regs, srcreg);
{	uae_s32 dst = m68k_areg (regs, dstreg);
	m68k_areg (regs, srcreg) = (dst);
	m68k_areg (regs, dstreg) = (src);
}}}	m68k_incpc (2);
return 3 * CYCLE_UNIT / 2;
}

/* AND.W Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_c150_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word_jit (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* AND.W Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_c158_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word_jit (dsta);
	m68k_areg (regs, dstreg) += 2;
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* AND.W Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_c160_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
{	uae_s16 dst = get_word_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

/* AND.W Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_c168_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 dst = get_word_jit (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* AND.W Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_c170_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	m68k_incpc (2);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s16 dst = get_word_jit (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}}return 14 * CYCLE_UNIT / 2;
}

/* AND.W Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_c178_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 dst = get_word_jit (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* AND.W Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_c179_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s16 dst = get_word_jit (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(src)) == 0);
	SET_NFLG   (((uae_s16)(src)) < 0);
	put_word_jit (dsta,src);
}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* EXG.L Dn,An */
uae_u32 REGPARAM2 CPUFUNC(op_c188_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uae_s32 dst = m68k_areg (regs, dstreg);
	m68k_dreg (regs, srcreg) = (dst);
	m68k_areg (regs, dstreg) = (src);
}}}	m68k_incpc (2);
return 3 * CYCLE_UNIT / 2;
}

/* AND.L Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_c190_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long_jit (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* AND.L Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_c198_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long_jit (dsta);
	m68k_areg (regs, dstreg) += 4;
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* AND.L Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_c1a0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
{	uae_s32 dst = get_long_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

/* AND.L Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_c1a8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 dst = get_long_jit (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* AND.L Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_c1b0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	m68k_incpc (2);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s32 dst = get_long_jit (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}}return 14 * CYCLE_UNIT / 2;
}

/* AND.L Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_c1b8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 dst = get_long_jit (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* AND.L Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_c1b9_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s32 dst = get_long_jit (dsta);
	src &= dst;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(src)) == 0);
	SET_NFLG   (((uae_s32)(src)) < 0);
	put_long_jit (dsta,src);
}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* MULS.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c1c0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}	m68k_incpc (2);
return 28 * CYCLE_UNIT / 2;
}

/* MULS.W (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c1d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (2);
return 32 * CYCLE_UNIT / 2;
}

/* MULS.W (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c1d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) += 2;
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (2);
return 32 * CYCLE_UNIT / 2;
}

/* MULS.W -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c1e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (2);
return 33 * CYCLE_UNIT / 2;
}

/* MULS.W (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c1e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (4);
return 34 * CYCLE_UNIT / 2;
}

/* MULS.W (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c1f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}}return 36 * CYCLE_UNIT / 2;
}

/* MULS.W (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c1f8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (4);
return 34 * CYCLE_UNIT / 2;
}

/* MULS.W (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c1f9_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (6);
return 35 * CYCLE_UNIT / 2;
}

/* MULS.W (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c1fa_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (4);
return 34 * CYCLE_UNIT / 2;
}

/* MULS.W (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c1fb_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}}return 36 * CYCLE_UNIT / 2;
}

/* MULS.W #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c1fc_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s32)(newv)) == 0);
	SET_NFLG   (((uae_s32)(newv)) < 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}	m68k_incpc (4);
return 31 * CYCLE_UNIT / 2;
}

/* ADD.B Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d000_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
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
return 3 * CYCLE_UNIT / 2;
}

/* ADD.B (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d010_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
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
return 7 * CYCLE_UNIT / 2;
}

/* ADD.B (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d018_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) += areg_byteinc[srcreg];
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
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
return 7 * CYCLE_UNIT / 2;
}

/* ADD.B -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d020_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
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

/* ADD.B (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d028_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
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
return 9 * CYCLE_UNIT / 2;
}

/* ADD.B (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d030_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s8 src = get_byte_jit (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}}return 11 * CYCLE_UNIT / 2;
}

/* ADD.B (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d038_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
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
return 9 * CYCLE_UNIT / 2;
}

/* ADD.B (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d039_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte_jit (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
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
return 10 * CYCLE_UNIT / 2;
}

/* ADD.B (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d03a_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
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
return 9 * CYCLE_UNIT / 2;
}

/* ADD.B (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d03b_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s8 src = get_byte_jit (srca);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}}return 11 * CYCLE_UNIT / 2;
}

/* ADD.B #<data>.B,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d03c_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = get_dibyte (2);
{	uae_s8 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
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
return 6 * CYCLE_UNIT / 2;
}

/* ADD.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d040_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
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
return 3 * CYCLE_UNIT / 2;
}

/* ADD.W An,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d048_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_areg (regs, srcreg);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
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
return 3 * CYCLE_UNIT / 2;
}

/* ADD.W (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d050_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
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
return 7 * CYCLE_UNIT / 2;
}

/* ADD.W (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d058_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) += 2;
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
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
return 7 * CYCLE_UNIT / 2;
}

/* ADD.W -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d060_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
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

/* ADD.W (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d068_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
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
return 9 * CYCLE_UNIT / 2;
}

/* ADD.W (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d070_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}}return 11 * CYCLE_UNIT / 2;
}

/* ADD.W (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d078_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
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
return 9 * CYCLE_UNIT / 2;
}

/* ADD.W (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d079_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
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
return 10 * CYCLE_UNIT / 2;
}

/* ADD.W (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d07a_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
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
return 9 * CYCLE_UNIT / 2;
}

/* ADD.W (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d07b_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s16 src = get_word_jit (srca);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}}return 11 * CYCLE_UNIT / 2;
}

/* ADD.W #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d07c_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s16 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
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
return 6 * CYCLE_UNIT / 2;
}

/* ADD.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d080_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
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
return 3 * CYCLE_UNIT / 2;
}

/* ADD.L An,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d088_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_areg (regs, srcreg);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
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
return 3 * CYCLE_UNIT / 2;
}

/* ADD.L (An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d090_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
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
return 7 * CYCLE_UNIT / 2;
}

/* ADD.L (An)+,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d098_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) += 4;
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
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
return 7 * CYCLE_UNIT / 2;
}

/* ADD.L -(An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d0a0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
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
return 8 * CYCLE_UNIT / 2;
}

/* ADD.L (d16,An),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d0a8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
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
return 9 * CYCLE_UNIT / 2;
}

/* ADD.L (d8,An,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d0b0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}}}}return 11 * CYCLE_UNIT / 2;
}

/* ADD.L (xxx).W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d0b8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
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
return 9 * CYCLE_UNIT / 2;
}

/* ADD.L (xxx).L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d0b9_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
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
return 10 * CYCLE_UNIT / 2;
}

/* ADD.L (d16,PC),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d0ba_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
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
return 9 * CYCLE_UNIT / 2;
}

/* ADD.L (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d0bb_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	m68k_dreg (regs, dstreg) = (newv);
}}}}}}}}return 11 * CYCLE_UNIT / 2;
}

/* ADD.L #<data>.L,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d0bc_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uae_s32 dst = m68k_dreg (regs, dstreg);
{{uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
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
return 8 * CYCLE_UNIT / 2;
}

/* ADDA.W Dn,An */
uae_u32 REGPARAM2 CPUFUNC(op_d0c0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}	m68k_incpc (2);
return 3 * CYCLE_UNIT / 2;
}

/* ADDA.W An,An */
uae_u32 REGPARAM2 CPUFUNC(op_d0c8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = m68k_areg (regs, srcreg);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}	m68k_incpc (2);
return 3 * CYCLE_UNIT / 2;
}

/* ADDA.W (An),An */
uae_u32 REGPARAM2 CPUFUNC(op_d0d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (2);
return 7 * CYCLE_UNIT / 2;
}

/* ADDA.W (An)+,An */
uae_u32 REGPARAM2 CPUFUNC(op_d0d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) += 2;
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (2);
return 7 * CYCLE_UNIT / 2;
}

/* ADDA.W -(An),An */
uae_u32 REGPARAM2 CPUFUNC(op_d0e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* ADDA.W (d16,An),An */
uae_u32 REGPARAM2 CPUFUNC(op_d0e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* ADDA.W (d8,An,Xn),An */
uae_u32 REGPARAM2 CPUFUNC(op_d0f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s16 src = get_word_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}}return 12 * CYCLE_UNIT / 2;
}

/* ADDA.W (xxx).W,An */
uae_u32 REGPARAM2 CPUFUNC(op_d0f8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* ADDA.W (xxx).L,An */
uae_u32 REGPARAM2 CPUFUNC(op_d0f9_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (6);
return 10 * CYCLE_UNIT / 2;
}

/* ADDA.W (d16,PC),An */
uae_u32 REGPARAM2 CPUFUNC(op_d0fa_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 src = get_word_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* ADDA.W (d8,PC,Xn),An */
uae_u32 REGPARAM2 CPUFUNC(op_d0fb_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s16 src = get_word_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}}return 11 * CYCLE_UNIT / 2;
}

/* ADDA.W #<data>.W,An */
uae_u32 REGPARAM2 CPUFUNC(op_d0fc_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = get_diword (2);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}	m68k_incpc (4);
return 6 * CYCLE_UNIT / 2;
}

/* ADDX.B Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d100_40)(uae_u32 opcode)
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
return 3 * CYCLE_UNIT / 2;
}

/* ADDX.B -(An),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_d108_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte_jit (dsta);
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
	put_byte_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 13 * CYCLE_UNIT / 2;
}

/* ADD.B Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_d110_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
{{uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* ADD.B Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_d118_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) += areg_byteinc[dstreg];
{{uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* ADD.B Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_d120_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

/* ADD.B Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_d128_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte_jit (dsta);
{{uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* ADD.B Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_d130_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	m68k_incpc (2);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s8 dst = get_byte_jit (dsta);
{{uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}}return 14 * CYCLE_UNIT / 2;
}

/* ADD.B Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_d138_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 dst = get_byte_jit (dsta);
{{uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* ADD.B Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_d139_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s8 dst = get_byte_jit (dsta);
{{uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG (((uae_s8)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_byte_jit (dsta,newv);
}}}}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* ADDX.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d140_40)(uae_u32 opcode)
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
return 3 * CYCLE_UNIT / 2;
}

/* ADDX.W -(An),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_d148_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 2;
{	uae_s16 src = get_word_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
{	uae_s16 dst = get_word_jit (dsta);
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
	put_word_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 13 * CYCLE_UNIT / 2;
}

/* ADD.W Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_d150_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word_jit (dsta);
{{uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* ADD.W Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_d158_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s16 dst = get_word_jit (dsta);
	m68k_areg (regs, dstreg) += 2;
{{uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* ADD.W Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_d160_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 2;
{	uae_s16 dst = get_word_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

/* ADD.W Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_d168_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 dst = get_word_jit (dsta);
{{uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* ADD.W Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_d170_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	m68k_incpc (2);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s16 dst = get_word_jit (dsta);
{{uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}}return 14 * CYCLE_UNIT / 2;
}

/* ADD.W Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_d178_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 dst = get_word_jit (dsta);
{{uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* ADD.W Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_d179_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s16 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s16 dst = get_word_jit (dsta);
{{uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG (((uae_s16)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_word_jit (dsta,newv);
}}}}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* ADDX.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_d180_40)(uae_u32 opcode)
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
return 3 * CYCLE_UNIT / 2;
}

/* ADDX.L -(An),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_d188_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
{	uae_s32 dst = get_long_jit (dsta);
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
	put_long_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 13 * CYCLE_UNIT / 2;
}

/* ADD.L Dn,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_d190_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long_jit (dsta);
{{uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* ADD.L Dn,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_d198_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_s32 dst = get_long_jit (dsta);
	m68k_areg (regs, dstreg) += 4;
{{uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* ADD.L Dn,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_d1a0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - 4;
{	uae_s32 dst = get_long_jit (dsta);
	m68k_areg (regs, dstreg) = dsta;
{{uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

/* ADD.L Dn,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_d1a8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 dst = get_long_jit (dsta);
{{uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* ADD.L Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_d1b0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	m68k_incpc (2);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_s32 dst = get_long_jit (dsta);
{{uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}}return 14 * CYCLE_UNIT / 2;
}

/* ADD.L Dn,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_d1b8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 dst = get_long_jit (dsta);
{{uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* ADD.L Dn,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_d1b9_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uaecptr dsta;
	dsta = get_dilong (2);
{	uae_s32 dst = get_long_jit (dsta);
{{uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG (((uae_s32)(newv)) == 0);
	SET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY ();
	SET_NFLG (flgn != 0);
	put_long_jit (dsta,newv);
}}}}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* ADDA.L Dn,An */
uae_u32 REGPARAM2 CPUFUNC(op_d1c0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_dreg (regs, srcreg);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}	m68k_incpc (2);
return 3 * CYCLE_UNIT / 2;
}

/* ADDA.L An,An */
uae_u32 REGPARAM2 CPUFUNC(op_d1c8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = m68k_areg (regs, srcreg);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}	m68k_incpc (2);
return 3 * CYCLE_UNIT / 2;
}

/* ADDA.L (An),An */
uae_u32 REGPARAM2 CPUFUNC(op_d1d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (2);
return 7 * CYCLE_UNIT / 2;
}

/* ADDA.L (An)+,An */
uae_u32 REGPARAM2 CPUFUNC(op_d1d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) += 4;
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (2);
return 7 * CYCLE_UNIT / 2;
}

/* ADDA.L -(An),An */
uae_u32 REGPARAM2 CPUFUNC(op_d1e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - 4;
{	uae_s32 src = get_long_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* ADDA.L (d16,An),An */
uae_u32 REGPARAM2 CPUFUNC(op_d1e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* ADDA.L (d8,An,Xn),An */
uae_u32 REGPARAM2 CPUFUNC(op_d1f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}}return 11 * CYCLE_UNIT / 2;
}

/* ADDA.L (xxx).W,An */
uae_u32 REGPARAM2 CPUFUNC(op_d1f8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* ADDA.L (xxx).L,An */
uae_u32 REGPARAM2 CPUFUNC(op_d1f9_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (6);
return 10 * CYCLE_UNIT / 2;
}

/* ADDA.L (d16,PC),An */
uae_u32 REGPARAM2 CPUFUNC(op_d1fa_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}	m68k_incpc (4);
return 9 * CYCLE_UNIT / 2;
}

/* ADDA.L (d8,PC,Xn),An */
uae_u32 REGPARAM2 CPUFUNC(op_d1fb_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr tmppc;
	uaecptr srca;
	m68k_incpc (2);
{	tmppc = m68k_getpc ();
	srca = get_disp_ea_020 (tmppc, 0);
{	uae_s32 src = get_long_jit (srca);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}}}return 11 * CYCLE_UNIT / 2;
}

/* ADDA.L #<data>.L,An */
uae_u32 REGPARAM2 CPUFUNC(op_d1fc_40)(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src;
	src = get_dilong (2);
{	uae_s32 dst = m68k_areg (regs, dstreg);
{	uae_u32 newv = dst + src;
	m68k_areg (regs, dstreg) = (newv);
}}}}	m68k_incpc (6);
return 8 * CYCLE_UNIT / 2;
}

/* ASRQ.B #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e000_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* LSRQ.B #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e008_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* ROXRQ.B #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e010_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* RORQ.B #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e018_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* ASR.B Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e020_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* LSR.B Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e028_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* ROXR.B Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e030_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* ROR.B Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e038_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* ASRQ.W #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e040_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

#endif

#ifdef PART_8
/* LSRQ.W #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e048_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* ROXRQ.W #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e050_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* RORQ.W #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e058_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* ASR.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e060_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* LSR.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e068_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* ROXR.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e070_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* ROR.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e078_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* ASRQ.L #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e080_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (val);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* LSRQ.L #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e088_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (val);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* ROXRQ.L #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e090_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (val);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* RORQ.L #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e098_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* ASR.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e0a0_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (val);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* LSR.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e0a8_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (val);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* ROXR.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e0b0_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (val);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* ROR.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e0b8_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* ASRW.W (An) */
uae_u32 REGPARAM2 CPUFUNC(op_e0d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg);
{	uae_s16 data = get_word_jit (dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 cflg = val & 1;
	val = (val >> 1) | sign;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (cflg);
	COPY_CARRY ();
	put_word_jit (dataa,val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* ASRW.W (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_e0d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg);
{	uae_s16 data = get_word_jit (dataa);
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
	put_word_jit (dataa,val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* ASRW.W -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_e0e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg) - 2;
{	uae_s16 data = get_word_jit (dataa);
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
	put_word_jit (dataa,val);
}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

/* ASRW.W (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_e0e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 data = get_word_jit (dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 cflg = val & 1;
	val = (val >> 1) | sign;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (cflg);
	COPY_CARRY ();
	put_word_jit (dataa,val);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* ASRW.W (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_e0f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	m68k_incpc (2);
{	dataa = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s16 data = get_word_jit (dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 cflg = val & 1;
	val = (val >> 1) | sign;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (cflg);
	COPY_CARRY ();
	put_word_jit (dataa,val);
}}}}}return 14 * CYCLE_UNIT / 2;
}

/* ASRW.W (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_e0f8_40)(uae_u32 opcode)
{
{{	uaecptr dataa;
	dataa = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 data = get_word_jit (dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 cflg = val & 1;
	val = (val >> 1) | sign;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (cflg);
	COPY_CARRY ();
	put_word_jit (dataa,val);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* ASRW.W (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_e0f9_40)(uae_u32 opcode)
{
{{	uaecptr dataa;
	dataa = get_dilong (2);
{	uae_s16 data = get_word_jit (dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 cflg = val & 1;
	val = (val >> 1) | sign;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (cflg);
	COPY_CARRY ();
	put_word_jit (dataa,val);
}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* ASLQ.B #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e100_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* LSLQ.B #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e108_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* ROXLQ.B #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e110_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* ROLQ.B #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e118_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* ASL.B Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e120_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* LSL.B Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e128_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* ROXL.B Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e130_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* ROL.B Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e138_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xff) | ((val) & 0xff);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* ASLQ.W #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e140_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* LSLQ.W #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e148_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* ROXLQ.W #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e150_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* ROLQ.W #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e158_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* ASL.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e160_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* LSL.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e168_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* ROXL.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e170_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* ROL.W Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e178_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (m68k_dreg (regs, dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* ASLQ.L #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e180_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* LSLQ.L #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e188_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (val);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* ROXLQ.L #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e190_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (val);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* ROLQ.L #<data>,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e198_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* ASL.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e1a0_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* LSL.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e1a8_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (val);
}}}}	m68k_incpc (2);
return 6 * CYCLE_UNIT / 2;
}

/* ROXL.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e1b0_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (val);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* ROL.L Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e1b8_40)(uae_u32 opcode)
{
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
	m68k_dreg (regs, dstreg) = (val);
}}}}	m68k_incpc (2);
return 8 * CYCLE_UNIT / 2;
}

/* ASLW.W (An) */
uae_u32 REGPARAM2 CPUFUNC(op_e1d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg);
{	uae_s16 data = get_word_jit (dataa);
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
	put_word_jit (dataa,val);
}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

/* ASLW.W (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_e1d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg);
{	uae_s16 data = get_word_jit (dataa);
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
	put_word_jit (dataa,val);
}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

/* ASLW.W -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_e1e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg) - 2;
{	uae_s16 data = get_word_jit (dataa);
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
	put_word_jit (dataa,val);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* ASLW.W (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_e1e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 data = get_word_jit (dataa);
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
	put_word_jit (dataa,val);
}}}}	m68k_incpc (4);
return 13 * CYCLE_UNIT / 2;
}

/* ASLW.W (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_e1f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	m68k_incpc (2);
{	dataa = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s16 data = get_word_jit (dataa);
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
	put_word_jit (dataa,val);
}}}}}return 15 * CYCLE_UNIT / 2;
}

/* ASLW.W (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_e1f8_40)(uae_u32 opcode)
{
{{	uaecptr dataa;
	dataa = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 data = get_word_jit (dataa);
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
	put_word_jit (dataa,val);
}}}}	m68k_incpc (4);
return 13 * CYCLE_UNIT / 2;
}

/* ASLW.W (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_e1f9_40)(uae_u32 opcode)
{
{{	uaecptr dataa;
	dataa = get_dilong (2);
{	uae_s16 data = get_word_jit (dataa);
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
	put_word_jit (dataa,val);
}}}}	m68k_incpc (6);
return 14 * CYCLE_UNIT / 2;
}

/* LSRW.W (An) */
uae_u32 REGPARAM2 CPUFUNC(op_e2d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg);
{	uae_s16 data = get_word_jit (dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 carry = val & 1;
	val >>= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry);
	COPY_CARRY ();
	put_word_jit (dataa,val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* LSRW.W (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_e2d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg);
{	uae_s16 data = get_word_jit (dataa);
	m68k_areg (regs, srcreg) += 2;
{	uae_u32 val = (uae_u16)data;
	uae_u32 carry = val & 1;
	val >>= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry);
	COPY_CARRY ();
	put_word_jit (dataa,val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* LSRW.W -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_e2e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg) - 2;
{	uae_s16 data = get_word_jit (dataa);
	m68k_areg (regs, srcreg) = dataa;
{	uae_u32 val = (uae_u16)data;
	uae_u32 carry = val & 1;
	val >>= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry);
	COPY_CARRY ();
	put_word_jit (dataa,val);
}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

/* LSRW.W (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_e2e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 data = get_word_jit (dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 carry = val & 1;
	val >>= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry);
	COPY_CARRY ();
	put_word_jit (dataa,val);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* LSRW.W (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_e2f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	m68k_incpc (2);
{	dataa = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s16 data = get_word_jit (dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 carry = val & 1;
	val >>= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry);
	COPY_CARRY ();
	put_word_jit (dataa,val);
}}}}}return 14 * CYCLE_UNIT / 2;
}

/* LSRW.W (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_e2f8_40)(uae_u32 opcode)
{
{{	uaecptr dataa;
	dataa = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 data = get_word_jit (dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 carry = val & 1;
	val >>= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry);
	COPY_CARRY ();
	put_word_jit (dataa,val);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* LSRW.W (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_e2f9_40)(uae_u32 opcode)
{
{{	uaecptr dataa;
	dataa = get_dilong (2);
{	uae_s16 data = get_word_jit (dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 carry = val & 1;
	val >>= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry);
	COPY_CARRY ();
	put_word_jit (dataa,val);
}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* LSLW.W (An) */
uae_u32 REGPARAM2 CPUFUNC(op_e3d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg);
{	uae_s16 data = get_word_jit (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry >> 15);
	COPY_CARRY ();
	put_word_jit (dataa,val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* LSLW.W (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_e3d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg);
{	uae_s16 data = get_word_jit (dataa);
	m68k_areg (regs, srcreg) += 2;
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry >> 15);
	COPY_CARRY ();
	put_word_jit (dataa,val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* LSLW.W -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_e3e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg) - 2;
{	uae_s16 data = get_word_jit (dataa);
	m68k_areg (regs, srcreg) = dataa;
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry >> 15);
	COPY_CARRY ();
	put_word_jit (dataa,val);
}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

/* LSLW.W (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_e3e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 data = get_word_jit (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry >> 15);
	COPY_CARRY ();
	put_word_jit (dataa,val);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* LSLW.W (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_e3f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	m68k_incpc (2);
{	dataa = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s16 data = get_word_jit (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry >> 15);
	COPY_CARRY ();
	put_word_jit (dataa,val);
}}}}}return 14 * CYCLE_UNIT / 2;
}

/* LSLW.W (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_e3f8_40)(uae_u32 opcode)
{
{{	uaecptr dataa;
	dataa = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 data = get_word_jit (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry >> 15);
	COPY_CARRY ();
	put_word_jit (dataa,val);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* LSLW.W (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_e3f9_40)(uae_u32 opcode)
{
{{	uaecptr dataa;
	dataa = get_dilong (2);
{	uae_s16 data = get_word_jit (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry >> 15);
	COPY_CARRY ();
	put_word_jit (dataa,val);
}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* ROXRW.W (An) */
uae_u32 REGPARAM2 CPUFUNC(op_e4d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg);
{	uae_s16 data = get_word_jit (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (GET_XFLG ()) val |= 0x8000;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry);
	COPY_CARRY ();
	put_word_jit (dataa,val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* ROXRW.W (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_e4d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg);
{	uae_s16 data = get_word_jit (dataa);
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
	put_word_jit (dataa,val);
}}}}	m68k_incpc (2);
return 13 * CYCLE_UNIT / 2;
}

/* ROXRW.W -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_e4e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg) - 2;
{	uae_s16 data = get_word_jit (dataa);
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
	put_word_jit (dataa,val);
}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

/* ROXRW.W (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_e4e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 data = get_word_jit (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (GET_XFLG ()) val |= 0x8000;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry);
	COPY_CARRY ();
	put_word_jit (dataa,val);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* ROXRW.W (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_e4f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	m68k_incpc (2);
{	dataa = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s16 data = get_word_jit (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (GET_XFLG ()) val |= 0x8000;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry);
	COPY_CARRY ();
	put_word_jit (dataa,val);
}}}}}return 14 * CYCLE_UNIT / 2;
}

/* ROXRW.W (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_e4f8_40)(uae_u32 opcode)
{
{{	uaecptr dataa;
	dataa = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 data = get_word_jit (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (GET_XFLG ()) val |= 0x8000;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry);
	COPY_CARRY ();
	put_word_jit (dataa,val);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* ROXRW.W (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_e4f9_40)(uae_u32 opcode)
{
{{	uaecptr dataa;
	dataa = get_dilong (2);
{	uae_s16 data = get_word_jit (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (GET_XFLG ()) val |= 0x8000;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry);
	COPY_CARRY ();
	put_word_jit (dataa,val);
}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* ROXLW.W (An) */
uae_u32 REGPARAM2 CPUFUNC(op_e5d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg);
{	uae_s16 data = get_word_jit (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (GET_XFLG ()) val |= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry >> 15);
	COPY_CARRY ();
	put_word_jit (dataa,val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* ROXLW.W (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_e5d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg);
{	uae_s16 data = get_word_jit (dataa);
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
	put_word_jit (dataa,val);
}}}}	m68k_incpc (2);
return 10 * CYCLE_UNIT / 2;
}

/* ROXLW.W -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_e5e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg) - 2;
{	uae_s16 data = get_word_jit (dataa);
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
	put_word_jit (dataa,val);
}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

/* ROXLW.W (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_e5e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 data = get_word_jit (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (GET_XFLG ()) val |= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry >> 15);
	COPY_CARRY ();
	put_word_jit (dataa,val);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* ROXLW.W (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_e5f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	m68k_incpc (2);
{	dataa = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s16 data = get_word_jit (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (GET_XFLG ()) val |= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry >> 15);
	COPY_CARRY ();
	put_word_jit (dataa,val);
}}}}}return 14 * CYCLE_UNIT / 2;
}

/* ROXLW.W (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_e5f8_40)(uae_u32 opcode)
{
{{	uaecptr dataa;
	dataa = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 data = get_word_jit (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (GET_XFLG ()) val |= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry >> 15);
	COPY_CARRY ();
	put_word_jit (dataa,val);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* ROXLW.W (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_e5f9_40)(uae_u32 opcode)
{
{{	uaecptr dataa;
	dataa = get_dilong (2);
{	uae_s16 data = get_word_jit (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (GET_XFLG ()) val |= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry >> 15);
	COPY_CARRY ();
	put_word_jit (dataa,val);
}}}}	m68k_incpc (6);
return 13 * CYCLE_UNIT / 2;
}

/* RORW.W (An) */
uae_u32 REGPARAM2 CPUFUNC(op_e6d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg);
{	uae_s16 data = get_word_jit (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (carry) val |= 0x8000;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry);
	put_word_jit (dataa,val);
}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

/* RORW.W (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_e6d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg);
{	uae_s16 data = get_word_jit (dataa);
	m68k_areg (regs, srcreg) += 2;
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (carry) val |= 0x8000;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry);
	put_word_jit (dataa,val);
}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

/* RORW.W -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_e6e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg) - 2;
{	uae_s16 data = get_word_jit (dataa);
	m68k_areg (regs, srcreg) = dataa;
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (carry) val |= 0x8000;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry);
	put_word_jit (dataa,val);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* RORW.W (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_e6e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 data = get_word_jit (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (carry) val |= 0x8000;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry);
	put_word_jit (dataa,val);
}}}}	m68k_incpc (4);
return 13 * CYCLE_UNIT / 2;
}

/* RORW.W (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_e6f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	m68k_incpc (2);
{	dataa = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s16 data = get_word_jit (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (carry) val |= 0x8000;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry);
	put_word_jit (dataa,val);
}}}}}return 15 * CYCLE_UNIT / 2;
}

/* RORW.W (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_e6f8_40)(uae_u32 opcode)
{
{{	uaecptr dataa;
	dataa = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 data = get_word_jit (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (carry) val |= 0x8000;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry);
	put_word_jit (dataa,val);
}}}}	m68k_incpc (4);
return 13 * CYCLE_UNIT / 2;
}

/* RORW.W (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_e6f9_40)(uae_u32 opcode)
{
{{	uaecptr dataa;
	dataa = get_dilong (2);
{	uae_s16 data = get_word_jit (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (carry) val |= 0x8000;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry);
	put_word_jit (dataa,val);
}}}}	m68k_incpc (6);
return 14 * CYCLE_UNIT / 2;
}

/* ROLW.W (An) */
uae_u32 REGPARAM2 CPUFUNC(op_e7d0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg);
{	uae_s16 data = get_word_jit (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (carry)  val |= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry >> 15);
	put_word_jit (dataa,val);
}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

/* ROLW.W (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_e7d8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg);
{	uae_s16 data = get_word_jit (dataa);
	m68k_areg (regs, srcreg) += 2;
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (carry)  val |= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry >> 15);
	put_word_jit (dataa,val);
}}}}	m68k_incpc (2);
return 11 * CYCLE_UNIT / 2;
}

/* ROLW.W -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_e7e0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg) - 2;
{	uae_s16 data = get_word_jit (dataa);
	m68k_areg (regs, srcreg) = dataa;
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (carry)  val |= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry >> 15);
	put_word_jit (dataa,val);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* ROLW.W (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_e7e8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	dataa = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 data = get_word_jit (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (carry)  val |= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry >> 15);
	put_word_jit (dataa,val);
}}}}	m68k_incpc (4);
return 13 * CYCLE_UNIT / 2;
}

/* ROLW.W (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_e7f0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa;
	m68k_incpc (2);
{	dataa = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s16 data = get_word_jit (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (carry)  val |= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry >> 15);
	put_word_jit (dataa,val);
}}}}}return 15 * CYCLE_UNIT / 2;
}

/* ROLW.W (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_e7f8_40)(uae_u32 opcode)
{
{{	uaecptr dataa;
	dataa = (uae_s32)(uae_s16)get_diword (2);
{	uae_s16 data = get_word_jit (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (carry)  val |= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry >> 15);
	put_word_jit (dataa,val);
}}}}	m68k_incpc (4);
return 13 * CYCLE_UNIT / 2;
}

/* ROLW.W (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_e7f9_40)(uae_u32 opcode)
{
{{	uaecptr dataa;
	dataa = get_dilong (2);
{	uae_s16 data = get_word_jit (dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (carry)  val |= 1;
	CLEAR_CZNV ();
	SET_ZFLG   (((uae_s16)(val)) == 0);
	SET_NFLG   (((uae_s16)(val)) < 0);
	SET_CFLG (carry >> 15);
	put_word_jit (dataa,val);
}}}}	m68k_incpc (6);
return 14 * CYCLE_UNIT / 2;
}

/* BFTST.L #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e8c0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp = m68k_dreg(regs, dstreg);
	offset &= 0x1f;
	tmp = (tmp << offset) | (tmp >> (32 - offset));
	bdata[0] = tmp & ((1 << (32 - width)) - 1);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
}}}}	m68k_incpc (4);
return 7 * CYCLE_UNIT / 2;
}

/* BFTST.L #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_e8d0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
}}}}	m68k_incpc (4);
return 15 * CYCLE_UNIT / 2;
}

/* BFTST.L #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_e8e8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
}}}}	m68k_incpc (6);
return 25 * CYCLE_UNIT / 2;
}

/* BFTST.L #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_e8f0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
}}}}}return 20 * CYCLE_UNIT / 2;
}

/* BFTST.L #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_e8f8_40)(uae_u32 opcode)
{
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
}}}}	m68k_incpc (6);
return 18 * CYCLE_UNIT / 2;
}

/* BFTST.L #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_e8f9_40)(uae_u32 opcode)
{
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
}}}}	m68k_incpc (8);
return 18 * CYCLE_UNIT / 2;
}

/* BFTST.L #<data>.W,(d16,PC) */
uae_u32 REGPARAM2 CPUFUNC(op_e8fa_40)(uae_u32 opcode)
{
	uae_u32 dstreg = 2;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_getpc () + 4;
	dsta += (uae_s32)(uae_s16)get_diword (4);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
}}}}	m68k_incpc (6);
return 25 * CYCLE_UNIT / 2;
}

/* BFTST.L #<data>.W,(d8,PC,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_e8fb_40)(uae_u32 opcode)
{
	uae_u32 dstreg = 3;
{{	uae_s16 extra = get_diword (2);
{	uaecptr tmppc;
	uaecptr dsta;
	m68k_incpc (4);
{	tmppc = m68k_getpc ();
	dsta = get_disp_ea_020 (tmppc, 0);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
}}}}}return 20 * CYCLE_UNIT / 2;
}

/* BFEXTU.L #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_e9c0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp = m68k_dreg(regs, dstreg);
	offset &= 0x1f;
	tmp = (tmp << offset) | (tmp >> (32 - offset));
	bdata[0] = tmp & ((1 << (32 - width)) - 1);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	m68k_dreg (regs, (extra >> 12) & 7) = tmp;
}}}}	m68k_incpc (4);
return 8 * CYCLE_UNIT / 2;
}

/* BFEXTU.L #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_e9d0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	m68k_dreg (regs, (extra >> 12) & 7) = tmp;
}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* BFEXTU.L #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_e9e8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	m68k_dreg (regs, (extra >> 12) & 7) = tmp;
}}}}	m68k_incpc (6);
return 26 * CYCLE_UNIT / 2;
}

/* BFEXTU.L #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_e9f0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	m68k_dreg (regs, (extra >> 12) & 7) = tmp;
}}}}}return 21 * CYCLE_UNIT / 2;
}

/* BFEXTU.L #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_e9f8_40)(uae_u32 opcode)
{
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	m68k_dreg (regs, (extra >> 12) & 7) = tmp;
}}}}	m68k_incpc (6);
return 18 * CYCLE_UNIT / 2;
}

/* BFEXTU.L #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_e9f9_40)(uae_u32 opcode)
{
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	m68k_dreg (regs, (extra >> 12) & 7) = tmp;
}}}}	m68k_incpc (8);
return 19 * CYCLE_UNIT / 2;
}

/* BFEXTU.L #<data>.W,(d16,PC) */
uae_u32 REGPARAM2 CPUFUNC(op_e9fa_40)(uae_u32 opcode)
{
	uae_u32 dstreg = 2;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_getpc () + 4;
	dsta += (uae_s32)(uae_s16)get_diword (4);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	m68k_dreg (regs, (extra >> 12) & 7) = tmp;
}}}}	m68k_incpc (6);
return 26 * CYCLE_UNIT / 2;
}

/* BFEXTU.L #<data>.W,(d8,PC,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_e9fb_40)(uae_u32 opcode)
{
	uae_u32 dstreg = 3;
{{	uae_s16 extra = get_diword (2);
{	uaecptr tmppc;
	uaecptr dsta;
	m68k_incpc (4);
{	tmppc = m68k_getpc ();
	dsta = get_disp_ea_020 (tmppc, 0);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	m68k_dreg (regs, (extra >> 12) & 7) = tmp;
}}}}}return 21 * CYCLE_UNIT / 2;
}

/* BFCHG.L #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_eac0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp = m68k_dreg(regs, dstreg);
	offset &= 0x1f;
	tmp = (tmp << offset) | (tmp >> (32 - offset));
	bdata[0] = tmp & ((1 << (32 - width)) - 1);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	tmp = tmp ^ (0xffffffffu >> (32 - width));
	tmp = bdata[0] | (tmp << (32 - width));
	m68k_dreg(regs, dstreg) = (tmp >> offset) | (tmp << (32 - offset));
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* BFCHG.L #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_ead0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	tmp = tmp ^ (0xffffffffu >> (32 - width));
	put_bitfield(dsta, bdata, tmp, offset, width);
}}}}	m68k_incpc (4);
return 21 * CYCLE_UNIT / 2;
}

/* BFCHG.L #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_eae8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	tmp = tmp ^ (0xffffffffu >> (32 - width));
	put_bitfield(dsta, bdata, tmp, offset, width);
}}}}	m68k_incpc (6);
return 29 * CYCLE_UNIT / 2;
}

/* BFCHG.L #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_eaf0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	tmp = tmp ^ (0xffffffffu >> (32 - width));
	put_bitfield(dsta, bdata, tmp, offset, width);
}}}}}return 24 * CYCLE_UNIT / 2;
}

/* BFCHG.L #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_eaf8_40)(uae_u32 opcode)
{
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	tmp = tmp ^ (0xffffffffu >> (32 - width));
	put_bitfield(dsta, bdata, tmp, offset, width);
}}}}	m68k_incpc (6);
return 21 * CYCLE_UNIT / 2;
}

/* BFCHG.L #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_eaf9_40)(uae_u32 opcode)
{
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	tmp = tmp ^ (0xffffffffu >> (32 - width));
	put_bitfield(dsta, bdata, tmp, offset, width);
}}}}	m68k_incpc (8);
return 22 * CYCLE_UNIT / 2;
}

/* BFEXTS.L #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_ebc0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp = m68k_dreg(regs, dstreg);
	offset &= 0x1f;
	tmp = (tmp << offset) | (tmp >> (32 - offset));
	bdata[0] = tmp & ((1 << (32 - width)) - 1);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp = (uae_s32)tmp >> (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	m68k_dreg (regs, (extra >> 12) & 7) = tmp;
}}}}	m68k_incpc (4);
return 8 * CYCLE_UNIT / 2;
}

/* BFEXTS.L #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_ebd0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp = (uae_s32)tmp >> (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	m68k_dreg (regs, (extra >> 12) & 7) = tmp;
}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* BFEXTS.L #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_ebe8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp = (uae_s32)tmp >> (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	m68k_dreg (regs, (extra >> 12) & 7) = tmp;
}}}}	m68k_incpc (6);
return 26 * CYCLE_UNIT / 2;
}

/* BFEXTS.L #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_ebf0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp = (uae_s32)tmp >> (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	m68k_dreg (regs, (extra >> 12) & 7) = tmp;
}}}}}return 21 * CYCLE_UNIT / 2;
}

/* BFEXTS.L #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_ebf8_40)(uae_u32 opcode)
{
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp = (uae_s32)tmp >> (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	m68k_dreg (regs, (extra >> 12) & 7) = tmp;
}}}}	m68k_incpc (6);
return 18 * CYCLE_UNIT / 2;
}

/* BFEXTS.L #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_ebf9_40)(uae_u32 opcode)
{
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp = (uae_s32)tmp >> (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	m68k_dreg (regs, (extra >> 12) & 7) = tmp;
}}}}	m68k_incpc (8);
return 19 * CYCLE_UNIT / 2;
}

/* BFEXTS.L #<data>.W,(d16,PC) */
uae_u32 REGPARAM2 CPUFUNC(op_ebfa_40)(uae_u32 opcode)
{
	uae_u32 dstreg = 2;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_getpc () + 4;
	dsta += (uae_s32)(uae_s16)get_diword (4);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp = (uae_s32)tmp >> (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	m68k_dreg (regs, (extra >> 12) & 7) = tmp;
}}}}	m68k_incpc (6);
return 26 * CYCLE_UNIT / 2;
}

/* BFEXTS.L #<data>.W,(d8,PC,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_ebfb_40)(uae_u32 opcode)
{
	uae_u32 dstreg = 3;
{{	uae_s16 extra = get_diword (2);
{	uaecptr tmppc;
	uaecptr dsta;
	m68k_incpc (4);
{	tmppc = m68k_getpc ();
	dsta = get_disp_ea_020 (tmppc, 0);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp = (uae_s32)tmp >> (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	m68k_dreg (regs, (extra >> 12) & 7) = tmp;
}}}}}return 21 * CYCLE_UNIT / 2;
}

/* BFCLR.L #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_ecc0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp = m68k_dreg(regs, dstreg);
	offset &= 0x1f;
	tmp = (tmp << offset) | (tmp >> (32 - offset));
	bdata[0] = tmp & ((1 << (32 - width)) - 1);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	tmp = 0;
	tmp = bdata[0] | (tmp << (32 - width));
	m68k_dreg(regs, dstreg) = (tmp >> offset) | (tmp << (32 - offset));
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* BFCLR.L #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_ecd0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	tmp = 0;
	put_bitfield(dsta, bdata, tmp, offset, width);
}}}}	m68k_incpc (4);
return 21 * CYCLE_UNIT / 2;
}

/* BFCLR.L #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_ece8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	tmp = 0;
	put_bitfield(dsta, bdata, tmp, offset, width);
}}}}	m68k_incpc (6);
return 29 * CYCLE_UNIT / 2;
}

/* BFCLR.L #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_ecf0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	tmp = 0;
	put_bitfield(dsta, bdata, tmp, offset, width);
}}}}}return 24 * CYCLE_UNIT / 2;
}

/* BFCLR.L #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_ecf8_40)(uae_u32 opcode)
{
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	tmp = 0;
	put_bitfield(dsta, bdata, tmp, offset, width);
}}}}	m68k_incpc (6);
return 21 * CYCLE_UNIT / 2;
}

/* BFCLR.L #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_ecf9_40)(uae_u32 opcode)
{
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	tmp = 0;
	put_bitfield(dsta, bdata, tmp, offset, width);
}}}}	m68k_incpc (8);
return 22 * CYCLE_UNIT / 2;
}

/* BFFFO.L #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_edc0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 offset2 = offset;
	uae_u32 tmp = m68k_dreg(regs, dstreg);
	offset &= 0x1f;
	tmp = (tmp << offset) | (tmp >> (32 - offset));
	bdata[0] = tmp & ((1 << (32 - width)) - 1);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	{ uae_u32 mask = 1 << (width - 1);
	while (mask) { if (tmp & mask) break; mask >>= 1; offset2++; }}
	m68k_dreg (regs, (extra >> 12) & 7) = offset2;
}}}}	m68k_incpc (4);
return 18 * CYCLE_UNIT / 2;
}

/* BFFFO.L #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_edd0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 offset2 = offset;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	{ uae_u32 mask = 1 << (width - 1);
	while (mask) { if (tmp & mask) break; mask >>= 1; offset2++; }}
	m68k_dreg (regs, (extra >> 12) & 7) = offset2;
}}}}	m68k_incpc (4);
return 29 * CYCLE_UNIT / 2;
}

/* BFFFO.L #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_ede8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 offset2 = offset;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	{ uae_u32 mask = 1 << (width - 1);
	while (mask) { if (tmp & mask) break; mask >>= 1; offset2++; }}
	m68k_dreg (regs, (extra >> 12) & 7) = offset2;
}}}}	m68k_incpc (6);
return 37 * CYCLE_UNIT / 2;
}

/* BFFFO.L #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_edf0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 offset2 = offset;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	{ uae_u32 mask = 1 << (width - 1);
	while (mask) { if (tmp & mask) break; mask >>= 1; offset2++; }}
	m68k_dreg (regs, (extra >> 12) & 7) = offset2;
}}}}}return 32 * CYCLE_UNIT / 2;
}

/* BFFFO.L #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_edf8_40)(uae_u32 opcode)
{
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 offset2 = offset;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	{ uae_u32 mask = 1 << (width - 1);
	while (mask) { if (tmp & mask) break; mask >>= 1; offset2++; }}
	m68k_dreg (regs, (extra >> 12) & 7) = offset2;
}}}}	m68k_incpc (6);
return 29 * CYCLE_UNIT / 2;
}

/* BFFFO.L #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_edf9_40)(uae_u32 opcode)
{
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 offset2 = offset;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	{ uae_u32 mask = 1 << (width - 1);
	while (mask) { if (tmp & mask) break; mask >>= 1; offset2++; }}
	m68k_dreg (regs, (extra >> 12) & 7) = offset2;
}}}}	m68k_incpc (8);
return 30 * CYCLE_UNIT / 2;
}

/* BFFFO.L #<data>.W,(d16,PC) */
uae_u32 REGPARAM2 CPUFUNC(op_edfa_40)(uae_u32 opcode)
{
	uae_u32 dstreg = 2;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_getpc () + 4;
	dsta += (uae_s32)(uae_s16)get_diword (4);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 offset2 = offset;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	{ uae_u32 mask = 1 << (width - 1);
	while (mask) { if (tmp & mask) break; mask >>= 1; offset2++; }}
	m68k_dreg (regs, (extra >> 12) & 7) = offset2;
}}}}	m68k_incpc (6);
return 37 * CYCLE_UNIT / 2;
}

/* BFFFO.L #<data>.W,(d8,PC,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_edfb_40)(uae_u32 opcode)
{
	uae_u32 dstreg = 3;
{{	uae_s16 extra = get_diword (2);
{	uaecptr tmppc;
	uaecptr dsta;
	m68k_incpc (4);
{	tmppc = m68k_getpc ();
	dsta = get_disp_ea_020 (tmppc, 0);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 offset2 = offset;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	{ uae_u32 mask = 1 << (width - 1);
	while (mask) { if (tmp & mask) break; mask >>= 1; offset2++; }}
	m68k_dreg (regs, (extra >> 12) & 7) = offset2;
}}}}}return 32 * CYCLE_UNIT / 2;
}

/* BFSET.L #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_eec0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp = m68k_dreg(regs, dstreg);
	offset &= 0x1f;
	tmp = (tmp << offset) | (tmp >> (32 - offset));
	bdata[0] = tmp & ((1 << (32 - width)) - 1);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	tmp = 0xffffffffu >> (32 - width);
	tmp = bdata[0] | (tmp << (32 - width));
	m68k_dreg(regs, dstreg) = (tmp >> offset) | (tmp << (32 - offset));
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* BFSET.L #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_eed0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	tmp = 0xffffffffu >> (32 - width);
	put_bitfield(dsta, bdata, tmp, offset, width);
}}}}	m68k_incpc (4);
return 21 * CYCLE_UNIT / 2;
}

/* BFSET.L #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_eee8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	tmp = 0xffffffffu >> (32 - width);
	put_bitfield(dsta, bdata, tmp, offset, width);
}}}}	m68k_incpc (6);
return 29 * CYCLE_UNIT / 2;
}

/* BFSET.L #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_eef0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	tmp = 0xffffffffu >> (32 - width);
	put_bitfield(dsta, bdata, tmp, offset, width);
}}}}}return 24 * CYCLE_UNIT / 2;
}

/* BFSET.L #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_eef8_40)(uae_u32 opcode)
{
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	tmp = 0xffffffffu >> (32 - width);
	put_bitfield(dsta, bdata, tmp, offset, width);
}}}}	m68k_incpc (6);
return 21 * CYCLE_UNIT / 2;
}

/* BFSET.L #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_eef9_40)(uae_u32 opcode)
{
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	tmp = 0xffffffffu >> (32 - width);
	put_bitfield(dsta, bdata, tmp, offset, width);
}}}}	m68k_incpc (8);
return 22 * CYCLE_UNIT / 2;
}

/* BFINS.L #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_efc0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp = m68k_dreg(regs, dstreg);
	offset &= 0x1f;
	tmp = (tmp << offset) | (tmp >> (32 - offset));
	bdata[0] = tmp & ((1 << (32 - width)) - 1);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	tmp = m68k_dreg (regs, (extra >> 12) & 7);
	tmp = tmp & (0xffffffffu >> (32 - width));
	SET_NFLG (tmp & (1 << (width - 1)) ? 1 : 0);
	SET_ZFLG (tmp == 0);
	tmp = bdata[0] | (tmp << (32 - width));
	m68k_dreg(regs, dstreg) = (tmp >> offset) | (tmp << (32 - offset));
}}}}	m68k_incpc (4);
return 10 * CYCLE_UNIT / 2;
}

/* BFINS.L #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_efd0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	tmp = m68k_dreg (regs, (extra >> 12) & 7);
	tmp = tmp & (0xffffffffu >> (32 - width));
	SET_NFLG (tmp & (1 << (width - 1)) ? 1 : 0);
	SET_ZFLG (tmp == 0);
	put_bitfield(dsta, bdata, tmp, offset, width);
}}}}	m68k_incpc (4);
return 20 * CYCLE_UNIT / 2;
}

/* BFINS.L #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_efe8_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	tmp = m68k_dreg (regs, (extra >> 12) & 7);
	tmp = tmp & (0xffffffffu >> (32 - width));
	SET_NFLG (tmp & (1 << (width - 1)) ? 1 : 0);
	SET_ZFLG (tmp == 0);
	put_bitfield(dsta, bdata, tmp, offset, width);
}}}}	m68k_incpc (6);
return 28 * CYCLE_UNIT / 2;
}

/* BFINS.L #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_eff0_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	m68k_incpc (4);
{	dsta = get_disp_ea_020 (m68k_areg (regs, dstreg), 0);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	tmp = m68k_dreg (regs, (extra >> 12) & 7);
	tmp = tmp & (0xffffffffu >> (32 - width));
	SET_NFLG (tmp & (1 << (width - 1)) ? 1 : 0);
	SET_ZFLG (tmp == 0);
	put_bitfield(dsta, bdata, tmp, offset, width);
}}}}}return 23 * CYCLE_UNIT / 2;
}

/* BFINS.L #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_eff8_40)(uae_u32 opcode)
{
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	tmp = m68k_dreg (regs, (extra >> 12) & 7);
	tmp = tmp & (0xffffffffu >> (32 - width));
	SET_NFLG (tmp & (1 << (width - 1)) ? 1 : 0);
	SET_ZFLG (tmp == 0);
	put_bitfield(dsta, bdata, tmp, offset, width);
}}}}	m68k_incpc (6);
return 20 * CYCLE_UNIT / 2;
}

/* BFINS.L #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_eff9_40)(uae_u32 opcode)
{
{{	uae_s16 extra = get_diword (2);
{	uaecptr dsta;
	dsta = get_dilong (4);
{	uae_u32 bdata[2];
	uae_s32 offset = extra & 0x800 ? m68k_dreg(regs, (extra >> 6) & 7) : (extra >> 6) & 0x1f;
	int width = (((extra & 0x20 ? m68k_dreg(regs, extra & 7) : extra) -1) & 0x1f) +1;
	uae_u32 tmp;
	dsta += offset >> 3;
	tmp = get_bitfield (dsta, bdata, offset, width);
	SET_NFLG_ALWAYS (((uae_s32)tmp) < 0 ? 1 : 0);
	tmp >>= (32 - width);
	SET_ZFLG (tmp == 0); SET_VFLG (0); SET_CFLG (0);
	tmp = m68k_dreg (regs, (extra >> 12) & 7);
	tmp = tmp & (0xffffffffu >> (32 - width));
	SET_NFLG (tmp & (1 << (width - 1)) ? 1 : 0);
	SET_ZFLG (tmp == 0);
	put_bitfield(dsta, bdata, tmp, offset, width);
}}}}	m68k_incpc (8);
return 21 * CYCLE_UNIT / 2;
}

/* MMUOP030.L Dn,#<data>.W */
uae_u32 REGPARAM2 CPUFUNC(op_f000_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	uaecptr pc = m68k_getpc ();
	uae_u16 extra = get_diword (2);
	m68k_incpc (4);
	uae_u16 extraa = 0;
	mmu_op30 (pc, opcode, extra, extraa);
}}return 4 * CYCLE_UNIT / 2;
}

/* MMUOP030.L An,#<data>.W */
uae_u32 REGPARAM2 CPUFUNC(op_f008_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	uaecptr pc = m68k_getpc ();
	uae_u16 extra = get_diword (2);
	m68k_incpc (4);
	uae_u16 extraa = 0;
	mmu_op30 (pc, opcode, extra, extraa);
}}return 4 * CYCLE_UNIT / 2;
}

/* MMUOP030.L (An),#<data>.W */
uae_u32 REGPARAM2 CPUFUNC(op_f010_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	uaecptr pc = m68k_getpc ();
	uae_u16 extra = get_diword (2);
	m68k_incpc (4);
{	uaecptr extraa;
	extraa = m68k_areg (regs, srcreg);
	mmu_op30 (pc, opcode, extra, extraa);
}}}return 4 * CYCLE_UNIT / 2;
}

/* MMUOP030.L (An)+,#<data>.W */
uae_u32 REGPARAM2 CPUFUNC(op_f018_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	uaecptr pc = m68k_getpc ();
	uae_u16 extra = get_diword (2);
	m68k_incpc (4);
{	uaecptr extraa;
	extraa = m68k_areg (regs, srcreg);
	m68k_areg (regs, srcreg) += 4;
	mmu_op30 (pc, opcode, extra, extraa);
}}}return 4 * CYCLE_UNIT / 2;
}

/* MMUOP030.L -(An),#<data>.W */
uae_u32 REGPARAM2 CPUFUNC(op_f020_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	uaecptr pc = m68k_getpc ();
	uae_u16 extra = get_diword (2);
	m68k_incpc (4);
{	uaecptr extraa;
	extraa = m68k_areg (regs, srcreg) - 4;
	m68k_areg (regs, srcreg) = extraa;
	mmu_op30 (pc, opcode, extra, extraa);
}}}return 6 * CYCLE_UNIT / 2;
}

/* MMUOP030.L (d16,An),#<data>.W */
uae_u32 REGPARAM2 CPUFUNC(op_f028_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	uaecptr pc = m68k_getpc ();
	uae_u16 extra = get_diword (2);
	m68k_incpc (4);
{	uaecptr extraa;
	extraa = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (0);
	m68k_incpc (2);
	mmu_op30 (pc, opcode, extra, extraa);
}}}return 8 * CYCLE_UNIT / 2;
}

/* MMUOP030.L (d8,An,Xn),#<data>.W */
uae_u32 REGPARAM2 CPUFUNC(op_f030_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	uaecptr pc = m68k_getpc ();
	uae_u16 extra = get_diword (2);
	m68k_incpc (4);
{	uaecptr extraa;
{	extraa = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
	mmu_op30 (pc, opcode, extra, extraa);
}}}}return 8 * CYCLE_UNIT / 2;
}

/* MMUOP030.L (xxx).W,#<data>.W */
uae_u32 REGPARAM2 CPUFUNC(op_f038_40)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	uaecptr pc = m68k_getpc ();
	uae_u16 extra = get_diword (2);
	m68k_incpc (4);
{	uaecptr extraa;
	extraa = (uae_s32)(uae_s16)get_diword (0);
	m68k_incpc (2);
	mmu_op30 (pc, opcode, extra, extraa);
}}}return 8 * CYCLE_UNIT / 2;
}

/* MMUOP030.L (xxx).L,#<data>.W */
uae_u32 REGPARAM2 CPUFUNC(op_f039_40)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	uaecptr pc = m68k_getpc ();
	uae_u16 extra = get_diword (2);
	m68k_incpc (4);
{	uaecptr extraa;
	extraa = get_dilong (0);
	m68k_incpc (4);
	mmu_op30 (pc, opcode, extra, extraa);
}}}return 12 * CYCLE_UNIT / 2;
}

/* FPP.L #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_f200_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{
#ifdef FPUEMU
{	uae_s16 extra = get_diword (2);
	m68k_incpc (4);
	fpuop_arithmetic(opcode, extra);
}
#endif
}return 8 * CYCLE_UNIT / 2;
}

/* FPP.L #<data>.W,An */
uae_u32 REGPARAM2 CPUFUNC(op_f208_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{
#ifdef FPUEMU
{	uae_s16 extra = get_diword (2);
	m68k_incpc (4);
	fpuop_arithmetic(opcode, extra);
}
#endif
}return 8 * CYCLE_UNIT / 2;
}

/* FPP.L #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_f210_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{
#ifdef FPUEMU
{	uae_s16 extra = get_diword (2);
	m68k_incpc (4);
	fpuop_arithmetic(opcode, extra);
}
#endif
}return 8 * CYCLE_UNIT / 2;
}

/* FPP.L #<data>.W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_f218_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{
#ifdef FPUEMU
{	uae_s16 extra = get_diword (2);
	m68k_incpc (4);
	fpuop_arithmetic(opcode, extra);
}
#endif
}return 8 * CYCLE_UNIT / 2;
}

/* FPP.L #<data>.W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_f220_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{
#ifdef FPUEMU
{	uae_s16 extra = get_diword (2);
	m68k_incpc (4);
	fpuop_arithmetic(opcode, extra);
}
#endif
}return 8 * CYCLE_UNIT / 2;
}

/* FPP.L #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_f228_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{
#ifdef FPUEMU
{	uae_s16 extra = get_diword (2);
	m68k_incpc (4);
	fpuop_arithmetic(opcode, extra);
}
#endif
}return 8 * CYCLE_UNIT / 2;
}

/* FPP.L #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_f230_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{
#ifdef FPUEMU
{	uae_s16 extra = get_diword (2);
	m68k_incpc (4);
	fpuop_arithmetic(opcode, extra);
}
#endif
}return 8 * CYCLE_UNIT / 2;
}

/* FPP.L #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_f238_40)(uae_u32 opcode)
{
{
#ifdef FPUEMU
{	uae_s16 extra = get_diword (2);
	m68k_incpc (4);
	fpuop_arithmetic(opcode, extra);
}
#endif
}return 8 * CYCLE_UNIT / 2;
}

/* FPP.L #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_f239_40)(uae_u32 opcode)
{
{
#ifdef FPUEMU
{	uae_s16 extra = get_diword (2);
	m68k_incpc (4);
	fpuop_arithmetic(opcode, extra);
}
#endif
}return 8 * CYCLE_UNIT / 2;
}

/* FPP.L #<data>.W,(d16,PC) */
uae_u32 REGPARAM2 CPUFUNC(op_f23a_40)(uae_u32 opcode)
{
	uae_u32 dstreg = 2;
{
#ifdef FPUEMU
{	uae_s16 extra = get_diword (2);
	m68k_incpc (4);
	fpuop_arithmetic(opcode, extra);
}
#endif
}return 8 * CYCLE_UNIT / 2;
}

/* FPP.L #<data>.W,(d8,PC,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_f23b_40)(uae_u32 opcode)
{
	uae_u32 dstreg = 3;
{
#ifdef FPUEMU
{	uae_s16 extra = get_diword (2);
	m68k_incpc (4);
	fpuop_arithmetic(opcode, extra);
}
#endif
}return 8 * CYCLE_UNIT / 2;
}

/* FPP.L #<data>.W,#<data>.L */
uae_u32 REGPARAM2 CPUFUNC(op_f23c_40)(uae_u32 opcode)
{
{
#ifdef FPUEMU
{	uae_s16 extra = get_diword (2);
	m68k_incpc (4);
	fpuop_arithmetic(opcode, extra);
}
#endif
}return 8 * CYCLE_UNIT / 2;
}

/* FScc.L #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_f240_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{
#ifdef FPUEMU
{	uae_s16 extra = get_diword (2);
	m68k_incpc (4);
	fpuop_scc (opcode, extra);
}
#endif
}return 8 * CYCLE_UNIT / 2;
}

/* FDBcc.L #<data>.W,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_f248_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{
#ifdef FPUEMU
{	uae_s16 extra = get_diword (2);
	m68k_incpc (4);
	fpuop_dbcc (opcode, extra);
	if (regs.fp_branch) {
		regs.fp_branch = false;
		if(regs.t0) check_t0_trace();
	}
}
#endif
}return 8 * CYCLE_UNIT / 2;
}

/* FScc.L #<data>.W,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_f250_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{
#ifdef FPUEMU
{	uae_s16 extra = get_diword (2);
	m68k_incpc (4);
	fpuop_scc (opcode, extra);
}
#endif
}return 8 * CYCLE_UNIT / 2;
}

/* FScc.L #<data>.W,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_f258_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{
#ifdef FPUEMU
{	uae_s16 extra = get_diword (2);
	m68k_incpc (4);
	fpuop_scc (opcode, extra);
}
#endif
}return 8 * CYCLE_UNIT / 2;
}

/* FScc.L #<data>.W,-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_f260_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{
#ifdef FPUEMU
{	uae_s16 extra = get_diword (2);
	m68k_incpc (4);
	fpuop_scc (opcode, extra);
}
#endif
}return 8 * CYCLE_UNIT / 2;
}

/* FScc.L #<data>.W,(d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_f268_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{
#ifdef FPUEMU
{	uae_s16 extra = get_diword (2);
	m68k_incpc (4);
	fpuop_scc (opcode, extra);
}
#endif
}return 8 * CYCLE_UNIT / 2;
}

/* FScc.L #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_f270_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{
#ifdef FPUEMU
{	uae_s16 extra = get_diword (2);
	m68k_incpc (4);
	fpuop_scc (opcode, extra);
}
#endif
}return 8 * CYCLE_UNIT / 2;
}

/* FScc.L #<data>.W,(xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_f278_40)(uae_u32 opcode)
{
{
#ifdef FPUEMU
{	uae_s16 extra = get_diword (2);
	m68k_incpc (4);
	fpuop_scc (opcode, extra);
}
#endif
}return 8 * CYCLE_UNIT / 2;
}

/* FScc.L #<data>.W,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_f279_40)(uae_u32 opcode)
{
{
#ifdef FPUEMU
{	uae_s16 extra = get_diword (2);
	m68k_incpc (4);
	fpuop_scc (opcode, extra);
}
#endif
}return 8 * CYCLE_UNIT / 2;
}

/* FTRAPcc.L #<data>.W */
uae_u32 REGPARAM2 CPUFUNC(op_f27a_40)(uae_u32 opcode)
{
{
#ifdef FPUEMU
	uaecptr oldpc = m68k_getpc ();
	uae_u16 extra = get_diword (2);
{	uae_s16 dummy = get_diword (4);
	m68k_incpc (6);
	fpuop_trapcc (opcode, oldpc, extra);
}
#endif
}return 8 * CYCLE_UNIT / 2;
}

/* FTRAPcc.L #<data>.L */
uae_u32 REGPARAM2 CPUFUNC(op_f27b_40)(uae_u32 opcode)
{
{
#ifdef FPUEMU
	uaecptr oldpc = m68k_getpc ();
	uae_u16 extra = get_diword (2);
{	uae_s32 dummy;
	dummy = get_dilong (4);
	m68k_incpc (8);
	fpuop_trapcc (opcode, oldpc, extra);
}
#endif
}return 12 * CYCLE_UNIT / 2;
}

/* FTRAPcc.L  */
uae_u32 REGPARAM2 CPUFUNC(op_f27c_40)(uae_u32 opcode)
{
{
#ifdef FPUEMU
	uaecptr oldpc = m68k_getpc ();
	uae_u16 extra = get_diword (2);
	m68k_incpc (4);
	fpuop_trapcc (opcode, oldpc, extra);

#endif
}return 4 * CYCLE_UNIT / 2;
}

/* FBccQ.L #<data>,#<data>.W */
uae_u32 REGPARAM2 CPUFUNC(op_f280_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 63);
{
#ifdef FPUEMU
	m68k_incpc (2);
{	uaecptr pc = m68k_getpc ();
{	uae_s16 extra = get_diword (0);
	m68k_incpc (2);
	fpuop_bcc (opcode, pc,extra);
	if (regs.fp_branch) {
		regs.fp_branch = false;
		if(regs.t0) check_t0_trace();
	}
}}
#endif
}return 8 * CYCLE_UNIT / 2;
}

/* FBccQ.L #<data>,#<data>.L */
uae_u32 REGPARAM2 CPUFUNC(op_f2c0_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 63);
{
#ifdef FPUEMU
	m68k_incpc (2);
{	uaecptr pc = m68k_getpc ();
{	uae_s32 extra;
	extra = get_dilong (0);
	m68k_incpc (4);
	fpuop_bcc (opcode, pc,extra);
	if (regs.fp_branch) {
		regs.fp_branch = false;
		if(regs.t0) check_t0_trace();
	}
}}
#endif
}return 12 * CYCLE_UNIT / 2;
}

/* FSAVE.L (An) */
uae_u32 REGPARAM2 CPUFUNC(op_f310_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{
#ifdef FPUEMU
	m68k_incpc (2);
	fpuop_save (opcode);

#endif
}}return 4 * CYCLE_UNIT / 2;
}

/* FSAVE.L -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_f320_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{
#ifdef FPUEMU
	m68k_incpc (2);
	fpuop_save (opcode);

#endif
}}return 4 * CYCLE_UNIT / 2;
}

/* FSAVE.L (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_f328_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{
#ifdef FPUEMU
	m68k_incpc (2);
	fpuop_save (opcode);

#endif
}}return 4 * CYCLE_UNIT / 2;
}

/* FSAVE.L (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_f330_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{
#ifdef FPUEMU
	m68k_incpc (2);
	fpuop_save (opcode);

#endif
}}return 4 * CYCLE_UNIT / 2;
}

/* FSAVE.L (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_f338_40)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{
#ifdef FPUEMU
	m68k_incpc (2);
	fpuop_save (opcode);

#endif
}}return 4 * CYCLE_UNIT / 2;
}

/* FSAVE.L (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_f339_40)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{
#ifdef FPUEMU
	m68k_incpc (2);
	fpuop_save (opcode);

#endif
}}return 4 * CYCLE_UNIT / 2;
}

/* FRESTORE.L (An) */
uae_u32 REGPARAM2 CPUFUNC(op_f350_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{
#ifdef FPUEMU
	m68k_incpc (2);
	fpuop_restore (opcode);

#endif
}}return 4 * CYCLE_UNIT / 2;
}

/* FRESTORE.L (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_f358_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{
#ifdef FPUEMU
	m68k_incpc (2);
	fpuop_restore (opcode);

#endif
}}return 4 * CYCLE_UNIT / 2;
}

/* FRESTORE.L (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_f368_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{
#ifdef FPUEMU
	m68k_incpc (2);
	fpuop_restore (opcode);

#endif
}}return 4 * CYCLE_UNIT / 2;
}

/* FRESTORE.L (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_f370_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{
#ifdef FPUEMU
	m68k_incpc (2);
	fpuop_restore (opcode);

#endif
}}return 4 * CYCLE_UNIT / 2;
}

/* FRESTORE.L (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_f378_40)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{
#ifdef FPUEMU
	m68k_incpc (2);
	fpuop_restore (opcode);

#endif
}}return 4 * CYCLE_UNIT / 2;
}

/* FRESTORE.L (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_f379_40)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{
#ifdef FPUEMU
	m68k_incpc (2);
	fpuop_restore (opcode);

#endif
}}return 4 * CYCLE_UNIT / 2;
}

/* FRESTORE.L (d16,PC) */
uae_u32 REGPARAM2 CPUFUNC(op_f37a_40)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{
#ifdef FPUEMU
	m68k_incpc (2);
	fpuop_restore (opcode);

#endif
}}return 4 * CYCLE_UNIT / 2;
}

/* FRESTORE.L (d8,PC,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_f37b_40)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{
#ifdef FPUEMU
	m68k_incpc (2);
	fpuop_restore (opcode);

#endif
}}return 4 * CYCLE_UNIT / 2;
}

/* CINVLQ.L #<data>,An */
uae_u32 REGPARAM2 CPUFUNC(op_f408_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 6) & 3);
	uae_u32 dstreg = opcode & 7;
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	if (opcode & 0x80)
		flush_icache((opcode >> 6) & 3);
}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* CINVPQ.L #<data>,An */
uae_u32 REGPARAM2 CPUFUNC(op_f410_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 6) & 3);
	uae_u32 dstreg = opcode & 7;
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	if (opcode & 0x80)
		flush_icache((opcode >> 6) & 3);
}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* CINVAQ.L #<data> */
uae_u32 REGPARAM2 CPUFUNC(op_f418_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 6) & 3);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	if (opcode & 0x80)
		flush_icache((opcode >> 6) & 3);
}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* CINVAQ.L #<data> */
uae_u32 REGPARAM2 CPUFUNC(op_f419_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 6) & 3);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	if (opcode & 0x80)
		flush_icache((opcode >> 6) & 3);
}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* CINVAQ.L #<data> */
uae_u32 REGPARAM2 CPUFUNC(op_f41a_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 6) & 3);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	if (opcode & 0x80)
		flush_icache((opcode >> 6) & 3);
}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* CINVAQ.L #<data> */
uae_u32 REGPARAM2 CPUFUNC(op_f41b_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 6) & 3);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	if (opcode & 0x80)
		flush_icache((opcode >> 6) & 3);
}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* CINVAQ.L #<data> */
uae_u32 REGPARAM2 CPUFUNC(op_f41c_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 6) & 3);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	if (opcode & 0x80)
		flush_icache((opcode >> 6) & 3);
}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* CINVAQ.L #<data> */
uae_u32 REGPARAM2 CPUFUNC(op_f41d_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 6) & 3);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	if (opcode & 0x80)
		flush_icache((opcode >> 6) & 3);
}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* CINVAQ.L #<data> */
uae_u32 REGPARAM2 CPUFUNC(op_f41e_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 6) & 3);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	if (opcode & 0x80)
		flush_icache((opcode >> 6) & 3);
}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* CINVAQ.L #<data> */
uae_u32 REGPARAM2 CPUFUNC(op_f41f_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 6) & 3);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	if (opcode & 0x80)
		flush_icache((opcode >> 6) & 3);
}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* CPUSHLQ.L #<data>,An */
uae_u32 REGPARAM2 CPUFUNC(op_f428_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 6) & 3);
	uae_u32 dstreg = opcode & 7;
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	if (opcode & 0x80)
		flush_icache((opcode >> 6) & 3);
}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* CPUSHPQ.L #<data>,An */
uae_u32 REGPARAM2 CPUFUNC(op_f430_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 6) & 3);
	uae_u32 dstreg = opcode & 7;
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	if (opcode & 0x80)
		flush_icache((opcode >> 6) & 3);
}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* CPUSHAQ.L #<data> */
uae_u32 REGPARAM2 CPUFUNC(op_f438_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 6) & 3);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	if (opcode & 0x80)
		flush_icache((opcode >> 6) & 3);
}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* CPUSHAQ.L #<data> */
uae_u32 REGPARAM2 CPUFUNC(op_f439_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 6) & 3);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	if (opcode & 0x80)
		flush_icache((opcode >> 6) & 3);
}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* CPUSHAQ.L #<data> */
uae_u32 REGPARAM2 CPUFUNC(op_f43a_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 6) & 3);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	if (opcode & 0x80)
		flush_icache((opcode >> 6) & 3);
}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* CPUSHAQ.L #<data> */
uae_u32 REGPARAM2 CPUFUNC(op_f43b_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 6) & 3);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	if (opcode & 0x80)
		flush_icache((opcode >> 6) & 3);
}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* CPUSHAQ.L #<data> */
uae_u32 REGPARAM2 CPUFUNC(op_f43c_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 6) & 3);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	if (opcode & 0x80)
		flush_icache((opcode >> 6) & 3);
}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* CPUSHAQ.L #<data> */
uae_u32 REGPARAM2 CPUFUNC(op_f43d_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 6) & 3);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	if (opcode & 0x80)
		flush_icache((opcode >> 6) & 3);
}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* CPUSHAQ.L #<data> */
uae_u32 REGPARAM2 CPUFUNC(op_f43e_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 6) & 3);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	if (opcode & 0x80)
		flush_icache((opcode >> 6) & 3);
}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* CPUSHAQ.L #<data> */
uae_u32 REGPARAM2 CPUFUNC(op_f43f_40)(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 6) & 3);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	if (opcode & 0x80)
		flush_icache((opcode >> 6) & 3);
}}	m68k_incpc (2);
return 4 * CYCLE_UNIT / 2;
}

/* PFLUSHN.L (An) */
uae_u32 REGPARAM2 CPUFUNC(op_f500_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	m68k_incpc (2);
	mmu_op (opcode, 0);
}}return 12 * CYCLE_UNIT / 2;
}

/* PFLUSH.L (An) */
uae_u32 REGPARAM2 CPUFUNC(op_f508_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	m68k_incpc (2);
	mmu_op (opcode, 0);
}}return 12 * CYCLE_UNIT / 2;
}

/* PFLUSHAN.L (An) */
uae_u32 REGPARAM2 CPUFUNC(op_f510_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	m68k_incpc (2);
	mmu_op (opcode, 0);
}}return 12 * CYCLE_UNIT / 2;
}

/* PFLUSHA.L (An) */
uae_u32 REGPARAM2 CPUFUNC(op_f518_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	m68k_incpc (2);
	mmu_op (opcode, 0);
}}return 12 * CYCLE_UNIT / 2;
}

/* PTESTW.L (An) */
uae_u32 REGPARAM2 CPUFUNC(op_f548_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	m68k_incpc (2);
	mmu_op (opcode, 0);
}}return 12 * CYCLE_UNIT / 2;
}

/* PTESTR.L (An) */
uae_u32 REGPARAM2 CPUFUNC(op_f568_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	m68k_incpc (2);
	mmu_op (opcode, 0);
}}return 12 * CYCLE_UNIT / 2;
}

/* PLPAW.L (An) */
uae_u32 REGPARAM2 CPUFUNC(op_f588_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	m68k_incpc (2);
	mmu_op (opcode, 0);
}}return 12 * CYCLE_UNIT / 2;
}

/* PLPAR.L (An) */
uae_u32 REGPARAM2 CPUFUNC(op_f5c8_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	m68k_incpc (2);
	mmu_op (opcode, 0);
}}return 12 * CYCLE_UNIT / 2;
}

/* MOVE16.L (An)+,(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_f600_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{	uae_u32 v[4];
{	uaecptr memsa;
	memsa = m68k_areg (regs, srcreg);
{	uaecptr memda;
	memda = get_dilong (2);
	memsa &= ~15;
	memda &= ~15;
	v[0] = get_long_jit (memsa);
	v[1] = get_long_jit (memsa + 4);
	v[2] = get_long_jit (memsa + 8);
	v[3] = get_long_jit (memsa + 12);
	put_long_jit (memda , v[0]);
	put_long_jit (memda + 4, v[1]);
	put_long_jit (memda + 8, v[2]);
	put_long_jit (memda + 12, v[3]);
	m68k_areg (regs, srcreg) += 16;
}}}	m68k_incpc (6);
return 12 * CYCLE_UNIT / 2;
}

/* MOVE16.L (xxx).L,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_f608_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{	uae_u32 v[4];
{	uaecptr memsa;
	memsa = get_dilong (2);
{	uaecptr memda;
	memda = m68k_areg (regs, dstreg);
	memsa &= ~15;
	memda &= ~15;
	v[0] = get_long_jit (memsa);
	v[1] = get_long_jit (memsa + 4);
	v[2] = get_long_jit (memsa + 8);
	v[3] = get_long_jit (memsa + 12);
	put_long_jit (memda , v[0]);
	put_long_jit (memda + 4, v[1]);
	put_long_jit (memda + 8, v[2]);
	put_long_jit (memda + 12, v[3]);
	m68k_areg (regs, dstreg) += 16;
}}}	m68k_incpc (6);
return 12 * CYCLE_UNIT / 2;
}

/* MOVE16.L (An),(xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_f610_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{	uae_u32 v[4];
{	uaecptr memsa;
	memsa = m68k_areg (regs, srcreg);
{	uaecptr memda;
	memda = get_dilong (2);
	memsa &= ~15;
	memda &= ~15;
	v[0] = get_long_jit (memsa);
	v[1] = get_long_jit (memsa + 4);
	v[2] = get_long_jit (memsa + 8);
	v[3] = get_long_jit (memsa + 12);
	put_long_jit (memda , v[0]);
	put_long_jit (memda + 4, v[1]);
	put_long_jit (memda + 8, v[2]);
	put_long_jit (memda + 12, v[3]);
}}}	m68k_incpc (6);
return 12 * CYCLE_UNIT / 2;
}

/* MOVE16.L (xxx).L,(An) */
uae_u32 REGPARAM2 CPUFUNC(op_f618_40)(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
{	uae_u32 v[4];
{	uaecptr memsa;
	memsa = get_dilong (2);
{	uaecptr memda;
	memda = m68k_areg (regs, dstreg);
	memsa &= ~15;
	memda &= ~15;
	v[0] = get_long_jit (memsa);
	v[1] = get_long_jit (memsa + 4);
	v[2] = get_long_jit (memsa + 8);
	v[3] = get_long_jit (memsa + 12);
	put_long_jit (memda , v[0]);
	put_long_jit (memda + 4, v[1]);
	put_long_jit (memda + 8, v[2]);
	put_long_jit (memda + 12, v[3]);
}}}	m68k_incpc (6);
return 12 * CYCLE_UNIT / 2;
}

/* MOVE16.L (An)+,(An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_f620_40)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = 0;
{	uae_u32 v[4];
	uaecptr mems = m68k_areg (regs, srcreg) & ~15, memd;
	dstreg = (get_diword (2) >> 12) & 7;
	memd = m68k_areg (regs, dstreg) & ~15;
	v[0] = get_long_jit (mems);
	v[1] = get_long_jit (mems + 4);
	v[2] = get_long_jit (mems + 8);
	v[3] = get_long_jit (mems + 12);
	put_long_jit (memd , v[0]);
	put_long_jit (memd + 4, v[1]);
	put_long_jit (memd + 8, v[2]);
	put_long_jit (memd + 12, v[3]);
	if (srcreg != dstreg)
		m68k_areg (regs, srcreg) += 16;
	m68k_areg (regs, dstreg) += 16;
}	m68k_incpc (4);
return 8 * CYCLE_UNIT / 2;
}

/* LPSTOP.L #<data>.W */
uae_u32 REGPARAM2 CPUFUNC(op_f800_40)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	uae_u16 sw = get_diword (2);
	uae_u16 sr;
	if (sw != (0x100|0x80|0x40)) { Exception (4); return 4 * CYCLE_UNIT / 2; }
	sr = get_diword (4);
	if (!(sr & 0x8000)) { Exception (8); return 4 * CYCLE_UNIT / 2; }
	regs.sr = sr;
	MakeFromSR ();
	m68k_setstopped();
	m68k_incpc (6);
}}return 4 * CYCLE_UNIT / 2;
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
#endif

#ifdef PART_4
/* RTE.L  */
uae_u32 REGPARAM2 CPUFUNC(op_4e73_41)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	uae_u16 newsr; uae_u32 newpc;
	for (;;) {
		uaecptr a = m68k_areg (regs, 7);
		uae_u16 sr = get_word_jit (a);
		uae_u32 pc = get_long_jit (a + 2);
		uae_u16 format = get_word_jit (a + 2 + 4);
		int frame = format >> 12;
		int offset = 8;
		newsr = sr; newpc = pc;
		if (frame == 0x0) { m68k_areg (regs, 7) += offset; break; }
		else if (frame == 0x1) { m68k_areg (regs, 7) += offset; }
		else if (frame == 0x2) { m68k_areg (regs, 7) += offset + 4; break; }
		else if (frame == 0x3) { m68k_areg (regs, 7) += offset + 4; break; }
		else if (frame == 0x4) { m68k_areg (regs, 7) += offset + 8; break; }
		else if (frame == 0x7) { m68k_areg (regs, 7) += offset + 52; break; }
		else { m68k_areg (regs, 7) += offset; Exception_cpu(14); return 4 * CYCLE_UNIT / 2; }
		regs.sr = newsr;
	MakeFromSR_T0();
}
	regs.sr = newsr;
	MakeFromSR_T0();
	if (newpc & 1) {
		exception3i (0x4E73, newpc);
		return 4 * CYCLE_UNIT / 2;
	}
	m68k_setpc (newpc);
}}return 24 * CYCLE_UNIT / 2;
}

#endif

#ifdef PART_5
#endif

#ifdef PART_6
#endif

#ifdef PART_7
#endif

#ifdef PART_8
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
#endif

#ifdef PART_4
/* NBCD.B Dn */
uae_u32 REGPARAM2 CPUFUNC(op_4800_42)(uae_u32 opcode)
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
uae_u32 REGPARAM2 CPUFUNC(op_4810_42)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
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
	put_byte_jit (srca,newv);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* NBCD.B (An)+ */
uae_u32 REGPARAM2 CPUFUNC(op_4818_42)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg);
{	uae_s8 src = get_byte_jit (srca);
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
	put_byte_jit (srca,newv);
}}}}	m68k_incpc (2);
return 12 * CYCLE_UNIT / 2;
}

/* NBCD.B -(An) */
uae_u32 REGPARAM2 CPUFUNC(op_4820_42)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte_jit (srca);
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
	put_byte_jit (srca,newv);
}}}}	m68k_incpc (2);
return 13 * CYCLE_UNIT / 2;
}

/* NBCD.B (d16,An) */
uae_u32 REGPARAM2 CPUFUNC(op_4828_42)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
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
	put_byte_jit (srca,newv);
}}}}	m68k_incpc (4);
return 13 * CYCLE_UNIT / 2;
}

/* NBCD.B (d8,An,Xn) */
uae_u32 REGPARAM2 CPUFUNC(op_4830_42)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca;
	m68k_incpc (2);
{	srca = get_disp_ea_020 (m68k_areg (regs, srcreg), 0);
{	uae_s8 src = get_byte_jit (srca);
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
	put_byte_jit (srca,newv);
}}}}}return 15 * CYCLE_UNIT / 2;
}

/* NBCD.B (xxx).W */
uae_u32 REGPARAM2 CPUFUNC(op_4838_42)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
{	uae_s8 src = get_byte_jit (srca);
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
	put_byte_jit (srca,newv);
}}}}	m68k_incpc (4);
return 12 * CYCLE_UNIT / 2;
}

/* NBCD.B (xxx).L */
uae_u32 REGPARAM2 CPUFUNC(op_4839_42)(uae_u32 opcode)
{
{{	uaecptr srca;
	srca = get_dilong (2);
{	uae_s8 src = get_byte_jit (srca);
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
	put_byte_jit (srca,newv);
}}}}	m68k_incpc (6);
return 12 * CYCLE_UNIT / 2;
}

/* RTE.L  */
uae_u32 REGPARAM2 CPUFUNC(op_4e73_42)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	uae_u16 newsr; uae_u32 newpc;
	for (;;) {
		uaecptr a = m68k_areg (regs, 7);
		uae_u16 sr = get_word_jit (a);
		uae_u32 pc = get_long_jit (a + 2);
		uae_u16 format = get_word_jit (a + 2 + 4);
		int frame = format >> 12;
		int offset = 8;
		newsr = sr; newpc = pc;
		if (frame == 0x0) { m68k_areg (regs, 7) += offset; break; }
		else if (frame == 0x1) { m68k_areg (regs, 7) += offset; }
		else if (frame == 0x2) { m68k_areg (regs, 7) += offset + 4; break; }
		else if (frame == 0x4) { m68k_areg (regs, 7) += offset + 8; break; }
		else if (frame == 0x9) { m68k_areg (regs, 7) += offset + 12; break; }
		else if (frame == 0xa) { m68k_areg (regs, 7) += offset + 24; break; }
		else if (frame == 0xb) { m68k_areg (regs, 7) += offset + 84; break; }
		else { m68k_areg (regs, 7) += offset; Exception_cpu(14); return 4 * CYCLE_UNIT / 2; }
		regs.sr = newsr;
	MakeFromSR_T0();
}
	regs.sr = newsr;
	MakeFromSR_T0();
	if (newpc & 1) {
		exception3i (0x4E73, newpc);
		return 4 * CYCLE_UNIT / 2;
	}
	m68k_setpc (newpc);
}}return 24 * CYCLE_UNIT / 2;
}

#endif

#ifdef PART_5
#endif

#ifdef PART_6
/* SBCD.B Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_8100_42)(uae_u32 opcode)
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
return 4 * CYCLE_UNIT / 2;
}

/* SBCD.B -(An),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_8108_42)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte_jit (dsta);
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
	put_byte_jit (dsta,newv);
}}}}}}	m68k_incpc (2);
return 16 * CYCLE_UNIT / 2;
}

#endif

#ifdef PART_7
/* ABCD.B Dn,Dn */
uae_u32 REGPARAM2 CPUFUNC(op_c100_42)(uae_u32 opcode)
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
return 4 * CYCLE_UNIT / 2;
}

/* ABCD.B -(An),-(An) */
uae_u32 REGPARAM2 CPUFUNC(op_c108_42)(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca;
	srca = m68k_areg (regs, srcreg) - areg_byteinc[srcreg];
{	uae_s8 src = get_byte_jit (srca);
	m68k_areg (regs, srcreg) = srca;
{	uaecptr dsta;
	dsta = m68k_areg (regs, dstreg) - areg_byteinc[dstreg];
{	uae_s8 dst = get_byte_jit (dsta);
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
	put_byte_jit (dsta,newv);
}}}}}}	m68k_incpc (2);
return 16 * CYCLE_UNIT / 2;
}

#endif

#ifdef PART_8
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
#endif

#ifdef PART_4
/* RTE.L  */
uae_u32 REGPARAM2 CPUFUNC(op_4e73_43)(uae_u32 opcode)
{
{if (!regs.s) { Exception (8); return 4 * CYCLE_UNIT / 2; }
{	uae_u16 newsr; uae_u32 newpc;
	for (;;) {
		uaecptr a = m68k_areg (regs, 7);
		uae_u16 sr = get_word_jit (a);
		uae_u32 pc = get_long_jit (a + 2);
		uae_u16 format = get_word_jit (a + 2 + 4);
		int frame = format >> 12;
		int offset = 8;
		newsr = sr; newpc = pc;
		if (frame == 0x0) { m68k_areg (regs, 7) += offset; break; }
		else if (frame == 0x1) { m68k_areg (regs, 7) += offset; }
		else if (frame == 0x2) { m68k_areg (regs, 7) += offset + 4; break; }
		else if (frame == 0x4) { m68k_areg (regs, 7) += offset + 8; break; }
		else if (frame == 0x9) { m68k_areg (regs, 7) += offset + 12; break; }
		else if (frame == 0xa) { m68k_areg (regs, 7) += offset + 24; break; }
		else if (frame == 0xb) { m68k_areg (regs, 7) += offset + 84; break; }
		else { m68k_areg (regs, 7) += offset; Exception_cpu(14); return 4 * CYCLE_UNIT / 2; }
		regs.sr = newsr;
	MakeFromSR_T0();
}
	regs.sr = newsr;
	MakeFromSR_T0();
	if (newpc & 1) {
		exception3i (0x4E73, newpc);
		return 4 * CYCLE_UNIT / 2;
	}
	m68k_setpc (newpc);
}}return 24 * CYCLE_UNIT / 2;
}

#endif

#ifdef PART_5
#endif

#ifdef PART_6
#endif

#ifdef PART_7
#endif

#ifdef PART_8
#endif

