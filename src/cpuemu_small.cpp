#include "cpu_small.h"
#include "cputbl_small.h"
void xop_0_0(uae_u32 opcode) /* OR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xget_ibyte(2);
{	uae_s8 dst = xm68k_dreg(dstreg);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((src) & 0xff);
}}}xm68k_incpc(4);
}
void xop_10_0(uae_u32 opcode) /* OR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_18_0(uae_u32 opcode) /* OR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg(dstreg) += xareg_byteinc[dstreg];
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_20_0(uae_u32 opcode) /* OR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = xm68k_areg(dstreg) - xareg_byteinc[dstreg];
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg (dstreg) = dsta;
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_28_0(uae_u32 opcode) /* OR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(4);
{	uae_s8 dst = xget_byte(dsta);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_30_0(uae_u32 opcode) /* OR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xget_ibyte(2);
{xm68k_incpc(4);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s8 dst = xget_byte(dsta);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}}}
void xop_38_0(uae_u32 opcode) /* OR */
{
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(4);
{	uae_s8 dst = xget_byte(dsta);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_39_0(uae_u32 opcode) /* OR */
{
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = xget_ilong(4);
{	uae_s8 dst = xget_byte(dsta);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(8);
}
void xop_3c_0(uae_u32 opcode) /* ORSR */
{
{}}
void xop_40_0(uae_u32 opcode) /* OR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uae_s16 dst = xm68k_dreg(dstreg);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((src) & 0xffff);
}}}xm68k_incpc(4);
}
void xop_50_0(uae_u32 opcode) /* OR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s16 dst = xget_word(dsta);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_58_0(uae_u32 opcode) /* OR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s16 dst = xget_word(dsta);
	xm68k_areg(dstreg) += 2;
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_60_0(uae_u32 opcode) /* OR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg) - 2;
{	uae_s16 dst = xget_word(dsta);
	xm68k_areg (dstreg) = dsta;
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_68_0(uae_u32 opcode) /* OR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(4);
{	uae_s16 dst = xget_word(dsta);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_70_0(uae_u32 opcode) /* OR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{xm68k_incpc(4);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s16 dst = xget_word(dsta);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}}}
void xop_78_0(uae_u32 opcode) /* OR */
{
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(4);
{	uae_s16 dst = xget_word(dsta);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_79_0(uae_u32 opcode) /* OR */
{
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xget_ilong(4);
{	uae_s16 dst = xget_word(dsta);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(8);
}
void xop_7c_0(uae_u32 opcode) /* ORSR */
{
}
void xop_80_0(uae_u32 opcode) /* OR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xget_ilong(2);
{	uae_s32 dst = xm68k_dreg(dstreg);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xm68k_dreg(dstreg) = (src);
}}}xm68k_incpc(6);
}
void xop_90_0(uae_u32 opcode) /* OR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s32 dst = xget_long(dsta);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_98_0(uae_u32 opcode) /* OR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s32 dst = xget_long(dsta);
	xm68k_areg(dstreg) += 4;
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_a0_0(uae_u32 opcode) /* OR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = xm68k_areg(dstreg) - 4;
{	uae_s32 dst = xget_long(dsta);
	xm68k_areg (dstreg) = dsta;
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_a8_0(uae_u32 opcode) /* OR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(6);
{	uae_s32 dst = xget_long(dsta);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(8);
}
void xop_b0_0(uae_u32 opcode) /* OR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xget_ilong(2);
{xm68k_incpc(6);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s32 dst = xget_long(dsta);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}}}
void xop_b8_0(uae_u32 opcode) /* OR */
{
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(6);
{	uae_s32 dst = xget_long(dsta);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(8);
}
void xop_b9_0(uae_u32 opcode) /* OR */
{
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = xget_ilong(6);
{	uae_s32 dst = xget_long(dsta);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(10);
}
void xop_d0_0(uae_u32 opcode) /* CHK2 */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_e8_0(uae_u32 opcode) /* CHK2 */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_f0_0(uae_u32 opcode) /* CHK2 */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_f8_0(uae_u32 opcode) /* CHK2 */
{
{}}
void xop_f9_0(uae_u32 opcode) /* CHK2 */
{
{}}
void xop_fa_0(uae_u32 opcode) /* CHK2 */
{
	uae_u32 dstreg = 2;
{}}
void xop_fb_0(uae_u32 opcode) /* CHK2 */
{
	uae_u32 dstreg = 3;
{}}
void xop_100_0(uae_u32 opcode) /* BTST */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uae_s32 dst = xm68k_dreg(dstreg);
	src &= 31;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
}}}xm68k_incpc(2);
}
void xop_108_0(uae_u32 opcode) /* MVPMR */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr memp = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_u16 val = (xget_byte(memp) << 8) + xget_byte(memp + 2);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((val) & 0xffff);
}}xm68k_incpc(4);
}
void xop_110_0(uae_u32 opcode) /* BTST */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}xm68k_incpc(2);
}
void xop_118_0(uae_u32 opcode) /* BTST */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg(dstreg) += xareg_byteinc[dstreg];
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}xm68k_incpc(2);
}
void xop_120_0(uae_u32 opcode) /* BTST */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) - xareg_byteinc[dstreg];
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg (dstreg) = dsta;
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}xm68k_incpc(2);
}
void xop_128_0(uae_u32 opcode) /* BTST */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}xm68k_incpc(4);
}
void xop_130_0(uae_u32 opcode) /* BTST */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{xm68k_incpc(2);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}}}
void xop_138_0(uae_u32 opcode) /* BTST */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}xm68k_incpc(4);
}
void xop_139_0(uae_u32 opcode) /* BTST */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xget_ilong(2);
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}xm68k_incpc(6);
}
void xop_13a_0(uae_u32 opcode) /* BTST */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = 2;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_getpc () + 2;
	dsta += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}xm68k_incpc(4);
}
void xop_13b_0(uae_u32 opcode) /* BTST */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = 3;
{{	uae_s8 src = xm68k_dreg(srcreg);
{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr dsta = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}}}
void xop_13c_0(uae_u32 opcode) /* BTST */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uae_s8 dst = xget_ibyte(2);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
}}}xm68k_incpc(4);
}
void xop_140_0(uae_u32 opcode) /* BCHG */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uae_s32 dst = xm68k_dreg(dstreg);
	src &= 31;
	dst ^= (1 << src);
	XSET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	xm68k_dreg(dstreg) = (dst);
}}}xm68k_incpc(2);
}
void xop_148_0(uae_u32 opcode) /* MVPMR */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr memp = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_u32 val = (xget_byte(memp) << 24) + (xget_byte(memp + 2) << 16)
	      + (xget_byte(memp + 4) << 8) + xget_byte(memp + 6);
	xm68k_dreg(dstreg) = (val);
}}xm68k_incpc(4);
}
void xop_150_0(uae_u32 opcode) /* BCHG */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	dst ^= (1 << src);
	XSET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(2);
}
void xop_158_0(uae_u32 opcode) /* BCHG */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg(dstreg) += xareg_byteinc[dstreg];
	src &= 7;
	dst ^= (1 << src);
	XSET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(2);
}
void xop_160_0(uae_u32 opcode) /* BCHG */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) - xareg_byteinc[dstreg];
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg (dstreg) = dsta;
	src &= 7;
	dst ^= (1 << src);
	XSET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(2);
}
void xop_168_0(uae_u32 opcode) /* BCHG */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	dst ^= (1 << src);
	XSET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(4);
}
void xop_170_0(uae_u32 opcode) /* BCHG */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{xm68k_incpc(2);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	dst ^= (1 << src);
	XSET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	xput_byte(dsta,dst);
}}}}}}
void xop_178_0(uae_u32 opcode) /* BCHG */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	dst ^= (1 << src);
	XSET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(4);
}
void xop_179_0(uae_u32 opcode) /* BCHG */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xget_ilong(2);
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	dst ^= (1 << src);
	XSET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(6);
}
void xop_17a_0(uae_u32 opcode) /* BCHG */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = 2;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_getpc () + 2;
	dsta += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	dst ^= (1 << src);
	XSET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(4);
}
void xop_17b_0(uae_u32 opcode) /* BCHG */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = 3;
{{	uae_s8 src = xm68k_dreg(srcreg);
{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr dsta = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	dst ^= (1 << src);
	XSET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	xput_byte(dsta,dst);
}}}}}}
void xop_180_0(uae_u32 opcode) /* BCLR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uae_s32 dst = xm68k_dreg(dstreg);
	src &= 31;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	xm68k_dreg(dstreg) = (dst);
}}}xm68k_incpc(2);
}
void xop_188_0(uae_u32 opcode) /* MVPRM */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
	uaecptr memp = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
	xput_byte(memp, src >> 8); xput_byte(memp + 2, src);
}}xm68k_incpc(4);
}
void xop_190_0(uae_u32 opcode) /* BCLR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(2);
}
void xop_198_0(uae_u32 opcode) /* BCLR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg(dstreg) += xareg_byteinc[dstreg];
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(2);
}
void xop_1a0_0(uae_u32 opcode) /* BCLR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) - xareg_byteinc[dstreg];
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg (dstreg) = dsta;
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(2);
}
void xop_1a8_0(uae_u32 opcode) /* BCLR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(4);
}
void xop_1b0_0(uae_u32 opcode) /* BCLR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{xm68k_incpc(2);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	xput_byte(dsta,dst);
}}}}}}
void xop_1b8_0(uae_u32 opcode) /* BCLR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(4);
}
void xop_1b9_0(uae_u32 opcode) /* BCLR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xget_ilong(2);
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(6);
}
void xop_1ba_0(uae_u32 opcode) /* BCLR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = 2;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_getpc () + 2;
	dsta += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(4);
}
void xop_1bb_0(uae_u32 opcode) /* BCLR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = 3;
{{	uae_s8 src = xm68k_dreg(srcreg);
{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr dsta = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	xput_byte(dsta,dst);
}}}}}}
void xop_1c0_0(uae_u32 opcode) /* BSET */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uae_s32 dst = xm68k_dreg(dstreg);
	src &= 31;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	xm68k_dreg(dstreg) = (dst);
}}}xm68k_incpc(2);
}
void xop_1c8_0(uae_u32 opcode) /* MVPRM */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
	uaecptr memp = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
	xput_byte(memp, src >> 24); xput_byte(memp + 2, src >> 16);
	xput_byte(memp + 4, src >> 8); xput_byte(memp + 6, src);
}}xm68k_incpc(4);
}
void xop_1d0_0(uae_u32 opcode) /* BSET */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(2);
}
void xop_1d8_0(uae_u32 opcode) /* BSET */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg(dstreg) += xareg_byteinc[dstreg];
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(2);
}
void xop_1e0_0(uae_u32 opcode) /* BSET */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) - xareg_byteinc[dstreg];
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg (dstreg) = dsta;
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(2);
}
void xop_1e8_0(uae_u32 opcode) /* BSET */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(4);
}
void xop_1f0_0(uae_u32 opcode) /* BSET */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{xm68k_incpc(2);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	xput_byte(dsta,dst);
}}}}}}
void xop_1f8_0(uae_u32 opcode) /* BSET */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(4);
}
void xop_1f9_0(uae_u32 opcode) /* BSET */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xget_ilong(2);
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(6);
}
void xop_1fa_0(uae_u32 opcode) /* BSET */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = 2;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_getpc () + 2;
	dsta += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(4);
}
void xop_1fb_0(uae_u32 opcode) /* BSET */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = 3;
{{	uae_s8 src = xm68k_dreg(srcreg);
{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr dsta = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	xput_byte(dsta,dst);
}}}}}}
void xop_200_0(uae_u32 opcode) /* AND */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xget_ibyte(2);
{	uae_s8 dst = xm68k_dreg(dstreg);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((src) & 0xff);
}}}xm68k_incpc(4);
}
void xop_210_0(uae_u32 opcode) /* AND */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_218_0(uae_u32 opcode) /* AND */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg(dstreg) += xareg_byteinc[dstreg];
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_220_0(uae_u32 opcode) /* AND */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = xm68k_areg(dstreg) - xareg_byteinc[dstreg];
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg (dstreg) = dsta;
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_228_0(uae_u32 opcode) /* AND */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(4);
{	uae_s8 dst = xget_byte(dsta);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_230_0(uae_u32 opcode) /* AND */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xget_ibyte(2);
{xm68k_incpc(4);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s8 dst = xget_byte(dsta);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}}}
void xop_238_0(uae_u32 opcode) /* AND */
{
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(4);
{	uae_s8 dst = xget_byte(dsta);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_239_0(uae_u32 opcode) /* AND */
{
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = xget_ilong(4);
{	uae_s8 dst = xget_byte(dsta);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(8);
}
void xop_23c_0(uae_u32 opcode) /* ANDSR */
{
{}}
void xop_240_0(uae_u32 opcode) /* AND */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uae_s16 dst = xm68k_dreg(dstreg);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((src) & 0xffff);
}}}xm68k_incpc(4);
}
void xop_250_0(uae_u32 opcode) /* AND */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s16 dst = xget_word(dsta);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_258_0(uae_u32 opcode) /* AND */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s16 dst = xget_word(dsta);
	xm68k_areg(dstreg) += 2;
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_260_0(uae_u32 opcode) /* AND */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg) - 2;
{	uae_s16 dst = xget_word(dsta);
	xm68k_areg (dstreg) = dsta;
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_268_0(uae_u32 opcode) /* AND */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(4);
{	uae_s16 dst = xget_word(dsta);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_270_0(uae_u32 opcode) /* AND */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{xm68k_incpc(4);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s16 dst = xget_word(dsta);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}}}
void xop_278_0(uae_u32 opcode) /* AND */
{
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(4);
{	uae_s16 dst = xget_word(dsta);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_279_0(uae_u32 opcode) /* AND */
{
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xget_ilong(4);
{	uae_s16 dst = xget_word(dsta);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(8);
}
void xop_27c_0(uae_u32 opcode) /* ANDSR */
{
}
void xop_280_0(uae_u32 opcode) /* AND */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xget_ilong(2);
{	uae_s32 dst = xm68k_dreg(dstreg);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xm68k_dreg(dstreg) = (src);
}}}xm68k_incpc(6);
}
void xop_290_0(uae_u32 opcode) /* AND */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s32 dst = xget_long(dsta);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_298_0(uae_u32 opcode) /* AND */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s32 dst = xget_long(dsta);
	xm68k_areg(dstreg) += 4;
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_2a0_0(uae_u32 opcode) /* AND */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = xm68k_areg(dstreg) - 4;
{	uae_s32 dst = xget_long(dsta);
	xm68k_areg (dstreg) = dsta;
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_2a8_0(uae_u32 opcode) /* AND */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(6);
{	uae_s32 dst = xget_long(dsta);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(8);
}
void xop_2b0_0(uae_u32 opcode) /* AND */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xget_ilong(2);
{xm68k_incpc(6);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s32 dst = xget_long(dsta);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}}}
void xop_2b8_0(uae_u32 opcode) /* AND */
{
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(6);
{	uae_s32 dst = xget_long(dsta);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(8);
}
void xop_2b9_0(uae_u32 opcode) /* AND */
{
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = xget_ilong(6);
{	uae_s32 dst = xget_long(dsta);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(10);
}
void xop_2d0_0(uae_u32 opcode) /* CHK2 */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_2e8_0(uae_u32 opcode) /* CHK2 */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_2f0_0(uae_u32 opcode) /* CHK2 */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_2f8_0(uae_u32 opcode) /* CHK2 */
{
{}}
void xop_2f9_0(uae_u32 opcode) /* CHK2 */
{
{}}
void xop_2fa_0(uae_u32 opcode) /* CHK2 */
{
	uae_u32 dstreg = 2;
{}}
void xop_2fb_0(uae_u32 opcode) /* CHK2 */
{
	uae_u32 dstreg = 3;
{}}
void xop_400_0(uae_u32 opcode) /* SUB */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xget_ibyte(2);
{	uae_s8 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}xm68k_incpc(4);
}
void xop_410_0(uae_u32 opcode) /* SUB */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}xm68k_incpc(4);
}
void xop_418_0(uae_u32 opcode) /* SUB */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg(dstreg) += xareg_byteinc[dstreg];
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}xm68k_incpc(4);
}
void xop_420_0(uae_u32 opcode) /* SUB */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = xm68k_areg(dstreg) - xareg_byteinc[dstreg];
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg (dstreg) = dsta;
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}xm68k_incpc(4);
}
void xop_428_0(uae_u32 opcode) /* SUB */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(4);
{	uae_s8 dst = xget_byte(dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}xm68k_incpc(6);
}
void xop_430_0(uae_u32 opcode) /* SUB */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xget_ibyte(2);
{xm68k_incpc(4);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s8 dst = xget_byte(dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}}}
void xop_438_0(uae_u32 opcode) /* SUB */
{
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(4);
{	uae_s8 dst = xget_byte(dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}xm68k_incpc(6);
}
void xop_439_0(uae_u32 opcode) /* SUB */
{
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = xget_ilong(4);
{	uae_s8 dst = xget_byte(dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}xm68k_incpc(8);
}
void xop_440_0(uae_u32 opcode) /* SUB */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}xm68k_incpc(4);
}
void xop_450_0(uae_u32 opcode) /* SUB */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s16 dst = xget_word(dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}xm68k_incpc(4);
}
void xop_458_0(uae_u32 opcode) /* SUB */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s16 dst = xget_word(dsta);
	xm68k_areg(dstreg) += 2;
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}xm68k_incpc(4);
}
void xop_460_0(uae_u32 opcode) /* SUB */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg) - 2;
{	uae_s16 dst = xget_word(dsta);
	xm68k_areg (dstreg) = dsta;
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}xm68k_incpc(4);
}
void xop_468_0(uae_u32 opcode) /* SUB */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(4);
{	uae_s16 dst = xget_word(dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}xm68k_incpc(6);
}
void xop_470_0(uae_u32 opcode) /* SUB */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{xm68k_incpc(4);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s16 dst = xget_word(dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}}}
void xop_478_0(uae_u32 opcode) /* SUB */
{
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(4);
{	uae_s16 dst = xget_word(dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}xm68k_incpc(6);
}
void xop_479_0(uae_u32 opcode) /* SUB */
{
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xget_ilong(4);
{	uae_s16 dst = xget_word(dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}xm68k_incpc(8);
}
void xop_480_0(uae_u32 opcode) /* SUB */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xget_ilong(2);
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}}xm68k_incpc(6);
}
void xop_490_0(uae_u32 opcode) /* SUB */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s32 dst = xget_long(dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}xm68k_incpc(6);
}
void xop_498_0(uae_u32 opcode) /* SUB */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s32 dst = xget_long(dsta);
	xm68k_areg(dstreg) += 4;
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}xm68k_incpc(6);
}
void xop_4a0_0(uae_u32 opcode) /* SUB */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = xm68k_areg(dstreg) - 4;
{	uae_s32 dst = xget_long(dsta);
	xm68k_areg (dstreg) = dsta;
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}xm68k_incpc(6);
}
void xop_4a8_0(uae_u32 opcode) /* SUB */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(6);
{	uae_s32 dst = xget_long(dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}xm68k_incpc(8);
}
void xop_4b0_0(uae_u32 opcode) /* SUB */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xget_ilong(2);
{xm68k_incpc(6);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s32 dst = xget_long(dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}}}
void xop_4b8_0(uae_u32 opcode) /* SUB */
{
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(6);
{	uae_s32 dst = xget_long(dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}xm68k_incpc(8);
}
void xop_4b9_0(uae_u32 opcode) /* SUB */
{
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = xget_ilong(6);
{	uae_s32 dst = xget_long(dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}xm68k_incpc(10);
}
void xop_4d0_0(uae_u32 opcode) /* CHK2 */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_4e8_0(uae_u32 opcode) /* CHK2 */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_4f0_0(uae_u32 opcode) /* CHK2 */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_4f8_0(uae_u32 opcode) /* CHK2 */
{
{}}
void xop_4f9_0(uae_u32 opcode) /* CHK2 */
{
{}}
void xop_4fa_0(uae_u32 opcode) /* CHK2 */
{
	uae_u32 dstreg = 2;
{}}
void xop_4fb_0(uae_u32 opcode) /* CHK2 */
{
	uae_u32 dstreg = 3;
{}}
void xop_600_0(uae_u32 opcode) /* ADD */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xget_ibyte(2);
{	uae_s8 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}xm68k_incpc(4);
}
void xop_610_0(uae_u32 opcode) /* ADD */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}xm68k_incpc(4);
}
void xop_618_0(uae_u32 opcode) /* ADD */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg(dstreg) += xareg_byteinc[dstreg];
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}xm68k_incpc(4);
}
void xop_620_0(uae_u32 opcode) /* ADD */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = xm68k_areg(dstreg) - xareg_byteinc[dstreg];
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg (dstreg) = dsta;
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}xm68k_incpc(4);
}
void xop_628_0(uae_u32 opcode) /* ADD */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(4);
{	uae_s8 dst = xget_byte(dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}xm68k_incpc(6);
}
void xop_630_0(uae_u32 opcode) /* ADD */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xget_ibyte(2);
{xm68k_incpc(4);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s8 dst = xget_byte(dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}}}
void xop_638_0(uae_u32 opcode) /* ADD */
{
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(4);
{	uae_s8 dst = xget_byte(dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}xm68k_incpc(6);
}
void xop_639_0(uae_u32 opcode) /* ADD */
{
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = xget_ilong(4);
{	uae_s8 dst = xget_byte(dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}xm68k_incpc(8);
}
void xop_640_0(uae_u32 opcode) /* ADD */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}xm68k_incpc(4);
}
void xop_650_0(uae_u32 opcode) /* ADD */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s16 dst = xget_word(dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}xm68k_incpc(4);
}
void xop_658_0(uae_u32 opcode) /* ADD */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s16 dst = xget_word(dsta);
	xm68k_areg(dstreg) += 2;
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}xm68k_incpc(4);
}
void xop_660_0(uae_u32 opcode) /* ADD */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg) - 2;
{	uae_s16 dst = xget_word(dsta);
	xm68k_areg (dstreg) = dsta;
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}xm68k_incpc(4);
}
void xop_668_0(uae_u32 opcode) /* ADD */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(4);
{	uae_s16 dst = xget_word(dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}xm68k_incpc(6);
}
void xop_670_0(uae_u32 opcode) /* ADD */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{xm68k_incpc(4);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s16 dst = xget_word(dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}}}
void xop_678_0(uae_u32 opcode) /* ADD */
{
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(4);
{	uae_s16 dst = xget_word(dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}xm68k_incpc(6);
}
void xop_679_0(uae_u32 opcode) /* ADD */
{
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xget_ilong(4);
{	uae_s16 dst = xget_word(dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}xm68k_incpc(8);
}
void xop_680_0(uae_u32 opcode) /* ADD */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xget_ilong(2);
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}}xm68k_incpc(6);
}
void xop_690_0(uae_u32 opcode) /* ADD */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s32 dst = xget_long(dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}xm68k_incpc(6);
}
void xop_698_0(uae_u32 opcode) /* ADD */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s32 dst = xget_long(dsta);
	xm68k_areg(dstreg) += 4;
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}xm68k_incpc(6);
}
void xop_6a0_0(uae_u32 opcode) /* ADD */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = xm68k_areg(dstreg) - 4;
{	uae_s32 dst = xget_long(dsta);
	xm68k_areg (dstreg) = dsta;
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}xm68k_incpc(6);
}
void xop_6a8_0(uae_u32 opcode) /* ADD */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(6);
{	uae_s32 dst = xget_long(dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}xm68k_incpc(8);
}
void xop_6b0_0(uae_u32 opcode) /* ADD */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xget_ilong(2);
{xm68k_incpc(6);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s32 dst = xget_long(dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}}}
void xop_6b8_0(uae_u32 opcode) /* ADD */
{
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(6);
{	uae_s32 dst = xget_long(dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}xm68k_incpc(8);
}
void xop_6b9_0(uae_u32 opcode) /* ADD */
{
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = xget_ilong(6);
{	uae_s32 dst = xget_long(dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}xm68k_incpc(10);
}
void xop_6c0_0(uae_u32 opcode) /* RTM */
{
	uae_u32 srcreg = (opcode & 7);
{}}
void xop_6c8_0(uae_u32 opcode) /* RTM */
{
	uae_u32 srcreg = (opcode & 7);
{}}
void xop_6d0_0(uae_u32 opcode) /* CALLM */
{
	uae_u32 srcreg = (opcode & 7);
{}}
void xop_6e8_0(uae_u32 opcode) /* CALLM */
{
	uae_u32 srcreg = (opcode & 7);
{}}
void xop_6f0_0(uae_u32 opcode) /* CALLM */
{
	uae_u32 srcreg = (opcode & 7);
{}}
void xop_6f8_0(uae_u32 opcode) /* CALLM */
{
{}}
void xop_6f9_0(uae_u32 opcode) /* CALLM */
{
{}}
void xop_6fa_0(uae_u32 opcode) /* CALLM */
{
{}}
void xop_6fb_0(uae_u32 opcode) /* CALLM */
{
{}}
void xop_800_0(uae_u32 opcode) /* BTST */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uae_s32 dst = xm68k_dreg(dstreg);
	src &= 31;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
}}}xm68k_incpc(4);
}
void xop_810_0(uae_u32 opcode) /* BTST */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}xm68k_incpc(4);
}
void xop_818_0(uae_u32 opcode) /* BTST */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg(dstreg) += xareg_byteinc[dstreg];
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}xm68k_incpc(4);
}
void xop_820_0(uae_u32 opcode) /* BTST */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg) - xareg_byteinc[dstreg];
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg (dstreg) = dsta;
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}xm68k_incpc(4);
}
void xop_828_0(uae_u32 opcode) /* BTST */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(4);
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}xm68k_incpc(6);
}
void xop_830_0(uae_u32 opcode) /* BTST */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{xm68k_incpc(4);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}}}
void xop_838_0(uae_u32 opcode) /* BTST */
{
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(4);
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}xm68k_incpc(6);
}
void xop_839_0(uae_u32 opcode) /* BTST */
{
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xget_ilong(4);
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}xm68k_incpc(8);
}
void xop_83a_0(uae_u32 opcode) /* BTST */
{
	uae_u32 dstreg = 2;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_getpc () + 4;
	dsta += (uae_s32)(uae_s16)xget_iword(4);
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}xm68k_incpc(6);
}
void xop_83b_0(uae_u32 opcode) /* BTST */
{
	uae_u32 dstreg = 3;
{{	uae_s16 src = xget_iword(2);
{xm68k_incpc(4);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr dsta = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
}}}}}}
void xop_83c_0(uae_u32 opcode) /* BTST */
{
{{	uae_s16 src = xget_iword(2);
{	uae_s8 dst = xget_ibyte(4);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
}}}xm68k_incpc(6);
}
void xop_840_0(uae_u32 opcode) /* BCHG */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uae_s32 dst = xm68k_dreg(dstreg);
	src &= 31;
	dst ^= (1 << src);
	XSET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	xm68k_dreg(dstreg) = (dst);
}}}xm68k_incpc(4);
}
void xop_850_0(uae_u32 opcode) /* BCHG */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	dst ^= (1 << src);
	XSET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(4);
}
void xop_858_0(uae_u32 opcode) /* BCHG */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg(dstreg) += xareg_byteinc[dstreg];
	src &= 7;
	dst ^= (1 << src);
	XSET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(4);
}
void xop_860_0(uae_u32 opcode) /* BCHG */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg) - xareg_byteinc[dstreg];
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg (dstreg) = dsta;
	src &= 7;
	dst ^= (1 << src);
	XSET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(4);
}
void xop_868_0(uae_u32 opcode) /* BCHG */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(4);
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	dst ^= (1 << src);
	XSET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(6);
}
void xop_870_0(uae_u32 opcode) /* BCHG */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{xm68k_incpc(4);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	dst ^= (1 << src);
	XSET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	xput_byte(dsta,dst);
}}}}}}
void xop_878_0(uae_u32 opcode) /* BCHG */
{
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(4);
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	dst ^= (1 << src);
	XSET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(6);
}
void xop_879_0(uae_u32 opcode) /* BCHG */
{
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xget_ilong(4);
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	dst ^= (1 << src);
	XSET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(8);
}
void xop_87a_0(uae_u32 opcode) /* BCHG */
{
	uae_u32 dstreg = 2;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_getpc () + 4;
	dsta += (uae_s32)(uae_s16)xget_iword(4);
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	dst ^= (1 << src);
	XSET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(6);
}
void xop_87b_0(uae_u32 opcode) /* BCHG */
{
	uae_u32 dstreg = 3;
{{	uae_s16 src = xget_iword(2);
{xm68k_incpc(4);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr dsta = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	dst ^= (1 << src);
	XSET_ZFLG (((uae_u32)dst & (1 << src)) >> src);
	xput_byte(dsta,dst);
}}}}}}
void xop_880_0(uae_u32 opcode) /* BCLR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uae_s32 dst = xm68k_dreg(dstreg);
	src &= 31;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	xm68k_dreg(dstreg) = (dst);
}}}xm68k_incpc(4);
}
void xop_890_0(uae_u32 opcode) /* BCLR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(4);
}
void xop_898_0(uae_u32 opcode) /* BCLR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg(dstreg) += xareg_byteinc[dstreg];
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(4);
}
void xop_8a0_0(uae_u32 opcode) /* BCLR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg) - xareg_byteinc[dstreg];
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg (dstreg) = dsta;
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(4);
}
void xop_8a8_0(uae_u32 opcode) /* BCLR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(4);
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(6);
}
void xop_8b0_0(uae_u32 opcode) /* BCLR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{xm68k_incpc(4);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	xput_byte(dsta,dst);
}}}}}}
void xop_8b8_0(uae_u32 opcode) /* BCLR */
{
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(4);
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(6);
}
void xop_8b9_0(uae_u32 opcode) /* BCLR */
{
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xget_ilong(4);
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(8);
}
void xop_8ba_0(uae_u32 opcode) /* BCLR */
{
	uae_u32 dstreg = 2;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_getpc () + 4;
	dsta += (uae_s32)(uae_s16)xget_iword(4);
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(6);
}
void xop_8bb_0(uae_u32 opcode) /* BCLR */
{
	uae_u32 dstreg = 3;
{{	uae_s16 src = xget_iword(2);
{xm68k_incpc(4);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr dsta = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
	dst &= ~(1 << src);
	xput_byte(dsta,dst);
}}}}}}
void xop_8c0_0(uae_u32 opcode) /* BSET */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uae_s32 dst = xm68k_dreg(dstreg);
	src &= 31;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	xm68k_dreg(dstreg) = (dst);
}}}xm68k_incpc(4);
}
void xop_8d0_0(uae_u32 opcode) /* BSET */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(4);
}
void xop_8d8_0(uae_u32 opcode) /* BSET */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg(dstreg) += xareg_byteinc[dstreg];
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(4);
}
void xop_8e0_0(uae_u32 opcode) /* BSET */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg) - xareg_byteinc[dstreg];
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg (dstreg) = dsta;
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(4);
}
void xop_8e8_0(uae_u32 opcode) /* BSET */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(4);
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(6);
}
void xop_8f0_0(uae_u32 opcode) /* BSET */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{xm68k_incpc(4);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	xput_byte(dsta,dst);
}}}}}}
void xop_8f8_0(uae_u32 opcode) /* BSET */
{
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(4);
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(6);
}
void xop_8f9_0(uae_u32 opcode) /* BSET */
{
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xget_ilong(4);
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(8);
}
void xop_8fa_0(uae_u32 opcode) /* BSET */
{
	uae_u32 dstreg = 2;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_getpc () + 4;
	dsta += (uae_s32)(uae_s16)xget_iword(4);
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	xput_byte(dsta,dst);
}}}}xm68k_incpc(6);
}
void xop_8fb_0(uae_u32 opcode) /* BSET */
{
	uae_u32 dstreg = 3;
{{	uae_s16 src = xget_iword(2);
{xm68k_incpc(4);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr dsta = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s8 dst = xget_byte(dsta);
	src &= 7;
	XSET_ZFLG (1 ^ ((dst >> src) & 1));
	dst |= (1 << src);
	xput_byte(dsta,dst);
}}}}}}
void xop_a00_0(uae_u32 opcode) /* EOR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xget_ibyte(2);
{	uae_s8 dst = xm68k_dreg(dstreg);
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((src) & 0xff);
}}}xm68k_incpc(4);
}
void xop_a10_0(uae_u32 opcode) /* EOR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_a18_0(uae_u32 opcode) /* EOR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg(dstreg) += xareg_byteinc[dstreg];
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_a20_0(uae_u32 opcode) /* EOR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = xm68k_areg(dstreg) - xareg_byteinc[dstreg];
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg (dstreg) = dsta;
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_a28_0(uae_u32 opcode) /* EOR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(4);
{	uae_s8 dst = xget_byte(dsta);
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_a30_0(uae_u32 opcode) /* EOR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xget_ibyte(2);
{xm68k_incpc(4);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s8 dst = xget_byte(dsta);
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}}}
void xop_a38_0(uae_u32 opcode) /* EOR */
{
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(4);
{	uae_s8 dst = xget_byte(dsta);
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_a39_0(uae_u32 opcode) /* EOR */
{
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = xget_ilong(4);
{	uae_s8 dst = xget_byte(dsta);
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(8);
}
void xop_a3c_0(uae_u32 opcode) /* EORSR */
{
{}}
void xop_a40_0(uae_u32 opcode) /* EOR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uae_s16 dst = xm68k_dreg(dstreg);
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((src) & 0xffff);
}}}xm68k_incpc(4);
}
void xop_a50_0(uae_u32 opcode) /* EOR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s16 dst = xget_word(dsta);
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_a58_0(uae_u32 opcode) /* EOR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s16 dst = xget_word(dsta);
	xm68k_areg(dstreg) += 2;
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_a60_0(uae_u32 opcode) /* EOR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg) - 2;
{	uae_s16 dst = xget_word(dsta);
	xm68k_areg (dstreg) = dsta;
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_a68_0(uae_u32 opcode) /* EOR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(4);
{	uae_s16 dst = xget_word(dsta);
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_a70_0(uae_u32 opcode) /* EOR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{xm68k_incpc(4);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s16 dst = xget_word(dsta);
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}}}
void xop_a78_0(uae_u32 opcode) /* EOR */
{
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(4);
{	uae_s16 dst = xget_word(dsta);
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_a79_0(uae_u32 opcode) /* EOR */
{
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xget_ilong(4);
{	uae_s16 dst = xget_word(dsta);
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(8);
}
void xop_a7c_0(uae_u32 opcode) /* EORSR */
{
}
void xop_a80_0(uae_u32 opcode) /* EOR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xget_ilong(2);
{	uae_s32 dst = xm68k_dreg(dstreg);
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xm68k_dreg(dstreg) = (src);
}}}xm68k_incpc(6);
}
void xop_a90_0(uae_u32 opcode) /* EOR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s32 dst = xget_long(dsta);
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_a98_0(uae_u32 opcode) /* EOR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s32 dst = xget_long(dsta);
	xm68k_areg(dstreg) += 4;
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_aa0_0(uae_u32 opcode) /* EOR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = xm68k_areg(dstreg) - 4;
{	uae_s32 dst = xget_long(dsta);
	xm68k_areg (dstreg) = dsta;
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_aa8_0(uae_u32 opcode) /* EOR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(6);
{	uae_s32 dst = xget_long(dsta);
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(8);
}
void xop_ab0_0(uae_u32 opcode) /* EOR */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xget_ilong(2);
{xm68k_incpc(6);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s32 dst = xget_long(dsta);
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}}}
void xop_ab8_0(uae_u32 opcode) /* EOR */
{
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(6);
{	uae_s32 dst = xget_long(dsta);
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(8);
}
void xop_ab9_0(uae_u32 opcode) /* EOR */
{
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = xget_ilong(6);
{	uae_s32 dst = xget_long(dsta);
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(10);
}
void xop_ad0_0(uae_u32 opcode) /* CAS */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_ad8_0(uae_u32 opcode) /* CAS */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_ae0_0(uae_u32 opcode) /* CAS */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_ae8_0(uae_u32 opcode) /* CAS */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_af0_0(uae_u32 opcode) /* CAS */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_af8_0(uae_u32 opcode) /* CAS */
{
{}}
void xop_af9_0(uae_u32 opcode) /* CAS */
{
{}}
void xop_c00_0(uae_u32 opcode) /* CMP */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xget_ibyte(2);
{	uae_s8 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}xm68k_incpc(4);
}
void xop_c10_0(uae_u32 opcode) /* CMP */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(4);
}
void xop_c18_0(uae_u32 opcode) /* CMP */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg(dstreg) += xareg_byteinc[dstreg];
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(4);
}
void xop_c20_0(uae_u32 opcode) /* CMP */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = xm68k_areg(dstreg) - xareg_byteinc[dstreg];
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg (dstreg) = dsta;
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(4);
}
void xop_c28_0(uae_u32 opcode) /* CMP */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(4);
{	uae_s8 dst = xget_byte(dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(6);
}
void xop_c30_0(uae_u32 opcode) /* CMP */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xget_ibyte(2);
{xm68k_incpc(4);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s8 dst = xget_byte(dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}}}
void xop_c38_0(uae_u32 opcode) /* CMP */
{
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(4);
{	uae_s8 dst = xget_byte(dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(6);
}
void xop_c39_0(uae_u32 opcode) /* CMP */
{
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = xget_ilong(4);
{	uae_s8 dst = xget_byte(dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(8);
}
void xop_c3a_0(uae_u32 opcode) /* CMP */
{
	uae_u32 dstreg = 2;
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = xm68k_getpc () + 4;
	dsta += (uae_s32)(uae_s16)xget_iword(4);
{	uae_s8 dst = xget_byte(dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(6);
}
void xop_c3b_0(uae_u32 opcode) /* CMP */
{
	uae_u32 dstreg = 3;
{{	uae_s8 src = xget_ibyte(2);
{xm68k_incpc(4);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr dsta = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s8 dst = xget_byte(dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}}}
void xop_c40_0(uae_u32 opcode) /* CMP */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}xm68k_incpc(4);
}
void xop_c50_0(uae_u32 opcode) /* CMP */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s16 dst = xget_word(dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(4);
}
void xop_c58_0(uae_u32 opcode) /* CMP */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s16 dst = xget_word(dsta);
	xm68k_areg(dstreg) += 2;
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(4);
}
void xop_c60_0(uae_u32 opcode) /* CMP */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg) - 2;
{	uae_s16 dst = xget_word(dsta);
	xm68k_areg (dstreg) = dsta;
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(4);
}
void xop_c68_0(uae_u32 opcode) /* CMP */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(4);
{	uae_s16 dst = xget_word(dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(6);
}
void xop_c70_0(uae_u32 opcode) /* CMP */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xget_iword(2);
{xm68k_incpc(4);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s16 dst = xget_word(dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}}}
void xop_c78_0(uae_u32 opcode) /* CMP */
{
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(4);
{	uae_s16 dst = xget_word(dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(6);
}
void xop_c79_0(uae_u32 opcode) /* CMP */
{
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xget_ilong(4);
{	uae_s16 dst = xget_word(dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(8);
}
void xop_c7a_0(uae_u32 opcode) /* CMP */
{
	uae_u32 dstreg = 2;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_getpc () + 4;
	dsta += (uae_s32)(uae_s16)xget_iword(4);
{	uae_s16 dst = xget_word(dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(6);
}
void xop_c7b_0(uae_u32 opcode) /* CMP */
{
	uae_u32 dstreg = 3;
{{	uae_s16 src = xget_iword(2);
{xm68k_incpc(4);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr dsta = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s16 dst = xget_word(dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}}}
void xop_c80_0(uae_u32 opcode) /* CMP */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xget_ilong(2);
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}xm68k_incpc(6);
}
void xop_c90_0(uae_u32 opcode) /* CMP */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s32 dst = xget_long(dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(6);
}
void xop_c98_0(uae_u32 opcode) /* CMP */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s32 dst = xget_long(dsta);
	xm68k_areg(dstreg) += 4;
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(6);
}
void xop_ca0_0(uae_u32 opcode) /* CMP */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = xm68k_areg(dstreg) - 4;
{	uae_s32 dst = xget_long(dsta);
	xm68k_areg (dstreg) = dsta;
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(6);
}
void xop_ca8_0(uae_u32 opcode) /* CMP */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(6);
{	uae_s32 dst = xget_long(dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(8);
}
void xop_cb0_0(uae_u32 opcode) /* CMP */
{
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xget_ilong(2);
{xm68k_incpc(6);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s32 dst = xget_long(dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}}}
void xop_cb8_0(uae_u32 opcode) /* CMP */
{
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(6);
{	uae_s32 dst = xget_long(dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(8);
}
void xop_cb9_0(uae_u32 opcode) /* CMP */
{
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = xget_ilong(6);
{	uae_s32 dst = xget_long(dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(10);
}
void xop_cba_0(uae_u32 opcode) /* CMP */
{
	uae_u32 dstreg = 2;
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = xm68k_getpc () + 6;
	dsta += (uae_s32)(uae_s16)xget_iword(6);
{	uae_s32 dst = xget_long(dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(8);
}
void xop_cbb_0(uae_u32 opcode) /* CMP */
{
	uae_u32 dstreg = 3;
{{	uae_s32 src = xget_ilong(2);
{xm68k_incpc(6);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr dsta = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s32 dst = xget_long(dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}}}
void xop_cd0_0(uae_u32 opcode) /* CAS */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_cd8_0(uae_u32 opcode) /* CAS */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_ce0_0(uae_u32 opcode) /* CAS */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_ce8_0(uae_u32 opcode) /* CAS */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_cf0_0(uae_u32 opcode) /* CAS */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_cf8_0(uae_u32 opcode) /* CAS */
{
{}}
void xop_cf9_0(uae_u32 opcode) /* CAS */
{
{}}
void xop_cfc_0(uae_u32 opcode) /* CAS2 */
{
{}}
void xop_e10_0(uae_u32 opcode) /* MOVES */
{
	uae_u32 dstreg = opcode & 7;
}
void xop_e18_0(uae_u32 opcode) /* MOVES */
{
	uae_u32 dstreg = opcode & 7;
}
void xop_e20_0(uae_u32 opcode) /* MOVES */
{
	uae_u32 dstreg = opcode & 7;
}
void xop_e28_0(uae_u32 opcode) /* MOVES */
{
	uae_u32 dstreg = opcode & 7;
}
void xop_e30_0(uae_u32 opcode) /* MOVES */
{
	uae_u32 dstreg = opcode & 7;
}
void xop_e38_0(uae_u32 opcode) /* MOVES */
{
}
void xop_e39_0(uae_u32 opcode) /* MOVES */
{
}
void xop_e50_0(uae_u32 opcode) /* MOVES */
{
	uae_u32 dstreg = opcode & 7;
}
void xop_e58_0(uae_u32 opcode) /* MOVES */
{
	uae_u32 dstreg = opcode & 7;
}
void xop_e60_0(uae_u32 opcode) /* MOVES */
{
	uae_u32 dstreg = opcode & 7;
}
void xop_e68_0(uae_u32 opcode) /* MOVES */
{
	uae_u32 dstreg = opcode & 7;
}
void xop_e70_0(uae_u32 opcode) /* MOVES */
{
	uae_u32 dstreg = opcode & 7;
}
void xop_e78_0(uae_u32 opcode) /* MOVES */
{
}
void xop_e79_0(uae_u32 opcode) /* MOVES */
{
}
void xop_e90_0(uae_u32 opcode) /* MOVES */
{
	uae_u32 dstreg = opcode & 7;
}
void xop_e98_0(uae_u32 opcode) /* MOVES */
{
	uae_u32 dstreg = opcode & 7;
}
void xop_ea0_0(uae_u32 opcode) /* MOVES */
{
	uae_u32 dstreg = opcode & 7;
}
void xop_ea8_0(uae_u32 opcode) /* MOVES */
{
	uae_u32 dstreg = opcode & 7;
}
void xop_eb0_0(uae_u32 opcode) /* MOVES */
{
	uae_u32 dstreg = opcode & 7;
}
void xop_eb8_0(uae_u32 opcode) /* MOVES */
{
}
void xop_eb9_0(uae_u32 opcode) /* MOVES */
{
}
void xop_ed0_0(uae_u32 opcode) /* CAS */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_ed8_0(uae_u32 opcode) /* CAS */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_ee0_0(uae_u32 opcode) /* CAS */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_ee8_0(uae_u32 opcode) /* CAS */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_ef0_0(uae_u32 opcode) /* CAS */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_ef8_0(uae_u32 opcode) /* CAS */
{
{}}
void xop_ef9_0(uae_u32 opcode) /* CAS */
{
{}}
void xop_efc_0(uae_u32 opcode) /* CAS2 */
{
{}}
void xop_1000_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((src) & 0xff);
}}}xm68k_incpc(2);
}
void xop_1010_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s8 src = xget_byte(srca);
{	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((src) & 0xff);
}}}}xm68k_incpc(2);
}
void xop_1018_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s8 src = xget_byte(srca);
	xm68k_areg(srcreg) += xareg_byteinc[srcreg];
{	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((src) & 0xff);
}}}}xm68k_incpc(2);
}
void xop_1020_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - xareg_byteinc[srcreg];
{	uae_s8 src = xget_byte(srca);
	xm68k_areg (srcreg) = srca;
{	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((src) & 0xff);
}}}}xm68k_incpc(2);
}
void xop_1028_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((src) & 0xff);
}}}}xm68k_incpc(4);
}
void xop_1030_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s8 src = xget_byte(srca);
{	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((src) & 0xff);
}}}}}}
void xop_1038_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((src) & 0xff);
}}}}xm68k_incpc(4);
}
void xop_1039_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s8 src = xget_byte(srca);
{	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((src) & 0xff);
}}}}xm68k_incpc(6);
}
void xop_103a_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((src) & 0xff);
}}}}xm68k_incpc(4);
}
void xop_103b_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s8 src = xget_byte(srca);
{	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((src) & 0xff);
}}}}}}
void xop_103c_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = xget_ibyte(2);
{	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((src) & 0xff);
}}}xm68k_incpc(4);
}
void xop_1080_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}xm68k_incpc(2);
}
void xop_1090_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_1098_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s8 src = xget_byte(srca);
	xm68k_areg(srcreg) += xareg_byteinc[srcreg];
{	uaecptr dsta = xm68k_areg(dstreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_10a0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - xareg_byteinc[srcreg];
{	uae_s8 src = xget_byte(srca);
	xm68k_areg (srcreg) = srca;
{	uaecptr dsta = xm68k_areg(dstreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_10a8_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_10b0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}}}
void xop_10b8_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_10b9_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_10ba_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_10bb_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}}}
void xop_10bc_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = xm68k_areg(dstreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}xm68k_incpc(4);
}
void xop_10c0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
	xm68k_areg(dstreg) += xareg_byteinc[dstreg];
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}xm68k_incpc(2);
}
void xop_10d0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	xm68k_areg(dstreg) += xareg_byteinc[dstreg];
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_10d8_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s8 src = xget_byte(srca);
	xm68k_areg(srcreg) += xareg_byteinc[srcreg];
{	uaecptr dsta = xm68k_areg(dstreg);
	xm68k_areg(dstreg) += xareg_byteinc[dstreg];
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_10e0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - xareg_byteinc[srcreg];
{	uae_s8 src = xget_byte(srca);
	xm68k_areg (srcreg) = srca;
{	uaecptr dsta = xm68k_areg(dstreg);
	xm68k_areg(dstreg) += xareg_byteinc[dstreg];
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_10e8_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	xm68k_areg(dstreg) += xareg_byteinc[dstreg];
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_10f0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	xm68k_areg(dstreg) += xareg_byteinc[dstreg];
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}}}
void xop_10f8_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	xm68k_areg(dstreg) += xareg_byteinc[dstreg];
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_10f9_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	xm68k_areg(dstreg) += xareg_byteinc[dstreg];
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_10fa_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	xm68k_areg(dstreg) += xareg_byteinc[dstreg];
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_10fb_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	xm68k_areg(dstreg) += xareg_byteinc[dstreg];
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}}}
void xop_10fc_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = xm68k_areg(dstreg);
	xm68k_areg(dstreg) += xareg_byteinc[dstreg];
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}xm68k_incpc(4);
}
void xop_1100_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) - xareg_byteinc[dstreg];
	xm68k_areg (dstreg) = dsta;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}xm68k_incpc(2);
}
void xop_1110_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = xm68k_areg(dstreg) - xareg_byteinc[dstreg];
	xm68k_areg (dstreg) = dsta;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_1118_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s8 src = xget_byte(srca);
	xm68k_areg(srcreg) += xareg_byteinc[srcreg];
{	uaecptr dsta = xm68k_areg(dstreg) - xareg_byteinc[dstreg];
	xm68k_areg (dstreg) = dsta;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_1120_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - xareg_byteinc[srcreg];
{	uae_s8 src = xget_byte(srca);
	xm68k_areg (srcreg) = srca;
{	uaecptr dsta = xm68k_areg(dstreg) - xareg_byteinc[dstreg];
	xm68k_areg (dstreg) = dsta;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_1128_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = xm68k_areg(dstreg) - xareg_byteinc[dstreg];
	xm68k_areg (dstreg) = dsta;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_1130_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = xm68k_areg(dstreg) - xareg_byteinc[dstreg];
	xm68k_areg (dstreg) = dsta;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}}}
void xop_1138_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = xm68k_areg(dstreg) - xareg_byteinc[dstreg];
	xm68k_areg (dstreg) = dsta;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_1139_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = xm68k_areg(dstreg) - xareg_byteinc[dstreg];
	xm68k_areg (dstreg) = dsta;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_113a_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = xm68k_areg(dstreg) - xareg_byteinc[dstreg];
	xm68k_areg (dstreg) = dsta;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_113b_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = xm68k_areg(dstreg) - xareg_byteinc[dstreg];
	xm68k_areg (dstreg) = dsta;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}}}
void xop_113c_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = xm68k_areg(dstreg) - xareg_byteinc[dstreg];
	xm68k_areg (dstreg) = dsta;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}xm68k_incpc(4);
}
void xop_1140_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}xm68k_incpc(4);
}
void xop_1150_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_1158_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s8 src = xget_byte(srca);
	xm68k_areg(srcreg) += xareg_byteinc[srcreg];
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_1160_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - xareg_byteinc[srcreg];
{	uae_s8 src = xget_byte(srca);
	xm68k_areg (srcreg) = srca;
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_1168_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(4);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_1170_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(0);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}}xm68k_incpc(2);
}
void xop_1178_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(4);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_1179_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(6);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(8);
}
void xop_117a_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(4);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_117b_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(0);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}}xm68k_incpc(2);
}
void xop_117c_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(4);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}xm68k_incpc(6);
}
void xop_1180_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{xm68k_incpc(2);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}}
void xop_1190_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s8 src = xget_byte(srca);
{xm68k_incpc(2);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}}}
void xop_1198_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s8 src = xget_byte(srca);
	xm68k_areg(srcreg) += xareg_byteinc[srcreg];
{xm68k_incpc(2);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}}}
void xop_11a0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - xareg_byteinc[srcreg];
{	uae_s8 src = xget_byte(srca);
	xm68k_areg (srcreg) = srca;
{xm68k_incpc(2);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}}}
void xop_11a8_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{xm68k_incpc(4);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}}}
void xop_11b0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s8 src = xget_byte(srca);
{{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}}}}
void xop_11b8_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{xm68k_incpc(4);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}}}
void xop_11b9_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s8 src = xget_byte(srca);
{xm68k_incpc(6);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}}}
void xop_11ba_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{xm68k_incpc(4);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}}}
void xop_11bb_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s8 src = xget_byte(srca);
{{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}}}}
void xop_11bc_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = xget_ibyte(2);
{xm68k_incpc(4);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}}
void xop_11c0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}xm68k_incpc(4);
}
void xop_11d0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_11d8_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s8 src = xget_byte(srca);
	xm68k_areg(srcreg) += xareg_byteinc[srcreg];
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_11e0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) - xareg_byteinc[srcreg];
{	uae_s8 src = xget_byte(srca);
	xm68k_areg (srcreg) = srca;
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_11e8_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(4);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_11f0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(0);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}}xm68k_incpc(2);
}
void xop_11f8_0(uae_u32 opcode) /* MOVE */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(4);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_11f9_0(uae_u32 opcode) /* MOVE */
{
{{	uaecptr srca = xget_ilong(2);
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(6);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(8);
}
void xop_11fa_0(uae_u32 opcode) /* MOVE */
{
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(4);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_11fb_0(uae_u32 opcode) /* MOVE */
{
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(0);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}}xm68k_incpc(2);
}
void xop_11fc_0(uae_u32 opcode) /* MOVE */
{
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(4);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}xm68k_incpc(6);
}
void xop_13c0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xget_ilong(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}xm68k_incpc(6);
}
void xop_13d0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = xget_ilong(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_13d8_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s8 src = xget_byte(srca);
	xm68k_areg(srcreg) += xareg_byteinc[srcreg];
{	uaecptr dsta = xget_ilong(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_13e0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) - xareg_byteinc[srcreg];
{	uae_s8 src = xget_byte(srca);
	xm68k_areg (srcreg) = srca;
{	uaecptr dsta = xget_ilong(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_13e8_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = xget_ilong(4);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(8);
}
void xop_13f0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = xget_ilong(0);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}}xm68k_incpc(4);
}
void xop_13f8_0(uae_u32 opcode) /* MOVE */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = xget_ilong(4);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(8);
}
void xop_13f9_0(uae_u32 opcode) /* MOVE */
{
{{	uaecptr srca = xget_ilong(2);
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = xget_ilong(6);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(10);
}
void xop_13fa_0(uae_u32 opcode) /* MOVE */
{
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = xget_ilong(4);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(8);
}
void xop_13fb_0(uae_u32 opcode) /* MOVE */
{
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s8 src = xget_byte(srca);
{	uaecptr dsta = xget_ilong(0);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}}xm68k_incpc(4);
}
void xop_13fc_0(uae_u32 opcode) /* MOVE */
{
{{	uae_s8 src = xget_ibyte(2);
{	uaecptr dsta = xget_ilong(4);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}xm68k_incpc(8);
}
void xop_2000_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xm68k_dreg(dstreg) = (src);
}}}xm68k_incpc(2);
}
void xop_2008_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xm68k_areg(srcreg);
{	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xm68k_dreg(dstreg) = (src);
}}}xm68k_incpc(2);
}
void xop_2010_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
{	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xm68k_dreg(dstreg) = (src);
}}}}xm68k_incpc(2);
}
void xop_2018_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
	xm68k_areg(srcreg) += 4;
{	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xm68k_dreg(dstreg) = (src);
}}}}xm68k_incpc(2);
}
void xop_2020_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - 4;
{	uae_s32 src = xget_long(srca);
	xm68k_areg (srcreg) = srca;
{	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xm68k_dreg(dstreg) = (src);
}}}}xm68k_incpc(2);
}
void xop_2028_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xm68k_dreg(dstreg) = (src);
}}}}xm68k_incpc(4);
}
void xop_2030_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s32 src = xget_long(srca);
{	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xm68k_dreg(dstreg) = (src);
}}}}}}
void xop_2038_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xm68k_dreg(dstreg) = (src);
}}}}xm68k_incpc(4);
}
void xop_2039_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s32 src = xget_long(srca);
{	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xm68k_dreg(dstreg) = (src);
}}}}xm68k_incpc(6);
}
void xop_203a_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xm68k_dreg(dstreg) = (src);
}}}}xm68k_incpc(4);
}
void xop_203b_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s32 src = xget_long(srca);
{	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xm68k_dreg(dstreg) = (src);
}}}}}}
void xop_203c_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xget_ilong(2);
{	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xm68k_dreg(dstreg) = (src);
}}}xm68k_incpc(6);
}
void xop_2040_0(uae_u32 opcode) /* MOVEA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uae_u32 val = src;
	xm68k_areg(dstreg) = (val);
}}}xm68k_incpc(2);
}
void xop_2048_0(uae_u32 opcode) /* MOVEA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xm68k_areg(srcreg);
{	uae_u32 val = src;
	xm68k_areg(dstreg) = (val);
}}}xm68k_incpc(2);
}
void xop_2050_0(uae_u32 opcode) /* MOVEA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
{	uae_u32 val = src;
	xm68k_areg(dstreg) = (val);
}}}}xm68k_incpc(2);
}
void xop_2058_0(uae_u32 opcode) /* MOVEA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
	xm68k_areg(srcreg) += 4;
{	uae_u32 val = src;
	xm68k_areg(dstreg) = (val);
}}}}xm68k_incpc(2);
}
void xop_2060_0(uae_u32 opcode) /* MOVEA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - 4;
{	uae_s32 src = xget_long(srca);
	xm68k_areg (srcreg) = srca;
{	uae_u32 val = src;
	xm68k_areg(dstreg) = (val);
}}}}xm68k_incpc(2);
}
void xop_2068_0(uae_u32 opcode) /* MOVEA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uae_u32 val = src;
	xm68k_areg(dstreg) = (val);
}}}}xm68k_incpc(4);
}
void xop_2070_0(uae_u32 opcode) /* MOVEA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s32 src = xget_long(srca);
{	uae_u32 val = src;
	xm68k_areg(dstreg) = (val);
}}}}}}
void xop_2078_0(uae_u32 opcode) /* MOVEA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uae_u32 val = src;
	xm68k_areg(dstreg) = (val);
}}}}xm68k_incpc(4);
}
void xop_2079_0(uae_u32 opcode) /* MOVEA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s32 src = xget_long(srca);
{	uae_u32 val = src;
	xm68k_areg(dstreg) = (val);
}}}}xm68k_incpc(6);
}
void xop_207a_0(uae_u32 opcode) /* MOVEA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uae_u32 val = src;
	xm68k_areg(dstreg) = (val);
}}}}xm68k_incpc(4);
}
void xop_207b_0(uae_u32 opcode) /* MOVEA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s32 src = xget_long(srca);
{	uae_u32 val = src;
	xm68k_areg(dstreg) = (val);
}}}}}}
void xop_207c_0(uae_u32 opcode) /* MOVEA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xget_ilong(2);
{	uae_u32 val = src;
	xm68k_areg(dstreg) = (val);
}}}xm68k_incpc(6);
}
void xop_2080_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}xm68k_incpc(2);
}
void xop_2088_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xm68k_areg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}xm68k_incpc(2);
}
void xop_2090_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_2098_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
	xm68k_areg(srcreg) += 4;
{	uaecptr dsta = xm68k_areg(dstreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_20a0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - 4;
{	uae_s32 src = xget_long(srca);
	xm68k_areg (srcreg) = srca;
{	uaecptr dsta = xm68k_areg(dstreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_20a8_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_20b0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}}}
void xop_20b8_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_20b9_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_20ba_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_20bb_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}}}
void xop_20bc_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = xm68k_areg(dstreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}xm68k_incpc(6);
}
void xop_20c0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
	xm68k_areg(dstreg) += 4;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}xm68k_incpc(2);
}
void xop_20c8_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xm68k_areg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
	xm68k_areg(dstreg) += 4;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}xm68k_incpc(2);
}
void xop_20d0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	xm68k_areg(dstreg) += 4;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_20d8_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
	xm68k_areg(srcreg) += 4;
{	uaecptr dsta = xm68k_areg(dstreg);
	xm68k_areg(dstreg) += 4;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_20e0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - 4;
{	uae_s32 src = xget_long(srca);
	xm68k_areg (srcreg) = srca;
{	uaecptr dsta = xm68k_areg(dstreg);
	xm68k_areg(dstreg) += 4;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_20e8_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	xm68k_areg(dstreg) += 4;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_20f0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	xm68k_areg(dstreg) += 4;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}}}
void xop_20f8_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	xm68k_areg(dstreg) += 4;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_20f9_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	xm68k_areg(dstreg) += 4;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_20fa_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	xm68k_areg(dstreg) += 4;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_20fb_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	xm68k_areg(dstreg) += 4;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}}}
void xop_20fc_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = xm68k_areg(dstreg);
	xm68k_areg(dstreg) += 4;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}xm68k_incpc(6);
}
void xop_2100_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) - 4;
	xm68k_areg (dstreg) = dsta;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}xm68k_incpc(2);
}
void xop_2108_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xm68k_areg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) - 4;
	xm68k_areg (dstreg) = dsta;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}xm68k_incpc(2);
}
void xop_2110_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = xm68k_areg(dstreg) - 4;
	xm68k_areg (dstreg) = dsta;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_2118_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
	xm68k_areg(srcreg) += 4;
{	uaecptr dsta = xm68k_areg(dstreg) - 4;
	xm68k_areg (dstreg) = dsta;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_2120_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - 4;
{	uae_s32 src = xget_long(srca);
	xm68k_areg (srcreg) = srca;
{	uaecptr dsta = xm68k_areg(dstreg) - 4;
	xm68k_areg (dstreg) = dsta;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_2128_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = xm68k_areg(dstreg) - 4;
	xm68k_areg (dstreg) = dsta;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_2130_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = xm68k_areg(dstreg) - 4;
	xm68k_areg (dstreg) = dsta;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}}}
void xop_2138_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = xm68k_areg(dstreg) - 4;
	xm68k_areg (dstreg) = dsta;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_2139_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = xm68k_areg(dstreg) - 4;
	xm68k_areg (dstreg) = dsta;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_213a_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = xm68k_areg(dstreg) - 4;
	xm68k_areg (dstreg) = dsta;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_213b_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = xm68k_areg(dstreg) - 4;
	xm68k_areg (dstreg) = dsta;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}}}
void xop_213c_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = xm68k_areg(dstreg) - 4;
	xm68k_areg (dstreg) = dsta;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}xm68k_incpc(6);
}
void xop_2140_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}xm68k_incpc(4);
}
void xop_2148_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xm68k_areg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}xm68k_incpc(4);
}
void xop_2150_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_2158_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
	xm68k_areg(srcreg) += 4;
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_2160_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - 4;
{	uae_s32 src = xget_long(srca);
	xm68k_areg (srcreg) = srca;
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_2168_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(4);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_2170_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(0);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}}xm68k_incpc(2);
}
void xop_2178_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(4);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_2179_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(6);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(8);
}
void xop_217a_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(4);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_217b_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(0);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}}xm68k_incpc(2);
}
void xop_217c_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(6);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}xm68k_incpc(8);
}
void xop_2180_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{xm68k_incpc(2);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}}
void xop_2188_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xm68k_areg(srcreg);
{xm68k_incpc(2);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}}
void xop_2190_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
{xm68k_incpc(2);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}}}
void xop_2198_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
	xm68k_areg(srcreg) += 4;
{xm68k_incpc(2);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}}}
void xop_21a0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - 4;
{	uae_s32 src = xget_long(srca);
	xm68k_areg (srcreg) = srca;
{xm68k_incpc(2);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}}}
void xop_21a8_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{xm68k_incpc(4);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}}}
void xop_21b0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s32 src = xget_long(srca);
{{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}}}}
void xop_21b8_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{xm68k_incpc(4);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}}}
void xop_21b9_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s32 src = xget_long(srca);
{xm68k_incpc(6);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}}}
void xop_21ba_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{xm68k_incpc(4);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}}}
void xop_21bb_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s32 src = xget_long(srca);
{{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}}}}
void xop_21bc_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xget_ilong(2);
{xm68k_incpc(6);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}}
void xop_21c0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}xm68k_incpc(4);
}
void xop_21c8_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = xm68k_areg(srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}xm68k_incpc(4);
}
void xop_21d0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_21d8_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
	xm68k_areg(srcreg) += 4;
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_21e0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) - 4;
{	uae_s32 src = xget_long(srca);
	xm68k_areg (srcreg) = srca;
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_21e8_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(4);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_21f0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(0);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}}xm68k_incpc(2);
}
void xop_21f8_0(uae_u32 opcode) /* MOVE */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(4);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_21f9_0(uae_u32 opcode) /* MOVE */
{
{{	uaecptr srca = xget_ilong(2);
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(6);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(8);
}
void xop_21fa_0(uae_u32 opcode) /* MOVE */
{
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(4);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_21fb_0(uae_u32 opcode) /* MOVE */
{
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(0);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}}xm68k_incpc(2);
}
void xop_21fc_0(uae_u32 opcode) /* MOVE */
{
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(6);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}xm68k_incpc(8);
}
void xop_23c0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xget_ilong(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}xm68k_incpc(6);
}
void xop_23c8_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = xm68k_areg(srcreg);
{	uaecptr dsta = xget_ilong(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}xm68k_incpc(6);
}
void xop_23d0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = xget_ilong(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_23d8_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
	xm68k_areg(srcreg) += 4;
{	uaecptr dsta = xget_ilong(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_23e0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) - 4;
{	uae_s32 src = xget_long(srca);
	xm68k_areg (srcreg) = srca;
{	uaecptr dsta = xget_ilong(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_23e8_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = xget_ilong(4);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(8);
}
void xop_23f0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = xget_ilong(0);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}}xm68k_incpc(4);
}
void xop_23f8_0(uae_u32 opcode) /* MOVE */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = xget_ilong(4);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(8);
}
void xop_23f9_0(uae_u32 opcode) /* MOVE */
{
{{	uaecptr srca = xget_ilong(2);
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = xget_ilong(6);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(10);
}
void xop_23fa_0(uae_u32 opcode) /* MOVE */
{
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = xget_ilong(4);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(8);
}
void xop_23fb_0(uae_u32 opcode) /* MOVE */
{
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s32 src = xget_long(srca);
{	uaecptr dsta = xget_ilong(0);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}}xm68k_incpc(4);
}
void xop_23fc_0(uae_u32 opcode) /* MOVE */
{
{{	uae_s32 src = xget_ilong(2);
{	uaecptr dsta = xget_ilong(6);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}xm68k_incpc(10);
}
void xop_3000_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((src) & 0xffff);
}}}xm68k_incpc(2);
}
void xop_3008_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xm68k_areg(srcreg);
{	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((src) & 0xffff);
}}}xm68k_incpc(2);
}
void xop_3010_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
{	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}xm68k_incpc(2);
}
void xop_3018_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
	xm68k_areg(srcreg) += 2;
{	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}xm68k_incpc(2);
}
void xop_3020_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - 2;
{	uae_s16 src = xget_word(srca);
	xm68k_areg (srcreg) = srca;
{	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}xm68k_incpc(2);
}
void xop_3028_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}xm68k_incpc(4);
}
void xop_3030_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s16 src = xget_word(srca);
{	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}}}
void xop_3038_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}xm68k_incpc(4);
}
void xop_3039_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s16 src = xget_word(srca);
{	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}xm68k_incpc(6);
}
void xop_303a_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}xm68k_incpc(4);
}
void xop_303b_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s16 src = xget_word(srca);
{	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}}}
void xop_303c_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xget_iword(2);
{	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((src) & 0xffff);
}}}xm68k_incpc(4);
}
void xop_3040_0(uae_u32 opcode) /* MOVEA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uae_u32 val = (uae_s32)(uae_s16)src;
	xm68k_areg(dstreg) = (val);
}}}xm68k_incpc(2);
}
void xop_3048_0(uae_u32 opcode) /* MOVEA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xm68k_areg(srcreg);
{	uae_u32 val = (uae_s32)(uae_s16)src;
	xm68k_areg(dstreg) = (val);
}}}xm68k_incpc(2);
}
void xop_3050_0(uae_u32 opcode) /* MOVEA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
{	uae_u32 val = (uae_s32)(uae_s16)src;
	xm68k_areg(dstreg) = (val);
}}}}xm68k_incpc(2);
}
void xop_3058_0(uae_u32 opcode) /* MOVEA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
	xm68k_areg(srcreg) += 2;
{	uae_u32 val = (uae_s32)(uae_s16)src;
	xm68k_areg(dstreg) = (val);
}}}}xm68k_incpc(2);
}
void xop_3060_0(uae_u32 opcode) /* MOVEA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - 2;
{	uae_s16 src = xget_word(srca);
	xm68k_areg (srcreg) = srca;
{	uae_u32 val = (uae_s32)(uae_s16)src;
	xm68k_areg(dstreg) = (val);
}}}}xm68k_incpc(2);
}
void xop_3068_0(uae_u32 opcode) /* MOVEA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_u32 val = (uae_s32)(uae_s16)src;
	xm68k_areg(dstreg) = (val);
}}}}xm68k_incpc(4);
}
void xop_3070_0(uae_u32 opcode) /* MOVEA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s16 src = xget_word(srca);
{	uae_u32 val = (uae_s32)(uae_s16)src;
	xm68k_areg(dstreg) = (val);
}}}}}}
void xop_3078_0(uae_u32 opcode) /* MOVEA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_u32 val = (uae_s32)(uae_s16)src;
	xm68k_areg(dstreg) = (val);
}}}}xm68k_incpc(4);
}
void xop_3079_0(uae_u32 opcode) /* MOVEA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s16 src = xget_word(srca);
{	uae_u32 val = (uae_s32)(uae_s16)src;
	xm68k_areg(dstreg) = (val);
}}}}xm68k_incpc(6);
}
void xop_307a_0(uae_u32 opcode) /* MOVEA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_u32 val = (uae_s32)(uae_s16)src;
	xm68k_areg(dstreg) = (val);
}}}}xm68k_incpc(4);
}
void xop_307b_0(uae_u32 opcode) /* MOVEA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s16 src = xget_word(srca);
{	uae_u32 val = (uae_s32)(uae_s16)src;
	xm68k_areg(dstreg) = (val);
}}}}}}
void xop_307c_0(uae_u32 opcode) /* MOVEA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xget_iword(2);
{	uae_u32 val = (uae_s32)(uae_s16)src;
	xm68k_areg(dstreg) = (val);
}}}xm68k_incpc(4);
}
void xop_3080_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}xm68k_incpc(2);
}
void xop_3088_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xm68k_areg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}xm68k_incpc(2);
}
void xop_3090_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_3098_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
	xm68k_areg(srcreg) += 2;
{	uaecptr dsta = xm68k_areg(dstreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_30a0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - 2;
{	uae_s16 src = xget_word(srca);
	xm68k_areg (srcreg) = srca;
{	uaecptr dsta = xm68k_areg(dstreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_30a8_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_30b0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}}}
void xop_30b8_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_30b9_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_30ba_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_30bb_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}}}
void xop_30bc_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}xm68k_incpc(4);
}
void xop_30c0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
	xm68k_areg(dstreg) += 2;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}xm68k_incpc(2);
}
void xop_30c8_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xm68k_areg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
	xm68k_areg(dstreg) += 2;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}xm68k_incpc(2);
}
void xop_30d0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	xm68k_areg(dstreg) += 2;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_30d8_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
	xm68k_areg(srcreg) += 2;
{	uaecptr dsta = xm68k_areg(dstreg);
	xm68k_areg(dstreg) += 2;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_30e0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - 2;
{	uae_s16 src = xget_word(srca);
	xm68k_areg (srcreg) = srca;
{	uaecptr dsta = xm68k_areg(dstreg);
	xm68k_areg(dstreg) += 2;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_30e8_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	xm68k_areg(dstreg) += 2;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_30f0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	xm68k_areg(dstreg) += 2;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}}}
void xop_30f8_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	xm68k_areg(dstreg) += 2;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_30f9_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	xm68k_areg(dstreg) += 2;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_30fa_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	xm68k_areg(dstreg) += 2;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_30fb_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = xm68k_areg(dstreg);
	xm68k_areg(dstreg) += 2;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}}}
void xop_30fc_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg);
	xm68k_areg(dstreg) += 2;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}xm68k_incpc(4);
}
void xop_3100_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) - 2;
	xm68k_areg (dstreg) = dsta;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}xm68k_incpc(2);
}
void xop_3108_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xm68k_areg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) - 2;
	xm68k_areg (dstreg) = dsta;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}xm68k_incpc(2);
}
void xop_3110_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = xm68k_areg(dstreg) - 2;
	xm68k_areg (dstreg) = dsta;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_3118_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
	xm68k_areg(srcreg) += 2;
{	uaecptr dsta = xm68k_areg(dstreg) - 2;
	xm68k_areg (dstreg) = dsta;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_3120_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - 2;
{	uae_s16 src = xget_word(srca);
	xm68k_areg (srcreg) = srca;
{	uaecptr dsta = xm68k_areg(dstreg) - 2;
	xm68k_areg (dstreg) = dsta;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_3128_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = xm68k_areg(dstreg) - 2;
	xm68k_areg (dstreg) = dsta;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_3130_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = xm68k_areg(dstreg) - 2;
	xm68k_areg (dstreg) = dsta;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}}}
void xop_3138_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = xm68k_areg(dstreg) - 2;
	xm68k_areg (dstreg) = dsta;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_3139_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = xm68k_areg(dstreg) - 2;
	xm68k_areg (dstreg) = dsta;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_313a_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = xm68k_areg(dstreg) - 2;
	xm68k_areg (dstreg) = dsta;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_313b_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = xm68k_areg(dstreg) - 2;
	xm68k_areg (dstreg) = dsta;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}}}
void xop_313c_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg) - 2;
	xm68k_areg (dstreg) = dsta;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}xm68k_incpc(4);
}
void xop_3140_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}xm68k_incpc(4);
}
void xop_3148_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xm68k_areg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}xm68k_incpc(4);
}
void xop_3150_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_3158_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
	xm68k_areg(srcreg) += 2;
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_3160_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - 2;
{	uae_s16 src = xget_word(srca);
	xm68k_areg (srcreg) = srca;
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_3168_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(4);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_3170_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(0);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}}xm68k_incpc(2);
}
void xop_3178_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(4);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_3179_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(6);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(8);
}
void xop_317a_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(4);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_317b_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(0);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}}xm68k_incpc(2);
}
void xop_317c_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(4);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}xm68k_incpc(6);
}
void xop_3180_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{xm68k_incpc(2);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}}
void xop_3188_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xm68k_areg(srcreg);
{xm68k_incpc(2);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}}
void xop_3190_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
{xm68k_incpc(2);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}}}
void xop_3198_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
	xm68k_areg(srcreg) += 2;
{xm68k_incpc(2);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}}}
void xop_31a0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - 2;
{	uae_s16 src = xget_word(srca);
	xm68k_areg (srcreg) = srca;
{xm68k_incpc(2);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}}}
void xop_31a8_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{xm68k_incpc(4);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}}}
void xop_31b0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s16 src = xget_word(srca);
{{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}}}}
void xop_31b8_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{xm68k_incpc(4);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}}}
void xop_31b9_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s16 src = xget_word(srca);
{xm68k_incpc(6);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}}}
void xop_31ba_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{xm68k_incpc(4);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}}}
void xop_31bb_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s16 src = xget_word(srca);
{{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}}}}
void xop_31bc_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xget_iword(2);
{xm68k_incpc(4);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}}
void xop_31c0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}xm68k_incpc(4);
}
void xop_31c8_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = xm68k_areg(srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}xm68k_incpc(4);
}
void xop_31d0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_31d8_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
	xm68k_areg(srcreg) += 2;
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_31e0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) - 2;
{	uae_s16 src = xget_word(srca);
	xm68k_areg (srcreg) = srca;
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_31e8_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(4);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_31f0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(0);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}}xm68k_incpc(2);
}
void xop_31f8_0(uae_u32 opcode) /* MOVE */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(4);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_31f9_0(uae_u32 opcode) /* MOVE */
{
{{	uaecptr srca = xget_ilong(2);
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(6);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(8);
}
void xop_31fa_0(uae_u32 opcode) /* MOVE */
{
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(4);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_31fb_0(uae_u32 opcode) /* MOVE */
{
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(0);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}}xm68k_incpc(2);
}
void xop_31fc_0(uae_u32 opcode) /* MOVE */
{
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(4);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}xm68k_incpc(6);
}
void xop_33c0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xget_ilong(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}xm68k_incpc(6);
}
void xop_33c8_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = xm68k_areg(srcreg);
{	uaecptr dsta = xget_ilong(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}xm68k_incpc(6);
}
void xop_33d0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = xget_ilong(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_33d8_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
	xm68k_areg(srcreg) += 2;
{	uaecptr dsta = xget_ilong(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_33e0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) - 2;
{	uae_s16 src = xget_word(srca);
	xm68k_areg (srcreg) = srca;
{	uaecptr dsta = xget_ilong(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_33e8_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = xget_ilong(4);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(8);
}
void xop_33f0_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = xget_ilong(0);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}}xm68k_incpc(4);
}
void xop_33f8_0(uae_u32 opcode) /* MOVE */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = xget_ilong(4);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(8);
}
void xop_33f9_0(uae_u32 opcode) /* MOVE */
{
{{	uaecptr srca = xget_ilong(2);
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = xget_ilong(6);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(10);
}
void xop_33fa_0(uae_u32 opcode) /* MOVE */
{
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = xget_ilong(4);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(8);
}
void xop_33fb_0(uae_u32 opcode) /* MOVE */
{
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s16 src = xget_word(srca);
{	uaecptr dsta = xget_ilong(0);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}}xm68k_incpc(4);
}
void xop_33fc_0(uae_u32 opcode) /* MOVE */
{
{{	uae_s16 src = xget_iword(2);
{	uaecptr dsta = xget_ilong(4);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}xm68k_incpc(8);
}
void xop_4000_0(uae_u32 opcode) /* NEGX */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uae_u32 newv = 0 - src - (XGET_XFLG ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	XSET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s8)(newv)) == 0));
	XSET_NFLG (((uae_s8)(newv)) < 0);
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xff) | ((newv) & 0xff);
}}}}xm68k_incpc(2);
}
void xop_4010_0(uae_u32 opcode) /* NEGX */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s8 src = xget_byte(srca);
{	uae_u32 newv = 0 - src - (XGET_XFLG ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	XSET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s8)(newv)) == 0));
	XSET_NFLG (((uae_s8)(newv)) < 0);
	xput_byte(srca,newv);
}}}}}xm68k_incpc(2);
}
void xop_4018_0(uae_u32 opcode) /* NEGX */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s8 src = xget_byte(srca);
	xm68k_areg(srcreg) += xareg_byteinc[srcreg];
{	uae_u32 newv = 0 - src - (XGET_XFLG ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	XSET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s8)(newv)) == 0));
	XSET_NFLG (((uae_s8)(newv)) < 0);
	xput_byte(srca,newv);
}}}}}xm68k_incpc(2);
}
void xop_4020_0(uae_u32 opcode) /* NEGX */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) - xareg_byteinc[srcreg];
{	uae_s8 src = xget_byte(srca);
	xm68k_areg (srcreg) = srca;
{	uae_u32 newv = 0 - src - (XGET_XFLG ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	XSET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s8)(newv)) == 0));
	XSET_NFLG (((uae_s8)(newv)) < 0);
	xput_byte(srca,newv);
}}}}}xm68k_incpc(2);
}
void xop_4028_0(uae_u32 opcode) /* NEGX */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	uae_u32 newv = 0 - src - (XGET_XFLG ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	XSET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s8)(newv)) == 0));
	XSET_NFLG (((uae_s8)(newv)) < 0);
	xput_byte(srca,newv);
}}}}}xm68k_incpc(4);
}
void xop_4030_0(uae_u32 opcode) /* NEGX */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s8 src = xget_byte(srca);
{	uae_u32 newv = 0 - src - (XGET_XFLG ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	XSET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s8)(newv)) == 0));
	XSET_NFLG (((uae_s8)(newv)) < 0);
	xput_byte(srca,newv);
}}}}}}}
void xop_4038_0(uae_u32 opcode) /* NEGX */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	uae_u32 newv = 0 - src - (XGET_XFLG ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	XSET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s8)(newv)) == 0));
	XSET_NFLG (((uae_s8)(newv)) < 0);
	xput_byte(srca,newv);
}}}}}xm68k_incpc(4);
}
void xop_4039_0(uae_u32 opcode) /* NEGX */
{
{{	uaecptr srca = xget_ilong(2);
{	uae_s8 src = xget_byte(srca);
{	uae_u32 newv = 0 - src - (XGET_XFLG ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	XSET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s8)(newv)) == 0));
	XSET_NFLG (((uae_s8)(newv)) < 0);
	xput_byte(srca,newv);
}}}}}xm68k_incpc(6);
}
void xop_4040_0(uae_u32 opcode) /* NEGX */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uae_u32 newv = 0 - src - (XGET_XFLG ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	XSET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s16)(newv)) == 0));
	XSET_NFLG (((uae_s16)(newv)) < 0);
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xffff) | ((newv) & 0xffff);
}}}}xm68k_incpc(2);
}
void xop_4050_0(uae_u32 opcode) /* NEGX */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
{	uae_u32 newv = 0 - src - (XGET_XFLG ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	XSET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s16)(newv)) == 0));
	XSET_NFLG (((uae_s16)(newv)) < 0);
	xput_word(srca,newv);
}}}}}xm68k_incpc(2);
}
void xop_4058_0(uae_u32 opcode) /* NEGX */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
	xm68k_areg(srcreg) += 2;
{	uae_u32 newv = 0 - src - (XGET_XFLG ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	XSET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s16)(newv)) == 0));
	XSET_NFLG (((uae_s16)(newv)) < 0);
	xput_word(srca,newv);
}}}}}xm68k_incpc(2);
}
void xop_4060_0(uae_u32 opcode) /* NEGX */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) - 2;
{	uae_s16 src = xget_word(srca);
	xm68k_areg (srcreg) = srca;
{	uae_u32 newv = 0 - src - (XGET_XFLG ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	XSET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s16)(newv)) == 0));
	XSET_NFLG (((uae_s16)(newv)) < 0);
	xput_word(srca,newv);
}}}}}xm68k_incpc(2);
}
void xop_4068_0(uae_u32 opcode) /* NEGX */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_u32 newv = 0 - src - (XGET_XFLG ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	XSET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s16)(newv)) == 0));
	XSET_NFLG (((uae_s16)(newv)) < 0);
	xput_word(srca,newv);
}}}}}xm68k_incpc(4);
}
void xop_4070_0(uae_u32 opcode) /* NEGX */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s16 src = xget_word(srca);
{	uae_u32 newv = 0 - src - (XGET_XFLG ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	XSET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s16)(newv)) == 0));
	XSET_NFLG (((uae_s16)(newv)) < 0);
	xput_word(srca,newv);
}}}}}}}
void xop_4078_0(uae_u32 opcode) /* NEGX */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_u32 newv = 0 - src - (XGET_XFLG ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	XSET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s16)(newv)) == 0));
	XSET_NFLG (((uae_s16)(newv)) < 0);
	xput_word(srca,newv);
}}}}}xm68k_incpc(4);
}
void xop_4079_0(uae_u32 opcode) /* NEGX */
{
{{	uaecptr srca = xget_ilong(2);
{	uae_s16 src = xget_word(srca);
{	uae_u32 newv = 0 - src - (XGET_XFLG ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	XSET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s16)(newv)) == 0));
	XSET_NFLG (((uae_s16)(newv)) < 0);
	xput_word(srca,newv);
}}}}}xm68k_incpc(6);
}
void xop_4080_0(uae_u32 opcode) /* NEGX */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uae_u32 newv = 0 - src - (XGET_XFLG ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	XSET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s32)(newv)) == 0));
	XSET_NFLG (((uae_s32)(newv)) < 0);
	xm68k_dreg(srcreg) = (newv);
}}}}xm68k_incpc(2);
}
void xop_4090_0(uae_u32 opcode) /* NEGX */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
{	uae_u32 newv = 0 - src - (XGET_XFLG ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	XSET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s32)(newv)) == 0));
	XSET_NFLG (((uae_s32)(newv)) < 0);
	xput_long(srca,newv);
}}}}}xm68k_incpc(2);
}
void xop_4098_0(uae_u32 opcode) /* NEGX */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
	xm68k_areg(srcreg) += 4;
{	uae_u32 newv = 0 - src - (XGET_XFLG ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	XSET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s32)(newv)) == 0));
	XSET_NFLG (((uae_s32)(newv)) < 0);
	xput_long(srca,newv);
}}}}}xm68k_incpc(2);
}
void xop_40a0_0(uae_u32 opcode) /* NEGX */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) - 4;
{	uae_s32 src = xget_long(srca);
	xm68k_areg (srcreg) = srca;
{	uae_u32 newv = 0 - src - (XGET_XFLG ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	XSET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s32)(newv)) == 0));
	XSET_NFLG (((uae_s32)(newv)) < 0);
	xput_long(srca,newv);
}}}}}xm68k_incpc(2);
}
void xop_40a8_0(uae_u32 opcode) /* NEGX */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uae_u32 newv = 0 - src - (XGET_XFLG ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	XSET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s32)(newv)) == 0));
	XSET_NFLG (((uae_s32)(newv)) < 0);
	xput_long(srca,newv);
}}}}}xm68k_incpc(4);
}
void xop_40b0_0(uae_u32 opcode) /* NEGX */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s32 src = xget_long(srca);
{	uae_u32 newv = 0 - src - (XGET_XFLG ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	XSET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s32)(newv)) == 0));
	XSET_NFLG (((uae_s32)(newv)) < 0);
	xput_long(srca,newv);
}}}}}}}
void xop_40b8_0(uae_u32 opcode) /* NEGX */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uae_u32 newv = 0 - src - (XGET_XFLG ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	XSET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s32)(newv)) == 0));
	XSET_NFLG (((uae_s32)(newv)) < 0);
	xput_long(srca,newv);
}}}}}xm68k_incpc(4);
}
void xop_40b9_0(uae_u32 opcode) /* NEGX */
{
{{	uaecptr srca = xget_ilong(2);
{	uae_s32 src = xget_long(srca);
{	uae_u32 newv = 0 - src - (XGET_XFLG ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	XSET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s32)(newv)) == 0));
	XSET_NFLG (((uae_s32)(newv)) < 0);
	xput_long(srca,newv);
}}}}}xm68k_incpc(6);
}
void xop_40c0_0(uae_u32 opcode) /* MVSR2 */
{
	uae_u32 srcreg = (opcode & 7);
}
void xop_40d0_0(uae_u32 opcode) /* MVSR2 */
{
	uae_u32 srcreg = (opcode & 7);
}
void xop_40d8_0(uae_u32 opcode) /* MVSR2 */
{
	uae_u32 srcreg = (opcode & 7);
}
void xop_40e0_0(uae_u32 opcode) /* MVSR2 */
{
	uae_u32 srcreg = (opcode & 7);
}
void xop_40e8_0(uae_u32 opcode) /* MVSR2 */
{
	uae_u32 srcreg = (opcode & 7);
}
void xop_40f0_0(uae_u32 opcode) /* MVSR2 */
{
	uae_u32 srcreg = (opcode & 7);
}
void xop_40f8_0(uae_u32 opcode) /* MVSR2 */
{
}
void xop_40f9_0(uae_u32 opcode) /* MVSR2 */
{
}
void xop_4100_0(uae_u32 opcode) /* CHK */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{}}
void xop_4110_0(uae_u32 opcode) /* CHK */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{}}
void xop_4118_0(uae_u32 opcode) /* CHK */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{}}
void xop_4120_0(uae_u32 opcode) /* CHK */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{}}
void xop_4128_0(uae_u32 opcode) /* CHK */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{}}
void xop_4130_0(uae_u32 opcode) /* CHK */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{}}
void xop_4138_0(uae_u32 opcode) /* CHK */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{}}
void xop_4139_0(uae_u32 opcode) /* CHK */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{}}
void xop_413a_0(uae_u32 opcode) /* CHK */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{}}
void xop_413b_0(uae_u32 opcode) /* CHK */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{}}
void xop_413c_0(uae_u32 opcode) /* CHK */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{}}
void xop_4180_0(uae_u32 opcode) /* CHK */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{}}
void xop_4190_0(uae_u32 opcode) /* CHK */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{}}
void xop_4198_0(uae_u32 opcode) /* CHK */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{}}
void xop_41a0_0(uae_u32 opcode) /* CHK */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{}}
void xop_41a8_0(uae_u32 opcode) /* CHK */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{}}
void xop_41b0_0(uae_u32 opcode) /* CHK */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{}}
void xop_41b8_0(uae_u32 opcode) /* CHK */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{}}
void xop_41b9_0(uae_u32 opcode) /* CHK */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{}}
void xop_41ba_0(uae_u32 opcode) /* CHK */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{}}
void xop_41bb_0(uae_u32 opcode) /* CHK */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{}}
void xop_41bc_0(uae_u32 opcode) /* CHK */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{}}
void xop_41d0_0(uae_u32 opcode) /* LEA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	xm68k_areg(dstreg) = (srca);
}}}xm68k_incpc(2);
}
void xop_41e8_0(uae_u32 opcode) /* LEA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	xm68k_areg(dstreg) = (srca);
}}}xm68k_incpc(4);
}
void xop_41f0_0(uae_u32 opcode) /* LEA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	xm68k_areg(dstreg) = (srca);
}}}}}
void xop_41f8_0(uae_u32 opcode) /* LEA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	xm68k_areg(dstreg) = (srca);
}}}xm68k_incpc(4);
}
void xop_41f9_0(uae_u32 opcode) /* LEA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	xm68k_areg(dstreg) = (srca);
}}}xm68k_incpc(6);
}
void xop_41fa_0(uae_u32 opcode) /* LEA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	xm68k_areg(dstreg) = (srca);
}}}xm68k_incpc(4);
}
void xop_41fb_0(uae_u32 opcode) /* LEA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	xm68k_areg(dstreg) = (srca);
}}}}}
void xop_4200_0(uae_u32 opcode) /* CLR */
{
	uae_u32 srcreg = (opcode & 7);
{{	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(0)) == 0);
	XSET_NFLG (((uae_s8)(0)) < 0);
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xff) | ((0) & 0xff);
}}xm68k_incpc(2);
}
void xop_4210_0(uae_u32 opcode) /* CLR */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(0)) == 0);
	XSET_NFLG (((uae_s8)(0)) < 0);
	xput_byte(srca,0);
}}xm68k_incpc(2);
}
void xop_4218_0(uae_u32 opcode) /* CLR */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
	xm68k_areg(srcreg) += xareg_byteinc[srcreg];
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(0)) == 0);
	XSET_NFLG (((uae_s8)(0)) < 0);
	xput_byte(srca,0);
}}xm68k_incpc(2);
}
void xop_4220_0(uae_u32 opcode) /* CLR */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) - xareg_byteinc[srcreg];
	xm68k_areg (srcreg) = srca;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(0)) == 0);
	XSET_NFLG (((uae_s8)(0)) < 0);
	xput_byte(srca,0);
}}xm68k_incpc(2);
}
void xop_4228_0(uae_u32 opcode) /* CLR */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(0)) == 0);
	XSET_NFLG (((uae_s8)(0)) < 0);
	xput_byte(srca,0);
}}xm68k_incpc(4);
}
void xop_4230_0(uae_u32 opcode) /* CLR */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(0)) == 0);
	XSET_NFLG (((uae_s8)(0)) < 0);
	xput_byte(srca,0);
}}}}
void xop_4238_0(uae_u32 opcode) /* CLR */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(0)) == 0);
	XSET_NFLG (((uae_s8)(0)) < 0);
	xput_byte(srca,0);
}}xm68k_incpc(4);
}
void xop_4239_0(uae_u32 opcode) /* CLR */
{
{{	uaecptr srca = xget_ilong(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(0)) == 0);
	XSET_NFLG (((uae_s8)(0)) < 0);
	xput_byte(srca,0);
}}xm68k_incpc(6);
}
void xop_4240_0(uae_u32 opcode) /* CLR */
{
	uae_u32 srcreg = (opcode & 7);
{{	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(0)) == 0);
	XSET_NFLG (((uae_s16)(0)) < 0);
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xffff) | ((0) & 0xffff);
}}xm68k_incpc(2);
}
void xop_4250_0(uae_u32 opcode) /* CLR */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(0)) == 0);
	XSET_NFLG (((uae_s16)(0)) < 0);
	xput_word(srca,0);
}}xm68k_incpc(2);
}
void xop_4258_0(uae_u32 opcode) /* CLR */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
	xm68k_areg(srcreg) += 2;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(0)) == 0);
	XSET_NFLG (((uae_s16)(0)) < 0);
	xput_word(srca,0);
}}xm68k_incpc(2);
}
void xop_4260_0(uae_u32 opcode) /* CLR */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) - 2;
	xm68k_areg (srcreg) = srca;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(0)) == 0);
	XSET_NFLG (((uae_s16)(0)) < 0);
	xput_word(srca,0);
}}xm68k_incpc(2);
}
void xop_4268_0(uae_u32 opcode) /* CLR */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(0)) == 0);
	XSET_NFLG (((uae_s16)(0)) < 0);
	xput_word(srca,0);
}}xm68k_incpc(4);
}
void xop_4270_0(uae_u32 opcode) /* CLR */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(0)) == 0);
	XSET_NFLG (((uae_s16)(0)) < 0);
	xput_word(srca,0);
}}}}
void xop_4278_0(uae_u32 opcode) /* CLR */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(0)) == 0);
	XSET_NFLG (((uae_s16)(0)) < 0);
	xput_word(srca,0);
}}xm68k_incpc(4);
}
void xop_4279_0(uae_u32 opcode) /* CLR */
{
{{	uaecptr srca = xget_ilong(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(0)) == 0);
	XSET_NFLG (((uae_s16)(0)) < 0);
	xput_word(srca,0);
}}xm68k_incpc(6);
}
void xop_4280_0(uae_u32 opcode) /* CLR */
{
	uae_u32 srcreg = (opcode & 7);
{{	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(0)) == 0);
	XSET_NFLG (((uae_s32)(0)) < 0);
	xm68k_dreg(srcreg) = (0);
}}xm68k_incpc(2);
}
void xop_4290_0(uae_u32 opcode) /* CLR */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(0)) == 0);
	XSET_NFLG (((uae_s32)(0)) < 0);
	xput_long(srca,0);
}}xm68k_incpc(2);
}
void xop_4298_0(uae_u32 opcode) /* CLR */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
	xm68k_areg(srcreg) += 4;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(0)) == 0);
	XSET_NFLG (((uae_s32)(0)) < 0);
	xput_long(srca,0);
}}xm68k_incpc(2);
}
void xop_42a0_0(uae_u32 opcode) /* CLR */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) - 4;
	xm68k_areg (srcreg) = srca;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(0)) == 0);
	XSET_NFLG (((uae_s32)(0)) < 0);
	xput_long(srca,0);
}}xm68k_incpc(2);
}
void xop_42a8_0(uae_u32 opcode) /* CLR */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(0)) == 0);
	XSET_NFLG (((uae_s32)(0)) < 0);
	xput_long(srca,0);
}}xm68k_incpc(4);
}
void xop_42b0_0(uae_u32 opcode) /* CLR */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(0)) == 0);
	XSET_NFLG (((uae_s32)(0)) < 0);
	xput_long(srca,0);
}}}}
void xop_42b8_0(uae_u32 opcode) /* CLR */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(0)) == 0);
	XSET_NFLG (((uae_s32)(0)) < 0);
	xput_long(srca,0);
}}xm68k_incpc(4);
}
void xop_42b9_0(uae_u32 opcode) /* CLR */
{
{{	uaecptr srca = xget_ilong(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(0)) == 0);
	XSET_NFLG (((uae_s32)(0)) < 0);
	xput_long(srca,0);
}}xm68k_incpc(6);
}
void xop_42c0_0(uae_u32 opcode) /* MVSR2 */
{
	uae_u32 srcreg = (opcode & 7);
{}}
void xop_42d0_0(uae_u32 opcode) /* MVSR2 */
{
	uae_u32 srcreg = (opcode & 7);
{}}
void xop_42d8_0(uae_u32 opcode) /* MVSR2 */
{
	uae_u32 srcreg = (opcode & 7);
{}}
void xop_42e0_0(uae_u32 opcode) /* MVSR2 */
{
	uae_u32 srcreg = (opcode & 7);
{}}
void xop_42e8_0(uae_u32 opcode) /* MVSR2 */
{
	uae_u32 srcreg = (opcode & 7);
{}}
void xop_42f0_0(uae_u32 opcode) /* MVSR2 */
{
	uae_u32 srcreg = (opcode & 7);
{}}
void xop_42f8_0(uae_u32 opcode) /* MVSR2 */
{
{}}
void xop_42f9_0(uae_u32 opcode) /* MVSR2 */
{
{}}
void xop_4400_0(uae_u32 opcode) /* NEG */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = xm68k_dreg(srcreg);
{{uae_u32 dst = ((uae_s8)(0)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(dst)) < 0;
	XSET_ZFLG (((uae_s8)(dst)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(0)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xff) | ((dst) & 0xff);
}}}}}xm68k_incpc(2);
}
void xop_4410_0(uae_u32 opcode) /* NEG */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s8 src = xget_byte(srca);
{{uae_u32 dst = ((uae_s8)(0)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(dst)) < 0;
	XSET_ZFLG (((uae_s8)(dst)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(0)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(srca,dst);
}}}}}}xm68k_incpc(2);
}
void xop_4418_0(uae_u32 opcode) /* NEG */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s8 src = xget_byte(srca);
	xm68k_areg(srcreg) += xareg_byteinc[srcreg];
{{uae_u32 dst = ((uae_s8)(0)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(dst)) < 0;
	XSET_ZFLG (((uae_s8)(dst)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(0)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(srca,dst);
}}}}}}xm68k_incpc(2);
}
void xop_4420_0(uae_u32 opcode) /* NEG */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) - xareg_byteinc[srcreg];
{	uae_s8 src = xget_byte(srca);
	xm68k_areg (srcreg) = srca;
{{uae_u32 dst = ((uae_s8)(0)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(dst)) < 0;
	XSET_ZFLG (((uae_s8)(dst)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(0)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(srca,dst);
}}}}}}xm68k_incpc(2);
}
void xop_4428_0(uae_u32 opcode) /* NEG */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{{uae_u32 dst = ((uae_s8)(0)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(dst)) < 0;
	XSET_ZFLG (((uae_s8)(dst)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(0)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(srca,dst);
}}}}}}xm68k_incpc(4);
}
void xop_4430_0(uae_u32 opcode) /* NEG */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s8 src = xget_byte(srca);
{{uae_u32 dst = ((uae_s8)(0)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(dst)) < 0;
	XSET_ZFLG (((uae_s8)(dst)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(0)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(srca,dst);
}}}}}}}}
void xop_4438_0(uae_u32 opcode) /* NEG */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{{uae_u32 dst = ((uae_s8)(0)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(dst)) < 0;
	XSET_ZFLG (((uae_s8)(dst)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(0)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(srca,dst);
}}}}}}xm68k_incpc(4);
}
void xop_4439_0(uae_u32 opcode) /* NEG */
{
{{	uaecptr srca = xget_ilong(2);
{	uae_s8 src = xget_byte(srca);
{{uae_u32 dst = ((uae_s8)(0)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(0)) < 0;
	int flgn = ((uae_s8)(dst)) < 0;
	XSET_ZFLG (((uae_s8)(dst)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(0)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(srca,dst);
}}}}}}xm68k_incpc(6);
}
void xop_4440_0(uae_u32 opcode) /* NEG */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = xm68k_dreg(srcreg);
{{uae_u32 dst = ((uae_s16)(0)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(dst)) < 0;
	XSET_ZFLG (((uae_s16)(dst)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(0)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xffff) | ((dst) & 0xffff);
}}}}}xm68k_incpc(2);
}
void xop_4450_0(uae_u32 opcode) /* NEG */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
{{uae_u32 dst = ((uae_s16)(0)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(dst)) < 0;
	XSET_ZFLG (((uae_s16)(dst)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(0)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(srca,dst);
}}}}}}xm68k_incpc(2);
}
void xop_4458_0(uae_u32 opcode) /* NEG */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
	xm68k_areg(srcreg) += 2;
{{uae_u32 dst = ((uae_s16)(0)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(dst)) < 0;
	XSET_ZFLG (((uae_s16)(dst)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(0)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(srca,dst);
}}}}}}xm68k_incpc(2);
}
void xop_4460_0(uae_u32 opcode) /* NEG */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) - 2;
{	uae_s16 src = xget_word(srca);
	xm68k_areg (srcreg) = srca;
{{uae_u32 dst = ((uae_s16)(0)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(dst)) < 0;
	XSET_ZFLG (((uae_s16)(dst)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(0)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(srca,dst);
}}}}}}xm68k_incpc(2);
}
void xop_4468_0(uae_u32 opcode) /* NEG */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{{uae_u32 dst = ((uae_s16)(0)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(dst)) < 0;
	XSET_ZFLG (((uae_s16)(dst)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(0)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(srca,dst);
}}}}}}xm68k_incpc(4);
}
void xop_4470_0(uae_u32 opcode) /* NEG */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s16 src = xget_word(srca);
{{uae_u32 dst = ((uae_s16)(0)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(dst)) < 0;
	XSET_ZFLG (((uae_s16)(dst)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(0)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(srca,dst);
}}}}}}}}
void xop_4478_0(uae_u32 opcode) /* NEG */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{{uae_u32 dst = ((uae_s16)(0)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(dst)) < 0;
	XSET_ZFLG (((uae_s16)(dst)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(0)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(srca,dst);
}}}}}}xm68k_incpc(4);
}
void xop_4479_0(uae_u32 opcode) /* NEG */
{
{{	uaecptr srca = xget_ilong(2);
{	uae_s16 src = xget_word(srca);
{{uae_u32 dst = ((uae_s16)(0)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(0)) < 0;
	int flgn = ((uae_s16)(dst)) < 0;
	XSET_ZFLG (((uae_s16)(dst)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(0)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(srca,dst);
}}}}}}xm68k_incpc(6);
}
void xop_4480_0(uae_u32 opcode) /* NEG */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = xm68k_dreg(srcreg);
{{uae_u32 dst = ((uae_s32)(0)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(dst)) < 0;
	XSET_ZFLG (((uae_s32)(dst)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(0)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(srcreg) = (dst);
}}}}}xm68k_incpc(2);
}
void xop_4490_0(uae_u32 opcode) /* NEG */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
{{uae_u32 dst = ((uae_s32)(0)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(dst)) < 0;
	XSET_ZFLG (((uae_s32)(dst)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(0)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(srca,dst);
}}}}}}xm68k_incpc(2);
}
void xop_4498_0(uae_u32 opcode) /* NEG */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
	xm68k_areg(srcreg) += 4;
{{uae_u32 dst = ((uae_s32)(0)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(dst)) < 0;
	XSET_ZFLG (((uae_s32)(dst)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(0)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(srca,dst);
}}}}}}xm68k_incpc(2);
}
void xop_44a0_0(uae_u32 opcode) /* NEG */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) - 4;
{	uae_s32 src = xget_long(srca);
	xm68k_areg (srcreg) = srca;
{{uae_u32 dst = ((uae_s32)(0)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(dst)) < 0;
	XSET_ZFLG (((uae_s32)(dst)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(0)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(srca,dst);
}}}}}}xm68k_incpc(2);
}
void xop_44a8_0(uae_u32 opcode) /* NEG */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{{uae_u32 dst = ((uae_s32)(0)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(dst)) < 0;
	XSET_ZFLG (((uae_s32)(dst)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(0)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(srca,dst);
}}}}}}xm68k_incpc(4);
}
void xop_44b0_0(uae_u32 opcode) /* NEG */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s32 src = xget_long(srca);
{{uae_u32 dst = ((uae_s32)(0)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(dst)) < 0;
	XSET_ZFLG (((uae_s32)(dst)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(0)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(srca,dst);
}}}}}}}}
void xop_44b8_0(uae_u32 opcode) /* NEG */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{{uae_u32 dst = ((uae_s32)(0)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(dst)) < 0;
	XSET_ZFLG (((uae_s32)(dst)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(0)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(srca,dst);
}}}}}}xm68k_incpc(4);
}
void xop_44b9_0(uae_u32 opcode) /* NEG */
{
{{	uaecptr srca = xget_ilong(2);
{	uae_s32 src = xget_long(srca);
{{uae_u32 dst = ((uae_s32)(0)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(0)) < 0;
	int flgn = ((uae_s32)(dst)) < 0;
	XSET_ZFLG (((uae_s32)(dst)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(0)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(srca,dst);
}}}}}}xm68k_incpc(6);
}
void xop_44c0_0(uae_u32 opcode) /* MV2SR */
{
	uae_u32 srcreg = (opcode & 7);
{}}
void xop_44d0_0(uae_u32 opcode) /* MV2SR */
{
	uae_u32 srcreg = (opcode & 7);
{}}
void xop_44d8_0(uae_u32 opcode) /* MV2SR */
{
	uae_u32 srcreg = (opcode & 7);
{}}
void xop_44e0_0(uae_u32 opcode) /* MV2SR */
{
	uae_u32 srcreg = (opcode & 7);
{}}
void xop_44e8_0(uae_u32 opcode) /* MV2SR */
{
	uae_u32 srcreg = (opcode & 7);
{}}
void xop_44f0_0(uae_u32 opcode) /* MV2SR */
{
	uae_u32 srcreg = (opcode & 7);
{}}
void xop_44f8_0(uae_u32 opcode) /* MV2SR */
{
{}}
void xop_44f9_0(uae_u32 opcode) /* MV2SR */
{
{}}
void xop_44fa_0(uae_u32 opcode) /* MV2SR */
{
{}}
void xop_44fb_0(uae_u32 opcode) /* MV2SR */
{
{}}
void xop_44fc_0(uae_u32 opcode) /* MV2SR */
{
{}}
void xop_4600_0(uae_u32 opcode) /* NOT */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uae_u32 dst = ~src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(dst)) == 0);
	XSET_NFLG (((uae_s8)(dst)) < 0);
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xff) | ((dst) & 0xff);
}}}xm68k_incpc(2);
}
void xop_4610_0(uae_u32 opcode) /* NOT */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s8 src = xget_byte(srca);
{	uae_u32 dst = ~src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(dst)) == 0);
	XSET_NFLG (((uae_s8)(dst)) < 0);
	xput_byte(srca,dst);
}}}}xm68k_incpc(2);
}
void xop_4618_0(uae_u32 opcode) /* NOT */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s8 src = xget_byte(srca);
	xm68k_areg(srcreg) += xareg_byteinc[srcreg];
{	uae_u32 dst = ~src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(dst)) == 0);
	XSET_NFLG (((uae_s8)(dst)) < 0);
	xput_byte(srca,dst);
}}}}xm68k_incpc(2);
}
void xop_4620_0(uae_u32 opcode) /* NOT */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) - xareg_byteinc[srcreg];
{	uae_s8 src = xget_byte(srca);
	xm68k_areg (srcreg) = srca;
{	uae_u32 dst = ~src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(dst)) == 0);
	XSET_NFLG (((uae_s8)(dst)) < 0);
	xput_byte(srca,dst);
}}}}xm68k_incpc(2);
}
void xop_4628_0(uae_u32 opcode) /* NOT */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	uae_u32 dst = ~src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(dst)) == 0);
	XSET_NFLG (((uae_s8)(dst)) < 0);
	xput_byte(srca,dst);
}}}}xm68k_incpc(4);
}
void xop_4630_0(uae_u32 opcode) /* NOT */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s8 src = xget_byte(srca);
{	uae_u32 dst = ~src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(dst)) == 0);
	XSET_NFLG (((uae_s8)(dst)) < 0);
	xput_byte(srca,dst);
}}}}}}
void xop_4638_0(uae_u32 opcode) /* NOT */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	uae_u32 dst = ~src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(dst)) == 0);
	XSET_NFLG (((uae_s8)(dst)) < 0);
	xput_byte(srca,dst);
}}}}xm68k_incpc(4);
}
void xop_4639_0(uae_u32 opcode) /* NOT */
{
{{	uaecptr srca = xget_ilong(2);
{	uae_s8 src = xget_byte(srca);
{	uae_u32 dst = ~src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(dst)) == 0);
	XSET_NFLG (((uae_s8)(dst)) < 0);
	xput_byte(srca,dst);
}}}}xm68k_incpc(6);
}
void xop_4640_0(uae_u32 opcode) /* NOT */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uae_u32 dst = ~src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(dst)) == 0);
	XSET_NFLG (((uae_s16)(dst)) < 0);
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xffff) | ((dst) & 0xffff);
}}}xm68k_incpc(2);
}
void xop_4650_0(uae_u32 opcode) /* NOT */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
{	uae_u32 dst = ~src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(dst)) == 0);
	XSET_NFLG (((uae_s16)(dst)) < 0);
	xput_word(srca,dst);
}}}}xm68k_incpc(2);
}
void xop_4658_0(uae_u32 opcode) /* NOT */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
	xm68k_areg(srcreg) += 2;
{	uae_u32 dst = ~src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(dst)) == 0);
	XSET_NFLG (((uae_s16)(dst)) < 0);
	xput_word(srca,dst);
}}}}xm68k_incpc(2);
}
void xop_4660_0(uae_u32 opcode) /* NOT */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) - 2;
{	uae_s16 src = xget_word(srca);
	xm68k_areg (srcreg) = srca;
{	uae_u32 dst = ~src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(dst)) == 0);
	XSET_NFLG (((uae_s16)(dst)) < 0);
	xput_word(srca,dst);
}}}}xm68k_incpc(2);
}
void xop_4668_0(uae_u32 opcode) /* NOT */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_u32 dst = ~src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(dst)) == 0);
	XSET_NFLG (((uae_s16)(dst)) < 0);
	xput_word(srca,dst);
}}}}xm68k_incpc(4);
}
void xop_4670_0(uae_u32 opcode) /* NOT */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s16 src = xget_word(srca);
{	uae_u32 dst = ~src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(dst)) == 0);
	XSET_NFLG (((uae_s16)(dst)) < 0);
	xput_word(srca,dst);
}}}}}}
void xop_4678_0(uae_u32 opcode) /* NOT */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_u32 dst = ~src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(dst)) == 0);
	XSET_NFLG (((uae_s16)(dst)) < 0);
	xput_word(srca,dst);
}}}}xm68k_incpc(4);
}
void xop_4679_0(uae_u32 opcode) /* NOT */
{
{{	uaecptr srca = xget_ilong(2);
{	uae_s16 src = xget_word(srca);
{	uae_u32 dst = ~src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(dst)) == 0);
	XSET_NFLG (((uae_s16)(dst)) < 0);
	xput_word(srca,dst);
}}}}xm68k_incpc(6);
}
void xop_4680_0(uae_u32 opcode) /* NOT */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uae_u32 dst = ~src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(dst)) == 0);
	XSET_NFLG (((uae_s32)(dst)) < 0);
	xm68k_dreg(srcreg) = (dst);
}}}xm68k_incpc(2);
}
void xop_4690_0(uae_u32 opcode) /* NOT */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
{	uae_u32 dst = ~src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(dst)) == 0);
	XSET_NFLG (((uae_s32)(dst)) < 0);
	xput_long(srca,dst);
}}}}xm68k_incpc(2);
}
void xop_4698_0(uae_u32 opcode) /* NOT */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
	xm68k_areg(srcreg) += 4;
{	uae_u32 dst = ~src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(dst)) == 0);
	XSET_NFLG (((uae_s32)(dst)) < 0);
	xput_long(srca,dst);
}}}}xm68k_incpc(2);
}
void xop_46a0_0(uae_u32 opcode) /* NOT */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) - 4;
{	uae_s32 src = xget_long(srca);
	xm68k_areg (srcreg) = srca;
{	uae_u32 dst = ~src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(dst)) == 0);
	XSET_NFLG (((uae_s32)(dst)) < 0);
	xput_long(srca,dst);
}}}}xm68k_incpc(2);
}
void xop_46a8_0(uae_u32 opcode) /* NOT */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uae_u32 dst = ~src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(dst)) == 0);
	XSET_NFLG (((uae_s32)(dst)) < 0);
	xput_long(srca,dst);
}}}}xm68k_incpc(4);
}
void xop_46b0_0(uae_u32 opcode) /* NOT */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s32 src = xget_long(srca);
{	uae_u32 dst = ~src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(dst)) == 0);
	XSET_NFLG (((uae_s32)(dst)) < 0);
	xput_long(srca,dst);
}}}}}}
void xop_46b8_0(uae_u32 opcode) /* NOT */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uae_u32 dst = ~src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(dst)) == 0);
	XSET_NFLG (((uae_s32)(dst)) < 0);
	xput_long(srca,dst);
}}}}xm68k_incpc(4);
}
void xop_46b9_0(uae_u32 opcode) /* NOT */
{
{{	uaecptr srca = xget_ilong(2);
{	uae_s32 src = xget_long(srca);
{	uae_u32 dst = ~src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(dst)) == 0);
	XSET_NFLG (((uae_s32)(dst)) < 0);
	xput_long(srca,dst);
}}}}xm68k_incpc(6);
}
void xop_46c0_0(uae_u32 opcode) /* MV2SR */
{
	uae_u32 srcreg = (opcode & 7);
}
void xop_46d0_0(uae_u32 opcode) /* MV2SR */
{
	uae_u32 srcreg = (opcode & 7);
}
void xop_46d8_0(uae_u32 opcode) /* MV2SR */
{
	uae_u32 srcreg = (opcode & 7);
}
void xop_46e0_0(uae_u32 opcode) /* MV2SR */
{
	uae_u32 srcreg = (opcode & 7);
}
void xop_46e8_0(uae_u32 opcode) /* MV2SR */
{
	uae_u32 srcreg = (opcode & 7);
}
void xop_46f0_0(uae_u32 opcode) /* MV2SR */
{
	uae_u32 srcreg = (opcode & 7);
}
void xop_46f8_0(uae_u32 opcode) /* MV2SR */
{
}
void xop_46f9_0(uae_u32 opcode) /* MV2SR */
{
}
void xop_46fa_0(uae_u32 opcode) /* MV2SR */
{
}
void xop_46fb_0(uae_u32 opcode) /* MV2SR */
{
}
void xop_46fc_0(uae_u32 opcode) /* MV2SR */
{
}
void xop_4800_0(uae_u32 opcode) /* NBCD */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uae_u16 newv_lo = - (src & 0xF) - (XGET_XFLG ? 1 : 0);
	uae_u16 newv_hi = - (src & 0xF0);
	uae_u16 newv;
	int cflg;
	if (newv_lo > 9) { newv_lo -= 6; }
	newv = newv_hi + newv_lo;	cflg = (newv & 0x1F0) > 0x90;
	if (cflg) newv -= 0x60;
	XSET_CFLG (cflg);
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s8)(newv)) == 0));
	XSET_NFLG (((uae_s8)(newv)) < 0);
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xff) | ((newv) & 0xff);
}}}xm68k_incpc(2);
}
void xop_4808_0(uae_u32 opcode) /* LINK */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr olda = xm68k_areg(7) - 4;
	xm68k_areg (7) = olda;
{	uae_s32 src = xm68k_areg(srcreg);
	xput_long(olda,src);
	xm68k_areg(srcreg) = (xm68k_areg(7));
{	uae_s32 offs = xget_ilong(2);
	xm68k_areg(7) += offs;
}}}}xm68k_incpc(6);
}
void xop_4810_0(uae_u32 opcode) /* NBCD */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s8 src = xget_byte(srca);
{	uae_u16 newv_lo = - (src & 0xF) - (XGET_XFLG ? 1 : 0);
	uae_u16 newv_hi = - (src & 0xF0);
	uae_u16 newv;
	int cflg;
	if (newv_lo > 9) { newv_lo -= 6; }
	newv = newv_hi + newv_lo;	cflg = (newv & 0x1F0) > 0x90;
	if (cflg) newv -= 0x60;
	XSET_CFLG (cflg);
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s8)(newv)) == 0));
	XSET_NFLG (((uae_s8)(newv)) < 0);
	xput_byte(srca,newv);
}}}}xm68k_incpc(2);
}
void xop_4818_0(uae_u32 opcode) /* NBCD */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s8 src = xget_byte(srca);
	xm68k_areg(srcreg) += xareg_byteinc[srcreg];
{	uae_u16 newv_lo = - (src & 0xF) - (XGET_XFLG ? 1 : 0);
	uae_u16 newv_hi = - (src & 0xF0);
	uae_u16 newv;
	int cflg;
	if (newv_lo > 9) { newv_lo -= 6; }
	newv = newv_hi + newv_lo;	cflg = (newv & 0x1F0) > 0x90;
	if (cflg) newv -= 0x60;
	XSET_CFLG (cflg);
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s8)(newv)) == 0));
	XSET_NFLG (((uae_s8)(newv)) < 0);
	xput_byte(srca,newv);
}}}}xm68k_incpc(2);
}
void xop_4820_0(uae_u32 opcode) /* NBCD */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) - xareg_byteinc[srcreg];
{	uae_s8 src = xget_byte(srca);
	xm68k_areg (srcreg) = srca;
{	uae_u16 newv_lo = - (src & 0xF) - (XGET_XFLG ? 1 : 0);
	uae_u16 newv_hi = - (src & 0xF0);
	uae_u16 newv;
	int cflg;
	if (newv_lo > 9) { newv_lo -= 6; }
	newv = newv_hi + newv_lo;	cflg = (newv & 0x1F0) > 0x90;
	if (cflg) newv -= 0x60;
	XSET_CFLG (cflg);
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s8)(newv)) == 0));
	XSET_NFLG (((uae_s8)(newv)) < 0);
	xput_byte(srca,newv);
}}}}xm68k_incpc(2);
}
void xop_4828_0(uae_u32 opcode) /* NBCD */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	uae_u16 newv_lo = - (src & 0xF) - (XGET_XFLG ? 1 : 0);
	uae_u16 newv_hi = - (src & 0xF0);
	uae_u16 newv;
	int cflg;
	if (newv_lo > 9) { newv_lo -= 6; }
	newv = newv_hi + newv_lo;	cflg = (newv & 0x1F0) > 0x90;
	if (cflg) newv -= 0x60;
	XSET_CFLG (cflg);
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s8)(newv)) == 0));
	XSET_NFLG (((uae_s8)(newv)) < 0);
	xput_byte(srca,newv);
}}}}xm68k_incpc(4);
}
void xop_4830_0(uae_u32 opcode) /* NBCD */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s8 src = xget_byte(srca);
{	uae_u16 newv_lo = - (src & 0xF) - (XGET_XFLG ? 1 : 0);
	uae_u16 newv_hi = - (src & 0xF0);
	uae_u16 newv;
	int cflg;
	if (newv_lo > 9) { newv_lo -= 6; }
	newv = newv_hi + newv_lo;	cflg = (newv & 0x1F0) > 0x90;
	if (cflg) newv -= 0x60;
	XSET_CFLG (cflg);
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s8)(newv)) == 0));
	XSET_NFLG (((uae_s8)(newv)) < 0);
	xput_byte(srca,newv);
}}}}}}
void xop_4838_0(uae_u32 opcode) /* NBCD */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	uae_u16 newv_lo = - (src & 0xF) - (XGET_XFLG ? 1 : 0);
	uae_u16 newv_hi = - (src & 0xF0);
	uae_u16 newv;
	int cflg;
	if (newv_lo > 9) { newv_lo -= 6; }
	newv = newv_hi + newv_lo;	cflg = (newv & 0x1F0) > 0x90;
	if (cflg) newv -= 0x60;
	XSET_CFLG (cflg);
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s8)(newv)) == 0));
	XSET_NFLG (((uae_s8)(newv)) < 0);
	xput_byte(srca,newv);
}}}}xm68k_incpc(4);
}
void xop_4839_0(uae_u32 opcode) /* NBCD */
{
{{	uaecptr srca = xget_ilong(2);
{	uae_s8 src = xget_byte(srca);
{	uae_u16 newv_lo = - (src & 0xF) - (XGET_XFLG ? 1 : 0);
	uae_u16 newv_hi = - (src & 0xF0);
	uae_u16 newv;
	int cflg;
	if (newv_lo > 9) { newv_lo -= 6; }
	newv = newv_hi + newv_lo;	cflg = (newv & 0x1F0) > 0x90;
	if (cflg) newv -= 0x60;
	XSET_CFLG (cflg);
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s8)(newv)) == 0));
	XSET_NFLG (((uae_s8)(newv)) < 0);
	xput_byte(srca,newv);
}}}}xm68k_incpc(6);
}
void xop_4840_0(uae_u32 opcode) /* SWAP */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uae_u32 dst = ((src >> 16)&0xFFFF) | ((src&0xFFFF)<<16);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(dst)) == 0);
	XSET_NFLG (((uae_s32)(dst)) < 0);
	xm68k_dreg(srcreg) = (dst);
}}}xm68k_incpc(2);
}
void xop_4848_0(uae_u32 opcode) /* BKPT */
{
	uae_u32 srcreg = (opcode & 7);
{}}
void xop_4850_0(uae_u32 opcode) /* PEA */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	uaecptr dsta = xm68k_areg(7) - 4;
	xm68k_areg (7) = dsta;
	xput_long(dsta,srca);
}}}xm68k_incpc(2);
}
void xop_4868_0(uae_u32 opcode) /* PEA */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uaecptr dsta = xm68k_areg(7) - 4;
	xm68k_areg (7) = dsta;
	xput_long(dsta,srca);
}}}xm68k_incpc(4);
}
void xop_4870_0(uae_u32 opcode) /* PEA */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uaecptr dsta = xm68k_areg(7) - 4;
	xm68k_areg (7) = dsta;
	xput_long(dsta,srca);
}}}}}
void xop_4878_0(uae_u32 opcode) /* PEA */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uaecptr dsta = xm68k_areg(7) - 4;
	xm68k_areg (7) = dsta;
	xput_long(dsta,srca);
}}}xm68k_incpc(4);
}
void xop_4879_0(uae_u32 opcode) /* PEA */
{
{{	uaecptr srca = xget_ilong(2);
{	uaecptr dsta = xm68k_areg(7) - 4;
	xm68k_areg (7) = dsta;
	xput_long(dsta,srca);
}}}xm68k_incpc(6);
}
void xop_487a_0(uae_u32 opcode) /* PEA */
{
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uaecptr dsta = xm68k_areg(7) - 4;
	xm68k_areg (7) = dsta;
	xput_long(dsta,srca);
}}}xm68k_incpc(4);
}
void xop_487b_0(uae_u32 opcode) /* PEA */
{
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uaecptr dsta = xm68k_areg(7) - 4;
	xm68k_areg (7) = dsta;
	xput_long(dsta,srca);
}}}}}
void xop_4880_0(uae_u32 opcode) /* EXT */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uae_u16 dst = (uae_s16)(uae_s8)src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(dst)) == 0);
	XSET_NFLG (((uae_s16)(dst)) < 0);
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xffff) | ((dst) & 0xffff);
}}}xm68k_incpc(2);
}
void xop_4890_0(uae_u32 opcode) /* MVMLE */
{
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = xget_iword(2);
{	uaecptr srca = xm68k_areg(dstreg);
{	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) { xput_word(srca, xm68k_dreg(xmovem_index1[dmask])); srca += 2; dmask = xmovem_next[dmask]; }
	while (amask) { xput_word(srca, xm68k_areg(xmovem_index1[amask])); srca += 2; amask = xmovem_next[amask]; }
}}}xm68k_incpc(4);
}
void xop_48a0_0(uae_u32 opcode) /* MVMLE */
{
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = xget_iword(2);
{	uaecptr srca = xm68k_areg(dstreg) - 0;
{	uae_u16 amask = mask & 0xff, dmask = (mask >> 8) & 0xff;
	while (amask) { srca -= 2; xput_word(srca, xm68k_areg(xmovem_index2[amask])); amask = xmovem_next[amask]; }
	while (dmask) { srca -= 2; xput_word(srca, xm68k_dreg(xmovem_index2[dmask])); dmask = xmovem_next[dmask]; }
	xm68k_areg(dstreg) = srca;
}}}xm68k_incpc(4);
}
void xop_48a8_0(uae_u32 opcode) /* MVMLE */
{
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = xget_iword(2);
{	uaecptr srca = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(4);
{	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) { xput_word(srca, xm68k_dreg(xmovem_index1[dmask])); srca += 2; dmask = xmovem_next[dmask]; }
	while (amask) { xput_word(srca, xm68k_areg(xmovem_index1[amask])); srca += 2; amask = xmovem_next[amask]; }
}}}xm68k_incpc(6);
}
void xop_48b0_0(uae_u32 opcode) /* MVMLE */
{
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = xget_iword(2);
{xm68k_incpc(4);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) { xput_word(srca, xm68k_dreg(xmovem_index1[dmask])); srca += 2; dmask = xmovem_next[dmask]; }
	while (amask) { xput_word(srca, xm68k_areg(xmovem_index1[amask])); srca += 2; amask = xmovem_next[amask]; }
}}}}}
void xop_48b8_0(uae_u32 opcode) /* MVMLE */
{
{	uae_u16 mask = xget_iword(2);
{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(4);
{	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) { xput_word(srca, xm68k_dreg(xmovem_index1[dmask])); srca += 2; dmask = xmovem_next[dmask]; }
	while (amask) { xput_word(srca, xm68k_areg(xmovem_index1[amask])); srca += 2; amask = xmovem_next[amask]; }
}}}xm68k_incpc(6);
}
void xop_48b9_0(uae_u32 opcode) /* MVMLE */
{
{	uae_u16 mask = xget_iword(2);
{	uaecptr srca = xget_ilong(4);
{	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) { xput_word(srca, xm68k_dreg(xmovem_index1[dmask])); srca += 2; dmask = xmovem_next[dmask]; }
	while (amask) { xput_word(srca, xm68k_areg(xmovem_index1[amask])); srca += 2; amask = xmovem_next[amask]; }
}}}xm68k_incpc(8);
}
void xop_48c0_0(uae_u32 opcode) /* EXT */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uae_u32 dst = (uae_s32)(uae_s16)src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(dst)) == 0);
	XSET_NFLG (((uae_s32)(dst)) < 0);
	xm68k_dreg(srcreg) = (dst);
}}}xm68k_incpc(2);
}
void xop_48d0_0(uae_u32 opcode) /* MVMLE */
{
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = xget_iword(2);
{	uaecptr srca = xm68k_areg(dstreg);
{	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) { xput_long(srca, xm68k_dreg(xmovem_index1[dmask])); srca += 4; dmask = xmovem_next[dmask]; }
	while (amask) { xput_long(srca, xm68k_areg(xmovem_index1[amask])); srca += 4; amask = xmovem_next[amask]; }
}}}xm68k_incpc(4);
}
void xop_48e0_0(uae_u32 opcode) /* MVMLE */
{
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = xget_iword(2);
{	uaecptr srca = xm68k_areg(dstreg) - 0;
{	uae_u16 amask = mask & 0xff, dmask = (mask >> 8) & 0xff;
	while (amask) { srca -= 4; xput_long(srca, xm68k_areg(xmovem_index2[amask])); amask = xmovem_next[amask]; }
	while (dmask) { srca -= 4; xput_long(srca, xm68k_dreg(xmovem_index2[dmask])); dmask = xmovem_next[dmask]; }
	xm68k_areg(dstreg) = srca;
}}}xm68k_incpc(4);
}
void xop_48e8_0(uae_u32 opcode) /* MVMLE */
{
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = xget_iword(2);
{	uaecptr srca = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(4);
{	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) { xput_long(srca, xm68k_dreg(xmovem_index1[dmask])); srca += 4; dmask = xmovem_next[dmask]; }
	while (amask) { xput_long(srca, xm68k_areg(xmovem_index1[amask])); srca += 4; amask = xmovem_next[amask]; }
}}}xm68k_incpc(6);
}
void xop_48f0_0(uae_u32 opcode) /* MVMLE */
{
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = xget_iword(2);
{xm68k_incpc(4);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) { xput_long(srca, xm68k_dreg(xmovem_index1[dmask])); srca += 4; dmask = xmovem_next[dmask]; }
	while (amask) { xput_long(srca, xm68k_areg(xmovem_index1[amask])); srca += 4; amask = xmovem_next[amask]; }
}}}}}
void xop_48f8_0(uae_u32 opcode) /* MVMLE */
{
{	uae_u16 mask = xget_iword(2);
{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(4);
{	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) { xput_long(srca, xm68k_dreg(xmovem_index1[dmask])); srca += 4; dmask = xmovem_next[dmask]; }
	while (amask) { xput_long(srca, xm68k_areg(xmovem_index1[amask])); srca += 4; amask = xmovem_next[amask]; }
}}}xm68k_incpc(6);
}
void xop_48f9_0(uae_u32 opcode) /* MVMLE */
{
{	uae_u16 mask = xget_iword(2);
{	uaecptr srca = xget_ilong(4);
{	uae_u16 dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
	while (dmask) { xput_long(srca, xm68k_dreg(xmovem_index1[dmask])); srca += 4; dmask = xmovem_next[dmask]; }
	while (amask) { xput_long(srca, xm68k_areg(xmovem_index1[amask])); srca += 4; amask = xmovem_next[amask]; }
}}}xm68k_incpc(8);
}
void xop_49c0_0(uae_u32 opcode) /* EXT */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uae_u32 dst = (uae_s32)(uae_s8)src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(dst)) == 0);
	XSET_NFLG (((uae_s32)(dst)) < 0);
	xm68k_dreg(srcreg) = (dst);
}}}xm68k_incpc(2);
}
void xop_4a00_0(uae_u32 opcode) /* TST */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s8 src = xm68k_dreg(srcreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
}}xm68k_incpc(2);
}
void xop_4a10_0(uae_u32 opcode) /* TST */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s8 src = xget_byte(srca);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
}}}xm68k_incpc(2);
}
void xop_4a18_0(uae_u32 opcode) /* TST */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s8 src = xget_byte(srca);
	xm68k_areg(srcreg) += xareg_byteinc[srcreg];
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
}}}xm68k_incpc(2);
}
void xop_4a20_0(uae_u32 opcode) /* TST */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) - xareg_byteinc[srcreg];
{	uae_s8 src = xget_byte(srca);
	xm68k_areg (srcreg) = srca;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
}}}xm68k_incpc(2);
}
void xop_4a28_0(uae_u32 opcode) /* TST */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
}}}xm68k_incpc(4);
}
void xop_4a30_0(uae_u32 opcode) /* TST */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s8 src = xget_byte(srca);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
}}}}}
void xop_4a38_0(uae_u32 opcode) /* TST */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
}}}xm68k_incpc(4);
}
void xop_4a39_0(uae_u32 opcode) /* TST */
{
{{	uaecptr srca = xget_ilong(2);
{	uae_s8 src = xget_byte(srca);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
}}}xm68k_incpc(6);
}
void xop_4a3a_0(uae_u32 opcode) /* TST */
{
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
}}}xm68k_incpc(4);
}
void xop_4a3b_0(uae_u32 opcode) /* TST */
{
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s8 src = xget_byte(srca);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
}}}}}
void xop_4a3c_0(uae_u32 opcode) /* TST */
{
{{	uae_s8 src = xget_ibyte(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
}}xm68k_incpc(4);
}
void xop_4a40_0(uae_u32 opcode) /* TST */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = xm68k_dreg(srcreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
}}xm68k_incpc(2);
}
void xop_4a48_0(uae_u32 opcode) /* TST */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = xm68k_areg(srcreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
}}xm68k_incpc(2);
}
void xop_4a50_0(uae_u32 opcode) /* TST */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
}}}xm68k_incpc(2);
}
void xop_4a58_0(uae_u32 opcode) /* TST */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
	xm68k_areg(srcreg) += 2;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
}}}xm68k_incpc(2);
}
void xop_4a60_0(uae_u32 opcode) /* TST */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) - 2;
{	uae_s16 src = xget_word(srca);
	xm68k_areg (srcreg) = srca;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
}}}xm68k_incpc(2);
}
void xop_4a68_0(uae_u32 opcode) /* TST */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
}}}xm68k_incpc(4);
}
void xop_4a70_0(uae_u32 opcode) /* TST */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s16 src = xget_word(srca);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
}}}}}
void xop_4a78_0(uae_u32 opcode) /* TST */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
}}}xm68k_incpc(4);
}
void xop_4a79_0(uae_u32 opcode) /* TST */
{
{{	uaecptr srca = xget_ilong(2);
{	uae_s16 src = xget_word(srca);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
}}}xm68k_incpc(6);
}
void xop_4a7a_0(uae_u32 opcode) /* TST */
{
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
}}}xm68k_incpc(4);
}
void xop_4a7b_0(uae_u32 opcode) /* TST */
{
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s16 src = xget_word(srca);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
}}}}}
void xop_4a7c_0(uae_u32 opcode) /* TST */
{
{{	uae_s16 src = xget_iword(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
}}xm68k_incpc(4);
}
void xop_4a80_0(uae_u32 opcode) /* TST */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = xm68k_dreg(srcreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
}}xm68k_incpc(2);
}
void xop_4a88_0(uae_u32 opcode) /* TST */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = xm68k_areg(srcreg);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
}}xm68k_incpc(2);
}
void xop_4a90_0(uae_u32 opcode) /* TST */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
}}}xm68k_incpc(2);
}
void xop_4a98_0(uae_u32 opcode) /* TST */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
	xm68k_areg(srcreg) += 4;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
}}}xm68k_incpc(2);
}
void xop_4aa0_0(uae_u32 opcode) /* TST */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) - 4;
{	uae_s32 src = xget_long(srca);
	xm68k_areg (srcreg) = srca;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
}}}xm68k_incpc(2);
}
void xop_4aa8_0(uae_u32 opcode) /* TST */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
}}}xm68k_incpc(4);
}
void xop_4ab0_0(uae_u32 opcode) /* TST */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s32 src = xget_long(srca);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
}}}}}
void xop_4ab8_0(uae_u32 opcode) /* TST */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
}}}xm68k_incpc(4);
}
void xop_4ab9_0(uae_u32 opcode) /* TST */
{
{{	uaecptr srca = xget_ilong(2);
{	uae_s32 src = xget_long(srca);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
}}}xm68k_incpc(6);
}
void xop_4aba_0(uae_u32 opcode) /* TST */
{
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
}}}xm68k_incpc(4);
}
void xop_4abb_0(uae_u32 opcode) /* TST */
{
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s32 src = xget_long(srca);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
}}}}}
void xop_4abc_0(uae_u32 opcode) /* TST */
{
{{	uae_s32 src = xget_ilong(2);
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
}}xm68k_incpc(6);
}
void xop_4ac0_0(uae_u32 opcode) /* TAS */
{
	uae_u32 srcreg = (opcode & 7);
{}}
void xop_4ad0_0(uae_u32 opcode) /* TAS */
{
	uae_u32 srcreg = (opcode & 7);
{}}
void xop_4ad8_0(uae_u32 opcode) /* TAS */
{
	uae_u32 srcreg = (opcode & 7);
{}}
void xop_4ae0_0(uae_u32 opcode) /* TAS */
{
	uae_u32 srcreg = (opcode & 7);
{}}
void xop_4ae8_0(uae_u32 opcode) /* TAS */
{
	uae_u32 srcreg = (opcode & 7);
{}}
void xop_4af0_0(uae_u32 opcode) /* TAS */
{
	uae_u32 srcreg = (opcode & 7);
{}}
void xop_4af8_0(uae_u32 opcode) /* TAS */
{
{}}
void xop_4af9_0(uae_u32 opcode) /* TAS */
{
{}}
void xop_4c00_0(uae_u32 opcode) /* MULL */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_4c10_0(uae_u32 opcode) /* MULL */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_4c18_0(uae_u32 opcode) /* MULL */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_4c20_0(uae_u32 opcode) /* MULL */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_4c28_0(uae_u32 opcode) /* MULL */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_4c30_0(uae_u32 opcode) /* MULL */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_4c38_0(uae_u32 opcode) /* MULL */
{
{}}
void xop_4c39_0(uae_u32 opcode) /* MULL */
{
{}}
void xop_4c3a_0(uae_u32 opcode) /* MULL */
{
	uae_u32 dstreg = 2;
{}}
void xop_4c3b_0(uae_u32 opcode) /* MULL */
{
	uae_u32 dstreg = 3;
{}}
void xop_4c3c_0(uae_u32 opcode) /* MULL */
{
{}}
void xop_4c40_0(uae_u32 opcode) /* DIVL */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_4c50_0(uae_u32 opcode) /* DIVL */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_4c58_0(uae_u32 opcode) /* DIVL */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_4c60_0(uae_u32 opcode) /* DIVL */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_4c68_0(uae_u32 opcode) /* DIVL */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_4c70_0(uae_u32 opcode) /* DIVL */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_4c78_0(uae_u32 opcode) /* DIVL */
{
{}}
void xop_4c79_0(uae_u32 opcode) /* DIVL */
{
{}}
void xop_4c7a_0(uae_u32 opcode) /* DIVL */
{
	uae_u32 dstreg = 2;
{}}
void xop_4c7b_0(uae_u32 opcode) /* DIVL */
{
	uae_u32 dstreg = 3;
{}}
void xop_4c7c_0(uae_u32 opcode) /* DIVL */
{
{}}
void xop_4c90_0(uae_u32 opcode) /* MVMEL */
{
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = xget_iword(2);
	unsigned int dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca = xm68k_areg(dstreg);
{	while (dmask) { xm68k_dreg(xmovem_index1[dmask]) = (uae_s32)(uae_s16)xget_word(srca); srca += 2; dmask = xmovem_next[dmask]; }
	while (amask) { xm68k_areg(xmovem_index1[amask]) = (uae_s32)(uae_s16)xget_word(srca); srca += 2; amask = xmovem_next[amask]; }
}}}xm68k_incpc(4);
}
void xop_4c98_0(uae_u32 opcode) /* MVMEL */
{
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = xget_iword(2);
	unsigned int dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca = xm68k_areg(dstreg);
{	while (dmask) { xm68k_dreg(xmovem_index1[dmask]) = (uae_s32)(uae_s16)xget_word(srca); srca += 2; dmask = xmovem_next[dmask]; }
	while (amask) { xm68k_areg(xmovem_index1[amask]) = (uae_s32)(uae_s16)xget_word(srca); srca += 2; amask = xmovem_next[amask]; }
	xm68k_areg(dstreg) = srca;
}}}xm68k_incpc(4);
}
void xop_4ca8_0(uae_u32 opcode) /* MVMEL */
{
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = xget_iword(2);
	unsigned int dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(4);
{	while (dmask) { xm68k_dreg(xmovem_index1[dmask]) = (uae_s32)(uae_s16)xget_word(srca); srca += 2; dmask = xmovem_next[dmask]; }
	while (amask) { xm68k_areg(xmovem_index1[amask]) = (uae_s32)(uae_s16)xget_word(srca); srca += 2; amask = xmovem_next[amask]; }
}}}xm68k_incpc(6);
}
void xop_4cb0_0(uae_u32 opcode) /* MVMEL */
{
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = xget_iword(2);
	unsigned int dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{xm68k_incpc(4);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	while (dmask) { xm68k_dreg(xmovem_index1[dmask]) = (uae_s32)(uae_s16)xget_word(srca); srca += 2; dmask = xmovem_next[dmask]; }
	while (amask) { xm68k_areg(xmovem_index1[amask]) = (uae_s32)(uae_s16)xget_word(srca); srca += 2; amask = xmovem_next[amask]; }
}}}}}
void xop_4cb8_0(uae_u32 opcode) /* MVMEL */
{
{	uae_u16 mask = xget_iword(2);
	unsigned int dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(4);
{	while (dmask) { xm68k_dreg(xmovem_index1[dmask]) = (uae_s32)(uae_s16)xget_word(srca); srca += 2; dmask = xmovem_next[dmask]; }
	while (amask) { xm68k_areg(xmovem_index1[amask]) = (uae_s32)(uae_s16)xget_word(srca); srca += 2; amask = xmovem_next[amask]; }
}}}xm68k_incpc(6);
}
void xop_4cb9_0(uae_u32 opcode) /* MVMEL */
{
{	uae_u16 mask = xget_iword(2);
	unsigned int dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca = xget_ilong(4);
{	while (dmask) { xm68k_dreg(xmovem_index1[dmask]) = (uae_s32)(uae_s16)xget_word(srca); srca += 2; dmask = xmovem_next[dmask]; }
	while (amask) { xm68k_areg(xmovem_index1[amask]) = (uae_s32)(uae_s16)xget_word(srca); srca += 2; amask = xmovem_next[amask]; }
}}}xm68k_incpc(8);
}
void xop_4cba_0(uae_u32 opcode) /* MVMEL */
{
	uae_u32 dstreg = 2;
{	uae_u16 mask = xget_iword(2);
	unsigned int dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca = xm68k_getpc () + 4;
	srca += (uae_s32)(uae_s16)xget_iword(4);
{	while (dmask) { xm68k_dreg(xmovem_index1[dmask]) = (uae_s32)(uae_s16)xget_word(srca); srca += 2; dmask = xmovem_next[dmask]; }
	while (amask) { xm68k_areg(xmovem_index1[amask]) = (uae_s32)(uae_s16)xget_word(srca); srca += 2; amask = xmovem_next[amask]; }
}}}xm68k_incpc(6);
}
void xop_4cbb_0(uae_u32 opcode) /* MVMEL */
{
	uae_u32 dstreg = 3;
{	uae_u16 mask = xget_iword(2);
	unsigned int dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{xm68k_incpc(4);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	while (dmask) { xm68k_dreg(xmovem_index1[dmask]) = (uae_s32)(uae_s16)xget_word(srca); srca += 2; dmask = xmovem_next[dmask]; }
	while (amask) { xm68k_areg(xmovem_index1[amask]) = (uae_s32)(uae_s16)xget_word(srca); srca += 2; amask = xmovem_next[amask]; }
}}}}}
void xop_4cd0_0(uae_u32 opcode) /* MVMEL */
{
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = xget_iword(2);
	unsigned int dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca = xm68k_areg(dstreg);
{	while (dmask) { xm68k_dreg(xmovem_index1[dmask]) = xget_long(srca); srca += 4; dmask = xmovem_next[dmask]; }
	while (amask) { xm68k_areg(xmovem_index1[amask]) = xget_long(srca); srca += 4; amask = xmovem_next[amask]; }
}}}xm68k_incpc(4);
}
void xop_4cd8_0(uae_u32 opcode) /* MVMEL */
{
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = xget_iword(2);
	unsigned int dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca = xm68k_areg(dstreg);
{	while (dmask) { xm68k_dreg(xmovem_index1[dmask]) = xget_long(srca); srca += 4; dmask = xmovem_next[dmask]; }
	while (amask) { xm68k_areg(xmovem_index1[amask]) = xget_long(srca); srca += 4; amask = xmovem_next[amask]; }
	xm68k_areg(dstreg) = srca;
}}}xm68k_incpc(4);
}
void xop_4ce8_0(uae_u32 opcode) /* MVMEL */
{
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = xget_iword(2);
	unsigned int dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(4);
{	while (dmask) { xm68k_dreg(xmovem_index1[dmask]) = xget_long(srca); srca += 4; dmask = xmovem_next[dmask]; }
	while (amask) { xm68k_areg(xmovem_index1[amask]) = xget_long(srca); srca += 4; amask = xmovem_next[amask]; }
}}}xm68k_incpc(6);
}
void xop_4cf0_0(uae_u32 opcode) /* MVMEL */
{
	uae_u32 dstreg = opcode & 7;
{	uae_u16 mask = xget_iword(2);
	unsigned int dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{xm68k_incpc(4);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	while (dmask) { xm68k_dreg(xmovem_index1[dmask]) = xget_long(srca); srca += 4; dmask = xmovem_next[dmask]; }
	while (amask) { xm68k_areg(xmovem_index1[amask]) = xget_long(srca); srca += 4; amask = xmovem_next[amask]; }
}}}}}
void xop_4cf8_0(uae_u32 opcode) /* MVMEL */
{
{	uae_u16 mask = xget_iword(2);
	unsigned int dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(4);
{	while (dmask) { xm68k_dreg(xmovem_index1[dmask]) = xget_long(srca); srca += 4; dmask = xmovem_next[dmask]; }
	while (amask) { xm68k_areg(xmovem_index1[amask]) = xget_long(srca); srca += 4; amask = xmovem_next[amask]; }
}}}xm68k_incpc(6);
}
void xop_4cf9_0(uae_u32 opcode) /* MVMEL */
{
{	uae_u16 mask = xget_iword(2);
	unsigned int dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca = xget_ilong(4);
{	while (dmask) { xm68k_dreg(xmovem_index1[dmask]) = xget_long(srca); srca += 4; dmask = xmovem_next[dmask]; }
	while (amask) { xm68k_areg(xmovem_index1[amask]) = xget_long(srca); srca += 4; amask = xmovem_next[amask]; }
}}}xm68k_incpc(8);
}
void xop_4cfa_0(uae_u32 opcode) /* MVMEL */
{
	uae_u32 dstreg = 2;
{	uae_u16 mask = xget_iword(2);
	unsigned int dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{	uaecptr srca = xm68k_getpc () + 4;
	srca += (uae_s32)(uae_s16)xget_iword(4);
{	while (dmask) { xm68k_dreg(xmovem_index1[dmask]) = xget_long(srca); srca += 4; dmask = xmovem_next[dmask]; }
	while (amask) { xm68k_areg(xmovem_index1[amask]) = xget_long(srca); srca += 4; amask = xmovem_next[amask]; }
}}}xm68k_incpc(6);
}
void xop_4cfb_0(uae_u32 opcode) /* MVMEL */
{
	uae_u32 dstreg = 3;
{	uae_u16 mask = xget_iword(2);
	unsigned int dmask = mask & 0xff, amask = (mask >> 8) & 0xff;
{xm68k_incpc(4);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	while (dmask) { xm68k_dreg(xmovem_index1[dmask]) = xget_long(srca); srca += 4; dmask = xmovem_next[dmask]; }
	while (amask) { xm68k_areg(xmovem_index1[amask]) = xget_long(srca); srca += 4; amask = xmovem_next[amask]; }
}}}}}
void xop_4e40_0(uae_u32 opcode) /* TRAP */
{
	uae_u32 srcreg = (opcode & 15);
{}}
void xop_4e50_0(uae_u32 opcode) /* LINK */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr olda = xm68k_areg(7) - 4;
	xm68k_areg (7) = olda;
{	uae_s32 src = xm68k_areg(srcreg);
	xput_long(olda,src);
	xm68k_areg(srcreg) = (xm68k_areg(7));
{	uae_s16 offs = xget_iword(2);
	xm68k_areg(7) += offs;
}}}}xm68k_incpc(4);
}
void xop_4e58_0(uae_u32 opcode) /* UNLK */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s32 src = xm68k_areg(srcreg);
	xm68k_areg(7) = src;
{	uaecptr olda = xm68k_areg(7);
{	uae_s32 old = xget_long(olda);
	xm68k_areg(7) += 4;
	xm68k_areg(srcreg) = (old);
}}}}xm68k_incpc(2);
}
void xop_4e60_0(uae_u32 opcode) /* MVR2USP */
{
	uae_u32 srcreg = (opcode & 7);
}
void xop_4e68_0(uae_u32 opcode) /* MVUSP2R */
{
	uae_u32 srcreg = (opcode & 7);
}
void xop_4e70_0(uae_u32 opcode) /* RESET */
{
}
void xop_4e71_0(uae_u32 opcode) /* NOP */
{
{}xm68k_incpc(2);
}
void xop_4e72_0(uae_u32 opcode) /* STOP */
{
}
void xop_4e73_0(uae_u32 opcode) /* RTE */
{
}
void xop_4e74_0(uae_u32 opcode) /* RTD */
{
{}}
void xop_4e75_0(uae_u32 opcode) /* RTS */
{
{	xm68k_setpc(xget_long(xm68k_areg(7)));
	xm68k_areg(7) += 4;
}}
void xop_4e76_0(uae_u32 opcode) /* TRAPV */
{
{}}
void xop_4e77_0(uae_u32 opcode) /* RTR */
{
{}}
void xop_4e7a_0(uae_u32 opcode) /* MOVEC2 */
{
}
void xop_4e7b_0(uae_u32 opcode) /* MOVE2C */
{
}
void xop_4e90_0(uae_u32 opcode) /* JSR */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
	xm68k_areg(7) -= 4;
	xput_long(xm68k_areg(7), xm68k_getpc() + 2);
	xm68k_setpc(srca);
}}}
void xop_4ea8_0(uae_u32 opcode) /* JSR */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
	xm68k_areg(7) -= 4;
	xput_long(xm68k_areg(7), xm68k_getpc() + 4);
	xm68k_setpc(srca);
}}}
void xop_4eb0_0(uae_u32 opcode) /* JSR */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
	xm68k_areg(7) -= 4;
	xput_long(xm68k_areg(7), xm68k_getpc() + 0);
	xm68k_setpc(srca);
}}}}
void xop_4eb8_0(uae_u32 opcode) /* JSR */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
	xm68k_areg(7) -= 4;
	xput_long(xm68k_areg(7), xm68k_getpc() + 4);
	xm68k_setpc(srca);
}}}
void xop_4eb9_0(uae_u32 opcode) /* JSR */
{
{{	uaecptr srca = xget_ilong(2);
	xm68k_areg(7) -= 4;
	xput_long(xm68k_areg(7), xm68k_getpc() + 6);
	xm68k_setpc(srca);
}}}
void xop_4eba_0(uae_u32 opcode) /* JSR */
{
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
	xm68k_areg(7) -= 4;
	xput_long(xm68k_areg(7), xm68k_getpc() + 4);
	xm68k_setpc(srca);
}}}
void xop_4ebb_0(uae_u32 opcode) /* JSR */
{
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
	xm68k_areg(7) -= 4;
	xput_long(xm68k_areg(7), xm68k_getpc() + 0);
	xm68k_setpc(srca);
}}}}
void xop_4ed0_0(uae_u32 opcode) /* JMP */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
	xm68k_setpc(srca);
}}}
void xop_4ee8_0(uae_u32 opcode) /* JMP */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
	xm68k_setpc(srca);
}}}
void xop_4ef0_0(uae_u32 opcode) /* JMP */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
	xm68k_setpc(srca);
}}}}
void xop_4ef8_0(uae_u32 opcode) /* JMP */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
	xm68k_setpc(srca);
}}}
void xop_4ef9_0(uae_u32 opcode) /* JMP */
{
{{	uaecptr srca = xget_ilong(2);
	xm68k_setpc(srca);
}}}
void xop_4efa_0(uae_u32 opcode) /* JMP */
{
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
	xm68k_setpc(srca);
}}}
void xop_4efb_0(uae_u32 opcode) /* JMP */
{
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
	xm68k_setpc(srca);
}}}}
void xop_5000_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uae_s8 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}xm68k_incpc(2);
}
void xop_5010_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_5018_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg(dstreg) += xareg_byteinc[dstreg];
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_5020_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = xm68k_areg(dstreg) - xareg_byteinc[dstreg];
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg (dstreg) = dsta;
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_5028_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 dst = xget_byte(dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}xm68k_incpc(4);
}
void xop_5030_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{xm68k_incpc(2);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s8 dst = xget_byte(dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}}}
void xop_5038_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 dst = xget_byte(dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}xm68k_incpc(4);
}
void xop_5039_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta = xget_ilong(2);
{	uae_s8 dst = xget_byte(dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}xm68k_incpc(6);
}
void xop_5040_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}xm68k_incpc(2);
}
void xop_5048_0(uae_u32 opcode) /* ADDA */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst + src;
	xm68k_areg(dstreg) = (newv);
}}}}xm68k_incpc(2);
}
void xop_5050_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s16 dst = xget_word(dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_5058_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s16 dst = xget_word(dsta);
	xm68k_areg(dstreg) += 2;
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_5060_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = xm68k_areg(dstreg) - 2;
{	uae_s16 dst = xget_word(dsta);
	xm68k_areg (dstreg) = dsta;
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_5068_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 dst = xget_word(dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}xm68k_incpc(4);
}
void xop_5070_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{xm68k_incpc(2);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s16 dst = xget_word(dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}}}
void xop_5078_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 dst = xget_word(dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}xm68k_incpc(4);
}
void xop_5079_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta = xget_ilong(2);
{	uae_s16 dst = xget_word(dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}xm68k_incpc(6);
}
void xop_5080_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}}xm68k_incpc(2);
}
void xop_5088_0(uae_u32 opcode) /* ADDA */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst + src;
	xm68k_areg(dstreg) = (newv);
}}}}xm68k_incpc(2);
}
void xop_5090_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s32 dst = xget_long(dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_5098_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s32 dst = xget_long(dsta);
	xm68k_areg(dstreg) += 4;
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_50a0_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = xm68k_areg(dstreg) - 4;
{	uae_s32 dst = xget_long(dsta);
	xm68k_areg (dstreg) = dsta;
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_50a8_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 dst = xget_long(dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}xm68k_incpc(4);
}
void xop_50b0_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{xm68k_incpc(2);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s32 dst = xget_long(dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}}}
void xop_50b8_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 dst = xget_long(dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}xm68k_incpc(4);
}
void xop_50b9_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta = xget_ilong(2);
{	uae_s32 dst = xget_long(dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}xm68k_incpc(6);
}
void xop_50c0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{{	int val = xcctrue(0) ? 0xff : 0;
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xff) | ((val) & 0xff);
}}}xm68k_incpc(2);
}
void xop_50c8_0(uae_u32 opcode) /* DBcc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uae_s16 offs = xget_iword(2);
	if (!xcctrue(0)) {
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xffff) | (((src-1)) & 0xffff);
		if (src) {
			xm68k_incpc((uae_s32)offs + 2);
		}
	}
}}}xm68k_incpc(4);
endlabel954: ;
}
void xop_50d0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	int val = xcctrue(0) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_50d8_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
	xm68k_areg(srcreg) += xareg_byteinc[srcreg];
{	int val = xcctrue(0) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_50e0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) - xareg_byteinc[srcreg];
	xm68k_areg (srcreg) = srca;
{	int val = xcctrue(0) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_50e8_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	int val = xcctrue(0) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(4);
}
void xop_50f0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	int val = xcctrue(0) ? 0xff : 0;
	xput_byte(srca,val);
}}}}}
void xop_50f8_0(uae_u32 opcode) /* Scc */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	int val = xcctrue(0) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(4);
}
void xop_50f9_0(uae_u32 opcode) /* Scc */
{
{{	uaecptr srca = xget_ilong(2);
{	int val = xcctrue(0) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(6);
}
void xop_50fa_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_50fb_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_50fc_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_5100_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uae_s8 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}xm68k_incpc(2);
}
void xop_5110_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_5118_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg(dstreg) += xareg_byteinc[dstreg];
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_5120_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = xm68k_areg(dstreg) - xareg_byteinc[dstreg];
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg (dstreg) = dsta;
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_5128_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 dst = xget_byte(dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}xm68k_incpc(4);
}
void xop_5130_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{xm68k_incpc(2);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s8 dst = xget_byte(dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}}}
void xop_5138_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 dst = xget_byte(dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}xm68k_incpc(4);
}
void xop_5139_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta = xget_ilong(2);
{	uae_s8 dst = xget_byte(dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}xm68k_incpc(6);
}
void xop_5140_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}xm68k_incpc(2);
}
void xop_5148_0(uae_u32 opcode) /* SUBA */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst - src;
	xm68k_areg(dstreg) = (newv);
}}}}xm68k_incpc(2);
}
void xop_5150_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s16 dst = xget_word(dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_5158_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s16 dst = xget_word(dsta);
	xm68k_areg(dstreg) += 2;
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_5160_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = xm68k_areg(dstreg) - 2;
{	uae_s16 dst = xget_word(dsta);
	xm68k_areg (dstreg) = dsta;
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_5168_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 dst = xget_word(dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}xm68k_incpc(4);
}
void xop_5170_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{xm68k_incpc(2);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s16 dst = xget_word(dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}}}
void xop_5178_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 dst = xget_word(dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}xm68k_incpc(4);
}
void xop_5179_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta = xget_ilong(2);
{	uae_s16 dst = xget_word(dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}xm68k_incpc(6);
}
void xop_5180_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}}xm68k_incpc(2);
}
void xop_5188_0(uae_u32 opcode) /* SUBA */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst - src;
	xm68k_areg(dstreg) = (newv);
}}}}xm68k_incpc(2);
}
void xop_5190_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s32 dst = xget_long(dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_5198_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s32 dst = xget_long(dsta);
	xm68k_areg(dstreg) += 4;
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_51a0_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = xm68k_areg(dstreg) - 4;
{	uae_s32 dst = xget_long(dsta);
	xm68k_areg (dstreg) = dsta;
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_51a8_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 dst = xget_long(dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}xm68k_incpc(4);
}
void xop_51b0_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 src = srcreg;
{xm68k_incpc(2);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s32 dst = xget_long(dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}}}
void xop_51b8_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 dst = xget_long(dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}xm68k_incpc(4);
}
void xop_51b9_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
{{	uae_u32 src = srcreg;
{	uaecptr dsta = xget_ilong(2);
{	uae_s32 dst = xget_long(dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}xm68k_incpc(6);
}
void xop_51c0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{{	int val = xcctrue(1) ? 0xff : 0;
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xff) | ((val) & 0xff);
}}}xm68k_incpc(2);
}
void xop_51c8_0(uae_u32 opcode) /* DBcc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uae_s16 offs = xget_iword(2);
	if (!xcctrue(1)) {
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xffff) | (((src-1)) & 0xffff);
		if (src) {
			xm68k_incpc((uae_s32)offs + 2);
		}
	}
}}}xm68k_incpc(4);
endlabel992: ;
}
void xop_51d0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	int val = xcctrue(1) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_51d8_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
	xm68k_areg(srcreg) += xareg_byteinc[srcreg];
{	int val = xcctrue(1) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_51e0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) - xareg_byteinc[srcreg];
	xm68k_areg (srcreg) = srca;
{	int val = xcctrue(1) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_51e8_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	int val = xcctrue(1) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(4);
}
void xop_51f0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	int val = xcctrue(1) ? 0xff : 0;
	xput_byte(srca,val);
}}}}}
void xop_51f8_0(uae_u32 opcode) /* Scc */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	int val = xcctrue(1) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(4);
}
void xop_51f9_0(uae_u32 opcode) /* Scc */
{
{{	uaecptr srca = xget_ilong(2);
{	int val = xcctrue(1) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(6);
}
void xop_51fa_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_51fb_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_51fc_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_52c0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{{	int val = xcctrue(2) ? 0xff : 0;
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xff) | ((val) & 0xff);
}}}xm68k_incpc(2);
}
void xop_52c8_0(uae_u32 opcode) /* DBcc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uae_s16 offs = xget_iword(2);
	if (!xcctrue(2)) {
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xffff) | (((src-1)) & 0xffff);
		if (src) {
			xm68k_incpc((uae_s32)offs + 2);
		}
	}
}}}xm68k_incpc(4);
endlabel1004: ;
}
void xop_52d0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	int val = xcctrue(2) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_52d8_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
	xm68k_areg(srcreg) += xareg_byteinc[srcreg];
{	int val = xcctrue(2) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_52e0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) - xareg_byteinc[srcreg];
	xm68k_areg (srcreg) = srca;
{	int val = xcctrue(2) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_52e8_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	int val = xcctrue(2) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(4);
}
void xop_52f0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	int val = xcctrue(2) ? 0xff : 0;
	xput_byte(srca,val);
}}}}}
void xop_52f8_0(uae_u32 opcode) /* Scc */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	int val = xcctrue(2) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(4);
}
void xop_52f9_0(uae_u32 opcode) /* Scc */
{
{{	uaecptr srca = xget_ilong(2);
{	int val = xcctrue(2) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(6);
}
void xop_52fa_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_52fb_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_52fc_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_53c0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{{	int val = xcctrue(3) ? 0xff : 0;
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xff) | ((val) & 0xff);
}}}xm68k_incpc(2);
}
void xop_53c8_0(uae_u32 opcode) /* DBcc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uae_s16 offs = xget_iword(2);
	if (!xcctrue(3)) {
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xffff) | (((src-1)) & 0xffff);
		if (src) {
			xm68k_incpc((uae_s32)offs + 2);
		}
	}
}}}xm68k_incpc(4);
endlabel1016: ;
}
void xop_53d0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	int val = xcctrue(3) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_53d8_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
	xm68k_areg(srcreg) += xareg_byteinc[srcreg];
{	int val = xcctrue(3) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_53e0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) - xareg_byteinc[srcreg];
	xm68k_areg (srcreg) = srca;
{	int val = xcctrue(3) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_53e8_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	int val = xcctrue(3) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(4);
}
void xop_53f0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	int val = xcctrue(3) ? 0xff : 0;
	xput_byte(srca,val);
}}}}}
void xop_53f8_0(uae_u32 opcode) /* Scc */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	int val = xcctrue(3) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(4);
}
void xop_53f9_0(uae_u32 opcode) /* Scc */
{
{{	uaecptr srca = xget_ilong(2);
{	int val = xcctrue(3) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(6);
}
void xop_53fa_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_53fb_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_53fc_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_54c0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{{	int val = xcctrue(4) ? 0xff : 0;
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xff) | ((val) & 0xff);
}}}xm68k_incpc(2);
}
void xop_54c8_0(uae_u32 opcode) /* DBcc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uae_s16 offs = xget_iword(2);
	if (!xcctrue(4)) {
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xffff) | (((src-1)) & 0xffff);
		if (src) {
			xm68k_incpc((uae_s32)offs + 2);
		}
	}
}}}xm68k_incpc(4);
endlabel1028: ;
}
void xop_54d0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	int val = xcctrue(4) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_54d8_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
	xm68k_areg(srcreg) += xareg_byteinc[srcreg];
{	int val = xcctrue(4) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_54e0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) - xareg_byteinc[srcreg];
	xm68k_areg (srcreg) = srca;
{	int val = xcctrue(4) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_54e8_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	int val = xcctrue(4) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(4);
}
void xop_54f0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	int val = xcctrue(4) ? 0xff : 0;
	xput_byte(srca,val);
}}}}}
void xop_54f8_0(uae_u32 opcode) /* Scc */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	int val = xcctrue(4) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(4);
}
void xop_54f9_0(uae_u32 opcode) /* Scc */
{
{{	uaecptr srca = xget_ilong(2);
{	int val = xcctrue(4) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(6);
}
void xop_54fa_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_54fb_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_54fc_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_55c0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{{	int val = xcctrue(5) ? 0xff : 0;
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xff) | ((val) & 0xff);
}}}xm68k_incpc(2);
}
void xop_55c8_0(uae_u32 opcode) /* DBcc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uae_s16 offs = xget_iword(2);
	if (!xcctrue(5)) {
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xffff) | (((src-1)) & 0xffff);
		if (src) {
			xm68k_incpc((uae_s32)offs + 2);
		}
	}
}}}xm68k_incpc(4);
endlabel1040: ;
}
void xop_55d0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	int val = xcctrue(5) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_55d8_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
	xm68k_areg(srcreg) += xareg_byteinc[srcreg];
{	int val = xcctrue(5) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_55e0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) - xareg_byteinc[srcreg];
	xm68k_areg (srcreg) = srca;
{	int val = xcctrue(5) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_55e8_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	int val = xcctrue(5) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(4);
}
void xop_55f0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	int val = xcctrue(5) ? 0xff : 0;
	xput_byte(srca,val);
}}}}}
void xop_55f8_0(uae_u32 opcode) /* Scc */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	int val = xcctrue(5) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(4);
}
void xop_55f9_0(uae_u32 opcode) /* Scc */
{
{{	uaecptr srca = xget_ilong(2);
{	int val = xcctrue(5) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(6);
}
void xop_55fa_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_55fb_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_55fc_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_56c0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{{	int val = xcctrue(6) ? 0xff : 0;
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xff) | ((val) & 0xff);
}}}xm68k_incpc(2);
}
void xop_56c8_0(uae_u32 opcode) /* DBcc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uae_s16 offs = xget_iword(2);
	if (!xcctrue(6)) {
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xffff) | (((src-1)) & 0xffff);
		if (src) {
			xm68k_incpc((uae_s32)offs + 2);
		}
	}
}}}xm68k_incpc(4);
endlabel1052: ;
}
void xop_56d0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	int val = xcctrue(6) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_56d8_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
	xm68k_areg(srcreg) += xareg_byteinc[srcreg];
{	int val = xcctrue(6) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_56e0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) - xareg_byteinc[srcreg];
	xm68k_areg (srcreg) = srca;
{	int val = xcctrue(6) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_56e8_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	int val = xcctrue(6) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(4);
}
void xop_56f0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	int val = xcctrue(6) ? 0xff : 0;
	xput_byte(srca,val);
}}}}}
void xop_56f8_0(uae_u32 opcode) /* Scc */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	int val = xcctrue(6) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(4);
}
void xop_56f9_0(uae_u32 opcode) /* Scc */
{
{{	uaecptr srca = xget_ilong(2);
{	int val = xcctrue(6) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(6);
}
void xop_56fa_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_56fb_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_56fc_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_57c0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{{	int val = xcctrue(7) ? 0xff : 0;
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xff) | ((val) & 0xff);
}}}xm68k_incpc(2);
}
void xop_57c8_0(uae_u32 opcode) /* DBcc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uae_s16 offs = xget_iword(2);
	if (!xcctrue(7)) {
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xffff) | (((src-1)) & 0xffff);
		if (src) {
			xm68k_incpc((uae_s32)offs + 2);
		}
	}
}}}xm68k_incpc(4);
endlabel1064: ;
}
void xop_57d0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	int val = xcctrue(7) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_57d8_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
	xm68k_areg(srcreg) += xareg_byteinc[srcreg];
{	int val = xcctrue(7) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_57e0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) - xareg_byteinc[srcreg];
	xm68k_areg (srcreg) = srca;
{	int val = xcctrue(7) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_57e8_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	int val = xcctrue(7) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(4);
}
void xop_57f0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	int val = xcctrue(7) ? 0xff : 0;
	xput_byte(srca,val);
}}}}}
void xop_57f8_0(uae_u32 opcode) /* Scc */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	int val = xcctrue(7) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(4);
}
void xop_57f9_0(uae_u32 opcode) /* Scc */
{
{{	uaecptr srca = xget_ilong(2);
{	int val = xcctrue(7) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(6);
}
void xop_57fa_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_57fb_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_57fc_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_58c0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{{	int val = xcctrue(8) ? 0xff : 0;
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xff) | ((val) & 0xff);
}}}xm68k_incpc(2);
}
void xop_58c8_0(uae_u32 opcode) /* DBcc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uae_s16 offs = xget_iword(2);
	if (!xcctrue(8)) {
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xffff) | (((src-1)) & 0xffff);
		if (src) {
			xm68k_incpc((uae_s32)offs + 2);
		}
	}
}}}xm68k_incpc(4);
endlabel1076: ;
}
void xop_58d0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	int val = xcctrue(8) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_58d8_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
	xm68k_areg(srcreg) += xareg_byteinc[srcreg];
{	int val = xcctrue(8) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_58e0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) - xareg_byteinc[srcreg];
	xm68k_areg (srcreg) = srca;
{	int val = xcctrue(8) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_58e8_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	int val = xcctrue(8) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(4);
}
void xop_58f0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	int val = xcctrue(8) ? 0xff : 0;
	xput_byte(srca,val);
}}}}}
void xop_58f8_0(uae_u32 opcode) /* Scc */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	int val = xcctrue(8) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(4);
}
void xop_58f9_0(uae_u32 opcode) /* Scc */
{
{{	uaecptr srca = xget_ilong(2);
{	int val = xcctrue(8) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(6);
}
void xop_58fa_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_58fb_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_58fc_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_59c0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{{	int val = xcctrue(9) ? 0xff : 0;
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xff) | ((val) & 0xff);
}}}xm68k_incpc(2);
}
void xop_59c8_0(uae_u32 opcode) /* DBcc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uae_s16 offs = xget_iword(2);
	if (!xcctrue(9)) {
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xffff) | (((src-1)) & 0xffff);
		if (src) {
			xm68k_incpc((uae_s32)offs + 2);
		}
	}
}}}xm68k_incpc(4);
endlabel1088: ;
}
void xop_59d0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	int val = xcctrue(9) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_59d8_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
	xm68k_areg(srcreg) += xareg_byteinc[srcreg];
{	int val = xcctrue(9) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_59e0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) - xareg_byteinc[srcreg];
	xm68k_areg (srcreg) = srca;
{	int val = xcctrue(9) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_59e8_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	int val = xcctrue(9) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(4);
}
void xop_59f0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	int val = xcctrue(9) ? 0xff : 0;
	xput_byte(srca,val);
}}}}}
void xop_59f8_0(uae_u32 opcode) /* Scc */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	int val = xcctrue(9) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(4);
}
void xop_59f9_0(uae_u32 opcode) /* Scc */
{
{{	uaecptr srca = xget_ilong(2);
{	int val = xcctrue(9) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(6);
}
void xop_59fa_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_59fb_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_59fc_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_5ac0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{{	int val = xcctrue(10) ? 0xff : 0;
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xff) | ((val) & 0xff);
}}}xm68k_incpc(2);
}
void xop_5ac8_0(uae_u32 opcode) /* DBcc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uae_s16 offs = xget_iword(2);
	if (!xcctrue(10)) {
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xffff) | (((src-1)) & 0xffff);
		if (src) {
			xm68k_incpc((uae_s32)offs + 2);
		}
	}
}}}xm68k_incpc(4);
endlabel1100: ;
}
void xop_5ad0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	int val = xcctrue(10) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_5ad8_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
	xm68k_areg(srcreg) += xareg_byteinc[srcreg];
{	int val = xcctrue(10) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_5ae0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) - xareg_byteinc[srcreg];
	xm68k_areg (srcreg) = srca;
{	int val = xcctrue(10) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_5ae8_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	int val = xcctrue(10) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(4);
}
void xop_5af0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	int val = xcctrue(10) ? 0xff : 0;
	xput_byte(srca,val);
}}}}}
void xop_5af8_0(uae_u32 opcode) /* Scc */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	int val = xcctrue(10) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(4);
}
void xop_5af9_0(uae_u32 opcode) /* Scc */
{
{{	uaecptr srca = xget_ilong(2);
{	int val = xcctrue(10) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(6);
}
void xop_5afa_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_5afb_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_5afc_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_5bc0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{{	int val = xcctrue(11) ? 0xff : 0;
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xff) | ((val) & 0xff);
}}}xm68k_incpc(2);
}
void xop_5bc8_0(uae_u32 opcode) /* DBcc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uae_s16 offs = xget_iword(2);
	if (!xcctrue(11)) {
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xffff) | (((src-1)) & 0xffff);
		if (src) {
			xm68k_incpc((uae_s32)offs + 2);
		}
	}
}}}xm68k_incpc(4);
endlabel1112: ;
}
void xop_5bd0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	int val = xcctrue(11) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_5bd8_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
	xm68k_areg(srcreg) += xareg_byteinc[srcreg];
{	int val = xcctrue(11) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_5be0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) - xareg_byteinc[srcreg];
	xm68k_areg (srcreg) = srca;
{	int val = xcctrue(11) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_5be8_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	int val = xcctrue(11) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(4);
}
void xop_5bf0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	int val = xcctrue(11) ? 0xff : 0;
	xput_byte(srca,val);
}}}}}
void xop_5bf8_0(uae_u32 opcode) /* Scc */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	int val = xcctrue(11) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(4);
}
void xop_5bf9_0(uae_u32 opcode) /* Scc */
{
{{	uaecptr srca = xget_ilong(2);
{	int val = xcctrue(11) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(6);
}
void xop_5bfa_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_5bfb_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_5bfc_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_5cc0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{{	int val = xcctrue(12) ? 0xff : 0;
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xff) | ((val) & 0xff);
}}}xm68k_incpc(2);
}
void xop_5cc8_0(uae_u32 opcode) /* DBcc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uae_s16 offs = xget_iword(2);
	if (!xcctrue(12)) {
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xffff) | (((src-1)) & 0xffff);
		if (src) {
			xm68k_incpc((uae_s32)offs + 2);
		}
	}
}}}xm68k_incpc(4);
endlabel1124: ;
}
void xop_5cd0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	int val = xcctrue(12) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_5cd8_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
	xm68k_areg(srcreg) += xareg_byteinc[srcreg];
{	int val = xcctrue(12) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_5ce0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) - xareg_byteinc[srcreg];
	xm68k_areg (srcreg) = srca;
{	int val = xcctrue(12) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_5ce8_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	int val = xcctrue(12) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(4);
}
void xop_5cf0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	int val = xcctrue(12) ? 0xff : 0;
	xput_byte(srca,val);
}}}}}
void xop_5cf8_0(uae_u32 opcode) /* Scc */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	int val = xcctrue(12) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(4);
}
void xop_5cf9_0(uae_u32 opcode) /* Scc */
{
{{	uaecptr srca = xget_ilong(2);
{	int val = xcctrue(12) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(6);
}
void xop_5cfa_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_5cfb_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_5cfc_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_5dc0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{{	int val = xcctrue(13) ? 0xff : 0;
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xff) | ((val) & 0xff);
}}}xm68k_incpc(2);
}
void xop_5dc8_0(uae_u32 opcode) /* DBcc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uae_s16 offs = xget_iword(2);
	if (!xcctrue(13)) {
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xffff) | (((src-1)) & 0xffff);
		if (src) {
			xm68k_incpc((uae_s32)offs + 2);
		}
	}
}}}xm68k_incpc(4);
endlabel1136: ;
}
void xop_5dd0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	int val = xcctrue(13) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_5dd8_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
	xm68k_areg(srcreg) += xareg_byteinc[srcreg];
{	int val = xcctrue(13) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_5de0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) - xareg_byteinc[srcreg];
	xm68k_areg (srcreg) = srca;
{	int val = xcctrue(13) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_5de8_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	int val = xcctrue(13) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(4);
}
void xop_5df0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	int val = xcctrue(13) ? 0xff : 0;
	xput_byte(srca,val);
}}}}}
void xop_5df8_0(uae_u32 opcode) /* Scc */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	int val = xcctrue(13) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(4);
}
void xop_5df9_0(uae_u32 opcode) /* Scc */
{
{{	uaecptr srca = xget_ilong(2);
{	int val = xcctrue(13) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(6);
}
void xop_5dfa_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_5dfb_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_5dfc_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_5ec0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{{	int val = xcctrue(14) ? 0xff : 0;
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xff) | ((val) & 0xff);
}}}xm68k_incpc(2);
}
void xop_5ec8_0(uae_u32 opcode) /* DBcc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uae_s16 offs = xget_iword(2);
	if (!xcctrue(14)) {
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xffff) | (((src-1)) & 0xffff);
		if (src) {
			xm68k_incpc((uae_s32)offs + 2);
		}
	}
}}}xm68k_incpc(4);
endlabel1148: ;
}
void xop_5ed0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	int val = xcctrue(14) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_5ed8_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
	xm68k_areg(srcreg) += xareg_byteinc[srcreg];
{	int val = xcctrue(14) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_5ee0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) - xareg_byteinc[srcreg];
	xm68k_areg (srcreg) = srca;
{	int val = xcctrue(14) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_5ee8_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	int val = xcctrue(14) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(4);
}
void xop_5ef0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	int val = xcctrue(14) ? 0xff : 0;
	xput_byte(srca,val);
}}}}}
void xop_5ef8_0(uae_u32 opcode) /* Scc */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	int val = xcctrue(14) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(4);
}
void xop_5ef9_0(uae_u32 opcode) /* Scc */
{
{{	uaecptr srca = xget_ilong(2);
{	int val = xcctrue(14) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(6);
}
void xop_5efa_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_5efb_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_5efc_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_5fc0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{{	int val = xcctrue(15) ? 0xff : 0;
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xff) | ((val) & 0xff);
}}}xm68k_incpc(2);
}
void xop_5fc8_0(uae_u32 opcode) /* DBcc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uae_s16 offs = xget_iword(2);
	if (!xcctrue(15)) {
	xm68k_dreg(srcreg) = (xm68k_dreg(srcreg) & ~0xffff) | (((src-1)) & 0xffff);
		if (src) {
			xm68k_incpc((uae_s32)offs + 2);
		}
	}
}}}xm68k_incpc(4);
endlabel1160: ;
}
void xop_5fd0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
{	int val = xcctrue(15) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_5fd8_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg);
	xm68k_areg(srcreg) += xareg_byteinc[srcreg];
{	int val = xcctrue(15) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_5fe0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) - xareg_byteinc[srcreg];
	xm68k_areg (srcreg) = srca;
{	int val = xcctrue(15) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(2);
}
void xop_5fe8_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	int val = xcctrue(15) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(4);
}
void xop_5ff0_0(uae_u32 opcode) /* Scc */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	int val = xcctrue(15) ? 0xff : 0;
	xput_byte(srca,val);
}}}}}
void xop_5ff8_0(uae_u32 opcode) /* Scc */
{
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	int val = xcctrue(15) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(4);
}
void xop_5ff9_0(uae_u32 opcode) /* Scc */
{
{{	uaecptr srca = xget_ilong(2);
{	int val = xcctrue(15) ? 0xff : 0;
	xput_byte(srca,val);
}}}xm68k_incpc(6);
}
void xop_5ffa_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_5ffb_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_5ffc_0(uae_u32 opcode) /* TRAPcc */
{
{}}
void xop_6000_0(uae_u32 opcode) /* Bcc */
{
{{	uae_s16 src = xget_iword(2);
	if (!xcctrue(0)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(4);
endlabel1171: ;
}
void xop_6001_0(uae_u32 opcode) /* Bcc */
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!xcctrue(0)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(2);
endlabel1172: ;
}
void xop_60ff_0(uae_u32 opcode) /* Bcc */
{
{{	uae_s32 src = xget_ilong(2);
	if (!xcctrue(0)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(6);
endlabel1173: ;
}
void xop_6100_0(uae_u32 opcode) /* BSR */
{
{{	uae_s16 src = xget_iword(2);
	uae_s32 s = (uae_s32)src + 2;
	xm68k_areg(7) -= 4;
	xput_long(xm68k_areg(7), xm68k_getpc() + 4);
	xm68k_incpc(s);
}}}
void xop_6101_0(uae_u32 opcode) /* BSR */
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	uae_s32 s = (uae_s32)src + 2;
	xm68k_areg(7) -= 4;
	xput_long(xm68k_areg(7), xm68k_getpc() + 2);
	xm68k_incpc(s);
}}}
void xop_61ff_0(uae_u32 opcode) /* BSR */
{
{{	uae_s32 src = xget_ilong(2);
	uae_s32 s = (uae_s32)src + 2;
	xm68k_areg(7) -= 4;
	xput_long(xm68k_areg(7), xm68k_getpc() + 6);
	xm68k_incpc(s);
}}}
void xop_6200_0(uae_u32 opcode) /* Bcc */
{
{{	uae_s16 src = xget_iword(2);
	if (!xcctrue(2)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(4);
endlabel1177: ;
}
void xop_6201_0(uae_u32 opcode) /* Bcc */
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!xcctrue(2)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(2);
endlabel1178: ;
}
void xop_62ff_0(uae_u32 opcode) /* Bcc */
{
{{	uae_s32 src = xget_ilong(2);
	if (!xcctrue(2)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(6);
endlabel1179: ;
}
void xop_6300_0(uae_u32 opcode) /* Bcc */
{
{{	uae_s16 src = xget_iword(2);
	if (!xcctrue(3)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(4);
endlabel1180: ;
}
void xop_6301_0(uae_u32 opcode) /* Bcc */
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!xcctrue(3)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(2);
endlabel1181: ;
}
void xop_63ff_0(uae_u32 opcode) /* Bcc */
{
{{	uae_s32 src = xget_ilong(2);
	if (!xcctrue(3)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(6);
endlabel1182: ;
}
void xop_6400_0(uae_u32 opcode) /* Bcc */
{
{{	uae_s16 src = xget_iword(2);
	if (!xcctrue(4)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(4);
endlabel1183: ;
}
void xop_6401_0(uae_u32 opcode) /* Bcc */
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!xcctrue(4)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(2);
endlabel1184: ;
}
void xop_64ff_0(uae_u32 opcode) /* Bcc */
{
{{	uae_s32 src = xget_ilong(2);
	if (!xcctrue(4)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(6);
endlabel1185: ;
}
void xop_6500_0(uae_u32 opcode) /* Bcc */
{
{{	uae_s16 src = xget_iword(2);
	if (!xcctrue(5)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(4);
endlabel1186: ;
}
void xop_6501_0(uae_u32 opcode) /* Bcc */
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!xcctrue(5)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(2);
endlabel1187: ;
}
void xop_65ff_0(uae_u32 opcode) /* Bcc */
{
{{	uae_s32 src = xget_ilong(2);
	if (!xcctrue(5)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(6);
endlabel1188: ;
}
void xop_6600_0(uae_u32 opcode) /* Bcc */
{
{{	uae_s16 src = xget_iword(2);
	if (!xcctrue(6)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(4);
endlabel1189: ;
}
void xop_6601_0(uae_u32 opcode) /* Bcc */
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!xcctrue(6)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(2);
endlabel1190: ;
}
void xop_66ff_0(uae_u32 opcode) /* Bcc */
{
{{	uae_s32 src = xget_ilong(2);
	if (!xcctrue(6)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(6);
endlabel1191: ;
}
void xop_6700_0(uae_u32 opcode) /* Bcc */
{
{{	uae_s16 src = xget_iword(2);
	if (!xcctrue(7)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(4);
endlabel1192: ;
}
void xop_6701_0(uae_u32 opcode) /* Bcc */
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!xcctrue(7)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(2);
endlabel1193: ;
}
void xop_67ff_0(uae_u32 opcode) /* Bcc */
{
{{	uae_s32 src = xget_ilong(2);
	if (!xcctrue(7)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(6);
endlabel1194: ;
}
void xop_6800_0(uae_u32 opcode) /* Bcc */
{
{{	uae_s16 src = xget_iword(2);
	if (!xcctrue(8)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(4);
endlabel1195: ;
}
void xop_6801_0(uae_u32 opcode) /* Bcc */
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!xcctrue(8)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(2);
endlabel1196: ;
}
void xop_68ff_0(uae_u32 opcode) /* Bcc */
{
{{	uae_s32 src = xget_ilong(2);
	if (!xcctrue(8)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(6);
endlabel1197: ;
}
void xop_6900_0(uae_u32 opcode) /* Bcc */
{
{{	uae_s16 src = xget_iword(2);
	if (!xcctrue(9)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(4);
endlabel1198: ;
}
void xop_6901_0(uae_u32 opcode) /* Bcc */
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!xcctrue(9)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(2);
endlabel1199: ;
}
void xop_69ff_0(uae_u32 opcode) /* Bcc */
{
{{	uae_s32 src = xget_ilong(2);
	if (!xcctrue(9)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(6);
endlabel1200: ;
}
void xop_6a00_0(uae_u32 opcode) /* Bcc */
{
{{	uae_s16 src = xget_iword(2);
	if (!xcctrue(10)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(4);
endlabel1201: ;
}
void xop_6a01_0(uae_u32 opcode) /* Bcc */
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!xcctrue(10)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(2);
endlabel1202: ;
}
void xop_6aff_0(uae_u32 opcode) /* Bcc */
{
{{	uae_s32 src = xget_ilong(2);
	if (!xcctrue(10)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(6);
endlabel1203: ;
}
void xop_6b00_0(uae_u32 opcode) /* Bcc */
{
{{	uae_s16 src = xget_iword(2);
	if (!xcctrue(11)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(4);
endlabel1204: ;
}
void xop_6b01_0(uae_u32 opcode) /* Bcc */
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!xcctrue(11)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(2);
endlabel1205: ;
}
void xop_6bff_0(uae_u32 opcode) /* Bcc */
{
{{	uae_s32 src = xget_ilong(2);
	if (!xcctrue(11)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(6);
endlabel1206: ;
}
void xop_6c00_0(uae_u32 opcode) /* Bcc */
{
{{	uae_s16 src = xget_iword(2);
	if (!xcctrue(12)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(4);
endlabel1207: ;
}
void xop_6c01_0(uae_u32 opcode) /* Bcc */
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!xcctrue(12)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(2);
endlabel1208: ;
}
void xop_6cff_0(uae_u32 opcode) /* Bcc */
{
{{	uae_s32 src = xget_ilong(2);
	if (!xcctrue(12)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(6);
endlabel1209: ;
}
void xop_6d00_0(uae_u32 opcode) /* Bcc */
{
{{	uae_s16 src = xget_iword(2);
	if (!xcctrue(13)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(4);
endlabel1210: ;
}
void xop_6d01_0(uae_u32 opcode) /* Bcc */
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!xcctrue(13)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(2);
endlabel1211: ;
}
void xop_6dff_0(uae_u32 opcode) /* Bcc */
{
{{	uae_s32 src = xget_ilong(2);
	if (!xcctrue(13)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(6);
endlabel1212: ;
}
void xop_6e00_0(uae_u32 opcode) /* Bcc */
{
{{	uae_s16 src = xget_iword(2);
	if (!xcctrue(14)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(4);
endlabel1213: ;
}
void xop_6e01_0(uae_u32 opcode) /* Bcc */
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!xcctrue(14)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(2);
endlabel1214: ;
}
void xop_6eff_0(uae_u32 opcode) /* Bcc */
{
{{	uae_s32 src = xget_ilong(2);
	if (!xcctrue(14)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(6);
endlabel1215: ;
}
void xop_6f00_0(uae_u32 opcode) /* Bcc */
{
{{	uae_s16 src = xget_iword(2);
	if (!xcctrue(15)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(4);
endlabel1216: ;
}
void xop_6f01_0(uae_u32 opcode) /* Bcc */
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
{{	uae_u32 src = srcreg;
	if (!xcctrue(15)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(2);
endlabel1217: ;
}
void xop_6fff_0(uae_u32 opcode) /* Bcc */
{
{{	uae_s32 src = xget_ilong(2);
	if (!xcctrue(15)) goto didnt_jump;
	xm68k_incpc ((uae_s32)src + 2);
	return;
didnt_jump:;
}}xm68k_incpc(6);
endlabel1218: ;
}
void xop_7000_0(uae_u32 opcode) /* MOVE */
{
	uae_u32 srcreg = (uae_s32)(uae_s8)(opcode & 255);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_u32 src = srcreg;
{	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xm68k_dreg(dstreg) = (src);
}}}xm68k_incpc(2);
}
void xop_8000_0(uae_u32 opcode) /* OR */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uae_s8 dst = xm68k_dreg(dstreg);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((src) & 0xff);
}}}xm68k_incpc(2);
}
void xop_8010_0(uae_u32 opcode) /* OR */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s8 src = xget_byte(srca);
{	uae_s8 dst = xm68k_dreg(dstreg);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((src) & 0xff);
}}}}xm68k_incpc(2);
}
void xop_8018_0(uae_u32 opcode) /* OR */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s8 src = xget_byte(srca);
	xm68k_areg(srcreg) += xareg_byteinc[srcreg];
{	uae_s8 dst = xm68k_dreg(dstreg);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((src) & 0xff);
}}}}xm68k_incpc(2);
}
void xop_8020_0(uae_u32 opcode) /* OR */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - xareg_byteinc[srcreg];
{	uae_s8 src = xget_byte(srca);
	xm68k_areg (srcreg) = srca;
{	uae_s8 dst = xm68k_dreg(dstreg);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((src) & 0xff);
}}}}xm68k_incpc(2);
}
void xop_8028_0(uae_u32 opcode) /* OR */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	uae_s8 dst = xm68k_dreg(dstreg);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((src) & 0xff);
}}}}xm68k_incpc(4);
}
void xop_8030_0(uae_u32 opcode) /* OR */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s8 src = xget_byte(srca);
{	uae_s8 dst = xm68k_dreg(dstreg);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((src) & 0xff);
}}}}}}
void xop_8038_0(uae_u32 opcode) /* OR */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	uae_s8 dst = xm68k_dreg(dstreg);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((src) & 0xff);
}}}}xm68k_incpc(4);
}
void xop_8039_0(uae_u32 opcode) /* OR */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s8 src = xget_byte(srca);
{	uae_s8 dst = xm68k_dreg(dstreg);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((src) & 0xff);
}}}}xm68k_incpc(6);
}
void xop_803a_0(uae_u32 opcode) /* OR */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	uae_s8 dst = xm68k_dreg(dstreg);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((src) & 0xff);
}}}}xm68k_incpc(4);
}
void xop_803b_0(uae_u32 opcode) /* OR */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s8 src = xget_byte(srca);
{	uae_s8 dst = xm68k_dreg(dstreg);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((src) & 0xff);
}}}}}}
void xop_803c_0(uae_u32 opcode) /* OR */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = xget_ibyte(2);
{	uae_s8 dst = xm68k_dreg(dstreg);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((src) & 0xff);
}}}xm68k_incpc(4);
}
void xop_8040_0(uae_u32 opcode) /* OR */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uae_s16 dst = xm68k_dreg(dstreg);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((src) & 0xffff);
}}}xm68k_incpc(2);
}
void xop_8050_0(uae_u32 opcode) /* OR */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}xm68k_incpc(2);
}
void xop_8058_0(uae_u32 opcode) /* OR */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
	xm68k_areg(srcreg) += 2;
{	uae_s16 dst = xm68k_dreg(dstreg);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}xm68k_incpc(2);
}
void xop_8060_0(uae_u32 opcode) /* OR */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - 2;
{	uae_s16 src = xget_word(srca);
	xm68k_areg (srcreg) = srca;
{	uae_s16 dst = xm68k_dreg(dstreg);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}xm68k_incpc(2);
}
void xop_8068_0(uae_u32 opcode) /* OR */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}xm68k_incpc(4);
}
void xop_8070_0(uae_u32 opcode) /* OR */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}}}
void xop_8078_0(uae_u32 opcode) /* OR */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}xm68k_incpc(4);
}
void xop_8079_0(uae_u32 opcode) /* OR */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}xm68k_incpc(6);
}
void xop_807a_0(uae_u32 opcode) /* OR */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}xm68k_incpc(4);
}
void xop_807b_0(uae_u32 opcode) /* OR */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}}}
void xop_807c_0(uae_u32 opcode) /* OR */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xget_iword(2);
{	uae_s16 dst = xm68k_dreg(dstreg);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((src) & 0xffff);
}}}xm68k_incpc(4);
}
void xop_8080_0(uae_u32 opcode) /* OR */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uae_s32 dst = xm68k_dreg(dstreg);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xm68k_dreg(dstreg) = (src);
}}}xm68k_incpc(2);
}
void xop_8090_0(uae_u32 opcode) /* OR */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xm68k_dreg(dstreg) = (src);
}}}}xm68k_incpc(2);
}
void xop_8098_0(uae_u32 opcode) /* OR */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
	xm68k_areg(srcreg) += 4;
{	uae_s32 dst = xm68k_dreg(dstreg);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xm68k_dreg(dstreg) = (src);
}}}}xm68k_incpc(2);
}
void xop_80a0_0(uae_u32 opcode) /* OR */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - 4;
{	uae_s32 src = xget_long(srca);
	xm68k_areg (srcreg) = srca;
{	uae_s32 dst = xm68k_dreg(dstreg);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xm68k_dreg(dstreg) = (src);
}}}}xm68k_incpc(2);
}
void xop_80a8_0(uae_u32 opcode) /* OR */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xm68k_dreg(dstreg) = (src);
}}}}xm68k_incpc(4);
}
void xop_80b0_0(uae_u32 opcode) /* OR */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xm68k_dreg(dstreg) = (src);
}}}}}}
void xop_80b8_0(uae_u32 opcode) /* OR */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xm68k_dreg(dstreg) = (src);
}}}}xm68k_incpc(4);
}
void xop_80b9_0(uae_u32 opcode) /* OR */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xm68k_dreg(dstreg) = (src);
}}}}xm68k_incpc(6);
}
void xop_80ba_0(uae_u32 opcode) /* OR */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xm68k_dreg(dstreg) = (src);
}}}}xm68k_incpc(4);
}
void xop_80bb_0(uae_u32 opcode) /* OR */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xm68k_dreg(dstreg) = (src);
}}}}}}
void xop_80bc_0(uae_u32 opcode) /* OR */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xget_ilong(2);
{	uae_s32 dst = xm68k_dreg(dstreg);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xm68k_dreg(dstreg) = (src);
}}}xm68k_incpc(6);
}
void xop_80c0_0(uae_u32 opcode) /* DIVU */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr oldpc = xm68k_getpc();
{	uae_s16 src = xm68k_dreg(srcreg);
{	uae_s32 dst = xm68k_dreg(dstreg);
xm68k_incpc(2);
{	uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
	uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
	if (newv > 0xffff) { XSET_VFLG (1); XSET_NFLG (1); XSET_CFLG (0); } else
	{
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_NFLG (((uae_s16)(newv)) < 0);
	newv = (newv & 0xffff) | ((uae_u32)rem << 16);
	xm68k_dreg(dstreg) = (newv);
	}
}}}}endlabel1253: ;
}
void xop_80d0_0(uae_u32 opcode) /* DIVU */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr oldpc = xm68k_getpc();
{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
xm68k_incpc(2);
{	uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
	uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
	if (newv > 0xffff) { XSET_VFLG (1); XSET_NFLG (1); XSET_CFLG (0); } else
	{
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_NFLG (((uae_s16)(newv)) < 0);
	newv = (newv & 0xffff) | ((uae_u32)rem << 16);
	xm68k_dreg(dstreg) = (newv);
	}
}}}}}endlabel1254: ;
}
void xop_80d8_0(uae_u32 opcode) /* DIVU */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr oldpc = xm68k_getpc();
{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
	xm68k_areg(srcreg) += 2;
{	uae_s32 dst = xm68k_dreg(dstreg);
xm68k_incpc(2);
{	uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
	uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
	if (newv > 0xffff) { XSET_VFLG (1); XSET_NFLG (1); XSET_CFLG (0); } else
	{
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_NFLG (((uae_s16)(newv)) < 0);
	newv = (newv & 0xffff) | ((uae_u32)rem << 16);
	xm68k_dreg(dstreg) = (newv);
	}
}}}}}endlabel1255: ;
}
void xop_80e0_0(uae_u32 opcode) /* DIVU */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr oldpc = xm68k_getpc();
{	uaecptr srca = xm68k_areg(srcreg) - 2;
{	uae_s16 src = xget_word(srca);
	xm68k_areg (srcreg) = srca;
{	uae_s32 dst = xm68k_dreg(dstreg);
xm68k_incpc(2);
{	uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
	uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
	if (newv > 0xffff) { XSET_VFLG (1); XSET_NFLG (1); XSET_CFLG (0); } else
	{
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_NFLG (((uae_s16)(newv)) < 0);
	newv = (newv & 0xffff) | ((uae_u32)rem << 16);
	xm68k_dreg(dstreg) = (newv);
	}
}}}}}endlabel1256: ;
}
void xop_80e8_0(uae_u32 opcode) /* DIVU */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr oldpc = xm68k_getpc();
{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
xm68k_incpc(4);
{	uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
	uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
	if (newv > 0xffff) { XSET_VFLG (1); XSET_NFLG (1); XSET_CFLG (0); } else
	{
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_NFLG (((uae_s16)(newv)) < 0);
	newv = (newv & 0xffff) | ((uae_u32)rem << 16);
	xm68k_dreg(dstreg) = (newv);
	}
}}}}}endlabel1257: ;
}
void xop_80f0_0(uae_u32 opcode) /* DIVU */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr oldpc = xm68k_getpc();
{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s16 src = xget_word(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
{	uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
	uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
	if (newv > 0xffff) { XSET_VFLG (1); XSET_NFLG (1); XSET_CFLG (0); } else
	{
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_NFLG (((uae_s16)(newv)) < 0);
	newv = (newv & 0xffff) | ((uae_u32)rem << 16);
	xm68k_dreg(dstreg) = (newv);
	}
}}}}}}endlabel1258: ;
}
void xop_80f8_0(uae_u32 opcode) /* DIVU */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr oldpc = xm68k_getpc();
{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
xm68k_incpc(4);
{	uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
	uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
	if (newv > 0xffff) { XSET_VFLG (1); XSET_NFLG (1); XSET_CFLG (0); } else
	{
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_NFLG (((uae_s16)(newv)) < 0);
	newv = (newv & 0xffff) | ((uae_u32)rem << 16);
	xm68k_dreg(dstreg) = (newv);
	}
}}}}}endlabel1259: ;
}
void xop_80f9_0(uae_u32 opcode) /* DIVU */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr oldpc = xm68k_getpc();
{	uaecptr srca = xget_ilong(2);
{	uae_s16 src = xget_word(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
xm68k_incpc(6);
{	uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
	uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
	if (newv > 0xffff) { XSET_VFLG (1); XSET_NFLG (1); XSET_CFLG (0); } else
	{
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_NFLG (((uae_s16)(newv)) < 0);
	newv = (newv & 0xffff) | ((uae_u32)rem << 16);
	xm68k_dreg(dstreg) = (newv);
	}
}}}}}endlabel1260: ;
}
void xop_80fa_0(uae_u32 opcode) /* DIVU */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr oldpc = xm68k_getpc();
{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
xm68k_incpc(4);
{	uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
	uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
	if (newv > 0xffff) { XSET_VFLG (1); XSET_NFLG (1); XSET_CFLG (0); } else
	{
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_NFLG (((uae_s16)(newv)) < 0);
	newv = (newv & 0xffff) | ((uae_u32)rem << 16);
	xm68k_dreg(dstreg) = (newv);
	}
}}}}}endlabel1261: ;
}
void xop_80fb_0(uae_u32 opcode) /* DIVU */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr oldpc = xm68k_getpc();
{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s16 src = xget_word(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
{	uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
	uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
	if (newv > 0xffff) { XSET_VFLG (1); XSET_NFLG (1); XSET_CFLG (0); } else
	{
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_NFLG (((uae_s16)(newv)) < 0);
	newv = (newv & 0xffff) | ((uae_u32)rem << 16);
	xm68k_dreg(dstreg) = (newv);
	}
}}}}}}endlabel1262: ;
}
void xop_80fc_0(uae_u32 opcode) /* DIVU */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr oldpc = xm68k_getpc();
{	uae_s16 src = xget_iword(2);
{	uae_s32 dst = xm68k_dreg(dstreg);
xm68k_incpc(4);
{	uae_u32 newv = (uae_u32)dst / (uae_u32)(uae_u16)src;
	uae_u32 rem = (uae_u32)dst % (uae_u32)(uae_u16)src;
	if (newv > 0xffff) { XSET_VFLG (1); XSET_NFLG (1); XSET_CFLG (0); } else
	{
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_NFLG (((uae_s16)(newv)) < 0);
	newv = (newv & 0xffff) | ((uae_u32)rem << 16);
	xm68k_dreg(dstreg) = (newv);
	}
}}}}endlabel1263: ;
}
void xop_8100_0(uae_u32 opcode) /* SBCD */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uae_s8 dst = xm68k_dreg(dstreg);
{	uae_u16 newv_lo = (dst & 0xF) - (src & 0xF) - (XGET_XFLG ? 1 : 0);
	uae_u16 newv_hi = (dst & 0xF0) - (src & 0xF0);
	uae_u16 newv, tmp_newv;
	int bcd = 0;
	newv = tmp_newv = newv_hi + newv_lo;
	if (newv_lo & 0xF0) { newv -= 6; bcd = 6; };
	if ((((dst & 0xFF) - (src & 0xFF) - (XGET_XFLG ? 1 : 0)) & 0x100) > 0xFF) { newv -= 0x60; }
	XSET_CFLG ((((dst & 0xFF) - (src & 0xFF) - bcd - (XGET_XFLG ? 1 : 0)) & 0x300) > 0xFF);
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s8)(newv)) == 0));
	XSET_NFLG (((uae_s8)(newv)) < 0);
	XSET_VFLG ((tmp_newv & 0x80) != 0 && (newv & 0x80) == 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((newv) & 0xff);
}}}}xm68k_incpc(2);
}
void xop_8108_0(uae_u32 opcode) /* SBCD */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - xareg_byteinc[srcreg];
{	uae_s8 src = xget_byte(srca);
	xm68k_areg (srcreg) = srca;
{	uaecptr dsta = xm68k_areg(dstreg) - xareg_byteinc[dstreg];
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg (dstreg) = dsta;
{	uae_u16 newv_lo = (dst & 0xF) - (src & 0xF) - (XGET_XFLG ? 1 : 0);
	uae_u16 newv_hi = (dst & 0xF0) - (src & 0xF0);
	uae_u16 newv, tmp_newv;
	int bcd = 0;
	newv = tmp_newv = newv_hi + newv_lo;
	if (newv_lo & 0xF0) { newv -= 6; bcd = 6; };
	if ((((dst & 0xFF) - (src & 0xFF) - (XGET_XFLG ? 1 : 0)) & 0x100) > 0xFF) { newv -= 0x60; }
	XSET_CFLG ((((dst & 0xFF) - (src & 0xFF) - bcd - (XGET_XFLG ? 1 : 0)) & 0x300) > 0xFF);
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s8)(newv)) == 0));
	XSET_NFLG (((uae_s8)(newv)) < 0);
	XSET_VFLG ((tmp_newv & 0x80) != 0 && (newv & 0x80) == 0);
	xput_byte(dsta,newv);
}}}}}}xm68k_incpc(2);
}
void xop_8110_0(uae_u32 opcode) /* OR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_8118_0(uae_u32 opcode) /* OR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg(dstreg) += xareg_byteinc[dstreg];
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_8120_0(uae_u32 opcode) /* OR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) - xareg_byteinc[dstreg];
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg (dstreg) = dsta;
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_8128_0(uae_u32 opcode) /* OR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 dst = xget_byte(dsta);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_8130_0(uae_u32 opcode) /* OR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{xm68k_incpc(2);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s8 dst = xget_byte(dsta);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}}}
void xop_8138_0(uae_u32 opcode) /* OR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 dst = xget_byte(dsta);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_8139_0(uae_u32 opcode) /* OR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xget_ilong(2);
{	uae_s8 dst = xget_byte(dsta);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_8140_0(uae_u32 opcode) /* PACK */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{}}
void xop_8148_0(uae_u32 opcode) /* PACK */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{}}
void xop_8150_0(uae_u32 opcode) /* OR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s16 dst = xget_word(dsta);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_8158_0(uae_u32 opcode) /* OR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s16 dst = xget_word(dsta);
	xm68k_areg(dstreg) += 2;
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_8160_0(uae_u32 opcode) /* OR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) - 2;
{	uae_s16 dst = xget_word(dsta);
	xm68k_areg (dstreg) = dsta;
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_8168_0(uae_u32 opcode) /* OR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 dst = xget_word(dsta);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_8170_0(uae_u32 opcode) /* OR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{xm68k_incpc(2);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s16 dst = xget_word(dsta);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}}}
void xop_8178_0(uae_u32 opcode) /* OR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 dst = xget_word(dsta);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_8179_0(uae_u32 opcode) /* OR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xget_ilong(2);
{	uae_s16 dst = xget_word(dsta);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_8180_0(uae_u32 opcode) /* UNPK */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{}}
void xop_8188_0(uae_u32 opcode) /* UNPK */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{}}
void xop_8190_0(uae_u32 opcode) /* OR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s32 dst = xget_long(dsta);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_8198_0(uae_u32 opcode) /* OR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s32 dst = xget_long(dsta);
	xm68k_areg(dstreg) += 4;
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_81a0_0(uae_u32 opcode) /* OR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) - 4;
{	uae_s32 dst = xget_long(dsta);
	xm68k_areg (dstreg) = dsta;
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_81a8_0(uae_u32 opcode) /* OR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 dst = xget_long(dsta);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_81b0_0(uae_u32 opcode) /* OR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{xm68k_incpc(2);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s32 dst = xget_long(dsta);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}}}
void xop_81b8_0(uae_u32 opcode) /* OR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 dst = xget_long(dsta);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_81b9_0(uae_u32 opcode) /* OR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xget_ilong(2);
{	uae_s32 dst = xget_long(dsta);
	src |= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_81c0_0(uae_u32 opcode) /* DIVS */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr oldpc = xm68k_getpc();
{	uae_s16 src = xm68k_dreg(srcreg);
{	uae_s32 dst = xm68k_dreg(dstreg);
xm68k_incpc(2);
{	uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
	uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
	if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) { XSET_VFLG (1); XSET_NFLG (1); XSET_CFLG (0); } else
	{
	if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_NFLG (((uae_s16)(newv)) < 0);
	newv = (newv & 0xffff) | ((uae_u32)rem << 16);
	xm68k_dreg(dstreg) = (newv);
	}
}}}}endlabel1291: ;
}
void xop_81d0_0(uae_u32 opcode) /* DIVS */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr oldpc = xm68k_getpc();
{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
xm68k_incpc(2);
{	uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
	uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
	if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) { XSET_VFLG (1); XSET_NFLG (1); XSET_CFLG (0); } else
	{
	if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_NFLG (((uae_s16)(newv)) < 0);
	newv = (newv & 0xffff) | ((uae_u32)rem << 16);
	xm68k_dreg(dstreg) = (newv);
	}
}}}}}endlabel1292: ;
}
void xop_81d8_0(uae_u32 opcode) /* DIVS */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr oldpc = xm68k_getpc();
{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
	xm68k_areg(srcreg) += 2;
{	uae_s32 dst = xm68k_dreg(dstreg);
xm68k_incpc(2);
{	uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
	uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
	if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) { XSET_VFLG (1); XSET_NFLG (1); XSET_CFLG (0); } else
	{
	if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_NFLG (((uae_s16)(newv)) < 0);
	newv = (newv & 0xffff) | ((uae_u32)rem << 16);
	xm68k_dreg(dstreg) = (newv);
	}
}}}}}endlabel1293: ;
}
void xop_81e0_0(uae_u32 opcode) /* DIVS */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr oldpc = xm68k_getpc();
{	uaecptr srca = xm68k_areg(srcreg) - 2;
{	uae_s16 src = xget_word(srca);
	xm68k_areg (srcreg) = srca;
{	uae_s32 dst = xm68k_dreg(dstreg);
xm68k_incpc(2);
{	uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
	uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
	if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) { XSET_VFLG (1); XSET_NFLG (1); XSET_CFLG (0); } else
	{
	if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_NFLG (((uae_s16)(newv)) < 0);
	newv = (newv & 0xffff) | ((uae_u32)rem << 16);
	xm68k_dreg(dstreg) = (newv);
	}
}}}}}endlabel1294: ;
}
void xop_81e8_0(uae_u32 opcode) /* DIVS */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr oldpc = xm68k_getpc();
{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
xm68k_incpc(4);
{	uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
	uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
	if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) { XSET_VFLG (1); XSET_NFLG (1); XSET_CFLG (0); } else
	{
	if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_NFLG (((uae_s16)(newv)) < 0);
	newv = (newv & 0xffff) | ((uae_u32)rem << 16);
	xm68k_dreg(dstreg) = (newv);
	}
}}}}}endlabel1295: ;
}
void xop_81f0_0(uae_u32 opcode) /* DIVS */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr oldpc = xm68k_getpc();
{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s16 src = xget_word(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
{	uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
	uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
	if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) { XSET_VFLG (1); XSET_NFLG (1); XSET_CFLG (0); } else
	{
	if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_NFLG (((uae_s16)(newv)) < 0);
	newv = (newv & 0xffff) | ((uae_u32)rem << 16);
	xm68k_dreg(dstreg) = (newv);
	}
}}}}}}endlabel1296: ;
}
void xop_81f8_0(uae_u32 opcode) /* DIVS */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr oldpc = xm68k_getpc();
{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
xm68k_incpc(4);
{	uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
	uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
	if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) { XSET_VFLG (1); XSET_NFLG (1); XSET_CFLG (0); } else
	{
	if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_NFLG (((uae_s16)(newv)) < 0);
	newv = (newv & 0xffff) | ((uae_u32)rem << 16);
	xm68k_dreg(dstreg) = (newv);
	}
}}}}}endlabel1297: ;
}
void xop_81f9_0(uae_u32 opcode) /* DIVS */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr oldpc = xm68k_getpc();
{	uaecptr srca = xget_ilong(2);
{	uae_s16 src = xget_word(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
xm68k_incpc(6);
{	uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
	uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
	if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) { XSET_VFLG (1); XSET_NFLG (1); XSET_CFLG (0); } else
	{
	if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_NFLG (((uae_s16)(newv)) < 0);
	newv = (newv & 0xffff) | ((uae_u32)rem << 16);
	xm68k_dreg(dstreg) = (newv);
	}
}}}}}endlabel1298: ;
}
void xop_81fa_0(uae_u32 opcode) /* DIVS */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr oldpc = xm68k_getpc();
{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
xm68k_incpc(4);
{	uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
	uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
	if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) { XSET_VFLG (1); XSET_NFLG (1); XSET_CFLG (0); } else
	{
	if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_NFLG (((uae_s16)(newv)) < 0);
	newv = (newv & 0xffff) | ((uae_u32)rem << 16);
	xm68k_dreg(dstreg) = (newv);
	}
}}}}}endlabel1299: ;
}
void xop_81fb_0(uae_u32 opcode) /* DIVS */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr oldpc = xm68k_getpc();
{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s16 src = xget_word(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
{	uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
	uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
	if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) { XSET_VFLG (1); XSET_NFLG (1); XSET_CFLG (0); } else
	{
	if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_NFLG (((uae_s16)(newv)) < 0);
	newv = (newv & 0xffff) | ((uae_u32)rem << 16);
	xm68k_dreg(dstreg) = (newv);
	}
}}}}}}endlabel1300: ;
}
void xop_81fc_0(uae_u32 opcode) /* DIVS */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{	uaecptr oldpc = xm68k_getpc();
{	uae_s16 src = xget_iword(2);
{	uae_s32 dst = xm68k_dreg(dstreg);
xm68k_incpc(4);
{	uae_s32 newv = (uae_s32)dst / (uae_s32)(uae_s16)src;
	uae_u16 rem = (uae_s32)dst % (uae_s32)(uae_s16)src;
	if ((newv & 0xffff8000) != 0 && (newv & 0xffff8000) != 0xffff8000) { XSET_VFLG (1); XSET_NFLG (1); XSET_CFLG (0); } else
	{
	if (((uae_s16)rem < 0) != ((uae_s32)dst < 0)) rem = -rem;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_NFLG (((uae_s16)(newv)) < 0);
	newv = (newv & 0xffff) | ((uae_u32)rem << 16);
	xm68k_dreg(dstreg) = (newv);
	}
}}}}endlabel1301: ;
}
void xop_9000_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uae_s8 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}xm68k_incpc(2);
}
void xop_9010_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s8 src = xget_byte(srca);
{	uae_s8 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}xm68k_incpc(2);
}
void xop_9018_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s8 src = xget_byte(srca);
	xm68k_areg(srcreg) += xareg_byteinc[srcreg];
{	uae_s8 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}xm68k_incpc(2);
}
void xop_9020_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - xareg_byteinc[srcreg];
{	uae_s8 src = xget_byte(srca);
	xm68k_areg (srcreg) = srca;
{	uae_s8 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}xm68k_incpc(2);
}
void xop_9028_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	uae_s8 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}xm68k_incpc(4);
}
void xop_9030_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s8 src = xget_byte(srca);
{	uae_s8 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}}}
void xop_9038_0(uae_u32 opcode) /* SUB */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	uae_s8 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}xm68k_incpc(4);
}
void xop_9039_0(uae_u32 opcode) /* SUB */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s8 src = xget_byte(srca);
{	uae_s8 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}xm68k_incpc(6);
}
void xop_903a_0(uae_u32 opcode) /* SUB */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	uae_s8 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}xm68k_incpc(4);
}
void xop_903b_0(uae_u32 opcode) /* SUB */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s8 src = xget_byte(srca);
{	uae_s8 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}}}
void xop_903c_0(uae_u32 opcode) /* SUB */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = xget_ibyte(2);
{	uae_s8 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}xm68k_incpc(4);
}
void xop_9040_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}xm68k_incpc(2);
}
void xop_9048_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xm68k_areg(srcreg);
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}xm68k_incpc(2);
}
void xop_9050_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}xm68k_incpc(2);
}
void xop_9058_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
	xm68k_areg(srcreg) += 2;
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}xm68k_incpc(2);
}
void xop_9060_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - 2;
{	uae_s16 src = xget_word(srca);
	xm68k_areg (srcreg) = srca;
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}xm68k_incpc(2);
}
void xop_9068_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}xm68k_incpc(4);
}
void xop_9070_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}}}
void xop_9078_0(uae_u32 opcode) /* SUB */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}xm68k_incpc(4);
}
void xop_9079_0(uae_u32 opcode) /* SUB */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}xm68k_incpc(6);
}
void xop_907a_0(uae_u32 opcode) /* SUB */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}xm68k_incpc(4);
}
void xop_907b_0(uae_u32 opcode) /* SUB */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}}}
void xop_907c_0(uae_u32 opcode) /* SUB */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xget_iword(2);
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}xm68k_incpc(4);
}
void xop_9080_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}}xm68k_incpc(2);
}
void xop_9088_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xm68k_areg(srcreg);
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}}xm68k_incpc(2);
}
void xop_9090_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}}}xm68k_incpc(2);
}
void xop_9098_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
	xm68k_areg(srcreg) += 4;
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}}}xm68k_incpc(2);
}
void xop_90a0_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - 4;
{	uae_s32 src = xget_long(srca);
	xm68k_areg (srcreg) = srca;
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}}}xm68k_incpc(2);
}
void xop_90a8_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}}}xm68k_incpc(4);
}
void xop_90b0_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}}}}}
void xop_90b8_0(uae_u32 opcode) /* SUB */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}}}xm68k_incpc(4);
}
void xop_90b9_0(uae_u32 opcode) /* SUB */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}}}xm68k_incpc(6);
}
void xop_90ba_0(uae_u32 opcode) /* SUB */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}}}xm68k_incpc(4);
}
void xop_90bb_0(uae_u32 opcode) /* SUB */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}}}}}
void xop_90bc_0(uae_u32 opcode) /* SUB */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xget_ilong(2);
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}}xm68k_incpc(6);
}
void xop_90c0_0(uae_u32 opcode) /* SUBA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst - src;
	xm68k_areg(dstreg) = (newv);
}}}}xm68k_incpc(2);
}
void xop_90c8_0(uae_u32 opcode) /* SUBA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xm68k_areg(srcreg);
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst - src;
	xm68k_areg(dstreg) = (newv);
}}}}xm68k_incpc(2);
}
void xop_90d0_0(uae_u32 opcode) /* SUBA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst - src;
	xm68k_areg(dstreg) = (newv);
}}}}}xm68k_incpc(2);
}
void xop_90d8_0(uae_u32 opcode) /* SUBA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
	xm68k_areg(srcreg) += 2;
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst - src;
	xm68k_areg(dstreg) = (newv);
}}}}}xm68k_incpc(2);
}
void xop_90e0_0(uae_u32 opcode) /* SUBA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - 2;
{	uae_s16 src = xget_word(srca);
	xm68k_areg (srcreg) = srca;
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst - src;
	xm68k_areg(dstreg) = (newv);
}}}}}xm68k_incpc(2);
}
void xop_90e8_0(uae_u32 opcode) /* SUBA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst - src;
	xm68k_areg(dstreg) = (newv);
}}}}}xm68k_incpc(4);
}
void xop_90f0_0(uae_u32 opcode) /* SUBA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s16 src = xget_word(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst - src;
	xm68k_areg(dstreg) = (newv);
}}}}}}}
void xop_90f8_0(uae_u32 opcode) /* SUBA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst - src;
	xm68k_areg(dstreg) = (newv);
}}}}}xm68k_incpc(4);
}
void xop_90f9_0(uae_u32 opcode) /* SUBA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s16 src = xget_word(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst - src;
	xm68k_areg(dstreg) = (newv);
}}}}}xm68k_incpc(6);
}
void xop_90fa_0(uae_u32 opcode) /* SUBA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst - src;
	xm68k_areg(dstreg) = (newv);
}}}}}xm68k_incpc(4);
}
void xop_90fb_0(uae_u32 opcode) /* SUBA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s16 src = xget_word(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst - src;
	xm68k_areg(dstreg) = (newv);
}}}}}}}
void xop_90fc_0(uae_u32 opcode) /* SUBA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xget_iword(2);
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst - src;
	xm68k_areg(dstreg) = (newv);
}}}}xm68k_incpc(4);
}
void xop_9100_0(uae_u32 opcode) /* SUBX */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uae_s8 dst = xm68k_dreg(dstreg);
{	uae_u32 newv = dst - src - (XGET_XFLG ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	XSET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s8)(newv)) == 0));
	XSET_NFLG (((uae_s8)(newv)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}xm68k_incpc(2);
}
void xop_9108_0(uae_u32 opcode) /* SUBX */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - xareg_byteinc[srcreg];
{	uae_s8 src = xget_byte(srca);
	xm68k_areg (srcreg) = srca;
{	uaecptr dsta = xm68k_areg(dstreg) - xareg_byteinc[dstreg];
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg (dstreg) = dsta;
{	uae_u32 newv = dst - src - (XGET_XFLG ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	XSET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s8)(newv)) == 0));
	XSET_NFLG (((uae_s8)(newv)) < 0);
	xput_byte(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_9110_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_9118_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg(dstreg) += xareg_byteinc[dstreg];
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_9120_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) - xareg_byteinc[dstreg];
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg (dstreg) = dsta;
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_9128_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 dst = xget_byte(dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}xm68k_incpc(4);
}
void xop_9130_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{xm68k_incpc(2);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s8 dst = xget_byte(dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}}}
void xop_9138_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 dst = xget_byte(dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}xm68k_incpc(4);
}
void xop_9139_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xget_ilong(2);
{	uae_s8 dst = xget_byte(dsta);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}xm68k_incpc(6);
}
void xop_9140_0(uae_u32 opcode) /* SUBX */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uae_s16 dst = xm68k_dreg(dstreg);
{	uae_u32 newv = dst - src - (XGET_XFLG ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	XSET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s16)(newv)) == 0));
	XSET_NFLG (((uae_s16)(newv)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}xm68k_incpc(2);
}
void xop_9148_0(uae_u32 opcode) /* SUBX */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - 2;
{	uae_s16 src = xget_word(srca);
	xm68k_areg (srcreg) = srca;
{	uaecptr dsta = xm68k_areg(dstreg) - 2;
{	uae_s16 dst = xget_word(dsta);
	xm68k_areg (dstreg) = dsta;
{	uae_u32 newv = dst - src - (XGET_XFLG ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	XSET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s16)(newv)) == 0));
	XSET_NFLG (((uae_s16)(newv)) < 0);
	xput_word(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_9150_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s16 dst = xget_word(dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_9158_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s16 dst = xget_word(dsta);
	xm68k_areg(dstreg) += 2;
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_9160_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) - 2;
{	uae_s16 dst = xget_word(dsta);
	xm68k_areg (dstreg) = dsta;
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_9168_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 dst = xget_word(dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}xm68k_incpc(4);
}
void xop_9170_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{xm68k_incpc(2);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s16 dst = xget_word(dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}}}
void xop_9178_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 dst = xget_word(dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}xm68k_incpc(4);
}
void xop_9179_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xget_ilong(2);
{	uae_s16 dst = xget_word(dsta);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}xm68k_incpc(6);
}
void xop_9180_0(uae_u32 opcode) /* SUBX */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uae_s32 dst = xm68k_dreg(dstreg);
{	uae_u32 newv = dst - src - (XGET_XFLG ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	XSET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s32)(newv)) == 0));
	XSET_NFLG (((uae_s32)(newv)) < 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}xm68k_incpc(2);
}
void xop_9188_0(uae_u32 opcode) /* SUBX */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - 4;
{	uae_s32 src = xget_long(srca);
	xm68k_areg (srcreg) = srca;
{	uaecptr dsta = xm68k_areg(dstreg) - 4;
{	uae_s32 dst = xget_long(dsta);
	xm68k_areg (dstreg) = dsta;
{	uae_u32 newv = dst - src - (XGET_XFLG ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_VFLG ((flgs ^ flgo) & (flgo ^ flgn));
	XSET_CFLG (flgs ^ ((flgs ^ flgn) & (flgo ^ flgn)));
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s32)(newv)) == 0));
	XSET_NFLG (((uae_s32)(newv)) < 0);
	xput_long(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_9190_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s32 dst = xget_long(dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_9198_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s32 dst = xget_long(dsta);
	xm68k_areg(dstreg) += 4;
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_91a0_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) - 4;
{	uae_s32 dst = xget_long(dsta);
	xm68k_areg (dstreg) = dsta;
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_91a8_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 dst = xget_long(dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}xm68k_incpc(4);
}
void xop_91b0_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{xm68k_incpc(2);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s32 dst = xget_long(dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}}}
void xop_91b8_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 dst = xget_long(dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}xm68k_incpc(4);
}
void xop_91b9_0(uae_u32 opcode) /* SUB */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xget_ilong(2);
{	uae_s32 dst = xget_long(dsta);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgo) & (flgn ^ flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}xm68k_incpc(6);
}
void xop_91c0_0(uae_u32 opcode) /* SUBA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst - src;
	xm68k_areg(dstreg) = (newv);
}}}}xm68k_incpc(2);
}
void xop_91c8_0(uae_u32 opcode) /* SUBA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xm68k_areg(srcreg);
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst - src;
	xm68k_areg(dstreg) = (newv);
}}}}xm68k_incpc(2);
}
void xop_91d0_0(uae_u32 opcode) /* SUBA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst - src;
	xm68k_areg(dstreg) = (newv);
}}}}}xm68k_incpc(2);
}
void xop_91d8_0(uae_u32 opcode) /* SUBA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
	xm68k_areg(srcreg) += 4;
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst - src;
	xm68k_areg(dstreg) = (newv);
}}}}}xm68k_incpc(2);
}
void xop_91e0_0(uae_u32 opcode) /* SUBA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - 4;
{	uae_s32 src = xget_long(srca);
	xm68k_areg (srcreg) = srca;
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst - src;
	xm68k_areg(dstreg) = (newv);
}}}}}xm68k_incpc(2);
}
void xop_91e8_0(uae_u32 opcode) /* SUBA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst - src;
	xm68k_areg(dstreg) = (newv);
}}}}}xm68k_incpc(4);
}
void xop_91f0_0(uae_u32 opcode) /* SUBA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst - src;
	xm68k_areg(dstreg) = (newv);
}}}}}}}
void xop_91f8_0(uae_u32 opcode) /* SUBA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst - src;
	xm68k_areg(dstreg) = (newv);
}}}}}xm68k_incpc(4);
}
void xop_91f9_0(uae_u32 opcode) /* SUBA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst - src;
	xm68k_areg(dstreg) = (newv);
}}}}}xm68k_incpc(6);
}
void xop_91fa_0(uae_u32 opcode) /* SUBA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst - src;
	xm68k_areg(dstreg) = (newv);
}}}}}xm68k_incpc(4);
}
void xop_91fb_0(uae_u32 opcode) /* SUBA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst - src;
	xm68k_areg(dstreg) = (newv);
}}}}}}}
void xop_91fc_0(uae_u32 opcode) /* SUBA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xget_ilong(2);
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst - src;
	xm68k_areg(dstreg) = (newv);
}}}}xm68k_incpc(6);
}
void xop_b000_0(uae_u32 opcode) /* CMP */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uae_s8 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}xm68k_incpc(2);
}
void xop_b010_0(uae_u32 opcode) /* CMP */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s8 src = xget_byte(srca);
{	uae_s8 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(2);
}
void xop_b018_0(uae_u32 opcode) /* CMP */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s8 src = xget_byte(srca);
	xm68k_areg(srcreg) += xareg_byteinc[srcreg];
{	uae_s8 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(2);
}
void xop_b020_0(uae_u32 opcode) /* CMP */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - xareg_byteinc[srcreg];
{	uae_s8 src = xget_byte(srca);
	xm68k_areg (srcreg) = srca;
{	uae_s8 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(2);
}
void xop_b028_0(uae_u32 opcode) /* CMP */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	uae_s8 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(4);
}
void xop_b030_0(uae_u32 opcode) /* CMP */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s8 src = xget_byte(srca);
{	uae_s8 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}}}
void xop_b038_0(uae_u32 opcode) /* CMP */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	uae_s8 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(4);
}
void xop_b039_0(uae_u32 opcode) /* CMP */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s8 src = xget_byte(srca);
{	uae_s8 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(6);
}
void xop_b03a_0(uae_u32 opcode) /* CMP */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	uae_s8 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(4);
}
void xop_b03b_0(uae_u32 opcode) /* CMP */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s8 src = xget_byte(srca);
{	uae_s8 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}}}
void xop_b03c_0(uae_u32 opcode) /* CMP */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = xget_ibyte(2);
{	uae_s8 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}xm68k_incpc(4);
}
void xop_b040_0(uae_u32 opcode) /* CMP */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}xm68k_incpc(2);
}
void xop_b048_0(uae_u32 opcode) /* CMP */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xm68k_areg(srcreg);
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}xm68k_incpc(2);
}
void xop_b050_0(uae_u32 opcode) /* CMP */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(2);
}
void xop_b058_0(uae_u32 opcode) /* CMP */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
	xm68k_areg(srcreg) += 2;
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(2);
}
void xop_b060_0(uae_u32 opcode) /* CMP */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - 2;
{	uae_s16 src = xget_word(srca);
	xm68k_areg (srcreg) = srca;
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(2);
}
void xop_b068_0(uae_u32 opcode) /* CMP */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(4);
}
void xop_b070_0(uae_u32 opcode) /* CMP */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}}}
void xop_b078_0(uae_u32 opcode) /* CMP */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(4);
}
void xop_b079_0(uae_u32 opcode) /* CMP */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(6);
}
void xop_b07a_0(uae_u32 opcode) /* CMP */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(4);
}
void xop_b07b_0(uae_u32 opcode) /* CMP */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}}}
void xop_b07c_0(uae_u32 opcode) /* CMP */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xget_iword(2);
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}xm68k_incpc(4);
}
void xop_b080_0(uae_u32 opcode) /* CMP */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}xm68k_incpc(2);
}
void xop_b088_0(uae_u32 opcode) /* CMP */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xm68k_areg(srcreg);
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}xm68k_incpc(2);
}
void xop_b090_0(uae_u32 opcode) /* CMP */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(2);
}
void xop_b098_0(uae_u32 opcode) /* CMP */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
	xm68k_areg(srcreg) += 4;
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(2);
}
void xop_b0a0_0(uae_u32 opcode) /* CMP */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - 4;
{	uae_s32 src = xget_long(srca);
	xm68k_areg (srcreg) = srca;
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(2);
}
void xop_b0a8_0(uae_u32 opcode) /* CMP */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(4);
}
void xop_b0b0_0(uae_u32 opcode) /* CMP */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}}}
void xop_b0b8_0(uae_u32 opcode) /* CMP */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(4);
}
void xop_b0b9_0(uae_u32 opcode) /* CMP */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(6);
}
void xop_b0ba_0(uae_u32 opcode) /* CMP */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(4);
}
void xop_b0bb_0(uae_u32 opcode) /* CMP */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}}}
void xop_b0bc_0(uae_u32 opcode) /* CMP */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xget_ilong(2);
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}xm68k_incpc(6);
}
void xop_b0c0_0(uae_u32 opcode) /* CMPA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uae_s32 dst = xm68k_areg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}xm68k_incpc(2);
}
void xop_b0c8_0(uae_u32 opcode) /* CMPA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xm68k_areg(srcreg);
{	uae_s32 dst = xm68k_areg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}xm68k_incpc(2);
}
void xop_b0d0_0(uae_u32 opcode) /* CMPA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(2);
}
void xop_b0d8_0(uae_u32 opcode) /* CMPA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
	xm68k_areg(srcreg) += 2;
{	uae_s32 dst = xm68k_areg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(2);
}
void xop_b0e0_0(uae_u32 opcode) /* CMPA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - 2;
{	uae_s16 src = xget_word(srca);
	xm68k_areg (srcreg) = srca;
{	uae_s32 dst = xm68k_areg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(2);
}
void xop_b0e8_0(uae_u32 opcode) /* CMPA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(4);
}
void xop_b0f0_0(uae_u32 opcode) /* CMPA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s16 src = xget_word(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}}}
void xop_b0f8_0(uae_u32 opcode) /* CMPA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(4);
}
void xop_b0f9_0(uae_u32 opcode) /* CMPA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s16 src = xget_word(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(6);
}
void xop_b0fa_0(uae_u32 opcode) /* CMPA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(4);
}
void xop_b0fb_0(uae_u32 opcode) /* CMPA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s16 src = xget_word(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}}}
void xop_b0fc_0(uae_u32 opcode) /* CMPA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xget_iword(2);
{	uae_s32 dst = xm68k_areg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}xm68k_incpc(4);
}
void xop_b100_0(uae_u32 opcode) /* EOR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uae_s8 dst = xm68k_dreg(dstreg);
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((src) & 0xff);
}}}xm68k_incpc(2);
}
void xop_b108_0(uae_u32 opcode) /* CMPM */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s8 src = xget_byte(srca);
	xm68k_areg(srcreg) += xareg_byteinc[srcreg];
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg(dstreg) += xareg_byteinc[dstreg];
{{uae_u32 newv = ((uae_s8)(dst)) - ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u8)(src)) > ((uae_u8)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}}xm68k_incpc(2);
}
void xop_b110_0(uae_u32 opcode) /* EOR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_b118_0(uae_u32 opcode) /* EOR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg(dstreg) += xareg_byteinc[dstreg];
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_b120_0(uae_u32 opcode) /* EOR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) - xareg_byteinc[dstreg];
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg (dstreg) = dsta;
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_b128_0(uae_u32 opcode) /* EOR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 dst = xget_byte(dsta);
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_b130_0(uae_u32 opcode) /* EOR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{xm68k_incpc(2);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s8 dst = xget_byte(dsta);
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}}}
void xop_b138_0(uae_u32 opcode) /* EOR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 dst = xget_byte(dsta);
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_b139_0(uae_u32 opcode) /* EOR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xget_ilong(2);
{	uae_s8 dst = xget_byte(dsta);
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_b140_0(uae_u32 opcode) /* EOR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uae_s16 dst = xm68k_dreg(dstreg);
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((src) & 0xffff);
}}}xm68k_incpc(2);
}
void xop_b148_0(uae_u32 opcode) /* CMPM */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
	xm68k_areg(srcreg) += 2;
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s16 dst = xget_word(dsta);
	xm68k_areg(dstreg) += 2;
{{uae_u32 newv = ((uae_s16)(dst)) - ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u16)(src)) > ((uae_u16)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}}xm68k_incpc(2);
}
void xop_b150_0(uae_u32 opcode) /* EOR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s16 dst = xget_word(dsta);
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_b158_0(uae_u32 opcode) /* EOR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s16 dst = xget_word(dsta);
	xm68k_areg(dstreg) += 2;
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_b160_0(uae_u32 opcode) /* EOR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) - 2;
{	uae_s16 dst = xget_word(dsta);
	xm68k_areg (dstreg) = dsta;
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_b168_0(uae_u32 opcode) /* EOR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 dst = xget_word(dsta);
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_b170_0(uae_u32 opcode) /* EOR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{xm68k_incpc(2);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s16 dst = xget_word(dsta);
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}}}
void xop_b178_0(uae_u32 opcode) /* EOR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 dst = xget_word(dsta);
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_b179_0(uae_u32 opcode) /* EOR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xget_ilong(2);
{	uae_s16 dst = xget_word(dsta);
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_b180_0(uae_u32 opcode) /* EOR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uae_s32 dst = xm68k_dreg(dstreg);
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xm68k_dreg(dstreg) = (src);
}}}xm68k_incpc(2);
}
void xop_b188_0(uae_u32 opcode) /* CMPM */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
	xm68k_areg(srcreg) += 4;
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s32 dst = xget_long(dsta);
	xm68k_areg(dstreg) += 4;
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}}xm68k_incpc(2);
}
void xop_b190_0(uae_u32 opcode) /* EOR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s32 dst = xget_long(dsta);
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_b198_0(uae_u32 opcode) /* EOR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s32 dst = xget_long(dsta);
	xm68k_areg(dstreg) += 4;
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_b1a0_0(uae_u32 opcode) /* EOR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) - 4;
{	uae_s32 dst = xget_long(dsta);
	xm68k_areg (dstreg) = dsta;
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_b1a8_0(uae_u32 opcode) /* EOR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 dst = xget_long(dsta);
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_b1b0_0(uae_u32 opcode) /* EOR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{xm68k_incpc(2);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s32 dst = xget_long(dsta);
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}}}
void xop_b1b8_0(uae_u32 opcode) /* EOR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 dst = xget_long(dsta);
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_b1b9_0(uae_u32 opcode) /* EOR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xget_ilong(2);
{	uae_s32 dst = xget_long(dsta);
	src ^= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_b1c0_0(uae_u32 opcode) /* CMPA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uae_s32 dst = xm68k_areg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}xm68k_incpc(2);
}
void xop_b1c8_0(uae_u32 opcode) /* CMPA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xm68k_areg(srcreg);
{	uae_s32 dst = xm68k_areg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}xm68k_incpc(2);
}
void xop_b1d0_0(uae_u32 opcode) /* CMPA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(2);
}
void xop_b1d8_0(uae_u32 opcode) /* CMPA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
	xm68k_areg(srcreg) += 4;
{	uae_s32 dst = xm68k_areg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(2);
}
void xop_b1e0_0(uae_u32 opcode) /* CMPA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - 4;
{	uae_s32 src = xget_long(srca);
	xm68k_areg (srcreg) = srca;
{	uae_s32 dst = xm68k_areg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(2);
}
void xop_b1e8_0(uae_u32 opcode) /* CMPA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(4);
}
void xop_b1f0_0(uae_u32 opcode) /* CMPA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}}}
void xop_b1f8_0(uae_u32 opcode) /* CMPA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(4);
}
void xop_b1f9_0(uae_u32 opcode) /* CMPA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(6);
}
void xop_b1fa_0(uae_u32 opcode) /* CMPA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}xm68k_incpc(4);
}
void xop_b1fb_0(uae_u32 opcode) /* CMPA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}}}}
void xop_b1fc_0(uae_u32 opcode) /* CMPA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xget_ilong(2);
{	uae_s32 dst = xm68k_areg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) - ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs != flgo) && (flgn != flgo));
	XSET_CFLG (((uae_u32)(src)) > ((uae_u32)(dst)));
	XSET_NFLG (flgn != 0);
}}}}}}xm68k_incpc(6);
}
void xop_c000_0(uae_u32 opcode) /* AND */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uae_s8 dst = xm68k_dreg(dstreg);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((src) & 0xff);
}}}xm68k_incpc(2);
}
void xop_c010_0(uae_u32 opcode) /* AND */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s8 src = xget_byte(srca);
{	uae_s8 dst = xm68k_dreg(dstreg);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((src) & 0xff);
}}}}xm68k_incpc(2);
}
void xop_c018_0(uae_u32 opcode) /* AND */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s8 src = xget_byte(srca);
	xm68k_areg(srcreg) += xareg_byteinc[srcreg];
{	uae_s8 dst = xm68k_dreg(dstreg);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((src) & 0xff);
}}}}xm68k_incpc(2);
}
void xop_c020_0(uae_u32 opcode) /* AND */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - xareg_byteinc[srcreg];
{	uae_s8 src = xget_byte(srca);
	xm68k_areg (srcreg) = srca;
{	uae_s8 dst = xm68k_dreg(dstreg);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((src) & 0xff);
}}}}xm68k_incpc(2);
}
void xop_c028_0(uae_u32 opcode) /* AND */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	uae_s8 dst = xm68k_dreg(dstreg);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((src) & 0xff);
}}}}xm68k_incpc(4);
}
void xop_c030_0(uae_u32 opcode) /* AND */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s8 src = xget_byte(srca);
{	uae_s8 dst = xm68k_dreg(dstreg);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((src) & 0xff);
}}}}}}
void xop_c038_0(uae_u32 opcode) /* AND */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	uae_s8 dst = xm68k_dreg(dstreg);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((src) & 0xff);
}}}}xm68k_incpc(4);
}
void xop_c039_0(uae_u32 opcode) /* AND */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s8 src = xget_byte(srca);
{	uae_s8 dst = xm68k_dreg(dstreg);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((src) & 0xff);
}}}}xm68k_incpc(6);
}
void xop_c03a_0(uae_u32 opcode) /* AND */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	uae_s8 dst = xm68k_dreg(dstreg);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((src) & 0xff);
}}}}xm68k_incpc(4);
}
void xop_c03b_0(uae_u32 opcode) /* AND */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s8 src = xget_byte(srca);
{	uae_s8 dst = xm68k_dreg(dstreg);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((src) & 0xff);
}}}}}}
void xop_c03c_0(uae_u32 opcode) /* AND */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = xget_ibyte(2);
{	uae_s8 dst = xm68k_dreg(dstreg);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((src) & 0xff);
}}}xm68k_incpc(4);
}
void xop_c040_0(uae_u32 opcode) /* AND */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uae_s16 dst = xm68k_dreg(dstreg);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((src) & 0xffff);
}}}xm68k_incpc(2);
}
void xop_c050_0(uae_u32 opcode) /* AND */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}xm68k_incpc(2);
}
void xop_c058_0(uae_u32 opcode) /* AND */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
	xm68k_areg(srcreg) += 2;
{	uae_s16 dst = xm68k_dreg(dstreg);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}xm68k_incpc(2);
}
void xop_c060_0(uae_u32 opcode) /* AND */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - 2;
{	uae_s16 src = xget_word(srca);
	xm68k_areg (srcreg) = srca;
{	uae_s16 dst = xm68k_dreg(dstreg);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}xm68k_incpc(2);
}
void xop_c068_0(uae_u32 opcode) /* AND */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}xm68k_incpc(4);
}
void xop_c070_0(uae_u32 opcode) /* AND */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}}}
void xop_c078_0(uae_u32 opcode) /* AND */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}xm68k_incpc(4);
}
void xop_c079_0(uae_u32 opcode) /* AND */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}xm68k_incpc(6);
}
void xop_c07a_0(uae_u32 opcode) /* AND */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}xm68k_incpc(4);
}
void xop_c07b_0(uae_u32 opcode) /* AND */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((src) & 0xffff);
}}}}}}
void xop_c07c_0(uae_u32 opcode) /* AND */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xget_iword(2);
{	uae_s16 dst = xm68k_dreg(dstreg);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((src) & 0xffff);
}}}xm68k_incpc(4);
}
void xop_c080_0(uae_u32 opcode) /* AND */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uae_s32 dst = xm68k_dreg(dstreg);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xm68k_dreg(dstreg) = (src);
}}}xm68k_incpc(2);
}
void xop_c090_0(uae_u32 opcode) /* AND */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xm68k_dreg(dstreg) = (src);
}}}}xm68k_incpc(2);
}
void xop_c098_0(uae_u32 opcode) /* AND */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
	xm68k_areg(srcreg) += 4;
{	uae_s32 dst = xm68k_dreg(dstreg);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xm68k_dreg(dstreg) = (src);
}}}}xm68k_incpc(2);
}
void xop_c0a0_0(uae_u32 opcode) /* AND */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - 4;
{	uae_s32 src = xget_long(srca);
	xm68k_areg (srcreg) = srca;
{	uae_s32 dst = xm68k_dreg(dstreg);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xm68k_dreg(dstreg) = (src);
}}}}xm68k_incpc(2);
}
void xop_c0a8_0(uae_u32 opcode) /* AND */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xm68k_dreg(dstreg) = (src);
}}}}xm68k_incpc(4);
}
void xop_c0b0_0(uae_u32 opcode) /* AND */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xm68k_dreg(dstreg) = (src);
}}}}}}
void xop_c0b8_0(uae_u32 opcode) /* AND */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xm68k_dreg(dstreg) = (src);
}}}}xm68k_incpc(4);
}
void xop_c0b9_0(uae_u32 opcode) /* AND */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xm68k_dreg(dstreg) = (src);
}}}}xm68k_incpc(6);
}
void xop_c0ba_0(uae_u32 opcode) /* AND */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xm68k_dreg(dstreg) = (src);
}}}}xm68k_incpc(4);
}
void xop_c0bb_0(uae_u32 opcode) /* AND */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xm68k_dreg(dstreg) = (src);
}}}}}}
void xop_c0bc_0(uae_u32 opcode) /* AND */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xget_ilong(2);
{	uae_s32 dst = xm68k_dreg(dstreg);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xm68k_dreg(dstreg) = (src);
}}}xm68k_incpc(6);
}
void xop_c0c0_0(uae_u32 opcode) /* MULU */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uae_s16 dst = xm68k_dreg(dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_NFLG (((uae_s32)(newv)) < 0);
	xm68k_dreg(dstreg) = (newv);
}}}}xm68k_incpc(2);
}
void xop_c0d0_0(uae_u32 opcode) /* MULU */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_NFLG (((uae_s32)(newv)) < 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}xm68k_incpc(2);
}
void xop_c0d8_0(uae_u32 opcode) /* MULU */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
	xm68k_areg(srcreg) += 2;
{	uae_s16 dst = xm68k_dreg(dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_NFLG (((uae_s32)(newv)) < 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}xm68k_incpc(2);
}
void xop_c0e0_0(uae_u32 opcode) /* MULU */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - 2;
{	uae_s16 src = xget_word(srca);
	xm68k_areg (srcreg) = srca;
{	uae_s16 dst = xm68k_dreg(dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_NFLG (((uae_s32)(newv)) < 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}xm68k_incpc(2);
}
void xop_c0e8_0(uae_u32 opcode) /* MULU */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_NFLG (((uae_s32)(newv)) < 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}xm68k_incpc(4);
}
void xop_c0f0_0(uae_u32 opcode) /* MULU */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_NFLG (((uae_s32)(newv)) < 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}}}
void xop_c0f8_0(uae_u32 opcode) /* MULU */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_NFLG (((uae_s32)(newv)) < 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}xm68k_incpc(4);
}
void xop_c0f9_0(uae_u32 opcode) /* MULU */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_NFLG (((uae_s32)(newv)) < 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}xm68k_incpc(6);
}
void xop_c0fa_0(uae_u32 opcode) /* MULU */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_NFLG (((uae_s32)(newv)) < 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}xm68k_incpc(4);
}
void xop_c0fb_0(uae_u32 opcode) /* MULU */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_NFLG (((uae_s32)(newv)) < 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}}}
void xop_c0fc_0(uae_u32 opcode) /* MULU */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xget_iword(2);
{	uae_s16 dst = xm68k_dreg(dstreg);
{	uae_u32 newv = (uae_u32)(uae_u16)dst * (uae_u32)(uae_u16)src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_NFLG (((uae_s32)(newv)) < 0);
	xm68k_dreg(dstreg) = (newv);
}}}}xm68k_incpc(4);
}
void xop_c100_0(uae_u32 opcode) /* ABCD */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uae_s8 dst = xm68k_dreg(dstreg);
{	uae_u16 newv_lo = (src & 0xF) + (dst & 0xF) + (XGET_XFLG ? 1 : 0);
	uae_u16 newv_hi = (src & 0xF0) + (dst & 0xF0);
	uae_u16 newv, tmp_newv;
	int cflg;
	newv = tmp_newv = newv_hi + newv_lo;	if (newv_lo > 9) { newv += 6; }
	cflg = (newv & 0x3F0) > 0x90;
	if (cflg) newv += 0x60;
	XSET_CFLG (cflg);
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s8)(newv)) == 0));
	XSET_NFLG (((uae_s8)(newv)) < 0);
	XSET_VFLG ((tmp_newv & 0x80) == 0 && (newv & 0x80) != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((newv) & 0xff);
}}}}xm68k_incpc(2);
}
void xop_c108_0(uae_u32 opcode) /* ABCD */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - xareg_byteinc[srcreg];
{	uae_s8 src = xget_byte(srca);
	xm68k_areg (srcreg) = srca;
{	uaecptr dsta = xm68k_areg(dstreg) - xareg_byteinc[dstreg];
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg (dstreg) = dsta;
{	uae_u16 newv_lo = (src & 0xF) + (dst & 0xF) + (XGET_XFLG ? 1 : 0);
	uae_u16 newv_hi = (src & 0xF0) + (dst & 0xF0);
	uae_u16 newv, tmp_newv;
	int cflg;
	newv = tmp_newv = newv_hi + newv_lo;	if (newv_lo > 9) { newv += 6; }
	cflg = (newv & 0x3F0) > 0x90;
	if (cflg) newv += 0x60;
	XSET_CFLG (cflg);
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s8)(newv)) == 0));
	XSET_NFLG (((uae_s8)(newv)) < 0);
	XSET_VFLG ((tmp_newv & 0x80) == 0 && (newv & 0x80) != 0);
	xput_byte(dsta,newv);
}}}}}}xm68k_incpc(2);
}
void xop_c110_0(uae_u32 opcode) /* AND */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_c118_0(uae_u32 opcode) /* AND */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg(dstreg) += xareg_byteinc[dstreg];
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_c120_0(uae_u32 opcode) /* AND */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) - xareg_byteinc[dstreg];
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg (dstreg) = dsta;
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_c128_0(uae_u32 opcode) /* AND */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 dst = xget_byte(dsta);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_c130_0(uae_u32 opcode) /* AND */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{xm68k_incpc(2);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s8 dst = xget_byte(dsta);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}}}
void xop_c138_0(uae_u32 opcode) /* AND */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 dst = xget_byte(dsta);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_c139_0(uae_u32 opcode) /* AND */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xget_ilong(2);
{	uae_s8 dst = xget_byte(dsta);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s8)(src)) == 0);
	XSET_NFLG (((uae_s8)(src)) < 0);
	xput_byte(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_c140_0(uae_u32 opcode) /* EXG */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uae_s32 dst = xm68k_dreg(dstreg);
	xm68k_dreg(srcreg) = (dst);
	xm68k_dreg(dstreg) = (src);
}}}xm68k_incpc(2);
}
void xop_c148_0(uae_u32 opcode) /* EXG */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xm68k_areg(srcreg);
{	uae_s32 dst = xm68k_areg(dstreg);
	xm68k_areg(srcreg) = (dst);
	xm68k_areg(dstreg) = (src);
}}}xm68k_incpc(2);
}
void xop_c150_0(uae_u32 opcode) /* AND */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s16 dst = xget_word(dsta);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_c158_0(uae_u32 opcode) /* AND */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s16 dst = xget_word(dsta);
	xm68k_areg(dstreg) += 2;
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_c160_0(uae_u32 opcode) /* AND */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) - 2;
{	uae_s16 dst = xget_word(dsta);
	xm68k_areg (dstreg) = dsta;
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_c168_0(uae_u32 opcode) /* AND */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 dst = xget_word(dsta);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_c170_0(uae_u32 opcode) /* AND */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{xm68k_incpc(2);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s16 dst = xget_word(dsta);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}}}
void xop_c178_0(uae_u32 opcode) /* AND */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 dst = xget_word(dsta);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_c179_0(uae_u32 opcode) /* AND */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xget_ilong(2);
{	uae_s16 dst = xget_word(dsta);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(src)) == 0);
	XSET_NFLG (((uae_s16)(src)) < 0);
	xput_word(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_c188_0(uae_u32 opcode) /* EXG */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uae_s32 dst = xm68k_areg(dstreg);
	xm68k_dreg(srcreg) = (dst);
	xm68k_areg(dstreg) = (src);
}}}xm68k_incpc(2);
}
void xop_c190_0(uae_u32 opcode) /* AND */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s32 dst = xget_long(dsta);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_c198_0(uae_u32 opcode) /* AND */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s32 dst = xget_long(dsta);
	xm68k_areg(dstreg) += 4;
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_c1a0_0(uae_u32 opcode) /* AND */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) - 4;
{	uae_s32 dst = xget_long(dsta);
	xm68k_areg (dstreg) = dsta;
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(2);
}
void xop_c1a8_0(uae_u32 opcode) /* AND */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 dst = xget_long(dsta);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_c1b0_0(uae_u32 opcode) /* AND */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{xm68k_incpc(2);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s32 dst = xget_long(dsta);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}}}
void xop_c1b8_0(uae_u32 opcode) /* AND */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 dst = xget_long(dsta);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(4);
}
void xop_c1b9_0(uae_u32 opcode) /* AND */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xget_ilong(2);
{	uae_s32 dst = xget_long(dsta);
	src &= dst;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(src)) == 0);
	XSET_NFLG (((uae_s32)(src)) < 0);
	xput_long(dsta,src);
}}}}xm68k_incpc(6);
}
void xop_c1c0_0(uae_u32 opcode) /* MULS */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uae_s16 dst = xm68k_dreg(dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_NFLG (((uae_s32)(newv)) < 0);
	xm68k_dreg(dstreg) = (newv);
}}}}xm68k_incpc(2);
}
void xop_c1d0_0(uae_u32 opcode) /* MULS */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_NFLG (((uae_s32)(newv)) < 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}xm68k_incpc(2);
}
void xop_c1d8_0(uae_u32 opcode) /* MULS */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
	xm68k_areg(srcreg) += 2;
{	uae_s16 dst = xm68k_dreg(dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_NFLG (((uae_s32)(newv)) < 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}xm68k_incpc(2);
}
void xop_c1e0_0(uae_u32 opcode) /* MULS */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - 2;
{	uae_s16 src = xget_word(srca);
	xm68k_areg (srcreg) = srca;
{	uae_s16 dst = xm68k_dreg(dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_NFLG (((uae_s32)(newv)) < 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}xm68k_incpc(2);
}
void xop_c1e8_0(uae_u32 opcode) /* MULS */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_NFLG (((uae_s32)(newv)) < 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}xm68k_incpc(4);
}
void xop_c1f0_0(uae_u32 opcode) /* MULS */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_NFLG (((uae_s32)(newv)) < 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}}}
void xop_c1f8_0(uae_u32 opcode) /* MULS */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_NFLG (((uae_s32)(newv)) < 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}xm68k_incpc(4);
}
void xop_c1f9_0(uae_u32 opcode) /* MULS */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_NFLG (((uae_s32)(newv)) < 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}xm68k_incpc(6);
}
void xop_c1fa_0(uae_u32 opcode) /* MULS */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_NFLG (((uae_s32)(newv)) < 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}xm68k_incpc(4);
}
void xop_c1fb_0(uae_u32 opcode) /* MULS */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_NFLG (((uae_s32)(newv)) < 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}}}
void xop_c1fc_0(uae_u32 opcode) /* MULS */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xget_iword(2);
{	uae_s16 dst = xm68k_dreg(dstreg);
{	uae_u32 newv = (uae_s32)(uae_s16)dst * (uae_s32)(uae_s16)src;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_NFLG (((uae_s32)(newv)) < 0);
	xm68k_dreg(dstreg) = (newv);
}}}}xm68k_incpc(4);
}
void xop_d000_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uae_s8 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}xm68k_incpc(2);
}
void xop_d010_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s8 src = xget_byte(srca);
{	uae_s8 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}xm68k_incpc(2);
}
void xop_d018_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s8 src = xget_byte(srca);
	xm68k_areg(srcreg) += xareg_byteinc[srcreg];
{	uae_s8 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}xm68k_incpc(2);
}
void xop_d020_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - xareg_byteinc[srcreg];
{	uae_s8 src = xget_byte(srca);
	xm68k_areg (srcreg) = srca;
{	uae_s8 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}xm68k_incpc(2);
}
void xop_d028_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	uae_s8 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}xm68k_incpc(4);
}
void xop_d030_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s8 src = xget_byte(srca);
{	uae_s8 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}}}
void xop_d038_0(uae_u32 opcode) /* ADD */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	uae_s8 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}xm68k_incpc(4);
}
void xop_d039_0(uae_u32 opcode) /* ADD */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s8 src = xget_byte(srca);
{	uae_s8 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}xm68k_incpc(6);
}
void xop_d03a_0(uae_u32 opcode) /* ADD */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 src = xget_byte(srca);
{	uae_s8 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}xm68k_incpc(4);
}
void xop_d03b_0(uae_u32 opcode) /* ADD */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s8 src = xget_byte(srca);
{	uae_s8 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}}}}
void xop_d03c_0(uae_u32 opcode) /* ADD */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = xget_ibyte(2);
{	uae_s8 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}}xm68k_incpc(4);
}
void xop_d040_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}xm68k_incpc(2);
}
void xop_d048_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xm68k_areg(srcreg);
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}xm68k_incpc(2);
}
void xop_d050_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}xm68k_incpc(2);
}
void xop_d058_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
	xm68k_areg(srcreg) += 2;
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}xm68k_incpc(2);
}
void xop_d060_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - 2;
{	uae_s16 src = xget_word(srca);
	xm68k_areg (srcreg) = srca;
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}xm68k_incpc(2);
}
void xop_d068_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}xm68k_incpc(4);
}
void xop_d070_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}}}
void xop_d078_0(uae_u32 opcode) /* ADD */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}xm68k_incpc(4);
}
void xop_d079_0(uae_u32 opcode) /* ADD */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}xm68k_incpc(6);
}
void xop_d07a_0(uae_u32 opcode) /* ADD */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}xm68k_incpc(4);
}
void xop_d07b_0(uae_u32 opcode) /* ADD */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s16 src = xget_word(srca);
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}}}}
void xop_d07c_0(uae_u32 opcode) /* ADD */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xget_iword(2);
{	uae_s16 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}}xm68k_incpc(4);
}
void xop_d080_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}}xm68k_incpc(2);
}
void xop_d088_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xm68k_areg(srcreg);
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}}xm68k_incpc(2);
}
void xop_d090_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}}}xm68k_incpc(2);
}
void xop_d098_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
	xm68k_areg(srcreg) += 4;
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}}}xm68k_incpc(2);
}
void xop_d0a0_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - 4;
{	uae_s32 src = xget_long(srca);
	xm68k_areg (srcreg) = srca;
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}}}xm68k_incpc(2);
}
void xop_d0a8_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}}}xm68k_incpc(4);
}
void xop_d0b0_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}}}}}
void xop_d0b8_0(uae_u32 opcode) /* ADD */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}}}xm68k_incpc(4);
}
void xop_d0b9_0(uae_u32 opcode) /* ADD */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}}}xm68k_incpc(6);
}
void xop_d0ba_0(uae_u32 opcode) /* ADD */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}}}xm68k_incpc(4);
}
void xop_d0bb_0(uae_u32 opcode) /* ADD */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}}}}}
void xop_d0bc_0(uae_u32 opcode) /* ADD */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xget_ilong(2);
{	uae_s32 dst = xm68k_dreg(dstreg);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}}xm68k_incpc(6);
}
void xop_d0c0_0(uae_u32 opcode) /* ADDA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst + src;
	xm68k_areg(dstreg) = (newv);
}}}}xm68k_incpc(2);
}
void xop_d0c8_0(uae_u32 opcode) /* ADDA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xm68k_areg(srcreg);
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst + src;
	xm68k_areg(dstreg) = (newv);
}}}}xm68k_incpc(2);
}
void xop_d0d0_0(uae_u32 opcode) /* ADDA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst + src;
	xm68k_areg(dstreg) = (newv);
}}}}}xm68k_incpc(2);
}
void xop_d0d8_0(uae_u32 opcode) /* ADDA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s16 src = xget_word(srca);
	xm68k_areg(srcreg) += 2;
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst + src;
	xm68k_areg(dstreg) = (newv);
}}}}}xm68k_incpc(2);
}
void xop_d0e0_0(uae_u32 opcode) /* ADDA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - 2;
{	uae_s16 src = xget_word(srca);
	xm68k_areg (srcreg) = srca;
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst + src;
	xm68k_areg(dstreg) = (newv);
}}}}}xm68k_incpc(2);
}
void xop_d0e8_0(uae_u32 opcode) /* ADDA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst + src;
	xm68k_areg(dstreg) = (newv);
}}}}}xm68k_incpc(4);
}
void xop_d0f0_0(uae_u32 opcode) /* ADDA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s16 src = xget_word(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst + src;
	xm68k_areg(dstreg) = (newv);
}}}}}}}
void xop_d0f8_0(uae_u32 opcode) /* ADDA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst + src;
	xm68k_areg(dstreg) = (newv);
}}}}}xm68k_incpc(4);
}
void xop_d0f9_0(uae_u32 opcode) /* ADDA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s16 src = xget_word(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst + src;
	xm68k_areg(dstreg) = (newv);
}}}}}xm68k_incpc(6);
}
void xop_d0fa_0(uae_u32 opcode) /* ADDA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 src = xget_word(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst + src;
	xm68k_areg(dstreg) = (newv);
}}}}}xm68k_incpc(4);
}
void xop_d0fb_0(uae_u32 opcode) /* ADDA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s16 src = xget_word(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst + src;
	xm68k_areg(dstreg) = (newv);
}}}}}}}
void xop_d0fc_0(uae_u32 opcode) /* ADDA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xget_iword(2);
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst + src;
	xm68k_areg(dstreg) = (newv);
}}}}xm68k_incpc(4);
}
void xop_d100_0(uae_u32 opcode) /* ADDX */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uae_s8 dst = xm68k_dreg(dstreg);
{	uae_u32 newv = dst + src + (XGET_XFLG ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (flgs ^ ((flgs ^ flgo) & (flgo ^ flgn)));
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s8)(newv)) == 0));
	XSET_NFLG (((uae_s8)(newv)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((newv) & 0xff);
}}}}}xm68k_incpc(2);
}
void xop_d108_0(uae_u32 opcode) /* ADDX */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - xareg_byteinc[srcreg];
{	uae_s8 src = xget_byte(srca);
	xm68k_areg (srcreg) = srca;
{	uaecptr dsta = xm68k_areg(dstreg) - xareg_byteinc[dstreg];
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg (dstreg) = dsta;
{	uae_u32 newv = dst + src + (XGET_XFLG ? 1 : 0);
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (flgs ^ ((flgs ^ flgo) & (flgo ^ flgn)));
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s8)(newv)) == 0));
	XSET_NFLG (((uae_s8)(newv)) < 0);
	xput_byte(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_d110_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_d118_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg(dstreg) += xareg_byteinc[dstreg];
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_d120_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) - xareg_byteinc[dstreg];
{	uae_s8 dst = xget_byte(dsta);
	xm68k_areg (dstreg) = dsta;
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_d128_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 dst = xget_byte(dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}xm68k_incpc(4);
}
void xop_d130_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 src = xm68k_dreg(srcreg);
{xm68k_incpc(2);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s8 dst = xget_byte(dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}}}
void xop_d138_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s8 dst = xget_byte(dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}xm68k_incpc(4);
}
void xop_d139_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s8 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xget_ilong(2);
{	uae_s8 dst = xget_byte(dsta);
{{uae_u32 newv = ((uae_s8)(dst)) + ((uae_s8)(src));
{	int flgs = ((uae_s8)(src)) < 0;
	int flgo = ((uae_s8)(dst)) < 0;
	int flgn = ((uae_s8)(newv)) < 0;
	XSET_ZFLG (((uae_s8)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u8)(~dst)) < ((uae_u8)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_byte(dsta,newv);
}}}}}}}xm68k_incpc(6);
}
void xop_d140_0(uae_u32 opcode) /* ADDX */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uae_s16 dst = xm68k_dreg(dstreg);
{	uae_u32 newv = dst + src + (XGET_XFLG ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (flgs ^ ((flgs ^ flgo) & (flgo ^ flgn)));
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s16)(newv)) == 0));
	XSET_NFLG (((uae_s16)(newv)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((newv) & 0xffff);
}}}}}xm68k_incpc(2);
}
void xop_d148_0(uae_u32 opcode) /* ADDX */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - 2;
{	uae_s16 src = xget_word(srca);
	xm68k_areg (srcreg) = srca;
{	uaecptr dsta = xm68k_areg(dstreg) - 2;
{	uae_s16 dst = xget_word(dsta);
	xm68k_areg (dstreg) = dsta;
{	uae_u32 newv = dst + src + (XGET_XFLG ? 1 : 0);
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (flgs ^ ((flgs ^ flgo) & (flgo ^ flgn)));
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s16)(newv)) == 0));
	XSET_NFLG (((uae_s16)(newv)) < 0);
	xput_word(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_d150_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s16 dst = xget_word(dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_d158_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s16 dst = xget_word(dsta);
	xm68k_areg(dstreg) += 2;
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_d160_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) - 2;
{	uae_s16 dst = xget_word(dsta);
	xm68k_areg (dstreg) = dsta;
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_d168_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 dst = xget_word(dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}xm68k_incpc(4);
}
void xop_d170_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 src = xm68k_dreg(srcreg);
{xm68k_incpc(2);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s16 dst = xget_word(dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}}}
void xop_d178_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 dst = xget_word(dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}xm68k_incpc(4);
}
void xop_d179_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s16 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xget_ilong(2);
{	uae_s16 dst = xget_word(dsta);
{{uae_u32 newv = ((uae_s16)(dst)) + ((uae_s16)(src));
{	int flgs = ((uae_s16)(src)) < 0;
	int flgo = ((uae_s16)(dst)) < 0;
	int flgn = ((uae_s16)(newv)) < 0;
	XSET_ZFLG (((uae_s16)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u16)(~dst)) < ((uae_u16)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_word(dsta,newv);
}}}}}}}xm68k_incpc(6);
}
void xop_d180_0(uae_u32 opcode) /* ADDX */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uae_s32 dst = xm68k_dreg(dstreg);
{	uae_u32 newv = dst + src + (XGET_XFLG ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (flgs ^ ((flgs ^ flgo) & (flgo ^ flgn)));
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s32)(newv)) == 0));
	XSET_NFLG (((uae_s32)(newv)) < 0);
	xm68k_dreg(dstreg) = (newv);
}}}}}xm68k_incpc(2);
}
void xop_d188_0(uae_u32 opcode) /* ADDX */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - 4;
{	uae_s32 src = xget_long(srca);
	xm68k_areg (srcreg) = srca;
{	uaecptr dsta = xm68k_areg(dstreg) - 4;
{	uae_s32 dst = xget_long(dsta);
	xm68k_areg (dstreg) = dsta;
{	uae_u32 newv = dst + src + (XGET_XFLG ? 1 : 0);
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (flgs ^ ((flgs ^ flgo) & (flgo ^ flgn)));
	XCOPY_CARRY;
	XSET_ZFLG (XGET_ZFLG & (((uae_s32)(newv)) == 0));
	XSET_NFLG (((uae_s32)(newv)) < 0);
	xput_long(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_d190_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s32 dst = xget_long(dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_d198_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg);
{	uae_s32 dst = xget_long(dsta);
	xm68k_areg(dstreg) += 4;
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_d1a0_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) - 4;
{	uae_s32 dst = xget_long(dsta);
	xm68k_areg (dstreg) = dsta;
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}xm68k_incpc(2);
}
void xop_d1a8_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xm68k_areg(dstreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 dst = xget_long(dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}xm68k_incpc(4);
}
void xop_d1b0_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{xm68k_incpc(2);
{	uaecptr dsta = xget_disp_ea_020(xm68k_areg(dstreg), xnext_iword());
{	uae_s32 dst = xget_long(dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}}}
void xop_d1b8_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uaecptr dsta = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 dst = xget_long(dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}xm68k_incpc(4);
}
void xop_d1b9_0(uae_u32 opcode) /* ADD */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uaecptr dsta = xget_ilong(2);
{	uae_s32 dst = xget_long(dsta);
{{uae_u32 newv = ((uae_s32)(dst)) + ((uae_s32)(src));
{	int flgs = ((uae_s32)(src)) < 0;
	int flgo = ((uae_s32)(dst)) < 0;
	int flgn = ((uae_s32)(newv)) < 0;
	XSET_ZFLG (((uae_s32)(newv)) == 0);
	XSET_VFLG ((flgs ^ flgn) & (flgo ^ flgn));
	XSET_CFLG (((uae_u32)(~dst)) < ((uae_u32)(src)));
	XCOPY_CARRY;
	XSET_NFLG (flgn != 0);
	xput_long(dsta,newv);
}}}}}}}xm68k_incpc(6);
}
void xop_d1c0_0(uae_u32 opcode) /* ADDA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xm68k_dreg(srcreg);
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst + src;
	xm68k_areg(dstreg) = (newv);
}}}}xm68k_incpc(2);
}
void xop_d1c8_0(uae_u32 opcode) /* ADDA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xm68k_areg(srcreg);
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst + src;
	xm68k_areg(dstreg) = (newv);
}}}}xm68k_incpc(2);
}
void xop_d1d0_0(uae_u32 opcode) /* ADDA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst + src;
	xm68k_areg(dstreg) = (newv);
}}}}}xm68k_incpc(2);
}
void xop_d1d8_0(uae_u32 opcode) /* ADDA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg);
{	uae_s32 src = xget_long(srca);
	xm68k_areg(srcreg) += 4;
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst + src;
	xm68k_areg(dstreg) = (newv);
}}}}}xm68k_incpc(2);
}
void xop_d1e0_0(uae_u32 opcode) /* ADDA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) - 4;
{	uae_s32 src = xget_long(srca);
	xm68k_areg (srcreg) = srca;
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst + src;
	xm68k_areg(dstreg) = (newv);
}}}}}xm68k_incpc(2);
}
void xop_d1e8_0(uae_u32 opcode) /* ADDA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst + src;
	xm68k_areg(dstreg) = (newv);
}}}}}xm68k_incpc(4);
}
void xop_d1f0_0(uae_u32 opcode) /* ADDA */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr srca = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst + src;
	xm68k_areg(dstreg) = (newv);
}}}}}}}
void xop_d1f8_0(uae_u32 opcode) /* ADDA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst + src;
	xm68k_areg(dstreg) = (newv);
}}}}}xm68k_incpc(4);
}
void xop_d1f9_0(uae_u32 opcode) /* ADDA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xget_ilong(2);
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst + src;
	xm68k_areg(dstreg) = (newv);
}}}}}xm68k_incpc(6);
}
void xop_d1fa_0(uae_u32 opcode) /* ADDA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uaecptr srca = xm68k_getpc () + 2;
	srca += (uae_s32)(uae_s16)xget_iword(2);
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst + src;
	xm68k_areg(dstreg) = (newv);
}}}}}xm68k_incpc(4);
}
void xop_d1fb_0(uae_u32 opcode) /* ADDA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{xm68k_incpc(2);
{	uaecptr tmppc = xm68k_getpc();
	uaecptr srca = xget_disp_ea_020(tmppc, xnext_iword());
{	uae_s32 src = xget_long(srca);
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst + src;
	xm68k_areg(dstreg) = (newv);
}}}}}}}
void xop_d1fc_0(uae_u32 opcode) /* ADDA */
{
	uae_u32 dstreg = (opcode >> 9) & 7;
{{	uae_s32 src = xget_ilong(2);
{	uae_s32 dst = xm68k_areg(dstreg);
{	uae_u32 newv = dst + src;
	xm68k_areg(dstreg) = (newv);
}}}}xm68k_incpc(6);
}
void xop_e000_0(uae_u32 opcode) /* ASR */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s8 data = xm68k_dreg(dstreg);
{	uae_u32 val = (uae_u8)data;
	uae_u32 sign = (0x80 & val) >> 7;
	cnt &= 63;
	XCLEAR_CZNV;
	if (cnt >= 8) {
		val = 0xff & (uae_u32)-sign;
		XSET_CFLG (sign);
	XCOPY_CARRY;
	} else {
		val >>= cnt - 1;
		XSET_CFLG (val & 1);
	XCOPY_CARRY;
		val >>= 1;
		val |= (0xff << (8 - cnt)) & (uae_u32)-sign;
		val &= 0xff;
	}
	XSET_ZFLG (((uae_s8)(val)) == 0);
	XSET_NFLG (((uae_s8)(val)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((val) & 0xff);
}}}}xm68k_incpc(2);
}
void xop_e008_0(uae_u32 opcode) /* LSR */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s8 data = xm68k_dreg(dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	XCLEAR_CZNV;
	if (cnt >= 8) {
		XSET_CFLG ((cnt == 8) & (val >> 7));
	XCOPY_CARRY;
		val = 0;
	} else {
		val >>= cnt - 1;
		XSET_CFLG (val & 1);
	XCOPY_CARRY;
		val >>= 1;
	}
	XSET_ZFLG (((uae_s8)(val)) == 0);
	XSET_NFLG (((uae_s8)(val)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((val) & 0xff);
}}}}xm68k_incpc(2);
}
void xop_e010_0(uae_u32 opcode) /* ROXR */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s8 data = xm68k_dreg(dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	XCLEAR_CZNV;
{	cnt--;
	{
	uae_u32 carry;
	uae_u32 hival = (val << 1) | XGET_XFLG;
	hival <<= (7 - cnt);
	val >>= cnt;
	carry = val & 1;
	val >>= 1;
	val |= hival;
	XSET_XFLG (carry);
	val &= 0xff;
	} }
	XSET_CFLG (XGET_XFLG);
	XSET_ZFLG (((uae_s8)(val)) == 0);
	XSET_NFLG (((uae_s8)(val)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((val) & 0xff);
}}}}xm68k_incpc(2);
}
void xop_e018_0(uae_u32 opcode) /* ROR */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s8 data = xm68k_dreg(dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	XCLEAR_CZNV;
{	uae_u32 hival;
	cnt &= 7;
	hival = val << (8 - cnt);
	val >>= cnt;
	val |= hival;
	val &= 0xff;
	XSET_CFLG ((val & 0x80) >> 7);
	}
	XSET_ZFLG (((uae_s8)(val)) == 0);
	XSET_NFLG (((uae_s8)(val)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((val) & 0xff);
}}}}xm68k_incpc(2);
}
void xop_e020_0(uae_u32 opcode) /* ASR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 cnt = xm68k_dreg(srcreg);
{	uae_s8 data = xm68k_dreg(dstreg);
{	uae_u32 val = (uae_u8)data;
	uae_u32 sign = (0x80 & val) >> 7;
	cnt &= 63;
	XCLEAR_CZNV;
	if (cnt >= 8) {
		val = 0xff & (uae_u32)-sign;
		XSET_CFLG (sign);
	XCOPY_CARRY;
	} else if (cnt > 0) {
		val >>= cnt - 1;
		XSET_CFLG (val & 1);
	XCOPY_CARRY;
		val >>= 1;
		val |= (0xff << (8 - cnt)) & (uae_u32)-sign;
		val &= 0xff;
	}
	XSET_ZFLG (((uae_s8)(val)) == 0);
	XSET_NFLG (((uae_s8)(val)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((val) & 0xff);
}}}}xm68k_incpc(2);
}
void xop_e028_0(uae_u32 opcode) /* LSR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 cnt = xm68k_dreg(srcreg);
{	uae_s8 data = xm68k_dreg(dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	XCLEAR_CZNV;
	if (cnt >= 8) {
		XSET_CFLG ((cnt == 8) & (val >> 7));
	XCOPY_CARRY;
		val = 0;
	} else if (cnt > 0) {
		val >>= cnt - 1;
		XSET_CFLG (val & 1);
	XCOPY_CARRY;
		val >>= 1;
	}
	XSET_ZFLG (((uae_s8)(val)) == 0);
	XSET_NFLG (((uae_s8)(val)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((val) & 0xff);
}}}}xm68k_incpc(2);
}
void xop_e030_0(uae_u32 opcode) /* ROXR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 cnt = xm68k_dreg(srcreg);
{	uae_s8 data = xm68k_dreg(dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	XCLEAR_CZNV;
	if (cnt >= 36) cnt -= 36;
	if (cnt >= 18) cnt -= 18;
	if (cnt >= 9) cnt -= 9;
	if (cnt > 0) {
	cnt--;
	{
	uae_u32 carry;
	uae_u32 hival = (val << 1) | XGET_XFLG;
	hival <<= (7 - cnt);
	val >>= cnt;
	carry = val & 1;
	val >>= 1;
	val |= hival;
	XSET_XFLG (carry);
	val &= 0xff;
	} }
	XSET_CFLG (XGET_XFLG);
	XSET_ZFLG (((uae_s8)(val)) == 0);
	XSET_NFLG (((uae_s8)(val)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((val) & 0xff);
}}}}xm68k_incpc(2);
}
void xop_e038_0(uae_u32 opcode) /* ROR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 cnt = xm68k_dreg(srcreg);
{	uae_s8 data = xm68k_dreg(dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	XCLEAR_CZNV;
	if (cnt > 0) {	uae_u32 hival;
	cnt &= 7;
	hival = val << (8 - cnt);
	val >>= cnt;
	val |= hival;
	val &= 0xff;
	XSET_CFLG ((val & 0x80) >> 7);
	}
	XSET_ZFLG (((uae_s8)(val)) == 0);
	XSET_NFLG (((uae_s8)(val)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((val) & 0xff);
}}}}xm68k_incpc(2);
}
void xop_e040_0(uae_u32 opcode) /* ASR */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s16 data = xm68k_dreg(dstreg);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = (0x8000 & val) >> 15;
	cnt &= 63;
	XCLEAR_CZNV;
	if (cnt >= 16) {
		val = 0xffff & (uae_u32)-sign;
		XSET_CFLG (sign);
	XCOPY_CARRY;
	} else {
		val >>= cnt - 1;
		XSET_CFLG (val & 1);
	XCOPY_CARRY;
		val >>= 1;
		val |= (0xffff << (16 - cnt)) & (uae_u32)-sign;
		val &= 0xffff;
	}
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}xm68k_incpc(2);
}
void xop_e048_0(uae_u32 opcode) /* LSR */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s16 data = xm68k_dreg(dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	XCLEAR_CZNV;
	if (cnt >= 16) {
		XSET_CFLG ((cnt == 16) & (val >> 15));
	XCOPY_CARRY;
		val = 0;
	} else {
		val >>= cnt - 1;
		XSET_CFLG (val & 1);
	XCOPY_CARRY;
		val >>= 1;
	}
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}xm68k_incpc(2);
}
void xop_e050_0(uae_u32 opcode) /* ROXR */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s16 data = xm68k_dreg(dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	XCLEAR_CZNV;
{	cnt--;
	{
	uae_u32 carry;
	uae_u32 hival = (val << 1) | XGET_XFLG;
	hival <<= (15 - cnt);
	val >>= cnt;
	carry = val & 1;
	val >>= 1;
	val |= hival;
	XSET_XFLG (carry);
	val &= 0xffff;
	} }
	XSET_CFLG (XGET_XFLG);
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}xm68k_incpc(2);
}
void xop_e058_0(uae_u32 opcode) /* ROR */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s16 data = xm68k_dreg(dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	XCLEAR_CZNV;
{	uae_u32 hival;
	cnt &= 15;
	hival = val << (16 - cnt);
	val >>= cnt;
	val |= hival;
	val &= 0xffff;
	XSET_CFLG ((val & 0x8000) >> 15);
	}
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}xm68k_incpc(2);
}
void xop_e060_0(uae_u32 opcode) /* ASR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 cnt = xm68k_dreg(srcreg);
{	uae_s16 data = xm68k_dreg(dstreg);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = (0x8000 & val) >> 15;
	cnt &= 63;
	XCLEAR_CZNV;
	if (cnt >= 16) {
		val = 0xffff & (uae_u32)-sign;
		XSET_CFLG (sign);
	XCOPY_CARRY;
	} else if (cnt > 0) {
		val >>= cnt - 1;
		XSET_CFLG (val & 1);
	XCOPY_CARRY;
		val >>= 1;
		val |= (0xffff << (16 - cnt)) & (uae_u32)-sign;
		val &= 0xffff;
	}
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}xm68k_incpc(2);
}
void xop_e068_0(uae_u32 opcode) /* LSR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 cnt = xm68k_dreg(srcreg);
{	uae_s16 data = xm68k_dreg(dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	XCLEAR_CZNV;
	if (cnt >= 16) {
		XSET_CFLG ((cnt == 16) & (val >> 15));
	XCOPY_CARRY;
		val = 0;
	} else if (cnt > 0) {
		val >>= cnt - 1;
		XSET_CFLG (val & 1);
	XCOPY_CARRY;
		val >>= 1;
	}
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}xm68k_incpc(2);
}
void xop_e070_0(uae_u32 opcode) /* ROXR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 cnt = xm68k_dreg(srcreg);
{	uae_s16 data = xm68k_dreg(dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	XCLEAR_CZNV;
	if (cnt >= 34) cnt -= 34;
	if (cnt >= 17) cnt -= 17;
	if (cnt > 0) {
	cnt--;
	{
	uae_u32 carry;
	uae_u32 hival = (val << 1) | XGET_XFLG;
	hival <<= (15 - cnt);
	val >>= cnt;
	carry = val & 1;
	val >>= 1;
	val |= hival;
	XSET_XFLG (carry);
	val &= 0xffff;
	} }
	XSET_CFLG (XGET_XFLG);
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}xm68k_incpc(2);
}
void xop_e078_0(uae_u32 opcode) /* ROR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 cnt = xm68k_dreg(srcreg);
{	uae_s16 data = xm68k_dreg(dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	XCLEAR_CZNV;
	if (cnt > 0) {	uae_u32 hival;
	cnt &= 15;
	hival = val << (16 - cnt);
	val >>= cnt;
	val |= hival;
	val &= 0xffff;
	XSET_CFLG ((val & 0x8000) >> 15);
	}
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}xm68k_incpc(2);
}
void xop_e080_0(uae_u32 opcode) /* ASR */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s32 data = xm68k_dreg(dstreg);
{	uae_u32 val = data;
	uae_u32 sign = (0x80000000 & val) >> 31;
	cnt &= 63;
	XCLEAR_CZNV;
	if (cnt >= 32) {
		val = 0xffffffff & (uae_u32)-sign;
		XSET_CFLG (sign);
	XCOPY_CARRY;
	} else {
		val >>= cnt - 1;
		XSET_CFLG (val & 1);
	XCOPY_CARRY;
		val >>= 1;
		val |= (0xffffffff << (32 - cnt)) & (uae_u32)-sign;
		val &= 0xffffffff;
	}
	XSET_ZFLG (((uae_s32)(val)) == 0);
	XSET_NFLG (((uae_s32)(val)) < 0);
	xm68k_dreg(dstreg) = (val);
}}}}xm68k_incpc(2);
}
void xop_e088_0(uae_u32 opcode) /* LSR */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s32 data = xm68k_dreg(dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	XCLEAR_CZNV;
	if (cnt >= 32) {
		XSET_CFLG ((cnt == 32) & (val >> 31));
	XCOPY_CARRY;
		val = 0;
	} else {
		val >>= cnt - 1;
		XSET_CFLG (val & 1);
	XCOPY_CARRY;
		val >>= 1;
	}
	XSET_ZFLG (((uae_s32)(val)) == 0);
	XSET_NFLG (((uae_s32)(val)) < 0);
	xm68k_dreg(dstreg) = (val);
}}}}xm68k_incpc(2);
}
void xop_e090_0(uae_u32 opcode) /* ROXR */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s32 data = xm68k_dreg(dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	XCLEAR_CZNV;
{	cnt--;
	{
	uae_u32 carry;
	uae_u32 hival = (val << 1) | XGET_XFLG;
	hival <<= (31 - cnt);
	val >>= cnt;
	carry = val & 1;
	val >>= 1;
	val |= hival;
	XSET_XFLG (carry);
	val &= 0xffffffff;
	} }
	XSET_CFLG (XGET_XFLG);
	XSET_ZFLG (((uae_s32)(val)) == 0);
	XSET_NFLG (((uae_s32)(val)) < 0);
	xm68k_dreg(dstreg) = (val);
}}}}xm68k_incpc(2);
}
void xop_e098_0(uae_u32 opcode) /* ROR */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s32 data = xm68k_dreg(dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	XCLEAR_CZNV;
{	uae_u32 hival;
	cnt &= 31;
	hival = val << (32 - cnt);
	val >>= cnt;
	val |= hival;
	val &= 0xffffffff;
	XSET_CFLG ((val & 0x80000000) >> 31);
	}
	XSET_ZFLG (((uae_s32)(val)) == 0);
	XSET_NFLG (((uae_s32)(val)) < 0);
	xm68k_dreg(dstreg) = (val);
}}}}xm68k_incpc(2);
}
void xop_e0a0_0(uae_u32 opcode) /* ASR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 cnt = xm68k_dreg(srcreg);
{	uae_s32 data = xm68k_dreg(dstreg);
{	uae_u32 val = data;
	uae_u32 sign = (0x80000000 & val) >> 31;
	cnt &= 63;
	XCLEAR_CZNV;
	if (cnt >= 32) {
		val = 0xffffffff & (uae_u32)-sign;
		XSET_CFLG (sign);
	XCOPY_CARRY;
	} else if (cnt > 0) {
		val >>= cnt - 1;
		XSET_CFLG (val & 1);
	XCOPY_CARRY;
		val >>= 1;
		val |= (0xffffffff << (32 - cnt)) & (uae_u32)-sign;
		val &= 0xffffffff;
	}
	XSET_ZFLG (((uae_s32)(val)) == 0);
	XSET_NFLG (((uae_s32)(val)) < 0);
	xm68k_dreg(dstreg) = (val);
}}}}xm68k_incpc(2);
}
void xop_e0a8_0(uae_u32 opcode) /* LSR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 cnt = xm68k_dreg(srcreg);
{	uae_s32 data = xm68k_dreg(dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	XCLEAR_CZNV;
	if (cnt >= 32) {
		XSET_CFLG ((cnt == 32) & (val >> 31));
	XCOPY_CARRY;
		val = 0;
	} else if (cnt > 0) {
		val >>= cnt - 1;
		XSET_CFLG (val & 1);
	XCOPY_CARRY;
		val >>= 1;
	}
	XSET_ZFLG (((uae_s32)(val)) == 0);
	XSET_NFLG (((uae_s32)(val)) < 0);
	xm68k_dreg(dstreg) = (val);
}}}}xm68k_incpc(2);
}
void xop_e0b0_0(uae_u32 opcode) /* ROXR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 cnt = xm68k_dreg(srcreg);
{	uae_s32 data = xm68k_dreg(dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	XCLEAR_CZNV;
	if (cnt >= 33) cnt -= 33;
	if (cnt > 0) {
	cnt--;
	{
	uae_u32 carry;
	uae_u32 hival = (val << 1) | XGET_XFLG;
	hival <<= (31 - cnt);
	val >>= cnt;
	carry = val & 1;
	val >>= 1;
	val |= hival;
	XSET_XFLG (carry);
	val &= 0xffffffff;
	} }
	XSET_CFLG (XGET_XFLG);
	XSET_ZFLG (((uae_s32)(val)) == 0);
	XSET_NFLG (((uae_s32)(val)) < 0);
	xm68k_dreg(dstreg) = (val);
}}}}xm68k_incpc(2);
}
void xop_e0b8_0(uae_u32 opcode) /* ROR */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 cnt = xm68k_dreg(srcreg);
{	uae_s32 data = xm68k_dreg(dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	XCLEAR_CZNV;
	if (cnt > 0) {	uae_u32 hival;
	cnt &= 31;
	hival = val << (32 - cnt);
	val >>= cnt;
	val |= hival;
	val &= 0xffffffff;
	XSET_CFLG ((val & 0x80000000) >> 31);
	}
	XSET_ZFLG (((uae_s32)(val)) == 0);
	XSET_NFLG (((uae_s32)(val)) < 0);
	xm68k_dreg(dstreg) = (val);
}}}}xm68k_incpc(2);
}
void xop_e0d0_0(uae_u32 opcode) /* ASRW */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = xm68k_areg(srcreg);
{	uae_s16 data = xget_word(dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 cflg = val & 1;
	val = (val >> 1) | sign;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
	XSET_CFLG (cflg);
	XCOPY_CARRY;
	xput_word(dataa,val);
}}}}xm68k_incpc(2);
}
void xop_e0d8_0(uae_u32 opcode) /* ASRW */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = xm68k_areg(srcreg);
{	uae_s16 data = xget_word(dataa);
	xm68k_areg(srcreg) += 2;
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 cflg = val & 1;
	val = (val >> 1) | sign;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
	XSET_CFLG (cflg);
	XCOPY_CARRY;
	xput_word(dataa,val);
}}}}xm68k_incpc(2);
}
void xop_e0e0_0(uae_u32 opcode) /* ASRW */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = xm68k_areg(srcreg) - 2;
{	uae_s16 data = xget_word(dataa);
	xm68k_areg (srcreg) = dataa;
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 cflg = val & 1;
	val = (val >> 1) | sign;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
	XSET_CFLG (cflg);
	XCOPY_CARRY;
	xput_word(dataa,val);
}}}}xm68k_incpc(2);
}
void xop_e0e8_0(uae_u32 opcode) /* ASRW */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 data = xget_word(dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 cflg = val & 1;
	val = (val >> 1) | sign;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
	XSET_CFLG (cflg);
	XCOPY_CARRY;
	xput_word(dataa,val);
}}}}xm68k_incpc(4);
}
void xop_e0f0_0(uae_u32 opcode) /* ASRW */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr dataa = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s16 data = xget_word(dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 cflg = val & 1;
	val = (val >> 1) | sign;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
	XSET_CFLG (cflg);
	XCOPY_CARRY;
	xput_word(dataa,val);
}}}}}}
void xop_e0f8_0(uae_u32 opcode) /* ASRW */
{
{{	uaecptr dataa = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 data = xget_word(dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 cflg = val & 1;
	val = (val >> 1) | sign;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
	XSET_CFLG (cflg);
	XCOPY_CARRY;
	xput_word(dataa,val);
}}}}xm68k_incpc(4);
}
void xop_e0f9_0(uae_u32 opcode) /* ASRW */
{
{{	uaecptr dataa = xget_ilong(2);
{	uae_s16 data = xget_word(dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 cflg = val & 1;
	val = (val >> 1) | sign;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
	XSET_CFLG (cflg);
	XCOPY_CARRY;
	xput_word(dataa,val);
}}}}xm68k_incpc(6);
}
void xop_e100_0(uae_u32 opcode) /* ASL */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s8 data = xm68k_dreg(dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	XCLEAR_CZNV;
	if (cnt >= 8) {
		XSET_VFLG (val != 0);
		XSET_CFLG (cnt == 8 ? val & 1 : 0);
	XCOPY_CARRY;
		val = 0;
	} else {
		uae_u32 mask = (0xff << (7 - cnt)) & 0xff;
		XSET_VFLG ((val & mask) != mask && (val & mask) != 0);
		val <<= cnt - 1;
		XSET_CFLG ((val & 0x80) >> 7);
	XCOPY_CARRY;
		val <<= 1;
		val &= 0xff;
	}
	XSET_ZFLG (((uae_s8)(val)) == 0);
	XSET_NFLG (((uae_s8)(val)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((val) & 0xff);
}}}}xm68k_incpc(2);
}
void xop_e108_0(uae_u32 opcode) /* LSL */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s8 data = xm68k_dreg(dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	XCLEAR_CZNV;
	if (cnt >= 8) {
		XSET_CFLG (cnt == 8 ? val & 1 : 0);
	XCOPY_CARRY;
		val = 0;
	} else {
		val <<= (cnt - 1);
		XSET_CFLG ((val & 0x80) >> 7);
	XCOPY_CARRY;
		val <<= 1;
	val &= 0xff;
	}
	XSET_ZFLG (((uae_s8)(val)) == 0);
	XSET_NFLG (((uae_s8)(val)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((val) & 0xff);
}}}}xm68k_incpc(2);
}
void xop_e110_0(uae_u32 opcode) /* ROXL */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s8 data = xm68k_dreg(dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	XCLEAR_CZNV;
{	cnt--;
	{
	uae_u32 carry;
	uae_u32 loval = val >> (7 - cnt);
	carry = loval & 1;
	val = (((val << 1) | XGET_XFLG) << cnt) | (loval >> 1);
	XSET_XFLG (carry);
	val &= 0xff;
	} }
	XSET_CFLG (XGET_XFLG);
	XSET_ZFLG (((uae_s8)(val)) == 0);
	XSET_NFLG (((uae_s8)(val)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((val) & 0xff);
}}}}xm68k_incpc(2);
}
void xop_e118_0(uae_u32 opcode) /* ROL */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s8 data = xm68k_dreg(dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	XCLEAR_CZNV;
{	uae_u32 loval;
	cnt &= 7;
	loval = val >> (8 - cnt);
	val <<= cnt;
	val |= loval;
	val &= 0xff;
	XSET_CFLG (val & 1);
}
	XSET_ZFLG (((uae_s8)(val)) == 0);
	XSET_NFLG (((uae_s8)(val)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((val) & 0xff);
}}}}xm68k_incpc(2);
}
void xop_e120_0(uae_u32 opcode) /* ASL */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 cnt = xm68k_dreg(srcreg);
{	uae_s8 data = xm68k_dreg(dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	XCLEAR_CZNV;
	if (cnt >= 8) {
		XSET_VFLG (val != 0);
		XSET_CFLG (cnt == 8 ? val & 1 : 0);
	XCOPY_CARRY;
		val = 0;
	} else if (cnt > 0) {
		uae_u32 mask = (0xff << (7 - cnt)) & 0xff;
		XSET_VFLG ((val & mask) != mask && (val & mask) != 0);
		val <<= cnt - 1;
		XSET_CFLG ((val & 0x80) >> 7);
	XCOPY_CARRY;
		val <<= 1;
		val &= 0xff;
	}
	XSET_ZFLG (((uae_s8)(val)) == 0);
	XSET_NFLG (((uae_s8)(val)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((val) & 0xff);
}}}}xm68k_incpc(2);
}
void xop_e128_0(uae_u32 opcode) /* LSL */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 cnt = xm68k_dreg(srcreg);
{	uae_s8 data = xm68k_dreg(dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	XCLEAR_CZNV;
	if (cnt >= 8) {
		XSET_CFLG (cnt == 8 ? val & 1 : 0);
	XCOPY_CARRY;
		val = 0;
	} else if (cnt > 0) {
		val <<= (cnt - 1);
		XSET_CFLG ((val & 0x80) >> 7);
	XCOPY_CARRY;
		val <<= 1;
	val &= 0xff;
	}
	XSET_ZFLG (((uae_s8)(val)) == 0);
	XSET_NFLG (((uae_s8)(val)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((val) & 0xff);
}}}}xm68k_incpc(2);
}
void xop_e130_0(uae_u32 opcode) /* ROXL */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 cnt = xm68k_dreg(srcreg);
{	uae_s8 data = xm68k_dreg(dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	XCLEAR_CZNV;
	if (cnt >= 36) cnt -= 36;
	if (cnt >= 18) cnt -= 18;
	if (cnt >= 9) cnt -= 9;
	if (cnt > 0) {
	cnt--;
	{
	uae_u32 carry;
	uae_u32 loval = val >> (7 - cnt);
	carry = loval & 1;
	val = (((val << 1) | XGET_XFLG) << cnt) | (loval >> 1);
	XSET_XFLG (carry);
	val &= 0xff;
	} }
	XSET_CFLG (XGET_XFLG);
	XSET_ZFLG (((uae_s8)(val)) == 0);
	XSET_NFLG (((uae_s8)(val)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((val) & 0xff);
}}}}xm68k_incpc(2);
}
void xop_e138_0(uae_u32 opcode) /* ROL */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s8 cnt = xm68k_dreg(srcreg);
{	uae_s8 data = xm68k_dreg(dstreg);
{	uae_u32 val = (uae_u8)data;
	cnt &= 63;
	XCLEAR_CZNV;
	if (cnt > 0) {
	uae_u32 loval;
	cnt &= 7;
	loval = val >> (8 - cnt);
	val <<= cnt;
	val |= loval;
	val &= 0xff;
	XSET_CFLG (val & 1);
}
	XSET_ZFLG (((uae_s8)(val)) == 0);
	XSET_NFLG (((uae_s8)(val)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xff) | ((val) & 0xff);
}}}}xm68k_incpc(2);
}
void xop_e140_0(uae_u32 opcode) /* ASL */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s16 data = xm68k_dreg(dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	XCLEAR_CZNV;
	if (cnt >= 16) {
		XSET_VFLG (val != 0);
		XSET_CFLG (cnt == 16 ? val & 1 : 0);
	XCOPY_CARRY;
		val = 0;
	} else {
		uae_u32 mask = (0xffff << (15 - cnt)) & 0xffff;
		XSET_VFLG ((val & mask) != mask && (val & mask) != 0);
		val <<= cnt - 1;
		XSET_CFLG ((val & 0x8000) >> 15);
	XCOPY_CARRY;
		val <<= 1;
		val &= 0xffff;
	}
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}xm68k_incpc(2);
}
void xop_e148_0(uae_u32 opcode) /* LSL */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s16 data = xm68k_dreg(dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	XCLEAR_CZNV;
	if (cnt >= 16) {
		XSET_CFLG (cnt == 16 ? val & 1 : 0);
	XCOPY_CARRY;
		val = 0;
	} else {
		val <<= (cnt - 1);
		XSET_CFLG ((val & 0x8000) >> 15);
	XCOPY_CARRY;
		val <<= 1;
	val &= 0xffff;
	}
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}xm68k_incpc(2);
}
void xop_e150_0(uae_u32 opcode) /* ROXL */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s16 data = xm68k_dreg(dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	XCLEAR_CZNV;
{	cnt--;
	{
	uae_u32 carry;
	uae_u32 loval = val >> (15 - cnt);
	carry = loval & 1;
	val = (((val << 1) | XGET_XFLG) << cnt) | (loval >> 1);
	XSET_XFLG (carry);
	val &= 0xffff;
	} }
	XSET_CFLG (XGET_XFLG);
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}xm68k_incpc(2);
}
void xop_e158_0(uae_u32 opcode) /* ROL */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s16 data = xm68k_dreg(dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	XCLEAR_CZNV;
{	uae_u32 loval;
	cnt &= 15;
	loval = val >> (16 - cnt);
	val <<= cnt;
	val |= loval;
	val &= 0xffff;
	XSET_CFLG (val & 1);
}
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}xm68k_incpc(2);
}
void xop_e160_0(uae_u32 opcode) /* ASL */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 cnt = xm68k_dreg(srcreg);
{	uae_s16 data = xm68k_dreg(dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	XCLEAR_CZNV;
	if (cnt >= 16) {
		XSET_VFLG (val != 0);
		XSET_CFLG (cnt == 16 ? val & 1 : 0);
	XCOPY_CARRY;
		val = 0;
	} else if (cnt > 0) {
		uae_u32 mask = (0xffff << (15 - cnt)) & 0xffff;
		XSET_VFLG ((val & mask) != mask && (val & mask) != 0);
		val <<= cnt - 1;
		XSET_CFLG ((val & 0x8000) >> 15);
	XCOPY_CARRY;
		val <<= 1;
		val &= 0xffff;
	}
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}xm68k_incpc(2);
}
void xop_e168_0(uae_u32 opcode) /* LSL */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 cnt = xm68k_dreg(srcreg);
{	uae_s16 data = xm68k_dreg(dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	XCLEAR_CZNV;
	if (cnt >= 16) {
		XSET_CFLG (cnt == 16 ? val & 1 : 0);
	XCOPY_CARRY;
		val = 0;
	} else if (cnt > 0) {
		val <<= (cnt - 1);
		XSET_CFLG ((val & 0x8000) >> 15);
	XCOPY_CARRY;
		val <<= 1;
	val &= 0xffff;
	}
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}xm68k_incpc(2);
}
void xop_e170_0(uae_u32 opcode) /* ROXL */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 cnt = xm68k_dreg(srcreg);
{	uae_s16 data = xm68k_dreg(dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	XCLEAR_CZNV;
	if (cnt >= 34) cnt -= 34;
	if (cnt >= 17) cnt -= 17;
	if (cnt > 0) {
	cnt--;
	{
	uae_u32 carry;
	uae_u32 loval = val >> (15 - cnt);
	carry = loval & 1;
	val = (((val << 1) | XGET_XFLG) << cnt) | (loval >> 1);
	XSET_XFLG (carry);
	val &= 0xffff;
	} }
	XSET_CFLG (XGET_XFLG);
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}xm68k_incpc(2);
}
void xop_e178_0(uae_u32 opcode) /* ROL */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s16 cnt = xm68k_dreg(srcreg);
{	uae_s16 data = xm68k_dreg(dstreg);
{	uae_u32 val = (uae_u16)data;
	cnt &= 63;
	XCLEAR_CZNV;
	if (cnt > 0) {
	uae_u32 loval;
	cnt &= 15;
	loval = val >> (16 - cnt);
	val <<= cnt;
	val |= loval;
	val &= 0xffff;
	XSET_CFLG (val & 1);
}
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
	xm68k_dreg(dstreg) = (xm68k_dreg(dstreg) & ~0xffff) | ((val) & 0xffff);
}}}}xm68k_incpc(2);
}
void xop_e180_0(uae_u32 opcode) /* ASL */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s32 data = xm68k_dreg(dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	XCLEAR_CZNV;
	if (cnt >= 32) {
		XSET_VFLG (val != 0);
		XSET_CFLG (cnt == 32 ? val & 1 : 0);
	XCOPY_CARRY;
		val = 0;
	} else {
		uae_u32 mask = (0xffffffff << (31 - cnt)) & 0xffffffff;
		XSET_VFLG ((val & mask) != mask && (val & mask) != 0);
		val <<= cnt - 1;
		XSET_CFLG ((val & 0x80000000) >> 31);
	XCOPY_CARRY;
		val <<= 1;
		val &= 0xffffffff;
	}
	XSET_ZFLG (((uae_s32)(val)) == 0);
	XSET_NFLG (((uae_s32)(val)) < 0);
	xm68k_dreg(dstreg) = (val);
}}}}xm68k_incpc(2);
}
void xop_e188_0(uae_u32 opcode) /* LSL */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s32 data = xm68k_dreg(dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	XCLEAR_CZNV;
	if (cnt >= 32) {
		XSET_CFLG (cnt == 32 ? val & 1 : 0);
	XCOPY_CARRY;
		val = 0;
	} else {
		val <<= (cnt - 1);
		XSET_CFLG ((val & 0x80000000) >> 31);
	XCOPY_CARRY;
		val <<= 1;
	val &= 0xffffffff;
	}
	XSET_ZFLG (((uae_s32)(val)) == 0);
	XSET_NFLG (((uae_s32)(val)) < 0);
	xm68k_dreg(dstreg) = (val);
}}}}xm68k_incpc(2);
}
void xop_e190_0(uae_u32 opcode) /* ROXL */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s32 data = xm68k_dreg(dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	XCLEAR_CZNV;
{	cnt--;
	{
	uae_u32 carry;
	uae_u32 loval = val >> (31 - cnt);
	carry = loval & 1;
	val = (((val << 1) | XGET_XFLG) << cnt) | (loval >> 1);
	XSET_XFLG (carry);
	val &= 0xffffffff;
	} }
	XSET_CFLG (XGET_XFLG);
	XSET_ZFLG (((uae_s32)(val)) == 0);
	XSET_NFLG (((uae_s32)(val)) < 0);
	xm68k_dreg(dstreg) = (val);
}}}}xm68k_incpc(2);
}
void xop_e198_0(uae_u32 opcode) /* ROL */
{
	uae_u32 srcreg = ximm8_table[((opcode >> 9) & 7)];
	uae_u32 dstreg = opcode & 7;
{{	uae_u32 cnt = srcreg;
{	uae_s32 data = xm68k_dreg(dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	XCLEAR_CZNV;
{	uae_u32 loval;
	cnt &= 31;
	loval = val >> (32 - cnt);
	val <<= cnt;
	val |= loval;
	val &= 0xffffffff;
	XSET_CFLG (val & 1);
}
	XSET_ZFLG (((uae_s32)(val)) == 0);
	XSET_NFLG (((uae_s32)(val)) < 0);
	xm68k_dreg(dstreg) = (val);
}}}}xm68k_incpc(2);
}
void xop_e1a0_0(uae_u32 opcode) /* ASL */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 cnt = xm68k_dreg(srcreg);
{	uae_s32 data = xm68k_dreg(dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	XCLEAR_CZNV;
	if (cnt >= 32) {
		XSET_VFLG (val != 0);
		XSET_CFLG (cnt == 32 ? val & 1 : 0);
	XCOPY_CARRY;
		val = 0;
	} else if (cnt > 0) {
		uae_u32 mask = (0xffffffff << (31 - cnt)) & 0xffffffff;
		XSET_VFLG ((val & mask) != mask && (val & mask) != 0);
		val <<= cnt - 1;
		XSET_CFLG ((val & 0x80000000) >> 31);
	XCOPY_CARRY;
		val <<= 1;
		val &= 0xffffffff;
	}
	XSET_ZFLG (((uae_s32)(val)) == 0);
	XSET_NFLG (((uae_s32)(val)) < 0);
	xm68k_dreg(dstreg) = (val);
}}}}xm68k_incpc(2);
}
void xop_e1a8_0(uae_u32 opcode) /* LSL */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 cnt = xm68k_dreg(srcreg);
{	uae_s32 data = xm68k_dreg(dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	XCLEAR_CZNV;
	if (cnt >= 32) {
		XSET_CFLG (cnt == 32 ? val & 1 : 0);
	XCOPY_CARRY;
		val = 0;
	} else if (cnt > 0) {
		val <<= (cnt - 1);
		XSET_CFLG ((val & 0x80000000) >> 31);
	XCOPY_CARRY;
		val <<= 1;
	val &= 0xffffffff;
	}
	XSET_ZFLG (((uae_s32)(val)) == 0);
	XSET_NFLG (((uae_s32)(val)) < 0);
	xm68k_dreg(dstreg) = (val);
}}}}xm68k_incpc(2);
}
void xop_e1b0_0(uae_u32 opcode) /* ROXL */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 cnt = xm68k_dreg(srcreg);
{	uae_s32 data = xm68k_dreg(dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	XCLEAR_CZNV;
	if (cnt >= 33) cnt -= 33;
	if (cnt > 0) {
	cnt--;
	{
	uae_u32 carry;
	uae_u32 loval = val >> (31 - cnt);
	carry = loval & 1;
	val = (((val << 1) | XGET_XFLG) << cnt) | (loval >> 1);
	XSET_XFLG (carry);
	val &= 0xffffffff;
	} }
	XSET_CFLG (XGET_XFLG);
	XSET_ZFLG (((uae_s32)(val)) == 0);
	XSET_NFLG (((uae_s32)(val)) < 0);
	xm68k_dreg(dstreg) = (val);
}}}}xm68k_incpc(2);
}
void xop_e1b8_0(uae_u32 opcode) /* ROL */
{
	uae_u32 srcreg = ((opcode >> 9) & 7);
	uae_u32 dstreg = opcode & 7;
{{	uae_s32 cnt = xm68k_dreg(srcreg);
{	uae_s32 data = xm68k_dreg(dstreg);
{	uae_u32 val = data;
	cnt &= 63;
	XCLEAR_CZNV;
	if (cnt > 0) {
	uae_u32 loval;
	cnt &= 31;
	loval = val >> (32 - cnt);
	val <<= cnt;
	val |= loval;
	val &= 0xffffffff;
	XSET_CFLG (val & 1);
}
	XSET_ZFLG (((uae_s32)(val)) == 0);
	XSET_NFLG (((uae_s32)(val)) < 0);
	xm68k_dreg(dstreg) = (val);
}}}}xm68k_incpc(2);
}
void xop_e1d0_0(uae_u32 opcode) /* ASLW */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = xm68k_areg(srcreg);
{	uae_s16 data = xget_word(dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 sign2;
	val <<= 1;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
	sign2 = 0x8000 & val;
	XSET_CFLG (sign != 0);
	XCOPY_CARRY;
	XSET_VFLG (XGET_VFLG | (sign2 != sign));
	xput_word(dataa,val);
}}}}xm68k_incpc(2);
}
void xop_e1d8_0(uae_u32 opcode) /* ASLW */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = xm68k_areg(srcreg);
{	uae_s16 data = xget_word(dataa);
	xm68k_areg(srcreg) += 2;
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 sign2;
	val <<= 1;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
	sign2 = 0x8000 & val;
	XSET_CFLG (sign != 0);
	XCOPY_CARRY;
	XSET_VFLG (XGET_VFLG | (sign2 != sign));
	xput_word(dataa,val);
}}}}xm68k_incpc(2);
}
void xop_e1e0_0(uae_u32 opcode) /* ASLW */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = xm68k_areg(srcreg) - 2;
{	uae_s16 data = xget_word(dataa);
	xm68k_areg (srcreg) = dataa;
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 sign2;
	val <<= 1;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
	sign2 = 0x8000 & val;
	XSET_CFLG (sign != 0);
	XCOPY_CARRY;
	XSET_VFLG (XGET_VFLG | (sign2 != sign));
	xput_word(dataa,val);
}}}}xm68k_incpc(2);
}
void xop_e1e8_0(uae_u32 opcode) /* ASLW */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 data = xget_word(dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 sign2;
	val <<= 1;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
	sign2 = 0x8000 & val;
	XSET_CFLG (sign != 0);
	XCOPY_CARRY;
	XSET_VFLG (XGET_VFLG | (sign2 != sign));
	xput_word(dataa,val);
}}}}xm68k_incpc(4);
}
void xop_e1f0_0(uae_u32 opcode) /* ASLW */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr dataa = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s16 data = xget_word(dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 sign2;
	val <<= 1;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
	sign2 = 0x8000 & val;
	XSET_CFLG (sign != 0);
	XCOPY_CARRY;
	XSET_VFLG (XGET_VFLG | (sign2 != sign));
	xput_word(dataa,val);
}}}}}}
void xop_e1f8_0(uae_u32 opcode) /* ASLW */
{
{{	uaecptr dataa = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 data = xget_word(dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 sign2;
	val <<= 1;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
	sign2 = 0x8000 & val;
	XSET_CFLG (sign != 0);
	XCOPY_CARRY;
	XSET_VFLG (XGET_VFLG | (sign2 != sign));
	xput_word(dataa,val);
}}}}xm68k_incpc(4);
}
void xop_e1f9_0(uae_u32 opcode) /* ASLW */
{
{{	uaecptr dataa = xget_ilong(2);
{	uae_s16 data = xget_word(dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 sign = 0x8000 & val;
	uae_u32 sign2;
	val <<= 1;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
	sign2 = 0x8000 & val;
	XSET_CFLG (sign != 0);
	XCOPY_CARRY;
	XSET_VFLG (XGET_VFLG | (sign2 != sign));
	xput_word(dataa,val);
}}}}xm68k_incpc(6);
}
void xop_e2d0_0(uae_u32 opcode) /* LSRW */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = xm68k_areg(srcreg);
{	uae_s16 data = xget_word(dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 carry = val & 1;
	val >>= 1;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry);
	XCOPY_CARRY;
	xput_word(dataa,val);
}}}}xm68k_incpc(2);
}
void xop_e2d8_0(uae_u32 opcode) /* LSRW */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = xm68k_areg(srcreg);
{	uae_s16 data = xget_word(dataa);
	xm68k_areg(srcreg) += 2;
{	uae_u32 val = (uae_u16)data;
	uae_u32 carry = val & 1;
	val >>= 1;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry);
	XCOPY_CARRY;
	xput_word(dataa,val);
}}}}xm68k_incpc(2);
}
void xop_e2e0_0(uae_u32 opcode) /* LSRW */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = xm68k_areg(srcreg) - 2;
{	uae_s16 data = xget_word(dataa);
	xm68k_areg (srcreg) = dataa;
{	uae_u32 val = (uae_u16)data;
	uae_u32 carry = val & 1;
	val >>= 1;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry);
	XCOPY_CARRY;
	xput_word(dataa,val);
}}}}xm68k_incpc(2);
}
void xop_e2e8_0(uae_u32 opcode) /* LSRW */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 data = xget_word(dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 carry = val & 1;
	val >>= 1;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry);
	XCOPY_CARRY;
	xput_word(dataa,val);
}}}}xm68k_incpc(4);
}
void xop_e2f0_0(uae_u32 opcode) /* LSRW */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr dataa = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s16 data = xget_word(dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 carry = val & 1;
	val >>= 1;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry);
	XCOPY_CARRY;
	xput_word(dataa,val);
}}}}}}
void xop_e2f8_0(uae_u32 opcode) /* LSRW */
{
{{	uaecptr dataa = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 data = xget_word(dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 carry = val & 1;
	val >>= 1;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry);
	XCOPY_CARRY;
	xput_word(dataa,val);
}}}}xm68k_incpc(4);
}
void xop_e2f9_0(uae_u32 opcode) /* LSRW */
{
{{	uaecptr dataa = xget_ilong(2);
{	uae_s16 data = xget_word(dataa);
{	uae_u32 val = (uae_u16)data;
	uae_u32 carry = val & 1;
	val >>= 1;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry);
	XCOPY_CARRY;
	xput_word(dataa,val);
}}}}xm68k_incpc(6);
}
void xop_e3d0_0(uae_u32 opcode) /* LSLW */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = xm68k_areg(srcreg);
{	uae_s16 data = xget_word(dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry >> 15);
	XCOPY_CARRY;
	xput_word(dataa,val);
}}}}xm68k_incpc(2);
}
void xop_e3d8_0(uae_u32 opcode) /* LSLW */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = xm68k_areg(srcreg);
{	uae_s16 data = xget_word(dataa);
	xm68k_areg(srcreg) += 2;
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry >> 15);
	XCOPY_CARRY;
	xput_word(dataa,val);
}}}}xm68k_incpc(2);
}
void xop_e3e0_0(uae_u32 opcode) /* LSLW */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = xm68k_areg(srcreg) - 2;
{	uae_s16 data = xget_word(dataa);
	xm68k_areg (srcreg) = dataa;
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry >> 15);
	XCOPY_CARRY;
	xput_word(dataa,val);
}}}}xm68k_incpc(2);
}
void xop_e3e8_0(uae_u32 opcode) /* LSLW */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 data = xget_word(dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry >> 15);
	XCOPY_CARRY;
	xput_word(dataa,val);
}}}}xm68k_incpc(4);
}
void xop_e3f0_0(uae_u32 opcode) /* LSLW */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr dataa = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s16 data = xget_word(dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry >> 15);
	XCOPY_CARRY;
	xput_word(dataa,val);
}}}}}}
void xop_e3f8_0(uae_u32 opcode) /* LSLW */
{
{{	uaecptr dataa = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 data = xget_word(dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry >> 15);
	XCOPY_CARRY;
	xput_word(dataa,val);
}}}}xm68k_incpc(4);
}
void xop_e3f9_0(uae_u32 opcode) /* LSLW */
{
{{	uaecptr dataa = xget_ilong(2);
{	uae_s16 data = xget_word(dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry >> 15);
	XCOPY_CARRY;
	xput_word(dataa,val);
}}}}xm68k_incpc(6);
}
void xop_e4d0_0(uae_u32 opcode) /* ROXRW */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = xm68k_areg(srcreg);
{	uae_s16 data = xget_word(dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (XGET_XFLG) val |= 0x8000;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry);
	XCOPY_CARRY;
	xput_word(dataa,val);
}}}}xm68k_incpc(2);
}
void xop_e4d8_0(uae_u32 opcode) /* ROXRW */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = xm68k_areg(srcreg);
{	uae_s16 data = xget_word(dataa);
	xm68k_areg(srcreg) += 2;
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (XGET_XFLG) val |= 0x8000;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry);
	XCOPY_CARRY;
	xput_word(dataa,val);
}}}}xm68k_incpc(2);
}
void xop_e4e0_0(uae_u32 opcode) /* ROXRW */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = xm68k_areg(srcreg) - 2;
{	uae_s16 data = xget_word(dataa);
	xm68k_areg (srcreg) = dataa;
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (XGET_XFLG) val |= 0x8000;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry);
	XCOPY_CARRY;
	xput_word(dataa,val);
}}}}xm68k_incpc(2);
}
void xop_e4e8_0(uae_u32 opcode) /* ROXRW */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 data = xget_word(dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (XGET_XFLG) val |= 0x8000;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry);
	XCOPY_CARRY;
	xput_word(dataa,val);
}}}}xm68k_incpc(4);
}
void xop_e4f0_0(uae_u32 opcode) /* ROXRW */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr dataa = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s16 data = xget_word(dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (XGET_XFLG) val |= 0x8000;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry);
	XCOPY_CARRY;
	xput_word(dataa,val);
}}}}}}
void xop_e4f8_0(uae_u32 opcode) /* ROXRW */
{
{{	uaecptr dataa = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 data = xget_word(dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (XGET_XFLG) val |= 0x8000;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry);
	XCOPY_CARRY;
	xput_word(dataa,val);
}}}}xm68k_incpc(4);
}
void xop_e4f9_0(uae_u32 opcode) /* ROXRW */
{
{{	uaecptr dataa = xget_ilong(2);
{	uae_s16 data = xget_word(dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (XGET_XFLG) val |= 0x8000;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry);
	XCOPY_CARRY;
	xput_word(dataa,val);
}}}}xm68k_incpc(6);
}
void xop_e5d0_0(uae_u32 opcode) /* ROXLW */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = xm68k_areg(srcreg);
{	uae_s16 data = xget_word(dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (XGET_XFLG) val |= 1;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry >> 15);
	XCOPY_CARRY;
	xput_word(dataa,val);
}}}}xm68k_incpc(2);
}
void xop_e5d8_0(uae_u32 opcode) /* ROXLW */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = xm68k_areg(srcreg);
{	uae_s16 data = xget_word(dataa);
	xm68k_areg(srcreg) += 2;
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (XGET_XFLG) val |= 1;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry >> 15);
	XCOPY_CARRY;
	xput_word(dataa,val);
}}}}xm68k_incpc(2);
}
void xop_e5e0_0(uae_u32 opcode) /* ROXLW */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = xm68k_areg(srcreg) - 2;
{	uae_s16 data = xget_word(dataa);
	xm68k_areg (srcreg) = dataa;
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (XGET_XFLG) val |= 1;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry >> 15);
	XCOPY_CARRY;
	xput_word(dataa,val);
}}}}xm68k_incpc(2);
}
void xop_e5e8_0(uae_u32 opcode) /* ROXLW */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 data = xget_word(dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (XGET_XFLG) val |= 1;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry >> 15);
	XCOPY_CARRY;
	xput_word(dataa,val);
}}}}xm68k_incpc(4);
}
void xop_e5f0_0(uae_u32 opcode) /* ROXLW */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr dataa = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s16 data = xget_word(dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (XGET_XFLG) val |= 1;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry >> 15);
	XCOPY_CARRY;
	xput_word(dataa,val);
}}}}}}
void xop_e5f8_0(uae_u32 opcode) /* ROXLW */
{
{{	uaecptr dataa = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 data = xget_word(dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (XGET_XFLG) val |= 1;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry >> 15);
	XCOPY_CARRY;
	xput_word(dataa,val);
}}}}xm68k_incpc(4);
}
void xop_e5f9_0(uae_u32 opcode) /* ROXLW */
{
{{	uaecptr dataa = xget_ilong(2);
{	uae_s16 data = xget_word(dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (XGET_XFLG) val |= 1;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry >> 15);
	XCOPY_CARRY;
	xput_word(dataa,val);
}}}}xm68k_incpc(6);
}
void xop_e6d0_0(uae_u32 opcode) /* RORW */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = xm68k_areg(srcreg);
{	uae_s16 data = xget_word(dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (carry) val |= 0x8000;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry);
	xput_word(dataa,val);
}}}}xm68k_incpc(2);
}
void xop_e6d8_0(uae_u32 opcode) /* RORW */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = xm68k_areg(srcreg);
{	uae_s16 data = xget_word(dataa);
	xm68k_areg(srcreg) += 2;
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (carry) val |= 0x8000;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry);
	xput_word(dataa,val);
}}}}xm68k_incpc(2);
}
void xop_e6e0_0(uae_u32 opcode) /* RORW */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = xm68k_areg(srcreg) - 2;
{	uae_s16 data = xget_word(dataa);
	xm68k_areg (srcreg) = dataa;
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (carry) val |= 0x8000;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry);
	xput_word(dataa,val);
}}}}xm68k_incpc(2);
}
void xop_e6e8_0(uae_u32 opcode) /* RORW */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 data = xget_word(dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (carry) val |= 0x8000;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry);
	xput_word(dataa,val);
}}}}xm68k_incpc(4);
}
void xop_e6f0_0(uae_u32 opcode) /* RORW */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr dataa = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s16 data = xget_word(dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (carry) val |= 0x8000;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry);
	xput_word(dataa,val);
}}}}}}
void xop_e6f8_0(uae_u32 opcode) /* RORW */
{
{{	uaecptr dataa = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 data = xget_word(dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (carry) val |= 0x8000;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry);
	xput_word(dataa,val);
}}}}xm68k_incpc(4);
}
void xop_e6f9_0(uae_u32 opcode) /* RORW */
{
{{	uaecptr dataa = xget_ilong(2);
{	uae_s16 data = xget_word(dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 1;
	val >>= 1;
	if (carry) val |= 0x8000;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry);
	xput_word(dataa,val);
}}}}xm68k_incpc(6);
}
void xop_e7d0_0(uae_u32 opcode) /* ROLW */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = xm68k_areg(srcreg);
{	uae_s16 data = xget_word(dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (carry)  val |= 1;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry >> 15);
	xput_word(dataa,val);
}}}}xm68k_incpc(2);
}
void xop_e7d8_0(uae_u32 opcode) /* ROLW */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = xm68k_areg(srcreg);
{	uae_s16 data = xget_word(dataa);
	xm68k_areg(srcreg) += 2;
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (carry)  val |= 1;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry >> 15);
	xput_word(dataa,val);
}}}}xm68k_incpc(2);
}
void xop_e7e0_0(uae_u32 opcode) /* ROLW */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = xm68k_areg(srcreg) - 2;
{	uae_s16 data = xget_word(dataa);
	xm68k_areg (srcreg) = dataa;
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (carry)  val |= 1;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry >> 15);
	xput_word(dataa,val);
}}}}xm68k_incpc(2);
}
void xop_e7e8_0(uae_u32 opcode) /* ROLW */
{
	uae_u32 srcreg = (opcode & 7);
{{	uaecptr dataa = xm68k_areg(srcreg) + (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 data = xget_word(dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (carry)  val |= 1;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry >> 15);
	xput_word(dataa,val);
}}}}xm68k_incpc(4);
}
void xop_e7f0_0(uae_u32 opcode) /* ROLW */
{
	uae_u32 srcreg = (opcode & 7);
{{xm68k_incpc(2);
{	uaecptr dataa = xget_disp_ea_020(xm68k_areg(srcreg), xnext_iword());
{	uae_s16 data = xget_word(dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (carry)  val |= 1;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry >> 15);
	xput_word(dataa,val);
}}}}}}
void xop_e7f8_0(uae_u32 opcode) /* ROLW */
{
{{	uaecptr dataa = (uae_s32)(uae_s16)xget_iword(2);
{	uae_s16 data = xget_word(dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (carry)  val |= 1;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry >> 15);
	xput_word(dataa,val);
}}}}xm68k_incpc(4);
}
void xop_e7f9_0(uae_u32 opcode) /* ROLW */
{
{{	uaecptr dataa = xget_ilong(2);
{	uae_s16 data = xget_word(dataa);
{	uae_u16 val = data;
	uae_u32 carry = val & 0x8000;
	val <<= 1;
	if (carry)  val |= 1;
	XCLEAR_CZNV;
	XSET_ZFLG (((uae_s16)(val)) == 0);
	XSET_NFLG (((uae_s16)(val)) < 0);
XSET_CFLG (carry >> 15);
	xput_word(dataa,val);
}}}}xm68k_incpc(6);
}
void xop_e8c0_0(uae_u32 opcode) /* BFTST */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_e8d0_0(uae_u32 opcode) /* BFTST */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_e8e8_0(uae_u32 opcode) /* BFTST */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_e8f0_0(uae_u32 opcode) /* BFTST */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_e8f8_0(uae_u32 opcode) /* BFTST */
{
{}}
void xop_e8f9_0(uae_u32 opcode) /* BFTST */
{
{}}
void xop_e8fa_0(uae_u32 opcode) /* BFTST */
{
	uae_u32 dstreg = 2;
{}}
void xop_e8fb_0(uae_u32 opcode) /* BFTST */
{
	uae_u32 dstreg = 3;
{}}
void xop_e9c0_0(uae_u32 opcode) /* BFEXTU */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_e9d0_0(uae_u32 opcode) /* BFEXTU */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_e9e8_0(uae_u32 opcode) /* BFEXTU */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_e9f0_0(uae_u32 opcode) /* BFEXTU */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_e9f8_0(uae_u32 opcode) /* BFEXTU */
{
{}}
void xop_e9f9_0(uae_u32 opcode) /* BFEXTU */
{
{}}
void xop_e9fa_0(uae_u32 opcode) /* BFEXTU */
{
	uae_u32 dstreg = 2;
{}}
void xop_e9fb_0(uae_u32 opcode) /* BFEXTU */
{
	uae_u32 dstreg = 3;
{}}
void xop_eac0_0(uae_u32 opcode) /* BFCHG */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_ead0_0(uae_u32 opcode) /* BFCHG */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_eae8_0(uae_u32 opcode) /* BFCHG */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_eaf0_0(uae_u32 opcode) /* BFCHG */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_eaf8_0(uae_u32 opcode) /* BFCHG */
{
{}}
void xop_eaf9_0(uae_u32 opcode) /* BFCHG */
{
{}}
void xop_ebc0_0(uae_u32 opcode) /* BFEXTS */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_ebd0_0(uae_u32 opcode) /* BFEXTS */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_ebe8_0(uae_u32 opcode) /* BFEXTS */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_ebf0_0(uae_u32 opcode) /* BFEXTS */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_ebf8_0(uae_u32 opcode) /* BFEXTS */
{
{}}
void xop_ebf9_0(uae_u32 opcode) /* BFEXTS */
{
{}}
void xop_ebfa_0(uae_u32 opcode) /* BFEXTS */
{
	uae_u32 dstreg = 2;
{}}
void xop_ebfb_0(uae_u32 opcode) /* BFEXTS */
{
	uae_u32 dstreg = 3;
{}}
void xop_ecc0_0(uae_u32 opcode) /* BFCLR */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_ecd0_0(uae_u32 opcode) /* BFCLR */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_ece8_0(uae_u32 opcode) /* BFCLR */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_ecf0_0(uae_u32 opcode) /* BFCLR */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_ecf8_0(uae_u32 opcode) /* BFCLR */
{
{}}
void xop_ecf9_0(uae_u32 opcode) /* BFCLR */
{
{}}
void xop_edc0_0(uae_u32 opcode) /* BFFFO */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_edd0_0(uae_u32 opcode) /* BFFFO */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_ede8_0(uae_u32 opcode) /* BFFFO */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_edf0_0(uae_u32 opcode) /* BFFFO */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_edf8_0(uae_u32 opcode) /* BFFFO */
{
{}}
void xop_edf9_0(uae_u32 opcode) /* BFFFO */
{
{}}
void xop_edfa_0(uae_u32 opcode) /* BFFFO */
{
	uae_u32 dstreg = 2;
{}}
void xop_edfb_0(uae_u32 opcode) /* BFFFO */
{
	uae_u32 dstreg = 3;
{}}
void xop_eec0_0(uae_u32 opcode) /* BFSET */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_eed0_0(uae_u32 opcode) /* BFSET */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_eee8_0(uae_u32 opcode) /* BFSET */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_eef0_0(uae_u32 opcode) /* BFSET */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_eef8_0(uae_u32 opcode) /* BFSET */
{
{}}
void xop_eef9_0(uae_u32 opcode) /* BFSET */
{
{}}
void xop_efc0_0(uae_u32 opcode) /* BFINS */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_efd0_0(uae_u32 opcode) /* BFINS */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_efe8_0(uae_u32 opcode) /* BFINS */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_eff0_0(uae_u32 opcode) /* BFINS */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_eff8_0(uae_u32 opcode) /* BFINS */
{
{}}
void xop_eff9_0(uae_u32 opcode) /* BFINS */
{
{}}
void xop_f000_0(uae_u32 opcode) /* MMUOP30A */
{
	uae_u32 srcreg = (opcode & 7);
{}}
void xop_f008_0(uae_u32 opcode) /* MMUOP30A */
{
	uae_u32 srcreg = (opcode & 7);
{}}
void xop_f010_0(uae_u32 opcode) /* MMUOP30A */
{
	uae_u32 srcreg = (opcode & 7);
{}}
void xop_f018_0(uae_u32 opcode) /* MMUOP30A */
{
	uae_u32 srcreg = (opcode & 7);
{}}
void xop_f020_0(uae_u32 opcode) /* MMUOP30A */
{
	uae_u32 srcreg = (opcode & 7);
{}}
void xop_f028_0(uae_u32 opcode) /* MMUOP30A */
{
	uae_u32 srcreg = (opcode & 7);
{}}
void xop_f030_0(uae_u32 opcode) /* MMUOP30A */
{
	uae_u32 srcreg = (opcode & 7);
{}}
void xop_f038_0(uae_u32 opcode) /* MMUOP30A */
{
{}}
void xop_f039_0(uae_u32 opcode) /* MMUOP30A */
{
{}}
void xop_f03a_0(uae_u32 opcode) /* MMUOP30A */
{
{}}
void xop_f03b_0(uae_u32 opcode) /* MMUOP30A */
{
{}}
void xop_f200_0(uae_u32 opcode) /* FPP */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_f208_0(uae_u32 opcode) /* FPP */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_f210_0(uae_u32 opcode) /* FPP */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_f218_0(uae_u32 opcode) /* FPP */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_f220_0(uae_u32 opcode) /* FPP */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_f228_0(uae_u32 opcode) /* FPP */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_f230_0(uae_u32 opcode) /* FPP */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_f238_0(uae_u32 opcode) /* FPP */
{
{}}
void xop_f239_0(uae_u32 opcode) /* FPP */
{
{}}
void xop_f23a_0(uae_u32 opcode) /* FPP */
{
	uae_u32 dstreg = 2;
{}}
void xop_f23b_0(uae_u32 opcode) /* FPP */
{
	uae_u32 dstreg = 3;
{}}
void xop_f23c_0(uae_u32 opcode) /* FPP */
{
{}}
void xop_f240_0(uae_u32 opcode) /* FScc */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_f248_0(uae_u32 opcode) /* FDBcc */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_f250_0(uae_u32 opcode) /* FScc */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_f258_0(uae_u32 opcode) /* FScc */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_f260_0(uae_u32 opcode) /* FScc */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_f268_0(uae_u32 opcode) /* FScc */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_f270_0(uae_u32 opcode) /* FScc */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_f278_0(uae_u32 opcode) /* FScc */
{
{}}
void xop_f279_0(uae_u32 opcode) /* FScc */
{
{}}
void xop_f27a_0(uae_u32 opcode) /* FTRAPcc */
{
{}}
void xop_f27b_0(uae_u32 opcode) /* FTRAPcc */
{
{}}
void xop_f27c_0(uae_u32 opcode) /* FTRAPcc */
{
{}}
void xop_f280_0(uae_u32 opcode) /* FBcc */
{
	uae_u32 srcreg = (opcode & 63);
{}}
void xop_f2c0_0(uae_u32 opcode) /* FBcc */
{
	uae_u32 srcreg = (opcode & 63);
{}}
void xop_f310_0(uae_u32 opcode) /* FSAVE */
{
	uae_u32 srcreg = (opcode & 7);
}
void xop_f320_0(uae_u32 opcode) /* FSAVE */
{
	uae_u32 srcreg = (opcode & 7);
}
void xop_f328_0(uae_u32 opcode) /* FSAVE */
{
	uae_u32 srcreg = (opcode & 7);
}
void xop_f330_0(uae_u32 opcode) /* FSAVE */
{
	uae_u32 srcreg = (opcode & 7);
}
void xop_f338_0(uae_u32 opcode) /* FSAVE */
{
}
void xop_f339_0(uae_u32 opcode) /* FSAVE */
{
}
void xop_f350_0(uae_u32 opcode) /* FRESTORE */
{
	uae_u32 srcreg = (opcode & 7);
}
void xop_f358_0(uae_u32 opcode) /* FRESTORE */
{
	uae_u32 srcreg = (opcode & 7);
}
void xop_f368_0(uae_u32 opcode) /* FRESTORE */
{
	uae_u32 srcreg = (opcode & 7);
}
void xop_f370_0(uae_u32 opcode) /* FRESTORE */
{
	uae_u32 srcreg = (opcode & 7);
}
void xop_f378_0(uae_u32 opcode) /* FRESTORE */
{
}
void xop_f379_0(uae_u32 opcode) /* FRESTORE */
{
}
void xop_f37a_0(uae_u32 opcode) /* FRESTORE */
{
}
void xop_f37b_0(uae_u32 opcode) /* FRESTORE */
{
}
void xop_f408_0(uae_u32 opcode) /* CINVL */
{
	uae_u32 srcreg = ((opcode >> 6) & 3);
	uae_u32 dstreg = opcode & 7;
}
void xop_f410_0(uae_u32 opcode) /* CINVP */
{
	uae_u32 srcreg = ((opcode >> 6) & 3);
	uae_u32 dstreg = opcode & 7;
}
void xop_f418_0(uae_u32 opcode) /* CINVA */
{
	uae_u32 srcreg = ((opcode >> 6) & 3);
}
void xop_f419_0(uae_u32 opcode) /* CINVA */
{
	uae_u32 srcreg = ((opcode >> 6) & 3);
}
void xop_f41a_0(uae_u32 opcode) /* CINVA */
{
	uae_u32 srcreg = ((opcode >> 6) & 3);
}
void xop_f41b_0(uae_u32 opcode) /* CINVA */
{
	uae_u32 srcreg = ((opcode >> 6) & 3);
}
void xop_f41c_0(uae_u32 opcode) /* CINVA */
{
	uae_u32 srcreg = ((opcode >> 6) & 3);
}
void xop_f41d_0(uae_u32 opcode) /* CINVA */
{
	uae_u32 srcreg = ((opcode >> 6) & 3);
}
void xop_f41e_0(uae_u32 opcode) /* CINVA */
{
	uae_u32 srcreg = ((opcode >> 6) & 3);
}
void xop_f41f_0(uae_u32 opcode) /* CINVA */
{
	uae_u32 srcreg = ((opcode >> 6) & 3);
}
void xop_f428_0(uae_u32 opcode) /* CPUSHL */
{
	uae_u32 srcreg = ((opcode >> 6) & 3);
	uae_u32 dstreg = opcode & 7;
}
void xop_f430_0(uae_u32 opcode) /* CPUSHP */
{
	uae_u32 srcreg = ((opcode >> 6) & 3);
	uae_u32 dstreg = opcode & 7;
}
void xop_f438_0(uae_u32 opcode) /* CPUSHA */
{
	uae_u32 srcreg = ((opcode >> 6) & 3);
}
void xop_f439_0(uae_u32 opcode) /* CPUSHA */
{
	uae_u32 srcreg = ((opcode >> 6) & 3);
}
void xop_f43a_0(uae_u32 opcode) /* CPUSHA */
{
	uae_u32 srcreg = ((opcode >> 6) & 3);
}
void xop_f43b_0(uae_u32 opcode) /* CPUSHA */
{
	uae_u32 srcreg = ((opcode >> 6) & 3);
}
void xop_f43c_0(uae_u32 opcode) /* CPUSHA */
{
	uae_u32 srcreg = ((opcode >> 6) & 3);
}
void xop_f43d_0(uae_u32 opcode) /* CPUSHA */
{
	uae_u32 srcreg = ((opcode >> 6) & 3);
}
void xop_f43e_0(uae_u32 opcode) /* CPUSHA */
{
	uae_u32 srcreg = ((opcode >> 6) & 3);
}
void xop_f43f_0(uae_u32 opcode) /* CPUSHA */
{
	uae_u32 srcreg = ((opcode >> 6) & 3);
}
void xop_f500_0(uae_u32 opcode) /* MMUOP */
{
	uae_u32 srcreg = (uae_s32)(uae_s8)((opcode >> 3) & 255);
	uae_u32 dstreg = opcode & 7;
{}}
void xop_f600_0(uae_u32 opcode) /* MOVE16 */
{
	uae_u32 srcreg = (opcode & 7);
{}}
void xop_f608_0(uae_u32 opcode) /* MOVE16 */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_f610_0(uae_u32 opcode) /* MOVE16 */
{
	uae_u32 srcreg = (opcode & 7);
{}}
void xop_f618_0(uae_u32 opcode) /* MOVE16 */
{
	uae_u32 dstreg = opcode & 7;
{}}
void xop_f620_0(uae_u32 opcode) /* MOVE16 */
{
	uae_u32 srcreg = (opcode & 7);
	uae_u32 dstreg = 0;
{}}
void xop_f800_0(uae_u32 opcode) /* MMUOP30B */
{
	uae_u32 srcreg = (opcode & 7);
{}}
void xop_f808_0(uae_u32 opcode) /* MMUOP30B */
{
	uae_u32 srcreg = (opcode & 7);
{}}
void xop_f810_0(uae_u32 opcode) /* MMUOP30B */
{
	uae_u32 srcreg = (opcode & 7);
{}}
void xop_f818_0(uae_u32 opcode) /* MMUOP30B */
{
	uae_u32 srcreg = (opcode & 7);
{}}
void xop_f820_0(uae_u32 opcode) /* MMUOP30B */
{
	uae_u32 srcreg = (opcode & 7);
{}}
void xop_f828_0(uae_u32 opcode) /* MMUOP30B */
{
	uae_u32 srcreg = (opcode & 7);
{}}
void xop_f830_0(uae_u32 opcode) /* MMUOP30B */
{
	uae_u32 srcreg = (opcode & 7);
{}}
void xop_f838_0(uae_u32 opcode) /* MMUOP30B */
{
{}}
void xop_f839_0(uae_u32 opcode) /* MMUOP30B */
{
{}}
void xop_f83a_0(uae_u32 opcode) /* MMUOP30B */
{
{}}
void xop_f83b_0(uae_u32 opcode) /* MMUOP30B */
{
{}}
