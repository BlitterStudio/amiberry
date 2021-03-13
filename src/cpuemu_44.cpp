#include "sysdeps.h"
#include "options.h"
#include "memory.h"
#include "newcpu.h"
#include "cpu_prefetch.h"
#include "cputbl.h"
#define SET_ALWAYS_CFLG(x) SET_CFLG(x)
#define SET_ALWAYS_NFLG(x) SET_NFLG(x)

/* OR.B #<data>.B,Dn */
uae_u32 REGPARAM2 op_0000_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = get_dibyte (2);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc(4);
	return 8 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* OR.B #<data>.B,(An) */
uae_u32 REGPARAM2 op_0010_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* OR.B #<data>.B,(An)+ */
uae_u32 REGPARAM2 op_0018_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* OR.B #<data>.B,-(An) */
uae_u32 REGPARAM2 op_0020_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* OR.B #<data>.B,(d16,An) */
uae_u32 REGPARAM2 op_0028_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	uae_s8 dst = get_byte_jit(dsta);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* OR.B #<data>.B,(d8,An,Xn) */
uae_u32 REGPARAM2 op_0030_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
	uae_s8 dst = get_byte_jit(dsta);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 4,0   */

/* OR.B #<data>.B,(xxx).W */
uae_u32 REGPARAM2 op_0038_44_ff(uae_u32 opcode)
{
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	uae_s8 dst = get_byte_jit(dsta);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* OR.B #<data>.B,(xxx).L */
uae_u32 REGPARAM2 op_0039_44_ff(uae_u32 opcode)
{
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = get_dilong(4);
	uae_s8 dst = get_byte_jit(dsta);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(8);
	return 24 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* ORSR.B #<data>.W */
uae_u32 REGPARAM2 op_003c_44_ff(uae_u32 opcode)
{
	MakeSR();
	uae_s16 src = get_diword (2);
	src &= 0xFF;
	regs.sr |= src;
	MakeFromSR();
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* OR.W #<data>.W,Dn */
uae_u32 REGPARAM2 op_0040_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc(4);
	return 8 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* OR.W #<data>.W,(An) */
uae_u32 REGPARAM2 op_0050_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s16 dst = get_word_jit(dsta);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* OR.W #<data>.W,(An)+ */
uae_u32 REGPARAM2 op_0058_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s16 dst = get_word_jit(dsta);
	m68k_areg(regs, dstreg) += 2;
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* OR.W #<data>.W,-(An) */
uae_u32 REGPARAM2 op_0060_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	uae_s16 dst = get_word_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* OR.W #<data>.W,(d16,An) */
uae_u32 REGPARAM2 op_0068_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	uae_s16 dst = get_word_jit(dsta);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* OR.W #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 op_0070_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
	uae_s16 dst = get_word_jit(dsta);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 4,0   */

/* OR.W #<data>.W,(xxx).W */
uae_u32 REGPARAM2 op_0078_44_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	uae_s16 dst = get_word_jit(dsta);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* OR.W #<data>.W,(xxx).L */
uae_u32 REGPARAM2 op_0079_44_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = get_dilong(4);
	uae_s16 dst = get_word_jit(dsta);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(8);
	return 24 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* ORSR.W #<data>.W */
uae_u32 REGPARAM2 op_007c_44_ff(uae_u32 opcode)
{
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	MakeSR();
	uae_s16 src = get_diword (2);
	regs.sr |= src;
	MakeFromSR_T0();
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* OR.L #<data>.L,Dn */
uae_u32 REGPARAM2 op_0080_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* OR.L #<data>.L,(An) */
uae_u32 REGPARAM2 op_0090_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s32 dst = get_long_jit(dsta);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(6);
	return 28 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* OR.L #<data>.L,(An)+ */
uae_u32 REGPARAM2 op_0098_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s32 dst = get_long_jit(dsta);
	m68k_areg(regs, dstreg) += 4;
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(6);
	return 28 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* OR.L #<data>.L,-(An) */
uae_u32 REGPARAM2 op_00a0_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	uae_s32 dst = get_long_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(6);
	return 30 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* OR.L #<data>.L,(d16,An) */
uae_u32 REGPARAM2 op_00a8_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (6);
	uae_s32 dst = get_long_jit(dsta);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(8);
	return 32 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* OR.L #<data>.L,(d8,An,Xn) */
uae_u32 REGPARAM2 op_00b0_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (6));
	uae_s32 dst = get_long_jit(dsta);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(8);
	return 34 * CYCLE_UNIT / 2;
}
/* 8 4,0   */

/* OR.L #<data>.L,(xxx).W */
uae_u32 REGPARAM2 op_00b8_44_ff(uae_u32 opcode)
{
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (6);
	uae_s32 dst = get_long_jit(dsta);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(8);
	return 32 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* OR.L #<data>.L,(xxx).L */
uae_u32 REGPARAM2 op_00b9_44_ff(uae_u32 opcode)
{
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = get_dilong(6);
	uae_s32 dst = get_long_jit(dsta);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(10);
	return 36 * CYCLE_UNIT / 2;
}
/* 10 0,0   */

/* BTST.L Dn,Dn */
uae_u32 REGPARAM2 op_0100_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= 31;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MVPMR.W (d16,An),Dn */
uae_u32 REGPARAM2 op_0108_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr mempa = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_u16 val  = (get_byte_jit(mempa) & 0xff) << 8;
	val |= (get_byte_jit(mempa + 2) & 0xff);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((val) & 0xffff);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* BTST.B Dn,(An) */
uae_u32 REGPARAM2 op_0110_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* BTST.B Dn,(An)+ */
uae_u32 REGPARAM2 op_0118_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* BTST.B Dn,-(An) */
uae_u32 REGPARAM2 op_0120_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* BTST.B Dn,(d16,An) */
uae_u32 REGPARAM2 op_0128_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* BTST.B Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 op_0130_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (2));
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* BTST.B Dn,(xxx).W */
uae_u32 REGPARAM2 op_0138_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* BTST.B Dn,(xxx).L */
uae_u32 REGPARAM2 op_0139_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_dilong(2);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* BTST.B Dn,(d16,PC) */
uae_u32 REGPARAM2 op_013a_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = 2;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_getpc () + 2;
	dsta += (uae_s32)(uae_s16)get_diword (2);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* BTST.B Dn,(d8,PC,Xn) */
uae_u32 REGPARAM2 op_013b_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = 3;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	uaecptr tmppc = m68k_getpc () + 2;
	dsta = get_disp_ea_000(tmppc, get_diword (2));
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* BTST.B Dn,#<data>.B */
uae_u32 REGPARAM2 op_013c_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	uae_s8 dst = get_dibyte (2);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	m68k_incpc(4);
	return 8 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* BCHG.L Dn,Dn */
uae_u32 REGPARAM2 op_0140_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= 31;
	dst ^= (1 << src);
	SET_ZFLG(((uae_u32)dst & (1 << src)) >> src);
	m68k_dreg(regs, dstreg) = (dst);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MVPMR.L (d16,An),Dn */
uae_u32 REGPARAM2 op_0148_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr mempa = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_u32 val  = (get_byte_jit(mempa) & 0xff) << 24;
	val |= (get_byte_jit(mempa + 2) & 0xff) << 16;
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & 0x0000ffff) | val;
	val |= (get_byte_jit(mempa + 4) & 0xff) << 8;
	val |= (get_byte_jit(mempa + 6) & 0xff);
	m68k_dreg(regs, dstreg) = (val);
	m68k_incpc(4);
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* BCHG.B Dn,(An) */
uae_u32 REGPARAM2 op_0150_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG(((uae_u32)dst & (1 << src)) >> src);
	put_byte_jit(dsta, dst);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* BCHG.B Dn,(An)+ */
uae_u32 REGPARAM2 op_0158_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG(((uae_u32)dst & (1 << src)) >> src);
	put_byte_jit(dsta, dst);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* BCHG.B Dn,-(An) */
uae_u32 REGPARAM2 op_0160_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG(((uae_u32)dst & (1 << src)) >> src);
	put_byte_jit(dsta, dst);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* BCHG.B Dn,(d16,An) */
uae_u32 REGPARAM2 op_0168_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG(((uae_u32)dst & (1 << src)) >> src);
	put_byte_jit(dsta, dst);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* BCHG.B Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 op_0170_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (2));
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG(((uae_u32)dst & (1 << src)) >> src);
	put_byte_jit(dsta, dst);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* BCHG.B Dn,(xxx).W */
uae_u32 REGPARAM2 op_0178_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG(((uae_u32)dst & (1 << src)) >> src);
	put_byte_jit(dsta, dst);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* BCHG.B Dn,(xxx).L */
uae_u32 REGPARAM2 op_0179_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_dilong(2);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG(((uae_u32)dst & (1 << src)) >> src);
	put_byte_jit(dsta, dst);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* BCLR.L Dn,Dn */
uae_u32 REGPARAM2 op_0180_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= 31;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	m68k_dreg(regs, dstreg) = (dst);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MVPRM.W Dn,(d16,An) */
uae_u32 REGPARAM2 op_0188_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr mempa = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	put_byte_jit(mempa, src >> 8);
	put_byte_jit(mempa + 2, src); 
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* BCLR.B Dn,(An) */
uae_u32 REGPARAM2 op_0190_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* BCLR.B Dn,(An)+ */
uae_u32 REGPARAM2 op_0198_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* BCLR.B Dn,-(An) */
uae_u32 REGPARAM2 op_01a0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(2);
	return 16 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* BCLR.B Dn,(d16,An) */
uae_u32 REGPARAM2 op_01a8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* BCLR.B Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 op_01b0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (2));
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* BCLR.B Dn,(xxx).W */
uae_u32 REGPARAM2 op_01b8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* BCLR.B Dn,(xxx).L */
uae_u32 REGPARAM2 op_01b9_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_dilong(2);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* BSET.L Dn,Dn */
uae_u32 REGPARAM2 op_01c0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= 31;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	m68k_dreg(regs, dstreg) = (dst);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MVPRM.L Dn,(d16,An) */
uae_u32 REGPARAM2 op_01c8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr mempa = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	put_byte_jit(mempa, src >> 24);
	put_byte_jit(mempa + 2, src >> 16);
	put_byte_jit(mempa + 4, src >> 8);
	put_byte_jit(mempa + 6, src); 
	m68k_incpc(4);
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* BSET.B Dn,(An) */
uae_u32 REGPARAM2 op_01d0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* BSET.B Dn,(An)+ */
uae_u32 REGPARAM2 op_01d8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* BSET.B Dn,-(An) */
uae_u32 REGPARAM2 op_01e0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* BSET.B Dn,(d16,An) */
uae_u32 REGPARAM2 op_01e8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* BSET.B Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 op_01f0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (2));
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* BSET.B Dn,(xxx).W */
uae_u32 REGPARAM2 op_01f8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* BSET.B Dn,(xxx).L */
uae_u32 REGPARAM2 op_01f9_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_dilong(2);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* AND.B #<data>.B,Dn */
uae_u32 REGPARAM2 op_0200_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = get_dibyte (2);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc(4);
	return 8 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* AND.B #<data>.B,(An) */
uae_u32 REGPARAM2 op_0210_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* AND.B #<data>.B,(An)+ */
uae_u32 REGPARAM2 op_0218_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* AND.B #<data>.B,-(An) */
uae_u32 REGPARAM2 op_0220_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* AND.B #<data>.B,(d16,An) */
uae_u32 REGPARAM2 op_0228_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	uae_s8 dst = get_byte_jit(dsta);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* AND.B #<data>.B,(d8,An,Xn) */
uae_u32 REGPARAM2 op_0230_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
	uae_s8 dst = get_byte_jit(dsta);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 4,0   */

/* AND.B #<data>.B,(xxx).W */
uae_u32 REGPARAM2 op_0238_44_ff(uae_u32 opcode)
{
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	uae_s8 dst = get_byte_jit(dsta);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* AND.B #<data>.B,(xxx).L */
uae_u32 REGPARAM2 op_0239_44_ff(uae_u32 opcode)
{
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = get_dilong(4);
	uae_s8 dst = get_byte_jit(dsta);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(8);
	return 24 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* ANDSR.B #<data>.W */
uae_u32 REGPARAM2 op_023c_44_ff(uae_u32 opcode)
{
	MakeSR();
	uae_s16 src = get_diword (2);
	src |= 0xff00;
	regs.sr &= src;
	MakeFromSR();
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* AND.W #<data>.W,Dn */
uae_u32 REGPARAM2 op_0240_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc(4);
	return 8 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* AND.W #<data>.W,(An) */
uae_u32 REGPARAM2 op_0250_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s16 dst = get_word_jit(dsta);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* AND.W #<data>.W,(An)+ */
uae_u32 REGPARAM2 op_0258_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s16 dst = get_word_jit(dsta);
	m68k_areg(regs, dstreg) += 2;
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* AND.W #<data>.W,-(An) */
uae_u32 REGPARAM2 op_0260_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	uae_s16 dst = get_word_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* AND.W #<data>.W,(d16,An) */
uae_u32 REGPARAM2 op_0268_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	uae_s16 dst = get_word_jit(dsta);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* AND.W #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 op_0270_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
	uae_s16 dst = get_word_jit(dsta);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 4,0   */

/* AND.W #<data>.W,(xxx).W */
uae_u32 REGPARAM2 op_0278_44_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	uae_s16 dst = get_word_jit(dsta);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* AND.W #<data>.W,(xxx).L */
uae_u32 REGPARAM2 op_0279_44_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = get_dilong(4);
	uae_s16 dst = get_word_jit(dsta);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(8);
	return 24 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* ANDSR.W #<data>.W */
uae_u32 REGPARAM2 op_027c_44_ff(uae_u32 opcode)
{
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	MakeSR();
	uae_s16 src = get_diword (2);
	regs.sr &= src;
	MakeFromSR_T0();
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* AND.L #<data>.L,Dn */
uae_u32 REGPARAM2 op_0280_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* AND.L #<data>.L,(An) */
uae_u32 REGPARAM2 op_0290_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s32 dst = get_long_jit(dsta);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(6);
	return 28 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* AND.L #<data>.L,(An)+ */
uae_u32 REGPARAM2 op_0298_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s32 dst = get_long_jit(dsta);
	m68k_areg(regs, dstreg) += 4;
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(6);
	return 28 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* AND.L #<data>.L,-(An) */
uae_u32 REGPARAM2 op_02a0_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	uae_s32 dst = get_long_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(6);
	return 30 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* AND.L #<data>.L,(d16,An) */
uae_u32 REGPARAM2 op_02a8_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (6);
	uae_s32 dst = get_long_jit(dsta);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(8);
	return 32 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* AND.L #<data>.L,(d8,An,Xn) */
uae_u32 REGPARAM2 op_02b0_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (6));
	uae_s32 dst = get_long_jit(dsta);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(8);
	return 34 * CYCLE_UNIT / 2;
}
/* 8 4,0   */

/* AND.L #<data>.L,(xxx).W */
uae_u32 REGPARAM2 op_02b8_44_ff(uae_u32 opcode)
{
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (6);
	uae_s32 dst = get_long_jit(dsta);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(8);
	return 32 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* AND.L #<data>.L,(xxx).L */
uae_u32 REGPARAM2 op_02b9_44_ff(uae_u32 opcode)
{
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = get_dilong(6);
	uae_s32 dst = get_long_jit(dsta);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(10);
	return 36 * CYCLE_UNIT / 2;
}
/* 10 0,0   */

/* SUB.B #<data>.B,Dn */
uae_u32 REGPARAM2 op_0400_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = get_dibyte (2);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
	m68k_incpc(4);
	return 8 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUB.B #<data>.B,(An) */
uae_u32 REGPARAM2 op_0410_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUB.B #<data>.B,(An)+ */
uae_u32 REGPARAM2 op_0418_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUB.B #<data>.B,-(An) */
uae_u32 REGPARAM2 op_0420_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUB.B #<data>.B,(d16,An) */
uae_u32 REGPARAM2 op_0428_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	uae_s8 dst = get_byte_jit(dsta);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* SUB.B #<data>.B,(d8,An,Xn) */
uae_u32 REGPARAM2 op_0430_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
	uae_s8 dst = get_byte_jit(dsta);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 4,0   */

/* SUB.B #<data>.B,(xxx).W */
uae_u32 REGPARAM2 op_0438_44_ff(uae_u32 opcode)
{
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	uae_s8 dst = get_byte_jit(dsta);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* SUB.B #<data>.B,(xxx).L */
uae_u32 REGPARAM2 op_0439_44_ff(uae_u32 opcode)
{
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = get_dilong(4);
	uae_s8 dst = get_byte_jit(dsta);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(8);
	return 24 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* SUB.W #<data>.W,Dn */
uae_u32 REGPARAM2 op_0440_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
	m68k_incpc(4);
	return 8 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUB.W #<data>.W,(An) */
uae_u32 REGPARAM2 op_0450_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s16 dst = get_word_jit(dsta);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUB.W #<data>.W,(An)+ */
uae_u32 REGPARAM2 op_0458_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s16 dst = get_word_jit(dsta);
	m68k_areg(regs, dstreg) += 2;
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUB.W #<data>.W,-(An) */
uae_u32 REGPARAM2 op_0460_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	uae_s16 dst = get_word_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUB.W #<data>.W,(d16,An) */
uae_u32 REGPARAM2 op_0468_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	uae_s16 dst = get_word_jit(dsta);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* SUB.W #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 op_0470_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
	uae_s16 dst = get_word_jit(dsta);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 4,0   */

/* SUB.W #<data>.W,(xxx).W */
uae_u32 REGPARAM2 op_0478_44_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	uae_s16 dst = get_word_jit(dsta);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* SUB.W #<data>.W,(xxx).L */
uae_u32 REGPARAM2 op_0479_44_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = get_dilong(4);
	uae_s16 dst = get_word_jit(dsta);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(8);
	return 24 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* SUB.L #<data>.L,Dn */
uae_u32 REGPARAM2 op_0480_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(6);
	return 14 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* SUB.L #<data>.L,(An) */
uae_u32 REGPARAM2 op_0490_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s32 dst = get_long_jit(dsta);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(6);
	return 28 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* SUB.L #<data>.L,(An)+ */
uae_u32 REGPARAM2 op_0498_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s32 dst = get_long_jit(dsta);
	m68k_areg(regs, dstreg) += 4;
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(6);
	return 28 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* SUB.L #<data>.L,-(An) */
uae_u32 REGPARAM2 op_04a0_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	uae_s32 dst = get_long_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(6);
	return 30 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* SUB.L #<data>.L,(d16,An) */
uae_u32 REGPARAM2 op_04a8_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (6);
	uae_s32 dst = get_long_jit(dsta);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(8);
	return 32 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* SUB.L #<data>.L,(d8,An,Xn) */
uae_u32 REGPARAM2 op_04b0_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (6));
	uae_s32 dst = get_long_jit(dsta);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(8);
	return 34 * CYCLE_UNIT / 2;
}
/* 8 4,0   */

/* SUB.L #<data>.L,(xxx).W */
uae_u32 REGPARAM2 op_04b8_44_ff(uae_u32 opcode)
{
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (6);
	uae_s32 dst = get_long_jit(dsta);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(8);
	return 32 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* SUB.L #<data>.L,(xxx).L */
uae_u32 REGPARAM2 op_04b9_44_ff(uae_u32 opcode)
{
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = get_dilong(6);
	uae_s32 dst = get_long_jit(dsta);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(10);
	return 36 * CYCLE_UNIT / 2;
}
/* 10 0,0   */

/* ADD.B #<data>.B,Dn */
uae_u32 REGPARAM2 op_0600_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = get_dibyte (2);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
	m68k_incpc(4);
	return 8 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADD.B #<data>.B,(An) */
uae_u32 REGPARAM2 op_0610_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADD.B #<data>.B,(An)+ */
uae_u32 REGPARAM2 op_0618_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADD.B #<data>.B,-(An) */
uae_u32 REGPARAM2 op_0620_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADD.B #<data>.B,(d16,An) */
uae_u32 REGPARAM2 op_0628_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	uae_s8 dst = get_byte_jit(dsta);
	uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* ADD.B #<data>.B,(d8,An,Xn) */
uae_u32 REGPARAM2 op_0630_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
	uae_s8 dst = get_byte_jit(dsta);
	uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 4,0   */

/* ADD.B #<data>.B,(xxx).W */
uae_u32 REGPARAM2 op_0638_44_ff(uae_u32 opcode)
{
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	uae_s8 dst = get_byte_jit(dsta);
	uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* ADD.B #<data>.B,(xxx).L */
uae_u32 REGPARAM2 op_0639_44_ff(uae_u32 opcode)
{
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = get_dilong(4);
	uae_s8 dst = get_byte_jit(dsta);
	uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(8);
	return 24 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* ADD.W #<data>.W,Dn */
uae_u32 REGPARAM2 op_0640_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
	m68k_incpc(4);
	return 8 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADD.W #<data>.W,(An) */
uae_u32 REGPARAM2 op_0650_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s16 dst = get_word_jit(dsta);
	uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADD.W #<data>.W,(An)+ */
uae_u32 REGPARAM2 op_0658_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s16 dst = get_word_jit(dsta);
	m68k_areg(regs, dstreg) += 2;
	uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADD.W #<data>.W,-(An) */
uae_u32 REGPARAM2 op_0660_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	uae_s16 dst = get_word_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADD.W #<data>.W,(d16,An) */
uae_u32 REGPARAM2 op_0668_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	uae_s16 dst = get_word_jit(dsta);
	uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* ADD.W #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 op_0670_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
	uae_s16 dst = get_word_jit(dsta);
	uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 4,0   */

/* ADD.W #<data>.W,(xxx).W */
uae_u32 REGPARAM2 op_0678_44_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	uae_s16 dst = get_word_jit(dsta);
	uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* ADD.W #<data>.W,(xxx).L */
uae_u32 REGPARAM2 op_0679_44_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = get_dilong(4);
	uae_s16 dst = get_word_jit(dsta);
	uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(8);
	return 24 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* ADD.L #<data>.L,Dn */
uae_u32 REGPARAM2 op_0680_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(6);
	return 14 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* ADD.L #<data>.L,(An) */
uae_u32 REGPARAM2 op_0690_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s32 dst = get_long_jit(dsta);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(6);
	return 28 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* ADD.L #<data>.L,(An)+ */
uae_u32 REGPARAM2 op_0698_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s32 dst = get_long_jit(dsta);
	m68k_areg(regs, dstreg) += 4;
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(6);
	return 28 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* ADD.L #<data>.L,-(An) */
uae_u32 REGPARAM2 op_06a0_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	uae_s32 dst = get_long_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(6);
	return 30 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* ADD.L #<data>.L,(d16,An) */
uae_u32 REGPARAM2 op_06a8_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (6);
	uae_s32 dst = get_long_jit(dsta);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(8);
	return 32 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* ADD.L #<data>.L,(d8,An,Xn) */
uae_u32 REGPARAM2 op_06b0_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (6));
	uae_s32 dst = get_long_jit(dsta);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(8);
	return 34 * CYCLE_UNIT / 2;
}
/* 8 4,0   */

/* ADD.L #<data>.L,(xxx).W */
uae_u32 REGPARAM2 op_06b8_44_ff(uae_u32 opcode)
{
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (6);
	uae_s32 dst = get_long_jit(dsta);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(8);
	return 32 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* ADD.L #<data>.L,(xxx).L */
uae_u32 REGPARAM2 op_06b9_44_ff(uae_u32 opcode)
{
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = get_dilong(6);
	uae_s32 dst = get_long_jit(dsta);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(10);
	return 36 * CYCLE_UNIT / 2;
}
/* 10 0,0   */

/* BTST.L #<data>.W,Dn */
uae_u32 REGPARAM2 op_0800_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= 31;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	m68k_incpc(4);
	return 10 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* BTST.B #<data>.W,(An) */
uae_u32 REGPARAM2 op_0810_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* BTST.B #<data>.W,(An)+ */
uae_u32 REGPARAM2 op_0818_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* BTST.B #<data>.W,-(An) */
uae_u32 REGPARAM2 op_0820_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* BTST.B #<data>.W,(d16,An) */
uae_u32 REGPARAM2 op_0828_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* BTST.B #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 op_0830_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	m68k_incpc(6);
	return 18 * CYCLE_UNIT / 2;
}
/* 6 4,0   */

/* BTST.B #<data>.W,(xxx).W */
uae_u32 REGPARAM2 op_0838_44_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* BTST.B #<data>.W,(xxx).L */
uae_u32 REGPARAM2 op_0839_44_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = get_dilong(4);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	m68k_incpc(8);
	return 20 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* BTST.B #<data>.W,(d16,PC) */
uae_u32 REGPARAM2 op_083a_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = 2;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_getpc () + 4;
	dsta += (uae_s32)(uae_s16)get_diword (4);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* BTST.B #<data>.W,(d8,PC,Xn) */
uae_u32 REGPARAM2 op_083b_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = 3;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	uaecptr tmppc = m68k_getpc () + 4;
	dsta = get_disp_ea_000(tmppc, get_diword (4));
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	m68k_incpc(6);
	return 18 * CYCLE_UNIT / 2;
}
/* 6 4,0   */

/* BCHG.L #<data>.W,Dn */
uae_u32 REGPARAM2 op_0840_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= 31;
	dst ^= (1 << src);
	SET_ZFLG(((uae_u32)dst & (1 << src)) >> src);
	m68k_dreg(regs, dstreg) = (dst);
	m68k_incpc(4);
	return 10 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* BCHG.B #<data>.W,(An) */
uae_u32 REGPARAM2 op_0850_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG(((uae_u32)dst & (1 << src)) >> src);
	put_byte_jit(dsta, dst);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* BCHG.B #<data>.W,(An)+ */
uae_u32 REGPARAM2 op_0858_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG(((uae_u32)dst & (1 << src)) >> src);
	put_byte_jit(dsta, dst);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* BCHG.B #<data>.W,-(An) */
uae_u32 REGPARAM2 op_0860_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG(((uae_u32)dst & (1 << src)) >> src);
	put_byte_jit(dsta, dst);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* BCHG.B #<data>.W,(d16,An) */
uae_u32 REGPARAM2 op_0868_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG(((uae_u32)dst & (1 << src)) >> src);
	put_byte_jit(dsta, dst);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* BCHG.B #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 op_0870_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG(((uae_u32)dst & (1 << src)) >> src);
	put_byte_jit(dsta, dst);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 4,0   */

/* BCHG.B #<data>.W,(xxx).W */
uae_u32 REGPARAM2 op_0878_44_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG(((uae_u32)dst & (1 << src)) >> src);
	put_byte_jit(dsta, dst);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* BCHG.B #<data>.W,(xxx).L */
uae_u32 REGPARAM2 op_0879_44_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = get_dilong(4);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	dst ^= (1 << src);
	SET_ZFLG(((uae_u32)dst & (1 << src)) >> src);
	put_byte_jit(dsta, dst);
	m68k_incpc(8);
	return 24 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* BCLR.L #<data>.W,Dn */
uae_u32 REGPARAM2 op_0880_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= 31;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	m68k_dreg(regs, dstreg) = (dst);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* BCLR.B #<data>.W,(An) */
uae_u32 REGPARAM2 op_0890_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* BCLR.B #<data>.W,(An)+ */
uae_u32 REGPARAM2 op_0898_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* BCLR.B #<data>.W,-(An) */
uae_u32 REGPARAM2 op_08a0_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* BCLR.B #<data>.W,(d16,An) */
uae_u32 REGPARAM2 op_08a8_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* BCLR.B #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 op_08b0_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(6);
	return 24 * CYCLE_UNIT / 2;
}
/* 6 4,0   */

/* BCLR.B #<data>.W,(xxx).W */
uae_u32 REGPARAM2 op_08b8_44_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* BCLR.B #<data>.W,(xxx).L */
uae_u32 REGPARAM2 op_08b9_44_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = get_dilong(4);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(8);
	return 26 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* BSET.L #<data>.W,Dn */
uae_u32 REGPARAM2 op_08c0_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= 31;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	m68k_dreg(regs, dstreg) = (dst);
	m68k_incpc(4);
	return 10 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* BSET.B #<data>.W,(An) */
uae_u32 REGPARAM2 op_08d0_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* BSET.B #<data>.W,(An)+ */
uae_u32 REGPARAM2 op_08d8_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* BSET.B #<data>.W,-(An) */
uae_u32 REGPARAM2 op_08e0_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* BSET.B #<data>.W,(d16,An) */
uae_u32 REGPARAM2 op_08e8_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* BSET.B #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 op_08f0_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 4,0   */

/* BSET.B #<data>.W,(xxx).W */
uae_u32 REGPARAM2 op_08f8_44_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* BSET.B #<data>.W,(xxx).L */
uae_u32 REGPARAM2 op_08f9_44_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = get_dilong(4);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(8);
	return 24 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* EOR.B #<data>.B,Dn */
uae_u32 REGPARAM2 op_0a00_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = get_dibyte (2);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc(4);
	return 8 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* EOR.B #<data>.B,(An) */
uae_u32 REGPARAM2 op_0a10_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* EOR.B #<data>.B,(An)+ */
uae_u32 REGPARAM2 op_0a18_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* EOR.B #<data>.B,-(An) */
uae_u32 REGPARAM2 op_0a20_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* EOR.B #<data>.B,(d16,An) */
uae_u32 REGPARAM2 op_0a28_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	uae_s8 dst = get_byte_jit(dsta);
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* EOR.B #<data>.B,(d8,An,Xn) */
uae_u32 REGPARAM2 op_0a30_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
	uae_s8 dst = get_byte_jit(dsta);
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 4,0   */

/* EOR.B #<data>.B,(xxx).W */
uae_u32 REGPARAM2 op_0a38_44_ff(uae_u32 opcode)
{
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	uae_s8 dst = get_byte_jit(dsta);
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* EOR.B #<data>.B,(xxx).L */
uae_u32 REGPARAM2 op_0a39_44_ff(uae_u32 opcode)
{
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = get_dilong(4);
	uae_s8 dst = get_byte_jit(dsta);
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(8);
	return 24 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* EORSR.B #<data>.W */
uae_u32 REGPARAM2 op_0a3c_44_ff(uae_u32 opcode)
{
	MakeSR();
	uae_s16 src = get_diword (2);
	src &= 0xFF;
	regs.sr ^= src;
	MakeFromSR();
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* EOR.W #<data>.W,Dn */
uae_u32 REGPARAM2 op_0a40_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc(4);
	return 8 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* EOR.W #<data>.W,(An) */
uae_u32 REGPARAM2 op_0a50_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s16 dst = get_word_jit(dsta);
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* EOR.W #<data>.W,(An)+ */
uae_u32 REGPARAM2 op_0a58_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s16 dst = get_word_jit(dsta);
	m68k_areg(regs, dstreg) += 2;
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* EOR.W #<data>.W,-(An) */
uae_u32 REGPARAM2 op_0a60_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	uae_s16 dst = get_word_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* EOR.W #<data>.W,(d16,An) */
uae_u32 REGPARAM2 op_0a68_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	uae_s16 dst = get_word_jit(dsta);
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* EOR.W #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 op_0a70_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
	uae_s16 dst = get_word_jit(dsta);
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 4,0   */

/* EOR.W #<data>.W,(xxx).W */
uae_u32 REGPARAM2 op_0a78_44_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	uae_s16 dst = get_word_jit(dsta);
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* EOR.W #<data>.W,(xxx).L */
uae_u32 REGPARAM2 op_0a79_44_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = get_dilong(4);
	uae_s16 dst = get_word_jit(dsta);
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(8);
	return 24 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* EORSR.W #<data>.W */
uae_u32 REGPARAM2 op_0a7c_44_ff(uae_u32 opcode)
{
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	MakeSR();
	uae_s16 src = get_diword (2);
	regs.sr ^= src;
	MakeFromSR_T0();
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* EOR.L #<data>.L,Dn */
uae_u32 REGPARAM2 op_0a80_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* EOR.L #<data>.L,(An) */
uae_u32 REGPARAM2 op_0a90_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s32 dst = get_long_jit(dsta);
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(6);
	return 28 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* EOR.L #<data>.L,(An)+ */
uae_u32 REGPARAM2 op_0a98_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s32 dst = get_long_jit(dsta);
	m68k_areg(regs, dstreg) += 4;
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(6);
	return 28 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* EOR.L #<data>.L,-(An) */
uae_u32 REGPARAM2 op_0aa0_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	uae_s32 dst = get_long_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(6);
	return 30 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* EOR.L #<data>.L,(d16,An) */
uae_u32 REGPARAM2 op_0aa8_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (6);
	uae_s32 dst = get_long_jit(dsta);
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(8);
	return 32 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* EOR.L #<data>.L,(d8,An,Xn) */
uae_u32 REGPARAM2 op_0ab0_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (6));
	uae_s32 dst = get_long_jit(dsta);
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(8);
	return 34 * CYCLE_UNIT / 2;
}
/* 8 4,0   */

/* EOR.L #<data>.L,(xxx).W */
uae_u32 REGPARAM2 op_0ab8_44_ff(uae_u32 opcode)
{
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (6);
	uae_s32 dst = get_long_jit(dsta);
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(8);
	return 32 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* EOR.L #<data>.L,(xxx).L */
uae_u32 REGPARAM2 op_0ab9_44_ff(uae_u32 opcode)
{
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = get_dilong(6);
	uae_s32 dst = get_long_jit(dsta);
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(10);
	return 36 * CYCLE_UNIT / 2;
}
/* 10 0,0   */

/* CMP.B #<data>.B,Dn */
uae_u32 REGPARAM2 op_0c00_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = get_dibyte (2);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(4);
	return 8 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* CMP.B #<data>.B,(An) */
uae_u32 REGPARAM2 op_0c10_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* CMP.B #<data>.B,(An)+ */
uae_u32 REGPARAM2 op_0c18_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* CMP.B #<data>.B,-(An) */
uae_u32 REGPARAM2 op_0c20_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* CMP.B #<data>.B,(d16,An) */
uae_u32 REGPARAM2 op_0c28_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	uae_s8 dst = get_byte_jit(dsta);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* CMP.B #<data>.B,(d8,An,Xn) */
uae_u32 REGPARAM2 op_0c30_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
	uae_s8 dst = get_byte_jit(dsta);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(6);
	return 18 * CYCLE_UNIT / 2;
}
/* 6 4,0   */

/* CMP.B #<data>.B,(xxx).W */
uae_u32 REGPARAM2 op_0c38_44_ff(uae_u32 opcode)
{
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	uae_s8 dst = get_byte_jit(dsta);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* CMP.B #<data>.B,(xxx).L */
uae_u32 REGPARAM2 op_0c39_44_ff(uae_u32 opcode)
{
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = get_dilong(4);
	uae_s8 dst = get_byte_jit(dsta);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(8);
	return 20 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* CMP.W #<data>.W,Dn */
uae_u32 REGPARAM2 op_0c40_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(4);
	return 8 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* CMP.W #<data>.W,(An) */
uae_u32 REGPARAM2 op_0c50_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s16 dst = get_word_jit(dsta);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* CMP.W #<data>.W,(An)+ */
uae_u32 REGPARAM2 op_0c58_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s16 dst = get_word_jit(dsta);
	m68k_areg(regs, dstreg) += 2;
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* CMP.W #<data>.W,-(An) */
uae_u32 REGPARAM2 op_0c60_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	uae_s16 dst = get_word_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* CMP.W #<data>.W,(d16,An) */
uae_u32 REGPARAM2 op_0c68_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	uae_s16 dst = get_word_jit(dsta);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* CMP.W #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 op_0c70_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
	uae_s16 dst = get_word_jit(dsta);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(6);
	return 18 * CYCLE_UNIT / 2;
}
/* 6 4,0   */

/* CMP.W #<data>.W,(xxx).W */
uae_u32 REGPARAM2 op_0c78_44_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	uae_s16 dst = get_word_jit(dsta);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* CMP.W #<data>.W,(xxx).L */
uae_u32 REGPARAM2 op_0c79_44_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = get_dilong(4);
	uae_s16 dst = get_word_jit(dsta);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(8);
	return 20 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* CMP.L #<data>.L,Dn */
uae_u32 REGPARAM2 op_0c80_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(6);
	return 14 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* CMP.L #<data>.L,(An) */
uae_u32 REGPARAM2 op_0c90_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s32 dst = get_long_jit(dsta);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* CMP.L #<data>.L,(An)+ */
uae_u32 REGPARAM2 op_0c98_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s32 dst = get_long_jit(dsta);
	m68k_areg(regs, dstreg) += 4;
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* CMP.L #<data>.L,-(An) */
uae_u32 REGPARAM2 op_0ca0_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	uae_s32 dst = get_long_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* CMP.L #<data>.L,(d16,An) */
uae_u32 REGPARAM2 op_0ca8_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (6);
	uae_s32 dst = get_long_jit(dsta);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(8);
	return 24 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* CMP.L #<data>.L,(d8,An,Xn) */
uae_u32 REGPARAM2 op_0cb0_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (6));
	uae_s32 dst = get_long_jit(dsta);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(8);
	return 26 * CYCLE_UNIT / 2;
}
/* 8 4,0   */

/* CMP.L #<data>.L,(xxx).W */
uae_u32 REGPARAM2 op_0cb8_44_ff(uae_u32 opcode)
{
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (6);
	uae_s32 dst = get_long_jit(dsta);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(8);
	return 24 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* CMP.L #<data>.L,(xxx).L */
uae_u32 REGPARAM2 op_0cb9_44_ff(uae_u32 opcode)
{
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = get_dilong(6);
	uae_s32 dst = get_long_jit(dsta);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(10);
	return 28 * CYCLE_UNIT / 2;
}
/* 10 0,0   */

/* MOVES.B #<data>.W,(An) */
uae_u32 REGPARAM2 op_0e10_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uae_s16 extra = get_diword (2);
	if (extra & 0x800) {
		uae_u32 src = regs.regs[(extra >> 12) & 15];
		uaecptr dsta;
		dsta = m68k_areg(regs, dstreg);
		put_byte_jit(dsta, src);
		m68k_incpc(4);
	} else {
		uaecptr srca;
		srca = m68k_areg(regs, dstreg);
		uae_s8 src = get_byte_jit(srca);
		if (extra & 0x8000) {
			m68k_areg(regs, (extra >> 12) & 7) = (uae_s32)(uae_s8)src;
		} else {
			m68k_dreg(regs, (extra >> 12) & 7) = (m68k_dreg(regs, (extra >> 12) & 7) & ~0xff) | ((src) & 0xff);
		}
		m68k_incpc(4);
	}
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVES.B #<data>.W,(An)+ */
uae_u32 REGPARAM2 op_0e18_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uae_s16 extra = get_diword (2);
	if (extra & 0x800) {
		uae_u32 src = regs.regs[(extra >> 12) & 15];
		uaecptr dsta;
		dsta = m68k_areg(regs, dstreg);
		m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
		put_byte_jit(dsta, src);
		m68k_incpc(4);
	} else {
		uaecptr srca;
		srca = m68k_areg(regs, dstreg);
		uae_s8 src = get_byte_jit(srca);
		m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
		if (extra & 0x8000) {
			m68k_areg(regs, (extra >> 12) & 7) = (uae_s32)(uae_s8)src;
		} else {
			m68k_dreg(regs, (extra >> 12) & 7) = (m68k_dreg(regs, (extra >> 12) & 7) & ~0xff) | ((src) & 0xff);
		}
		m68k_incpc(4);
	}
	return 20 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVES.B #<data>.W,-(An) */
uae_u32 REGPARAM2 op_0e20_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uae_s16 extra = get_diword (2);
	if (extra & 0x800) {
		uae_u32 src = regs.regs[(extra >> 12) & 15];
		uaecptr dsta;
		dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
		m68k_areg(regs, dstreg) = dsta;
		src = regs.regs[(extra >> 12) & 15];
		put_byte_jit(dsta, src);
		m68k_incpc(4);
	} else {
		uaecptr srca;
		srca = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
		uae_s8 src = get_byte_jit(srca);
		m68k_areg(regs, dstreg) = srca;
		if (extra & 0x8000) {
			m68k_areg(regs, (extra >> 12) & 7) = (uae_s32)(uae_s8)src;
		} else {
			m68k_dreg(regs, (extra >> 12) & 7) = (m68k_dreg(regs, (extra >> 12) & 7) & ~0xff) | ((src) & 0xff);
		}
		m68k_incpc(4);
	}
	return 20 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVES.B #<data>.W,(d16,An) */
uae_u32 REGPARAM2 op_0e28_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uae_s16 extra = get_diword (2);
	if (extra & 0x800) {
		uae_u32 src = regs.regs[(extra >> 12) & 15];
		uaecptr dsta;
		dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
		put_byte_jit(dsta, src);
		m68k_incpc(6);
	} else {
		uaecptr srca;
		srca = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
		uae_s8 src = get_byte_jit(srca);
		if (extra & 0x8000) {
			m68k_areg(regs, (extra >> 12) & 7) = (uae_s32)(uae_s8)src;
		} else {
			m68k_dreg(regs, (extra >> 12) & 7) = (m68k_dreg(regs, (extra >> 12) & 7) & ~0xff) | ((src) & 0xff);
		}
		m68k_incpc(6);
	}
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVES.B #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 op_0e30_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uae_s16 extra = get_diword (2);
	if (extra & 0x800) {
		uae_u32 src = regs.regs[(extra >> 12) & 15];
		uaecptr dsta;
		dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
		put_byte_jit(dsta, src);
		m68k_incpc(6);
	} else {
		uaecptr srca;
		srca = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
		uae_s8 src = get_byte_jit(srca);
		if (extra & 0x8000) {
			m68k_areg(regs, (extra >> 12) & 7) = (uae_s32)(uae_s8)src;
		} else {
			m68k_dreg(regs, (extra >> 12) & 7) = (m68k_dreg(regs, (extra >> 12) & 7) & ~0xff) | ((src) & 0xff);
		}
		m68k_incpc(6);
	}
	return 24 * CYCLE_UNIT / 2;
}
/* 6 4,4   */

/* MOVES.B #<data>.W,(xxx).W */
uae_u32 REGPARAM2 op_0e38_44_ff(uae_u32 opcode)
{
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uae_s16 extra = get_diword (2);
	if (extra & 0x800) {
		uae_u32 src = regs.regs[(extra >> 12) & 15];
		uaecptr dsta;
		dsta = (uae_s32)(uae_s16)get_diword (4);
		put_byte_jit(dsta, src);
		m68k_incpc(6);
	} else {
		uaecptr srca;
		srca = (uae_s32)(uae_s16)get_diword (4);
		uae_s8 src = get_byte_jit(srca);
		if (extra & 0x8000) {
			m68k_areg(regs, (extra >> 12) & 7) = (uae_s32)(uae_s8)src;
		} else {
			m68k_dreg(regs, (extra >> 12) & 7) = (m68k_dreg(regs, (extra >> 12) & 7) & ~0xff) | ((src) & 0xff);
		}
		m68k_incpc(6);
	}
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVES.B #<data>.W,(xxx).L */
uae_u32 REGPARAM2 op_0e39_44_ff(uae_u32 opcode)
{
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uae_s16 extra = get_diword (2);
	if (extra & 0x800) {
		uae_u32 src = regs.regs[(extra >> 12) & 15];
		uaecptr dsta;
		dsta = get_dilong(4);
		put_byte_jit(dsta, src);
		m68k_incpc(8);
	} else {
		uaecptr srca;
		srca = get_dilong(4);
		uae_s8 src = get_byte_jit(srca);
		if (extra & 0x8000) {
			m68k_areg(regs, (extra >> 12) & 7) = (uae_s32)(uae_s8)src;
		} else {
			m68k_dreg(regs, (extra >> 12) & 7) = (m68k_dreg(regs, (extra >> 12) & 7) & ~0xff) | ((src) & 0xff);
		}
		m68k_incpc(8);
	}
	return 24 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* MOVES.W #<data>.W,(An) */
uae_u32 REGPARAM2 op_0e50_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uae_s16 extra = get_diword (2);
	if (extra & 0x800) {
		uae_u32 src = regs.regs[(extra >> 12) & 15];
		uaecptr dsta;
		dsta = m68k_areg(regs, dstreg);
		put_word_jit(dsta, src);
		m68k_incpc(4);
	} else {
		uaecptr srca;
		srca = m68k_areg(regs, dstreg);
		uae_s16 src = get_word_jit(srca);
		if (extra & 0x8000) {
			m68k_areg(regs, (extra >> 12) & 7) = (uae_s32)(uae_s16)src;
		} else {
			m68k_dreg(regs, (extra >> 12) & 7) = (m68k_dreg(regs, (extra >> 12) & 7) & ~0xffff) | ((src) & 0xffff);
		}
		m68k_incpc(4);
	}
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVES.W #<data>.W,(An)+ */
uae_u32 REGPARAM2 op_0e58_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uae_s16 extra = get_diword (2);
	if (extra & 0x800) {
		uae_u32 src = regs.regs[(extra >> 12) & 15];
		uaecptr dsta;
		dsta = m68k_areg(regs, dstreg);
		m68k_areg(regs, dstreg) += 2;
		put_word_jit(dsta, src);
		m68k_incpc(4);
	} else {
		uaecptr srca;
		srca = m68k_areg(regs, dstreg);
		uae_s16 src = get_word_jit(srca);
		m68k_areg(regs, dstreg) += 2;
		if (extra & 0x8000) {
			m68k_areg(regs, (extra >> 12) & 7) = (uae_s32)(uae_s16)src;
		} else {
			m68k_dreg(regs, (extra >> 12) & 7) = (m68k_dreg(regs, (extra >> 12) & 7) & ~0xffff) | ((src) & 0xffff);
		}
		m68k_incpc(4);
	}
	return 20 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVES.W #<data>.W,-(An) */
uae_u32 REGPARAM2 op_0e60_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uae_s16 extra = get_diword (2);
	if (extra & 0x800) {
		uae_u32 src = regs.regs[(extra >> 12) & 15];
		uaecptr dsta;
		dsta = m68k_areg(regs, dstreg) - 2;
		m68k_areg(regs, dstreg) = dsta;
		src = regs.regs[(extra >> 12) & 15];
		put_word_jit(dsta, src);
		m68k_incpc(4);
	} else {
		uaecptr srca;
		srca = m68k_areg(regs, dstreg) - 2;
		uae_s16 src = get_word_jit(srca);
		m68k_areg(regs, dstreg) = srca;
		if (extra & 0x8000) {
			m68k_areg(regs, (extra >> 12) & 7) = (uae_s32)(uae_s16)src;
		} else {
			m68k_dreg(regs, (extra >> 12) & 7) = (m68k_dreg(regs, (extra >> 12) & 7) & ~0xffff) | ((src) & 0xffff);
		}
		m68k_incpc(4);
	}
	return 20 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVES.W #<data>.W,(d16,An) */
uae_u32 REGPARAM2 op_0e68_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uae_s16 extra = get_diword (2);
	if (extra & 0x800) {
		uae_u32 src = regs.regs[(extra >> 12) & 15];
		uaecptr dsta;
		dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
		put_word_jit(dsta, src);
		m68k_incpc(6);
	} else {
		uaecptr srca;
		srca = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
		uae_s16 src = get_word_jit(srca);
		if (extra & 0x8000) {
			m68k_areg(regs, (extra >> 12) & 7) = (uae_s32)(uae_s16)src;
		} else {
			m68k_dreg(regs, (extra >> 12) & 7) = (m68k_dreg(regs, (extra >> 12) & 7) & ~0xffff) | ((src) & 0xffff);
		}
		m68k_incpc(6);
	}
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVES.W #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 op_0e70_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uae_s16 extra = get_diword (2);
	if (extra & 0x800) {
		uae_u32 src = regs.regs[(extra >> 12) & 15];
		uaecptr dsta;
		dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
		put_word_jit(dsta, src);
		m68k_incpc(6);
	} else {
		uaecptr srca;
		srca = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
		uae_s16 src = get_word_jit(srca);
		if (extra & 0x8000) {
			m68k_areg(regs, (extra >> 12) & 7) = (uae_s32)(uae_s16)src;
		} else {
			m68k_dreg(regs, (extra >> 12) & 7) = (m68k_dreg(regs, (extra >> 12) & 7) & ~0xffff) | ((src) & 0xffff);
		}
		m68k_incpc(6);
	}
	return 24 * CYCLE_UNIT / 2;
}
/* 6 4,4   */

/* MOVES.W #<data>.W,(xxx).W */
uae_u32 REGPARAM2 op_0e78_44_ff(uae_u32 opcode)
{
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uae_s16 extra = get_diword (2);
	if (extra & 0x800) {
		uae_u32 src = regs.regs[(extra >> 12) & 15];
		uaecptr dsta;
		dsta = (uae_s32)(uae_s16)get_diword (4);
		put_word_jit(dsta, src);
		m68k_incpc(6);
	} else {
		uaecptr srca;
		srca = (uae_s32)(uae_s16)get_diword (4);
		uae_s16 src = get_word_jit(srca);
		if (extra & 0x8000) {
			m68k_areg(regs, (extra >> 12) & 7) = (uae_s32)(uae_s16)src;
		} else {
			m68k_dreg(regs, (extra >> 12) & 7) = (m68k_dreg(regs, (extra >> 12) & 7) & ~0xffff) | ((src) & 0xffff);
		}
		m68k_incpc(6);
	}
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVES.W #<data>.W,(xxx).L */
uae_u32 REGPARAM2 op_0e79_44_ff(uae_u32 opcode)
{
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uae_s16 extra = get_diword (2);
	if (extra & 0x800) {
		uae_u32 src = regs.regs[(extra >> 12) & 15];
		uaecptr dsta;
		dsta = get_dilong(4);
		put_word_jit(dsta, src);
		m68k_incpc(8);
	} else {
		uaecptr srca;
		srca = get_dilong(4);
		uae_s16 src = get_word_jit(srca);
		if (extra & 0x8000) {
			m68k_areg(regs, (extra >> 12) & 7) = (uae_s32)(uae_s16)src;
		} else {
			m68k_dreg(regs, (extra >> 12) & 7) = (m68k_dreg(regs, (extra >> 12) & 7) & ~0xffff) | ((src) & 0xffff);
		}
		m68k_incpc(8);
	}
	return 24 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* MOVES.L #<data>.W,(An) */
uae_u32 REGPARAM2 op_0e90_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uae_s16 extra = get_diword (2);
	if (extra & 0x800) {
		uae_u32 src = regs.regs[(extra >> 12) & 15];
		uaecptr dsta;
		dsta = m68k_areg(regs, dstreg);
		put_long_jit(dsta, src);
		m68k_incpc(4);
	} else {
		uaecptr srca;
		srca = m68k_areg(regs, dstreg);
		uae_s32 src = get_long_jit(srca);
		if (extra & 0x8000) {
			m68k_areg(regs, (extra >> 12) & 7) = src;
		} else {
			m68k_dreg(regs, (extra >> 12) & 7) = (src);
		}
		m68k_incpc(4);
	}
	return 22 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVES.L #<data>.W,(An)+ */
uae_u32 REGPARAM2 op_0e98_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uae_s16 extra = get_diword (2);
	if (extra & 0x800) {
		uae_u32 src = regs.regs[(extra >> 12) & 15];
		uaecptr dsta;
		dsta = m68k_areg(regs, dstreg);
		m68k_areg(regs, dstreg) += 4;
		put_long_jit(dsta, src);
		m68k_incpc(4);
	} else {
		uaecptr srca;
		srca = m68k_areg(regs, dstreg);
		uae_s32 src = get_long_jit(srca);
		m68k_areg(regs, dstreg) += 4;
		if (extra & 0x8000) {
			m68k_areg(regs, (extra >> 12) & 7) = src;
		} else {
			m68k_dreg(regs, (extra >> 12) & 7) = (src);
		}
		m68k_incpc(4);
	}
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVES.L #<data>.W,-(An) */
uae_u32 REGPARAM2 op_0ea0_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uae_s16 extra = get_diword (2);
	if (extra & 0x800) {
		uae_u32 src = regs.regs[(extra >> 12) & 15];
		uaecptr dsta;
		dsta = m68k_areg(regs, dstreg) - 4;
		m68k_areg(regs, dstreg) = dsta;
		src = regs.regs[(extra >> 12) & 15];
		put_long_jit(dsta, src);
		m68k_incpc(4);
	} else {
		uaecptr srca;
		srca = m68k_areg(regs, dstreg) - 4;
		uae_s32 src = get_long_jit(srca);
		m68k_areg(regs, dstreg) = srca;
		if (extra & 0x8000) {
			m68k_areg(regs, (extra >> 12) & 7) = src;
		} else {
			m68k_dreg(regs, (extra >> 12) & 7) = (src);
		}
		m68k_incpc(4);
	}
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVES.L #<data>.W,(d16,An) */
uae_u32 REGPARAM2 op_0ea8_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uae_s16 extra = get_diword (2);
	if (extra & 0x800) {
		uae_u32 src = regs.regs[(extra >> 12) & 15];
		uaecptr dsta;
		dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
		put_long_jit(dsta, src);
		m68k_incpc(6);
	} else {
		uaecptr srca;
		srca = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
		uae_s32 src = get_long_jit(srca);
		if (extra & 0x8000) {
			m68k_areg(regs, (extra >> 12) & 7) = src;
		} else {
			m68k_dreg(regs, (extra >> 12) & 7) = (src);
		}
		m68k_incpc(6);
	}
	return 24 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVES.L #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 op_0eb0_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uae_s16 extra = get_diword (2);
	if (extra & 0x800) {
		uae_u32 src = regs.regs[(extra >> 12) & 15];
		uaecptr dsta;
		dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
		put_long_jit(dsta, src);
		m68k_incpc(6);
	} else {
		uaecptr srca;
		srca = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
		uae_s32 src = get_long_jit(srca);
		if (extra & 0x8000) {
			m68k_areg(regs, (extra >> 12) & 7) = src;
		} else {
			m68k_dreg(regs, (extra >> 12) & 7) = (src);
		}
		m68k_incpc(6);
	}
	return 28 * CYCLE_UNIT / 2;
}
/* 6 4,4   */

/* MOVES.L #<data>.W,(xxx).W */
uae_u32 REGPARAM2 op_0eb8_44_ff(uae_u32 opcode)
{
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uae_s16 extra = get_diword (2);
	if (extra & 0x800) {
		uae_u32 src = regs.regs[(extra >> 12) & 15];
		uaecptr dsta;
		dsta = (uae_s32)(uae_s16)get_diword (4);
		put_long_jit(dsta, src);
		m68k_incpc(6);
	} else {
		uaecptr srca;
		srca = (uae_s32)(uae_s16)get_diword (4);
		uae_s32 src = get_long_jit(srca);
		if (extra & 0x8000) {
			m68k_areg(regs, (extra >> 12) & 7) = src;
		} else {
			m68k_dreg(regs, (extra >> 12) & 7) = (src);
		}
		m68k_incpc(6);
	}
	return 24 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVES.L #<data>.W,(xxx).L */
uae_u32 REGPARAM2 op_0eb9_44_ff(uae_u32 opcode)
{
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uae_s16 extra = get_diword (2);
	if (extra & 0x800) {
		uae_u32 src = regs.regs[(extra >> 12) & 15];
		uaecptr dsta;
		dsta = get_dilong(4);
		put_long_jit(dsta, src);
		m68k_incpc(8);
	} else {
		uaecptr srca;
		srca = get_dilong(4);
		uae_s32 src = get_long_jit(srca);
		if (extra & 0x8000) {
			m68k_areg(regs, (extra >> 12) & 7) = src;
		} else {
			m68k_dreg(regs, (extra >> 12) & 7) = (src);
		}
		m68k_incpc(8);
	}
	return 28 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* MOVE.B Dn,Dn */
uae_u32 REGPARAM2 op_1000_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.B (An),Dn */
uae_u32 REGPARAM2 op_1010_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.B (An)+,Dn */
uae_u32 REGPARAM2 op_1018_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.B -(An),Dn */
uae_u32 REGPARAM2 op_1020_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.B (d16,An),Dn */
uae_u32 REGPARAM2 op_1028_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.B (d8,An,Xn),Dn */
uae_u32 REGPARAM2 op_1030_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVE.B (xxx).W,Dn */
uae_u32 REGPARAM2 op_1038_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.B (xxx).L,Dn */
uae_u32 REGPARAM2 op_1039_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s8 src = get_byte_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.B (d16,PC),Dn */
uae_u32 REGPARAM2 op_103a_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.B (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 op_103b_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVE.B #<data>.B,Dn */
uae_u32 REGPARAM2 op_103c_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s8 src = get_dibyte (2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc(4);
	return 8 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.B Dn,(An) */
uae_u32 REGPARAM2 op_1080_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.B (An),(An) */
uae_u32 REGPARAM2 op_1090_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.B (An)+,(An) */
uae_u32 REGPARAM2 op_1098_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.B -(An),(An) */
uae_u32 REGPARAM2 op_10a0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.B (d16,An),(An) */
uae_u32 REGPARAM2 op_10a8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.B (d8,An,Xn),(An) */
uae_u32 REGPARAM2 op_10b0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVE.B (xxx).W,(An) */
uae_u32 REGPARAM2 op_10b8_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.B (xxx).L,(An) */
uae_u32 REGPARAM2 op_10b9_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.B (d16,PC),(An) */
uae_u32 REGPARAM2 op_10ba_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.B (d8,PC,Xn),(An) */
uae_u32 REGPARAM2 op_10bb_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVE.B #<data>.B,(An) */
uae_u32 REGPARAM2 op_10bc_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.B Dn,(An)+ */
uae_u32 REGPARAM2 op_10c0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.B (An),(An)+ */
uae_u32 REGPARAM2 op_10d0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.B (An)+,(An)+ */
uae_u32 REGPARAM2 op_10d8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.B -(An),(An)+ */
uae_u32 REGPARAM2 op_10e0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.B (d16,An),(An)+ */
uae_u32 REGPARAM2 op_10e8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.B (d8,An,Xn),(An)+ */
uae_u32 REGPARAM2 op_10f0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVE.B (xxx).W,(An)+ */
uae_u32 REGPARAM2 op_10f8_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.B (xxx).L,(An)+ */
uae_u32 REGPARAM2 op_10f9_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.B (d16,PC),(An)+ */
uae_u32 REGPARAM2 op_10fa_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.B (d8,PC,Xn),(An)+ */
uae_u32 REGPARAM2 op_10fb_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVE.B #<data>.B,(An)+ */
uae_u32 REGPARAM2 op_10fc_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.B Dn,-(An) */
uae_u32 REGPARAM2 op_1100_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg(regs, dstreg) = dsta;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.B (An),-(An) */
uae_u32 REGPARAM2 op_1110_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg(regs, dstreg) = dsta;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.B (An)+,-(An) */
uae_u32 REGPARAM2 op_1118_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg(regs, dstreg) = dsta;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.B -(An),-(An) */
uae_u32 REGPARAM2 op_1120_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg(regs, dstreg) = dsta;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.B (d16,An),-(An) */
uae_u32 REGPARAM2 op_1128_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg(regs, dstreg) = dsta;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.B (d8,An,Xn),-(An) */
uae_u32 REGPARAM2 op_1130_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg(regs, dstreg) = dsta;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVE.B (xxx).W,-(An) */
uae_u32 REGPARAM2 op_1138_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg(regs, dstreg) = dsta;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.B (xxx).L,-(An) */
uae_u32 REGPARAM2 op_1139_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg(regs, dstreg) = dsta;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.B (d16,PC),-(An) */
uae_u32 REGPARAM2 op_113a_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg(regs, dstreg) = dsta;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.B (d8,PC,Xn),-(An) */
uae_u32 REGPARAM2 op_113b_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg(regs, dstreg) = dsta;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVE.B #<data>.B,-(An) */
uae_u32 REGPARAM2 op_113c_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	m68k_areg(regs, dstreg) = dsta;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.B Dn,(d16,An) */
uae_u32 REGPARAM2 op_1140_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.B (An),(d16,An) */
uae_u32 REGPARAM2 op_1150_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.B (An)+,(d16,An) */
uae_u32 REGPARAM2 op_1158_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.B -(An),(d16,An) */
uae_u32 REGPARAM2 op_1160_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.B (d16,An),(d16,An) */
uae_u32 REGPARAM2 op_1168_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.B (d8,An,Xn),(d16,An) */
uae_u32 REGPARAM2 op_1170_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 6,0   */

/* MOVE.B (xxx).W,(d16,An) */
uae_u32 REGPARAM2 op_1178_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.B (xxx).L,(d16,An) */
uae_u32 REGPARAM2 op_1179_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (6);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(8);
	return 24 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* MOVE.B (d16,PC),(d16,An) */
uae_u32 REGPARAM2 op_117a_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.B (d8,PC,Xn),(d16,An) */
uae_u32 REGPARAM2 op_117b_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 6,0   */

/* MOVE.B #<data>.B,(d16,An) */
uae_u32 REGPARAM2 op_117c_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.B Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 op_1180_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (2));
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVE.B (An),(d8,An,Xn) */
uae_u32 REGPARAM2 op_1190_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (2));
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVE.B (An)+,(d8,An,Xn) */
uae_u32 REGPARAM2 op_1198_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (2));
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVE.B -(An),(d8,An,Xn) */
uae_u32 REGPARAM2 op_11a0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (2));
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVE.B (d16,An),(d8,An,Xn) */
uae_u32 REGPARAM2 op_11a8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 4,0   */

/* MOVE.B (d8,An,Xn),(d8,An,Xn) */
uae_u32 REGPARAM2 op_11b0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(6);
	return 24 * CYCLE_UNIT / 2;
}
/* 6 6,4   */

/* MOVE.B (xxx).W,(d8,An,Xn) */
uae_u32 REGPARAM2 op_11b8_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 4,0   */

/* MOVE.B (xxx).L,(d8,An,Xn) */
uae_u32 REGPARAM2 op_11b9_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (6));
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(8);
	return 26 * CYCLE_UNIT / 2;
}
/* 8 4,0   */

/* MOVE.B (d16,PC),(d8,An,Xn) */
uae_u32 REGPARAM2 op_11ba_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 4,0   */

/* MOVE.B (d8,PC,Xn),(d8,An,Xn) */
uae_u32 REGPARAM2 op_11bb_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(6);
	return 24 * CYCLE_UNIT / 2;
}
/* 6 6,4   */

/* MOVE.B #<data>.B,(d8,An,Xn) */
uae_u32 REGPARAM2 op_11bc_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(6);
	return 18 * CYCLE_UNIT / 2;
}
/* 6 4,0   */

/* MOVE.B Dn,(xxx).W */
uae_u32 REGPARAM2 op_11c0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.B (An),(xxx).W */
uae_u32 REGPARAM2 op_11d0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.B (An)+,(xxx).W */
uae_u32 REGPARAM2 op_11d8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.B -(An),(xxx).W */
uae_u32 REGPARAM2 op_11e0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.B (d16,An),(xxx).W */
uae_u32 REGPARAM2 op_11e8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.B (d8,An,Xn),(xxx).W */
uae_u32 REGPARAM2 op_11f0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 6,0   */

/* MOVE.B (xxx).W,(xxx).W */
uae_u32 REGPARAM2 op_11f8_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.B (xxx).L,(xxx).W */
uae_u32 REGPARAM2 op_11f9_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (6);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(8);
	return 24 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* MOVE.B (d16,PC),(xxx).W */
uae_u32 REGPARAM2 op_11fa_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.B (d8,PC,Xn),(xxx).W */
uae_u32 REGPARAM2 op_11fb_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 6,0   */

/* MOVE.B #<data>.B,(xxx).W */
uae_u32 REGPARAM2 op_11fc_44_ff(uae_u32 opcode)
{
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.B Dn,(xxx).L */
uae_u32 REGPARAM2 op_13c0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_dilong(2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.B (An),(xxx).L */
uae_u32 REGPARAM2 op_13d0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = get_dilong(2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.B (An)+,(xxx).L */
uae_u32 REGPARAM2 op_13d8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	uaecptr dsta;
	dsta = get_dilong(2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.B -(An),(xxx).L */
uae_u32 REGPARAM2 op_13e0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uaecptr dsta;
	dsta = get_dilong(2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.B (d16,An),(xxx).L */
uae_u32 REGPARAM2 op_13e8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = get_dilong(4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(8);
	return 24 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* MOVE.B (d8,An,Xn),(xxx).L */
uae_u32 REGPARAM2 op_13f0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = get_dilong(4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(8);
	return 26 * CYCLE_UNIT / 2;
}
/* 8 8,0   */

/* MOVE.B (xxx).W,(xxx).L */
uae_u32 REGPARAM2 op_13f8_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = get_dilong(4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(8);
	return 24 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* MOVE.B (xxx).L,(xxx).L */
uae_u32 REGPARAM2 op_13f9_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = get_dilong(6);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(10);
	return 28 * CYCLE_UNIT / 2;
}
/* 10 0,0   */

/* MOVE.B (d16,PC),(xxx).L */
uae_u32 REGPARAM2 op_13fa_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = get_dilong(4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(8);
	return 24 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* MOVE.B (d8,PC,Xn),(xxx).L */
uae_u32 REGPARAM2 op_13fb_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	uaecptr dsta;
	dsta = get_dilong(4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(8);
	return 26 * CYCLE_UNIT / 2;
}
/* 8 8,0   */

/* MOVE.B #<data>.B,(xxx).L */
uae_u32 REGPARAM2 op_13fc_44_ff(uae_u32 opcode)
{
	uae_s8 src = get_dibyte (2);
	uaecptr dsta;
	dsta = get_dilong(4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(8);
	return 20 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* MOVE.L Dn,Dn */
uae_u32 REGPARAM2 op_2000_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.L An,Dn */
uae_u32 REGPARAM2 op_2008_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src = m68k_areg(regs, srcreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.L (An),Dn */
uae_u32 REGPARAM2 op_2010_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.L (An)+,Dn */
uae_u32 REGPARAM2 op_2018_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) += 4;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.L -(An),Dn */
uae_u32 REGPARAM2 op_2020_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.L (d16,An),Dn */
uae_u32 REGPARAM2 op_2028_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.L (d8,An,Xn),Dn */
uae_u32 REGPARAM2 op_2030_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s32 src = get_long_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVE.L (xxx).W,Dn */
uae_u32 REGPARAM2 op_2038_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.L (xxx).L,Dn */
uae_u32 REGPARAM2 op_2039_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s32 src = get_long_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.L (d16,PC),Dn */
uae_u32 REGPARAM2 op_203a_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.L (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 op_203b_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s32 src = get_long_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVE.L #<data>.L,Dn */
uae_u32 REGPARAM2 op_203c_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src;
	src = get_dilong(2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(6);
	return 12 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVEA.L Dn,An */
uae_u32 REGPARAM2 op_2040_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	m68k_areg(regs, dstreg) = (src);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVEA.L An,An */
uae_u32 REGPARAM2 op_2048_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src = m68k_areg(regs, srcreg);
	m68k_areg(regs, dstreg) = (src);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVEA.L (An),An */
uae_u32 REGPARAM2 op_2050_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, dstreg) = (src);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVEA.L (An)+,An */
uae_u32 REGPARAM2 op_2058_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) += 4;
	m68k_areg(regs, dstreg) = (src);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVEA.L -(An),An */
uae_u32 REGPARAM2 op_2060_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	m68k_areg(regs, dstreg) = (src);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVEA.L (d16,An),An */
uae_u32 REGPARAM2 op_2068_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, dstreg) = (src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVEA.L (d8,An,Xn),An */
uae_u32 REGPARAM2 op_2070_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, dstreg) = (src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVEA.L (xxx).W,An */
uae_u32 REGPARAM2 op_2078_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, dstreg) = (src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVEA.L (xxx).L,An */
uae_u32 REGPARAM2 op_2079_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, dstreg) = (src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVEA.L (d16,PC),An */
uae_u32 REGPARAM2 op_207a_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, dstreg) = (src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVEA.L (d8,PC,Xn),An */
uae_u32 REGPARAM2 op_207b_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, dstreg) = (src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVEA.L #<data>.L,An */
uae_u32 REGPARAM2 op_207c_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src;
	src = get_dilong(2);
	m68k_areg(regs, dstreg) = (src);
	m68k_incpc(6);
	return 12 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.L Dn,(An) */
uae_u32 REGPARAM2 op_2080_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.L An,(An) */
uae_u32 REGPARAM2 op_2088_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src = m68k_areg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.L (An),(An) */
uae_u32 REGPARAM2 op_2090_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(2);
	return 20 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.L (An)+,(An) */
uae_u32 REGPARAM2 op_2098_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) += 4;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(2);
	return 20 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.L -(An),(An) */
uae_u32 REGPARAM2 op_20a0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(2);
	return 22 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.L (d16,An),(An) */
uae_u32 REGPARAM2 op_20a8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(4);
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.L (d8,An,Xn),(An) */
uae_u32 REGPARAM2 op_20b0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(4);
	return 26 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVE.L (xxx).W,(An) */
uae_u32 REGPARAM2 op_20b8_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(4);
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.L (xxx).L,(An) */
uae_u32 REGPARAM2 op_20b9_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(6);
	return 28 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.L (d16,PC),(An) */
uae_u32 REGPARAM2 op_20ba_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(4);
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.L (d8,PC,Xn),(An) */
uae_u32 REGPARAM2 op_20bb_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(4);
	return 26 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVE.L #<data>.L,(An) */
uae_u32 REGPARAM2 op_20bc_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.L Dn,(An)+ */
uae_u32 REGPARAM2 op_20c0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 4;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.L An,(An)+ */
uae_u32 REGPARAM2 op_20c8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src = m68k_areg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 4;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.L (An),(An)+ */
uae_u32 REGPARAM2 op_20d0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 4;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(2);
	return 20 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.L (An)+,(An)+ */
uae_u32 REGPARAM2 op_20d8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) += 4;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 4;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(2);
	return 20 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.L -(An),(An)+ */
uae_u32 REGPARAM2 op_20e0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 4;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(2);
	return 22 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.L (d16,An),(An)+ */
uae_u32 REGPARAM2 op_20e8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 4;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(4);
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.L (d8,An,Xn),(An)+ */
uae_u32 REGPARAM2 op_20f0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 4;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(4);
	return 26 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVE.L (xxx).W,(An)+ */
uae_u32 REGPARAM2 op_20f8_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 4;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(4);
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.L (xxx).L,(An)+ */
uae_u32 REGPARAM2 op_20f9_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 4;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(6);
	return 28 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.L (d16,PC),(An)+ */
uae_u32 REGPARAM2 op_20fa_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 4;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(4);
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.L (d8,PC,Xn),(An)+ */
uae_u32 REGPARAM2 op_20fb_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 4;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(4);
	return 26 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVE.L #<data>.L,(An)+ */
uae_u32 REGPARAM2 op_20fc_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 4;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.L Dn,-(An) */
uae_u32 REGPARAM2 op_2100_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	m68k_areg(regs, dstreg) = dsta;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.L An,-(An) */
uae_u32 REGPARAM2 op_2108_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src = m68k_areg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	m68k_areg(regs, dstreg) = dsta;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.L (An),-(An) */
uae_u32 REGPARAM2 op_2110_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	m68k_areg(regs, dstreg) = dsta;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(2);
	return 20 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.L (An)+,-(An) */
uae_u32 REGPARAM2 op_2118_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) += 4;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	m68k_areg(regs, dstreg) = dsta;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(2);
	return 20 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.L -(An),-(An) */
uae_u32 REGPARAM2 op_2120_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	m68k_areg(regs, dstreg) = dsta;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(2);
	return 22 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.L (d16,An),-(An) */
uae_u32 REGPARAM2 op_2128_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	m68k_areg(regs, dstreg) = dsta;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(4);
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.L (d8,An,Xn),-(An) */
uae_u32 REGPARAM2 op_2130_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	m68k_areg(regs, dstreg) = dsta;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(4);
	return 26 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVE.L (xxx).W,-(An) */
uae_u32 REGPARAM2 op_2138_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	m68k_areg(regs, dstreg) = dsta;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(4);
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.L (xxx).L,-(An) */
uae_u32 REGPARAM2 op_2139_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	m68k_areg(regs, dstreg) = dsta;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(6);
	return 28 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.L (d16,PC),-(An) */
uae_u32 REGPARAM2 op_213a_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	m68k_areg(regs, dstreg) = dsta;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(4);
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.L (d8,PC,Xn),-(An) */
uae_u32 REGPARAM2 op_213b_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	m68k_areg(regs, dstreg) = dsta;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(4);
	return 26 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVE.L #<data>.L,-(An) */
uae_u32 REGPARAM2 op_213c_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	m68k_areg(regs, dstreg) = dsta;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.L Dn,(d16,An) */
uae_u32 REGPARAM2 op_2140_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.L An,(d16,An) */
uae_u32 REGPARAM2 op_2148_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src = m68k_areg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.L (An),(d16,An) */
uae_u32 REGPARAM2 op_2150_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(4);
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.L (An)+,(d16,An) */
uae_u32 REGPARAM2 op_2158_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) += 4;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(4);
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.L -(An),(d16,An) */
uae_u32 REGPARAM2 op_2160_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(4);
	return 26 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.L (d16,An),(d16,An) */
uae_u32 REGPARAM2 op_2168_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(6);
	return 28 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.L (d8,An,Xn),(d16,An) */
uae_u32 REGPARAM2 op_2170_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(6);
	return 30 * CYCLE_UNIT / 2;
}
/* 6 6,0   */

/* MOVE.L (xxx).W,(d16,An) */
uae_u32 REGPARAM2 op_2178_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(6);
	return 28 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.L (xxx).L,(d16,An) */
uae_u32 REGPARAM2 op_2179_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (6);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(8);
	return 32 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* MOVE.L (d16,PC),(d16,An) */
uae_u32 REGPARAM2 op_217a_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(6);
	return 28 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.L (d8,PC,Xn),(d16,An) */
uae_u32 REGPARAM2 op_217b_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(6);
	return 30 * CYCLE_UNIT / 2;
}
/* 6 6,0   */

/* MOVE.L #<data>.L,(d16,An) */
uae_u32 REGPARAM2 op_217c_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (6);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(8);
	return 24 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* MOVE.L Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 op_2180_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (2));
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVE.L An,(d8,An,Xn) */
uae_u32 REGPARAM2 op_2188_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src = m68k_areg(regs, srcreg);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (2));
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVE.L (An),(d8,An,Xn) */
uae_u32 REGPARAM2 op_2190_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (2));
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(4);
	return 26 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVE.L (An)+,(d8,An,Xn) */
uae_u32 REGPARAM2 op_2198_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) += 4;
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (2));
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(4);
	return 26 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVE.L -(An),(d8,An,Xn) */
uae_u32 REGPARAM2 op_21a0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (2));
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(4);
	return 28 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVE.L (d16,An),(d8,An,Xn) */
uae_u32 REGPARAM2 op_21a8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(6);
	return 30 * CYCLE_UNIT / 2;
}
/* 6 4,0   */

/* MOVE.L (d8,An,Xn),(d8,An,Xn) */
uae_u32 REGPARAM2 op_21b0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(6);
	return 32 * CYCLE_UNIT / 2;
}
/* 6 6,4   */

/* MOVE.L (xxx).W,(d8,An,Xn) */
uae_u32 REGPARAM2 op_21b8_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(6);
	return 30 * CYCLE_UNIT / 2;
}
/* 6 4,0   */

/* MOVE.L (xxx).L,(d8,An,Xn) */
uae_u32 REGPARAM2 op_21b9_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (6));
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(8);
	return 34 * CYCLE_UNIT / 2;
}
/* 8 4,0   */

/* MOVE.L (d16,PC),(d8,An,Xn) */
uae_u32 REGPARAM2 op_21ba_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(6);
	return 30 * CYCLE_UNIT / 2;
}
/* 6 4,0   */

/* MOVE.L (d8,PC,Xn),(d8,An,Xn) */
uae_u32 REGPARAM2 op_21bb_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(6);
	return 32 * CYCLE_UNIT / 2;
}
/* 6 6,4   */

/* MOVE.L #<data>.L,(d8,An,Xn) */
uae_u32 REGPARAM2 op_21bc_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (6));
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(8);
	return 26 * CYCLE_UNIT / 2;
}
/* 8 4,0   */

/* MOVE.L Dn,(xxx).W */
uae_u32 REGPARAM2 op_21c0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.L An,(xxx).W */
uae_u32 REGPARAM2 op_21c8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_s32 src = m68k_areg(regs, srcreg);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.L (An),(xxx).W */
uae_u32 REGPARAM2 op_21d0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(4);
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.L (An)+,(xxx).W */
uae_u32 REGPARAM2 op_21d8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) += 4;
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(4);
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.L -(An),(xxx).W */
uae_u32 REGPARAM2 op_21e0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(4);
	return 26 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.L (d16,An),(xxx).W */
uae_u32 REGPARAM2 op_21e8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(6);
	return 28 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.L (d8,An,Xn),(xxx).W */
uae_u32 REGPARAM2 op_21f0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(6);
	return 30 * CYCLE_UNIT / 2;
}
/* 6 6,0   */

/* MOVE.L (xxx).W,(xxx).W */
uae_u32 REGPARAM2 op_21f8_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(6);
	return 28 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.L (xxx).L,(xxx).W */
uae_u32 REGPARAM2 op_21f9_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (6);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(8);
	return 32 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* MOVE.L (d16,PC),(xxx).W */
uae_u32 REGPARAM2 op_21fa_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(6);
	return 28 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.L (d8,PC,Xn),(xxx).W */
uae_u32 REGPARAM2 op_21fb_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(6);
	return 30 * CYCLE_UNIT / 2;
}
/* 6 6,0   */

/* MOVE.L #<data>.L,(xxx).W */
uae_u32 REGPARAM2 op_21fc_44_ff(uae_u32 opcode)
{
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (6);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(8);
	return 24 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* MOVE.L Dn,(xxx).L */
uae_u32 REGPARAM2 op_23c0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_dilong(2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.L An,(xxx).L */
uae_u32 REGPARAM2 op_23c8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_s32 src = m68k_areg(regs, srcreg);
	uaecptr dsta;
	dsta = get_dilong(2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.L (An),(xxx).L */
uae_u32 REGPARAM2 op_23d0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = get_dilong(2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(6);
	return 28 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.L (An)+,(xxx).L */
uae_u32 REGPARAM2 op_23d8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) += 4;
	uaecptr dsta;
	dsta = get_dilong(2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(6);
	return 28 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.L -(An),(xxx).L */
uae_u32 REGPARAM2 op_23e0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uaecptr dsta;
	dsta = get_dilong(2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(6);
	return 30 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.L (d16,An),(xxx).L */
uae_u32 REGPARAM2 op_23e8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = get_dilong(4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(8);
	return 32 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* MOVE.L (d8,An,Xn),(xxx).L */
uae_u32 REGPARAM2 op_23f0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = get_dilong(4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(8);
	return 34 * CYCLE_UNIT / 2;
}
/* 8 8,0   */

/* MOVE.L (xxx).W,(xxx).L */
uae_u32 REGPARAM2 op_23f8_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = get_dilong(4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(8);
	return 32 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* MOVE.L (xxx).L,(xxx).L */
uae_u32 REGPARAM2 op_23f9_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = get_dilong(6);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(10);
	return 36 * CYCLE_UNIT / 2;
}
/* 10 0,0   */

/* MOVE.L (d16,PC),(xxx).L */
uae_u32 REGPARAM2 op_23fa_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = get_dilong(4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(8);
	return 32 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* MOVE.L (d8,PC,Xn),(xxx).L */
uae_u32 REGPARAM2 op_23fb_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uaecptr dsta;
	dsta = get_dilong(4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(8);
	return 34 * CYCLE_UNIT / 2;
}
/* 8 8,0   */

/* MOVE.L #<data>.L,(xxx).L */
uae_u32 REGPARAM2 op_23fc_44_ff(uae_u32 opcode)
{
	uae_s32 src;
	src = get_dilong(2);
	uaecptr dsta;
	dsta = get_dilong(6);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(10);
	return 28 * CYCLE_UNIT / 2;
}
/* 10 0,0   */

/* MOVE.W Dn,Dn */
uae_u32 REGPARAM2 op_3000_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.W An,Dn */
uae_u32 REGPARAM2 op_3008_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = m68k_areg(regs, srcreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.W (An),Dn */
uae_u32 REGPARAM2 op_3010_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.W (An)+,Dn */
uae_u32 REGPARAM2 op_3018_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) += 2;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.W -(An),Dn */
uae_u32 REGPARAM2 op_3020_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.W (d16,An),Dn */
uae_u32 REGPARAM2 op_3028_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.W (d8,An,Xn),Dn */
uae_u32 REGPARAM2 op_3030_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s16 src = get_word_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVE.W (xxx).W,Dn */
uae_u32 REGPARAM2 op_3038_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.W (xxx).L,Dn */
uae_u32 REGPARAM2 op_3039_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s16 src = get_word_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.W (d16,PC),Dn */
uae_u32 REGPARAM2 op_303a_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.W (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 op_303b_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s16 src = get_word_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVE.W #<data>.W,Dn */
uae_u32 REGPARAM2 op_303c_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = get_diword (2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc(4);
	return 8 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVEA.W Dn,An */
uae_u32 REGPARAM2 op_3040_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	src = (uae_s32)(uae_s16)src;
	m68k_areg(regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVEA.W An,An */
uae_u32 REGPARAM2 op_3048_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = m68k_areg(regs, srcreg);
	src = (uae_s32)(uae_s16)src;
	m68k_areg(regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVEA.W (An),An */
uae_u32 REGPARAM2 op_3050_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	src = (uae_s32)(uae_s16)src;
	m68k_areg(regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVEA.W (An)+,An */
uae_u32 REGPARAM2 op_3058_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) += 2;
	src = (uae_s32)(uae_s16)src;
	m68k_areg(regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVEA.W -(An),An */
uae_u32 REGPARAM2 op_3060_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	src = (uae_s32)(uae_s16)src;
	m68k_areg(regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVEA.W (d16,An),An */
uae_u32 REGPARAM2 op_3068_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	src = (uae_s32)(uae_s16)src;
	m68k_areg(regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVEA.W (d8,An,Xn),An */
uae_u32 REGPARAM2 op_3070_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s16 src = get_word_jit(srca);
	src = (uae_s32)(uae_s16)src;
	m68k_areg(regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVEA.W (xxx).W,An */
uae_u32 REGPARAM2 op_3078_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	src = (uae_s32)(uae_s16)src;
	m68k_areg(regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVEA.W (xxx).L,An */
uae_u32 REGPARAM2 op_3079_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s16 src = get_word_jit(srca);
	src = (uae_s32)(uae_s16)src;
	m68k_areg(regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVEA.W (d16,PC),An */
uae_u32 REGPARAM2 op_307a_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	src = (uae_s32)(uae_s16)src;
	m68k_areg(regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVEA.W (d8,PC,Xn),An */
uae_u32 REGPARAM2 op_307b_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s16 src = get_word_jit(srca);
	src = (uae_s32)(uae_s16)src;
	m68k_areg(regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVEA.W #<data>.W,An */
uae_u32 REGPARAM2 op_307c_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = get_diword (2);
	src = (uae_s32)(uae_s16)src;
	m68k_areg(regs, dstreg) = (uae_s32)(uae_s16)(src);
	m68k_incpc(4);
	return 8 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.W Dn,(An) */
uae_u32 REGPARAM2 op_3080_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.W An,(An) */
uae_u32 REGPARAM2 op_3088_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = m68k_areg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.W (An),(An) */
uae_u32 REGPARAM2 op_3090_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.W (An)+,(An) */
uae_u32 REGPARAM2 op_3098_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) += 2;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.W -(An),(An) */
uae_u32 REGPARAM2 op_30a0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.W (d16,An),(An) */
uae_u32 REGPARAM2 op_30a8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.W (d8,An,Xn),(An) */
uae_u32 REGPARAM2 op_30b0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVE.W (xxx).W,(An) */
uae_u32 REGPARAM2 op_30b8_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.W (xxx).L,(An) */
uae_u32 REGPARAM2 op_30b9_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.W (d16,PC),(An) */
uae_u32 REGPARAM2 op_30ba_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.W (d8,PC,Xn),(An) */
uae_u32 REGPARAM2 op_30bb_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVE.W #<data>.W,(An) */
uae_u32 REGPARAM2 op_30bc_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.W Dn,(An)+ */
uae_u32 REGPARAM2 op_30c0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 2;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.W An,(An)+ */
uae_u32 REGPARAM2 op_30c8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = m68k_areg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 2;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.W (An),(An)+ */
uae_u32 REGPARAM2 op_30d0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 2;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.W (An)+,(An)+ */
uae_u32 REGPARAM2 op_30d8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) += 2;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 2;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.W -(An),(An)+ */
uae_u32 REGPARAM2 op_30e0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 2;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.W (d16,An),(An)+ */
uae_u32 REGPARAM2 op_30e8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 2;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.W (d8,An,Xn),(An)+ */
uae_u32 REGPARAM2 op_30f0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 2;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVE.W (xxx).W,(An)+ */
uae_u32 REGPARAM2 op_30f8_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 2;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.W (xxx).L,(An)+ */
uae_u32 REGPARAM2 op_30f9_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 2;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.W (d16,PC),(An)+ */
uae_u32 REGPARAM2 op_30fa_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 2;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.W (d8,PC,Xn),(An)+ */
uae_u32 REGPARAM2 op_30fb_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 2;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVE.W #<data>.W,(An)+ */
uae_u32 REGPARAM2 op_30fc_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	m68k_areg(regs, dstreg) += 2;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.W Dn,-(An) */
uae_u32 REGPARAM2 op_3100_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	m68k_areg(regs, dstreg) = dsta;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.W An,-(An) */
uae_u32 REGPARAM2 op_3108_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = m68k_areg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	m68k_areg(regs, dstreg) = dsta;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.W (An),-(An) */
uae_u32 REGPARAM2 op_3110_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	m68k_areg(regs, dstreg) = dsta;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.W (An)+,-(An) */
uae_u32 REGPARAM2 op_3118_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) += 2;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	m68k_areg(regs, dstreg) = dsta;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.W -(An),-(An) */
uae_u32 REGPARAM2 op_3120_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	m68k_areg(regs, dstreg) = dsta;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.W (d16,An),-(An) */
uae_u32 REGPARAM2 op_3128_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	m68k_areg(regs, dstreg) = dsta;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.W (d8,An,Xn),-(An) */
uae_u32 REGPARAM2 op_3130_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	m68k_areg(regs, dstreg) = dsta;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVE.W (xxx).W,-(An) */
uae_u32 REGPARAM2 op_3138_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	m68k_areg(regs, dstreg) = dsta;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.W (xxx).L,-(An) */
uae_u32 REGPARAM2 op_3139_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	m68k_areg(regs, dstreg) = dsta;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.W (d16,PC),-(An) */
uae_u32 REGPARAM2 op_313a_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	m68k_areg(regs, dstreg) = dsta;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.W (d8,PC,Xn),-(An) */
uae_u32 REGPARAM2 op_313b_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	m68k_areg(regs, dstreg) = dsta;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVE.W #<data>.W,-(An) */
uae_u32 REGPARAM2 op_313c_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	m68k_areg(regs, dstreg) = dsta;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.W Dn,(d16,An) */
uae_u32 REGPARAM2 op_3140_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.W An,(d16,An) */
uae_u32 REGPARAM2 op_3148_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = m68k_areg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.W (An),(d16,An) */
uae_u32 REGPARAM2 op_3150_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.W (An)+,(d16,An) */
uae_u32 REGPARAM2 op_3158_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) += 2;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.W -(An),(d16,An) */
uae_u32 REGPARAM2 op_3160_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.W (d16,An),(d16,An) */
uae_u32 REGPARAM2 op_3168_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.W (d8,An,Xn),(d16,An) */
uae_u32 REGPARAM2 op_3170_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 6,0   */

/* MOVE.W (xxx).W,(d16,An) */
uae_u32 REGPARAM2 op_3178_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.W (xxx).L,(d16,An) */
uae_u32 REGPARAM2 op_3179_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (6);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(8);
	return 24 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* MOVE.W (d16,PC),(d16,An) */
uae_u32 REGPARAM2 op_317a_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.W (d8,PC,Xn),(d16,An) */
uae_u32 REGPARAM2 op_317b_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 6,0   */

/* MOVE.W #<data>.W,(d16,An) */
uae_u32 REGPARAM2 op_317c_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.W Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 op_3180_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (2));
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVE.W An,(d8,An,Xn) */
uae_u32 REGPARAM2 op_3188_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = m68k_areg(regs, srcreg);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (2));
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVE.W (An),(d8,An,Xn) */
uae_u32 REGPARAM2 op_3190_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (2));
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVE.W (An)+,(d8,An,Xn) */
uae_u32 REGPARAM2 op_3198_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) += 2;
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (2));
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVE.W -(An),(d8,An,Xn) */
uae_u32 REGPARAM2 op_31a0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (2));
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MOVE.W (d16,An),(d8,An,Xn) */
uae_u32 REGPARAM2 op_31a8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 4,0   */

/* MOVE.W (d8,An,Xn),(d8,An,Xn) */
uae_u32 REGPARAM2 op_31b0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(6);
	return 24 * CYCLE_UNIT / 2;
}
/* 6 6,4   */

/* MOVE.W (xxx).W,(d8,An,Xn) */
uae_u32 REGPARAM2 op_31b8_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 4,0   */

/* MOVE.W (xxx).L,(d8,An,Xn) */
uae_u32 REGPARAM2 op_31b9_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (6));
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(8);
	return 26 * CYCLE_UNIT / 2;
}
/* 8 4,0   */

/* MOVE.W (d16,PC),(d8,An,Xn) */
uae_u32 REGPARAM2 op_31ba_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 4,0   */

/* MOVE.W (d8,PC,Xn),(d8,An,Xn) */
uae_u32 REGPARAM2 op_31bb_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(6);
	return 24 * CYCLE_UNIT / 2;
}
/* 6 6,4   */

/* MOVE.W #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 op_31bc_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(6);
	return 18 * CYCLE_UNIT / 2;
}
/* 6 4,0   */

/* MOVE.W Dn,(xxx).W */
uae_u32 REGPARAM2 op_31c0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.W An,(xxx).W */
uae_u32 REGPARAM2 op_31c8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_areg(regs, srcreg);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.W (An),(xxx).W */
uae_u32 REGPARAM2 op_31d0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.W (An)+,(xxx).W */
uae_u32 REGPARAM2 op_31d8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) += 2;
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.W -(An),(xxx).W */
uae_u32 REGPARAM2 op_31e0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE.W (d16,An),(xxx).W */
uae_u32 REGPARAM2 op_31e8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.W (d8,An,Xn),(xxx).W */
uae_u32 REGPARAM2 op_31f0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 6,0   */

/* MOVE.W (xxx).W,(xxx).W */
uae_u32 REGPARAM2 op_31f8_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.W (xxx).L,(xxx).W */
uae_u32 REGPARAM2 op_31f9_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (6);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(8);
	return 24 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* MOVE.W (d16,PC),(xxx).W */
uae_u32 REGPARAM2 op_31fa_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.W (d8,PC,Xn),(xxx).W */
uae_u32 REGPARAM2 op_31fb_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 6,0   */

/* MOVE.W #<data>.W,(xxx).W */
uae_u32 REGPARAM2 op_31fc_44_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.W Dn,(xxx).L */
uae_u32 REGPARAM2 op_33c0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_dilong(2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.W An,(xxx).L */
uae_u32 REGPARAM2 op_33c8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_areg(regs, srcreg);
	uaecptr dsta;
	dsta = get_dilong(2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.W (An),(xxx).L */
uae_u32 REGPARAM2 op_33d0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = get_dilong(2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.W (An)+,(xxx).L */
uae_u32 REGPARAM2 op_33d8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) += 2;
	uaecptr dsta;
	dsta = get_dilong(2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.W -(An),(xxx).L */
uae_u32 REGPARAM2 op_33e0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uaecptr dsta;
	dsta = get_dilong(2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.W (d16,An),(xxx).L */
uae_u32 REGPARAM2 op_33e8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = get_dilong(4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(8);
	return 24 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* MOVE.W (d8,An,Xn),(xxx).L */
uae_u32 REGPARAM2 op_33f0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = get_dilong(4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(8);
	return 26 * CYCLE_UNIT / 2;
}
/* 8 8,0   */

/* MOVE.W (xxx).W,(xxx).L */
uae_u32 REGPARAM2 op_33f8_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = get_dilong(4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(8);
	return 24 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* MOVE.W (xxx).L,(xxx).L */
uae_u32 REGPARAM2 op_33f9_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = get_dilong(6);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(10);
	return 28 * CYCLE_UNIT / 2;
}
/* 10 0,0   */

/* MOVE.W (d16,PC),(xxx).L */
uae_u32 REGPARAM2 op_33fa_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = get_dilong(4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(8);
	return 24 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* MOVE.W (d8,PC,Xn),(xxx).L */
uae_u32 REGPARAM2 op_33fb_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uaecptr dsta;
	dsta = get_dilong(4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(8);
	return 26 * CYCLE_UNIT / 2;
}
/* 8 8,0   */

/* MOVE.W #<data>.W,(xxx).L */
uae_u32 REGPARAM2 op_33fc_44_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = get_dilong(4);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(8);
	return 20 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* NEGX.B Dn */
uae_u32 REGPARAM2 op_4000_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	uae_u32 newv = 0 - src - (GET_XFLG() ? 1 : 0);
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s8)(newv)) == 0));
	SET_NFLG(((uae_s8)(newv)) < 0);
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((newv) & 0xff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NEGX.B (An) */
uae_u32 REGPARAM2 op_4010_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	uae_u32 newv = 0 - src - (GET_XFLG() ? 1 : 0);
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s8)(newv)) == 0));
	SET_NFLG(((uae_s8)(newv)) < 0);
	put_byte_jit(srca, newv);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NEGX.B (An)+ */
uae_u32 REGPARAM2 op_4018_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	uae_u32 newv = 0 - src - (GET_XFLG() ? 1 : 0);
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s8)(newv)) == 0));
	SET_NFLG(((uae_s8)(newv)) < 0);
	put_byte_jit(srca, newv);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NEGX.B -(An) */
uae_u32 REGPARAM2 op_4020_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uae_u32 newv = 0 - src - (GET_XFLG() ? 1 : 0);
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s8)(newv)) == 0));
	SET_NFLG(((uae_s8)(newv)) < 0);
	put_byte_jit(srca, newv);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NEGX.B (d16,An) */
uae_u32 REGPARAM2 op_4028_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uae_u32 newv = 0 - src - (GET_XFLG() ? 1 : 0);
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s8)(newv)) == 0));
	SET_NFLG(((uae_s8)(newv)) < 0);
	put_byte_jit(srca, newv);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* NEGX.B (d8,An,Xn) */
uae_u32 REGPARAM2 op_4030_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	uae_u32 newv = 0 - src - (GET_XFLG() ? 1 : 0);
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s8)(newv)) == 0));
	SET_NFLG(((uae_s8)(newv)) < 0);
	put_byte_jit(srca, newv);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* NEGX.B (xxx).W */
uae_u32 REGPARAM2 op_4038_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uae_u32 newv = 0 - src - (GET_XFLG() ? 1 : 0);
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s8)(newv)) == 0));
	SET_NFLG(((uae_s8)(newv)) < 0);
	put_byte_jit(srca, newv);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* NEGX.B (xxx).L */
uae_u32 REGPARAM2 op_4039_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s8 src = get_byte_jit(srca);
	uae_u32 newv = 0 - src - (GET_XFLG() ? 1 : 0);
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s8)(newv)) == 0));
	SET_NFLG(((uae_s8)(newv)) < 0);
	put_byte_jit(srca, newv);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* NEGX.W Dn */
uae_u32 REGPARAM2 op_4040_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_u32 newv = 0 - src - (GET_XFLG() ? 1 : 0);
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s16)(newv)) == 0));
	SET_NFLG(((uae_s16)(newv)) < 0);
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | ((newv) & 0xffff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NEGX.W (An) */
uae_u32 REGPARAM2 op_4050_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	uae_u32 newv = 0 - src - (GET_XFLG() ? 1 : 0);
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s16)(newv)) == 0));
	SET_NFLG(((uae_s16)(newv)) < 0);
	put_word_jit(srca, newv);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NEGX.W (An)+ */
uae_u32 REGPARAM2 op_4058_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) += 2;
	uae_u32 newv = 0 - src - (GET_XFLG() ? 1 : 0);
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s16)(newv)) == 0));
	SET_NFLG(((uae_s16)(newv)) < 0);
	put_word_jit(srca, newv);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NEGX.W -(An) */
uae_u32 REGPARAM2 op_4060_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uae_u32 newv = 0 - src - (GET_XFLG() ? 1 : 0);
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s16)(newv)) == 0));
	SET_NFLG(((uae_s16)(newv)) < 0);
	put_word_jit(srca, newv);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NEGX.W (d16,An) */
uae_u32 REGPARAM2 op_4068_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_u32 newv = 0 - src - (GET_XFLG() ? 1 : 0);
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s16)(newv)) == 0));
	SET_NFLG(((uae_s16)(newv)) < 0);
	put_word_jit(srca, newv);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* NEGX.W (d8,An,Xn) */
uae_u32 REGPARAM2 op_4070_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uae_u32 newv = 0 - src - (GET_XFLG() ? 1 : 0);
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s16)(newv)) == 0));
	SET_NFLG(((uae_s16)(newv)) < 0);
	put_word_jit(srca, newv);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* NEGX.W (xxx).W */
uae_u32 REGPARAM2 op_4078_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_u32 newv = 0 - src - (GET_XFLG() ? 1 : 0);
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s16)(newv)) == 0));
	SET_NFLG(((uae_s16)(newv)) < 0);
	put_word_jit(srca, newv);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* NEGX.W (xxx).L */
uae_u32 REGPARAM2 op_4079_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s16 src = get_word_jit(srca);
	uae_u32 newv = 0 - src - (GET_XFLG() ? 1 : 0);
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s16)(newv)) == 0));
	SET_NFLG(((uae_s16)(newv)) < 0);
	put_word_jit(srca, newv);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* NEGX.L Dn */
uae_u32 REGPARAM2 op_4080_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_s32 src = m68k_dreg(regs, srcreg);
	uae_u32 newv = 0 - src - (GET_XFLG() ? 1 : 0);
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s32)(newv)) == 0));
	SET_NFLG(((uae_s32)(newv)) < 0);
	m68k_dreg(regs, srcreg) = (newv);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NEGX.L (An) */
uae_u32 REGPARAM2 op_4090_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	uae_u32 newv = 0 - src - (GET_XFLG() ? 1 : 0);
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s32)(newv)) == 0));
	SET_NFLG(((uae_s32)(newv)) < 0);
	put_long_jit(srca, newv);
	m68k_incpc(2);
	return 20 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NEGX.L (An)+ */
uae_u32 REGPARAM2 op_4098_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) += 4;
	uae_u32 newv = 0 - src - (GET_XFLG() ? 1 : 0);
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s32)(newv)) == 0));
	SET_NFLG(((uae_s32)(newv)) < 0);
	put_long_jit(srca, newv);
	m68k_incpc(2);
	return 20 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NEGX.L -(An) */
uae_u32 REGPARAM2 op_40a0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uae_u32 newv = 0 - src - (GET_XFLG() ? 1 : 0);
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s32)(newv)) == 0));
	SET_NFLG(((uae_s32)(newv)) < 0);
	put_long_jit(srca, newv);
	m68k_incpc(2);
	return 22 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NEGX.L (d16,An) */
uae_u32 REGPARAM2 op_40a8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_u32 newv = 0 - src - (GET_XFLG() ? 1 : 0);
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s32)(newv)) == 0));
	SET_NFLG(((uae_s32)(newv)) < 0);
	put_long_jit(srca, newv);
	m68k_incpc(4);
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* NEGX.L (d8,An,Xn) */
uae_u32 REGPARAM2 op_40b0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uae_u32 newv = 0 - src - (GET_XFLG() ? 1 : 0);
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s32)(newv)) == 0));
	SET_NFLG(((uae_s32)(newv)) < 0);
	put_long_jit(srca, newv);
	m68k_incpc(4);
	return 26 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* NEGX.L (xxx).W */
uae_u32 REGPARAM2 op_40b8_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_u32 newv = 0 - src - (GET_XFLG() ? 1 : 0);
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s32)(newv)) == 0));
	SET_NFLG(((uae_s32)(newv)) < 0);
	put_long_jit(srca, newv);
	m68k_incpc(4);
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* NEGX.L (xxx).L */
uae_u32 REGPARAM2 op_40b9_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s32 src = get_long_jit(srca);
	uae_u32 newv = 0 - src - (GET_XFLG() ? 1 : 0);
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s32)(newv)) == 0));
	SET_NFLG(((uae_s32)(newv)) < 0);
	put_long_jit(srca, newv);
	m68k_incpc(6);
	return 28 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MVSR2.W Dn */
uae_u32 REGPARAM2 op_40c0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	MakeSR();
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | ((regs.sr) & 0xffff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MVSR2.W (An) */
uae_u32 REGPARAM2 op_40d0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	MakeSR();
	put_word_jit(srca, regs.sr);
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MVSR2.W (An)+ */
uae_u32 REGPARAM2 op_40d8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += 2;
	MakeSR();
	put_word_jit(srca, regs.sr);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MVSR2.W -(An) */
uae_u32 REGPARAM2 op_40e0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	m68k_areg(regs, srcreg) = srca;
	MakeSR();
	put_word_jit(srca, regs.sr);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MVSR2.W (d16,An) */
uae_u32 REGPARAM2 op_40e8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	MakeSR();
	put_word_jit(srca, regs.sr);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MVSR2.W (d8,An,Xn) */
uae_u32 REGPARAM2 op_40f0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	MakeSR();
	put_word_jit(srca, regs.sr);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MVSR2.W (xxx).W */
uae_u32 REGPARAM2 op_40f8_44_ff(uae_u32 opcode)
{
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	MakeSR();
	put_word_jit(srca, regs.sr);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MVSR2.W (xxx).L */
uae_u32 REGPARAM2 op_40f9_44_ff(uae_u32 opcode)
{
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uaecptr srca;
	srca = get_dilong(2);
	MakeSR();
	put_word_jit(srca, regs.sr);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* CHK.W Dn,Dn */
uae_u32 REGPARAM2 op_4180_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc ();
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	m68k_incpc(2);
	if (dst > src) {
		setchkundefinedflags(src, dst, 1);
		Exception_cpu_oldpc(6,oldpc);
		return 8 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		setchkundefinedflags(src, dst, 1);
		Exception_cpu_oldpc(6,oldpc);
		return 10 * CYCLE_UNIT / 2;
	}
	setchkundefinedflags(src, dst, 1);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CHK.W (An),Dn */
uae_u32 REGPARAM2 op_4190_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc ();
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	m68k_incpc(2);
	if (dst > src) {
		setchkundefinedflags(src, dst, 1);
		Exception_cpu_oldpc(6,oldpc);
		return 12 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		setchkundefinedflags(src, dst, 1);
		Exception_cpu_oldpc(6,oldpc);
		return 14 * CYCLE_UNIT / 2;
	}
	setchkundefinedflags(src, dst, 1);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CHK.W (An)+,Dn */
uae_u32 REGPARAM2 op_4198_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc ();
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) += 2;
	uae_s16 dst = m68k_dreg(regs, dstreg);
	m68k_incpc(2);
	if (dst > src) {
		setchkundefinedflags(src, dst, 1);
		Exception_cpu_oldpc(6,oldpc);
		return 12 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		setchkundefinedflags(src, dst, 1);
		Exception_cpu_oldpc(6,oldpc);
		return 14 * CYCLE_UNIT / 2;
	}
	setchkundefinedflags(src, dst, 1);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CHK.W -(An),Dn */
uae_u32 REGPARAM2 op_41a0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc ();
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uae_s16 dst = m68k_dreg(regs, dstreg);
	m68k_incpc(2);
	if (dst > src) {
		setchkundefinedflags(src, dst, 1);
		Exception_cpu_oldpc(6,oldpc);
		return 14 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		setchkundefinedflags(src, dst, 1);
		Exception_cpu_oldpc(6,oldpc);
		return 16 * CYCLE_UNIT / 2;
	}
	setchkundefinedflags(src, dst, 1);
	return 16 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CHK.W (d16,An),Dn */
uae_u32 REGPARAM2 op_41a8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc ();
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	m68k_incpc(4);
	if (dst > src) {
		setchkundefinedflags(src, dst, 1);
		Exception_cpu_oldpc(6,oldpc);
		return 16 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		setchkundefinedflags(src, dst, 1);
		Exception_cpu_oldpc(6,oldpc);
		return 18 * CYCLE_UNIT / 2;
	}
	setchkundefinedflags(src, dst, 1);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* CHK.W (d8,An,Xn),Dn */
uae_u32 REGPARAM2 op_41b0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc ();
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	m68k_incpc(4);
	if (dst > src) {
		setchkundefinedflags(src, dst, 1);
		Exception_cpu_oldpc(6,oldpc);
		return 18 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		setchkundefinedflags(src, dst, 1);
		Exception_cpu_oldpc(6,oldpc);
		return 20 * CYCLE_UNIT / 2;
	}
	setchkundefinedflags(src, dst, 1);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* CHK.W (xxx).W,Dn */
uae_u32 REGPARAM2 op_41b8_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc ();
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	m68k_incpc(4);
	if (dst > src) {
		setchkundefinedflags(src, dst, 1);
		Exception_cpu_oldpc(6,oldpc);
		return 16 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		setchkundefinedflags(src, dst, 1);
		Exception_cpu_oldpc(6,oldpc);
		return 18 * CYCLE_UNIT / 2;
	}
	setchkundefinedflags(src, dst, 1);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* CHK.W (xxx).L,Dn */
uae_u32 REGPARAM2 op_41b9_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc ();
	uaecptr srca;
	srca = get_dilong(2);
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	m68k_incpc(6);
	if (dst > src) {
		setchkundefinedflags(src, dst, 1);
		Exception_cpu_oldpc(6,oldpc);
		return 20 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		setchkundefinedflags(src, dst, 1);
		Exception_cpu_oldpc(6,oldpc);
		return 22 * CYCLE_UNIT / 2;
	}
	setchkundefinedflags(src, dst, 1);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* CHK.W (d16,PC),Dn */
uae_u32 REGPARAM2 op_41ba_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc ();
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	m68k_incpc(4);
	if (dst > src) {
		setchkundefinedflags(src, dst, 1);
		Exception_cpu_oldpc(6,oldpc);
		return 16 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		setchkundefinedflags(src, dst, 1);
		Exception_cpu_oldpc(6,oldpc);
		return 18 * CYCLE_UNIT / 2;
	}
	setchkundefinedflags(src, dst, 1);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* CHK.W (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 op_41bb_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc ();
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	m68k_incpc(4);
	if (dst > src) {
		setchkundefinedflags(src, dst, 1);
		Exception_cpu_oldpc(6,oldpc);
		return 18 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		setchkundefinedflags(src, dst, 1);
		Exception_cpu_oldpc(6,oldpc);
		return 20 * CYCLE_UNIT / 2;
	}
	setchkundefinedflags(src, dst, 1);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* CHK.W #<data>.W,Dn */
uae_u32 REGPARAM2 op_41bc_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc ();
	uae_s16 src = get_diword (2);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	m68k_incpc(4);
	if (dst > src) {
		setchkundefinedflags(src, dst, 1);
		Exception_cpu_oldpc(6,oldpc);
		return 12 * CYCLE_UNIT / 2;
	}
	if ((uae_s32)dst < 0) {
		setchkundefinedflags(src, dst, 1);
		Exception_cpu_oldpc(6,oldpc);
		return 14 * CYCLE_UNIT / 2;
	}
	setchkundefinedflags(src, dst, 1);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* LEA.L (An),An */
uae_u32 REGPARAM2 op_41d0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, dstreg) = (srca);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* LEA.L (d16,An),An */
uae_u32 REGPARAM2 op_41e8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	m68k_areg(regs, dstreg) = (srca);
	m68k_incpc(4);
	return 8 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* LEA.L (d8,An,Xn),An */
uae_u32 REGPARAM2 op_41f0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	m68k_areg(regs, dstreg) = (srca);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* LEA.L (xxx).W,An */
uae_u32 REGPARAM2 op_41f8_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	m68k_areg(regs, dstreg) = (srca);
	m68k_incpc(4);
	return 8 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* LEA.L (xxx).L,An */
uae_u32 REGPARAM2 op_41f9_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	m68k_areg(regs, dstreg) = (srca);
	m68k_incpc(6);
	return 12 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* LEA.L (d16,PC),An */
uae_u32 REGPARAM2 op_41fa_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	m68k_areg(regs, dstreg) = (srca);
	m68k_incpc(4);
	return 8 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* LEA.L (d8,PC,Xn),An */
uae_u32 REGPARAM2 op_41fb_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	m68k_areg(regs, dstreg) = (srca);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* CLR.B Dn */
uae_u32 REGPARAM2 op_4200_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(0)) == 0);
	SET_NFLG(((uae_s8)(0)) < 0);
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((0) & 0xff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CLR.B (An) */
uae_u32 REGPARAM2 op_4210_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(0)) == 0);
	SET_NFLG(((uae_s8)(0)) < 0);
	put_byte_jit(srca, 0);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CLR.B (An)+ */
uae_u32 REGPARAM2 op_4218_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(0)) == 0);
	SET_NFLG(((uae_s8)(0)) < 0);
	put_byte_jit(srca, 0);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CLR.B -(An) */
uae_u32 REGPARAM2 op_4220_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg(regs, srcreg) = srca;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(0)) == 0);
	SET_NFLG(((uae_s8)(0)) < 0);
	put_byte_jit(srca, 0);
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CLR.B (d16,An) */
uae_u32 REGPARAM2 op_4228_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(0)) == 0);
	SET_NFLG(((uae_s8)(0)) < 0);
	put_byte_jit(srca, 0);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* CLR.B (d8,An,Xn) */
uae_u32 REGPARAM2 op_4230_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(0)) == 0);
	SET_NFLG(((uae_s8)(0)) < 0);
	put_byte_jit(srca, 0);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* CLR.B (xxx).W */
uae_u32 REGPARAM2 op_4238_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(0)) == 0);
	SET_NFLG(((uae_s8)(0)) < 0);
	put_byte_jit(srca, 0);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* CLR.B (xxx).L */
uae_u32 REGPARAM2 op_4239_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(0)) == 0);
	SET_NFLG(((uae_s8)(0)) < 0);
	put_byte_jit(srca, 0);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* CLR.W Dn */
uae_u32 REGPARAM2 op_4240_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(0)) == 0);
	SET_NFLG(((uae_s16)(0)) < 0);
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | ((0) & 0xffff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CLR.W (An) */
uae_u32 REGPARAM2 op_4250_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(0)) == 0);
	SET_NFLG(((uae_s16)(0)) < 0);
	put_word_jit(srca, 0);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CLR.W (An)+ */
uae_u32 REGPARAM2 op_4258_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += 2;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(0)) == 0);
	SET_NFLG(((uae_s16)(0)) < 0);
	put_word_jit(srca, 0);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CLR.W -(An) */
uae_u32 REGPARAM2 op_4260_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	m68k_areg(regs, srcreg) = srca;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(0)) == 0);
	SET_NFLG(((uae_s16)(0)) < 0);
	put_word_jit(srca, 0);
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CLR.W (d16,An) */
uae_u32 REGPARAM2 op_4268_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(0)) == 0);
	SET_NFLG(((uae_s16)(0)) < 0);
	put_word_jit(srca, 0);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* CLR.W (d8,An,Xn) */
uae_u32 REGPARAM2 op_4270_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(0)) == 0);
	SET_NFLG(((uae_s16)(0)) < 0);
	put_word_jit(srca, 0);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* CLR.W (xxx).W */
uae_u32 REGPARAM2 op_4278_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(0)) == 0);
	SET_NFLG(((uae_s16)(0)) < 0);
	put_word_jit(srca, 0);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* CLR.W (xxx).L */
uae_u32 REGPARAM2 op_4279_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(0)) == 0);
	SET_NFLG(((uae_s16)(0)) < 0);
	put_word_jit(srca, 0);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* CLR.L Dn */
uae_u32 REGPARAM2 op_4280_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(0)) == 0);
	SET_NFLG(((uae_s32)(0)) < 0);
	m68k_dreg(regs, srcreg) = (0);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CLR.L (An) */
uae_u32 REGPARAM2 op_4290_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(0)) == 0);
	SET_NFLG(((uae_s32)(0)) < 0);
	put_long_jit(srca, 0);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CLR.L (An)+ */
uae_u32 REGPARAM2 op_4298_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += 4;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(0)) == 0);
	SET_NFLG(((uae_s32)(0)) < 0);
	put_long_jit(srca, 0);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CLR.L -(An) */
uae_u32 REGPARAM2 op_42a0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
	m68k_areg(regs, srcreg) = srca;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(0)) == 0);
	SET_NFLG(((uae_s32)(0)) < 0);
	put_long_jit(srca, 0);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CLR.L (d16,An) */
uae_u32 REGPARAM2 op_42a8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(0)) == 0);
	SET_NFLG(((uae_s32)(0)) < 0);
	put_long_jit(srca, 0);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* CLR.L (d8,An,Xn) */
uae_u32 REGPARAM2 op_42b0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(0)) == 0);
	SET_NFLG(((uae_s32)(0)) < 0);
	put_long_jit(srca, 0);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* CLR.L (xxx).W */
uae_u32 REGPARAM2 op_42b8_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(0)) == 0);
	SET_NFLG(((uae_s32)(0)) < 0);
	put_long_jit(srca, 0);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* CLR.L (xxx).L */
uae_u32 REGPARAM2 op_42b9_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(0)) == 0);
	SET_NFLG(((uae_s32)(0)) < 0);
	put_long_jit(srca, 0);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MVSR2.B Dn */
uae_u32 REGPARAM2 op_42c0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	MakeSR();
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | ((regs.sr & 0xff) & 0xffff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MVSR2.B (An) */
uae_u32 REGPARAM2 op_42d0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	MakeSR();
	put_word_jit(srca, regs.sr & 0xff);
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MVSR2.B (An)+ */
uae_u32 REGPARAM2 op_42d8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += 2;
	MakeSR();
	put_word_jit(srca, regs.sr & 0xff);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MVSR2.B -(An) */
uae_u32 REGPARAM2 op_42e0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	m68k_areg(regs, srcreg) = srca;
	MakeSR();
	put_word_jit(srca, regs.sr & 0xff);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MVSR2.B (d16,An) */
uae_u32 REGPARAM2 op_42e8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	MakeSR();
	put_word_jit(srca, regs.sr & 0xff);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MVSR2.B (d8,An,Xn) */
uae_u32 REGPARAM2 op_42f0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	MakeSR();
	put_word_jit(srca, regs.sr & 0xff);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MVSR2.B (xxx).W */
uae_u32 REGPARAM2 op_42f8_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	MakeSR();
	put_word_jit(srca, regs.sr & 0xff);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MVSR2.B (xxx).L */
uae_u32 REGPARAM2 op_42f9_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	MakeSR();
	put_word_jit(srca, regs.sr & 0xff);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* NEG.B Dn */
uae_u32 REGPARAM2 op_4400_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	uae_u32 dst = ((uae_u8)(0)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(dst)) < 0;
	SET_ZFLG(((uae_s8)(dst)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(0)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((dst) & 0xff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NEG.B (An) */
uae_u32 REGPARAM2 op_4410_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	uae_u32 dst = ((uae_u8)(0)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(dst)) < 0;
	SET_ZFLG(((uae_s8)(dst)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(0)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(srca, dst);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NEG.B (An)+ */
uae_u32 REGPARAM2 op_4418_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	uae_u32 dst = ((uae_u8)(0)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(dst)) < 0;
	SET_ZFLG(((uae_s8)(dst)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(0)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(srca, dst);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NEG.B -(An) */
uae_u32 REGPARAM2 op_4420_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uae_u32 dst = ((uae_u8)(0)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(dst)) < 0;
	SET_ZFLG(((uae_s8)(dst)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(0)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(srca, dst);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NEG.B (d16,An) */
uae_u32 REGPARAM2 op_4428_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uae_u32 dst = ((uae_u8)(0)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(dst)) < 0;
	SET_ZFLG(((uae_s8)(dst)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(0)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(srca, dst);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* NEG.B (d8,An,Xn) */
uae_u32 REGPARAM2 op_4430_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	uae_u32 dst = ((uae_u8)(0)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(dst)) < 0;
	SET_ZFLG(((uae_s8)(dst)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(0)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(srca, dst);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* NEG.B (xxx).W */
uae_u32 REGPARAM2 op_4438_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uae_u32 dst = ((uae_u8)(0)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(dst)) < 0;
	SET_ZFLG(((uae_s8)(dst)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(0)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(srca, dst);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* NEG.B (xxx).L */
uae_u32 REGPARAM2 op_4439_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s8 src = get_byte_jit(srca);
	uae_u32 dst = ((uae_u8)(0)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(dst)) < 0;
	SET_ZFLG(((uae_s8)(dst)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(0)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(srca, dst);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* NEG.W Dn */
uae_u32 REGPARAM2 op_4440_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_u32 dst = ((uae_u16)(0)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(dst)) < 0;
	SET_ZFLG(((uae_s16)(dst)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(0)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | ((dst) & 0xffff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NEG.W (An) */
uae_u32 REGPARAM2 op_4450_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	uae_u32 dst = ((uae_u16)(0)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(dst)) < 0;
	SET_ZFLG(((uae_s16)(dst)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(0)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(srca, dst);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NEG.W (An)+ */
uae_u32 REGPARAM2 op_4458_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) += 2;
	uae_u32 dst = ((uae_u16)(0)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(dst)) < 0;
	SET_ZFLG(((uae_s16)(dst)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(0)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(srca, dst);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NEG.W -(An) */
uae_u32 REGPARAM2 op_4460_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uae_u32 dst = ((uae_u16)(0)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(dst)) < 0;
	SET_ZFLG(((uae_s16)(dst)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(0)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(srca, dst);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NEG.W (d16,An) */
uae_u32 REGPARAM2 op_4468_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_u32 dst = ((uae_u16)(0)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(dst)) < 0;
	SET_ZFLG(((uae_s16)(dst)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(0)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(srca, dst);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* NEG.W (d8,An,Xn) */
uae_u32 REGPARAM2 op_4470_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uae_u32 dst = ((uae_u16)(0)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(dst)) < 0;
	SET_ZFLG(((uae_s16)(dst)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(0)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(srca, dst);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* NEG.W (xxx).W */
uae_u32 REGPARAM2 op_4478_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_u32 dst = ((uae_u16)(0)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(dst)) < 0;
	SET_ZFLG(((uae_s16)(dst)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(0)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(srca, dst);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* NEG.W (xxx).L */
uae_u32 REGPARAM2 op_4479_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s16 src = get_word_jit(srca);
	uae_u32 dst = ((uae_u16)(0)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(dst)) < 0;
	SET_ZFLG(((uae_s16)(dst)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(0)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(srca, dst);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* NEG.L Dn */
uae_u32 REGPARAM2 op_4480_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_s32 src = m68k_dreg(regs, srcreg);
	uae_u32 dst = ((uae_u32)(0)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(dst)) < 0;
	SET_ZFLG(((uae_s32)(dst)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(0)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, srcreg) = (dst);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NEG.L (An) */
uae_u32 REGPARAM2 op_4490_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	uae_u32 dst = ((uae_u32)(0)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(dst)) < 0;
	SET_ZFLG(((uae_s32)(dst)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(0)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(srca, dst);
	m68k_incpc(2);
	return 20 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NEG.L (An)+ */
uae_u32 REGPARAM2 op_4498_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) += 4;
	uae_u32 dst = ((uae_u32)(0)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(dst)) < 0;
	SET_ZFLG(((uae_s32)(dst)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(0)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(srca, dst);
	m68k_incpc(2);
	return 20 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NEG.L -(An) */
uae_u32 REGPARAM2 op_44a0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uae_u32 dst = ((uae_u32)(0)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(dst)) < 0;
	SET_ZFLG(((uae_s32)(dst)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(0)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(srca, dst);
	m68k_incpc(2);
	return 22 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NEG.L (d16,An) */
uae_u32 REGPARAM2 op_44a8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_u32 dst = ((uae_u32)(0)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(dst)) < 0;
	SET_ZFLG(((uae_s32)(dst)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(0)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(srca, dst);
	m68k_incpc(4);
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* NEG.L (d8,An,Xn) */
uae_u32 REGPARAM2 op_44b0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uae_u32 dst = ((uae_u32)(0)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(dst)) < 0;
	SET_ZFLG(((uae_s32)(dst)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(0)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(srca, dst);
	m68k_incpc(4);
	return 26 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* NEG.L (xxx).W */
uae_u32 REGPARAM2 op_44b8_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_u32 dst = ((uae_u32)(0)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(dst)) < 0;
	SET_ZFLG(((uae_s32)(dst)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(0)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(srca, dst);
	m68k_incpc(4);
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* NEG.L (xxx).L */
uae_u32 REGPARAM2 op_44b9_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s32 src = get_long_jit(srca);
	uae_u32 dst = ((uae_u32)(0)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(dst)) < 0;
	SET_ZFLG(((uae_s32)(dst)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(0)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(srca, dst);
	m68k_incpc(6);
	return 28 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MV2SR.B Dn */
uae_u32 REGPARAM2 op_44c0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	MakeSR();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR();
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MV2SR.B (An) */
uae_u32 REGPARAM2 op_44d0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	MakeSR();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR();
	m68k_incpc(2);
	return 16 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MV2SR.B (An)+ */
uae_u32 REGPARAM2 op_44d8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) += 2;
	MakeSR();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR();
	m68k_incpc(2);
	return 16 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MV2SR.B -(An) */
uae_u32 REGPARAM2 op_44e0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	MakeSR();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR();
	m68k_incpc(2);
	return 18 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MV2SR.B (d16,An) */
uae_u32 REGPARAM2 op_44e8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	MakeSR();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR();
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MV2SR.B (d8,An,Xn) */
uae_u32 REGPARAM2 op_44f0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s16 src = get_word_jit(srca);
	MakeSR();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR();
	m68k_incpc(4);
	return 22 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MV2SR.B (xxx).W */
uae_u32 REGPARAM2 op_44f8_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	MakeSR();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR();
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MV2SR.B (xxx).L */
uae_u32 REGPARAM2 op_44f9_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s16 src = get_word_jit(srca);
	MakeSR();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR();
	m68k_incpc(6);
	return 24 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MV2SR.B (d16,PC) */
uae_u32 REGPARAM2 op_44fa_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	MakeSR();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR();
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MV2SR.B (d8,PC,Xn) */
uae_u32 REGPARAM2 op_44fb_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s16 src = get_word_jit(srca);
	MakeSR();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR();
	m68k_incpc(4);
	return 22 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MV2SR.B #<data>.B */
uae_u32 REGPARAM2 op_44fc_44_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	MakeSR();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR();
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* NOT.B Dn */
uae_u32 REGPARAM2 op_4600_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	uae_u32 dst = ~src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(dst)) == 0);
	SET_NFLG(((uae_s8)(dst)) < 0);
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((dst) & 0xff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NOT.B (An) */
uae_u32 REGPARAM2 op_4610_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	uae_u32 dst = ~src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(dst)) == 0);
	SET_NFLG(((uae_s8)(dst)) < 0);
	put_byte_jit(srca, dst);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NOT.B (An)+ */
uae_u32 REGPARAM2 op_4618_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	uae_u32 dst = ~src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(dst)) == 0);
	SET_NFLG(((uae_s8)(dst)) < 0);
	put_byte_jit(srca, dst);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NOT.B -(An) */
uae_u32 REGPARAM2 op_4620_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uae_u32 dst = ~src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(dst)) == 0);
	SET_NFLG(((uae_s8)(dst)) < 0);
	put_byte_jit(srca, dst);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NOT.B (d16,An) */
uae_u32 REGPARAM2 op_4628_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uae_u32 dst = ~src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(dst)) == 0);
	SET_NFLG(((uae_s8)(dst)) < 0);
	put_byte_jit(srca, dst);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* NOT.B (d8,An,Xn) */
uae_u32 REGPARAM2 op_4630_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	uae_u32 dst = ~src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(dst)) == 0);
	SET_NFLG(((uae_s8)(dst)) < 0);
	put_byte_jit(srca, dst);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* NOT.B (xxx).W */
uae_u32 REGPARAM2 op_4638_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uae_u32 dst = ~src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(dst)) == 0);
	SET_NFLG(((uae_s8)(dst)) < 0);
	put_byte_jit(srca, dst);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* NOT.B (xxx).L */
uae_u32 REGPARAM2 op_4639_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s8 src = get_byte_jit(srca);
	uae_u32 dst = ~src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(dst)) == 0);
	SET_NFLG(((uae_s8)(dst)) < 0);
	put_byte_jit(srca, dst);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* NOT.W Dn */
uae_u32 REGPARAM2 op_4640_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_u32 dst = ~src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(dst)) == 0);
	SET_NFLG(((uae_s16)(dst)) < 0);
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | ((dst) & 0xffff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NOT.W (An) */
uae_u32 REGPARAM2 op_4650_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	uae_u32 dst = ~src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(dst)) == 0);
	SET_NFLG(((uae_s16)(dst)) < 0);
	put_word_jit(srca, dst);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NOT.W (An)+ */
uae_u32 REGPARAM2 op_4658_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) += 2;
	uae_u32 dst = ~src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(dst)) == 0);
	SET_NFLG(((uae_s16)(dst)) < 0);
	put_word_jit(srca, dst);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NOT.W -(An) */
uae_u32 REGPARAM2 op_4660_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uae_u32 dst = ~src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(dst)) == 0);
	SET_NFLG(((uae_s16)(dst)) < 0);
	put_word_jit(srca, dst);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NOT.W (d16,An) */
uae_u32 REGPARAM2 op_4668_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_u32 dst = ~src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(dst)) == 0);
	SET_NFLG(((uae_s16)(dst)) < 0);
	put_word_jit(srca, dst);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* NOT.W (d8,An,Xn) */
uae_u32 REGPARAM2 op_4670_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uae_u32 dst = ~src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(dst)) == 0);
	SET_NFLG(((uae_s16)(dst)) < 0);
	put_word_jit(srca, dst);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* NOT.W (xxx).W */
uae_u32 REGPARAM2 op_4678_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_u32 dst = ~src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(dst)) == 0);
	SET_NFLG(((uae_s16)(dst)) < 0);
	put_word_jit(srca, dst);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* NOT.W (xxx).L */
uae_u32 REGPARAM2 op_4679_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s16 src = get_word_jit(srca);
	uae_u32 dst = ~src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(dst)) == 0);
	SET_NFLG(((uae_s16)(dst)) < 0);
	put_word_jit(srca, dst);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* NOT.L Dn */
uae_u32 REGPARAM2 op_4680_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_s32 src = m68k_dreg(regs, srcreg);
	uae_u32 dst = ~src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(dst)) == 0);
	SET_NFLG(((uae_s32)(dst)) < 0);
	m68k_dreg(regs, srcreg) = (dst);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NOT.L (An) */
uae_u32 REGPARAM2 op_4690_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	uae_u32 dst = ~src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(dst)) == 0);
	SET_NFLG(((uae_s32)(dst)) < 0);
	put_long_jit(srca, dst);
	m68k_incpc(2);
	return 20 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NOT.L (An)+ */
uae_u32 REGPARAM2 op_4698_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) += 4;
	uae_u32 dst = ~src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(dst)) == 0);
	SET_NFLG(((uae_s32)(dst)) < 0);
	put_long_jit(srca, dst);
	m68k_incpc(2);
	return 20 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NOT.L -(An) */
uae_u32 REGPARAM2 op_46a0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uae_u32 dst = ~src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(dst)) == 0);
	SET_NFLG(((uae_s32)(dst)) < 0);
	put_long_jit(srca, dst);
	m68k_incpc(2);
	return 22 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NOT.L (d16,An) */
uae_u32 REGPARAM2 op_46a8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_u32 dst = ~src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(dst)) == 0);
	SET_NFLG(((uae_s32)(dst)) < 0);
	put_long_jit(srca, dst);
	m68k_incpc(4);
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* NOT.L (d8,An,Xn) */
uae_u32 REGPARAM2 op_46b0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uae_u32 dst = ~src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(dst)) == 0);
	SET_NFLG(((uae_s32)(dst)) < 0);
	put_long_jit(srca, dst);
	m68k_incpc(4);
	return 26 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* NOT.L (xxx).W */
uae_u32 REGPARAM2 op_46b8_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_u32 dst = ~src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(dst)) == 0);
	SET_NFLG(((uae_s32)(dst)) < 0);
	put_long_jit(srca, dst);
	m68k_incpc(4);
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* NOT.L (xxx).L */
uae_u32 REGPARAM2 op_46b9_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s32 src = get_long_jit(srca);
	uae_u32 dst = ~src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(dst)) == 0);
	SET_NFLG(((uae_s32)(dst)) < 0);
	put_long_jit(srca, dst);
	m68k_incpc(6);
	return 28 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MV2SR.W Dn */
uae_u32 REGPARAM2 op_46c0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uae_s16 src = m68k_dreg(regs, srcreg);
	regs.sr = src;
	MakeFromSR_T0();
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MV2SR.W (An) */
uae_u32 REGPARAM2 op_46d0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	regs.sr = src;
	MakeFromSR_T0();
	m68k_incpc(2);
	return 16 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MV2SR.W (An)+ */
uae_u32 REGPARAM2 op_46d8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) += 2;
	regs.sr = src;
	MakeFromSR_T0();
	m68k_incpc(2);
	return 16 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MV2SR.W -(An) */
uae_u32 REGPARAM2 op_46e0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	regs.sr = src;
	MakeFromSR_T0();
	m68k_incpc(2);
	return 18 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MV2SR.W (d16,An) */
uae_u32 REGPARAM2 op_46e8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	regs.sr = src;
	MakeFromSR_T0();
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MV2SR.W (d8,An,Xn) */
uae_u32 REGPARAM2 op_46f0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s16 src = get_word_jit(srca);
	regs.sr = src;
	MakeFromSR_T0();
	m68k_incpc(4);
	return 22 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MV2SR.W (xxx).W */
uae_u32 REGPARAM2 op_46f8_44_ff(uae_u32 opcode)
{
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	regs.sr = src;
	MakeFromSR_T0();
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MV2SR.W (xxx).L */
uae_u32 REGPARAM2 op_46f9_44_ff(uae_u32 opcode)
{
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uaecptr srca;
	srca = get_dilong(2);
	uae_s16 src = get_word_jit(srca);
	regs.sr = src;
	MakeFromSR_T0();
	m68k_incpc(6);
	return 24 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MV2SR.W (d16,PC) */
uae_u32 REGPARAM2 op_46fa_44_ff(uae_u32 opcode)
{
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	regs.sr = src;
	MakeFromSR_T0();
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MV2SR.W (d8,PC,Xn) */
uae_u32 REGPARAM2 op_46fb_44_ff(uae_u32 opcode)
{
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s16 src = get_word_jit(srca);
	regs.sr = src;
	MakeFromSR_T0();
	m68k_incpc(4);
	return 22 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MV2SR.W #<data>.W */
uae_u32 REGPARAM2 op_46fc_44_ff(uae_u32 opcode)
{
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uae_s16 src = get_diword (2);
	regs.sr = src;
	MakeFromSR_T0();
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* NBCD.B Dn */
uae_u32 REGPARAM2 op_4800_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	uae_u16 newv_lo = - (src & 0xF) - (GET_XFLG() ? 1 : 0);
	uae_u16 newv_hi = - (src & 0xF0);
	uae_u16 newv;
	int cflg, tmp_newv;
	tmp_newv = newv_hi + newv_lo;
	if (newv_lo > 9) newv_lo -= 6;
	newv = newv_hi + newv_lo;
	cflg = (newv & 0x1F0) > 0x90;
	if (cflg) newv -= 0x60;
	SET_CFLG(cflg);
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s8)(newv)) == 0));
	SET_NFLG(((uae_s8)(newv)) < 0);
	SET_VFLG((tmp_newv & 0x80) != 0 && (newv & 0x80) == 0);
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((newv) & 0xff);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NBCD.B (An) */
uae_u32 REGPARAM2 op_4810_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	uae_u16 newv_lo = - (src & 0xF) - (GET_XFLG() ? 1 : 0);
	uae_u16 newv_hi = - (src & 0xF0);
	uae_u16 newv;
	int cflg, tmp_newv;
	tmp_newv = newv_hi + newv_lo;
	if (newv_lo > 9) newv_lo -= 6;
	newv = newv_hi + newv_lo;
	cflg = (newv & 0x1F0) > 0x90;
	if (cflg) newv -= 0x60;
	SET_CFLG(cflg);
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s8)(newv)) == 0));
	SET_NFLG(((uae_s8)(newv)) < 0);
	SET_VFLG((tmp_newv & 0x80) != 0 && (newv & 0x80) == 0);
	put_byte_jit(srca, newv);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NBCD.B (An)+ */
uae_u32 REGPARAM2 op_4818_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	uae_u16 newv_lo = - (src & 0xF) - (GET_XFLG() ? 1 : 0);
	uae_u16 newv_hi = - (src & 0xF0);
	uae_u16 newv;
	int cflg, tmp_newv;
	tmp_newv = newv_hi + newv_lo;
	if (newv_lo > 9) newv_lo -= 6;
	newv = newv_hi + newv_lo;
	cflg = (newv & 0x1F0) > 0x90;
	if (cflg) newv -= 0x60;
	SET_CFLG(cflg);
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s8)(newv)) == 0));
	SET_NFLG(((uae_s8)(newv)) < 0);
	SET_VFLG((tmp_newv & 0x80) != 0 && (newv & 0x80) == 0);
	put_byte_jit(srca, newv);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NBCD.B -(An) */
uae_u32 REGPARAM2 op_4820_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uae_u16 newv_lo = - (src & 0xF) - (GET_XFLG() ? 1 : 0);
	uae_u16 newv_hi = - (src & 0xF0);
	uae_u16 newv;
	int cflg, tmp_newv;
	tmp_newv = newv_hi + newv_lo;
	if (newv_lo > 9) newv_lo -= 6;
	newv = newv_hi + newv_lo;
	cflg = (newv & 0x1F0) > 0x90;
	if (cflg) newv -= 0x60;
	SET_CFLG(cflg);
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s8)(newv)) == 0));
	SET_NFLG(((uae_s8)(newv)) < 0);
	SET_VFLG((tmp_newv & 0x80) != 0 && (newv & 0x80) == 0);
	put_byte_jit(srca, newv);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NBCD.B (d16,An) */
uae_u32 REGPARAM2 op_4828_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uae_u16 newv_lo = - (src & 0xF) - (GET_XFLG() ? 1 : 0);
	uae_u16 newv_hi = - (src & 0xF0);
	uae_u16 newv;
	int cflg, tmp_newv;
	tmp_newv = newv_hi + newv_lo;
	if (newv_lo > 9) newv_lo -= 6;
	newv = newv_hi + newv_lo;
	cflg = (newv & 0x1F0) > 0x90;
	if (cflg) newv -= 0x60;
	SET_CFLG(cflg);
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s8)(newv)) == 0));
	SET_NFLG(((uae_s8)(newv)) < 0);
	SET_VFLG((tmp_newv & 0x80) != 0 && (newv & 0x80) == 0);
	put_byte_jit(srca, newv);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* NBCD.B (d8,An,Xn) */
uae_u32 REGPARAM2 op_4830_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	uae_u16 newv_lo = - (src & 0xF) - (GET_XFLG() ? 1 : 0);
	uae_u16 newv_hi = - (src & 0xF0);
	uae_u16 newv;
	int cflg, tmp_newv;
	tmp_newv = newv_hi + newv_lo;
	if (newv_lo > 9) newv_lo -= 6;
	newv = newv_hi + newv_lo;
	cflg = (newv & 0x1F0) > 0x90;
	if (cflg) newv -= 0x60;
	SET_CFLG(cflg);
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s8)(newv)) == 0));
	SET_NFLG(((uae_s8)(newv)) < 0);
	SET_VFLG((tmp_newv & 0x80) != 0 && (newv & 0x80) == 0);
	put_byte_jit(srca, newv);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* NBCD.B (xxx).W */
uae_u32 REGPARAM2 op_4838_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uae_u16 newv_lo = - (src & 0xF) - (GET_XFLG() ? 1 : 0);
	uae_u16 newv_hi = - (src & 0xF0);
	uae_u16 newv;
	int cflg, tmp_newv;
	tmp_newv = newv_hi + newv_lo;
	if (newv_lo > 9) newv_lo -= 6;
	newv = newv_hi + newv_lo;
	cflg = (newv & 0x1F0) > 0x90;
	if (cflg) newv -= 0x60;
	SET_CFLG(cflg);
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s8)(newv)) == 0));
	SET_NFLG(((uae_s8)(newv)) < 0);
	SET_VFLG((tmp_newv & 0x80) != 0 && (newv & 0x80) == 0);
	put_byte_jit(srca, newv);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* NBCD.B (xxx).L */
uae_u32 REGPARAM2 op_4839_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s8 src = get_byte_jit(srca);
	uae_u16 newv_lo = - (src & 0xF) - (GET_XFLG() ? 1 : 0);
	uae_u16 newv_hi = - (src & 0xF0);
	uae_u16 newv;
	int cflg, tmp_newv;
	tmp_newv = newv_hi + newv_lo;
	if (newv_lo > 9) newv_lo -= 6;
	newv = newv_hi + newv_lo;
	cflg = (newv & 0x1F0) > 0x90;
	if (cflg) newv -= 0x60;
	SET_CFLG(cflg);
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s8)(newv)) == 0));
	SET_NFLG(((uae_s8)(newv)) < 0);
	SET_VFLG((tmp_newv & 0x80) != 0 && (newv & 0x80) == 0);
	put_byte_jit(srca, newv);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* SWAP.W Dn */
uae_u32 REGPARAM2 op_4840_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_s32 src = m68k_dreg(regs, srcreg);
	uae_u32 dst = ((src >> 16)&0xFFFF) | ((src&0xFFFF)<<16);
	m68k_dreg(regs, srcreg) = (dst);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(dst)) == 0);
	SET_NFLG(((uae_s32)(dst)) < 0);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* BKPTQ.L #<data> */
uae_u32 REGPARAM2 op_4848_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	m68k_incpc(2);
	op_illg(opcode);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* PEA.L (An) */
uae_u32 REGPARAM2 op_4850_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u16 old_opcode = opcode;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, 7) - 4;
	m68k_areg(regs, 7) = dsta;
	if (dsta & 1) {
		regs.ir = old_opcode;
		m68k_incpc(2);
		exception3_write_access(old_opcode, dsta, sz_word, srca >> 16, 1);
		return 4 * CYCLE_UNIT / 2;
	}
	put_long_jit(dsta, srca);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* PEA.L (d16,An) */
uae_u32 REGPARAM2 op_4868_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u16 old_opcode = opcode;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, 7) - 4;
	m68k_areg(regs, 7) = dsta;
	if (dsta & 1) {
		regs.ir = old_opcode;
		m68k_incpc(2);
		exception3_write_access(old_opcode, dsta, sz_word, srca >> 16, 1);
		return 8 * CYCLE_UNIT / 2;
	}
	put_long_jit(dsta, srca);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* PEA.L (d8,An,Xn) */
uae_u32 REGPARAM2 op_4870_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u16 old_opcode = opcode;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uaecptr dsta;
	dsta = m68k_areg(regs, 7) - 4;
	m68k_areg(regs, 7) = dsta;
	if (dsta & 1) {
		regs.ir = old_opcode;
		m68k_incpc(2);
		exception3_write_access(old_opcode, dsta, sz_word, srca >> 16, 1);
		return 12 * CYCLE_UNIT / 2;
	}
	put_long_jit(dsta, srca);
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* PEA.L (xxx).W */
uae_u32 REGPARAM2 op_4878_44_ff(uae_u32 opcode)
{
	uae_u16 old_opcode = opcode;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, 7) - 4;
	m68k_areg(regs, 7) = dsta;
	if (dsta & 1) {
		regs.ir = old_opcode;
		m68k_incpc(2);
		exception3_write_access(old_opcode, dsta, sz_word, srca >> 16, 1);
		return 8 * CYCLE_UNIT / 2;
	}
	put_long_jit(dsta, srca);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* PEA.L (xxx).L */
uae_u32 REGPARAM2 op_4879_44_ff(uae_u32 opcode)
{
	uae_u16 old_opcode = opcode;
	uaecptr srca;
	srca = get_dilong(2);
	uaecptr dsta;
	dsta = m68k_areg(regs, 7) - 4;
	m68k_areg(regs, 7) = dsta;
	if (dsta & 1) {
		regs.ir = old_opcode;
		m68k_incpc(2);
		exception3_write_access(old_opcode, dsta, sz_word, srca >> 16, 1);
		return 12 * CYCLE_UNIT / 2;
	}
	put_long_jit(dsta, srca);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* PEA.L (d16,PC) */
uae_u32 REGPARAM2 op_487a_44_ff(uae_u32 opcode)
{
	uae_u16 old_opcode = opcode;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, 7) - 4;
	m68k_areg(regs, 7) = dsta;
	if (dsta & 1) {
		regs.ir = old_opcode;
		m68k_incpc(2);
		exception3_write_access(old_opcode, dsta, sz_word, srca >> 16, 1);
		return 8 * CYCLE_UNIT / 2;
	}
	put_long_jit(dsta, srca);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* PEA.L (d8,PC,Xn) */
uae_u32 REGPARAM2 op_487b_44_ff(uae_u32 opcode)
{
	uae_u16 old_opcode = opcode;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uaecptr dsta;
	dsta = m68k_areg(regs, 7) - 4;
	m68k_areg(regs, 7) = dsta;
	if (dsta & 1) {
		regs.ir = old_opcode;
		m68k_incpc(2);
		exception3_write_access(old_opcode, dsta, sz_word, srca >> 16, 1);
		return 12 * CYCLE_UNIT / 2;
	}
	put_long_jit(dsta, srca);
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* EXT.W Dn */
uae_u32 REGPARAM2 op_4880_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_s32 src = m68k_dreg(regs, srcreg);
	uae_u16 dst = (uae_s16)(uae_s8)src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(dst)) == 0);
	SET_NFLG(((uae_s16)(dst)) < 0);
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | ((dst) & 0xffff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MVMLE.W #<data>.W,(An) */
uae_u32 REGPARAM2 op_4890_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
	uae_u16 mask = get_diword (2);
	uaecptr srca;
	srca = m68k_areg(regs, dstreg);
	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) {
		put_word_jit(srca, m68k_dreg(regs, movem_index1[dmask]));
		count_cycles += 4 * CYCLE_UNIT / 2;
		srca += 2;
		dmask = movem_next[dmask];
	}
	while (amask) {
		put_word_jit(srca, m68k_areg(regs, movem_index1[amask]));
		count_cycles += 4 * CYCLE_UNIT / 2;
		srca += 2;
		amask = movem_next[amask];
	}
	m68k_incpc(4);
	return 8 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0   */

/* MVMLE.W #<data>.W,-(An) */
uae_u32 REGPARAM2 op_48a0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
	uae_u16 mask = get_diword (2);
	uaecptr srca;
	srca = m68k_areg(regs, dstreg) - 0;
	uae_u16 amask = mask & 0xff, dmask = (mask >> 8) & 0xff;
	while (amask) {
		srca -= 2;
		put_word_jit(srca, m68k_areg(regs, movem_index2[amask]));
		count_cycles += 4 * CYCLE_UNIT / 2;
		amask = movem_next[amask];
	}
	while (dmask) {
		srca -= 2;
		put_word_jit(srca, m68k_dreg(regs, movem_index2[dmask]));
		count_cycles += 4 * CYCLE_UNIT / 2;
		dmask = movem_next[dmask];
	}
	m68k_areg(regs, dstreg) = srca;
	m68k_incpc(4);
	return 8 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0   */

/* MVMLE.W #<data>.W,(d16,An) */
uae_u32 REGPARAM2 op_48a8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
	uae_u16 mask = get_diword (2);
	uaecptr srca;
	srca = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) {
		put_word_jit(srca, m68k_dreg(regs, movem_index1[dmask]));
		count_cycles += 4 * CYCLE_UNIT / 2;
		srca += 2;
		dmask = movem_next[dmask];
	}
	while (amask) {
		put_word_jit(srca, m68k_areg(regs, movem_index1[amask]));
		count_cycles += 4 * CYCLE_UNIT / 2;
		srca += 2;
		amask = movem_next[amask];
	}
	m68k_incpc(6);
	return 12 * CYCLE_UNIT / 2 + count_cycles;
}
/* 6 0,0   */

/* MVMLE.W #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 op_48b0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
	uae_u16 mask = get_diword (2);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) {
		put_word_jit(srca, m68k_dreg(regs, movem_index1[dmask]));
		count_cycles += 4 * CYCLE_UNIT / 2;
		srca += 2;
		dmask = movem_next[dmask];
	}
	while (amask) {
		put_word_jit(srca, m68k_areg(regs, movem_index1[amask]));
		count_cycles += 4 * CYCLE_UNIT / 2;
		srca += 2;
		amask = movem_next[amask];
	}
	m68k_incpc(6);
	return 14 * CYCLE_UNIT / 2 + count_cycles;
}
/* 6 4,0   */

/* MVMLE.W #<data>.W,(xxx).W */
uae_u32 REGPARAM2 op_48b8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u16 mask = get_diword (2);
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (4);
	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) {
		put_word_jit(srca, m68k_dreg(regs, movem_index1[dmask]));
		count_cycles += 4 * CYCLE_UNIT / 2;
		srca += 2;
		dmask = movem_next[dmask];
	}
	while (amask) {
		put_word_jit(srca, m68k_areg(regs, movem_index1[amask]));
		count_cycles += 4 * CYCLE_UNIT / 2;
		srca += 2;
		amask = movem_next[amask];
	}
	m68k_incpc(6);
	return 12 * CYCLE_UNIT / 2 + count_cycles;
}
/* 6 0,0   */

/* MVMLE.W #<data>.W,(xxx).L */
uae_u32 REGPARAM2 op_48b9_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u16 mask = get_diword (2);
	uaecptr srca;
	srca = get_dilong(4);
	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) {
		put_word_jit(srca, m68k_dreg(regs, movem_index1[dmask]));
		count_cycles += 4 * CYCLE_UNIT / 2;
		srca += 2;
		dmask = movem_next[dmask];
	}
	while (amask) {
		put_word_jit(srca, m68k_areg(regs, movem_index1[amask]));
		count_cycles += 4 * CYCLE_UNIT / 2;
		srca += 2;
		amask = movem_next[amask];
	}
	m68k_incpc(8);
	return 16 * CYCLE_UNIT / 2 + count_cycles;
}
/* 8 0,0   */

/* EXT.L Dn */
uae_u32 REGPARAM2 op_48c0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_s32 src = m68k_dreg(regs, srcreg);
	uae_u32 dst = (uae_s32)(uae_s16)src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(dst)) == 0);
	SET_NFLG(((uae_s32)(dst)) < 0);
	m68k_dreg(regs, srcreg) = (dst);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MVMLE.L #<data>.W,(An) */
uae_u32 REGPARAM2 op_48d0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
	uae_u16 mask = get_diword (2);
	uaecptr srca;
	srca = m68k_areg(regs, dstreg);
	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) {
		put_long_jit(srca, m68k_dreg(regs, movem_index1[dmask]));
		count_cycles += 8 * CYCLE_UNIT / 2;
		srca += 4;
		dmask = movem_next[dmask];
	}
	while (amask) {
		put_long_jit(srca, m68k_areg(regs, movem_index1[amask]));
		count_cycles += 8 * CYCLE_UNIT / 2;
		srca += 4;
		amask = movem_next[amask];
	}
	m68k_incpc(4);
	return 8 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0   */

/* MVMLE.L #<data>.W,-(An) */
uae_u32 REGPARAM2 op_48e0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
	uae_u16 mask = get_diword (2);
	uaecptr srca;
	srca = m68k_areg(regs, dstreg) - 0;
	uae_u16 amask = mask & 0xff, dmask = (mask >> 8) & 0xff;
	while (amask) {
		put_long_jit(srca - 4, m68k_areg(regs, movem_index2[amask]));
		count_cycles += 8 * CYCLE_UNIT / 2;
		srca -= 4;
		amask = movem_next[amask];
	}
	while (dmask) {
		put_long_jit(srca - 4, m68k_dreg(regs, movem_index2[dmask]));
		count_cycles += 8 * CYCLE_UNIT / 2;
		srca -= 4;
		dmask = movem_next[dmask];
	}
	m68k_areg(regs, dstreg) = srca;
	m68k_incpc(4);
	return 8 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0   */

/* MVMLE.L #<data>.W,(d16,An) */
uae_u32 REGPARAM2 op_48e8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
	uae_u16 mask = get_diword (2);
	uaecptr srca;
	srca = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) {
		put_long_jit(srca, m68k_dreg(regs, movem_index1[dmask]));
		count_cycles += 8 * CYCLE_UNIT / 2;
		srca += 4;
		dmask = movem_next[dmask];
	}
	while (amask) {
		put_long_jit(srca, m68k_areg(regs, movem_index1[amask]));
		count_cycles += 8 * CYCLE_UNIT / 2;
		srca += 4;
		amask = movem_next[amask];
	}
	m68k_incpc(6);
	return 12 * CYCLE_UNIT / 2 + count_cycles;
}
/* 6 0,0   */

/* MVMLE.L #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 op_48f0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
	uae_u16 mask = get_diword (2);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) {
		put_long_jit(srca, m68k_dreg(regs, movem_index1[dmask]));
		count_cycles += 8 * CYCLE_UNIT / 2;
		srca += 4;
		dmask = movem_next[dmask];
	}
	while (amask) {
		put_long_jit(srca, m68k_areg(regs, movem_index1[amask]));
		count_cycles += 8 * CYCLE_UNIT / 2;
		srca += 4;
		amask = movem_next[amask];
	}
	m68k_incpc(6);
	return 14 * CYCLE_UNIT / 2 + count_cycles;
}
/* 6 4,0   */

/* MVMLE.L #<data>.W,(xxx).W */
uae_u32 REGPARAM2 op_48f8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u16 mask = get_diword (2);
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (4);
	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) {
		put_long_jit(srca, m68k_dreg(regs, movem_index1[dmask]));
		count_cycles += 8 * CYCLE_UNIT / 2;
		srca += 4;
		dmask = movem_next[dmask];
	}
	while (amask) {
		put_long_jit(srca, m68k_areg(regs, movem_index1[amask]));
		count_cycles += 8 * CYCLE_UNIT / 2;
		srca += 4;
		amask = movem_next[amask];
	}
	m68k_incpc(6);
	return 12 * CYCLE_UNIT / 2 + count_cycles;
}
/* 6 0,0   */

/* MVMLE.L #<data>.W,(xxx).L */
uae_u32 REGPARAM2 op_48f9_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u16 mask = get_diword (2);
	uaecptr srca;
	srca = get_dilong(4);
	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) {
		put_long_jit(srca, m68k_dreg(regs, movem_index1[dmask]));
		count_cycles += 8 * CYCLE_UNIT / 2;
		srca += 4;
		dmask = movem_next[dmask];
	}
	while (amask) {
		put_long_jit(srca, m68k_areg(regs, movem_index1[amask]));
		count_cycles += 8 * CYCLE_UNIT / 2;
		srca += 4;
		amask = movem_next[amask];
	}
	m68k_incpc(8);
	return 16 * CYCLE_UNIT / 2 + count_cycles;
}
/* 8 0,0   */

/* TST.B Dn */
uae_u32 REGPARAM2 op_4a00_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* TST.B (An) */
uae_u32 REGPARAM2 op_4a10_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* TST.B (An)+ */
uae_u32 REGPARAM2 op_4a18_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* TST.B -(An) */
uae_u32 REGPARAM2 op_4a20_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* TST.B (d16,An) */
uae_u32 REGPARAM2 op_4a28_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* TST.B (d8,An,Xn) */
uae_u32 REGPARAM2 op_4a30_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* TST.B (xxx).W */
uae_u32 REGPARAM2 op_4a38_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* TST.B (xxx).L */
uae_u32 REGPARAM2 op_4a39_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s8 src = get_byte_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* TST.W Dn */
uae_u32 REGPARAM2 op_4a40_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* TST.W (An) */
uae_u32 REGPARAM2 op_4a50_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* TST.W (An)+ */
uae_u32 REGPARAM2 op_4a58_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) += 2;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* TST.W -(An) */
uae_u32 REGPARAM2 op_4a60_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* TST.W (d16,An) */
uae_u32 REGPARAM2 op_4a68_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* TST.W (d8,An,Xn) */
uae_u32 REGPARAM2 op_4a70_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s16 src = get_word_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* TST.W (xxx).W */
uae_u32 REGPARAM2 op_4a78_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* TST.W (xxx).L */
uae_u32 REGPARAM2 op_4a79_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s16 src = get_word_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* TST.L Dn */
uae_u32 REGPARAM2 op_4a80_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_s32 src = m68k_dreg(regs, srcreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* TST.L (An) */
uae_u32 REGPARAM2 op_4a90_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* TST.L (An)+ */
uae_u32 REGPARAM2 op_4a98_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) += 4;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* TST.L -(An) */
uae_u32 REGPARAM2 op_4aa0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* TST.L (d16,An) */
uae_u32 REGPARAM2 op_4aa8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* TST.L (d8,An,Xn) */
uae_u32 REGPARAM2 op_4ab0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s32 src = get_long_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* TST.L (xxx).W */
uae_u32 REGPARAM2 op_4ab8_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* TST.L (xxx).L */
uae_u32 REGPARAM2 op_4ab9_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s32 src = get_long_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* TAS.B Dn */
uae_u32 REGPARAM2 op_4ac0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	src |= 0x80;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* TAS.B (An) */
uae_u32 REGPARAM2 op_4ad0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_u8 src = get_byte_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	src |= 0x80;
	put_byte_jit(srca, src);
	m68k_incpc(2);
	return 16 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* TAS.B (An)+ */
uae_u32 REGPARAM2 op_4ad8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	uae_u8 src = get_byte_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	src |= 0x80;
	put_byte_jit(srca, src);
	m68k_incpc(2);
	return 18 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* TAS.B -(An) */
uae_u32 REGPARAM2 op_4ae0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg(regs, srcreg) = srca;
	uae_u8 src = get_byte_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	src |= 0x80;
	put_byte_jit(srca, src);
	m68k_incpc(2);
	return 18 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* TAS.B (d16,An) */
uae_u32 REGPARAM2 op_4ae8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_u8 src = get_byte_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	src |= 0x80;
	put_byte_jit(srca, src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* TAS.B (d8,An,Xn) */
uae_u32 REGPARAM2 op_4af0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_u8 src = get_byte_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	src |= 0x80;
	put_byte_jit(srca, src);
	m68k_incpc(4);
	return 22 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* TAS.B (xxx).W */
uae_u32 REGPARAM2 op_4af8_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_u8 src = get_byte_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	src |= 0x80;
	put_byte_jit(srca, src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* TAS.B (xxx).L */
uae_u32 REGPARAM2 op_4af9_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_u8 src = get_byte_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	src |= 0x80;
	put_byte_jit(srca, src);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MVMEL.W #<data>.W,(An) */
uae_u32 REGPARAM2 op_4c90_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	uaecptr srca;
	srca = m68k_areg(regs, dstreg);
	while (dmask) {
		m68k_dreg(regs, movem_index1[dmask]) = (uae_s32)(uae_s16)get_word_jit(srca);
		count_cycles += 4 * CYCLE_UNIT / 2;
		srca += 2;
		dmask = movem_next[dmask];
	}
	while (amask) {
		m68k_areg(regs, movem_index1[amask]) = (uae_s32)(uae_s16)get_word_jit(srca);
		count_cycles += 4 * CYCLE_UNIT / 2;
		srca += 2;
		amask = movem_next[amask];
	}
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0   */

/* MVMEL.W #<data>.W,(An)+ */
uae_u32 REGPARAM2 op_4c98_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	uaecptr srca;
	srca = m68k_areg(regs, dstreg);
	while (dmask) {
		m68k_dreg(regs, movem_index1[dmask]) = (uae_s32)(uae_s16)get_word_jit(srca);
		count_cycles += 4 * CYCLE_UNIT / 2;
		srca += 2;
		dmask = movem_next[dmask];
	}
	while (amask) {
		m68k_areg(regs, movem_index1[amask]) = (uae_s32)(uae_s16)get_word_jit(srca);
		count_cycles += 4 * CYCLE_UNIT / 2;
		srca += 2;
		amask = movem_next[amask];
	}
	m68k_areg(regs, dstreg) = srca;
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0   */

/* MVMEL.W #<data>.W,(d16,An) */
uae_u32 REGPARAM2 op_4ca8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	uaecptr srca;
	srca = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	while (dmask) {
		m68k_dreg(regs, movem_index1[dmask]) = (uae_s32)(uae_s16)get_word_jit(srca);
		count_cycles += 4 * CYCLE_UNIT / 2;
		srca += 2;
		dmask = movem_next[dmask];
	}
	while (amask) {
		m68k_areg(regs, movem_index1[amask]) = (uae_s32)(uae_s16)get_word_jit(srca);
		count_cycles += 4 * CYCLE_UNIT / 2;
		srca += 2;
		amask = movem_next[amask];
	}
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2 + count_cycles;
}
/* 6 0,0   */

/* MVMEL.W #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 op_4cb0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
	while (dmask) {
		m68k_dreg(regs, movem_index1[dmask]) = (uae_s32)(uae_s16)get_word_jit(srca);
		count_cycles += 4 * CYCLE_UNIT / 2;
		srca += 2;
		dmask = movem_next[dmask];
	}
	while (amask) {
		m68k_areg(regs, movem_index1[amask]) = (uae_s32)(uae_s16)get_word_jit(srca);
		count_cycles += 4 * CYCLE_UNIT / 2;
		srca += 2;
		amask = movem_next[amask];
	}
	m68k_incpc(6);
	return 18 * CYCLE_UNIT / 2 + count_cycles;
}
/* 6 4,0   */

/* MVMEL.W #<data>.W,(xxx).W */
uae_u32 REGPARAM2 op_4cb8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (4);
	while (dmask) {
		m68k_dreg(regs, movem_index1[dmask]) = (uae_s32)(uae_s16)get_word_jit(srca);
		count_cycles += 4 * CYCLE_UNIT / 2;
		srca += 2;
		dmask = movem_next[dmask];
	}
	while (amask) {
		m68k_areg(regs, movem_index1[amask]) = (uae_s32)(uae_s16)get_word_jit(srca);
		count_cycles += 4 * CYCLE_UNIT / 2;
		srca += 2;
		amask = movem_next[amask];
	}
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2 + count_cycles;
}
/* 6 0,0   */

/* MVMEL.W #<data>.W,(xxx).L */
uae_u32 REGPARAM2 op_4cb9_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	uaecptr srca;
	srca = get_dilong(4);
	while (dmask) {
		m68k_dreg(regs, movem_index1[dmask]) = (uae_s32)(uae_s16)get_word_jit(srca);
		count_cycles += 4 * CYCLE_UNIT / 2;
		srca += 2;
		dmask = movem_next[dmask];
	}
	while (amask) {
		m68k_areg(regs, movem_index1[amask]) = (uae_s32)(uae_s16)get_word_jit(srca);
		count_cycles += 4 * CYCLE_UNIT / 2;
		srca += 2;
		amask = movem_next[amask];
	}
	m68k_incpc(8);
	return 20 * CYCLE_UNIT / 2 + count_cycles;
}
/* 8 0,0   */

/* MVMEL.W #<data>.W,(d16,PC) */
uae_u32 REGPARAM2 op_4cba_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = 2;
	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	uaecptr srca;
	srca = m68k_getpc () + 4;
	srca += (uae_s32)(uae_s16)get_diword (4);
	while (dmask) {
		m68k_dreg(regs, movem_index1[dmask]) = (uae_s32)(uae_s16)get_word_jit(srca);
		count_cycles += 4 * CYCLE_UNIT / 2;
		srca += 2;
		dmask = movem_next[dmask];
	}
	while (amask) {
		m68k_areg(regs, movem_index1[amask]) = (uae_s32)(uae_s16)get_word_jit(srca);
		count_cycles += 4 * CYCLE_UNIT / 2;
		srca += 2;
		amask = movem_next[amask];
	}
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2 + count_cycles;
}
/* 6 0,0   */

/* MVMEL.W #<data>.W,(d8,PC,Xn) */
uae_u32 REGPARAM2 op_4cbb_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = 3;
	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 4;
	srca = get_disp_ea_000(tmppc, get_diword (4));
	while (dmask) {
		m68k_dreg(regs, movem_index1[dmask]) = (uae_s32)(uae_s16)get_word_jit(srca);
		count_cycles += 4 * CYCLE_UNIT / 2;
		srca += 2;
		dmask = movem_next[dmask];
	}
	while (amask) {
		m68k_areg(regs, movem_index1[amask]) = (uae_s32)(uae_s16)get_word_jit(srca);
		count_cycles += 4 * CYCLE_UNIT / 2;
		srca += 2;
		amask = movem_next[amask];
	}
	m68k_incpc(6);
	return 18 * CYCLE_UNIT / 2 + count_cycles;
}
/* 6 4,0   */

/* MVMEL.L #<data>.W,(An) */
uae_u32 REGPARAM2 op_4cd0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	uaecptr srca;
	srca = m68k_areg(regs, dstreg);
	while (dmask) {
		m68k_dreg(regs, movem_index1[dmask]) = get_long_jit(srca);
		count_cycles += 8 * CYCLE_UNIT / 2;
		srca += 4;
		dmask = movem_next[dmask];
	}
	while (amask) {
		m68k_areg(regs, movem_index1[amask]) = get_long_jit(srca);
		count_cycles += 8 * CYCLE_UNIT / 2;
		srca += 4;
		amask = movem_next[amask];
	}
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0   */

/* MVMEL.L #<data>.W,(An)+ */
uae_u32 REGPARAM2 op_4cd8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	uaecptr srca;
	srca = m68k_areg(regs, dstreg);
	while (dmask) {
		m68k_dreg(regs, movem_index1[dmask]) = get_long_jit(srca);
		count_cycles += 8 * CYCLE_UNIT / 2;
		srca += 4;
		dmask = movem_next[dmask];
	}
	while (amask) {
		m68k_areg(regs, movem_index1[amask]) = get_long_jit(srca);
		count_cycles += 8 * CYCLE_UNIT / 2;
		srca += 4;
		amask = movem_next[amask];
	}
	m68k_areg(regs, dstreg) = srca;
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0   */

/* MVMEL.L #<data>.W,(d16,An) */
uae_u32 REGPARAM2 op_4ce8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	uaecptr srca;
	srca = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	while (dmask) {
		m68k_dreg(regs, movem_index1[dmask]) = get_long_jit(srca);
		count_cycles += 8 * CYCLE_UNIT / 2;
		srca += 4;
		dmask = movem_next[dmask];
	}
	while (amask) {
		m68k_areg(regs, movem_index1[amask]) = get_long_jit(srca);
		count_cycles += 8 * CYCLE_UNIT / 2;
		srca += 4;
		amask = movem_next[amask];
	}
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2 + count_cycles;
}
/* 6 0,0   */

/* MVMEL.L #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 op_4cf0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = opcode & 7;
	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
	while (dmask) {
		m68k_dreg(regs, movem_index1[dmask]) = get_long_jit(srca);
		count_cycles += 8 * CYCLE_UNIT / 2;
		srca += 4;
		dmask = movem_next[dmask];
	}
	while (amask) {
		m68k_areg(regs, movem_index1[amask]) = get_long_jit(srca);
		count_cycles += 8 * CYCLE_UNIT / 2;
		srca += 4;
		amask = movem_next[amask];
	}
	m68k_incpc(6);
	return 18 * CYCLE_UNIT / 2 + count_cycles;
}
/* 6 4,0   */

/* MVMEL.L #<data>.W,(xxx).W */
uae_u32 REGPARAM2 op_4cf8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (4);
	while (dmask) {
		m68k_dreg(regs, movem_index1[dmask]) = get_long_jit(srca);
		count_cycles += 8 * CYCLE_UNIT / 2;
		srca += 4;
		dmask = movem_next[dmask];
	}
	while (amask) {
		m68k_areg(regs, movem_index1[amask]) = get_long_jit(srca);
		count_cycles += 8 * CYCLE_UNIT / 2;
		srca += 4;
		amask = movem_next[amask];
	}
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2 + count_cycles;
}
/* 6 0,0   */

/* MVMEL.L #<data>.W,(xxx).L */
uae_u32 REGPARAM2 op_4cf9_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	uaecptr srca;
	srca = get_dilong(4);
	while (dmask) {
		m68k_dreg(regs, movem_index1[dmask]) = get_long_jit(srca);
		count_cycles += 8 * CYCLE_UNIT / 2;
		srca += 4;
		dmask = movem_next[dmask];
	}
	while (amask) {
		m68k_areg(regs, movem_index1[amask]) = get_long_jit(srca);
		count_cycles += 8 * CYCLE_UNIT / 2;
		srca += 4;
		amask = movem_next[amask];
	}
	m68k_incpc(8);
	return 20 * CYCLE_UNIT / 2 + count_cycles;
}
/* 8 0,0   */

/* MVMEL.L #<data>.W,(d16,PC) */
uae_u32 REGPARAM2 op_4cfa_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = 2;
	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	uaecptr srca;
	srca = m68k_getpc () + 4;
	srca += (uae_s32)(uae_s16)get_diword (4);
	while (dmask) {
		m68k_dreg(regs, movem_index1[dmask]) = get_long_jit(srca);
		count_cycles += 8 * CYCLE_UNIT / 2;
		srca += 4;
		dmask = movem_next[dmask];
	}
	while (amask) {
		m68k_areg(regs, movem_index1[amask]) = get_long_jit(srca);
		count_cycles += 8 * CYCLE_UNIT / 2;
		srca += 4;
		amask = movem_next[amask];
	}
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2 + count_cycles;
}
/* 6 0,0   */

/* MVMEL.L #<data>.W,(d8,PC,Xn) */
uae_u32 REGPARAM2 op_4cfb_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = 3;
	uae_u16 mask = get_diword (2);
	uae_u32 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 4;
	srca = get_disp_ea_000(tmppc, get_diword (4));
	while (dmask) {
		m68k_dreg(regs, movem_index1[dmask]) = get_long_jit(srca);
		count_cycles += 8 * CYCLE_UNIT / 2;
		srca += 4;
		dmask = movem_next[dmask];
	}
	while (amask) {
		m68k_areg(regs, movem_index1[amask]) = get_long_jit(srca);
		count_cycles += 8 * CYCLE_UNIT / 2;
		srca += 4;
		amask = movem_next[amask];
	}
	m68k_incpc(6);
	return 18 * CYCLE_UNIT / 2 + count_cycles;
}
/* 6 4,0   */

/* TRAPQ.L #<data> */
uae_u32 REGPARAM2 op_4e40_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 15);
	uaecptr oldpc = m68k_getpc ();
	uae_u32 src = srcreg;
	m68k_incpc(2);
	Exception_cpu_oldpc(src + 32,oldpc);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* LINK.W An,#<data>.W */
uae_u32 REGPARAM2 op_4e50_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_s32 src = m68k_areg(regs, srcreg);
	uaecptr olda;
	olda = m68k_areg(regs, 7) - 4;
	m68k_areg(regs, 7) = olda;
	uae_s16 offs = get_diword (2);
	if (olda & 1) {
		m68k_areg(regs, 7) += 4;
		m68k_areg(regs, srcreg) = olda;
		m68k_incpc(6);
		exception3_write_access(opcode, olda, sz_word, src >> 16, 1);
		return 8 * CYCLE_UNIT / 2;
	}
	put_long_jit(olda, src);
	m68k_areg(regs, srcreg) = (m68k_areg (regs, 7));
	m68k_areg(regs, 7) += offs;
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* UNLK.L An */
uae_u32 REGPARAM2 op_4e58_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_s32 src = m68k_areg(regs, srcreg);
	uae_u32 olda = src;
	uae_s32 old = get_long_jit(olda);
	m68k_areg(regs, 7) = src + 4;
	m68k_areg(regs, srcreg) = (old);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MVR2USP.L An */
uae_u32 REGPARAM2 op_4e60_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uae_s32 src = m68k_areg(regs, srcreg);
	regs.usp = src;
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MVUSP2R.L An */
uae_u32 REGPARAM2 op_4e68_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	m68k_areg(regs, srcreg) = (regs.usp);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* RESET.L  */
uae_u32 REGPARAM2 op_4e70_44_ff(uae_u32 opcode)
{
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	cpureset();
	m68k_incpc(2);
	return 132 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* NOP.L  */
uae_u32 REGPARAM2 op_4e71_44_ff(uae_u32 opcode)
{
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* STOP.L #<data>.W */
uae_u32 REGPARAM2 op_4e72_44_ff(uae_u32 opcode)
{
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uae_s16 src = get_diword (2);
	uae_u16 sr = src;
	regs.sr = sr;
	MakeFromSR();
	m68k_setstopped();
	m68k_incpc(4);
	return 4 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* RTE.L  */
uae_u32 REGPARAM2 op_4e73_44_ff(uae_u32 opcode)
{
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	if (m68k_areg(regs, 7) & 1) {
		exception3_read_access(opcode, m68k_areg(regs, 7), 1, 1);
		return 4 * CYCLE_UNIT / 2;
	}
	uaecptr oldpc = m68k_getpc ();
	uae_u16 oldsr = regs.sr, newsr;
	uae_u32 newpc;
	for (;;) {
		uaecptr a = m68k_areg(regs, 7);
		uae_u16 sr = get_word_jit(a);
		uae_u32 pc = get_long_jit(a + 2);
		uae_u16 format = get_word_jit(a + 2 + 4);
		int frame = format >> 12;
		int offset = 8;
		newsr = sr; newpc = pc;
		if (frame == 0x0) {
		m68k_areg(regs, 7) += offset; break; }
		else if (frame == 0x8) {
		m68k_areg(regs, 7) += offset + 50; break; }
		else {
			SET_NFLG(((uae_s16)format) < 0); 
			SET_ZFLG(format == 0);
			SET_VFLG(0);
			Exception_cpu_oldpc(14,oldpc);
			return 20 * CYCLE_UNIT / 2;
		}
		regs.sr = newsr;
		oldsr = newsr;
		MakeFromSR_T0();
	}
	regs.sr = newsr;
	MakeFromSR_T0();
	if (newpc & 1) {
		exception3_read_prefetch(opcode, newpc);
		return 20 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(newpc);
	return 24 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* RTD.L #<data>.W */
uae_u32 REGPARAM2 op_4e74_44_ff(uae_u32 opcode)
{
	uaecptr pca;
	pca = m68k_areg(regs, 7);
	uae_s32 pc = get_long_jit(pca);
	m68k_areg(regs, 7) += 4;
	uae_s16 offs = get_diword (2);
	m68k_areg(regs, 7) += offs;
	if (pc & 1) {
		m68k_incpc(2);
		exception3_read_prefetch_only(opcode, pc);
		return 12 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(pc);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* RTS.L  */
uae_u32 REGPARAM2 op_4e75_44_ff(uae_u32 opcode)
{
	uaecptr oldpc = m68k_getpc ();
	if (m68k_areg(regs, 7) & 1) {
		m68k_incpc(2);
		exception3_read_access(opcode, m68k_areg(regs, 7), 1, 1);
		return 4 * CYCLE_UNIT / 2;
	}
	m68k_do_rts();
	if (m68k_getpc () & 1) {
		uaecptr faultpc = m68k_getpc ();
		m68k_setpc_j(oldpc);
		m68k_incpc(2);
		exception3_read_prefetch_only(opcode, faultpc);
		return 12 * CYCLE_UNIT / 2;
	}
	return 16 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* TRAPV.L  */
uae_u32 REGPARAM2 op_4e76_44_ff(uae_u32 opcode)
{
	uaecptr oldpc = m68k_getpc ();
	m68k_incpc(2);
	if (GET_VFLG()) {
		Exception_cpu_oldpc(7,oldpc);
		return 6 * CYCLE_UNIT / 2;
	}
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* RTR.L  */
uae_u32 REGPARAM2 op_4e77_44_ff(uae_u32 opcode)
{
	if (m68k_areg(regs, 7) & 1) {
		m68k_incpc(2);
		exception3_read_access(opcode, m68k_areg(regs, 7), 1, 1);
		return 4 * CYCLE_UNIT / 2;
	}
	uaecptr oldpc = m68k_getpc ();
	MakeSR();
	uaecptr sra;
	sra = m68k_areg(regs, 7);
	uae_s16 sr = get_word_jit(sra);
	m68k_areg(regs, 7) += 2;
	uaecptr pca;
	pca = m68k_areg(regs, 7);
	uae_s32 pc = get_long_jit(pca);
	m68k_areg(regs, 7) += 4;
	regs.sr &= 0xFF00; sr &= 0xFF;
	regs.sr |= sr;
	MakeFromSR();
	m68k_setpc_j(pc);
	if (m68k_getpc () & 1) {
		uaecptr faultpc = m68k_getpc ();
		m68k_setpc_j(oldpc + 2);
		exception3_read_prefetch_only(opcode, faultpc);
		return 16 * CYCLE_UNIT / 2;
	}
	return 20 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* MOVEC2.L #<data>.W */
uae_u32 REGPARAM2 op_4e7a_44_ff(uae_u32 opcode)
{
	if(!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uae_s16 src = get_diword (2);
	int regno = (src >> 12) & 15;
	uae_u32 *regp = regs.regs + regno;
	if (!m68k_movec2(src & 0xFFF, regp)) {
		return 8 * CYCLE_UNIT / 2;
	}
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MOVE2C.L #<data>.W */
uae_u32 REGPARAM2 op_4e7b_44_ff(uae_u32 opcode)
{
	if(!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uae_s16 src = get_diword (2);
	int regno = (src >> 12) & 15;
	uae_u32 *regp = regs.regs + regno;
	if (!m68k_move2c(src & 0xFFF, regp)) {
		return 8 * CYCLE_UNIT / 2;
	}
	m68k_incpc(4);
	return 10 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* JSR.L (An) */
uae_u32 REGPARAM2 op_4e90_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uaecptr oldpc = m68k_getpc ();
	uaecptr nextpc = oldpc + 2;
	if (srca & 1) {
		m68k_incpc(2);
		exception3_read_prefetch_only(opcode, srca);
		return 4 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(srca);
	m68k_areg(regs, 7) -= 4;
	if (m68k_areg(regs, 7) & 1) {
		m68k_setpc_j(oldpc);
		m68k_incpc(2);
		exception3_write_access(opcode, m68k_areg(regs, 7), 1, oldpc >> 16, 1);
		return 4 * CYCLE_UNIT / 2;
	}
	put_long_jit (m68k_areg(regs, 7), nextpc);
	return 16 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* JSR.L (d16,An) */
uae_u32 REGPARAM2 op_4ea8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	uaecptr nextpc = oldpc + 4;
	if (srca & 1) {
		m68k_incpc(2);
		exception3_read_prefetch_only(opcode, srca);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(srca);
	m68k_areg(regs, 7) -= 4;
	if (m68k_areg(regs, 7) & 1) {
		m68k_setpc_j(oldpc);
		m68k_incpc(4);
		exception3_write_access(opcode, m68k_areg(regs, 7), 1, oldpc >> 16, 1);
		return 6 * CYCLE_UNIT / 2;
	}
	put_long_jit (m68k_areg(regs, 7), nextpc);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* JSR.L (d8,An,Xn) */
uae_u32 REGPARAM2 op_4eb0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uaecptr oldpc = m68k_getpc ();
	uaecptr nextpc = oldpc + 4;
	if (srca & 1) {
		m68k_incpc(2);
		exception3_read_prefetch_only(opcode, srca);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(srca);
	m68k_areg(regs, 7) -= 4;
	if (m68k_areg(regs, 7) & 1) {
		m68k_setpc_j(oldpc);
		m68k_incpc(4);
		exception3_write_access(opcode, m68k_areg(regs, 7), 1, oldpc >> 16, 1);
		return 10 * CYCLE_UNIT / 2;
	}
	put_long_jit (m68k_areg(regs, 7), nextpc);
	return 22 * CYCLE_UNIT / 2;
}
/* 4 4,0 B */

/* JSR.L (xxx).W */
uae_u32 REGPARAM2 op_4eb8_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	uaecptr nextpc = oldpc + 4;
	if (srca & 1) {
		m68k_incpc(4);
		exception3_read_prefetch_only(opcode, srca);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(srca);
	m68k_areg(regs, 7) -= 4;
	if (m68k_areg(regs, 7) & 1) {
		m68k_setpc_j(oldpc);
		m68k_incpc(4);
		exception3_write_access(opcode, m68k_areg(regs, 7), 1, oldpc >> 16, 1);
		return 6 * CYCLE_UNIT / 2;
	}
	put_long_jit (m68k_areg(regs, 7), nextpc);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* JSR.L (xxx).L */
uae_u32 REGPARAM2 op_4eb9_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uaecptr oldpc = m68k_getpc ();
	uaecptr nextpc = oldpc + 6;
	if (srca & 1) {
		m68k_incpc(2);
		exception3_read_prefetch_only(opcode, srca);
		return 8 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(srca);
	m68k_areg(regs, 7) -= 4;
	if (m68k_areg(regs, 7) & 1) {
		m68k_setpc_j(oldpc);
		m68k_incpc(6);
		exception3_write_access(opcode, m68k_areg(regs, 7), 1, oldpc >> 16, 1);
		return 8 * CYCLE_UNIT / 2;
	}
	put_long_jit (m68k_areg(regs, 7), nextpc);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0 B */

/* JSR.L (d16,PC) */
uae_u32 REGPARAM2 op_4eba_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	uaecptr nextpc = oldpc + 4;
	if (srca & 1) {
		m68k_incpc(2);
		exception3_read_prefetch_only(opcode, srca);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(srca);
	m68k_areg(regs, 7) -= 4;
	if (m68k_areg(regs, 7) & 1) {
		m68k_setpc_j(oldpc);
		m68k_incpc(4);
		exception3_write_access(opcode, m68k_areg(regs, 7), 1, oldpc >> 16, 1);
		return 6 * CYCLE_UNIT / 2;
	}
	put_long_jit (m68k_areg(regs, 7), nextpc);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* JSR.L (d8,PC,Xn) */
uae_u32 REGPARAM2 op_4ebb_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uaecptr oldpc = m68k_getpc ();
	uaecptr nextpc = oldpc + 4;
	if (srca & 1) {
		m68k_incpc(2);
		exception3_read_prefetch_only(opcode, srca);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(srca);
	m68k_areg(regs, 7) -= 4;
	if (m68k_areg(regs, 7) & 1) {
		m68k_setpc_j(oldpc);
		m68k_incpc(4);
		exception3_write_access(opcode, m68k_areg(regs, 7), 1, oldpc >> 16, 1);
		return 10 * CYCLE_UNIT / 2;
	}
	put_long_jit (m68k_areg(regs, 7), nextpc);
	return 22 * CYCLE_UNIT / 2;
}
/* 4 4,0 B */

/* JMP.L (An) */
uae_u32 REGPARAM2 op_4ed0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	if (srca & 1) {
		m68k_incpc(2);
		exception3_read_prefetch_only(opcode, srca);
		return 4 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(srca);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* JMP.L (d16,An) */
uae_u32 REGPARAM2 op_4ee8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	if (srca & 1) {
		m68k_incpc(2);
		exception3_read_prefetch_only(opcode, srca);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(srca);
	return 10 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* JMP.L (d8,An,Xn) */
uae_u32 REGPARAM2 op_4ef0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	if (srca & 1) {
		m68k_incpc(2);
		exception3_read_prefetch_only(opcode, srca);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(srca);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 4,0 B */

/* JMP.L (xxx).W */
uae_u32 REGPARAM2 op_4ef8_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	if (srca & 1) {
		m68k_incpc(2);
		exception3_read_prefetch_only(opcode, srca);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(srca);
	return 10 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* JMP.L (xxx).L */
uae_u32 REGPARAM2 op_4ef9_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	if (srca & 1) {
		m68k_incpc(2);
		exception3_read_prefetch_only(opcode, srca);
		return 8 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(srca);
	return 12 * CYCLE_UNIT / 2;
}
/* 6 0,0 B */

/* JMP.L (d16,PC) */
uae_u32 REGPARAM2 op_4efa_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	if (srca & 1) {
		m68k_incpc(2);
		exception3_read_prefetch_only(opcode, srca);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(srca);
	return 10 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* JMP.L (d8,PC,Xn) */
uae_u32 REGPARAM2 op_4efb_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	if (srca & 1) {
		m68k_incpc(2);
		exception3_read_prefetch_only(opcode, srca);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(srca);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 4,0 B */

/* ADDQ.B #<data>,Dn */
uae_u32 REGPARAM2 op_5000_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADDQ.B #<data>,(An) */
uae_u32 REGPARAM2 op_5010_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADDQ.B #<data>,(An)+ */
uae_u32 REGPARAM2 op_5018_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADDQ.B #<data>,-(An) */
uae_u32 REGPARAM2 op_5020_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADDQ.B #<data>,(d16,An) */
uae_u32 REGPARAM2 op_5028_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 dst = get_byte_jit(dsta);
	uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADDQ.B #<data>,(d8,An,Xn) */
uae_u32 REGPARAM2 op_5030_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (2));
	uae_s8 dst = get_byte_jit(dsta);
	uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* ADDQ.B #<data>,(xxx).W */
uae_u32 REGPARAM2 op_5038_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 dst = get_byte_jit(dsta);
	uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADDQ.B #<data>,(xxx).L */
uae_u32 REGPARAM2 op_5039_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = get_dilong(2);
	uae_s8 dst = get_byte_jit(dsta);
	uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* ADDQ.W #<data>,Dn */
uae_u32 REGPARAM2 op_5040_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADDAQ.W #<data>,An */
uae_u32 REGPARAM2 op_5048_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADDQ.W #<data>,(An) */
uae_u32 REGPARAM2 op_5050_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s16 dst = get_word_jit(dsta);
	uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADDQ.W #<data>,(An)+ */
uae_u32 REGPARAM2 op_5058_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s16 dst = get_word_jit(dsta);
	m68k_areg(regs, dstreg) += 2;
	uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADDQ.W #<data>,-(An) */
uae_u32 REGPARAM2 op_5060_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	uae_s16 dst = get_word_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADDQ.W #<data>,(d16,An) */
uae_u32 REGPARAM2 op_5068_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 dst = get_word_jit(dsta);
	uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADDQ.W #<data>,(d8,An,Xn) */
uae_u32 REGPARAM2 op_5070_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (2));
	uae_s16 dst = get_word_jit(dsta);
	uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* ADDQ.W #<data>,(xxx).W */
uae_u32 REGPARAM2 op_5078_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 dst = get_word_jit(dsta);
	uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADDQ.W #<data>,(xxx).L */
uae_u32 REGPARAM2 op_5079_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = get_dilong(2);
	uae_s16 dst = get_word_jit(dsta);
	uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* ADDQ.L #<data>,Dn */
uae_u32 REGPARAM2 op_5080_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADDAQ.L #<data>,An */
uae_u32 REGPARAM2 op_5088_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADDQ.L #<data>,(An) */
uae_u32 REGPARAM2 op_5090_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s32 dst = get_long_jit(dsta);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(2);
	return 20 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADDQ.L #<data>,(An)+ */
uae_u32 REGPARAM2 op_5098_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s32 dst = get_long_jit(dsta);
	m68k_areg(regs, dstreg) += 4;
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(2);
	return 20 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADDQ.L #<data>,-(An) */
uae_u32 REGPARAM2 op_50a0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	uae_s32 dst = get_long_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(2);
	return 22 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADDQ.L #<data>,(d16,An) */
uae_u32 REGPARAM2 op_50a8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s32 dst = get_long_jit(dsta);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(4);
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADDQ.L #<data>,(d8,An,Xn) */
uae_u32 REGPARAM2 op_50b0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (2));
	uae_s32 dst = get_long_jit(dsta);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(4);
	return 26 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* ADDQ.L #<data>,(xxx).W */
uae_u32 REGPARAM2 op_50b8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	uae_s32 dst = get_long_jit(dsta);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(4);
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADDQ.L #<data>,(xxx).L */
uae_u32 REGPARAM2 op_50b9_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = get_dilong(2);
	uae_s32 dst = get_long_jit(dsta);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(6);
	return 28 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* Scc.B Dn (T) */
uae_u32 REGPARAM2 op_50c0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	int val = cctrue(0) ? 0xff : 0x00;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* DBcc.W Dn,#<data>.W (T) */
uae_u32 REGPARAM2 op_50c8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue(0)) {
		m68k_incpc((uae_s32)offs + 2);
		if (offs & 1) {
			exception3_read_prefetch(opcode, m68k_getpc ());
			return 6 * CYCLE_UNIT / 2 + count_cycles;
		}
		if (src) {
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 6 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(oldpc + 4);
	if (!cctrue(0)) {
		m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
	}
	return 10 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0 B */

/* Scc.B (An) (T) */
uae_u32 REGPARAM2 op_50d0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	int val = cctrue(0) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (An)+ (T) */
uae_u32 REGPARAM2 op_50d8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	int val = cctrue(0) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B -(An) (T) */
uae_u32 REGPARAM2 op_50e0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg(regs, srcreg) = srca;
	int val = cctrue(0) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (d16,An) (T) */
uae_u32 REGPARAM2 op_50e8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	int val = cctrue(0) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (d8,An,Xn) (T) */
uae_u32 REGPARAM2 op_50f0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	int val = cctrue(0) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* Scc.B (xxx).W (T) */
uae_u32 REGPARAM2 op_50f8_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	int val = cctrue(0) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (xxx).L (T) */
uae_u32 REGPARAM2 op_50f9_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	int val = cctrue(0) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* SUBQ.B #<data>,Dn */
uae_u32 REGPARAM2 op_5100_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUBQ.B #<data>,(An) */
uae_u32 REGPARAM2 op_5110_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUBQ.B #<data>,(An)+ */
uae_u32 REGPARAM2 op_5118_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUBQ.B #<data>,-(An) */
uae_u32 REGPARAM2 op_5120_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUBQ.B #<data>,(d16,An) */
uae_u32 REGPARAM2 op_5128_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 dst = get_byte_jit(dsta);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUBQ.B #<data>,(d8,An,Xn) */
uae_u32 REGPARAM2 op_5130_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (2));
	uae_s8 dst = get_byte_jit(dsta);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* SUBQ.B #<data>,(xxx).W */
uae_u32 REGPARAM2 op_5138_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 dst = get_byte_jit(dsta);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUBQ.B #<data>,(xxx).L */
uae_u32 REGPARAM2 op_5139_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = get_dilong(2);
	uae_s8 dst = get_byte_jit(dsta);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* SUBQ.W #<data>,Dn */
uae_u32 REGPARAM2 op_5140_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUBAQ.W #<data>,An */
uae_u32 REGPARAM2 op_5148_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUBQ.W #<data>,(An) */
uae_u32 REGPARAM2 op_5150_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s16 dst = get_word_jit(dsta);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUBQ.W #<data>,(An)+ */
uae_u32 REGPARAM2 op_5158_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s16 dst = get_word_jit(dsta);
	m68k_areg(regs, dstreg) += 2;
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUBQ.W #<data>,-(An) */
uae_u32 REGPARAM2 op_5160_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	uae_s16 dst = get_word_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUBQ.W #<data>,(d16,An) */
uae_u32 REGPARAM2 op_5168_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 dst = get_word_jit(dsta);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUBQ.W #<data>,(d8,An,Xn) */
uae_u32 REGPARAM2 op_5170_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (2));
	uae_s16 dst = get_word_jit(dsta);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* SUBQ.W #<data>,(xxx).W */
uae_u32 REGPARAM2 op_5178_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 dst = get_word_jit(dsta);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUBQ.W #<data>,(xxx).L */
uae_u32 REGPARAM2 op_5179_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = get_dilong(2);
	uae_s16 dst = get_word_jit(dsta);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* SUBQ.L #<data>,Dn */
uae_u32 REGPARAM2 op_5180_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUBAQ.L #<data>,An */
uae_u32 REGPARAM2 op_5188_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUBQ.L #<data>,(An) */
uae_u32 REGPARAM2 op_5190_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s32 dst = get_long_jit(dsta);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(2);
	return 20 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUBQ.L #<data>,(An)+ */
uae_u32 REGPARAM2 op_5198_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s32 dst = get_long_jit(dsta);
	m68k_areg(regs, dstreg) += 4;
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(2);
	return 20 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUBQ.L #<data>,-(An) */
uae_u32 REGPARAM2 op_51a0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	uae_s32 dst = get_long_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(2);
	return 22 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUBQ.L #<data>,(d16,An) */
uae_u32 REGPARAM2 op_51a8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s32 dst = get_long_jit(dsta);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(4);
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUBQ.L #<data>,(d8,An,Xn) */
uae_u32 REGPARAM2 op_51b0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (2));
	uae_s32 dst = get_long_jit(dsta);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(4);
	return 26 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* SUBQ.L #<data>,(xxx).W */
uae_u32 REGPARAM2 op_51b8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	uae_s32 dst = get_long_jit(dsta);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(4);
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUBQ.L #<data>,(xxx).L */
uae_u32 REGPARAM2 op_51b9_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 src = srcreg;
	uaecptr dsta;
	dsta = get_dilong(2);
	uae_s32 dst = get_long_jit(dsta);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(6);
	return 28 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* Scc.B Dn (F) */
uae_u32 REGPARAM2 op_51c0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	int val = cctrue(1) ? 0xff : 0x00;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* DBcc.W Dn,#<data>.W (F) */
uae_u32 REGPARAM2 op_51c8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue(1)) {
		m68k_incpc((uae_s32)offs + 2);
		if (offs & 1) {
			exception3_read_prefetch(opcode, m68k_getpc ());
			return 6 * CYCLE_UNIT / 2 + count_cycles;
		}
		if (src) {
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 6 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(oldpc + 4);
	if (!cctrue(1)) {
		m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
	}
	return 10 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0 B */

/* Scc.B (An) (F) */
uae_u32 REGPARAM2 op_51d0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	int val = cctrue(1) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (An)+ (F) */
uae_u32 REGPARAM2 op_51d8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	int val = cctrue(1) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B -(An) (F) */
uae_u32 REGPARAM2 op_51e0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg(regs, srcreg) = srca;
	int val = cctrue(1) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (d16,An) (F) */
uae_u32 REGPARAM2 op_51e8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	int val = cctrue(1) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (d8,An,Xn) (F) */
uae_u32 REGPARAM2 op_51f0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	int val = cctrue(1) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* Scc.B (xxx).W (F) */
uae_u32 REGPARAM2 op_51f8_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	int val = cctrue(1) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (xxx).L (F) */
uae_u32 REGPARAM2 op_51f9_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	int val = cctrue(1) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* Scc.B Dn (HI) */
uae_u32 REGPARAM2 op_52c0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	int val = cctrue(2) ? 0xff : 0x00;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* DBcc.W Dn,#<data>.W (HI) */
uae_u32 REGPARAM2 op_52c8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue(2)) {
		m68k_incpc((uae_s32)offs + 2);
		if (offs & 1) {
			exception3_read_prefetch(opcode, m68k_getpc ());
			return 6 * CYCLE_UNIT / 2 + count_cycles;
		}
		if (src) {
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 6 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(oldpc + 4);
	if (!cctrue(2)) {
		m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
	}
	return 10 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0 B */

/* Scc.B (An) (HI) */
uae_u32 REGPARAM2 op_52d0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	int val = cctrue(2) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (An)+ (HI) */
uae_u32 REGPARAM2 op_52d8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	int val = cctrue(2) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B -(An) (HI) */
uae_u32 REGPARAM2 op_52e0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg(regs, srcreg) = srca;
	int val = cctrue(2) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (d16,An) (HI) */
uae_u32 REGPARAM2 op_52e8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	int val = cctrue(2) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (d8,An,Xn) (HI) */
uae_u32 REGPARAM2 op_52f0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	int val = cctrue(2) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* Scc.B (xxx).W (HI) */
uae_u32 REGPARAM2 op_52f8_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	int val = cctrue(2) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (xxx).L (HI) */
uae_u32 REGPARAM2 op_52f9_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	int val = cctrue(2) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* Scc.B Dn (LS) */
uae_u32 REGPARAM2 op_53c0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	int val = cctrue(3) ? 0xff : 0x00;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* DBcc.W Dn,#<data>.W (LS) */
uae_u32 REGPARAM2 op_53c8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue(3)) {
		m68k_incpc((uae_s32)offs + 2);
		if (offs & 1) {
			exception3_read_prefetch(opcode, m68k_getpc ());
			return 6 * CYCLE_UNIT / 2 + count_cycles;
		}
		if (src) {
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 6 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(oldpc + 4);
	if (!cctrue(3)) {
		m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
	}
	return 10 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0 B */

/* Scc.B (An) (LS) */
uae_u32 REGPARAM2 op_53d0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	int val = cctrue(3) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (An)+ (LS) */
uae_u32 REGPARAM2 op_53d8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	int val = cctrue(3) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B -(An) (LS) */
uae_u32 REGPARAM2 op_53e0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg(regs, srcreg) = srca;
	int val = cctrue(3) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (d16,An) (LS) */
uae_u32 REGPARAM2 op_53e8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	int val = cctrue(3) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (d8,An,Xn) (LS) */
uae_u32 REGPARAM2 op_53f0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	int val = cctrue(3) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* Scc.B (xxx).W (LS) */
uae_u32 REGPARAM2 op_53f8_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	int val = cctrue(3) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (xxx).L (LS) */
uae_u32 REGPARAM2 op_53f9_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	int val = cctrue(3) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* Scc.B Dn (CC) */
uae_u32 REGPARAM2 op_54c0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	int val = cctrue(4) ? 0xff : 0x00;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* DBcc.W Dn,#<data>.W (CC) */
uae_u32 REGPARAM2 op_54c8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue(4)) {
		m68k_incpc((uae_s32)offs + 2);
		if (offs & 1) {
			exception3_read_prefetch(opcode, m68k_getpc ());
			return 6 * CYCLE_UNIT / 2 + count_cycles;
		}
		if (src) {
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 6 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(oldpc + 4);
	if (!cctrue(4)) {
		m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
	}
	return 10 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0 B */

/* Scc.B (An) (CC) */
uae_u32 REGPARAM2 op_54d0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	int val = cctrue(4) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (An)+ (CC) */
uae_u32 REGPARAM2 op_54d8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	int val = cctrue(4) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B -(An) (CC) */
uae_u32 REGPARAM2 op_54e0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg(regs, srcreg) = srca;
	int val = cctrue(4) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (d16,An) (CC) */
uae_u32 REGPARAM2 op_54e8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	int val = cctrue(4) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (d8,An,Xn) (CC) */
uae_u32 REGPARAM2 op_54f0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	int val = cctrue(4) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* Scc.B (xxx).W (CC) */
uae_u32 REGPARAM2 op_54f8_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	int val = cctrue(4) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (xxx).L (CC) */
uae_u32 REGPARAM2 op_54f9_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	int val = cctrue(4) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* Scc.B Dn (CS) */
uae_u32 REGPARAM2 op_55c0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	int val = cctrue(5) ? 0xff : 0x00;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* DBcc.W Dn,#<data>.W (CS) */
uae_u32 REGPARAM2 op_55c8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue(5)) {
		m68k_incpc((uae_s32)offs + 2);
		if (offs & 1) {
			exception3_read_prefetch(opcode, m68k_getpc ());
			return 6 * CYCLE_UNIT / 2 + count_cycles;
		}
		if (src) {
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 6 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(oldpc + 4);
	if (!cctrue(5)) {
		m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
	}
	return 10 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0 B */

/* Scc.B (An) (CS) */
uae_u32 REGPARAM2 op_55d0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	int val = cctrue(5) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (An)+ (CS) */
uae_u32 REGPARAM2 op_55d8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	int val = cctrue(5) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B -(An) (CS) */
uae_u32 REGPARAM2 op_55e0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg(regs, srcreg) = srca;
	int val = cctrue(5) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (d16,An) (CS) */
uae_u32 REGPARAM2 op_55e8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	int val = cctrue(5) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (d8,An,Xn) (CS) */
uae_u32 REGPARAM2 op_55f0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	int val = cctrue(5) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* Scc.B (xxx).W (CS) */
uae_u32 REGPARAM2 op_55f8_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	int val = cctrue(5) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (xxx).L (CS) */
uae_u32 REGPARAM2 op_55f9_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	int val = cctrue(5) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* Scc.B Dn (NE) */
uae_u32 REGPARAM2 op_56c0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	int val = cctrue(6) ? 0xff : 0x00;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* DBcc.W Dn,#<data>.W (NE) */
uae_u32 REGPARAM2 op_56c8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue(6)) {
		m68k_incpc((uae_s32)offs + 2);
		if (offs & 1) {
			exception3_read_prefetch(opcode, m68k_getpc ());
			return 6 * CYCLE_UNIT / 2 + count_cycles;
		}
		if (src) {
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 6 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(oldpc + 4);
	if (!cctrue(6)) {
		m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
	}
	return 10 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0 B */

/* Scc.B (An) (NE) */
uae_u32 REGPARAM2 op_56d0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	int val = cctrue(6) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (An)+ (NE) */
uae_u32 REGPARAM2 op_56d8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	int val = cctrue(6) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B -(An) (NE) */
uae_u32 REGPARAM2 op_56e0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg(regs, srcreg) = srca;
	int val = cctrue(6) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (d16,An) (NE) */
uae_u32 REGPARAM2 op_56e8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	int val = cctrue(6) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (d8,An,Xn) (NE) */
uae_u32 REGPARAM2 op_56f0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	int val = cctrue(6) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* Scc.B (xxx).W (NE) */
uae_u32 REGPARAM2 op_56f8_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	int val = cctrue(6) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (xxx).L (NE) */
uae_u32 REGPARAM2 op_56f9_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	int val = cctrue(6) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* Scc.B Dn (EQ) */
uae_u32 REGPARAM2 op_57c0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	int val = cctrue(7) ? 0xff : 0x00;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* DBcc.W Dn,#<data>.W (EQ) */
uae_u32 REGPARAM2 op_57c8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue(7)) {
		m68k_incpc((uae_s32)offs + 2);
		if (offs & 1) {
			exception3_read_prefetch(opcode, m68k_getpc ());
			return 6 * CYCLE_UNIT / 2 + count_cycles;
		}
		if (src) {
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 6 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(oldpc + 4);
	if (!cctrue(7)) {
		m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
	}
	return 10 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0 B */

/* Scc.B (An) (EQ) */
uae_u32 REGPARAM2 op_57d0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	int val = cctrue(7) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (An)+ (EQ) */
uae_u32 REGPARAM2 op_57d8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	int val = cctrue(7) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B -(An) (EQ) */
uae_u32 REGPARAM2 op_57e0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg(regs, srcreg) = srca;
	int val = cctrue(7) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (d16,An) (EQ) */
uae_u32 REGPARAM2 op_57e8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	int val = cctrue(7) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (d8,An,Xn) (EQ) */
uae_u32 REGPARAM2 op_57f0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	int val = cctrue(7) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* Scc.B (xxx).W (EQ) */
uae_u32 REGPARAM2 op_57f8_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	int val = cctrue(7) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (xxx).L (EQ) */
uae_u32 REGPARAM2 op_57f9_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	int val = cctrue(7) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* Scc.B Dn (VC) */
uae_u32 REGPARAM2 op_58c0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	int val = cctrue(8) ? 0xff : 0x00;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* DBcc.W Dn,#<data>.W (VC) */
uae_u32 REGPARAM2 op_58c8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue(8)) {
		m68k_incpc((uae_s32)offs + 2);
		if (offs & 1) {
			exception3_read_prefetch(opcode, m68k_getpc ());
			return 6 * CYCLE_UNIT / 2 + count_cycles;
		}
		if (src) {
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 6 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(oldpc + 4);
	if (!cctrue(8)) {
		m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
	}
	return 10 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0 B */

/* Scc.B (An) (VC) */
uae_u32 REGPARAM2 op_58d0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	int val = cctrue(8) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (An)+ (VC) */
uae_u32 REGPARAM2 op_58d8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	int val = cctrue(8) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B -(An) (VC) */
uae_u32 REGPARAM2 op_58e0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg(regs, srcreg) = srca;
	int val = cctrue(8) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (d16,An) (VC) */
uae_u32 REGPARAM2 op_58e8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	int val = cctrue(8) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (d8,An,Xn) (VC) */
uae_u32 REGPARAM2 op_58f0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	int val = cctrue(8) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* Scc.B (xxx).W (VC) */
uae_u32 REGPARAM2 op_58f8_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	int val = cctrue(8) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (xxx).L (VC) */
uae_u32 REGPARAM2 op_58f9_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	int val = cctrue(8) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* Scc.B Dn (VS) */
uae_u32 REGPARAM2 op_59c0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	int val = cctrue(9) ? 0xff : 0x00;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* DBcc.W Dn,#<data>.W (VS) */
uae_u32 REGPARAM2 op_59c8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue(9)) {
		m68k_incpc((uae_s32)offs + 2);
		if (offs & 1) {
			exception3_read_prefetch(opcode, m68k_getpc ());
			return 6 * CYCLE_UNIT / 2 + count_cycles;
		}
		if (src) {
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 6 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(oldpc + 4);
	if (!cctrue(9)) {
		m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
	}
	return 10 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0 B */

/* Scc.B (An) (VS) */
uae_u32 REGPARAM2 op_59d0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	int val = cctrue(9) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (An)+ (VS) */
uae_u32 REGPARAM2 op_59d8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	int val = cctrue(9) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B -(An) (VS) */
uae_u32 REGPARAM2 op_59e0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg(regs, srcreg) = srca;
	int val = cctrue(9) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (d16,An) (VS) */
uae_u32 REGPARAM2 op_59e8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	int val = cctrue(9) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (d8,An,Xn) (VS) */
uae_u32 REGPARAM2 op_59f0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	int val = cctrue(9) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* Scc.B (xxx).W (VS) */
uae_u32 REGPARAM2 op_59f8_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	int val = cctrue(9) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (xxx).L (VS) */
uae_u32 REGPARAM2 op_59f9_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	int val = cctrue(9) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* Scc.B Dn (PL) */
uae_u32 REGPARAM2 op_5ac0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	int val = cctrue(10) ? 0xff : 0x00;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* DBcc.W Dn,#<data>.W (PL) */
uae_u32 REGPARAM2 op_5ac8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue(10)) {
		m68k_incpc((uae_s32)offs + 2);
		if (offs & 1) {
			exception3_read_prefetch(opcode, m68k_getpc ());
			return 6 * CYCLE_UNIT / 2 + count_cycles;
		}
		if (src) {
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 6 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(oldpc + 4);
	if (!cctrue(10)) {
		m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
	}
	return 10 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0 B */

/* Scc.B (An) (PL) */
uae_u32 REGPARAM2 op_5ad0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	int val = cctrue(10) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (An)+ (PL) */
uae_u32 REGPARAM2 op_5ad8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	int val = cctrue(10) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B -(An) (PL) */
uae_u32 REGPARAM2 op_5ae0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg(regs, srcreg) = srca;
	int val = cctrue(10) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (d16,An) (PL) */
uae_u32 REGPARAM2 op_5ae8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	int val = cctrue(10) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (d8,An,Xn) (PL) */
uae_u32 REGPARAM2 op_5af0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	int val = cctrue(10) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* Scc.B (xxx).W (PL) */
uae_u32 REGPARAM2 op_5af8_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	int val = cctrue(10) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (xxx).L (PL) */
uae_u32 REGPARAM2 op_5af9_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	int val = cctrue(10) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* Scc.B Dn (MI) */
uae_u32 REGPARAM2 op_5bc0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	int val = cctrue(11) ? 0xff : 0x00;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* DBcc.W Dn,#<data>.W (MI) */
uae_u32 REGPARAM2 op_5bc8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue(11)) {
		m68k_incpc((uae_s32)offs + 2);
		if (offs & 1) {
			exception3_read_prefetch(opcode, m68k_getpc ());
			return 6 * CYCLE_UNIT / 2 + count_cycles;
		}
		if (src) {
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 6 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(oldpc + 4);
	if (!cctrue(11)) {
		m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
	}
	return 10 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0 B */

/* Scc.B (An) (MI) */
uae_u32 REGPARAM2 op_5bd0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	int val = cctrue(11) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (An)+ (MI) */
uae_u32 REGPARAM2 op_5bd8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	int val = cctrue(11) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B -(An) (MI) */
uae_u32 REGPARAM2 op_5be0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg(regs, srcreg) = srca;
	int val = cctrue(11) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (d16,An) (MI) */
uae_u32 REGPARAM2 op_5be8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	int val = cctrue(11) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (d8,An,Xn) (MI) */
uae_u32 REGPARAM2 op_5bf0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	int val = cctrue(11) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* Scc.B (xxx).W (MI) */
uae_u32 REGPARAM2 op_5bf8_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	int val = cctrue(11) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (xxx).L (MI) */
uae_u32 REGPARAM2 op_5bf9_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	int val = cctrue(11) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* Scc.B Dn (GE) */
uae_u32 REGPARAM2 op_5cc0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	int val = cctrue(12) ? 0xff : 0x00;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* DBcc.W Dn,#<data>.W (GE) */
uae_u32 REGPARAM2 op_5cc8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue(12)) {
		m68k_incpc((uae_s32)offs + 2);
		if (offs & 1) {
			exception3_read_prefetch(opcode, m68k_getpc ());
			return 6 * CYCLE_UNIT / 2 + count_cycles;
		}
		if (src) {
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 6 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(oldpc + 4);
	if (!cctrue(12)) {
		m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
	}
	return 10 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0 B */

/* Scc.B (An) (GE) */
uae_u32 REGPARAM2 op_5cd0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	int val = cctrue(12) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (An)+ (GE) */
uae_u32 REGPARAM2 op_5cd8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	int val = cctrue(12) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B -(An) (GE) */
uae_u32 REGPARAM2 op_5ce0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg(regs, srcreg) = srca;
	int val = cctrue(12) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (d16,An) (GE) */
uae_u32 REGPARAM2 op_5ce8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	int val = cctrue(12) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (d8,An,Xn) (GE) */
uae_u32 REGPARAM2 op_5cf0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	int val = cctrue(12) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* Scc.B (xxx).W (GE) */
uae_u32 REGPARAM2 op_5cf8_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	int val = cctrue(12) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (xxx).L (GE) */
uae_u32 REGPARAM2 op_5cf9_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	int val = cctrue(12) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* Scc.B Dn (LT) */
uae_u32 REGPARAM2 op_5dc0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	int val = cctrue(13) ? 0xff : 0x00;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* DBcc.W Dn,#<data>.W (LT) */
uae_u32 REGPARAM2 op_5dc8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue(13)) {
		m68k_incpc((uae_s32)offs + 2);
		if (offs & 1) {
			exception3_read_prefetch(opcode, m68k_getpc ());
			return 6 * CYCLE_UNIT / 2 + count_cycles;
		}
		if (src) {
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 6 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(oldpc + 4);
	if (!cctrue(13)) {
		m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
	}
	return 10 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0 B */

/* Scc.B (An) (LT) */
uae_u32 REGPARAM2 op_5dd0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	int val = cctrue(13) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (An)+ (LT) */
uae_u32 REGPARAM2 op_5dd8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	int val = cctrue(13) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B -(An) (LT) */
uae_u32 REGPARAM2 op_5de0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg(regs, srcreg) = srca;
	int val = cctrue(13) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (d16,An) (LT) */
uae_u32 REGPARAM2 op_5de8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	int val = cctrue(13) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (d8,An,Xn) (LT) */
uae_u32 REGPARAM2 op_5df0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	int val = cctrue(13) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* Scc.B (xxx).W (LT) */
uae_u32 REGPARAM2 op_5df8_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	int val = cctrue(13) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (xxx).L (LT) */
uae_u32 REGPARAM2 op_5df9_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	int val = cctrue(13) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* Scc.B Dn (GT) */
uae_u32 REGPARAM2 op_5ec0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	int val = cctrue(14) ? 0xff : 0x00;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* DBcc.W Dn,#<data>.W (GT) */
uae_u32 REGPARAM2 op_5ec8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue(14)) {
		m68k_incpc((uae_s32)offs + 2);
		if (offs & 1) {
			exception3_read_prefetch(opcode, m68k_getpc ());
			return 6 * CYCLE_UNIT / 2 + count_cycles;
		}
		if (src) {
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 6 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(oldpc + 4);
	if (!cctrue(14)) {
		m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
	}
	return 10 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0 B */

/* Scc.B (An) (GT) */
uae_u32 REGPARAM2 op_5ed0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	int val = cctrue(14) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (An)+ (GT) */
uae_u32 REGPARAM2 op_5ed8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	int val = cctrue(14) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B -(An) (GT) */
uae_u32 REGPARAM2 op_5ee0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg(regs, srcreg) = srca;
	int val = cctrue(14) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (d16,An) (GT) */
uae_u32 REGPARAM2 op_5ee8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	int val = cctrue(14) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (d8,An,Xn) (GT) */
uae_u32 REGPARAM2 op_5ef0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	int val = cctrue(14) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* Scc.B (xxx).W (GT) */
uae_u32 REGPARAM2 op_5ef8_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	int val = cctrue(14) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (xxx).L (GT) */
uae_u32 REGPARAM2 op_5ef9_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	int val = cctrue(14) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* Scc.B Dn (LE) */
uae_u32 REGPARAM2 op_5fc0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	int val = cctrue(15) ? 0xff : 0x00;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* DBcc.W Dn,#<data>.W (LE) */
uae_u32 REGPARAM2 op_5fc8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue(15)) {
		m68k_incpc((uae_s32)offs + 2);
		if (offs & 1) {
			exception3_read_prefetch(opcode, m68k_getpc ());
			return 6 * CYCLE_UNIT / 2 + count_cycles;
		}
		if (src) {
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 6 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(oldpc + 4);
	if (!cctrue(15)) {
		m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
	}
	return 10 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0 B */

/* Scc.B (An) (LE) */
uae_u32 REGPARAM2 op_5fd0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	int val = cctrue(15) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (An)+ (LE) */
uae_u32 REGPARAM2 op_5fd8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	int val = cctrue(15) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B -(An) (LE) */
uae_u32 REGPARAM2 op_5fe0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	m68k_areg(regs, srcreg) = srca;
	int val = cctrue(15) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (d16,An) (LE) */
uae_u32 REGPARAM2 op_5fe8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	int val = cctrue(15) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (d8,An,Xn) (LE) */
uae_u32 REGPARAM2 op_5ff0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	int val = cctrue(15) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* Scc.B (xxx).W (LE) */
uae_u32 REGPARAM2 op_5ff8_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	int val = cctrue(15) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (xxx).L (LE) */
uae_u32 REGPARAM2 op_5ff9_44_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	int val = cctrue(15) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* Bcc.W #<data>.W (T) */
uae_u32 REGPARAM2 op_6000_44_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	if (cctrue(0)) {
		if (src & 1) {
			uae_u16 rb = regs.irc;
			m68k_incpc(((uae_s32)src + 2) & ~1);
			uaecptr newpc = m68k_getpc () + (uae_s32)src + 2;
			m68k_incpc(2);
			regs.read_buffer = rb;
			exception3_read_prefetch(opcode, newpc);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(4);
	return 10 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* BccQ.B #<data> (T) */
uae_u32 REGPARAM2 op_6001_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
	uae_u32 src = srcreg;
	if (cctrue(0)) {
		if (src & 1) {
			uae_u16 rb = regs.irc;
			m68k_incpc(((uae_s32)src + 2) & ~1);
			uaecptr newpc = m68k_getpc () + (uae_s32)src + 2;
			m68k_incpc(2);
			regs.read_buffer = rb;
			exception3_read_prefetch(opcode, newpc);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* Bcc.L #<data>.L (T) */
uae_u32 REGPARAM2 op_60ff_44_ff(uae_u32 opcode)
{
	if (cctrue(0)) {
		exception3_read_prefetch(opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* BSR.W #<data>.W */
uae_u32 REGPARAM2 op_6100_44_ff(uae_u32 opcode)
{
	uae_s32 s;
	uae_s16 src = get_diword (2);
	s = (uae_s32)src + 2;
	uaecptr oldpc = m68k_getpc ();
	uaecptr nextpc = oldpc + 4;
	if (m68k_areg(regs, 7) & 1) {
		m68k_incpc(2);
		exception3_write_access(opcode, oldpc + s, sz_word, oldpc, 1);
		return 6 * CYCLE_UNIT / 2;
	}
	if (s & 1) {
		m68k_incpc(2);
		exception3_read_prefetch_only(opcode, oldpc + s);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_do_bsr(nextpc, s);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* BSRQ.B #<data> */
uae_u32 REGPARAM2 op_6101_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
	uae_s32 s;
	uae_u32 src = srcreg;
	s = (uae_s32)src + 2;
	uaecptr oldpc = m68k_getpc ();
	uaecptr nextpc = oldpc + 2;
	if (m68k_areg(regs, 7) & 1) {
		m68k_incpc(2);
		exception3_write_access(opcode, oldpc + s, sz_word, oldpc, 1);
		return 6 * CYCLE_UNIT / 2;
	}
	if (s & 1) {
		m68k_incpc(2);
		exception3_read_prefetch_only(opcode, oldpc + s);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_do_bsr(nextpc, s);
	return 18 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* BSR.L #<data>.L */
uae_u32 REGPARAM2 op_61ff_44_ff(uae_u32 opcode)
{
	uae_s32 s;
	uae_u32 src = 0xffffffff;
	s = (uae_s32)src + 2;
	uaecptr oldpc = m68k_getpc ();
	uaecptr nextpc = oldpc + 2;
	if (m68k_areg(regs, 7) & 1) {
		m68k_incpc(2);
		exception3_write_access(opcode, oldpc + s, sz_word, oldpc, 1);
		return 6 * CYCLE_UNIT / 2;
	}
	if (s & 1) {
		m68k_incpc(2);
		exception3_read_prefetch_only(opcode, oldpc + s);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_do_bsr(nextpc, s);
	return 18 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* Bcc.W #<data>.W (HI) */
uae_u32 REGPARAM2 op_6200_44_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	if (cctrue(2)) {
		if (src & 1) {
			uae_u16 rb = regs.irc;
			m68k_incpc(((uae_s32)src + 2) & ~1);
			uaecptr newpc = m68k_getpc () + (uae_s32)src + 2;
			m68k_incpc(2);
			regs.read_buffer = rb;
			exception3_read_prefetch(opcode, newpc);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(4);
	return 10 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* BccQ.B #<data> (HI) */
uae_u32 REGPARAM2 op_6201_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
	uae_u32 src = srcreg;
	if (cctrue(2)) {
		if (src & 1) {
			uae_u16 rb = regs.irc;
			m68k_incpc(((uae_s32)src + 2) & ~1);
			uaecptr newpc = m68k_getpc () + (uae_s32)src + 2;
			m68k_incpc(2);
			regs.read_buffer = rb;
			exception3_read_prefetch(opcode, newpc);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* Bcc.L #<data>.L (HI) */
uae_u32 REGPARAM2 op_62ff_44_ff(uae_u32 opcode)
{
	if (cctrue(2)) {
		exception3_read_prefetch(opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Bcc.W #<data>.W (LS) */
uae_u32 REGPARAM2 op_6300_44_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	if (cctrue(3)) {
		if (src & 1) {
			uae_u16 rb = regs.irc;
			m68k_incpc(((uae_s32)src + 2) & ~1);
			uaecptr newpc = m68k_getpc () + (uae_s32)src + 2;
			m68k_incpc(2);
			regs.read_buffer = rb;
			exception3_read_prefetch(opcode, newpc);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(4);
	return 10 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* BccQ.B #<data> (LS) */
uae_u32 REGPARAM2 op_6301_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
	uae_u32 src = srcreg;
	if (cctrue(3)) {
		if (src & 1) {
			uae_u16 rb = regs.irc;
			m68k_incpc(((uae_s32)src + 2) & ~1);
			uaecptr newpc = m68k_getpc () + (uae_s32)src + 2;
			m68k_incpc(2);
			regs.read_buffer = rb;
			exception3_read_prefetch(opcode, newpc);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* Bcc.L #<data>.L (LS) */
uae_u32 REGPARAM2 op_63ff_44_ff(uae_u32 opcode)
{
	if (cctrue(3)) {
		exception3_read_prefetch(opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Bcc.W #<data>.W (CC) */
uae_u32 REGPARAM2 op_6400_44_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	if (cctrue(4)) {
		if (src & 1) {
			uae_u16 rb = regs.irc;
			m68k_incpc(((uae_s32)src + 2) & ~1);
			uaecptr newpc = m68k_getpc () + (uae_s32)src + 2;
			m68k_incpc(2);
			regs.read_buffer = rb;
			exception3_read_prefetch(opcode, newpc);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(4);
	return 10 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* BccQ.B #<data> (CC) */
uae_u32 REGPARAM2 op_6401_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
	uae_u32 src = srcreg;
	if (cctrue(4)) {
		if (src & 1) {
			uae_u16 rb = regs.irc;
			m68k_incpc(((uae_s32)src + 2) & ~1);
			uaecptr newpc = m68k_getpc () + (uae_s32)src + 2;
			m68k_incpc(2);
			regs.read_buffer = rb;
			exception3_read_prefetch(opcode, newpc);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* Bcc.L #<data>.L (CC) */
uae_u32 REGPARAM2 op_64ff_44_ff(uae_u32 opcode)
{
	if (cctrue(4)) {
		exception3_read_prefetch(opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Bcc.W #<data>.W (CS) */
uae_u32 REGPARAM2 op_6500_44_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	if (cctrue(5)) {
		if (src & 1) {
			uae_u16 rb = regs.irc;
			m68k_incpc(((uae_s32)src + 2) & ~1);
			uaecptr newpc = m68k_getpc () + (uae_s32)src + 2;
			m68k_incpc(2);
			regs.read_buffer = rb;
			exception3_read_prefetch(opcode, newpc);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(4);
	return 10 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* BccQ.B #<data> (CS) */
uae_u32 REGPARAM2 op_6501_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
	uae_u32 src = srcreg;
	if (cctrue(5)) {
		if (src & 1) {
			uae_u16 rb = regs.irc;
			m68k_incpc(((uae_s32)src + 2) & ~1);
			uaecptr newpc = m68k_getpc () + (uae_s32)src + 2;
			m68k_incpc(2);
			regs.read_buffer = rb;
			exception3_read_prefetch(opcode, newpc);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* Bcc.L #<data>.L (CS) */
uae_u32 REGPARAM2 op_65ff_44_ff(uae_u32 opcode)
{
	if (cctrue(5)) {
		exception3_read_prefetch(opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Bcc.W #<data>.W (NE) */
uae_u32 REGPARAM2 op_6600_44_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	if (cctrue(6)) {
		if (src & 1) {
			uae_u16 rb = regs.irc;
			m68k_incpc(((uae_s32)src + 2) & ~1);
			uaecptr newpc = m68k_getpc () + (uae_s32)src + 2;
			m68k_incpc(2);
			regs.read_buffer = rb;
			exception3_read_prefetch(opcode, newpc);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(4);
	return 10 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* BccQ.B #<data> (NE) */
uae_u32 REGPARAM2 op_6601_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
	uae_u32 src = srcreg;
	if (cctrue(6)) {
		if (src & 1) {
			uae_u16 rb = regs.irc;
			m68k_incpc(((uae_s32)src + 2) & ~1);
			uaecptr newpc = m68k_getpc () + (uae_s32)src + 2;
			m68k_incpc(2);
			regs.read_buffer = rb;
			exception3_read_prefetch(opcode, newpc);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* Bcc.L #<data>.L (NE) */
uae_u32 REGPARAM2 op_66ff_44_ff(uae_u32 opcode)
{
	if (cctrue(6)) {
		exception3_read_prefetch(opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Bcc.W #<data>.W (EQ) */
uae_u32 REGPARAM2 op_6700_44_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	if (cctrue(7)) {
		if (src & 1) {
			uae_u16 rb = regs.irc;
			m68k_incpc(((uae_s32)src + 2) & ~1);
			uaecptr newpc = m68k_getpc () + (uae_s32)src + 2;
			m68k_incpc(2);
			regs.read_buffer = rb;
			exception3_read_prefetch(opcode, newpc);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(4);
	return 10 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* BccQ.B #<data> (EQ) */
uae_u32 REGPARAM2 op_6701_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
	uae_u32 src = srcreg;
	if (cctrue(7)) {
		if (src & 1) {
			uae_u16 rb = regs.irc;
			m68k_incpc(((uae_s32)src + 2) & ~1);
			uaecptr newpc = m68k_getpc () + (uae_s32)src + 2;
			m68k_incpc(2);
			regs.read_buffer = rb;
			exception3_read_prefetch(opcode, newpc);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* Bcc.L #<data>.L (EQ) */
uae_u32 REGPARAM2 op_67ff_44_ff(uae_u32 opcode)
{
	if (cctrue(7)) {
		exception3_read_prefetch(opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Bcc.W #<data>.W (VC) */
uae_u32 REGPARAM2 op_6800_44_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	if (cctrue(8)) {
		if (src & 1) {
			uae_u16 rb = regs.irc;
			m68k_incpc(((uae_s32)src + 2) & ~1);
			uaecptr newpc = m68k_getpc () + (uae_s32)src + 2;
			m68k_incpc(2);
			regs.read_buffer = rb;
			exception3_read_prefetch(opcode, newpc);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(4);
	return 10 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* BccQ.B #<data> (VC) */
uae_u32 REGPARAM2 op_6801_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
	uae_u32 src = srcreg;
	if (cctrue(8)) {
		if (src & 1) {
			uae_u16 rb = regs.irc;
			m68k_incpc(((uae_s32)src + 2) & ~1);
			uaecptr newpc = m68k_getpc () + (uae_s32)src + 2;
			m68k_incpc(2);
			regs.read_buffer = rb;
			exception3_read_prefetch(opcode, newpc);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* Bcc.L #<data>.L (VC) */
uae_u32 REGPARAM2 op_68ff_44_ff(uae_u32 opcode)
{
	if (cctrue(8)) {
		exception3_read_prefetch(opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Bcc.W #<data>.W (VS) */
uae_u32 REGPARAM2 op_6900_44_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	if (cctrue(9)) {
		if (src & 1) {
			uae_u16 rb = regs.irc;
			m68k_incpc(((uae_s32)src + 2) & ~1);
			uaecptr newpc = m68k_getpc () + (uae_s32)src + 2;
			m68k_incpc(2);
			regs.read_buffer = rb;
			exception3_read_prefetch(opcode, newpc);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(4);
	return 10 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* BccQ.B #<data> (VS) */
uae_u32 REGPARAM2 op_6901_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
	uae_u32 src = srcreg;
	if (cctrue(9)) {
		if (src & 1) {
			uae_u16 rb = regs.irc;
			m68k_incpc(((uae_s32)src + 2) & ~1);
			uaecptr newpc = m68k_getpc () + (uae_s32)src + 2;
			m68k_incpc(2);
			regs.read_buffer = rb;
			exception3_read_prefetch(opcode, newpc);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* Bcc.L #<data>.L (VS) */
uae_u32 REGPARAM2 op_69ff_44_ff(uae_u32 opcode)
{
	if (cctrue(9)) {
		exception3_read_prefetch(opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Bcc.W #<data>.W (PL) */
uae_u32 REGPARAM2 op_6a00_44_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	if (cctrue(10)) {
		if (src & 1) {
			uae_u16 rb = regs.irc;
			m68k_incpc(((uae_s32)src + 2) & ~1);
			uaecptr newpc = m68k_getpc () + (uae_s32)src + 2;
			m68k_incpc(2);
			regs.read_buffer = rb;
			exception3_read_prefetch(opcode, newpc);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(4);
	return 10 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* BccQ.B #<data> (PL) */
uae_u32 REGPARAM2 op_6a01_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
	uae_u32 src = srcreg;
	if (cctrue(10)) {
		if (src & 1) {
			uae_u16 rb = regs.irc;
			m68k_incpc(((uae_s32)src + 2) & ~1);
			uaecptr newpc = m68k_getpc () + (uae_s32)src + 2;
			m68k_incpc(2);
			regs.read_buffer = rb;
			exception3_read_prefetch(opcode, newpc);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* Bcc.L #<data>.L (PL) */
uae_u32 REGPARAM2 op_6aff_44_ff(uae_u32 opcode)
{
	if (cctrue(10)) {
		exception3_read_prefetch(opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Bcc.W #<data>.W (MI) */
uae_u32 REGPARAM2 op_6b00_44_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	if (cctrue(11)) {
		if (src & 1) {
			uae_u16 rb = regs.irc;
			m68k_incpc(((uae_s32)src + 2) & ~1);
			uaecptr newpc = m68k_getpc () + (uae_s32)src + 2;
			m68k_incpc(2);
			regs.read_buffer = rb;
			exception3_read_prefetch(opcode, newpc);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(4);
	return 10 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* BccQ.B #<data> (MI) */
uae_u32 REGPARAM2 op_6b01_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
	uae_u32 src = srcreg;
	if (cctrue(11)) {
		if (src & 1) {
			uae_u16 rb = regs.irc;
			m68k_incpc(((uae_s32)src + 2) & ~1);
			uaecptr newpc = m68k_getpc () + (uae_s32)src + 2;
			m68k_incpc(2);
			regs.read_buffer = rb;
			exception3_read_prefetch(opcode, newpc);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* Bcc.L #<data>.L (MI) */
uae_u32 REGPARAM2 op_6bff_44_ff(uae_u32 opcode)
{
	if (cctrue(11)) {
		exception3_read_prefetch(opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Bcc.W #<data>.W (GE) */
uae_u32 REGPARAM2 op_6c00_44_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	if (cctrue(12)) {
		if (src & 1) {
			uae_u16 rb = regs.irc;
			m68k_incpc(((uae_s32)src + 2) & ~1);
			uaecptr newpc = m68k_getpc () + (uae_s32)src + 2;
			m68k_incpc(2);
			regs.read_buffer = rb;
			exception3_read_prefetch(opcode, newpc);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(4);
	return 10 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* BccQ.B #<data> (GE) */
uae_u32 REGPARAM2 op_6c01_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
	uae_u32 src = srcreg;
	if (cctrue(12)) {
		if (src & 1) {
			uae_u16 rb = regs.irc;
			m68k_incpc(((uae_s32)src + 2) & ~1);
			uaecptr newpc = m68k_getpc () + (uae_s32)src + 2;
			m68k_incpc(2);
			regs.read_buffer = rb;
			exception3_read_prefetch(opcode, newpc);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* Bcc.L #<data>.L (GE) */
uae_u32 REGPARAM2 op_6cff_44_ff(uae_u32 opcode)
{
	if (cctrue(12)) {
		exception3_read_prefetch(opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Bcc.W #<data>.W (LT) */
uae_u32 REGPARAM2 op_6d00_44_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	if (cctrue(13)) {
		if (src & 1) {
			uae_u16 rb = regs.irc;
			m68k_incpc(((uae_s32)src + 2) & ~1);
			uaecptr newpc = m68k_getpc () + (uae_s32)src + 2;
			m68k_incpc(2);
			regs.read_buffer = rb;
			exception3_read_prefetch(opcode, newpc);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(4);
	return 10 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* BccQ.B #<data> (LT) */
uae_u32 REGPARAM2 op_6d01_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
	uae_u32 src = srcreg;
	if (cctrue(13)) {
		if (src & 1) {
			uae_u16 rb = regs.irc;
			m68k_incpc(((uae_s32)src + 2) & ~1);
			uaecptr newpc = m68k_getpc () + (uae_s32)src + 2;
			m68k_incpc(2);
			regs.read_buffer = rb;
			exception3_read_prefetch(opcode, newpc);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* Bcc.L #<data>.L (LT) */
uae_u32 REGPARAM2 op_6dff_44_ff(uae_u32 opcode)
{
	if (cctrue(13)) {
		exception3_read_prefetch(opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Bcc.W #<data>.W (GT) */
uae_u32 REGPARAM2 op_6e00_44_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	if (cctrue(14)) {
		if (src & 1) {
			uae_u16 rb = regs.irc;
			m68k_incpc(((uae_s32)src + 2) & ~1);
			uaecptr newpc = m68k_getpc () + (uae_s32)src + 2;
			m68k_incpc(2);
			regs.read_buffer = rb;
			exception3_read_prefetch(opcode, newpc);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(4);
	return 10 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* BccQ.B #<data> (GT) */
uae_u32 REGPARAM2 op_6e01_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
	uae_u32 src = srcreg;
	if (cctrue(14)) {
		if (src & 1) {
			uae_u16 rb = regs.irc;
			m68k_incpc(((uae_s32)src + 2) & ~1);
			uaecptr newpc = m68k_getpc () + (uae_s32)src + 2;
			m68k_incpc(2);
			regs.read_buffer = rb;
			exception3_read_prefetch(opcode, newpc);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* Bcc.L #<data>.L (GT) */
uae_u32 REGPARAM2 op_6eff_44_ff(uae_u32 opcode)
{
	if (cctrue(14)) {
		exception3_read_prefetch(opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Bcc.W #<data>.W (LE) */
uae_u32 REGPARAM2 op_6f00_44_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	if (cctrue(15)) {
		if (src & 1) {
			uae_u16 rb = regs.irc;
			m68k_incpc(((uae_s32)src + 2) & ~1);
			uaecptr newpc = m68k_getpc () + (uae_s32)src + 2;
			m68k_incpc(2);
			regs.read_buffer = rb;
			exception3_read_prefetch(opcode, newpc);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(4);
	return 10 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* BccQ.B #<data> (LE) */
uae_u32 REGPARAM2 op_6f01_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
	uae_u32 src = srcreg;
	if (cctrue(15)) {
		if (src & 1) {
			uae_u16 rb = regs.irc;
			m68k_incpc(((uae_s32)src + 2) & ~1);
			uaecptr newpc = m68k_getpc () + (uae_s32)src + 2;
			m68k_incpc(2);
			regs.read_buffer = rb;
			exception3_read_prefetch(opcode, newpc);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* Bcc.L #<data>.L (LE) */
uae_u32 REGPARAM2 op_6fff_44_ff(uae_u32 opcode)
{
	if (cctrue(15)) {
		exception3_read_prefetch(opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVEQ.L #<data>,Dn */
uae_u32 REGPARAM2 op_7000_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_u32 src = srcreg;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* OR.B Dn,Dn */
uae_u32 REGPARAM2 op_8000_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* OR.B (An),Dn */
uae_u32 REGPARAM2 op_8010_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* OR.B (An)+,Dn */
uae_u32 REGPARAM2 op_8018_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	uae_s8 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* OR.B -(An),Dn */
uae_u32 REGPARAM2 op_8020_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uae_s8 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* OR.B (d16,An),Dn */
uae_u32 REGPARAM2 op_8028_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* OR.B (d8,An,Xn),Dn */
uae_u32 REGPARAM2 op_8030_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* OR.B (xxx).W,Dn */
uae_u32 REGPARAM2 op_8038_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* OR.B (xxx).L,Dn */
uae_u32 REGPARAM2 op_8039_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s8 src = get_byte_jit(srca);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* OR.B (d16,PC),Dn */
uae_u32 REGPARAM2 op_803a_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* OR.B (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 op_803b_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* OR.B #<data>.B,Dn */
uae_u32 REGPARAM2 op_803c_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s8 src = get_dibyte (2);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc(4);
	return 8 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* OR.W Dn,Dn */
uae_u32 REGPARAM2 op_8040_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* OR.W (An),Dn */
uae_u32 REGPARAM2 op_8050_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* OR.W (An)+,Dn */
uae_u32 REGPARAM2 op_8058_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) += 2;
	uae_s16 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* OR.W -(An),Dn */
uae_u32 REGPARAM2 op_8060_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uae_s16 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* OR.W (d16,An),Dn */
uae_u32 REGPARAM2 op_8068_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* OR.W (d8,An,Xn),Dn */
uae_u32 REGPARAM2 op_8070_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* OR.W (xxx).W,Dn */
uae_u32 REGPARAM2 op_8078_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* OR.W (xxx).L,Dn */
uae_u32 REGPARAM2 op_8079_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* OR.W (d16,PC),Dn */
uae_u32 REGPARAM2 op_807a_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* OR.W (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 op_807b_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* OR.W #<data>.W,Dn */
uae_u32 REGPARAM2 op_807c_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = get_diword (2);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc(4);
	return 8 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* OR.L Dn,Dn */
uae_u32 REGPARAM2 op_8080_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* OR.L (An),Dn */
uae_u32 REGPARAM2 op_8090_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* OR.L (An)+,Dn */
uae_u32 REGPARAM2 op_8098_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) += 4;
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* OR.L -(An),Dn */
uae_u32 REGPARAM2 op_80a0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(2);
	return 16 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* OR.L (d16,An),Dn */
uae_u32 REGPARAM2 op_80a8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* OR.L (d8,An,Xn),Dn */
uae_u32 REGPARAM2 op_80b0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* OR.L (xxx).W,Dn */
uae_u32 REGPARAM2 op_80b8_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* OR.L (xxx).L,Dn */
uae_u32 REGPARAM2 op_80b9_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* OR.L (d16,PC),Dn */
uae_u32 REGPARAM2 op_80ba_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* OR.L (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 op_80bb_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* OR.L #<data>.L,Dn */
uae_u32 REGPARAM2 op_80bc_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src;
	src = get_dilong(2);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* DIVU.W Dn,Dn */
uae_u32 REGPARAM2 op_80c0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc ();
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	if (src == 0) {
		divbyzero_special(0, dst);
		m68k_incpc(2);
		Exception_cpu_oldpc(5,oldpc);
		return 8 * CYCLE_UNIT / 2 + count_cycles;
	}
	uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
	uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
	count_cycles += (getDivu68kCycles((uae_u32)dst, (uae_u16)src)) * CYCLE_UNIT / 2;
	if (newv > 0xffff) {
		setdivuflags((uae_u32)dst, (uae_u16)src);
	} else {
		CLEAR_CZNV();
		SET_ZFLG(((uae_s16)(newv)) == 0);
		SET_NFLG(((uae_s16)(newv)) < 0);
		newv = (newv & 0xffff) | ((uae_u32)rem << 16);
		m68k_dreg(regs, dstreg) = (newv);
	}
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* DIVU.W (An),Dn */
uae_u32 REGPARAM2 op_80d0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc ();
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	if (src == 0) {
		divbyzero_special(0, dst);
		m68k_incpc(2);
		Exception_cpu_oldpc(5,oldpc);
		return 12 * CYCLE_UNIT / 2 + count_cycles;
	}
	uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
	uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
	count_cycles += (getDivu68kCycles((uae_u32)dst, (uae_u16)src)) * CYCLE_UNIT / 2;
	if (newv > 0xffff) {
		setdivuflags((uae_u32)dst, (uae_u16)src);
	} else {
		CLEAR_CZNV();
		SET_ZFLG(((uae_s16)(newv)) == 0);
		SET_NFLG(((uae_s16)(newv)) < 0);
		newv = (newv & 0xffff) | ((uae_u32)rem << 16);
		m68k_dreg(regs, dstreg) = (newv);
	}
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* DIVU.W (An)+,Dn */
uae_u32 REGPARAM2 op_80d8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc ();
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) += 2;
	uae_s32 dst = m68k_dreg(regs, dstreg);
	if (src == 0) {
		divbyzero_special(0, dst);
		m68k_incpc(2);
		Exception_cpu_oldpc(5,oldpc);
		return 12 * CYCLE_UNIT / 2 + count_cycles;
	}
	uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
	uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
	count_cycles += (getDivu68kCycles((uae_u32)dst, (uae_u16)src)) * CYCLE_UNIT / 2;
	if (newv > 0xffff) {
		setdivuflags((uae_u32)dst, (uae_u16)src);
	} else {
		CLEAR_CZNV();
		SET_ZFLG(((uae_s16)(newv)) == 0);
		SET_NFLG(((uae_s16)(newv)) < 0);
		newv = (newv & 0xffff) | ((uae_u32)rem << 16);
		m68k_dreg(regs, dstreg) = (newv);
	}
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* DIVU.W -(An),Dn */
uae_u32 REGPARAM2 op_80e0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc ();
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uae_s32 dst = m68k_dreg(regs, dstreg);
	if (src == 0) {
		divbyzero_special(0, dst);
		m68k_incpc(2);
		Exception_cpu_oldpc(5,oldpc);
		return 14 * CYCLE_UNIT / 2 + count_cycles;
	}
	uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
	uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
	count_cycles += (getDivu68kCycles((uae_u32)dst, (uae_u16)src)) * CYCLE_UNIT / 2;
	if (newv > 0xffff) {
		setdivuflags((uae_u32)dst, (uae_u16)src);
	} else {
		CLEAR_CZNV();
		SET_ZFLG(((uae_s16)(newv)) == 0);
		SET_NFLG(((uae_s16)(newv)) < 0);
		newv = (newv & 0xffff) | ((uae_u32)rem << 16);
		m68k_dreg(regs, dstreg) = (newv);
	}
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* DIVU.W (d16,An),Dn */
uae_u32 REGPARAM2 op_80e8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc ();
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	if (src == 0) {
		divbyzero_special(0, dst);
		m68k_incpc(4);
		Exception_cpu_oldpc(5,oldpc);
		return 16 * CYCLE_UNIT / 2 + count_cycles;
	}
	uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
	uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
	count_cycles += (getDivu68kCycles((uae_u32)dst, (uae_u16)src)) * CYCLE_UNIT / 2;
	if (newv > 0xffff) {
		setdivuflags((uae_u32)dst, (uae_u16)src);
	} else {
		CLEAR_CZNV();
		SET_ZFLG(((uae_s16)(newv)) == 0);
		SET_NFLG(((uae_s16)(newv)) < 0);
		newv = (newv & 0xffff) | ((uae_u32)rem << 16);
		m68k_dreg(regs, dstreg) = (newv);
	}
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0   */

/* DIVU.W (d8,An,Xn),Dn */
uae_u32 REGPARAM2 op_80f0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc ();
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	if (src == 0) {
		divbyzero_special(0, dst);
		m68k_incpc(4);
		Exception_cpu_oldpc(5,oldpc);
		return 18 * CYCLE_UNIT / 2 + count_cycles;
	}
	uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
	uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
	count_cycles += (getDivu68kCycles((uae_u32)dst, (uae_u16)src)) * CYCLE_UNIT / 2;
	if (newv > 0xffff) {
		setdivuflags((uae_u32)dst, (uae_u16)src);
	} else {
		CLEAR_CZNV();
		SET_ZFLG(((uae_s16)(newv)) == 0);
		SET_NFLG(((uae_s16)(newv)) < 0);
		newv = (newv & 0xffff) | ((uae_u32)rem << 16);
		m68k_dreg(regs, dstreg) = (newv);
	}
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 4,0   */

/* DIVU.W (xxx).W,Dn */
uae_u32 REGPARAM2 op_80f8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc ();
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	if (src == 0) {
		divbyzero_special(0, dst);
		m68k_incpc(4);
		Exception_cpu_oldpc(5,oldpc);
		return 16 * CYCLE_UNIT / 2 + count_cycles;
	}
	uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
	uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
	count_cycles += (getDivu68kCycles((uae_u32)dst, (uae_u16)src)) * CYCLE_UNIT / 2;
	if (newv > 0xffff) {
		setdivuflags((uae_u32)dst, (uae_u16)src);
	} else {
		CLEAR_CZNV();
		SET_ZFLG(((uae_s16)(newv)) == 0);
		SET_NFLG(((uae_s16)(newv)) < 0);
		newv = (newv & 0xffff) | ((uae_u32)rem << 16);
		m68k_dreg(regs, dstreg) = (newv);
	}
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0   */

/* DIVU.W (xxx).L,Dn */
uae_u32 REGPARAM2 op_80f9_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc ();
	uaecptr srca;
	srca = get_dilong(2);
	uae_s16 src = get_word_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	if (src == 0) {
		divbyzero_special(0, dst);
		m68k_incpc(6);
		Exception_cpu_oldpc(5,oldpc);
		return 20 * CYCLE_UNIT / 2 + count_cycles;
	}
	uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
	uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
	count_cycles += (getDivu68kCycles((uae_u32)dst, (uae_u16)src)) * CYCLE_UNIT / 2;
	if (newv > 0xffff) {
		setdivuflags((uae_u32)dst, (uae_u16)src);
	} else {
		CLEAR_CZNV();
		SET_ZFLG(((uae_s16)(newv)) == 0);
		SET_NFLG(((uae_s16)(newv)) < 0);
		newv = (newv & 0xffff) | ((uae_u32)rem << 16);
		m68k_dreg(regs, dstreg) = (newv);
	}
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2 + count_cycles;
}
/* 6 0,0   */

/* DIVU.W (d16,PC),Dn */
uae_u32 REGPARAM2 op_80fa_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc ();
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	if (src == 0) {
		divbyzero_special(0, dst);
		m68k_incpc(4);
		Exception_cpu_oldpc(5,oldpc);
		return 16 * CYCLE_UNIT / 2 + count_cycles;
	}
	uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
	uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
	count_cycles += (getDivu68kCycles((uae_u32)dst, (uae_u16)src)) * CYCLE_UNIT / 2;
	if (newv > 0xffff) {
		setdivuflags((uae_u32)dst, (uae_u16)src);
	} else {
		CLEAR_CZNV();
		SET_ZFLG(((uae_s16)(newv)) == 0);
		SET_NFLG(((uae_s16)(newv)) < 0);
		newv = (newv & 0xffff) | ((uae_u32)rem << 16);
		m68k_dreg(regs, dstreg) = (newv);
	}
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0   */

/* DIVU.W (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 op_80fb_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc ();
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	if (src == 0) {
		divbyzero_special(0, dst);
		m68k_incpc(4);
		Exception_cpu_oldpc(5,oldpc);
		return 18 * CYCLE_UNIT / 2 + count_cycles;
	}
	uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
	uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
	count_cycles += (getDivu68kCycles((uae_u32)dst, (uae_u16)src)) * CYCLE_UNIT / 2;
	if (newv > 0xffff) {
		setdivuflags((uae_u32)dst, (uae_u16)src);
	} else {
		CLEAR_CZNV();
		SET_ZFLG(((uae_s16)(newv)) == 0);
		SET_NFLG(((uae_s16)(newv)) < 0);
		newv = (newv & 0xffff) | ((uae_u32)rem << 16);
		m68k_dreg(regs, dstreg) = (newv);
	}
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 4,0   */

/* DIVU.W #<data>.W,Dn */
uae_u32 REGPARAM2 op_80fc_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc ();
	uae_s16 src = get_diword (2);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	if (src == 0) {
		divbyzero_special(0, dst);
		m68k_incpc(4);
		Exception_cpu_oldpc(5,oldpc);
		return 12 * CYCLE_UNIT / 2 + count_cycles;
	}
	uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
	uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
	count_cycles += (getDivu68kCycles((uae_u32)dst, (uae_u16)src)) * CYCLE_UNIT / 2;
	if (newv > 0xffff) {
		setdivuflags((uae_u32)dst, (uae_u16)src);
	} else {
		CLEAR_CZNV();
		SET_ZFLG(((uae_s16)(newv)) == 0);
		SET_NFLG(((uae_s16)(newv)) < 0);
		newv = (newv & 0xffff) | ((uae_u32)rem << 16);
		m68k_dreg(regs, dstreg) = (newv);
	}
	m68k_incpc(4);
	return 8 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0   */

/* SBCD.B Dn,Dn */
uae_u32 REGPARAM2 op_8100_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u16 newv_lo = (dst & 0xF) - (src & 0xF) - (GET_XFLG() ? 1 : 0);
	uae_u16 newv_hi = (dst & 0xF0) - (src & 0xF0);
	uae_u16 newv, tmp_newv;
	int bcd = 0;
	newv = tmp_newv = newv_hi + newv_lo;
if (newv_lo & 0xF0) { newv -= 6; bcd = 6; };
if ((((dst & 0xFF) - (src & 0xFF) - (GET_XFLG() ? 1 : 0)) & 0x100) > 0xFF) { newv -= 0x60; }
	SET_CFLG((((dst & 0xFF) - (src & 0xFF) - bcd - (GET_XFLG() ? 1 : 0)) & 0x300) > 0xFF);
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s8)(newv)) == 0));
	SET_NFLG(((uae_s8)(newv)) < 0);
	SET_VFLG((tmp_newv & 0x80) != 0 && (newv & 0x80) == 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SBCD.B -(An),-(An) */
uae_u32 REGPARAM2 op_8108_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	uae_u16 newv_lo = (dst & 0xF) - (src & 0xF) - (GET_XFLG() ? 1 : 0);
	uae_u16 newv_hi = (dst & 0xF0) - (src & 0xF0);
	uae_u16 newv, tmp_newv;
	int bcd = 0;
	newv = tmp_newv = newv_hi + newv_lo;
if (newv_lo & 0xF0) { newv -= 6; bcd = 6; };
if ((((dst & 0xFF) - (src & 0xFF) - (GET_XFLG() ? 1 : 0)) & 0x100) > 0xFF) { newv -= 0x60; }
	SET_CFLG((((dst & 0xFF) - (src & 0xFF) - bcd - (GET_XFLG() ? 1 : 0)) & 0x300) > 0xFF);
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s8)(newv)) == 0));
	SET_NFLG(((uae_s8)(newv)) < 0);
	SET_VFLG((tmp_newv & 0x80) != 0 && (newv & 0x80) == 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(2);
	return 18 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* OR.B Dn,(An) */
uae_u32 REGPARAM2 op_8110_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* OR.B Dn,(An)+ */
uae_u32 REGPARAM2 op_8118_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* OR.B Dn,-(An) */
uae_u32 REGPARAM2 op_8120_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* OR.B Dn,(d16,An) */
uae_u32 REGPARAM2 op_8128_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 dst = get_byte_jit(dsta);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* OR.B Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 op_8130_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (2));
	uae_s8 dst = get_byte_jit(dsta);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* OR.B Dn,(xxx).W */
uae_u32 REGPARAM2 op_8138_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 dst = get_byte_jit(dsta);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* OR.B Dn,(xxx).L */
uae_u32 REGPARAM2 op_8139_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_dilong(2);
	uae_s8 dst = get_byte_jit(dsta);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* OR.W Dn,(An) */
uae_u32 REGPARAM2 op_8150_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s16 dst = get_word_jit(dsta);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* OR.W Dn,(An)+ */
uae_u32 REGPARAM2 op_8158_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s16 dst = get_word_jit(dsta);
	m68k_areg(regs, dstreg) += 2;
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* OR.W Dn,-(An) */
uae_u32 REGPARAM2 op_8160_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	uae_s16 dst = get_word_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* OR.W Dn,(d16,An) */
uae_u32 REGPARAM2 op_8168_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 dst = get_word_jit(dsta);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* OR.W Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 op_8170_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (2));
	uae_s16 dst = get_word_jit(dsta);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* OR.W Dn,(xxx).W */
uae_u32 REGPARAM2 op_8178_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 dst = get_word_jit(dsta);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* OR.W Dn,(xxx).L */
uae_u32 REGPARAM2 op_8179_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_dilong(2);
	uae_s16 dst = get_word_jit(dsta);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* OR.L Dn,(An) */
uae_u32 REGPARAM2 op_8190_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s32 dst = get_long_jit(dsta);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(2);
	return 20 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* OR.L Dn,(An)+ */
uae_u32 REGPARAM2 op_8198_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s32 dst = get_long_jit(dsta);
	m68k_areg(regs, dstreg) += 4;
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(2);
	return 20 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* OR.L Dn,-(An) */
uae_u32 REGPARAM2 op_81a0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	uae_s32 dst = get_long_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(2);
	return 22 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* OR.L Dn,(d16,An) */
uae_u32 REGPARAM2 op_81a8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s32 dst = get_long_jit(dsta);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(4);
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* OR.L Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 op_81b0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (2));
	uae_s32 dst = get_long_jit(dsta);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(4);
	return 26 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* OR.L Dn,(xxx).W */
uae_u32 REGPARAM2 op_81b8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	uae_s32 dst = get_long_jit(dsta);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(4);
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* OR.L Dn,(xxx).L */
uae_u32 REGPARAM2 op_81b9_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_dilong(2);
	uae_s32 dst = get_long_jit(dsta);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(6);
	return 28 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* DIVS.W Dn,Dn */
uae_u32 REGPARAM2 op_81c0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc ();
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	if (src == 0) {
		divbyzero_special(1, dst);
		m68k_incpc(2);
		Exception_cpu_oldpc(5,oldpc);
		return 8 * CYCLE_UNIT / 2 + count_cycles;
	}
	count_cycles += (getDivs68kCycles((uae_s32)dst, (uae_s16)src)) * CYCLE_UNIT / 2;
	if (dst == 0x80000000 && src == -1) {
		setdivsflags((uae_s32)dst, (uae_s16)src);
	} else {
		uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
		uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
		if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) {
			setdivsflags((uae_s32)dst, (uae_s16)src);
		} else {
			if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
			CLEAR_CZNV();
			SET_ZFLG(((uae_s16)(newv)) == 0);
			SET_NFLG(((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg(regs, dstreg) = (newv);
		}
	}
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* DIVS.W (An),Dn */
uae_u32 REGPARAM2 op_81d0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc ();
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	if (src == 0) {
		divbyzero_special(1, dst);
		m68k_incpc(2);
		Exception_cpu_oldpc(5,oldpc);
		return 12 * CYCLE_UNIT / 2 + count_cycles;
	}
	count_cycles += (getDivs68kCycles((uae_s32)dst, (uae_s16)src)) * CYCLE_UNIT / 2;
	if (dst == 0x80000000 && src == -1) {
		setdivsflags((uae_s32)dst, (uae_s16)src);
	} else {
		uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
		uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
		if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) {
			setdivsflags((uae_s32)dst, (uae_s16)src);
		} else {
			if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
			CLEAR_CZNV();
			SET_ZFLG(((uae_s16)(newv)) == 0);
			SET_NFLG(((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg(regs, dstreg) = (newv);
		}
	}
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* DIVS.W (An)+,Dn */
uae_u32 REGPARAM2 op_81d8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc ();
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) += 2;
	uae_s32 dst = m68k_dreg(regs, dstreg);
	if (src == 0) {
		divbyzero_special(1, dst);
		m68k_incpc(2);
		Exception_cpu_oldpc(5,oldpc);
		return 12 * CYCLE_UNIT / 2 + count_cycles;
	}
	count_cycles += (getDivs68kCycles((uae_s32)dst, (uae_s16)src)) * CYCLE_UNIT / 2;
	if (dst == 0x80000000 && src == -1) {
		setdivsflags((uae_s32)dst, (uae_s16)src);
	} else {
		uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
		uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
		if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) {
			setdivsflags((uae_s32)dst, (uae_s16)src);
		} else {
			if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
			CLEAR_CZNV();
			SET_ZFLG(((uae_s16)(newv)) == 0);
			SET_NFLG(((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg(regs, dstreg) = (newv);
		}
	}
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* DIVS.W -(An),Dn */
uae_u32 REGPARAM2 op_81e0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc ();
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uae_s32 dst = m68k_dreg(regs, dstreg);
	if (src == 0) {
		divbyzero_special(1, dst);
		m68k_incpc(2);
		Exception_cpu_oldpc(5,oldpc);
		return 14 * CYCLE_UNIT / 2 + count_cycles;
	}
	count_cycles += (getDivs68kCycles((uae_s32)dst, (uae_s16)src)) * CYCLE_UNIT / 2;
	if (dst == 0x80000000 && src == -1) {
		setdivsflags((uae_s32)dst, (uae_s16)src);
	} else {
		uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
		uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
		if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) {
			setdivsflags((uae_s32)dst, (uae_s16)src);
		} else {
			if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
			CLEAR_CZNV();
			SET_ZFLG(((uae_s16)(newv)) == 0);
			SET_NFLG(((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg(regs, dstreg) = (newv);
		}
	}
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* DIVS.W (d16,An),Dn */
uae_u32 REGPARAM2 op_81e8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc ();
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	if (src == 0) {
		divbyzero_special(1, dst);
		m68k_incpc(4);
		Exception_cpu_oldpc(5,oldpc);
		return 16 * CYCLE_UNIT / 2 + count_cycles;
	}
	count_cycles += (getDivs68kCycles((uae_s32)dst, (uae_s16)src)) * CYCLE_UNIT / 2;
	if (dst == 0x80000000 && src == -1) {
		setdivsflags((uae_s32)dst, (uae_s16)src);
	} else {
		uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
		uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
		if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) {
			setdivsflags((uae_s32)dst, (uae_s16)src);
		} else {
			if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
			CLEAR_CZNV();
			SET_ZFLG(((uae_s16)(newv)) == 0);
			SET_NFLG(((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg(regs, dstreg) = (newv);
		}
	}
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0   */

/* DIVS.W (d8,An,Xn),Dn */
uae_u32 REGPARAM2 op_81f0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc ();
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	if (src == 0) {
		divbyzero_special(1, dst);
		m68k_incpc(4);
		Exception_cpu_oldpc(5,oldpc);
		return 18 * CYCLE_UNIT / 2 + count_cycles;
	}
	count_cycles += (getDivs68kCycles((uae_s32)dst, (uae_s16)src)) * CYCLE_UNIT / 2;
	if (dst == 0x80000000 && src == -1) {
		setdivsflags((uae_s32)dst, (uae_s16)src);
	} else {
		uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
		uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
		if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) {
			setdivsflags((uae_s32)dst, (uae_s16)src);
		} else {
			if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
			CLEAR_CZNV();
			SET_ZFLG(((uae_s16)(newv)) == 0);
			SET_NFLG(((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg(regs, dstreg) = (newv);
		}
	}
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 4,0   */

/* DIVS.W (xxx).W,Dn */
uae_u32 REGPARAM2 op_81f8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc ();
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	if (src == 0) {
		divbyzero_special(1, dst);
		m68k_incpc(4);
		Exception_cpu_oldpc(5,oldpc);
		return 16 * CYCLE_UNIT / 2 + count_cycles;
	}
	count_cycles += (getDivs68kCycles((uae_s32)dst, (uae_s16)src)) * CYCLE_UNIT / 2;
	if (dst == 0x80000000 && src == -1) {
		setdivsflags((uae_s32)dst, (uae_s16)src);
	} else {
		uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
		uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
		if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) {
			setdivsflags((uae_s32)dst, (uae_s16)src);
		} else {
			if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
			CLEAR_CZNV();
			SET_ZFLG(((uae_s16)(newv)) == 0);
			SET_NFLG(((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg(regs, dstreg) = (newv);
		}
	}
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0   */

/* DIVS.W (xxx).L,Dn */
uae_u32 REGPARAM2 op_81f9_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc ();
	uaecptr srca;
	srca = get_dilong(2);
	uae_s16 src = get_word_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	if (src == 0) {
		divbyzero_special(1, dst);
		m68k_incpc(6);
		Exception_cpu_oldpc(5,oldpc);
		return 20 * CYCLE_UNIT / 2 + count_cycles;
	}
	count_cycles += (getDivs68kCycles((uae_s32)dst, (uae_s16)src)) * CYCLE_UNIT / 2;
	if (dst == 0x80000000 && src == -1) {
		setdivsflags((uae_s32)dst, (uae_s16)src);
	} else {
		uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
		uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
		if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) {
			setdivsflags((uae_s32)dst, (uae_s16)src);
		} else {
			if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
			CLEAR_CZNV();
			SET_ZFLG(((uae_s16)(newv)) == 0);
			SET_NFLG(((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg(regs, dstreg) = (newv);
		}
	}
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2 + count_cycles;
}
/* 6 0,0   */

/* DIVS.W (d16,PC),Dn */
uae_u32 REGPARAM2 op_81fa_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc ();
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	if (src == 0) {
		divbyzero_special(1, dst);
		m68k_incpc(4);
		Exception_cpu_oldpc(5,oldpc);
		return 16 * CYCLE_UNIT / 2 + count_cycles;
	}
	count_cycles += (getDivs68kCycles((uae_s32)dst, (uae_s16)src)) * CYCLE_UNIT / 2;
	if (dst == 0x80000000 && src == -1) {
		setdivsflags((uae_s32)dst, (uae_s16)src);
	} else {
		uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
		uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
		if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) {
			setdivsflags((uae_s32)dst, (uae_s16)src);
		} else {
			if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
			CLEAR_CZNV();
			SET_ZFLG(((uae_s16)(newv)) == 0);
			SET_NFLG(((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg(regs, dstreg) = (newv);
		}
	}
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0   */

/* DIVS.W (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 op_81fb_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc ();
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	if (src == 0) {
		divbyzero_special(1, dst);
		m68k_incpc(4);
		Exception_cpu_oldpc(5,oldpc);
		return 18 * CYCLE_UNIT / 2 + count_cycles;
	}
	count_cycles += (getDivs68kCycles((uae_s32)dst, (uae_s16)src)) * CYCLE_UNIT / 2;
	if (dst == 0x80000000 && src == -1) {
		setdivsflags((uae_s32)dst, (uae_s16)src);
	} else {
		uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
		uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
		if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) {
			setdivsflags((uae_s32)dst, (uae_s16)src);
		} else {
			if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
			CLEAR_CZNV();
			SET_ZFLG(((uae_s16)(newv)) == 0);
			SET_NFLG(((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg(regs, dstreg) = (newv);
		}
	}
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 4,0   */

/* DIVS.W #<data>.W,Dn */
uae_u32 REGPARAM2 op_81fc_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr oldpc = m68k_getpc ();
	uae_s16 src = get_diword (2);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	if (src == 0) {
		divbyzero_special(1, dst);
		m68k_incpc(4);
		Exception_cpu_oldpc(5,oldpc);
		return 12 * CYCLE_UNIT / 2 + count_cycles;
	}
	count_cycles += (getDivs68kCycles((uae_s32)dst, (uae_s16)src)) * CYCLE_UNIT / 2;
	if (dst == 0x80000000 && src == -1) {
		setdivsflags((uae_s32)dst, (uae_s16)src);
	} else {
		uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
		uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
		if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) {
			setdivsflags((uae_s32)dst, (uae_s16)src);
		} else {
			if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
			CLEAR_CZNV();
			SET_ZFLG(((uae_s16)(newv)) == 0);
			SET_NFLG(((uae_s16)(newv)) < 0);
			newv = (newv & 0xffff) | ((uae_u32)rem << 16);
			m68k_dreg(regs, dstreg) = (newv);
		}
	}
	m68k_incpc(4);
	return 8 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0   */

/* SUB.B Dn,Dn */
uae_u32 REGPARAM2 op_9000_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUB.B (An),Dn */
uae_u32 REGPARAM2 op_9010_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUB.B (An)+,Dn */
uae_u32 REGPARAM2 op_9018_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUB.B -(An),Dn */
uae_u32 REGPARAM2 op_9020_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUB.B (d16,An),Dn */
uae_u32 REGPARAM2 op_9028_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUB.B (d8,An,Xn),Dn */
uae_u32 REGPARAM2 op_9030_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* SUB.B (xxx).W,Dn */
uae_u32 REGPARAM2 op_9038_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUB.B (xxx).L,Dn */
uae_u32 REGPARAM2 op_9039_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s8 src = get_byte_jit(srca);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* SUB.B (d16,PC),Dn */
uae_u32 REGPARAM2 op_903a_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUB.B (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 op_903b_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* SUB.B #<data>.B,Dn */
uae_u32 REGPARAM2 op_903c_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s8 src = get_dibyte (2);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
	m68k_incpc(4);
	return 8 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUB.W Dn,Dn */
uae_u32 REGPARAM2 op_9040_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUB.W An,Dn */
uae_u32 REGPARAM2 op_9048_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = m68k_areg(regs, srcreg);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUB.W (An),Dn */
uae_u32 REGPARAM2 op_9050_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUB.W (An)+,Dn */
uae_u32 REGPARAM2 op_9058_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) += 2;
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUB.W -(An),Dn */
uae_u32 REGPARAM2 op_9060_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUB.W (d16,An),Dn */
uae_u32 REGPARAM2 op_9068_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUB.W (d8,An,Xn),Dn */
uae_u32 REGPARAM2 op_9070_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* SUB.W (xxx).W,Dn */
uae_u32 REGPARAM2 op_9078_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUB.W (xxx).L,Dn */
uae_u32 REGPARAM2 op_9079_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* SUB.W (d16,PC),Dn */
uae_u32 REGPARAM2 op_907a_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUB.W (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 op_907b_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* SUB.W #<data>.W,Dn */
uae_u32 REGPARAM2 op_907c_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = get_diword (2);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
	m68k_incpc(4);
	return 8 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUB.L Dn,Dn */
uae_u32 REGPARAM2 op_9080_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUB.L An,Dn */
uae_u32 REGPARAM2 op_9088_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src = m68k_areg(regs, srcreg);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUB.L (An),Dn */
uae_u32 REGPARAM2 op_9090_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUB.L (An)+,Dn */
uae_u32 REGPARAM2 op_9098_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) += 4;
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUB.L -(An),Dn */
uae_u32 REGPARAM2 op_90a0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 16 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUB.L (d16,An),Dn */
uae_u32 REGPARAM2 op_90a8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUB.L (d8,An,Xn),Dn */
uae_u32 REGPARAM2 op_90b0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* SUB.L (xxx).W,Dn */
uae_u32 REGPARAM2 op_90b8_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUB.L (xxx).L,Dn */
uae_u32 REGPARAM2 op_90b9_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* SUB.L (d16,PC),Dn */
uae_u32 REGPARAM2 op_90ba_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUB.L (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 op_90bb_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* SUB.L #<data>.L,Dn */
uae_u32 REGPARAM2 op_90bc_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src;
	src = get_dilong(2);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(6);
	return 14 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* SUBA.W Dn,An */
uae_u32 REGPARAM2 op_90c0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUBA.W An,An */
uae_u32 REGPARAM2 op_90c8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = m68k_areg(regs, srcreg);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUBA.W (An),An */
uae_u32 REGPARAM2 op_90d0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUBA.W (An)+,An */
uae_u32 REGPARAM2 op_90d8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) += 2;
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUBA.W -(An),An */
uae_u32 REGPARAM2 op_90e0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUBA.W (d16,An),An */
uae_u32 REGPARAM2 op_90e8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUBA.W (d8,An,Xn),An */
uae_u32 REGPARAM2 op_90f0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* SUBA.W (xxx).W,An */
uae_u32 REGPARAM2 op_90f8_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUBA.W (xxx).L,An */
uae_u32 REGPARAM2 op_90f9_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s16 src = get_word_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* SUBA.W (d16,PC),An */
uae_u32 REGPARAM2 op_90fa_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUBA.W (d8,PC,Xn),An */
uae_u32 REGPARAM2 op_90fb_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* SUBA.W #<data>.W,An */
uae_u32 REGPARAM2 op_90fc_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = get_diword (2);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUBX.B Dn,Dn */
uae_u32 REGPARAM2 op_9100_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = dst - src - (GET_XFLG () ? 1 : 0);
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s8)(newv)) == 0));
	SET_NFLG(((uae_s8)(newv)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUBX.B -(An),-(An) */
uae_u32 REGPARAM2 op_9108_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	uae_u32 newv = dst - src - (GET_XFLG () ? 1 : 0);
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s8)(newv)) == 0));
	SET_NFLG(((uae_s8)(newv)) < 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(2);
	return 18 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUB.B Dn,(An) */
uae_u32 REGPARAM2 op_9110_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUB.B Dn,(An)+ */
uae_u32 REGPARAM2 op_9118_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUB.B Dn,-(An) */
uae_u32 REGPARAM2 op_9120_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUB.B Dn,(d16,An) */
uae_u32 REGPARAM2 op_9128_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 dst = get_byte_jit(dsta);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUB.B Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 op_9130_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (2));
	uae_s8 dst = get_byte_jit(dsta);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* SUB.B Dn,(xxx).W */
uae_u32 REGPARAM2 op_9138_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 dst = get_byte_jit(dsta);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUB.B Dn,(xxx).L */
uae_u32 REGPARAM2 op_9139_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_dilong(2);
	uae_s8 dst = get_byte_jit(dsta);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* SUBX.W Dn,Dn */
uae_u32 REGPARAM2 op_9140_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = dst - src - (GET_XFLG () ? 1 : 0);
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s16)(newv)) == 0));
	SET_NFLG(((uae_s16)(newv)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUBX.W -(An),-(An) */
uae_u32 REGPARAM2 op_9148_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	uae_s16 dst = get_word_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	uae_u32 newv = dst - src - (GET_XFLG () ? 1 : 0);
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s16)(newv)) == 0));
	SET_NFLG(((uae_s16)(newv)) < 0);
	put_word_jit(dsta, newv);
	m68k_incpc(2);
	return 18 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUB.W Dn,(An) */
uae_u32 REGPARAM2 op_9150_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s16 dst = get_word_jit(dsta);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUB.W Dn,(An)+ */
uae_u32 REGPARAM2 op_9158_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s16 dst = get_word_jit(dsta);
	m68k_areg(regs, dstreg) += 2;
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUB.W Dn,-(An) */
uae_u32 REGPARAM2 op_9160_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	uae_s16 dst = get_word_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUB.W Dn,(d16,An) */
uae_u32 REGPARAM2 op_9168_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 dst = get_word_jit(dsta);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUB.W Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 op_9170_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (2));
	uae_s16 dst = get_word_jit(dsta);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* SUB.W Dn,(xxx).W */
uae_u32 REGPARAM2 op_9178_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 dst = get_word_jit(dsta);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUB.W Dn,(xxx).L */
uae_u32 REGPARAM2 op_9179_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_dilong(2);
	uae_s16 dst = get_word_jit(dsta);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* SUBX.L Dn,Dn */
uae_u32 REGPARAM2 op_9180_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = dst - src - (GET_XFLG () ? 1 : 0);
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s32)(newv)) == 0));
	SET_NFLG(((uae_s32)(newv)) < 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUBX.L -(An),-(An) */
uae_u32 REGPARAM2 op_9188_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	uae_s32 dst = get_long_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	uae_u32 newv = dst - src - (GET_XFLG () ? 1 : 0);
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s32)(newv)) == 0));
	SET_NFLG(((uae_s32)(newv)) < 0);
	put_long_jit(dsta, newv);
	m68k_incpc(2);
	return 30 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUB.L Dn,(An) */
uae_u32 REGPARAM2 op_9190_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s32 dst = get_long_jit(dsta);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(2);
	return 20 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUB.L Dn,(An)+ */
uae_u32 REGPARAM2 op_9198_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s32 dst = get_long_jit(dsta);
	m68k_areg(regs, dstreg) += 4;
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(2);
	return 20 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUB.L Dn,-(An) */
uae_u32 REGPARAM2 op_91a0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	uae_s32 dst = get_long_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(2);
	return 22 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUB.L Dn,(d16,An) */
uae_u32 REGPARAM2 op_91a8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s32 dst = get_long_jit(dsta);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(4);
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUB.L Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 op_91b0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (2));
	uae_s32 dst = get_long_jit(dsta);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(4);
	return 26 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* SUB.L Dn,(xxx).W */
uae_u32 REGPARAM2 op_91b8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	uae_s32 dst = get_long_jit(dsta);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(4);
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUB.L Dn,(xxx).L */
uae_u32 REGPARAM2 op_91b9_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_dilong(2);
	uae_s32 dst = get_long_jit(dsta);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(6);
	return 28 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* SUBA.L Dn,An */
uae_u32 REGPARAM2 op_91c0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUBA.L An,An */
uae_u32 REGPARAM2 op_91c8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src = m68k_areg(regs, srcreg);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUBA.L (An),An */
uae_u32 REGPARAM2 op_91d0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUBA.L (An)+,An */
uae_u32 REGPARAM2 op_91d8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) += 4;
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUBA.L -(An),An */
uae_u32 REGPARAM2 op_91e0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 16 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUBA.L (d16,An),An */
uae_u32 REGPARAM2 op_91e8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUBA.L (d8,An,Xn),An */
uae_u32 REGPARAM2 op_91f0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* SUBA.L (xxx).W,An */
uae_u32 REGPARAM2 op_91f8_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUBA.L (xxx).L,An */
uae_u32 REGPARAM2 op_91f9_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* SUBA.L (d16,PC),An */
uae_u32 REGPARAM2 op_91fa_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUBA.L (d8,PC,Xn),An */
uae_u32 REGPARAM2 op_91fb_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* SUBA.L #<data>.L,An */
uae_u32 REGPARAM2 op_91fc_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src;
	src = get_dilong(2);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst - src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* CMP.B Dn,Dn */
uae_u32 REGPARAM2 op_b000_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CMP.B (An),Dn */
uae_u32 REGPARAM2 op_b010_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CMP.B (An)+,Dn */
uae_u32 REGPARAM2 op_b018_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CMP.B -(An),Dn */
uae_u32 REGPARAM2 op_b020_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CMP.B (d16,An),Dn */
uae_u32 REGPARAM2 op_b028_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* CMP.B (d8,An,Xn),Dn */
uae_u32 REGPARAM2 op_b030_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* CMP.B (xxx).W,Dn */
uae_u32 REGPARAM2 op_b038_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* CMP.B (xxx).L,Dn */
uae_u32 REGPARAM2 op_b039_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s8 src = get_byte_jit(srca);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* CMP.B (d16,PC),Dn */
uae_u32 REGPARAM2 op_b03a_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* CMP.B (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 op_b03b_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* CMP.B #<data>.B,Dn */
uae_u32 REGPARAM2 op_b03c_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s8 src = get_dibyte (2);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(4);
	return 8 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* CMP.W Dn,Dn */
uae_u32 REGPARAM2 op_b040_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CMP.W An,Dn */
uae_u32 REGPARAM2 op_b048_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = m68k_areg(regs, srcreg);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CMP.W (An),Dn */
uae_u32 REGPARAM2 op_b050_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CMP.W (An)+,Dn */
uae_u32 REGPARAM2 op_b058_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) += 2;
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CMP.W -(An),Dn */
uae_u32 REGPARAM2 op_b060_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CMP.W (d16,An),Dn */
uae_u32 REGPARAM2 op_b068_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* CMP.W (d8,An,Xn),Dn */
uae_u32 REGPARAM2 op_b070_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* CMP.W (xxx).W,Dn */
uae_u32 REGPARAM2 op_b078_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* CMP.W (xxx).L,Dn */
uae_u32 REGPARAM2 op_b079_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* CMP.W (d16,PC),Dn */
uae_u32 REGPARAM2 op_b07a_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* CMP.W (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 op_b07b_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* CMP.W #<data>.W,Dn */
uae_u32 REGPARAM2 op_b07c_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = get_diword (2);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(4);
	return 8 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* CMP.L Dn,Dn */
uae_u32 REGPARAM2 op_b080_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CMP.L An,Dn */
uae_u32 REGPARAM2 op_b088_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src = m68k_areg(regs, srcreg);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CMP.L (An),Dn */
uae_u32 REGPARAM2 op_b090_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CMP.L (An)+,Dn */
uae_u32 REGPARAM2 op_b098_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) += 4;
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CMP.L -(An),Dn */
uae_u32 REGPARAM2 op_b0a0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(2);
	return 16 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CMP.L (d16,An),Dn */
uae_u32 REGPARAM2 op_b0a8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* CMP.L (d8,An,Xn),Dn */
uae_u32 REGPARAM2 op_b0b0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* CMP.L (xxx).W,Dn */
uae_u32 REGPARAM2 op_b0b8_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* CMP.L (xxx).L,Dn */
uae_u32 REGPARAM2 op_b0b9_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* CMP.L (d16,PC),Dn */
uae_u32 REGPARAM2 op_b0ba_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* CMP.L (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 op_b0bb_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* CMP.L #<data>.L,Dn */
uae_u32 REGPARAM2 op_b0bc_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src;
	src = get_dilong(2);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(6);
	return 14 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* CMPA.W Dn,An */
uae_u32 REGPARAM2 op_b0c0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CMPA.W An,An */
uae_u32 REGPARAM2 op_b0c8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = m68k_areg(regs, srcreg);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CMPA.W (An),An */
uae_u32 REGPARAM2 op_b0d0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CMPA.W (An)+,An */
uae_u32 REGPARAM2 op_b0d8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) += 2;
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CMPA.W -(An),An */
uae_u32 REGPARAM2 op_b0e0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CMPA.W (d16,An),An */
uae_u32 REGPARAM2 op_b0e8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* CMPA.W (d8,An,Xn),An */
uae_u32 REGPARAM2 op_b0f0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* CMPA.W (xxx).W,An */
uae_u32 REGPARAM2 op_b0f8_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* CMPA.W (xxx).L,An */
uae_u32 REGPARAM2 op_b0f9_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s16 src = get_word_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(6);
	return 18 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* CMPA.W (d16,PC),An */
uae_u32 REGPARAM2 op_b0fa_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* CMPA.W (d8,PC,Xn),An */
uae_u32 REGPARAM2 op_b0fb_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* CMPA.W #<data>.W,An */
uae_u32 REGPARAM2 op_b0fc_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = get_diword (2);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(4);
	return 10 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* EOR.B Dn,Dn */
uae_u32 REGPARAM2 op_b100_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CMPM.B (An)+,(An)+ */
uae_u32 REGPARAM2 op_b108_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	uae_u32 newv = ((uae_u8)(dst)) - ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u8)(src)) > ((uae_u8)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* EOR.B Dn,(An) */
uae_u32 REGPARAM2 op_b110_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* EOR.B Dn,(An)+ */
uae_u32 REGPARAM2 op_b118_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* EOR.B Dn,-(An) */
uae_u32 REGPARAM2 op_b120_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* EOR.B Dn,(d16,An) */
uae_u32 REGPARAM2 op_b128_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 dst = get_byte_jit(dsta);
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* EOR.B Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 op_b130_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (2));
	uae_s8 dst = get_byte_jit(dsta);
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* EOR.B Dn,(xxx).W */
uae_u32 REGPARAM2 op_b138_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 dst = get_byte_jit(dsta);
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* EOR.B Dn,(xxx).L */
uae_u32 REGPARAM2 op_b139_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_dilong(2);
	uae_s8 dst = get_byte_jit(dsta);
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* EOR.W Dn,Dn */
uae_u32 REGPARAM2 op_b140_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CMPM.W (An)+,(An)+ */
uae_u32 REGPARAM2 op_b148_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) += 2;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s16 dst = get_word_jit(dsta);
	m68k_areg(regs, dstreg) += 2;
	uae_u32 newv = ((uae_u16)(dst)) - ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u16)(src)) > ((uae_u16)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* EOR.W Dn,(An) */
uae_u32 REGPARAM2 op_b150_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s16 dst = get_word_jit(dsta);
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* EOR.W Dn,(An)+ */
uae_u32 REGPARAM2 op_b158_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s16 dst = get_word_jit(dsta);
	m68k_areg(regs, dstreg) += 2;
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* EOR.W Dn,-(An) */
uae_u32 REGPARAM2 op_b160_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	uae_s16 dst = get_word_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* EOR.W Dn,(d16,An) */
uae_u32 REGPARAM2 op_b168_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 dst = get_word_jit(dsta);
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* EOR.W Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 op_b170_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (2));
	uae_s16 dst = get_word_jit(dsta);
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* EOR.W Dn,(xxx).W */
uae_u32 REGPARAM2 op_b178_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 dst = get_word_jit(dsta);
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* EOR.W Dn,(xxx).L */
uae_u32 REGPARAM2 op_b179_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_dilong(2);
	uae_s16 dst = get_word_jit(dsta);
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* EOR.L Dn,Dn */
uae_u32 REGPARAM2 op_b180_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CMPM.L (An)+,(An)+ */
uae_u32 REGPARAM2 op_b188_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) += 4;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s32 dst = get_long_jit(dsta);
	m68k_areg(regs, dstreg) += 4;
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(2);
	return 20 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* EOR.L Dn,(An) */
uae_u32 REGPARAM2 op_b190_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s32 dst = get_long_jit(dsta);
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(2);
	return 20 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* EOR.L Dn,(An)+ */
uae_u32 REGPARAM2 op_b198_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s32 dst = get_long_jit(dsta);
	m68k_areg(regs, dstreg) += 4;
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(2);
	return 20 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* EOR.L Dn,-(An) */
uae_u32 REGPARAM2 op_b1a0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	uae_s32 dst = get_long_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(2);
	return 22 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* EOR.L Dn,(d16,An) */
uae_u32 REGPARAM2 op_b1a8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s32 dst = get_long_jit(dsta);
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(4);
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* EOR.L Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 op_b1b0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (2));
	uae_s32 dst = get_long_jit(dsta);
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(4);
	return 26 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* EOR.L Dn,(xxx).W */
uae_u32 REGPARAM2 op_b1b8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	uae_s32 dst = get_long_jit(dsta);
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(4);
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* EOR.L Dn,(xxx).L */
uae_u32 REGPARAM2 op_b1b9_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_dilong(2);
	uae_s32 dst = get_long_jit(dsta);
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(6);
	return 28 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* CMPA.L Dn,An */
uae_u32 REGPARAM2 op_b1c0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CMPA.L An,An */
uae_u32 REGPARAM2 op_b1c8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src = m68k_areg(regs, srcreg);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CMPA.L (An),An */
uae_u32 REGPARAM2 op_b1d0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CMPA.L (An)+,An */
uae_u32 REGPARAM2 op_b1d8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) += 4;
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CMPA.L -(An),An */
uae_u32 REGPARAM2 op_b1e0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(2);
	return 16 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CMPA.L (d16,An),An */
uae_u32 REGPARAM2 op_b1e8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* CMPA.L (d8,An,Xn),An */
uae_u32 REGPARAM2 op_b1f0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* CMPA.L (xxx).W,An */
uae_u32 REGPARAM2 op_b1f8_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* CMPA.L (xxx).L,An */
uae_u32 REGPARAM2 op_b1f9_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* CMPA.L (d16,PC),An */
uae_u32 REGPARAM2 op_b1fa_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* CMPA.L (d8,PC,Xn),An */
uae_u32 REGPARAM2 op_b1fb_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* CMPA.L #<data>.L,An */
uae_u32 REGPARAM2 op_b1fc_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src;
	src = get_dilong(2);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs != flgo) && (flgn != flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	SET_NFLG(flgn != 0);
	m68k_incpc(6);
	return 14 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* AND.B Dn,Dn */
uae_u32 REGPARAM2 op_c000_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* AND.B (An),Dn */
uae_u32 REGPARAM2 op_c010_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* AND.B (An)+,Dn */
uae_u32 REGPARAM2 op_c018_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	uae_s8 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* AND.B -(An),Dn */
uae_u32 REGPARAM2 op_c020_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uae_s8 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* AND.B (d16,An),Dn */
uae_u32 REGPARAM2 op_c028_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* AND.B (d8,An,Xn),Dn */
uae_u32 REGPARAM2 op_c030_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* AND.B (xxx).W,Dn */
uae_u32 REGPARAM2 op_c038_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* AND.B (xxx).L,Dn */
uae_u32 REGPARAM2 op_c039_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s8 src = get_byte_jit(srca);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* AND.B (d16,PC),Dn */
uae_u32 REGPARAM2 op_c03a_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* AND.B (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 op_c03b_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* AND.B #<data>.B,Dn */
uae_u32 REGPARAM2 op_c03c_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s8 src = get_dibyte (2);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc(4);
	return 8 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* AND.W Dn,Dn */
uae_u32 REGPARAM2 op_c040_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* AND.W (An),Dn */
uae_u32 REGPARAM2 op_c050_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* AND.W (An)+,Dn */
uae_u32 REGPARAM2 op_c058_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) += 2;
	uae_s16 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* AND.W -(An),Dn */
uae_u32 REGPARAM2 op_c060_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uae_s16 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* AND.W (d16,An),Dn */
uae_u32 REGPARAM2 op_c068_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* AND.W (d8,An,Xn),Dn */
uae_u32 REGPARAM2 op_c070_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* AND.W (xxx).W,Dn */
uae_u32 REGPARAM2 op_c078_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* AND.W (xxx).L,Dn */
uae_u32 REGPARAM2 op_c079_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* AND.W (d16,PC),Dn */
uae_u32 REGPARAM2 op_c07a_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* AND.W (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 op_c07b_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* AND.W #<data>.W,Dn */
uae_u32 REGPARAM2 op_c07c_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = get_diword (2);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((src) & 0xffff);
	m68k_incpc(4);
	return 8 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* AND.L Dn,Dn */
uae_u32 REGPARAM2 op_c080_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* AND.L (An),Dn */
uae_u32 REGPARAM2 op_c090_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* AND.L (An)+,Dn */
uae_u32 REGPARAM2 op_c098_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) += 4;
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* AND.L -(An),Dn */
uae_u32 REGPARAM2 op_c0a0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(2);
	return 16 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* AND.L (d16,An),Dn */
uae_u32 REGPARAM2 op_c0a8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* AND.L (d8,An,Xn),Dn */
uae_u32 REGPARAM2 op_c0b0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* AND.L (xxx).W,Dn */
uae_u32 REGPARAM2 op_c0b8_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* AND.L (xxx).L,Dn */
uae_u32 REGPARAM2 op_c0b9_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* AND.L (d16,PC),Dn */
uae_u32 REGPARAM2 op_c0ba_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* AND.L (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 op_c0bb_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* AND.L #<data>.L,Dn */
uae_u32 REGPARAM2 op_c0bc_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src;
	src = get_dilong(2);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MULU.W Dn,Dn */
uae_u32 REGPARAM2 op_c0c0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_NFLG(((uae_s32)(newv)) < 0);
	count_cycles += (getMulu68kCycles(src)) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* MULU.W (An),Dn */
uae_u32 REGPARAM2 op_c0d0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_NFLG(((uae_s32)(newv)) < 0);
	count_cycles += (getMulu68kCycles(src)) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* MULU.W (An)+,Dn */
uae_u32 REGPARAM2 op_c0d8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) += 2;
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_NFLG(((uae_s32)(newv)) < 0);
	count_cycles += (getMulu68kCycles(src)) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* MULU.W -(An),Dn */
uae_u32 REGPARAM2 op_c0e0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_NFLG(((uae_s32)(newv)) < 0);
	count_cycles += (getMulu68kCycles(src)) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* MULU.W (d16,An),Dn */
uae_u32 REGPARAM2 op_c0e8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_NFLG(((uae_s32)(newv)) < 0);
	count_cycles += (getMulu68kCycles(src)) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0   */

/* MULU.W (d8,An,Xn),Dn */
uae_u32 REGPARAM2 op_c0f0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_NFLG(((uae_s32)(newv)) < 0);
	count_cycles += (getMulu68kCycles(src)) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 4,0   */

/* MULU.W (xxx).W,Dn */
uae_u32 REGPARAM2 op_c0f8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_NFLG(((uae_s32)(newv)) < 0);
	count_cycles += (getMulu68kCycles(src)) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0   */

/* MULU.W (xxx).L,Dn */
uae_u32 REGPARAM2 op_c0f9_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_NFLG(((uae_s32)(newv)) < 0);
	count_cycles += (getMulu68kCycles(src)) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2 + count_cycles;
}
/* 6 0,0   */

/* MULU.W (d16,PC),Dn */
uae_u32 REGPARAM2 op_c0fa_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_NFLG(((uae_s32)(newv)) < 0);
	count_cycles += (getMulu68kCycles(src)) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0   */

/* MULU.W (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 op_c0fb_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_NFLG(((uae_s32)(newv)) < 0);
	count_cycles += (getMulu68kCycles(src)) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 4,0   */

/* MULU.W #<data>.W,Dn */
uae_u32 REGPARAM2 op_c0fc_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = get_diword (2);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_NFLG(((uae_s32)(newv)) < 0);
	count_cycles += (getMulu68kCycles(src)) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 8 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0   */

/* ABCD.B Dn,Dn */
uae_u32 REGPARAM2 op_c100_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u16 newv_lo = (src & 0xF) + (dst & 0xF) + (GET_XFLG() ? 1 : 0);
	uae_u16 newv_hi = (src & 0xF0) + (dst & 0xF0);
	uae_u16 newv, tmp_newv;
	int cflg;
	newv = tmp_newv = newv_hi + newv_lo;
if (newv_lo > 9) { newv += 6; }
	cflg = (newv & 0x3F0) > 0x90;
	if (cflg) newv += 0x60;
	SET_CFLG(cflg);
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s8)(newv)) == 0));
	SET_NFLG(((uae_s8)(newv)) < 0);
	SET_VFLG((tmp_newv & 0x80) == 0 && (newv & 0x80) != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ABCD.B -(An),-(An) */
uae_u32 REGPARAM2 op_c108_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	uae_u16 newv_lo = (src & 0xF) + (dst & 0xF) + (GET_XFLG() ? 1 : 0);
	uae_u16 newv_hi = (src & 0xF0) + (dst & 0xF0);
	uae_u16 newv, tmp_newv;
	int cflg;
	newv = tmp_newv = newv_hi + newv_lo;
if (newv_lo > 9) { newv += 6; }
	cflg = (newv & 0x3F0) > 0x90;
	if (cflg) newv += 0x60;
	SET_CFLG(cflg);
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s8)(newv)) == 0));
	SET_NFLG(((uae_s8)(newv)) < 0);
	SET_VFLG((tmp_newv & 0x80) == 0 && (newv & 0x80) != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(2);
	return 18 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* AND.B Dn,(An) */
uae_u32 REGPARAM2 op_c110_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* AND.B Dn,(An)+ */
uae_u32 REGPARAM2 op_c118_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* AND.B Dn,-(An) */
uae_u32 REGPARAM2 op_c120_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* AND.B Dn,(d16,An) */
uae_u32 REGPARAM2 op_c128_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 dst = get_byte_jit(dsta);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* AND.B Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 op_c130_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (2));
	uae_s8 dst = get_byte_jit(dsta);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* AND.B Dn,(xxx).W */
uae_u32 REGPARAM2 op_c138_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 dst = get_byte_jit(dsta);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* AND.B Dn,(xxx).L */
uae_u32 REGPARAM2 op_c139_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_dilong(2);
	uae_s8 dst = get_byte_jit(dsta);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	put_byte_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* EXG.L Dn,Dn */
uae_u32 REGPARAM2 op_c140_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	m68k_dreg(regs, srcreg) = (dst);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* EXG.L An,An */
uae_u32 REGPARAM2 op_c148_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 src = m68k_areg(regs, srcreg);
	uae_s32 dst = m68k_areg(regs, dstreg);
	m68k_areg(regs, srcreg) = (dst);
	m68k_areg(regs, dstreg) = (src);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* AND.W Dn,(An) */
uae_u32 REGPARAM2 op_c150_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s16 dst = get_word_jit(dsta);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* AND.W Dn,(An)+ */
uae_u32 REGPARAM2 op_c158_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s16 dst = get_word_jit(dsta);
	m68k_areg(regs, dstreg) += 2;
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* AND.W Dn,-(An) */
uae_u32 REGPARAM2 op_c160_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	uae_s16 dst = get_word_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* AND.W Dn,(d16,An) */
uae_u32 REGPARAM2 op_c168_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 dst = get_word_jit(dsta);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* AND.W Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 op_c170_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (2));
	uae_s16 dst = get_word_jit(dsta);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* AND.W Dn,(xxx).W */
uae_u32 REGPARAM2 op_c178_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 dst = get_word_jit(dsta);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* AND.W Dn,(xxx).L */
uae_u32 REGPARAM2 op_c179_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_dilong(2);
	uae_s16 dst = get_word_jit(dsta);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(src)) == 0);
	SET_NFLG(((uae_s16)(src)) < 0);
	put_word_jit(dsta, src);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* EXG.L Dn,An */
uae_u32 REGPARAM2 op_c188_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uae_s32 dst = m68k_areg(regs, dstreg);
	m68k_dreg(regs, srcreg) = (dst);
	m68k_areg(regs, dstreg) = (src);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* AND.L Dn,(An) */
uae_u32 REGPARAM2 op_c190_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s32 dst = get_long_jit(dsta);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(2);
	return 20 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* AND.L Dn,(An)+ */
uae_u32 REGPARAM2 op_c198_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s32 dst = get_long_jit(dsta);
	m68k_areg(regs, dstreg) += 4;
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(2);
	return 20 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* AND.L Dn,-(An) */
uae_u32 REGPARAM2 op_c1a0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	uae_s32 dst = get_long_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(2);
	return 22 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* AND.L Dn,(d16,An) */
uae_u32 REGPARAM2 op_c1a8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s32 dst = get_long_jit(dsta);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(4);
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* AND.L Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 op_c1b0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (2));
	uae_s32 dst = get_long_jit(dsta);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(4);
	return 26 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* AND.L Dn,(xxx).W */
uae_u32 REGPARAM2 op_c1b8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	uae_s32 dst = get_long_jit(dsta);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(4);
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* AND.L Dn,(xxx).L */
uae_u32 REGPARAM2 op_c1b9_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_dilong(2);
	uae_s32 dst = get_long_jit(dsta);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(6);
	return 28 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MULS.W Dn,Dn */
uae_u32 REGPARAM2 op_c1c0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_NFLG(((uae_s32)(newv)) < 0);
	count_cycles += (getMuls68kCycles(src)) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* MULS.W (An),Dn */
uae_u32 REGPARAM2 op_c1d0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_NFLG(((uae_s32)(newv)) < 0);
	count_cycles += (getMuls68kCycles(src)) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* MULS.W (An)+,Dn */
uae_u32 REGPARAM2 op_c1d8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) += 2;
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_NFLG(((uae_s32)(newv)) < 0);
	count_cycles += (getMuls68kCycles(src)) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* MULS.W -(An),Dn */
uae_u32 REGPARAM2 op_c1e0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_NFLG(((uae_s32)(newv)) < 0);
	count_cycles += (getMuls68kCycles(src)) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* MULS.W (d16,An),Dn */
uae_u32 REGPARAM2 op_c1e8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_NFLG(((uae_s32)(newv)) < 0);
	count_cycles += (getMuls68kCycles(src)) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0   */

/* MULS.W (d8,An,Xn),Dn */
uae_u32 REGPARAM2 op_c1f0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_NFLG(((uae_s32)(newv)) < 0);
	count_cycles += (getMuls68kCycles(src)) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 4,0   */

/* MULS.W (xxx).W,Dn */
uae_u32 REGPARAM2 op_c1f8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_NFLG(((uae_s32)(newv)) < 0);
	count_cycles += (getMuls68kCycles(src)) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0   */

/* MULS.W (xxx).L,Dn */
uae_u32 REGPARAM2 op_c1f9_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_NFLG(((uae_s32)(newv)) < 0);
	count_cycles += (getMuls68kCycles(src)) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2 + count_cycles;
}
/* 6 0,0   */

/* MULS.W (d16,PC),Dn */
uae_u32 REGPARAM2 op_c1fa_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_NFLG(((uae_s32)(newv)) < 0);
	count_cycles += (getMuls68kCycles(src)) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0   */

/* MULS.W (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 op_c1fb_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_NFLG(((uae_s32)(newv)) < 0);
	count_cycles += (getMuls68kCycles(src)) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 4,0   */

/* MULS.W #<data>.W,Dn */
uae_u32 REGPARAM2 op_c1fc_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = get_diword (2);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_NFLG(((uae_s32)(newv)) < 0);
	count_cycles += (getMuls68kCycles(src)) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 8 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0   */

/* ADD.B Dn,Dn */
uae_u32 REGPARAM2 op_d000_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADD.B (An),Dn */
uae_u32 REGPARAM2 op_d010_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADD.B (An)+,Dn */
uae_u32 REGPARAM2 op_d018_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADD.B -(An),Dn */
uae_u32 REGPARAM2 op_d020_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADD.B (d16,An),Dn */
uae_u32 REGPARAM2 op_d028_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADD.B (d8,An,Xn),Dn */
uae_u32 REGPARAM2 op_d030_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* ADD.B (xxx).W,Dn */
uae_u32 REGPARAM2 op_d038_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADD.B (xxx).L,Dn */
uae_u32 REGPARAM2 op_d039_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s8 src = get_byte_jit(srca);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* ADD.B (d16,PC),Dn */
uae_u32 REGPARAM2 op_d03a_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADD.B (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 op_d03b_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* ADD.B #<data>.B,Dn */
uae_u32 REGPARAM2 op_d03c_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s8 src = get_dibyte (2);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
	m68k_incpc(4);
	return 8 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADD.W Dn,Dn */
uae_u32 REGPARAM2 op_d040_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADD.W An,Dn */
uae_u32 REGPARAM2 op_d048_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = m68k_areg(regs, srcreg);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADD.W (An),Dn */
uae_u32 REGPARAM2 op_d050_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADD.W (An)+,Dn */
uae_u32 REGPARAM2 op_d058_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) += 2;
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADD.W -(An),Dn */
uae_u32 REGPARAM2 op_d060_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
	m68k_incpc(2);
	return 10 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADD.W (d16,An),Dn */
uae_u32 REGPARAM2 op_d068_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADD.W (d8,An,Xn),Dn */
uae_u32 REGPARAM2 op_d070_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* ADD.W (xxx).W,Dn */
uae_u32 REGPARAM2 op_d078_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADD.W (xxx).L,Dn */
uae_u32 REGPARAM2 op_d079_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* ADD.W (d16,PC),Dn */
uae_u32 REGPARAM2 op_d07a_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADD.W (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 op_d07b_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
	m68k_incpc(4);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* ADD.W #<data>.W,Dn */
uae_u32 REGPARAM2 op_d07c_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = get_diword (2);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
	m68k_incpc(4);
	return 8 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADD.L Dn,Dn */
uae_u32 REGPARAM2 op_d080_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADD.L An,Dn */
uae_u32 REGPARAM2 op_d088_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src = m68k_areg(regs, srcreg);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADD.L (An),Dn */
uae_u32 REGPARAM2 op_d090_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADD.L (An)+,Dn */
uae_u32 REGPARAM2 op_d098_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) += 4;
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADD.L -(An),Dn */
uae_u32 REGPARAM2 op_d0a0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 16 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADD.L (d16,An),Dn */
uae_u32 REGPARAM2 op_d0a8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADD.L (d8,An,Xn),Dn */
uae_u32 REGPARAM2 op_d0b0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* ADD.L (xxx).W,Dn */
uae_u32 REGPARAM2 op_d0b8_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADD.L (xxx).L,Dn */
uae_u32 REGPARAM2 op_d0b9_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* ADD.L (d16,PC),Dn */
uae_u32 REGPARAM2 op_d0ba_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADD.L (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 op_d0bb_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* ADD.L #<data>.L,Dn */
uae_u32 REGPARAM2 op_d0bc_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src;
	src = get_dilong(2);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(6);
	return 14 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* ADDA.W Dn,An */
uae_u32 REGPARAM2 op_d0c0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADDA.W An,An */
uae_u32 REGPARAM2 op_d0c8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = m68k_areg(regs, srcreg);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADDA.W (An),An */
uae_u32 REGPARAM2 op_d0d0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADDA.W (An)+,An */
uae_u32 REGPARAM2 op_d0d8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) += 2;
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADDA.W -(An),An */
uae_u32 REGPARAM2 op_d0e0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADDA.W (d16,An),An */
uae_u32 REGPARAM2 op_d0e8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADDA.W (d8,An,Xn),An */
uae_u32 REGPARAM2 op_d0f0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* ADDA.W (xxx).W,An */
uae_u32 REGPARAM2 op_d0f8_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADDA.W (xxx).L,An */
uae_u32 REGPARAM2 op_d0f9_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s16 src = get_word_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* ADDA.W (d16,PC),An */
uae_u32 REGPARAM2 op_d0fa_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADDA.W (d8,PC,Xn),An */
uae_u32 REGPARAM2 op_d0fb_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s16 src = get_word_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* ADDA.W #<data>.W,An */
uae_u32 REGPARAM2 op_d0fc_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = get_diword (2);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADDX.B Dn,Dn */
uae_u32 REGPARAM2 op_d100_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = dst + src + (GET_XFLG() ? 1 : 0);
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgo) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s8)(newv)) == 0));
	SET_NFLG(((uae_s8)(newv)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADDX.B -(An),-(An) */
uae_u32 REGPARAM2 op_d108_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	uae_u32 newv = dst + src + (GET_XFLG() ? 1 : 0);
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgo) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s8)(newv)) == 0));
	SET_NFLG(((uae_s8)(newv)) < 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(2);
	return 18 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADD.B Dn,(An) */
uae_u32 REGPARAM2 op_d110_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADD.B Dn,(An)+ */
uae_u32 REGPARAM2 op_d118_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADD.B Dn,-(An) */
uae_u32 REGPARAM2 op_d120_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADD.B Dn,(d16,An) */
uae_u32 REGPARAM2 op_d128_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 dst = get_byte_jit(dsta);
	uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADD.B Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 op_d130_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (2));
	uae_s8 dst = get_byte_jit(dsta);
	uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* ADD.B Dn,(xxx).W */
uae_u32 REGPARAM2 op_d138_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 dst = get_byte_jit(dsta);
	uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADD.B Dn,(xxx).L */
uae_u32 REGPARAM2 op_d139_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_dilong(2);
	uae_s8 dst = get_byte_jit(dsta);
	uae_u32 newv = ((uae_u8)(dst)) + ((uae_u8)(src));
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_ZFLG(((uae_s8)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u8)(~dst)) < ((uae_u8)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* ADDX.W Dn,Dn */
uae_u32 REGPARAM2 op_d140_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = dst + src + (GET_XFLG() ? 1 : 0);
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgo) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s16)(newv)) == 0));
	SET_NFLG(((uae_s16)(newv)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADDX.W -(An),-(An) */
uae_u32 REGPARAM2 op_d148_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	uae_s16 dst = get_word_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	uae_u32 newv = dst + src + (GET_XFLG() ? 1 : 0);
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgo) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s16)(newv)) == 0));
	SET_NFLG(((uae_s16)(newv)) < 0);
	put_word_jit(dsta, newv);
	m68k_incpc(2);
	return 18 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADD.W Dn,(An) */
uae_u32 REGPARAM2 op_d150_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s16 dst = get_word_jit(dsta);
	uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADD.W Dn,(An)+ */
uae_u32 REGPARAM2 op_d158_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s16 dst = get_word_jit(dsta);
	m68k_areg(regs, dstreg) += 2;
	uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADD.W Dn,-(An) */
uae_u32 REGPARAM2 op_d160_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	uae_s16 dst = get_word_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADD.W Dn,(d16,An) */
uae_u32 REGPARAM2 op_d168_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 dst = get_word_jit(dsta);
	uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADD.W Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 op_d170_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (2));
	uae_s16 dst = get_word_jit(dsta);
	uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* ADD.W Dn,(xxx).W */
uae_u32 REGPARAM2 op_d178_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 dst = get_word_jit(dsta);
	uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADD.W Dn,(xxx).L */
uae_u32 REGPARAM2 op_d179_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_dilong(2);
	uae_s16 dst = get_word_jit(dsta);
	uae_u32 newv = ((uae_u16)(dst)) + ((uae_u16)(src));
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_ZFLG(((uae_s16)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u16)(~dst)) < ((uae_u16)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_word_jit(dsta, newv);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* ADDX.L Dn,Dn */
uae_u32 REGPARAM2 op_d180_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = dst + src + (GET_XFLG() ? 1 : 0);
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgo) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s32)(newv)) == 0));
	SET_NFLG(((uae_s32)(newv)) < 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADDX.L -(An),-(An) */
uae_u32 REGPARAM2 op_d188_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	uae_s32 dst = get_long_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	uae_u32 newv = dst + src + (GET_XFLG() ? 1 : 0);
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgo) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s32)(newv)) == 0));
	SET_NFLG(((uae_s32)(newv)) < 0);
	put_long_jit(dsta, newv);
	m68k_incpc(2);
	return 30 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADD.L Dn,(An) */
uae_u32 REGPARAM2 op_d190_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s32 dst = get_long_jit(dsta);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(2);
	return 20 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADD.L Dn,(An)+ */
uae_u32 REGPARAM2 op_d198_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s32 dst = get_long_jit(dsta);
	m68k_areg(regs, dstreg) += 4;
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(2);
	return 20 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADD.L Dn,-(An) */
uae_u32 REGPARAM2 op_d1a0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	uae_s32 dst = get_long_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(2);
	return 22 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADD.L Dn,(d16,An) */
uae_u32 REGPARAM2 op_d1a8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s32 dst = get_long_jit(dsta);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(4);
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADD.L Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 op_d1b0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (2));
	uae_s32 dst = get_long_jit(dsta);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(4);
	return 26 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* ADD.L Dn,(xxx).W */
uae_u32 REGPARAM2 op_d1b8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	uae_s32 dst = get_long_jit(dsta);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(4);
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADD.L Dn,(xxx).L */
uae_u32 REGPARAM2 op_d1b9_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_dilong(2);
	uae_s32 dst = get_long_jit(dsta);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	put_long_jit(dsta, newv);
	m68k_incpc(6);
	return 28 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* ADDA.L Dn,An */
uae_u32 REGPARAM2 op_d1c0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADDA.L An,An */
uae_u32 REGPARAM2 op_d1c8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src = m68k_areg(regs, srcreg);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADDA.L (An),An */
uae_u32 REGPARAM2 op_d1d0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADDA.L (An)+,An */
uae_u32 REGPARAM2 op_d1d8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) += 4;
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADDA.L -(An),An */
uae_u32 REGPARAM2 op_d1e0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 16 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADDA.L (d16,An),An */
uae_u32 REGPARAM2 op_d1e8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADDA.L (d8,An,Xn),An */
uae_u32 REGPARAM2 op_d1f0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* ADDA.L (xxx).W,An */
uae_u32 REGPARAM2 op_d1f8_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADDA.L (xxx).L,An */
uae_u32 REGPARAM2 op_d1f9_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* ADDA.L (d16,PC),An */
uae_u32 REGPARAM2 op_d1fa_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADDA.L (d8,PC,Xn),An */
uae_u32 REGPARAM2 op_d1fb_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* ADDA.L #<data>.L,An */
uae_u32 REGPARAM2 op_d1fc_44_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src;
	src = get_dilong(2);
	uae_s32 dst = m68k_areg(regs, dstreg);
	uae_u32 newv = dst + src;
	m68k_areg(regs, dstreg) = (newv);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* ASRQ.B #<data>,Dn */
uae_u32 REGPARAM2 op_e000_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
	uae_s8 data = m68k_dreg(regs, dstreg);
	uae_u32 val = (uae_u8)data;
	CLEAR_CZNV();
	uae_u32 sign = (0x80 & val) >> 7;
	int ccnt = cnt & 63;
	cnt &= 63;
	if (cnt >= 8) {
		val = 0xff & (uae_u32)(0 - sign);
		SET_CFLG(sign);
		COPY_CARRY();
	} else {
		val >>= cnt - 1;
		SET_CFLG(val & 1);
		COPY_CARRY();
		val >>= 1;
		val |= (0xff << (8 - cnt)) & (uae_u32)(0 - sign);
		val &= 0xff;
	}
	SET_ZFLG(((uae_s8)(val)) == 0);
	SET_NFLG(((uae_s8)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((val) & 0xff);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* LSRQ.B #<data>,Dn */
uae_u32 REGPARAM2 op_e008_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
	uae_s8 data = m68k_dreg(regs, dstreg);
	uae_u32 val = (uae_u8)data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	if (cnt >= 8) {
		SET_CFLG((cnt == 8) & (val >> 7));
		COPY_CARRY();
		val = 0;
	} else {
		val >>= cnt - 1;
		SET_CFLG(val & 1);
		COPY_CARRY();
		val >>= 1;
	}
	SET_ZFLG(((uae_s8)(val)) == 0);
	SET_NFLG(((uae_s8)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((val) & 0xff);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* ROXRQ.B #<data>,Dn */
uae_u32 REGPARAM2 op_e010_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
	uae_s8 data = m68k_dreg(regs, dstreg);
	uae_u32 val = (uae_u8)data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	{
		cnt--;
		{
			uae_u32 carry;
			uae_u32 hival = (val << 1) | GET_XFLG();
			hival <<= (7 - cnt);
			val >>= cnt;
			carry = val & 1;
			val >>= 1;
			val |= hival;
			SET_XFLG(carry);
			val &= 0xff;
		}
	}
	SET_CFLG(GET_XFLG());
	SET_ZFLG(((uae_s8)(val)) == 0);
	SET_NFLG(((uae_s8)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((val) & 0xff);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* RORQ.B #<data>,Dn */
uae_u32 REGPARAM2 op_e018_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
	uae_s8 data = m68k_dreg(regs, dstreg);
	uae_u32 val = (uae_u8)data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	{
		uae_u32 hival;
		cnt &= 7;
		hival = val << (8 - cnt);
		val >>= cnt;
		val |= hival;
		val &= 0xff;
		SET_CFLG((val & 0x80) >> 7);
	}
	SET_ZFLG(((uae_s8)(val)) == 0);
	SET_NFLG(((uae_s8)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((val) & 0xff);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* ASR.B Dn,Dn */
uae_u32 REGPARAM2 op_e020_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 cnt = m68k_dreg(regs, srcreg);
	uae_s8 data = m68k_dreg(regs, dstreg);
	uae_u32 val = (uae_u8)data;
	CLEAR_CZNV();
	uae_u32 sign = (0x80 & val) >> 7;
	int ccnt = cnt & 63;
	cnt &= 63;
	if (cnt >= 8) {
		val = 0xff & (uae_u32)(0 - sign);
		SET_CFLG(sign);
		COPY_CARRY();
	} else if (cnt > 0) {
		val >>= cnt - 1;
		SET_CFLG(val & 1);
		COPY_CARRY();
		val >>= 1;
		val |= (0xff << (8 - cnt)) & (uae_u32)(0 - sign);
		val &= 0xff;
	}
	SET_ZFLG(((uae_s8)(val)) == 0);
	SET_NFLG(((uae_s8)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((val) & 0xff);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* LSR.B Dn,Dn */
uae_u32 REGPARAM2 op_e028_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 cnt = m68k_dreg(regs, srcreg);
	uae_s8 data = m68k_dreg(regs, dstreg);
	uae_u32 val = (uae_u8)data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	if (cnt >= 8) {
		SET_CFLG((cnt == 8) & (val >> 7));
		COPY_CARRY();
		val = 0;
	} else if (cnt > 0) {
		val >>= cnt - 1;
		SET_CFLG(val & 1);
		COPY_CARRY();
		val >>= 1;
	}
	SET_ZFLG(((uae_s8)(val)) == 0);
	SET_NFLG(((uae_s8)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((val) & 0xff);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* ROXR.B Dn,Dn */
uae_u32 REGPARAM2 op_e030_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 cnt = m68k_dreg(regs, srcreg);
	uae_s8 data = m68k_dreg(regs, dstreg);
	uae_u32 val = (uae_u8)data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	if (cnt >= 36) cnt -= 36;
	if (cnt >= 18) cnt -= 18;
	if (cnt >= 9) cnt -= 9;
	if (cnt > 0) {
		cnt--;
		{
			uae_u32 carry;
			uae_u32 hival = (val << 1) | GET_XFLG();
			hival <<= (7 - cnt);
			val >>= cnt;
			carry = val & 1;
			val >>= 1;
			val |= hival;
			SET_XFLG(carry);
			val &= 0xff;
		}
	}
	SET_CFLG(GET_XFLG());
	SET_ZFLG(((uae_s8)(val)) == 0);
	SET_NFLG(((uae_s8)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((val) & 0xff);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* ROR.B Dn,Dn */
uae_u32 REGPARAM2 op_e038_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 cnt = m68k_dreg(regs, srcreg);
	uae_s8 data = m68k_dreg(regs, dstreg);
	uae_u32 val = (uae_u8)data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	if (cnt > 0) {
		uae_u32 hival;
		cnt &= 7;
		hival = val << (8 - cnt);
		val >>= cnt;
		val |= hival;
		val &= 0xff;
		SET_CFLG((val & 0x80) >> 7);
	}
	SET_ZFLG(((uae_s8)(val)) == 0);
	SET_NFLG(((uae_s8)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((val) & 0xff);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* ASRQ.W #<data>,Dn */
uae_u32 REGPARAM2 op_e040_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
	uae_s16 data = m68k_dreg(regs, dstreg);
	uae_u32 val = (uae_u16)data;
	CLEAR_CZNV();
	uae_u32 sign = (0x8000 & val) >> 15;
	int ccnt = cnt & 63;
	cnt &= 63;
	if (cnt >= 16) {
		val = 0xffff & (uae_u32)(0 - sign);
		SET_CFLG(sign);
		COPY_CARRY();
	} else {
		val >>= cnt - 1;
		SET_CFLG(val & 1);
		COPY_CARRY();
		val >>= 1;
		val |= (0xffff << (16 - cnt)) & (uae_u32)(0 - sign);
		val &= 0xffff;
	}
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((val) & 0xffff);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* LSRQ.W #<data>,Dn */
uae_u32 REGPARAM2 op_e048_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
	uae_s16 data = m68k_dreg(regs, dstreg);
	uae_u32 val = (uae_u16)data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	if (cnt >= 16) {
		SET_CFLG((cnt == 16) & (val >> 15));
		COPY_CARRY();
		val = 0;
	} else {
		val >>= cnt - 1;
		SET_CFLG(val & 1);
		COPY_CARRY();
		val >>= 1;
	}
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((val) & 0xffff);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* ROXRQ.W #<data>,Dn */
uae_u32 REGPARAM2 op_e050_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
	uae_s16 data = m68k_dreg(regs, dstreg);
	uae_u32 val = (uae_u16)data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	{
		cnt--;
		{
			uae_u32 carry;
			uae_u32 hival = (val << 1) | GET_XFLG();
			hival <<= (15 - cnt);
			val >>= cnt;
			carry = val & 1;
			val >>= 1;
			val |= hival;
			SET_XFLG(carry);
			val &= 0xffff;
		}
	}
	SET_CFLG(GET_XFLG());
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((val) & 0xffff);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* RORQ.W #<data>,Dn */
uae_u32 REGPARAM2 op_e058_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
	uae_s16 data = m68k_dreg(regs, dstreg);
	uae_u32 val = (uae_u16)data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	{
		uae_u32 hival;
		cnt &= 15;
		hival = val << (16 - cnt);
		val >>= cnt;
		val |= hival;
		val &= 0xffff;
		SET_CFLG((val & 0x8000) >> 15);
	}
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((val) & 0xffff);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* ASR.W Dn,Dn */
uae_u32 REGPARAM2 op_e060_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 cnt = m68k_dreg(regs, srcreg);
	uae_s16 data = m68k_dreg(regs, dstreg);
	uae_u32 val = (uae_u16)data;
	CLEAR_CZNV();
	uae_u32 sign = (0x8000 & val) >> 15;
	int ccnt = cnt & 63;
	cnt &= 63;
	if (cnt >= 16) {
		val = 0xffff & (uae_u32)(0 - sign);
		SET_CFLG(sign);
		COPY_CARRY();
	} else if (cnt > 0) {
		val >>= cnt - 1;
		SET_CFLG(val & 1);
		COPY_CARRY();
		val >>= 1;
		val |= (0xffff << (16 - cnt)) & (uae_u32)(0 - sign);
		val &= 0xffff;
	}
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((val) & 0xffff);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* LSR.W Dn,Dn */
uae_u32 REGPARAM2 op_e068_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 cnt = m68k_dreg(regs, srcreg);
	uae_s16 data = m68k_dreg(regs, dstreg);
	uae_u32 val = (uae_u16)data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	if (cnt >= 16) {
		SET_CFLG((cnt == 16) & (val >> 15));
		COPY_CARRY();
		val = 0;
	} else if (cnt > 0) {
		val >>= cnt - 1;
		SET_CFLG(val & 1);
		COPY_CARRY();
		val >>= 1;
	}
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((val) & 0xffff);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* ROXR.W Dn,Dn */
uae_u32 REGPARAM2 op_e070_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 cnt = m68k_dreg(regs, srcreg);
	uae_s16 data = m68k_dreg(regs, dstreg);
	uae_u32 val = (uae_u16)data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	if (cnt >= 34) cnt -= 34;
	if (cnt >= 17) cnt -= 17;
	if (cnt > 0) {
		cnt--;
		{
			uae_u32 carry;
			uae_u32 hival = (val << 1) | GET_XFLG();
			hival <<= (15 - cnt);
			val >>= cnt;
			carry = val & 1;
			val >>= 1;
			val |= hival;
			SET_XFLG(carry);
			val &= 0xffff;
		}
	}
	SET_CFLG(GET_XFLG());
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((val) & 0xffff);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* ROR.W Dn,Dn */
uae_u32 REGPARAM2 op_e078_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 cnt = m68k_dreg(regs, srcreg);
	uae_s16 data = m68k_dreg(regs, dstreg);
	uae_u32 val = (uae_u16)data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	if (cnt > 0) {
		uae_u32 hival;
		cnt &= 15;
		hival = val << (16 - cnt);
		val >>= cnt;
		val |= hival;
		val &= 0xffff;
		SET_CFLG((val & 0x8000) >> 15);
	}
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((val) & 0xffff);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* ASRQ.L #<data>,Dn */
uae_u32 REGPARAM2 op_e080_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
	uae_s32 data = m68k_dreg(regs, dstreg);
	uae_u32 val = data;
	CLEAR_CZNV();
	uae_u32 sign = (0x80000000 & val) >> 31;
	int ccnt = cnt & 63;
	cnt &= 63;
	if (cnt >= 32) {
		val = 0xffffffff & (uae_u32)(0 - sign);
		SET_CFLG(sign);
		COPY_CARRY();
	} else {
		val >>= cnt - 1;
		SET_CFLG(val & 1);
		COPY_CARRY();
		val >>= 1;
		val |= (0xffffffff << (32 - cnt)) & (uae_u32)(0 - sign);
		val &= 0xffffffff;
	}
	SET_ZFLG(((uae_s32)(val)) == 0);
	SET_NFLG(((uae_s32)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (val);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* LSRQ.L #<data>,Dn */
uae_u32 REGPARAM2 op_e088_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
	uae_s32 data = m68k_dreg(regs, dstreg);
	uae_u32 val = data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	if (cnt >= 32) {
		SET_CFLG((cnt == 32) & (val >> 31));
		COPY_CARRY();
		val = 0;
	} else {
		val >>= cnt - 1;
		SET_CFLG(val & 1);
		COPY_CARRY();
		val >>= 1;
	}
	SET_ZFLG(((uae_s32)(val)) == 0);
	SET_NFLG(((uae_s32)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (val);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* ROXRQ.L #<data>,Dn */
uae_u32 REGPARAM2 op_e090_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
	uae_s32 data = m68k_dreg(regs, dstreg);
	uae_u32 val = data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	{
		cnt--;
		{
			uae_u32 carry;
			uae_u32 hival = (val << 1) | GET_XFLG();
			hival <<= (31 - cnt);
			val >>= cnt;
			carry = val & 1;
			val >>= 1;
			val |= hival;
			SET_XFLG(carry);
			val &= 0xffffffff;
		}
	}
	SET_CFLG(GET_XFLG());
	SET_ZFLG(((uae_s32)(val)) == 0);
	SET_NFLG(((uae_s32)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (val);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* RORQ.L #<data>,Dn */
uae_u32 REGPARAM2 op_e098_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
	uae_s32 data = m68k_dreg(regs, dstreg);
	uae_u32 val = data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	{
		uae_u32 hival;
		cnt &= 31;
		hival = val << (32 - cnt);
		val >>= cnt;
		val |= hival;
		val &= 0xffffffff;
		SET_CFLG((val & 0x80000000) >> 31);
	}
	SET_ZFLG(((uae_s32)(val)) == 0);
	SET_NFLG(((uae_s32)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (val);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* ASR.L Dn,Dn */
uae_u32 REGPARAM2 op_e0a0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 cnt = m68k_dreg(regs, srcreg);
	uae_s32 data = m68k_dreg(regs, dstreg);
	uae_u32 val = data;
	CLEAR_CZNV();
	uae_u32 sign = (0x80000000 & val) >> 31;
	int ccnt = cnt & 63;
	cnt &= 63;
	if (cnt >= 32) {
		val = 0xffffffff & (uae_u32)(0 - sign);
		SET_CFLG(sign);
		COPY_CARRY();
	} else if (cnt > 0) {
		val >>= cnt - 1;
		SET_CFLG(val & 1);
		COPY_CARRY();
		val >>= 1;
		val |= (0xffffffff << (32 - cnt)) & (uae_u32)(0 - sign);
		val &= 0xffffffff;
	}
	SET_ZFLG(((uae_s32)(val)) == 0);
	SET_NFLG(((uae_s32)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (val);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* LSR.L Dn,Dn */
uae_u32 REGPARAM2 op_e0a8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 cnt = m68k_dreg(regs, srcreg);
	uae_s32 data = m68k_dreg(regs, dstreg);
	uae_u32 val = data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	if (cnt >= 32) {
		SET_CFLG((cnt == 32) & (val >> 31));
		COPY_CARRY();
		val = 0;
	} else if (cnt > 0) {
		val >>= cnt - 1;
		SET_CFLG(val & 1);
		COPY_CARRY();
		val >>= 1;
	}
	SET_ZFLG(((uae_s32)(val)) == 0);
	SET_NFLG(((uae_s32)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (val);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* ROXR.L Dn,Dn */
uae_u32 REGPARAM2 op_e0b0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 cnt = m68k_dreg(regs, srcreg);
	uae_s32 data = m68k_dreg(regs, dstreg);
	uae_u32 val = data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	if (cnt >= 33) cnt -= 33;
	if (cnt > 0) {
		cnt--;
		{
			uae_u32 carry;
			uae_u32 hival = (val << 1) | GET_XFLG();
			hival <<= (31 - cnt);
			val >>= cnt;
			carry = val & 1;
			val >>= 1;
			val |= hival;
			SET_XFLG(carry);
			val &= 0xffffffff;
		}
	}
	SET_CFLG(GET_XFLG());
	SET_ZFLG(((uae_s32)(val)) == 0);
	SET_NFLG(((uae_s32)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (val);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* ROR.L Dn,Dn */
uae_u32 REGPARAM2 op_e0b8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 cnt = m68k_dreg(regs, srcreg);
	uae_s32 data = m68k_dreg(regs, dstreg);
	uae_u32 val = data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	if (cnt > 0) {
		uae_u32 hival;
		cnt &= 31;
		hival = val << (32 - cnt);
		val >>= cnt;
		val |= hival;
		val &= 0xffffffff;
		SET_CFLG((val & 0x80000000) >> 31);
	}
	SET_ZFLG(((uae_s32)(val)) == 0);
	SET_NFLG(((uae_s32)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (val);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* ASRW.W (An) */
uae_u32 REGPARAM2 op_e0d0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr dataa;
	dataa = m68k_areg(regs, srcreg);
	uae_s16 data = get_word_jit(dataa);
	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 cflg = val & 1;
	val = (val >> 1) | sign;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(cflg);
	COPY_CARRY();
	put_word_jit(dataa, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ASRW.W (An)+ */
uae_u32 REGPARAM2 op_e0d8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr dataa;
	dataa = m68k_areg(regs, srcreg);
	uae_s16 data = get_word_jit(dataa);
	m68k_areg(regs, srcreg) += 2;
	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 cflg = val & 1;
	val = (val >> 1) | sign;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(cflg);
	COPY_CARRY();
	put_word_jit(dataa, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ASRW.W -(An) */
uae_u32 REGPARAM2 op_e0e0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr dataa;
	dataa = m68k_areg(regs, srcreg) - 2;
	uae_s16 data = get_word_jit(dataa);
	m68k_areg(regs, srcreg) = dataa;
	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 cflg = val & 1;
	val = (val >> 1) | sign;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(cflg);
	COPY_CARRY();
	put_word_jit(dataa, val);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ASRW.W (d16,An) */
uae_u32 REGPARAM2 op_e0e8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr dataa;
	dataa = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 data = get_word_jit(dataa);
	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 cflg = val & 1;
	val = (val >> 1) | sign;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(cflg);
	COPY_CARRY();
	put_word_jit(dataa, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ASRW.W (d8,An,Xn) */
uae_u32 REGPARAM2 op_e0f0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr dataa;
	dataa = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s16 data = get_word_jit(dataa);
	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 cflg = val & 1;
	val = (val >> 1) | sign;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(cflg);
	COPY_CARRY();
	put_word_jit(dataa, val);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* ASRW.W (xxx).W */
uae_u32 REGPARAM2 op_e0f8_44_ff(uae_u32 opcode)
{
	uaecptr dataa;
	dataa = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 data = get_word_jit(dataa);
	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 cflg = val & 1;
	val = (val >> 1) | sign;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(cflg);
	COPY_CARRY();
	put_word_jit(dataa, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ASRW.W (xxx).L */
uae_u32 REGPARAM2 op_e0f9_44_ff(uae_u32 opcode)
{
	uaecptr dataa;
	dataa = get_dilong(2);
	uae_s16 data = get_word_jit(dataa);
	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 cflg = val & 1;
	val = (val >> 1) | sign;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(cflg);
	COPY_CARRY();
	put_word_jit(dataa, val);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* ASLQ.B #<data>,Dn */
uae_u32 REGPARAM2 op_e100_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
	uae_s8 data = m68k_dreg(regs, dstreg);
	uae_u32 val = (uae_u8)data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	if (cnt >= 8) {
		SET_VFLG(val != 0);
		SET_CFLG(cnt == 8 ? val & 1 : 0);
		COPY_CARRY();
		val = 0;
	} else {
		uae_u32 mask = (0xff << (7 - cnt)) & 0xff;
		SET_VFLG((val & mask) != mask && (val & mask) != 0);
		val <<= cnt - 1;
		SET_CFLG((val & 0x80) >> 7);
		COPY_CARRY();
		val <<= 1;
		val &= 0xff;
	}
	SET_ZFLG(((uae_s8)(val)) == 0);
	SET_NFLG(((uae_s8)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((val) & 0xff);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* LSLQ.B #<data>,Dn */
uae_u32 REGPARAM2 op_e108_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
	uae_s8 data = m68k_dreg(regs, dstreg);
	uae_u32 val = (uae_u8)data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	if (cnt >= 8) {
		SET_CFLG(cnt == 8 ? val & 1 : 0);
		COPY_CARRY();
		val = 0;
	} else {
		val <<= (cnt - 1);
		SET_CFLG((val & 0x80) >> 7);
		COPY_CARRY();
		val <<= 1;
		val &= 0xff;
	}
	SET_ZFLG(((uae_s8)(val)) == 0);
	SET_NFLG(((uae_s8)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((val) & 0xff);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* ROXLQ.B #<data>,Dn */
uae_u32 REGPARAM2 op_e110_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
	uae_s8 data = m68k_dreg(regs, dstreg);
	uae_u32 val = (uae_u8)data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	{
		cnt--;
		{
			uae_u32 carry;
			uae_u32 loval = val >> (7 - cnt);
			carry = loval & 1;
			val = (((val << 1) | GET_XFLG()) << cnt) | (loval >> 1);
			SET_XFLG(carry);
			val &= 0xff;
		}
	}
	SET_CFLG(GET_XFLG());
	SET_ZFLG(((uae_s8)(val)) == 0);
	SET_NFLG(((uae_s8)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((val) & 0xff);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* ROLQ.B #<data>,Dn */
uae_u32 REGPARAM2 op_e118_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
	uae_s8 data = m68k_dreg(regs, dstreg);
	uae_u32 val = (uae_u8)data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	{
		uae_u32 loval;
		cnt &= 7;
		loval = val >> (8 - cnt);
		val <<= cnt;
		val |= loval;
		val &= 0xff;
		SET_CFLG(val & 1);
	}
	SET_ZFLG(((uae_s8)(val)) == 0);
	SET_NFLG(((uae_s8)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((val) & 0xff);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* ASL.B Dn,Dn */
uae_u32 REGPARAM2 op_e120_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 cnt = m68k_dreg(regs, srcreg);
	uae_s8 data = m68k_dreg(regs, dstreg);
	uae_u32 val = (uae_u8)data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	if (cnt >= 8) {
		SET_VFLG(val != 0);
		SET_CFLG(cnt == 8 ? val & 1 : 0);
		COPY_CARRY();
		val = 0;
	} else if (cnt > 0) {
		uae_u32 mask = (0xff << (7 - cnt)) & 0xff;
		SET_VFLG((val & mask) != mask && (val & mask) != 0);
		val <<= cnt - 1;
		SET_CFLG((val & 0x80) >> 7);
		COPY_CARRY();
		val <<= 1;
		val &= 0xff;
	}
	SET_ZFLG(((uae_s8)(val)) == 0);
	SET_NFLG(((uae_s8)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((val) & 0xff);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* LSL.B Dn,Dn */
uae_u32 REGPARAM2 op_e128_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 cnt = m68k_dreg(regs, srcreg);
	uae_s8 data = m68k_dreg(regs, dstreg);
	uae_u32 val = (uae_u8)data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	if (cnt >= 8) {
		SET_CFLG(cnt == 8 ? val & 1 : 0);
		COPY_CARRY();
		val = 0;
	} else if (cnt > 0) {
		val <<= (cnt - 1);
		SET_CFLG((val & 0x80) >> 7);
		COPY_CARRY();
		val <<= 1;
		val &= 0xff;
	}
	SET_ZFLG(((uae_s8)(val)) == 0);
	SET_NFLG(((uae_s8)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((val) & 0xff);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* ROXL.B Dn,Dn */
uae_u32 REGPARAM2 op_e130_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 cnt = m68k_dreg(regs, srcreg);
	uae_s8 data = m68k_dreg(regs, dstreg);
	uae_u32 val = (uae_u8)data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	if (cnt >= 36) cnt -= 36;
	if (cnt >= 18) cnt -= 18;
	if (cnt >= 9) cnt -= 9;
	if (cnt > 0) {
		cnt--;
		{
			uae_u32 carry;
			uae_u32 loval = val >> (7 - cnt);
			carry = loval & 1;
			val = (((val << 1) | GET_XFLG()) << cnt) | (loval >> 1);
			SET_XFLG(carry);
			val &= 0xff;
		}
	}
	SET_CFLG(GET_XFLG());
	SET_ZFLG(((uae_s8)(val)) == 0);
	SET_NFLG(((uae_s8)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((val) & 0xff);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* ROL.B Dn,Dn */
uae_u32 REGPARAM2 op_e138_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 cnt = m68k_dreg(regs, srcreg);
	uae_s8 data = m68k_dreg(regs, dstreg);
	uae_u32 val = (uae_u8)data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	if (cnt > 0) {
		uae_u32 loval;
		cnt &= 7;
		loval = val >> (8 - cnt);
		val <<= cnt;
		val |= loval;
		val &= 0xff;
		SET_CFLG(val & 1);
	}
	SET_ZFLG(((uae_s8)(val)) == 0);
	SET_NFLG(((uae_s8)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((val) & 0xff);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* ASLQ.W #<data>,Dn */
uae_u32 REGPARAM2 op_e140_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
	uae_s16 data = m68k_dreg(regs, dstreg);
	uae_u32 val = (uae_u16)data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	if (cnt >= 16) {
		SET_VFLG(val != 0);
		SET_CFLG(cnt == 16 ? val & 1 : 0);
		COPY_CARRY();
		val = 0;
	} else {
		uae_u32 mask = (0xffff << (15 - cnt)) & 0xffff;
		SET_VFLG((val & mask) != mask && (val & mask) != 0);
		val <<= cnt - 1;
		SET_CFLG((val & 0x8000) >> 15);
		COPY_CARRY();
		val <<= 1;
		val &= 0xffff;
	}
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((val) & 0xffff);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* LSLQ.W #<data>,Dn */
uae_u32 REGPARAM2 op_e148_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
	uae_s16 data = m68k_dreg(regs, dstreg);
	uae_u32 val = (uae_u16)data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	if (cnt >= 16) {
		SET_CFLG(cnt == 16 ? val & 1 : 0);
		COPY_CARRY();
		val = 0;
	} else {
		val <<= (cnt - 1);
		SET_CFLG((val & 0x8000) >> 15);
		COPY_CARRY();
		val <<= 1;
		val &= 0xffff;
	}
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((val) & 0xffff);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* ROXLQ.W #<data>,Dn */
uae_u32 REGPARAM2 op_e150_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
	uae_s16 data = m68k_dreg(regs, dstreg);
	uae_u32 val = (uae_u16)data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	{
		cnt--;
		{
			uae_u32 carry;
			uae_u32 loval = val >> (15 - cnt);
			carry = loval & 1;
			val = (((val << 1) | GET_XFLG()) << cnt) | (loval >> 1);
			SET_XFLG(carry);
			val &= 0xffff;
		}
	}
	SET_CFLG(GET_XFLG());
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((val) & 0xffff);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* ROLQ.W #<data>,Dn */
uae_u32 REGPARAM2 op_e158_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
	uae_s16 data = m68k_dreg(regs, dstreg);
	uae_u32 val = (uae_u16)data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	{
		uae_u32 loval;
		cnt &= 15;
		loval = val >> (16 - cnt);
		val <<= cnt;
		val |= loval;
		val &= 0xffff;
		SET_CFLG(val & 1);
	}
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((val) & 0xffff);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* ASL.W Dn,Dn */
uae_u32 REGPARAM2 op_e160_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 cnt = m68k_dreg(regs, srcreg);
	uae_s16 data = m68k_dreg(regs, dstreg);
	uae_u32 val = (uae_u16)data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	if (cnt >= 16) {
		SET_VFLG(val != 0);
		SET_CFLG(cnt == 16 ? val & 1 : 0);
		COPY_CARRY();
		val = 0;
	} else if (cnt > 0) {
		uae_u32 mask = (0xffff << (15 - cnt)) & 0xffff;
		SET_VFLG((val & mask) != mask && (val & mask) != 0);
		val <<= cnt - 1;
		SET_CFLG((val & 0x8000) >> 15);
		COPY_CARRY();
		val <<= 1;
		val &= 0xffff;
	}
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((val) & 0xffff);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* LSL.W Dn,Dn */
uae_u32 REGPARAM2 op_e168_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 cnt = m68k_dreg(regs, srcreg);
	uae_s16 data = m68k_dreg(regs, dstreg);
	uae_u32 val = (uae_u16)data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	if (cnt >= 16) {
		SET_CFLG(cnt == 16 ? val & 1 : 0);
		COPY_CARRY();
		val = 0;
	} else if (cnt > 0) {
		val <<= (cnt - 1);
		SET_CFLG((val & 0x8000) >> 15);
		COPY_CARRY();
		val <<= 1;
		val &= 0xffff;
	}
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((val) & 0xffff);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* ROXL.W Dn,Dn */
uae_u32 REGPARAM2 op_e170_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 cnt = m68k_dreg(regs, srcreg);
	uae_s16 data = m68k_dreg(regs, dstreg);
	uae_u32 val = (uae_u16)data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	if (cnt >= 34) cnt -= 34;
	if (cnt >= 17) cnt -= 17;
	if (cnt > 0) {
		cnt--;
		{
			uae_u32 carry;
			uae_u32 loval = val >> (15 - cnt);
			carry = loval & 1;
			val = (((val << 1) | GET_XFLG()) << cnt) | (loval >> 1);
			SET_XFLG(carry);
			val &= 0xffff;
		}
	}
	SET_CFLG(GET_XFLG());
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((val) & 0xffff);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* ROL.W Dn,Dn */
uae_u32 REGPARAM2 op_e178_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 cnt = m68k_dreg(regs, srcreg);
	uae_s16 data = m68k_dreg(regs, dstreg);
	uae_u32 val = (uae_u16)data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	if (cnt > 0) {
		uae_u32 loval;
		cnt &= 15;
		loval = val >> (16 - cnt);
		val <<= cnt;
		val |= loval;
		val &= 0xffff;
		SET_CFLG(val & 1);
	}
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((val) & 0xffff);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* ASLQ.L #<data>,Dn */
uae_u32 REGPARAM2 op_e180_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
	uae_s32 data = m68k_dreg(regs, dstreg);
	uae_u32 val = data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	if (cnt >= 32) {
		SET_VFLG(val != 0);
		SET_CFLG(cnt == 32 ? val & 1 : 0);
		COPY_CARRY();
		val = 0;
	} else {
		uae_u32 mask = (0xffffffff << (31 - cnt)) & 0xffffffff;
		SET_VFLG((val & mask) != mask && (val & mask) != 0);
		val <<= cnt - 1;
		SET_CFLG((val & 0x80000000) >> 31);
		COPY_CARRY();
		val <<= 1;
		val &= 0xffffffff;
	}
	SET_ZFLG(((uae_s32)(val)) == 0);
	SET_NFLG(((uae_s32)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (val);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* LSLQ.L #<data>,Dn */
uae_u32 REGPARAM2 op_e188_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
	uae_s32 data = m68k_dreg(regs, dstreg);
	uae_u32 val = data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	if (cnt >= 32) {
		SET_CFLG(cnt == 32 ? val & 1 : 0);
		COPY_CARRY();
		val = 0;
	} else {
		val <<= (cnt - 1);
		SET_CFLG((val & 0x80000000) >> 31);
		COPY_CARRY();
		val <<= 1;
		val &= 0xffffffff;
	}
	SET_ZFLG(((uae_s32)(val)) == 0);
	SET_NFLG(((uae_s32)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (val);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* ROXLQ.L #<data>,Dn */
uae_u32 REGPARAM2 op_e190_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
	uae_s32 data = m68k_dreg(regs, dstreg);
	uae_u32 val = data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	{
		cnt--;
		{
			uae_u32 carry;
			uae_u32 loval = val >> (31 - cnt);
			carry = loval & 1;
			val = (((val << 1) | GET_XFLG()) << cnt) | (loval >> 1);
			SET_XFLG(carry);
			val &= 0xffffffff;
		}
	}
	SET_CFLG(GET_XFLG());
	SET_ZFLG(((uae_s32)(val)) == 0);
	SET_NFLG(((uae_s32)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (val);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* ROLQ.L #<data>,Dn */
uae_u32 REGPARAM2 op_e198_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 cnt = srcreg;
	uae_s32 data = m68k_dreg(regs, dstreg);
	uae_u32 val = data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	{
		uae_u32 loval;
		cnt &= 31;
		loval = val >> (32 - cnt);
		val <<= cnt;
		val |= loval;
		val &= 0xffffffff;
		SET_CFLG(val & 1);
	}
	SET_ZFLG(((uae_s32)(val)) == 0);
	SET_NFLG(((uae_s32)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (val);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* ASL.L Dn,Dn */
uae_u32 REGPARAM2 op_e1a0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 cnt = m68k_dreg(regs, srcreg);
	uae_s32 data = m68k_dreg(regs, dstreg);
	uae_u32 val = data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	if (cnt >= 32) {
		SET_VFLG(val != 0);
		SET_CFLG(cnt == 32 ? val & 1 : 0);
		COPY_CARRY();
		val = 0;
	} else if (cnt > 0) {
		uae_u32 mask = (0xffffffff << (31 - cnt)) & 0xffffffff;
		SET_VFLG((val & mask) != mask && (val & mask) != 0);
		val <<= cnt - 1;
		SET_CFLG((val & 0x80000000) >> 31);
		COPY_CARRY();
		val <<= 1;
		val &= 0xffffffff;
	}
	SET_ZFLG(((uae_s32)(val)) == 0);
	SET_NFLG(((uae_s32)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (val);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* LSL.L Dn,Dn */
uae_u32 REGPARAM2 op_e1a8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 cnt = m68k_dreg(regs, srcreg);
	uae_s32 data = m68k_dreg(regs, dstreg);
	uae_u32 val = data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	if (cnt >= 32) {
		SET_CFLG(cnt == 32 ? val & 1 : 0);
		COPY_CARRY();
		val = 0;
	} else if (cnt > 0) {
		val <<= (cnt - 1);
		SET_CFLG((val & 0x80000000) >> 31);
		COPY_CARRY();
		val <<= 1;
		val &= 0xffffffff;
	}
	SET_ZFLG(((uae_s32)(val)) == 0);
	SET_NFLG(((uae_s32)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (val);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* ROXL.L Dn,Dn */
uae_u32 REGPARAM2 op_e1b0_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 cnt = m68k_dreg(regs, srcreg);
	uae_s32 data = m68k_dreg(regs, dstreg);
	uae_u32 val = data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	if (cnt >= 33) cnt -= 33;
	if (cnt > 0) {
		cnt--;
		{
			uae_u32 carry;
			uae_u32 loval = val >> (31 - cnt);
			carry = loval & 1;
			val = (((val << 1) | GET_XFLG()) << cnt) | (loval >> 1);
			SET_XFLG(carry);
			val &= 0xffffffff;
		}
	}
	SET_CFLG(GET_XFLG());
	SET_ZFLG(((uae_s32)(val)) == 0);
	SET_NFLG(((uae_s32)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (val);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* ROL.L Dn,Dn */
uae_u32 REGPARAM2 op_e1b8_44_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 cnt = m68k_dreg(regs, srcreg);
	uae_s32 data = m68k_dreg(regs, dstreg);
	uae_u32 val = data;
	CLEAR_CZNV();
	int ccnt = cnt & 63;
	cnt &= 63;
	if (cnt > 0) {
		uae_u32 loval;
		cnt &= 31;
		loval = val >> (32 - cnt);
		val <<= cnt;
		val |= loval;
		val &= 0xffffffff;
		SET_CFLG(val & 1);
	}
	SET_ZFLG(((uae_s32)(val)) == 0);
	SET_NFLG(((uae_s32)(val)) < 0);
	count_cycles += (2 * ccnt) * CYCLE_UNIT / 2;
	m68k_dreg(regs, dstreg) = (val);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* ASLW.W (An) */
uae_u32 REGPARAM2 op_e1d0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr dataa;
	dataa = m68k_areg(regs, srcreg);
	uae_s16 data = get_word_jit(dataa);
	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 sign2;
	val <<= 1;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	sign2 = 0x8000 & val;
	SET_CFLG(sign != 0);
	COPY_CARRY();
	SET_VFLG(GET_VFLG() | (sign2 != sign));
	put_word_jit(dataa, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ASLW.W (An)+ */
uae_u32 REGPARAM2 op_e1d8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr dataa;
	dataa = m68k_areg(regs, srcreg);
	uae_s16 data = get_word_jit(dataa);
	m68k_areg(regs, srcreg) += 2;
	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 sign2;
	val <<= 1;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	sign2 = 0x8000 & val;
	SET_CFLG(sign != 0);
	COPY_CARRY();
	SET_VFLG(GET_VFLG() | (sign2 != sign));
	put_word_jit(dataa, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ASLW.W -(An) */
uae_u32 REGPARAM2 op_e1e0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr dataa;
	dataa = m68k_areg(regs, srcreg) - 2;
	uae_s16 data = get_word_jit(dataa);
	m68k_areg(regs, srcreg) = dataa;
	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 sign2;
	val <<= 1;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	sign2 = 0x8000 & val;
	SET_CFLG(sign != 0);
	COPY_CARRY();
	SET_VFLG(GET_VFLG() | (sign2 != sign));
	put_word_jit(dataa, val);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ASLW.W (d16,An) */
uae_u32 REGPARAM2 op_e1e8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr dataa;
	dataa = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 data = get_word_jit(dataa);
	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 sign2;
	val <<= 1;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	sign2 = 0x8000 & val;
	SET_CFLG(sign != 0);
	COPY_CARRY();
	SET_VFLG(GET_VFLG() | (sign2 != sign));
	put_word_jit(dataa, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ASLW.W (d8,An,Xn) */
uae_u32 REGPARAM2 op_e1f0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr dataa;
	dataa = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s16 data = get_word_jit(dataa);
	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 sign2;
	val <<= 1;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	sign2 = 0x8000 & val;
	SET_CFLG(sign != 0);
	COPY_CARRY();
	SET_VFLG(GET_VFLG() | (sign2 != sign));
	put_word_jit(dataa, val);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* ASLW.W (xxx).W */
uae_u32 REGPARAM2 op_e1f8_44_ff(uae_u32 opcode)
{
	uaecptr dataa;
	dataa = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 data = get_word_jit(dataa);
	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 sign2;
	val <<= 1;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	sign2 = 0x8000 & val;
	SET_CFLG(sign != 0);
	COPY_CARRY();
	SET_VFLG(GET_VFLG() | (sign2 != sign));
	put_word_jit(dataa, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ASLW.W (xxx).L */
uae_u32 REGPARAM2 op_e1f9_44_ff(uae_u32 opcode)
{
	uaecptr dataa;
	dataa = get_dilong(2);
	uae_s16 data = get_word_jit(dataa);
	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 sign2;
	val <<= 1;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	sign2 = 0x8000 & val;
	SET_CFLG(sign != 0);
	COPY_CARRY();
	SET_VFLG(GET_VFLG() | (sign2 != sign));
	put_word_jit(dataa, val);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* LSRW.W (An) */
uae_u32 REGPARAM2 op_e2d0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr dataa;
	dataa = m68k_areg(regs, srcreg);
	uae_s16 data = get_word_jit(dataa);
	uae_u32 val = (uae_u16)data;
	uae_u32 carry = val & 1;
	val >>= 1;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry);
	COPY_CARRY();
	put_word_jit(dataa, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* LSRW.W (An)+ */
uae_u32 REGPARAM2 op_e2d8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr dataa;
	dataa = m68k_areg(regs, srcreg);
	uae_s16 data = get_word_jit(dataa);
	m68k_areg(regs, srcreg) += 2;
	uae_u32 val = (uae_u16)data;
	uae_u32 carry = val & 1;
	val >>= 1;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry);
	COPY_CARRY();
	put_word_jit(dataa, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* LSRW.W -(An) */
uae_u32 REGPARAM2 op_e2e0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr dataa;
	dataa = m68k_areg(regs, srcreg) - 2;
	uae_s16 data = get_word_jit(dataa);
	m68k_areg(regs, srcreg) = dataa;
	uae_u32 val = (uae_u16)data;
	uae_u32 carry = val & 1;
	val >>= 1;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry);
	COPY_CARRY();
	put_word_jit(dataa, val);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* LSRW.W (d16,An) */
uae_u32 REGPARAM2 op_e2e8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr dataa;
	dataa = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 data = get_word_jit(dataa);
	uae_u32 val = (uae_u16)data;
	uae_u32 carry = val & 1;
	val >>= 1;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry);
	COPY_CARRY();
	put_word_jit(dataa, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* LSRW.W (d8,An,Xn) */
uae_u32 REGPARAM2 op_e2f0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr dataa;
	dataa = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s16 data = get_word_jit(dataa);
	uae_u32 val = (uae_u16)data;
	uae_u32 carry = val & 1;
	val >>= 1;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry);
	COPY_CARRY();
	put_word_jit(dataa, val);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* LSRW.W (xxx).W */
uae_u32 REGPARAM2 op_e2f8_44_ff(uae_u32 opcode)
{
	uaecptr dataa;
	dataa = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 data = get_word_jit(dataa);
	uae_u32 val = (uae_u16)data;
	uae_u32 carry = val & 1;
	val >>= 1;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry);
	COPY_CARRY();
	put_word_jit(dataa, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* LSRW.W (xxx).L */
uae_u32 REGPARAM2 op_e2f9_44_ff(uae_u32 opcode)
{
	uaecptr dataa;
	dataa = get_dilong(2);
	uae_s16 data = get_word_jit(dataa);
	uae_u32 val = (uae_u16)data;
	uae_u32 carry = val & 1;
	val >>= 1;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry);
	COPY_CARRY();
	put_word_jit(dataa, val);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* LSLW.W (An) */
uae_u32 REGPARAM2 op_e3d0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr dataa;
	dataa = m68k_areg(regs, srcreg);
	uae_s16 data = get_word_jit(dataa);
	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry >> 15);
	COPY_CARRY();
	put_word_jit(dataa, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* LSLW.W (An)+ */
uae_u32 REGPARAM2 op_e3d8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr dataa;
	dataa = m68k_areg(regs, srcreg);
	uae_s16 data = get_word_jit(dataa);
	m68k_areg(regs, srcreg) += 2;
	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry >> 15);
	COPY_CARRY();
	put_word_jit(dataa, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* LSLW.W -(An) */
uae_u32 REGPARAM2 op_e3e0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr dataa;
	dataa = m68k_areg(regs, srcreg) - 2;
	uae_s16 data = get_word_jit(dataa);
	m68k_areg(regs, srcreg) = dataa;
	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry >> 15);
	COPY_CARRY();
	put_word_jit(dataa, val);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* LSLW.W (d16,An) */
uae_u32 REGPARAM2 op_e3e8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr dataa;
	dataa = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 data = get_word_jit(dataa);
	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry >> 15);
	COPY_CARRY();
	put_word_jit(dataa, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* LSLW.W (d8,An,Xn) */
uae_u32 REGPARAM2 op_e3f0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr dataa;
	dataa = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s16 data = get_word_jit(dataa);
	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry >> 15);
	COPY_CARRY();
	put_word_jit(dataa, val);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* LSLW.W (xxx).W */
uae_u32 REGPARAM2 op_e3f8_44_ff(uae_u32 opcode)
{
	uaecptr dataa;
	dataa = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 data = get_word_jit(dataa);
	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry >> 15);
	COPY_CARRY();
	put_word_jit(dataa, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* LSLW.W (xxx).L */
uae_u32 REGPARAM2 op_e3f9_44_ff(uae_u32 opcode)
{
	uaecptr dataa;
	dataa = get_dilong(2);
	uae_s16 data = get_word_jit(dataa);
	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry >> 15);
	COPY_CARRY();
	put_word_jit(dataa, val);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* ROXRW.W (An) */
uae_u32 REGPARAM2 op_e4d0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr dataa;
	dataa = m68k_areg(regs, srcreg);
	uae_s16 data = get_word_jit(dataa);
	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (GET_XFLG()) val |= 0x8000;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry);
	COPY_CARRY();
	put_word_jit(dataa, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ROXRW.W (An)+ */
uae_u32 REGPARAM2 op_e4d8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr dataa;
	dataa = m68k_areg(regs, srcreg);
	uae_s16 data = get_word_jit(dataa);
	m68k_areg(regs, srcreg) += 2;
	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (GET_XFLG()) val |= 0x8000;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry);
	COPY_CARRY();
	put_word_jit(dataa, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ROXRW.W -(An) */
uae_u32 REGPARAM2 op_e4e0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr dataa;
	dataa = m68k_areg(regs, srcreg) - 2;
	uae_s16 data = get_word_jit(dataa);
	m68k_areg(regs, srcreg) = dataa;
	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (GET_XFLG()) val |= 0x8000;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry);
	COPY_CARRY();
	put_word_jit(dataa, val);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ROXRW.W (d16,An) */
uae_u32 REGPARAM2 op_e4e8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr dataa;
	dataa = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 data = get_word_jit(dataa);
	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (GET_XFLG()) val |= 0x8000;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry);
	COPY_CARRY();
	put_word_jit(dataa, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ROXRW.W (d8,An,Xn) */
uae_u32 REGPARAM2 op_e4f0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr dataa;
	dataa = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s16 data = get_word_jit(dataa);
	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (GET_XFLG()) val |= 0x8000;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry);
	COPY_CARRY();
	put_word_jit(dataa, val);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* ROXRW.W (xxx).W */
uae_u32 REGPARAM2 op_e4f8_44_ff(uae_u32 opcode)
{
	uaecptr dataa;
	dataa = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 data = get_word_jit(dataa);
	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (GET_XFLG()) val |= 0x8000;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry);
	COPY_CARRY();
	put_word_jit(dataa, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ROXRW.W (xxx).L */
uae_u32 REGPARAM2 op_e4f9_44_ff(uae_u32 opcode)
{
	uaecptr dataa;
	dataa = get_dilong(2);
	uae_s16 data = get_word_jit(dataa);
	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (GET_XFLG()) val |= 0x8000;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry);
	COPY_CARRY();
	put_word_jit(dataa, val);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* ROXLW.W (An) */
uae_u32 REGPARAM2 op_e5d0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr dataa;
	dataa = m68k_areg(regs, srcreg);
	uae_s16 data = get_word_jit(dataa);
	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (GET_XFLG()) val |= 1;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry >> 15);
	COPY_CARRY();
	put_word_jit(dataa, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ROXLW.W (An)+ */
uae_u32 REGPARAM2 op_e5d8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr dataa;
	dataa = m68k_areg(regs, srcreg);
	uae_s16 data = get_word_jit(dataa);
	m68k_areg(regs, srcreg) += 2;
	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (GET_XFLG()) val |= 1;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry >> 15);
	COPY_CARRY();
	put_word_jit(dataa, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ROXLW.W -(An) */
uae_u32 REGPARAM2 op_e5e0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr dataa;
	dataa = m68k_areg(regs, srcreg) - 2;
	uae_s16 data = get_word_jit(dataa);
	m68k_areg(regs, srcreg) = dataa;
	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (GET_XFLG()) val |= 1;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry >> 15);
	COPY_CARRY();
	put_word_jit(dataa, val);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ROXLW.W (d16,An) */
uae_u32 REGPARAM2 op_e5e8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr dataa;
	dataa = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 data = get_word_jit(dataa);
	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (GET_XFLG()) val |= 1;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry >> 15);
	COPY_CARRY();
	put_word_jit(dataa, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ROXLW.W (d8,An,Xn) */
uae_u32 REGPARAM2 op_e5f0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr dataa;
	dataa = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s16 data = get_word_jit(dataa);
	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (GET_XFLG()) val |= 1;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry >> 15);
	COPY_CARRY();
	put_word_jit(dataa, val);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* ROXLW.W (xxx).W */
uae_u32 REGPARAM2 op_e5f8_44_ff(uae_u32 opcode)
{
	uaecptr dataa;
	dataa = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 data = get_word_jit(dataa);
	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (GET_XFLG()) val |= 1;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry >> 15);
	COPY_CARRY();
	put_word_jit(dataa, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ROXLW.W (xxx).L */
uae_u32 REGPARAM2 op_e5f9_44_ff(uae_u32 opcode)
{
	uaecptr dataa;
	dataa = get_dilong(2);
	uae_s16 data = get_word_jit(dataa);
	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (GET_XFLG()) val |= 1;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry >> 15);
	COPY_CARRY();
	put_word_jit(dataa, val);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* RORW.W (An) */
uae_u32 REGPARAM2 op_e6d0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr dataa;
	dataa = m68k_areg(regs, srcreg);
	uae_s16 data = get_word_jit(dataa);
	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (carry) val |= 0x8000;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry);
	put_word_jit(dataa, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* RORW.W (An)+ */
uae_u32 REGPARAM2 op_e6d8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr dataa;
	dataa = m68k_areg(regs, srcreg);
	uae_s16 data = get_word_jit(dataa);
	m68k_areg(regs, srcreg) += 2;
	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (carry) val |= 0x8000;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry);
	put_word_jit(dataa, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* RORW.W -(An) */
uae_u32 REGPARAM2 op_e6e0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr dataa;
	dataa = m68k_areg(regs, srcreg) - 2;
	uae_s16 data = get_word_jit(dataa);
	m68k_areg(regs, srcreg) = dataa;
	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (carry) val |= 0x8000;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry);
	put_word_jit(dataa, val);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* RORW.W (d16,An) */
uae_u32 REGPARAM2 op_e6e8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr dataa;
	dataa = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 data = get_word_jit(dataa);
	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (carry) val |= 0x8000;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry);
	put_word_jit(dataa, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* RORW.W (d8,An,Xn) */
uae_u32 REGPARAM2 op_e6f0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr dataa;
	dataa = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s16 data = get_word_jit(dataa);
	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (carry) val |= 0x8000;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry);
	put_word_jit(dataa, val);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* RORW.W (xxx).W */
uae_u32 REGPARAM2 op_e6f8_44_ff(uae_u32 opcode)
{
	uaecptr dataa;
	dataa = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 data = get_word_jit(dataa);
	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (carry) val |= 0x8000;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry);
	put_word_jit(dataa, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* RORW.W (xxx).L */
uae_u32 REGPARAM2 op_e6f9_44_ff(uae_u32 opcode)
{
	uaecptr dataa;
	dataa = get_dilong(2);
	uae_s16 data = get_word_jit(dataa);
	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (carry) val |= 0x8000;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry);
	put_word_jit(dataa, val);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* ROLW.W (An) */
uae_u32 REGPARAM2 op_e7d0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr dataa;
	dataa = m68k_areg(regs, srcreg);
	uae_s16 data = get_word_jit(dataa);
	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (carry)  val |= 1;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry >> 15);
	put_word_jit(dataa, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ROLW.W (An)+ */
uae_u32 REGPARAM2 op_e7d8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr dataa;
	dataa = m68k_areg(regs, srcreg);
	uae_s16 data = get_word_jit(dataa);
	m68k_areg(regs, srcreg) += 2;
	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (carry)  val |= 1;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry >> 15);
	put_word_jit(dataa, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ROLW.W -(An) */
uae_u32 REGPARAM2 op_e7e0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr dataa;
	dataa = m68k_areg(regs, srcreg) - 2;
	uae_s16 data = get_word_jit(dataa);
	m68k_areg(regs, srcreg) = dataa;
	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (carry)  val |= 1;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry >> 15);
	put_word_jit(dataa, val);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ROLW.W (d16,An) */
uae_u32 REGPARAM2 op_e7e8_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr dataa;
	dataa = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 data = get_word_jit(dataa);
	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (carry)  val |= 1;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry >> 15);
	put_word_jit(dataa, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ROLW.W (d8,An,Xn) */
uae_u32 REGPARAM2 op_e7f0_44_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr dataa;
	dataa = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s16 data = get_word_jit(dataa);
	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (carry)  val |= 1;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry >> 15);
	put_word_jit(dataa, val);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* ROLW.W (xxx).W */
uae_u32 REGPARAM2 op_e7f8_44_ff(uae_u32 opcode)
{
	uaecptr dataa;
	dataa = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 data = get_word_jit(dataa);
	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (carry)  val |= 1;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry >> 15);
	put_word_jit(dataa, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ROLW.W (xxx).L */
uae_u32 REGPARAM2 op_e7f9_44_ff(uae_u32 opcode)
{
	uaecptr dataa;
	dataa = get_dilong(2);
	uae_s16 data = get_word_jit(dataa);
	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (carry)  val |= 1;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(val)) == 0);
	SET_NFLG(((uae_s16)(val)) < 0);
	SET_CFLG(carry >> 15);
	put_word_jit(dataa, val);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* ORSR.B #<data>.W */
uae_u32 REGPARAM2 op_003c_45_ff(uae_u32 opcode)
{
	MakeSR();
	uae_s16 src = get_diword (2);
	src &= 0xFF;
	regs.sr |= src;
	MakeFromSR();
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ORSR.W #<data>.W */
uae_u32 REGPARAM2 op_007c_45_ff(uae_u32 opcode)
{
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	MakeSR();
	uae_s16 src = get_diword (2);
	regs.sr |= src;
	MakeFromSR_T0();
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* OR.L #<data>.L,Dn */
uae_u32 REGPARAM2 op_0080_45_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MVPMR.W (d16,An),Dn */
uae_u32 REGPARAM2 op_0108_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr mempa = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_u16 val  = (get_byte_jit(mempa) & 0xff) << 8;
	val |= (get_byte_jit(mempa + 2) & 0xff);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((val) & 0xffff);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MVPMR.L (d16,An),Dn */
uae_u32 REGPARAM2 op_0148_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr mempa = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_u32 val  = (get_byte_jit(mempa) & 0xff) << 24;
	val |= (get_byte_jit(mempa + 2) & 0xff) << 16;
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & 0x0000ffff) | val;
	val |= (get_byte_jit(mempa + 4) & 0xff) << 8;
	val |= (get_byte_jit(mempa + 6) & 0xff);
	m68k_dreg(regs, dstreg) = (val);
	m68k_incpc(4);
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MVPRM.W Dn,(d16,An) */
uae_u32 REGPARAM2 op_0188_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uaecptr mempa = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	put_byte_jit(mempa, src >> 8);
	put_byte_jit(mempa + 2, src); 
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* BCLR.B Dn,(An) */
uae_u32 REGPARAM2 op_0190_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* BCLR.B Dn,(An)+ */
uae_u32 REGPARAM2 op_0198_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* BCLR.B Dn,-(An) */
uae_u32 REGPARAM2 op_01a0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* BCLR.B Dn,(d16,An) */
uae_u32 REGPARAM2 op_01a8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* BCLR.B Dn,(d8,An,Xn) */
uae_u32 REGPARAM2 op_01b0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (2));
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* BCLR.B Dn,(xxx).W */
uae_u32 REGPARAM2 op_01b8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* BCLR.B Dn,(xxx).L */
uae_u32 REGPARAM2 op_01b9_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = get_dilong(2);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MVPRM.L Dn,(d16,An) */
uae_u32 REGPARAM2 op_01c8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr mempa = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (2);
	put_byte_jit(mempa, src >> 24);
	put_byte_jit(mempa + 2, src >> 16);
	put_byte_jit(mempa + 4, src >> 8);
	put_byte_jit(mempa + 6, src); 
	m68k_incpc(4);
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ANDSR.B #<data>.W */
uae_u32 REGPARAM2 op_023c_45_ff(uae_u32 opcode)
{
	MakeSR();
	uae_s16 src = get_diword (2);
	src |= 0xff00;
	regs.sr &= src;
	MakeFromSR();
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ANDSR.W #<data>.W */
uae_u32 REGPARAM2 op_027c_45_ff(uae_u32 opcode)
{
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	MakeSR();
	uae_s16 src = get_diword (2);
	regs.sr &= src;
	MakeFromSR_T0();
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* AND.L #<data>.L,Dn */
uae_u32 REGPARAM2 op_0280_45_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* SUB.L #<data>.L,Dn */
uae_u32 REGPARAM2 op_0480_45_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* ADD.L #<data>.L,Dn */
uae_u32 REGPARAM2 op_0680_45_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* BCLR.B #<data>.W,(An) */
uae_u32 REGPARAM2 op_0890_45_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* BCLR.B #<data>.W,(An)+ */
uae_u32 REGPARAM2 op_0898_45_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg);
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) += areg_byteinc[dstreg];
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* BCLR.B #<data>.W,-(An) */
uae_u32 REGPARAM2 op_08a0_45_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* BCLR.B #<data>.W,(d16,An) */
uae_u32 REGPARAM2 op_08a8_45_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) + (uae_s32)(uae_s16)get_diword (4);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* BCLR.B #<data>.W,(d8,An,Xn) */
uae_u32 REGPARAM2 op_08b0_45_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = get_disp_ea_000(m68k_areg(regs, dstreg), get_diword (4));
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 4,0   */

/* BCLR.B #<data>.W,(xxx).W */
uae_u32 REGPARAM2 op_08b8_45_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = (uae_s32)(uae_s16)get_diword (4);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* BCLR.B #<data>.W,(xxx).L */
uae_u32 REGPARAM2 op_08b9_45_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	uaecptr dsta;
	dsta = get_dilong(4);
	uae_s8 dst = get_byte_jit(dsta);
	src &= 7;
	SET_ZFLG(1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	put_byte_jit(dsta, dst);
	m68k_incpc(8);
	return 24 * CYCLE_UNIT / 2;
}
/* 8 0,0   */

/* EORSR.B #<data>.W */
uae_u32 REGPARAM2 op_0a3c_45_ff(uae_u32 opcode)
{
	MakeSR();
	uae_s16 src = get_diword (2);
	src &= 0xFF;
	regs.sr ^= src;
	MakeFromSR();
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* EORSR.W #<data>.W */
uae_u32 REGPARAM2 op_0a7c_45_ff(uae_u32 opcode)
{
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	MakeSR();
	uae_s16 src = get_diword (2);
	regs.sr ^= src;
	MakeFromSR_T0();
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* EOR.L #<data>.L,Dn */
uae_u32 REGPARAM2 op_0a80_45_ff(uae_u32 opcode)
{
	uae_u32 dstreg = opcode & 7;
	uae_s32 src;
	src = get_dilong(2);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MOVE.L Dn,-(An) */
uae_u32 REGPARAM2 op_2100_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	m68k_areg(regs, dstreg) = dsta;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MOVE.L An,-(An) */
uae_u32 REGPARAM2 op_2108_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src = m68k_areg(regs, srcreg);
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	m68k_areg(regs, dstreg) = dsta;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	put_long_jit(dsta, src);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MVSR2.W Dn */
uae_u32 REGPARAM2 op_40c0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	MakeSR();
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | ((regs.sr) & 0xffff);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MVSR2.W (An) */
uae_u32 REGPARAM2 op_40d0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	MakeSR();
	get_word_jit(srca);
	put_word_jit(srca, regs.sr);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MVSR2.W (An)+ */
uae_u32 REGPARAM2 op_40d8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	m68k_areg(regs, srcreg) += 2;
	MakeSR();
	get_word_jit(srca);
	put_word_jit(srca, regs.sr);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MVSR2.W -(An) */
uae_u32 REGPARAM2 op_40e0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	m68k_areg(regs, srcreg) = srca;
	MakeSR();
	get_word_jit(srca);
	put_word_jit(srca, regs.sr);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MVSR2.W (d16,An) */
uae_u32 REGPARAM2 op_40e8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	MakeSR();
	get_word_jit(srca);
	put_word_jit(srca, regs.sr);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MVSR2.W (d8,An,Xn) */
uae_u32 REGPARAM2 op_40f0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	MakeSR();
	get_word_jit(srca);
	put_word_jit(srca, regs.sr);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MVSR2.W (xxx).W */
uae_u32 REGPARAM2 op_40f8_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	MakeSR();
	get_word_jit(srca);
	put_word_jit(srca, regs.sr);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MVSR2.W (xxx).L */
uae_u32 REGPARAM2 op_40f9_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	MakeSR();
	get_word_jit(srca);
	put_word_jit(srca, regs.sr);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* LEA.L (xxx).L,An */
uae_u32 REGPARAM2 op_41f9_45_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	m68k_areg(regs, dstreg) = (srca);
	m68k_incpc(6);
	return 12 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* CLR.B Dn */
uae_u32 REGPARAM2 op_4200_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(0)) == 0);
	SET_NFLG(((uae_s8)(0)) < 0);
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((0) & 0xff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CLR.B (An) */
uae_u32 REGPARAM2 op_4210_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(0)) == 0);
	SET_NFLG(((uae_s8)(0)) < 0);
	put_byte_jit(srca, 0);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CLR.B (An)+ */
uae_u32 REGPARAM2 op_4218_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(0)) == 0);
	SET_NFLG(((uae_s8)(0)) < 0);
	put_byte_jit(srca, 0);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CLR.B -(An) */
uae_u32 REGPARAM2 op_4220_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(0)) == 0);
	SET_NFLG(((uae_s8)(0)) < 0);
	put_byte_jit(srca, 0);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CLR.B (d16,An) */
uae_u32 REGPARAM2 op_4228_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(0)) == 0);
	SET_NFLG(((uae_s8)(0)) < 0);
	put_byte_jit(srca, 0);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* CLR.B (d8,An,Xn) */
uae_u32 REGPARAM2 op_4230_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(0)) == 0);
	SET_NFLG(((uae_s8)(0)) < 0);
	put_byte_jit(srca, 0);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* CLR.B (xxx).W */
uae_u32 REGPARAM2 op_4238_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(0)) == 0);
	SET_NFLG(((uae_s8)(0)) < 0);
	put_byte_jit(srca, 0);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* CLR.B (xxx).L */
uae_u32 REGPARAM2 op_4239_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s8 src = get_byte_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(0)) == 0);
	SET_NFLG(((uae_s8)(0)) < 0);
	put_byte_jit(srca, 0);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* CLR.W Dn */
uae_u32 REGPARAM2 op_4240_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(0)) == 0);
	SET_NFLG(((uae_s16)(0)) < 0);
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | ((0) & 0xffff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CLR.W (An) */
uae_u32 REGPARAM2 op_4250_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(0)) == 0);
	SET_NFLG(((uae_s16)(0)) < 0);
	put_word_jit(srca, 0);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CLR.W (An)+ */
uae_u32 REGPARAM2 op_4258_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) += 2;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(0)) == 0);
	SET_NFLG(((uae_s16)(0)) < 0);
	put_word_jit(srca, 0);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CLR.W -(An) */
uae_u32 REGPARAM2 op_4260_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(0)) == 0);
	SET_NFLG(((uae_s16)(0)) < 0);
	put_word_jit(srca, 0);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CLR.W (d16,An) */
uae_u32 REGPARAM2 op_4268_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(0)) == 0);
	SET_NFLG(((uae_s16)(0)) < 0);
	put_word_jit(srca, 0);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* CLR.W (d8,An,Xn) */
uae_u32 REGPARAM2 op_4270_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s16 src = get_word_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(0)) == 0);
	SET_NFLG(((uae_s16)(0)) < 0);
	put_word_jit(srca, 0);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* CLR.W (xxx).W */
uae_u32 REGPARAM2 op_4278_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(0)) == 0);
	SET_NFLG(((uae_s16)(0)) < 0);
	put_word_jit(srca, 0);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* CLR.W (xxx).L */
uae_u32 REGPARAM2 op_4279_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s16 src = get_word_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s16)(0)) == 0);
	SET_NFLG(((uae_s16)(0)) < 0);
	put_word_jit(srca, 0);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* CLR.L Dn */
uae_u32 REGPARAM2 op_4280_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_s32 src = m68k_dreg(regs, srcreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(0)) == 0);
	SET_NFLG(((uae_s32)(0)) < 0);
	m68k_dreg(regs, srcreg) = (0);
	m68k_incpc(2);
	return 6 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CLR.L (An) */
uae_u32 REGPARAM2 op_4290_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(0)) == 0);
	SET_NFLG(((uae_s32)(0)) < 0);
	put_long_jit(srca, 0);
	m68k_incpc(2);
	return 20 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CLR.L (An)+ */
uae_u32 REGPARAM2 op_4298_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) += 4;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(0)) == 0);
	SET_NFLG(((uae_s32)(0)) < 0);
	put_long_jit(srca, 0);
	m68k_incpc(2);
	return 20 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CLR.L -(An) */
uae_u32 REGPARAM2 op_42a0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(0)) == 0);
	SET_NFLG(((uae_s32)(0)) < 0);
	put_long_jit(srca, 0);
	m68k_incpc(2);
	return 22 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* CLR.L (d16,An) */
uae_u32 REGPARAM2 op_42a8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(0)) == 0);
	SET_NFLG(((uae_s32)(0)) < 0);
	put_long_jit(srca, 0);
	m68k_incpc(4);
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* CLR.L (d8,An,Xn) */
uae_u32 REGPARAM2 op_42b0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s32 src = get_long_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(0)) == 0);
	SET_NFLG(((uae_s32)(0)) < 0);
	put_long_jit(srca, 0);
	m68k_incpc(4);
	return 26 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* CLR.L (xxx).W */
uae_u32 REGPARAM2 op_42b8_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(0)) == 0);
	SET_NFLG(((uae_s32)(0)) < 0);
	put_long_jit(srca, 0);
	m68k_incpc(4);
	return 24 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* CLR.L (xxx).L */
uae_u32 REGPARAM2 op_42b9_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s32 src = get_long_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(0)) == 0);
	SET_NFLG(((uae_s32)(0)) < 0);
	put_long_jit(srca, 0);
	m68k_incpc(6);
	return 28 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MV2SR.B Dn */
uae_u32 REGPARAM2 op_44c0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	MakeSR();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR();
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MV2SR.B (An) */
uae_u32 REGPARAM2 op_44d0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	MakeSR();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR();
	m68k_incpc(2);
	return 16 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MV2SR.B (An)+ */
uae_u32 REGPARAM2 op_44d8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) += 2;
	MakeSR();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR();
	m68k_incpc(2);
	return 16 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MV2SR.B -(An) */
uae_u32 REGPARAM2 op_44e0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	MakeSR();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR();
	m68k_incpc(2);
	return 18 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MV2SR.B (d16,An) */
uae_u32 REGPARAM2 op_44e8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	MakeSR();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR();
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MV2SR.B (d8,An,Xn) */
uae_u32 REGPARAM2 op_44f0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s16 src = get_word_jit(srca);
	MakeSR();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR();
	m68k_incpc(4);
	return 22 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MV2SR.B (xxx).W */
uae_u32 REGPARAM2 op_44f8_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	MakeSR();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR();
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MV2SR.B (xxx).L */
uae_u32 REGPARAM2 op_44f9_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s16 src = get_word_jit(srca);
	MakeSR();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR();
	m68k_incpc(6);
	return 24 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MV2SR.B (d16,PC) */
uae_u32 REGPARAM2 op_44fa_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	MakeSR();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR();
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MV2SR.B (d8,PC,Xn) */
uae_u32 REGPARAM2 op_44fb_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s16 src = get_word_jit(srca);
	MakeSR();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR();
	m68k_incpc(4);
	return 22 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MV2SR.B #<data>.B */
uae_u32 REGPARAM2 op_44fc_45_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	MakeSR();
	regs.sr &= 0xFF00;
	regs.sr |= src & 0xFF;
	MakeFromSR();
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MV2SR.W Dn */
uae_u32 REGPARAM2 op_46c0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uae_s16 src = m68k_dreg(regs, srcreg);
	regs.sr = src;
	MakeFromSR_T0();
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MV2SR.W (An) */
uae_u32 REGPARAM2 op_46d0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	regs.sr = src;
	MakeFromSR_T0();
	m68k_incpc(2);
	return 16 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MV2SR.W (An)+ */
uae_u32 REGPARAM2 op_46d8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) += 2;
	regs.sr = src;
	MakeFromSR_T0();
	m68k_incpc(2);
	return 16 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MV2SR.W -(An) */
uae_u32 REGPARAM2 op_46e0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	regs.sr = src;
	MakeFromSR_T0();
	m68k_incpc(2);
	return 18 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MV2SR.W (d16,An) */
uae_u32 REGPARAM2 op_46e8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	regs.sr = src;
	MakeFromSR_T0();
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MV2SR.W (d8,An,Xn) */
uae_u32 REGPARAM2 op_46f0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s16 src = get_word_jit(srca);
	regs.sr = src;
	MakeFromSR_T0();
	m68k_incpc(4);
	return 22 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MV2SR.W (xxx).W */
uae_u32 REGPARAM2 op_46f8_45_ff(uae_u32 opcode)
{
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	regs.sr = src;
	MakeFromSR_T0();
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MV2SR.W (xxx).L */
uae_u32 REGPARAM2 op_46f9_45_ff(uae_u32 opcode)
{
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uaecptr srca;
	srca = get_dilong(2);
	uae_s16 src = get_word_jit(srca);
	regs.sr = src;
	MakeFromSR_T0();
	m68k_incpc(6);
	return 24 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MV2SR.W (d16,PC) */
uae_u32 REGPARAM2 op_46fa_45_ff(uae_u32 opcode)
{
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s16 src = get_word_jit(srca);
	regs.sr = src;
	MakeFromSR_T0();
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* MV2SR.W (d8,PC,Xn) */
uae_u32 REGPARAM2 op_46fb_45_ff(uae_u32 opcode)
{
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s16 src = get_word_jit(srca);
	regs.sr = src;
	MakeFromSR_T0();
	m68k_incpc(4);
	return 22 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* MV2SR.W #<data>.W */
uae_u32 REGPARAM2 op_46fc_45_ff(uae_u32 opcode)
{
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uae_s16 src = get_diword (2);
	regs.sr = src;
	MakeFromSR_T0();
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* TAS.B Dn */
uae_u32 REGPARAM2 op_4ac0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	src |= 0x80;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((src) & 0xff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* TAS.B (An) */
uae_u32 REGPARAM2 op_4ad0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	src |= 0x80;
	put_byte_jit(srca, src);
	m68k_incpc(2);
	return 18 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* TAS.B (An)+ */
uae_u32 REGPARAM2 op_4ad8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	src |= 0x80;
	put_byte_jit(srca, src);
	m68k_incpc(2);
	return 18 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* TAS.B -(An) */
uae_u32 REGPARAM2 op_4ae0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	src |= 0x80;
	put_byte_jit(srca, src);
	m68k_incpc(2);
	return 20 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* TAS.B (d16,An) */
uae_u32 REGPARAM2 op_4ae8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	src |= 0x80;
	put_byte_jit(srca, src);
	m68k_incpc(4);
	return 22 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* TAS.B (d8,An,Xn) */
uae_u32 REGPARAM2 op_4af0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	src |= 0x80;
	put_byte_jit(srca, src);
	m68k_incpc(4);
	return 24 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* TAS.B (xxx).W */
uae_u32 REGPARAM2 op_4af8_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	src |= 0x80;
	put_byte_jit(srca, src);
	m68k_incpc(4);
	return 22 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* TAS.B (xxx).L */
uae_u32 REGPARAM2 op_4af9_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s8 src = get_byte_jit(srca);
	CLEAR_CZNV();
	SET_ZFLG(((uae_s8)(src)) == 0);
	SET_NFLG(((uae_s8)(src)) < 0);
	src |= 0x80;
	put_byte_jit(srca, src);
	m68k_incpc(6);
	return 26 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* MVR2USP.L An */
uae_u32 REGPARAM2 op_4e60_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uae_s32 src = m68k_areg(regs, srcreg);
	regs.usp = src;
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* MVUSP2R.L An */
uae_u32 REGPARAM2 op_4e68_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	m68k_areg(regs, srcreg) = (regs.usp);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* STOP.L #<data>.W */
uae_u32 REGPARAM2 op_4e72_45_ff(uae_u32 opcode)
{
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	uae_s16 src = get_diword (2);
	uae_u16 sr = src;
	regs.sr = sr;
	MakeFromSR();
	m68k_setstopped();
	m68k_incpc(4);
	return 4 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* RTE.L  */
uae_u32 REGPARAM2 op_4e73_45_ff(uae_u32 opcode)
{
	if (!regs.s) {
		Exception(8);
		return 4 * CYCLE_UNIT / 2;
	}
	if (m68k_areg(regs, 7) & 1) {
		exception3_read_access(opcode, m68k_areg(regs, 7), 1, 1);
		return 4 * CYCLE_UNIT / 2;
	}
	uaecptr oldpc = m68k_getpc ();
	uaecptr a = m68k_areg(regs, 7);
	uae_u16 sr = get_word_jit(a);
	m68k_areg(regs, 7) += 6;
	uae_u32 pc = get_word_jit(a + 2) << 16;
	pc |= get_word_jit(a + 2 + 2); 
	uae_u16 oldt1 = regs.t1;
	regs.sr = sr;
	MakeFromSR();
	if (pc & 1) {
		m68k_incpc(2);
		exception3_read_access(opcode | 0x20000, pc, 1, 2);
		return 16 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(pc);
	return 20 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* RTS.L  */
uae_u32 REGPARAM2 op_4e75_45_ff(uae_u32 opcode)
{
	uaecptr oldpc = m68k_getpc ();
	if (m68k_areg(regs, 7) & 1) {
		m68k_incpc(2);
		exception3_read_access(opcode, m68k_areg(regs, 7), 1, 1);
		return 4 * CYCLE_UNIT / 2;
	}
	m68k_do_rts();
	if (m68k_getpc () & 1) {
		uaecptr faultpc = m68k_getpc ();
		m68k_setpc_j(oldpc);
		m68k_incpc(2);
		exception3_read_prefetch_only(opcode, faultpc);
		return 12 * CYCLE_UNIT / 2;
	}
	return 16 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* TRAPV.L  */
uae_u32 REGPARAM2 op_4e76_45_ff(uae_u32 opcode)
{
	uaecptr oldpc = m68k_getpc ();
	m68k_incpc(2);
	if (GET_VFLG()) {
		Exception_cpu_oldpc(7,oldpc);
		return 4 * CYCLE_UNIT / 2;
	}
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* RTR.L  */
uae_u32 REGPARAM2 op_4e77_45_ff(uae_u32 opcode)
{
	if (m68k_areg(regs, 7) & 1) {
		m68k_incpc(2);
		exception3_read_access(opcode, m68k_areg(regs, 7), 1, 1);
		return 4 * CYCLE_UNIT / 2;
	}
	uaecptr oldpc = m68k_getpc ();
	MakeSR();
	uaecptr sra;
	sra = m68k_areg(regs, 7);
	uae_s16 sr = get_word_jit(sra);
	m68k_areg(regs, 7) += 2;
	uaecptr pca;
	pca = m68k_areg(regs, 7);
	uae_s32 pc = get_long_jit(pca);
	m68k_areg(regs, 7) += 4;
	regs.sr &= 0xFF00; sr &= 0xFF;
	regs.sr |= sr;
	MakeFromSR();
	m68k_setpc_j(pc);
	if (m68k_getpc () & 1) {
		uaecptr faultpc = m68k_getpc ();
		m68k_setpc_j(oldpc + 2);
		exception3_read_prefetch_only(opcode, faultpc);
		return 16 * CYCLE_UNIT / 2;
	}
	return 20 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* JSR.L (An) */
uae_u32 REGPARAM2 op_4e90_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uaecptr oldpc = m68k_getpc ();
	uaecptr nextpc = oldpc + 2;
	if (srca & 1) {
		m68k_incpc(2);
		exception3_read_prefetch_only(opcode, srca);
		return 4 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(srca);
	m68k_areg(regs, 7) -= 4;
	if (m68k_areg(regs, 7) & 1) {
		m68k_setpc_j(oldpc);
		m68k_incpc(2);
		exception3_write_access(opcode, m68k_areg(regs, 7), 1, m68k_areg(regs, 7) >> 16, 1);
		return 4 * CYCLE_UNIT / 2;
	}
	put_long_jit (m68k_areg(regs, 7), nextpc);
	return 16 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* JSR.L (d16,An) */
uae_u32 REGPARAM2 op_4ea8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	uaecptr nextpc = oldpc + 4;
	if (srca & 1) {
		m68k_incpc(2);
		exception3_read_prefetch_only(opcode, srca);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(srca);
	m68k_areg(regs, 7) -= 4;
	if (m68k_areg(regs, 7) & 1) {
		m68k_setpc_j(oldpc);
		m68k_incpc(4);
		exception3_write_access(opcode, m68k_areg(regs, 7), 1, m68k_areg(regs, 7) >> 16, 1);
		return 6 * CYCLE_UNIT / 2;
	}
	put_long_jit (m68k_areg(regs, 7), nextpc);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* JSR.L (d8,An,Xn) */
uae_u32 REGPARAM2 op_4eb0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uaecptr oldpc = m68k_getpc ();
	uaecptr nextpc = oldpc + 4;
	if (srca & 1) {
		m68k_incpc(2);
		exception3_read_prefetch_only(opcode, srca);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(srca);
	m68k_areg(regs, 7) -= 4;
	if (m68k_areg(regs, 7) & 1) {
		m68k_setpc_j(oldpc);
		m68k_incpc(4);
		exception3_write_access(opcode, m68k_areg(regs, 7), 1, m68k_areg(regs, 7) >> 16, 1);
		return 10 * CYCLE_UNIT / 2;
	}
	put_long_jit (m68k_areg(regs, 7), nextpc);
	return 22 * CYCLE_UNIT / 2;
}
/* 4 4,0 B */

/* JSR.L (xxx).W */
uae_u32 REGPARAM2 op_4eb8_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	uaecptr nextpc = oldpc + 4;
	if (srca & 1) {
		m68k_incpc(4);
		exception3_read_prefetch_only(opcode, srca);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(srca);
	m68k_areg(regs, 7) -= 4;
	if (m68k_areg(regs, 7) & 1) {
		m68k_setpc_j(oldpc);
		m68k_incpc(4);
		exception3_write_access(opcode, m68k_areg(regs, 7), 1, m68k_areg(regs, 7) >> 16, 1);
		return 6 * CYCLE_UNIT / 2;
	}
	put_long_jit (m68k_areg(regs, 7), nextpc);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* JSR.L (xxx).L */
uae_u32 REGPARAM2 op_4eb9_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uaecptr oldpc = m68k_getpc ();
	uaecptr nextpc = oldpc + 6;
	if (srca & 1) {
		m68k_incpc(6);
		exception3_read_prefetch_only(opcode, srca);
		return 8 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(srca);
	m68k_areg(regs, 7) -= 4;
	if (m68k_areg(regs, 7) & 1) {
		m68k_setpc_j(oldpc);
		m68k_incpc(6);
		exception3_write_access(opcode, m68k_areg(regs, 7), 1, m68k_areg(regs, 7) >> 16, 1);
		return 8 * CYCLE_UNIT / 2;
	}
	put_long_jit (m68k_areg(regs, 7), nextpc);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0 B */

/* JSR.L (d16,PC) */
uae_u32 REGPARAM2 op_4eba_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	uaecptr nextpc = oldpc + 4;
	if (srca & 1) {
		m68k_incpc(2);
		exception3_read_prefetch_only(opcode, srca);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(srca);
	m68k_areg(regs, 7) -= 4;
	if (m68k_areg(regs, 7) & 1) {
		m68k_setpc_j(oldpc);
		m68k_incpc(4);
		exception3_write_access(opcode, m68k_areg(regs, 7), 1, m68k_areg(regs, 7) >> 16, 1);
		return 6 * CYCLE_UNIT / 2;
	}
	put_long_jit (m68k_areg(regs, 7), nextpc);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* JSR.L (d8,PC,Xn) */
uae_u32 REGPARAM2 op_4ebb_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uaecptr oldpc = m68k_getpc ();
	uaecptr nextpc = oldpc + 4;
	if (srca & 1) {
		m68k_incpc(2);
		exception3_read_prefetch_only(opcode, srca);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(srca);
	m68k_areg(regs, 7) -= 4;
	if (m68k_areg(regs, 7) & 1) {
		m68k_setpc_j(oldpc);
		m68k_incpc(4);
		exception3_write_access(opcode, m68k_areg(regs, 7), 1, m68k_areg(regs, 7) >> 16, 1);
		return 10 * CYCLE_UNIT / 2;
	}
	put_long_jit (m68k_areg(regs, 7), nextpc);
	return 22 * CYCLE_UNIT / 2;
}
/* 4 4,0 B */

/* JMP.L (An) */
uae_u32 REGPARAM2 op_4ed0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	if (srca & 1) {
		m68k_incpc(2);
		exception3_read_prefetch_only(opcode, srca);
		return 4 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(srca);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* JMP.L (d16,An) */
uae_u32 REGPARAM2 op_4ee8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	if (srca & 1) {
		m68k_incpc(2);
		exception3_read_prefetch_only(opcode, srca);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(srca);
	return 10 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* JMP.L (d8,An,Xn) */
uae_u32 REGPARAM2 op_4ef0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	if (srca & 1) {
		m68k_incpc(2);
		exception3_read_prefetch_only(opcode, srca);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(srca);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 4,0 B */

/* JMP.L (xxx).W */
uae_u32 REGPARAM2 op_4ef8_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	if (srca & 1) {
		m68k_incpc(2);
		exception3_read_prefetch_only(opcode, srca);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(srca);
	return 10 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* JMP.L (xxx).L */
uae_u32 REGPARAM2 op_4ef9_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	if (srca & 1) {
		m68k_incpc(2);
		exception3_read_prefetch_only(opcode, srca);
		return 8 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(srca);
	return 12 * CYCLE_UNIT / 2;
}
/* 6 0,0 B */

/* JMP.L (d16,PC) */
uae_u32 REGPARAM2 op_4efa_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	if (srca & 1) {
		m68k_incpc(2);
		exception3_read_prefetch_only(opcode, srca);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(srca);
	return 10 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* JMP.L (d8,PC,Xn) */
uae_u32 REGPARAM2 op_4efb_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	if (srca & 1) {
		m68k_incpc(2);
		exception3_read_prefetch_only(opcode, srca);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(srca);
	return 14 * CYCLE_UNIT / 2;
}
/* 4 4,0 B */

/* ADDQ.L #<data>,Dn */
uae_u32 REGPARAM2 op_5080_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B Dn (T) */
uae_u32 REGPARAM2 op_50c0_45_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	int val = cctrue(0) ? 0xff : 0x00;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* DBcc.W Dn,#<data>.W (T) */
uae_u32 REGPARAM2 op_50c8_45_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue(0)) {
		m68k_incpc((uae_s32)offs + 2);
		if (offs & 1) {
			exception3_read_prefetch(opcode, m68k_getpc ());
			return 6 * CYCLE_UNIT / 2 + count_cycles;
		}
		if (src) {
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 2 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(oldpc + 4);
	if (!cctrue(0)) {
		m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
	}
	return 12 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0 B */

/* Scc.B (An) (T) */
uae_u32 REGPARAM2 op_50d0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(0) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (An)+ (T) */
uae_u32 REGPARAM2 op_50d8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	int val = cctrue(0) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B -(An) (T) */
uae_u32 REGPARAM2 op_50e0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	int val = cctrue(0) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (d16,An) (T) */
uae_u32 REGPARAM2 op_50e8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(0) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (d8,An,Xn) (T) */
uae_u32 REGPARAM2 op_50f0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(0) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* Scc.B (xxx).W (T) */
uae_u32 REGPARAM2 op_50f8_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(0) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (xxx).L (T) */
uae_u32 REGPARAM2 op_50f9_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(0) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* SUBQ.L #<data>,Dn */
uae_u32 REGPARAM2 op_5180_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = imm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
	uae_u32 src = srcreg;
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B Dn (F) */
uae_u32 REGPARAM2 op_51c0_45_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	int val = cctrue(1) ? 0xff : 0x00;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* DBcc.W Dn,#<data>.W (F) */
uae_u32 REGPARAM2 op_51c8_45_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue(1)) {
		m68k_incpc((uae_s32)offs + 2);
		if (offs & 1) {
			exception3_read_prefetch(opcode, m68k_getpc ());
			return 6 * CYCLE_UNIT / 2 + count_cycles;
		}
		if (src) {
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 2 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(oldpc + 4);
	if (!cctrue(1)) {
		m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
	}
	return 12 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0 B */

/* Scc.B (An) (F) */
uae_u32 REGPARAM2 op_51d0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(1) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (An)+ (F) */
uae_u32 REGPARAM2 op_51d8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	int val = cctrue(1) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B -(An) (F) */
uae_u32 REGPARAM2 op_51e0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	int val = cctrue(1) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (d16,An) (F) */
uae_u32 REGPARAM2 op_51e8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(1) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (d8,An,Xn) (F) */
uae_u32 REGPARAM2 op_51f0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(1) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* Scc.B (xxx).W (F) */
uae_u32 REGPARAM2 op_51f8_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(1) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (xxx).L (F) */
uae_u32 REGPARAM2 op_51f9_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(1) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* Scc.B Dn (HI) */
uae_u32 REGPARAM2 op_52c0_45_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	int val = cctrue(2) ? 0xff : 0x00;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* DBcc.W Dn,#<data>.W (HI) */
uae_u32 REGPARAM2 op_52c8_45_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue(2)) {
		m68k_incpc((uae_s32)offs + 2);
		if (offs & 1) {
			exception3_read_prefetch(opcode, m68k_getpc ());
			return 6 * CYCLE_UNIT / 2 + count_cycles;
		}
		if (src) {
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 2 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(oldpc + 4);
	if (!cctrue(2)) {
		m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
	}
	return 12 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0 B */

/* Scc.B (An) (HI) */
uae_u32 REGPARAM2 op_52d0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(2) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (An)+ (HI) */
uae_u32 REGPARAM2 op_52d8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	int val = cctrue(2) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B -(An) (HI) */
uae_u32 REGPARAM2 op_52e0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	int val = cctrue(2) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (d16,An) (HI) */
uae_u32 REGPARAM2 op_52e8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(2) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (d8,An,Xn) (HI) */
uae_u32 REGPARAM2 op_52f0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(2) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* Scc.B (xxx).W (HI) */
uae_u32 REGPARAM2 op_52f8_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(2) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (xxx).L (HI) */
uae_u32 REGPARAM2 op_52f9_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(2) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* Scc.B Dn (LS) */
uae_u32 REGPARAM2 op_53c0_45_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	int val = cctrue(3) ? 0xff : 0x00;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* DBcc.W Dn,#<data>.W (LS) */
uae_u32 REGPARAM2 op_53c8_45_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue(3)) {
		m68k_incpc((uae_s32)offs + 2);
		if (offs & 1) {
			exception3_read_prefetch(opcode, m68k_getpc ());
			return 6 * CYCLE_UNIT / 2 + count_cycles;
		}
		if (src) {
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 2 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(oldpc + 4);
	if (!cctrue(3)) {
		m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
	}
	return 12 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0 B */

/* Scc.B (An) (LS) */
uae_u32 REGPARAM2 op_53d0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(3) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (An)+ (LS) */
uae_u32 REGPARAM2 op_53d8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	int val = cctrue(3) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B -(An) (LS) */
uae_u32 REGPARAM2 op_53e0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	int val = cctrue(3) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (d16,An) (LS) */
uae_u32 REGPARAM2 op_53e8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(3) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (d8,An,Xn) (LS) */
uae_u32 REGPARAM2 op_53f0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(3) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* Scc.B (xxx).W (LS) */
uae_u32 REGPARAM2 op_53f8_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(3) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (xxx).L (LS) */
uae_u32 REGPARAM2 op_53f9_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(3) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* Scc.B Dn (CC) */
uae_u32 REGPARAM2 op_54c0_45_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	int val = cctrue(4) ? 0xff : 0x00;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* DBcc.W Dn,#<data>.W (CC) */
uae_u32 REGPARAM2 op_54c8_45_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue(4)) {
		m68k_incpc((uae_s32)offs + 2);
		if (offs & 1) {
			exception3_read_prefetch(opcode, m68k_getpc ());
			return 6 * CYCLE_UNIT / 2 + count_cycles;
		}
		if (src) {
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 2 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(oldpc + 4);
	if (!cctrue(4)) {
		m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
	}
	return 12 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0 B */

/* Scc.B (An) (CC) */
uae_u32 REGPARAM2 op_54d0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(4) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (An)+ (CC) */
uae_u32 REGPARAM2 op_54d8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	int val = cctrue(4) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B -(An) (CC) */
uae_u32 REGPARAM2 op_54e0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	int val = cctrue(4) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (d16,An) (CC) */
uae_u32 REGPARAM2 op_54e8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(4) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (d8,An,Xn) (CC) */
uae_u32 REGPARAM2 op_54f0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(4) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* Scc.B (xxx).W (CC) */
uae_u32 REGPARAM2 op_54f8_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(4) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (xxx).L (CC) */
uae_u32 REGPARAM2 op_54f9_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(4) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* Scc.B Dn (CS) */
uae_u32 REGPARAM2 op_55c0_45_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	int val = cctrue(5) ? 0xff : 0x00;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* DBcc.W Dn,#<data>.W (CS) */
uae_u32 REGPARAM2 op_55c8_45_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue(5)) {
		m68k_incpc((uae_s32)offs + 2);
		if (offs & 1) {
			exception3_read_prefetch(opcode, m68k_getpc ());
			return 6 * CYCLE_UNIT / 2 + count_cycles;
		}
		if (src) {
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 2 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(oldpc + 4);
	if (!cctrue(5)) {
		m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
	}
	return 12 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0 B */

/* Scc.B (An) (CS) */
uae_u32 REGPARAM2 op_55d0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(5) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (An)+ (CS) */
uae_u32 REGPARAM2 op_55d8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	int val = cctrue(5) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B -(An) (CS) */
uae_u32 REGPARAM2 op_55e0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	int val = cctrue(5) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (d16,An) (CS) */
uae_u32 REGPARAM2 op_55e8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(5) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (d8,An,Xn) (CS) */
uae_u32 REGPARAM2 op_55f0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(5) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* Scc.B (xxx).W (CS) */
uae_u32 REGPARAM2 op_55f8_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(5) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (xxx).L (CS) */
uae_u32 REGPARAM2 op_55f9_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(5) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* Scc.B Dn (NE) */
uae_u32 REGPARAM2 op_56c0_45_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	int val = cctrue(6) ? 0xff : 0x00;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* DBcc.W Dn,#<data>.W (NE) */
uae_u32 REGPARAM2 op_56c8_45_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue(6)) {
		m68k_incpc((uae_s32)offs + 2);
		if (offs & 1) {
			exception3_read_prefetch(opcode, m68k_getpc ());
			return 6 * CYCLE_UNIT / 2 + count_cycles;
		}
		if (src) {
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 2 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(oldpc + 4);
	if (!cctrue(6)) {
		m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
	}
	return 12 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0 B */

/* Scc.B (An) (NE) */
uae_u32 REGPARAM2 op_56d0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(6) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (An)+ (NE) */
uae_u32 REGPARAM2 op_56d8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	int val = cctrue(6) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B -(An) (NE) */
uae_u32 REGPARAM2 op_56e0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	int val = cctrue(6) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (d16,An) (NE) */
uae_u32 REGPARAM2 op_56e8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(6) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (d8,An,Xn) (NE) */
uae_u32 REGPARAM2 op_56f0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(6) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* Scc.B (xxx).W (NE) */
uae_u32 REGPARAM2 op_56f8_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(6) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (xxx).L (NE) */
uae_u32 REGPARAM2 op_56f9_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(6) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* Scc.B Dn (EQ) */
uae_u32 REGPARAM2 op_57c0_45_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	int val = cctrue(7) ? 0xff : 0x00;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* DBcc.W Dn,#<data>.W (EQ) */
uae_u32 REGPARAM2 op_57c8_45_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue(7)) {
		m68k_incpc((uae_s32)offs + 2);
		if (offs & 1) {
			exception3_read_prefetch(opcode, m68k_getpc ());
			return 6 * CYCLE_UNIT / 2 + count_cycles;
		}
		if (src) {
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 2 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(oldpc + 4);
	if (!cctrue(7)) {
		m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
	}
	return 12 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0 B */

/* Scc.B (An) (EQ) */
uae_u32 REGPARAM2 op_57d0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(7) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (An)+ (EQ) */
uae_u32 REGPARAM2 op_57d8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	int val = cctrue(7) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B -(An) (EQ) */
uae_u32 REGPARAM2 op_57e0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	int val = cctrue(7) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (d16,An) (EQ) */
uae_u32 REGPARAM2 op_57e8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(7) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (d8,An,Xn) (EQ) */
uae_u32 REGPARAM2 op_57f0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(7) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* Scc.B (xxx).W (EQ) */
uae_u32 REGPARAM2 op_57f8_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(7) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (xxx).L (EQ) */
uae_u32 REGPARAM2 op_57f9_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(7) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* Scc.B Dn (VC) */
uae_u32 REGPARAM2 op_58c0_45_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	int val = cctrue(8) ? 0xff : 0x00;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* DBcc.W Dn,#<data>.W (VC) */
uae_u32 REGPARAM2 op_58c8_45_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue(8)) {
		m68k_incpc((uae_s32)offs + 2);
		if (offs & 1) {
			exception3_read_prefetch(opcode, m68k_getpc ());
			return 6 * CYCLE_UNIT / 2 + count_cycles;
		}
		if (src) {
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 2 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(oldpc + 4);
	if (!cctrue(8)) {
		m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
	}
	return 12 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0 B */

/* Scc.B (An) (VC) */
uae_u32 REGPARAM2 op_58d0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(8) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (An)+ (VC) */
uae_u32 REGPARAM2 op_58d8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	int val = cctrue(8) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B -(An) (VC) */
uae_u32 REGPARAM2 op_58e0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	int val = cctrue(8) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (d16,An) (VC) */
uae_u32 REGPARAM2 op_58e8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(8) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (d8,An,Xn) (VC) */
uae_u32 REGPARAM2 op_58f0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(8) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* Scc.B (xxx).W (VC) */
uae_u32 REGPARAM2 op_58f8_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(8) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (xxx).L (VC) */
uae_u32 REGPARAM2 op_58f9_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(8) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* Scc.B Dn (VS) */
uae_u32 REGPARAM2 op_59c0_45_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	int val = cctrue(9) ? 0xff : 0x00;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* DBcc.W Dn,#<data>.W (VS) */
uae_u32 REGPARAM2 op_59c8_45_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue(9)) {
		m68k_incpc((uae_s32)offs + 2);
		if (offs & 1) {
			exception3_read_prefetch(opcode, m68k_getpc ());
			return 6 * CYCLE_UNIT / 2 + count_cycles;
		}
		if (src) {
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 2 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(oldpc + 4);
	if (!cctrue(9)) {
		m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
	}
	return 12 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0 B */

/* Scc.B (An) (VS) */
uae_u32 REGPARAM2 op_59d0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(9) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (An)+ (VS) */
uae_u32 REGPARAM2 op_59d8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	int val = cctrue(9) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B -(An) (VS) */
uae_u32 REGPARAM2 op_59e0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	int val = cctrue(9) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (d16,An) (VS) */
uae_u32 REGPARAM2 op_59e8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(9) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (d8,An,Xn) (VS) */
uae_u32 REGPARAM2 op_59f0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(9) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* Scc.B (xxx).W (VS) */
uae_u32 REGPARAM2 op_59f8_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(9) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (xxx).L (VS) */
uae_u32 REGPARAM2 op_59f9_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(9) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* Scc.B Dn (PL) */
uae_u32 REGPARAM2 op_5ac0_45_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	int val = cctrue(10) ? 0xff : 0x00;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* DBcc.W Dn,#<data>.W (PL) */
uae_u32 REGPARAM2 op_5ac8_45_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue(10)) {
		m68k_incpc((uae_s32)offs + 2);
		if (offs & 1) {
			exception3_read_prefetch(opcode, m68k_getpc ());
			return 6 * CYCLE_UNIT / 2 + count_cycles;
		}
		if (src) {
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 2 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(oldpc + 4);
	if (!cctrue(10)) {
		m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
	}
	return 12 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0 B */

/* Scc.B (An) (PL) */
uae_u32 REGPARAM2 op_5ad0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(10) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (An)+ (PL) */
uae_u32 REGPARAM2 op_5ad8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	int val = cctrue(10) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B -(An) (PL) */
uae_u32 REGPARAM2 op_5ae0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	int val = cctrue(10) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (d16,An) (PL) */
uae_u32 REGPARAM2 op_5ae8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(10) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (d8,An,Xn) (PL) */
uae_u32 REGPARAM2 op_5af0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(10) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* Scc.B (xxx).W (PL) */
uae_u32 REGPARAM2 op_5af8_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(10) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (xxx).L (PL) */
uae_u32 REGPARAM2 op_5af9_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(10) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* Scc.B Dn (MI) */
uae_u32 REGPARAM2 op_5bc0_45_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	int val = cctrue(11) ? 0xff : 0x00;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* DBcc.W Dn,#<data>.W (MI) */
uae_u32 REGPARAM2 op_5bc8_45_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue(11)) {
		m68k_incpc((uae_s32)offs + 2);
		if (offs & 1) {
			exception3_read_prefetch(opcode, m68k_getpc ());
			return 6 * CYCLE_UNIT / 2 + count_cycles;
		}
		if (src) {
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 2 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(oldpc + 4);
	if (!cctrue(11)) {
		m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
	}
	return 12 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0 B */

/* Scc.B (An) (MI) */
uae_u32 REGPARAM2 op_5bd0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(11) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (An)+ (MI) */
uae_u32 REGPARAM2 op_5bd8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	int val = cctrue(11) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B -(An) (MI) */
uae_u32 REGPARAM2 op_5be0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	int val = cctrue(11) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (d16,An) (MI) */
uae_u32 REGPARAM2 op_5be8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(11) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (d8,An,Xn) (MI) */
uae_u32 REGPARAM2 op_5bf0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(11) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* Scc.B (xxx).W (MI) */
uae_u32 REGPARAM2 op_5bf8_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(11) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (xxx).L (MI) */
uae_u32 REGPARAM2 op_5bf9_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(11) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* Scc.B Dn (GE) */
uae_u32 REGPARAM2 op_5cc0_45_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	int val = cctrue(12) ? 0xff : 0x00;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* DBcc.W Dn,#<data>.W (GE) */
uae_u32 REGPARAM2 op_5cc8_45_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue(12)) {
		m68k_incpc((uae_s32)offs + 2);
		if (offs & 1) {
			exception3_read_prefetch(opcode, m68k_getpc ());
			return 6 * CYCLE_UNIT / 2 + count_cycles;
		}
		if (src) {
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 2 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(oldpc + 4);
	if (!cctrue(12)) {
		m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
	}
	return 12 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0 B */

/* Scc.B (An) (GE) */
uae_u32 REGPARAM2 op_5cd0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(12) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (An)+ (GE) */
uae_u32 REGPARAM2 op_5cd8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	int val = cctrue(12) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B -(An) (GE) */
uae_u32 REGPARAM2 op_5ce0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	int val = cctrue(12) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (d16,An) (GE) */
uae_u32 REGPARAM2 op_5ce8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(12) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (d8,An,Xn) (GE) */
uae_u32 REGPARAM2 op_5cf0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(12) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* Scc.B (xxx).W (GE) */
uae_u32 REGPARAM2 op_5cf8_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(12) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (xxx).L (GE) */
uae_u32 REGPARAM2 op_5cf9_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(12) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* Scc.B Dn (LT) */
uae_u32 REGPARAM2 op_5dc0_45_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	int val = cctrue(13) ? 0xff : 0x00;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* DBcc.W Dn,#<data>.W (LT) */
uae_u32 REGPARAM2 op_5dc8_45_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue(13)) {
		m68k_incpc((uae_s32)offs + 2);
		if (offs & 1) {
			exception3_read_prefetch(opcode, m68k_getpc ());
			return 6 * CYCLE_UNIT / 2 + count_cycles;
		}
		if (src) {
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 2 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(oldpc + 4);
	if (!cctrue(13)) {
		m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
	}
	return 12 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0 B */

/* Scc.B (An) (LT) */
uae_u32 REGPARAM2 op_5dd0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(13) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (An)+ (LT) */
uae_u32 REGPARAM2 op_5dd8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	int val = cctrue(13) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B -(An) (LT) */
uae_u32 REGPARAM2 op_5de0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	int val = cctrue(13) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (d16,An) (LT) */
uae_u32 REGPARAM2 op_5de8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(13) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (d8,An,Xn) (LT) */
uae_u32 REGPARAM2 op_5df0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(13) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* Scc.B (xxx).W (LT) */
uae_u32 REGPARAM2 op_5df8_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(13) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (xxx).L (LT) */
uae_u32 REGPARAM2 op_5df9_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(13) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* Scc.B Dn (GT) */
uae_u32 REGPARAM2 op_5ec0_45_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	int val = cctrue(14) ? 0xff : 0x00;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* DBcc.W Dn,#<data>.W (GT) */
uae_u32 REGPARAM2 op_5ec8_45_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue(14)) {
		m68k_incpc((uae_s32)offs + 2);
		if (offs & 1) {
			exception3_read_prefetch(opcode, m68k_getpc ());
			return 6 * CYCLE_UNIT / 2 + count_cycles;
		}
		if (src) {
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 2 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(oldpc + 4);
	if (!cctrue(14)) {
		m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
	}
	return 12 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0 B */

/* Scc.B (An) (GT) */
uae_u32 REGPARAM2 op_5ed0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(14) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (An)+ (GT) */
uae_u32 REGPARAM2 op_5ed8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	int val = cctrue(14) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B -(An) (GT) */
uae_u32 REGPARAM2 op_5ee0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	int val = cctrue(14) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (d16,An) (GT) */
uae_u32 REGPARAM2 op_5ee8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(14) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (d8,An,Xn) (GT) */
uae_u32 REGPARAM2 op_5ef0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(14) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* Scc.B (xxx).W (GT) */
uae_u32 REGPARAM2 op_5ef8_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(14) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (xxx).L (GT) */
uae_u32 REGPARAM2 op_5ef9_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(14) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* Scc.B Dn (LE) */
uae_u32 REGPARAM2 op_5fc0_45_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s8 src = m68k_dreg(regs, srcreg);
	int val = cctrue(15) ? 0xff : 0x00;
	m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xff) | ((val) & 0xff);
	count_cycles += ((val ? 2 : 0)) * CYCLE_UNIT / 2;
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2 + count_cycles;
}
/* 2 0,0   */

/* DBcc.W Dn,#<data>.W (LE) */
uae_u32 REGPARAM2 op_5fc8_45_ff(uae_u32 opcode)
{
	int count_cycles = 0;
	uae_u32 srcreg = (opcode & 7);
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 offs = get_diword (2);
	uaecptr oldpc = m68k_getpc ();
	if (!cctrue(15)) {
		m68k_incpc((uae_s32)offs + 2);
		if (offs & 1) {
			exception3_read_prefetch(opcode, m68k_getpc ());
			return 6 * CYCLE_UNIT / 2 + count_cycles;
		}
		if (src) {
			m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
			return 10 * CYCLE_UNIT / 2 + count_cycles;
		}
		count_cycles += 2 * CYCLE_UNIT / 2;
	}
	m68k_setpc_j(oldpc + 4);
	if (!cctrue(15)) {
		m68k_dreg(regs, srcreg) = (m68k_dreg(regs, srcreg) & ~0xffff) | (((src - 1)) & 0xffff);
	}
	return 12 * CYCLE_UNIT / 2 + count_cycles;
}
/* 4 0,0 B */

/* Scc.B (An) (LE) */
uae_u32 REGPARAM2 op_5fd0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(15) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (An)+ (LE) */
uae_u32 REGPARAM2 op_5fd8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) += areg_byteinc[srcreg];
	int val = cctrue(15) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B -(An) (LE) */
uae_u32 REGPARAM2 op_5fe0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	int val = cctrue(15) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Scc.B (d16,An) (LE) */
uae_u32 REGPARAM2 op_5fe8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(15) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (d8,An,Xn) (LE) */
uae_u32 REGPARAM2 op_5ff0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(15) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* Scc.B (xxx).W (LE) */
uae_u32 REGPARAM2 op_5ff8_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(15) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(4);
	return 16 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* Scc.B (xxx).L (LE) */
uae_u32 REGPARAM2 op_5ff9_45_ff(uae_u32 opcode)
{
	uaecptr srca;
	srca = get_dilong(2);
	uae_s8 src = get_byte_jit(srca);
	int val = cctrue(15) ? 0xff : 0x00;
	put_byte_jit(srca, val);
	m68k_incpc(6);
	return 20 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* Bcc.W #<data>.W (T) */
uae_u32 REGPARAM2 op_6000_45_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	if (cctrue(0)) {
		if (src & 1) {
			uaecptr addr = m68k_getpc () + (uae_s32)src + 2;
			exception3_read_prefetch(opcode, addr);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* BccQ.B #<data> (T) */
uae_u32 REGPARAM2 op_6001_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
	uae_u32 src = srcreg;
	if (cctrue(0)) {
		if (src & 1) {
			uaecptr addr = m68k_getpc () + (uae_s32)src + 2;
			exception3_read_prefetch(opcode, addr);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* Bcc.L #<data>.L (T) */
uae_u32 REGPARAM2 op_60ff_45_ff(uae_u32 opcode)
{
	if (cctrue(0)) {
		exception3_read_prefetch(opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* BSR.W #<data>.W */
uae_u32 REGPARAM2 op_6100_45_ff(uae_u32 opcode)
{
	uae_s32 s;
	uae_s16 src = get_diword (2);
	s = (uae_s32)src + 2;
	uaecptr oldpc = m68k_getpc ();
	uaecptr nextpc = oldpc + 4;
	if (m68k_areg(regs, 7) & 1) {
		m68k_areg(regs, 7) -= 4;
		m68k_incpc(2);
		exception3_write_access(opcode, m68k_areg(regs, 7), sz_word, oldpc, 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_do_bsr(nextpc, s);
	if (m68k_getpc () & 1) {
		uaecptr addr = m68k_getpc ();
		m68k_incpc(-2);
		exception3_read_prefetch(opcode, addr);
		return 14 * CYCLE_UNIT / 2;
	}
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* BSRQ.B #<data> */
uae_u32 REGPARAM2 op_6101_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
	uae_s32 s;
	uae_u32 src = srcreg;
	s = (uae_s32)src + 2;
	uaecptr oldpc = m68k_getpc ();
	uaecptr nextpc = oldpc + 2;
	if (m68k_areg(regs, 7) & 1) {
		m68k_areg(regs, 7) -= 4;
		m68k_incpc(2);
		exception3_write_access(opcode, m68k_areg(regs, 7), sz_word, oldpc, 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_do_bsr(nextpc, s);
	if (m68k_getpc () & 1) {
		uaecptr addr = m68k_getpc ();
		m68k_incpc(-2);
		exception3_read_prefetch(opcode, addr);
		return 14 * CYCLE_UNIT / 2;
	}
	return 18 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* BSR.L #<data>.L */
uae_u32 REGPARAM2 op_61ff_45_ff(uae_u32 opcode)
{
	uae_s32 s;
	uae_u32 src = 0xffffffff;
	s = (uae_s32)src + 2;
	uaecptr oldpc = m68k_getpc ();
	uaecptr nextpc = oldpc + 2;
	if (m68k_areg(regs, 7) & 1) {
		m68k_areg(regs, 7) -= 4;
		m68k_incpc(2);
		exception3_write_access(opcode, m68k_areg(regs, 7), sz_word, oldpc, 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_do_bsr(nextpc, s);
	if (m68k_getpc () & 1) {
		uaecptr addr = m68k_getpc ();
		m68k_incpc(-2);
		exception3_read_prefetch(opcode, addr);
		return 14 * CYCLE_UNIT / 2;
	}
	return 18 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* Bcc.W #<data>.W (HI) */
uae_u32 REGPARAM2 op_6200_45_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	if (cctrue(2)) {
		if (src & 1) {
			uaecptr addr = m68k_getpc () + (uae_s32)src + 2;
			exception3_read_prefetch(opcode, addr);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* BccQ.B #<data> (HI) */
uae_u32 REGPARAM2 op_6201_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
	uae_u32 src = srcreg;
	if (cctrue(2)) {
		if (src & 1) {
			uaecptr addr = m68k_getpc () + (uae_s32)src + 2;
			exception3_read_prefetch(opcode, addr);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* Bcc.L #<data>.L (HI) */
uae_u32 REGPARAM2 op_62ff_45_ff(uae_u32 opcode)
{
	if (cctrue(2)) {
		exception3_read_prefetch(opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Bcc.W #<data>.W (LS) */
uae_u32 REGPARAM2 op_6300_45_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	if (cctrue(3)) {
		if (src & 1) {
			uaecptr addr = m68k_getpc () + (uae_s32)src + 2;
			exception3_read_prefetch(opcode, addr);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* BccQ.B #<data> (LS) */
uae_u32 REGPARAM2 op_6301_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
	uae_u32 src = srcreg;
	if (cctrue(3)) {
		if (src & 1) {
			uaecptr addr = m68k_getpc () + (uae_s32)src + 2;
			exception3_read_prefetch(opcode, addr);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* Bcc.L #<data>.L (LS) */
uae_u32 REGPARAM2 op_63ff_45_ff(uae_u32 opcode)
{
	if (cctrue(3)) {
		exception3_read_prefetch(opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Bcc.W #<data>.W (CC) */
uae_u32 REGPARAM2 op_6400_45_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	if (cctrue(4)) {
		if (src & 1) {
			uaecptr addr = m68k_getpc () + (uae_s32)src + 2;
			exception3_read_prefetch(opcode, addr);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* BccQ.B #<data> (CC) */
uae_u32 REGPARAM2 op_6401_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
	uae_u32 src = srcreg;
	if (cctrue(4)) {
		if (src & 1) {
			uaecptr addr = m68k_getpc () + (uae_s32)src + 2;
			exception3_read_prefetch(opcode, addr);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* Bcc.L #<data>.L (CC) */
uae_u32 REGPARAM2 op_64ff_45_ff(uae_u32 opcode)
{
	if (cctrue(4)) {
		exception3_read_prefetch(opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Bcc.W #<data>.W (CS) */
uae_u32 REGPARAM2 op_6500_45_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	if (cctrue(5)) {
		if (src & 1) {
			uaecptr addr = m68k_getpc () + (uae_s32)src + 2;
			exception3_read_prefetch(opcode, addr);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* BccQ.B #<data> (CS) */
uae_u32 REGPARAM2 op_6501_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
	uae_u32 src = srcreg;
	if (cctrue(5)) {
		if (src & 1) {
			uaecptr addr = m68k_getpc () + (uae_s32)src + 2;
			exception3_read_prefetch(opcode, addr);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* Bcc.L #<data>.L (CS) */
uae_u32 REGPARAM2 op_65ff_45_ff(uae_u32 opcode)
{
	if (cctrue(5)) {
		exception3_read_prefetch(opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Bcc.W #<data>.W (NE) */
uae_u32 REGPARAM2 op_6600_45_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	if (cctrue(6)) {
		if (src & 1) {
			uaecptr addr = m68k_getpc () + (uae_s32)src + 2;
			exception3_read_prefetch(opcode, addr);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* BccQ.B #<data> (NE) */
uae_u32 REGPARAM2 op_6601_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
	uae_u32 src = srcreg;
	if (cctrue(6)) {
		if (src & 1) {
			uaecptr addr = m68k_getpc () + (uae_s32)src + 2;
			exception3_read_prefetch(opcode, addr);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* Bcc.L #<data>.L (NE) */
uae_u32 REGPARAM2 op_66ff_45_ff(uae_u32 opcode)
{
	if (cctrue(6)) {
		exception3_read_prefetch(opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Bcc.W #<data>.W (EQ) */
uae_u32 REGPARAM2 op_6700_45_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	if (cctrue(7)) {
		if (src & 1) {
			uaecptr addr = m68k_getpc () + (uae_s32)src + 2;
			exception3_read_prefetch(opcode, addr);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* BccQ.B #<data> (EQ) */
uae_u32 REGPARAM2 op_6701_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
	uae_u32 src = srcreg;
	if (cctrue(7)) {
		if (src & 1) {
			uaecptr addr = m68k_getpc () + (uae_s32)src + 2;
			exception3_read_prefetch(opcode, addr);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* Bcc.L #<data>.L (EQ) */
uae_u32 REGPARAM2 op_67ff_45_ff(uae_u32 opcode)
{
	if (cctrue(7)) {
		exception3_read_prefetch(opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Bcc.W #<data>.W (VC) */
uae_u32 REGPARAM2 op_6800_45_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	if (cctrue(8)) {
		if (src & 1) {
			uaecptr addr = m68k_getpc () + (uae_s32)src + 2;
			exception3_read_prefetch(opcode, addr);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* BccQ.B #<data> (VC) */
uae_u32 REGPARAM2 op_6801_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
	uae_u32 src = srcreg;
	if (cctrue(8)) {
		if (src & 1) {
			uaecptr addr = m68k_getpc () + (uae_s32)src + 2;
			exception3_read_prefetch(opcode, addr);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* Bcc.L #<data>.L (VC) */
uae_u32 REGPARAM2 op_68ff_45_ff(uae_u32 opcode)
{
	if (cctrue(8)) {
		exception3_read_prefetch(opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Bcc.W #<data>.W (VS) */
uae_u32 REGPARAM2 op_6900_45_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	if (cctrue(9)) {
		if (src & 1) {
			uaecptr addr = m68k_getpc () + (uae_s32)src + 2;
			exception3_read_prefetch(opcode, addr);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* BccQ.B #<data> (VS) */
uae_u32 REGPARAM2 op_6901_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
	uae_u32 src = srcreg;
	if (cctrue(9)) {
		if (src & 1) {
			uaecptr addr = m68k_getpc () + (uae_s32)src + 2;
			exception3_read_prefetch(opcode, addr);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* Bcc.L #<data>.L (VS) */
uae_u32 REGPARAM2 op_69ff_45_ff(uae_u32 opcode)
{
	if (cctrue(9)) {
		exception3_read_prefetch(opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Bcc.W #<data>.W (PL) */
uae_u32 REGPARAM2 op_6a00_45_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	if (cctrue(10)) {
		if (src & 1) {
			uaecptr addr = m68k_getpc () + (uae_s32)src + 2;
			exception3_read_prefetch(opcode, addr);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* BccQ.B #<data> (PL) */
uae_u32 REGPARAM2 op_6a01_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
	uae_u32 src = srcreg;
	if (cctrue(10)) {
		if (src & 1) {
			uaecptr addr = m68k_getpc () + (uae_s32)src + 2;
			exception3_read_prefetch(opcode, addr);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* Bcc.L #<data>.L (PL) */
uae_u32 REGPARAM2 op_6aff_45_ff(uae_u32 opcode)
{
	if (cctrue(10)) {
		exception3_read_prefetch(opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Bcc.W #<data>.W (MI) */
uae_u32 REGPARAM2 op_6b00_45_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	if (cctrue(11)) {
		if (src & 1) {
			uaecptr addr = m68k_getpc () + (uae_s32)src + 2;
			exception3_read_prefetch(opcode, addr);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* BccQ.B #<data> (MI) */
uae_u32 REGPARAM2 op_6b01_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
	uae_u32 src = srcreg;
	if (cctrue(11)) {
		if (src & 1) {
			uaecptr addr = m68k_getpc () + (uae_s32)src + 2;
			exception3_read_prefetch(opcode, addr);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* Bcc.L #<data>.L (MI) */
uae_u32 REGPARAM2 op_6bff_45_ff(uae_u32 opcode)
{
	if (cctrue(11)) {
		exception3_read_prefetch(opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Bcc.W #<data>.W (GE) */
uae_u32 REGPARAM2 op_6c00_45_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	if (cctrue(12)) {
		if (src & 1) {
			uaecptr addr = m68k_getpc () + (uae_s32)src + 2;
			exception3_read_prefetch(opcode, addr);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* BccQ.B #<data> (GE) */
uae_u32 REGPARAM2 op_6c01_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
	uae_u32 src = srcreg;
	if (cctrue(12)) {
		if (src & 1) {
			uaecptr addr = m68k_getpc () + (uae_s32)src + 2;
			exception3_read_prefetch(opcode, addr);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* Bcc.L #<data>.L (GE) */
uae_u32 REGPARAM2 op_6cff_45_ff(uae_u32 opcode)
{
	if (cctrue(12)) {
		exception3_read_prefetch(opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Bcc.W #<data>.W (LT) */
uae_u32 REGPARAM2 op_6d00_45_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	if (cctrue(13)) {
		if (src & 1) {
			uaecptr addr = m68k_getpc () + (uae_s32)src + 2;
			exception3_read_prefetch(opcode, addr);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* BccQ.B #<data> (LT) */
uae_u32 REGPARAM2 op_6d01_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
	uae_u32 src = srcreg;
	if (cctrue(13)) {
		if (src & 1) {
			uaecptr addr = m68k_getpc () + (uae_s32)src + 2;
			exception3_read_prefetch(opcode, addr);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* Bcc.L #<data>.L (LT) */
uae_u32 REGPARAM2 op_6dff_45_ff(uae_u32 opcode)
{
	if (cctrue(13)) {
		exception3_read_prefetch(opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Bcc.W #<data>.W (GT) */
uae_u32 REGPARAM2 op_6e00_45_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	if (cctrue(14)) {
		if (src & 1) {
			uaecptr addr = m68k_getpc () + (uae_s32)src + 2;
			exception3_read_prefetch(opcode, addr);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* BccQ.B #<data> (GT) */
uae_u32 REGPARAM2 op_6e01_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
	uae_u32 src = srcreg;
	if (cctrue(14)) {
		if (src & 1) {
			uaecptr addr = m68k_getpc () + (uae_s32)src + 2;
			exception3_read_prefetch(opcode, addr);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* Bcc.L #<data>.L (GT) */
uae_u32 REGPARAM2 op_6eff_45_ff(uae_u32 opcode)
{
	if (cctrue(14)) {
		exception3_read_prefetch(opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* Bcc.W #<data>.W (LE) */
uae_u32 REGPARAM2 op_6f00_45_ff(uae_u32 opcode)
{
	uae_s16 src = get_diword (2);
	if (cctrue(15)) {
		if (src & 1) {
			uaecptr addr = m68k_getpc () + (uae_s32)src + 2;
			exception3_read_prefetch(opcode, addr);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(4);
	return 12 * CYCLE_UNIT / 2;
}
/* 4 0,0 B */

/* BccQ.B #<data> (LE) */
uae_u32 REGPARAM2 op_6f01_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
	uae_u32 src = srcreg;
	if (cctrue(15)) {
		if (src & 1) {
			uaecptr addr = m68k_getpc () + (uae_s32)src + 2;
			exception3_read_prefetch(opcode, addr);
			return 6 * CYCLE_UNIT / 2;
		}
		m68k_incpc((uae_s32)src + 2);
		return 10 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0 B */

/* Bcc.L #<data>.L (LE) */
uae_u32 REGPARAM2 op_6fff_45_ff(uae_u32 opcode)
{
	if (cctrue(15)) {
		exception3_read_prefetch(opcode, m68k_getpc () + 1);
		return 6 * CYCLE_UNIT / 2;
	}
	m68k_incpc(2);
	return 12 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* OR.L Dn,Dn */
uae_u32 REGPARAM2 op_8080_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* OR.L (An),Dn */
uae_u32 REGPARAM2 op_8090_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* OR.L (An)+,Dn */
uae_u32 REGPARAM2 op_8098_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) += 4;
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* OR.L -(An),Dn */
uae_u32 REGPARAM2 op_80a0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(2);
	return 16 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* OR.L (d16,An),Dn */
uae_u32 REGPARAM2 op_80a8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* OR.L (d8,An,Xn),Dn */
uae_u32 REGPARAM2 op_80b0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* OR.L (xxx).W,Dn */
uae_u32 REGPARAM2 op_80b8_45_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* OR.L (xxx).L,Dn */
uae_u32 REGPARAM2 op_80b9_45_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* OR.L (d16,PC),Dn */
uae_u32 REGPARAM2 op_80ba_45_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* OR.L (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 op_80bb_45_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* OR.L #<data>.L,Dn */
uae_u32 REGPARAM2 op_80bc_45_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src;
	src = get_dilong(2);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src |= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* SUB.L Dn,Dn */
uae_u32 REGPARAM2 op_9080_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUB.L An,Dn */
uae_u32 REGPARAM2 op_9088_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src = m68k_areg(regs, srcreg);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUB.L (An),Dn */
uae_u32 REGPARAM2 op_9090_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUB.L (An)+,Dn */
uae_u32 REGPARAM2 op_9098_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) += 4;
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUB.L -(An),Dn */
uae_u32 REGPARAM2 op_90a0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 16 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUB.L (d16,An),Dn */
uae_u32 REGPARAM2 op_90a8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUB.L (d8,An,Xn),Dn */
uae_u32 REGPARAM2 op_90b0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* SUB.L (xxx).W,Dn */
uae_u32 REGPARAM2 op_90b8_45_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUB.L (xxx).L,Dn */
uae_u32 REGPARAM2 op_90b9_45_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* SUB.L (d16,PC),Dn */
uae_u32 REGPARAM2 op_90ba_45_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* SUB.L (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 op_90bb_45_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* SUB.L #<data>.L,Dn */
uae_u32 REGPARAM2 op_90bc_45_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src;
	src = get_dilong(2);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) - ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
	SET_CFLG(((uae_u32)(src)) > ((uae_u32)(dst)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* SUBX.B Dn,Dn */
uae_u32 REGPARAM2 op_9100_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = dst - src - (GET_XFLG () ? 1 : 0);
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s8)(newv)) == 0));
	SET_NFLG(((uae_s8)(newv)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUBX.B -(An),-(An) */
uae_u32 REGPARAM2 op_9108_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	uae_u32 newv = dst - src - (GET_XFLG () ? 1 : 0);
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s8)(newv)) == 0));
	SET_NFLG(((uae_s8)(newv)) < 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(2);
	return 18 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUBX.W Dn,Dn */
uae_u32 REGPARAM2 op_9140_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = dst - src - (GET_XFLG () ? 1 : 0);
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s16)(newv)) == 0));
	SET_NFLG(((uae_s16)(newv)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUBX.W -(An),-(An) */
uae_u32 REGPARAM2 op_9148_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	uae_s16 dst = get_word_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	uae_u32 newv = dst - src - (GET_XFLG () ? 1 : 0);
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s16)(newv)) == 0));
	SET_NFLG(((uae_s16)(newv)) < 0);
	put_word_jit(dsta, newv);
	m68k_incpc(2);
	return 18 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUBX.L Dn,Dn */
uae_u32 REGPARAM2 op_9180_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = dst - src - (GET_XFLG () ? 1 : 0);
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s32)(newv)) == 0));
	SET_NFLG(((uae_s32)(newv)) < 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* SUBX.L -(An),-(An) */
uae_u32 REGPARAM2 op_9188_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	uae_s32 dst = get_long_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	uae_u32 newv = dst - src - (GET_XFLG () ? 1 : 0);
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG((flgs ^ flgo) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s32)(newv)) == 0));
	SET_NFLG(((uae_s32)(newv)) < 0);
	put_long_jit(dsta, newv);
	m68k_incpc(2);
	return 30 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* EOR.L Dn,Dn */
uae_u32 REGPARAM2 op_b180_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src ^= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* AND.L Dn,Dn */
uae_u32 REGPARAM2 op_c080_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* AND.L (An),Dn */
uae_u32 REGPARAM2 op_c090_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* AND.L (An)+,Dn */
uae_u32 REGPARAM2 op_c098_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) += 4;
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* AND.L -(An),Dn */
uae_u32 REGPARAM2 op_c0a0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(2);
	return 16 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* AND.L (d16,An),Dn */
uae_u32 REGPARAM2 op_c0a8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* AND.L (d8,An,Xn),Dn */
uae_u32 REGPARAM2 op_c0b0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* AND.L (xxx).W,Dn */
uae_u32 REGPARAM2 op_c0b8_45_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* AND.L (xxx).L,Dn */
uae_u32 REGPARAM2 op_c0b9_45_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* AND.L (d16,PC),Dn */
uae_u32 REGPARAM2 op_c0ba_45_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* AND.L (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 op_c0bb_45_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* AND.L #<data>.L,Dn */
uae_u32 REGPARAM2 op_c0bc_45_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src;
	src = get_dilong(2);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	src &= dst;
	CLEAR_CZNV();
	SET_ZFLG(((uae_s32)(src)) == 0);
	SET_NFLG(((uae_s32)(src)) < 0);
	m68k_dreg(regs, dstreg) = (src);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* ADD.L Dn,Dn */
uae_u32 REGPARAM2 op_d080_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADD.L An,Dn */
uae_u32 REGPARAM2 op_d088_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src = m68k_areg(regs, srcreg);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADD.L (An),Dn */
uae_u32 REGPARAM2 op_d090_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADD.L (An)+,Dn */
uae_u32 REGPARAM2 op_d098_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg);
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) += 4;
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 14 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADD.L -(An),Dn */
uae_u32 REGPARAM2 op_d0a0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 16 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADD.L (d16,An),Dn */
uae_u32 REGPARAM2 op_d0a8_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) + (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADD.L (d8,An,Xn),Dn */
uae_u32 REGPARAM2 op_d0b0_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_disp_ea_000(m68k_areg(regs, srcreg), get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* ADD.L (xxx).W,Dn */
uae_u32 REGPARAM2 op_d0b8_45_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADD.L (xxx).L,Dn */
uae_u32 REGPARAM2 op_d0b9_45_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = get_dilong(2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(6);
	return 22 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* ADD.L (d16,PC),Dn */
uae_u32 REGPARAM2 op_d0ba_45_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)get_diword (2);
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 18 * CYCLE_UNIT / 2;
}
/* 4 0,0   */

/* ADD.L (d8,PC,Xn),Dn */
uae_u32 REGPARAM2 op_d0bb_45_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	uaecptr tmppc = m68k_getpc () + 2;
	srca = get_disp_ea_000(tmppc, get_diword (2));
	uae_s32 src = get_long_jit(srca);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(4);
	return 20 * CYCLE_UNIT / 2;
}
/* 4 4,0   */

/* ADD.L #<data>.L,Dn */
uae_u32 REGPARAM2 op_d0bc_45_ff(uae_u32 opcode)
{
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src;
	src = get_dilong(2);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = ((uae_u32)(dst)) + ((uae_u32)(src));
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_ZFLG(((uae_s32)(newv)) == 0);
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(((uae_u32)(~dst)) < ((uae_u32)(src)));
	COPY_CARRY();
	SET_NFLG(flgn != 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(6);
	return 16 * CYCLE_UNIT / 2;
}
/* 6 0,0   */

/* ADDX.B Dn,Dn */
uae_u32 REGPARAM2 op_d100_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s8 src = m68k_dreg(regs, srcreg);
	uae_s8 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = dst + src + (GET_XFLG() ? 1 : 0);
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgo) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s8)(newv)) == 0));
	SET_NFLG(((uae_s8)(newv)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xff) | ((newv) & 0xff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADDX.B -(An),-(An) */
uae_u32 REGPARAM2 op_d108_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - areg_byteinc[srcreg];
	uae_s8 src = get_byte_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - areg_byteinc[dstreg];
	uae_s8 dst = get_byte_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	uae_u32 newv = dst + src + (GET_XFLG() ? 1 : 0);
	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgo) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s8)(newv)) == 0));
	SET_NFLG(((uae_s8)(newv)) < 0);
	put_byte_jit(dsta, newv);
	m68k_incpc(2);
	return 18 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADDX.W Dn,Dn */
uae_u32 REGPARAM2 op_d140_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s16 src = m68k_dreg(regs, srcreg);
	uae_s16 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = dst + src + (GET_XFLG() ? 1 : 0);
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgo) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s16)(newv)) == 0));
	SET_NFLG(((uae_s16)(newv)) < 0);
	m68k_dreg(regs, dstreg) = (m68k_dreg(regs, dstreg) & ~0xffff) | ((newv) & 0xffff);
	m68k_incpc(2);
	return 4 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADDX.W -(An),-(An) */
uae_u32 REGPARAM2 op_d148_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 2;
	uae_s16 src = get_word_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 2;
	uae_s16 dst = get_word_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	uae_u32 newv = dst + src + (GET_XFLG() ? 1 : 0);
	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgo) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s16)(newv)) == 0));
	SET_NFLG(((uae_s16)(newv)) < 0);
	put_word_jit(dsta, newv);
	m68k_incpc(2);
	return 18 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADDX.L Dn,Dn */
uae_u32 REGPARAM2 op_d180_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uae_s32 src = m68k_dreg(regs, srcreg);
	uae_s32 dst = m68k_dreg(regs, dstreg);
	uae_u32 newv = dst + src + (GET_XFLG() ? 1 : 0);
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgo) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s32)(newv)) == 0));
	SET_NFLG(((uae_s32)(newv)) < 0);
	m68k_dreg(regs, dstreg) = (newv);
	m68k_incpc(2);
	return 8 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

/* ADDX.L -(An),-(An) */
uae_u32 REGPARAM2 op_d188_45_ff(uae_u32 opcode)
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
	uaecptr srca;
	srca = m68k_areg(regs, srcreg) - 4;
	uae_s32 src = get_long_jit(srca);
	m68k_areg(regs, srcreg) = srca;
	uaecptr dsta;
	dsta = m68k_areg(regs, dstreg) - 4;
	uae_s32 dst = get_long_jit(dsta);
	m68k_areg(regs, dstreg) = dsta;
	uae_u32 newv = dst + src + (GET_XFLG() ? 1 : 0);
	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	SET_VFLG((flgs ^ flgn) & (flgo ^ flgn));
	SET_CFLG(flgs ^ ((flgs ^ flgo) & (flgo ^ flgn)));
	COPY_CARRY();
	SET_ZFLG(GET_ZFLG() & (((uae_s32)(newv)) == 0));
	SET_NFLG(((uae_s32)(newv)) < 0);
	put_long_jit(dsta, newv);
	m68k_incpc(2);
	return 30 * CYCLE_UNIT / 2;
}
/* 2 0,0   */

