#include "sysdeps.h"

#include "options.h"
#include "memory.h"
#include "newcpu.h"

int get_cpu_model(void)
{
	return currprefs.cpu_model;
}

static int movec_illg (int regno)
{
	int regno2 = regno & 0x7ff;

  if (currprefs.cpu_model == 68010) {
		if (regno2 < 2)
			return 0;
		return 1;
	} else if (currprefs.cpu_model == 68020) {
		if (regno == 3)
			return 1; /* 68040/060 only */
		/* 4 is >=68040, but 0x804 is in 68020 */
		if (regno2 < 4 || regno == 0x804)
			return 0;
		return 1;
	} else if (currprefs.cpu_model == 68030) {
		if (regno2 <= 2)
			return 0;
		if (regno == 0x803 || regno == 0x804)
			return 0;
		return 1;
	} else if (currprefs.cpu_model == 68040) {
		if (regno == 0x802)
			return 1; /* 68020/030 only */
		if (regno2 < 8) return 0;
		return 1;
	}
	return 1;
}

int m68k_move2c (int regno, uae_u32 *regp)
{
  if (movec_illg (regno)) {
		op_illg (0x4E7B);
		return 0;
  } else {
  	switch (regno) {
  	case 0: regs.sfc = *regp & 7; break;
  	case 1: regs.dfc = *regp & 7; break;
  	case 2:
    	{
	      uae_u32 cacr_mask = 0;
	      if (currprefs.cpu_model == 68020)
      		cacr_mask = 0x0000000f;
	      else if (currprefs.cpu_model == 68030)
      		cacr_mask = 0x00003f1f;
	      else if (currprefs.cpu_model == 68040)
      		cacr_mask = 0x80008000;
	      regs.cacr = *regp & cacr_mask;
	      set_cpu_caches(false);
    	}
	    break;
	  /* 68040/060 only */
	  case 3: 
      regs.tcr = *regp & 0xc000;
	    break;

  	/* no differences between 68040 and 68060 */
  	case 4: regs.itt0 = *regp & 0xffffe364; break;
	  case 5: regs.itt1 = *regp & 0xffffe364; break;
	  case 6: regs.dtt0 = *regp & 0xffffe364; break;
	  case 7: regs.dtt1 = *regp & 0xffffe364; break;
			/* 68060 only */
		case 8: regs.buscr = *regp & 0xf0000000; break;

	  case 0x800: regs.usp = *regp; break;
	  case 0x801: regs.vbr = *regp; break;
	  case 0x802: regs.caar = *regp; break;
	  case 0x803: regs.msp = *regp; if (regs.m == 1) m68k_areg(regs, 7) = regs.msp; break;
	  case 0x804: regs.isp = *regp; if (regs.m == 0) m68k_areg(regs, 7) = regs.isp; break;
	  /* 68040 only */
	  case 0x805: regs.mmusr = *regp; break;
	  /* 68040 stores all bits, 68060 zeroes low 9 bits */
    case 0x806: regs.urp = *regp; break;
	  case 0x807: regs.srp = *regp; break;
	  default:
			op_illg (0x4E7B);
			return 0;
	  }
  }
	return 1;
}

int m68k_movec2 (int regno, uae_u32 *regp)
{
  if (movec_illg (regno)) {
		op_illg (0x4E7A);
		return 0;
  } else {
  	switch (regno) {
  	case 0: *regp = regs.sfc; break;
  	case 1: *regp = regs.dfc; break;
  	case 2: 
    	{
  	    uae_u32 v = regs.cacr;
  	    uae_u32 cacr_mask = 0;
  	    if (currprefs.cpu_model == 68020)
      		cacr_mask = 0x00000003;
  	    else if (currprefs.cpu_model == 68030)
      		cacr_mask = 0x00003313;
  	    else if (currprefs.cpu_model == 68040)
      		cacr_mask = 0x80008000;
  	    *regp = v & cacr_mask;
    	}
    	break;
  	case 3: *regp = regs.tcr; break;
  	case 4: *regp = regs.itt0; break;
  	case 5: *regp = regs.itt1; break;
  	case 6: *regp = regs.dtt0; break;
  	case 7: *regp = regs.dtt1; break;
  	case 8: *regp = regs.buscr; break;

  	case 0x800: *regp = regs.usp; break;
  	case 0x801: *regp = regs.vbr; break;
  	case 0x802: *regp = regs.caar; break;
  	case 0x803: *regp = regs.m == 1 ? m68k_areg(regs, 7) : regs.msp; break;
  	case 0x804: *regp = regs.m == 0 ? m68k_areg(regs, 7) : regs.isp; break;
  	case 0x805: *regp = regs.mmusr; break;
  	case 0x806: *regp = regs.urp; break;
  	case 0x807: *regp = regs.srp; break;
  	case 0x808: *regp = regs.pcr; break;

  	default:
			op_illg (0x4E7A);
			return 0;
  	}
  }
	return 1;
}

/*
* extract bitfield data from memory and return it in the MSBs
* bdata caches the unmodified data for put_bitfield()
*/
uae_u32 REGPARAM2 get_bitfield (uae_u32 src, uae_u32 bdata[2], uae_s32 offset, int width)
{
	uae_u32 tmp, res, mask;

	offset &= 7;
	mask = 0xffffffffu << (32 - width);
	switch ((offset + width + 7) >> 3) {
	case 1:
		tmp = get_byte (src);
		res = tmp << (24 + offset);
		bdata[0] = tmp & ~(mask >> (24 + offset));
		break;
	case 2:
		tmp = get_word (src);
		res = tmp << (16 + offset);
		bdata[0] = tmp & ~(mask >> (16 + offset));
		break;
	case 3:
		tmp = get_word (src);
		res = tmp << (16 + offset);
		bdata[0] = tmp & ~(mask >> (16 + offset));
		tmp = get_byte (src + 2);
		res |= tmp << (8 + offset);
		bdata[1] = tmp & ~(mask >> (8 + offset));
		break;
	case 4:
		tmp = get_long (src);
		res = tmp << offset;
		bdata[0] = tmp & ~(mask >> offset);
		break;
	case 5:
		tmp = get_long (src);
		res = tmp << offset;
		bdata[0] = tmp & ~(mask >> offset);
		tmp = get_byte (src + 4);
		res |= tmp >> (8 - offset);
		bdata[1] = tmp & ~(mask << (8 - offset));
		break;
	default:
		/* Panic? */
		write_log (_T("get_bitfield() can't happen %d\n"), (offset + width + 7) >> 3);
		res = 0;
		break;
	}
	return res;
}
/*
* write bitfield data (in the LSBs) back to memory, upper bits
* must be cleared already.
*/
void REGPARAM2 put_bitfield (uae_u32 dst, uae_u32 bdata[2], uae_u32 val, uae_s32 offset, int width)
{
	offset = (offset & 7) + width;
	switch ((offset + 7) >> 3) {
	case 1:
		put_byte (dst, bdata[0] | (val << (8 - offset)));
		break;
	case 2:
		put_word (dst, bdata[0] | (val << (16 - offset)));
		break;
	case 3:
		put_word (dst, bdata[0] | (val >> (offset - 16)));
		put_byte (dst + 2, bdata[1] | (val << (24 - offset)));
		break;
	case 4:
		put_long (dst, bdata[0] | (val << (32 - offset)));
		break;
	case 5:
		put_long (dst, bdata[0] | (val >> (offset - 32)));
		put_byte (dst + 4, bdata[1] | (val << (40 - offset)));
		break;
	default:
		write_log (_T("put_bitfield() can't happen %d\n"), (offset + 7) >> 3);
		break;
	}
}

uae_u32 REGPARAM2 _get_disp_ea_020 (uae_u32 base)
{
	uae_u16 dp = next_diword ();
  int reg = (dp >> 12) & 15;
  uae_s32 regd = regs.regs[reg];
  if ((dp & 0x800) == 0)
  	regd = (uae_s32)(uae_s16)regd;
  regd <<= (dp >> 9) & 3;
  if (dp & 0x100) {

  	uae_s32 outer = 0;
	  if (dp & 0x80) base = 0;
	  if (dp & 0x40) regd = 0;

	  if ((dp & 0x30) == 0x20) 
      base += (uae_s32)(uae_s16) next_diword ();
	  if ((dp & 0x30) == 0x30) 
      base += next_dilong ();

	  if ((dp & 0x3) == 0x2) 
      outer = (uae_s32)(uae_s16) next_diword ();
	  if ((dp & 0x3) == 0x3) 
      outer = next_dilong ();

	  if ((dp & 0x4) == 0) 
      base += regd;
	  if (dp & 0x3) 
      base = get_long (base);
	  if (dp & 0x4) 
      base += regd;

	  return base + outer;
  } else {
  	return base + (uae_s32)((uae_s8)dp) + regd;
  }
}

/*
* Compute exact number of CPU cycles taken
* by DIVU and DIVS on a 68000 processor.
*
* Copyright (c) 2005 by Jorge Cwik, pasti@fxatari.com
*
* This is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This software is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this software; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*/


/*

 The routines below take dividend and divisor as parameters.
 They return 0 if division by zero, or exact number of cycles otherwise.

 The number of cycles returned assumes a register operand.
 Effective address time must be added if memory operand.

 For 68000 only (not 68010, 68012, 68020, etc).
 Probably valid for 68008 after adding the extra prefetch cycle.


 Best and worst cases for register operand:
 (Note the difference with the documented range.)


 DIVU:

 Overflow (always): 10 cycles.
 Worst case: 136 cycles.
 Best case: 76 cycles.


 DIVS:

 Absolute overflow: 16-18 cycles.
 Signed overflow is not detected prematurely.

 Worst case: 156 cycles.
 Best case without signed overflow: 122 cycles.
 Best case with signed overflow: 120 cycles


 */

int getDivu68kCycles (uae_u32 dividend, uae_u16 divisor)
{
	int mcycles;
	uae_u32 hdivisor;
	int i;

	if(divisor == 0)
		return 0;

	// Overflow
	if((dividend >> 16) >= divisor)
		return (mcycles = 5) * 2;

	mcycles = 38;
	hdivisor = divisor << 16;

	for( i = 0; i < 15; i++) {
		uae_u32 temp;
		temp = dividend;

		dividend <<= 1;

		// If carry from shift
		if((uae_s32)temp < 0)
			dividend -= hdivisor;
		else {
			mcycles += 2;
			if(dividend >= hdivisor) {
				dividend -= hdivisor;
				mcycles--;
			}
		}
	}
	return mcycles * 2;
}

int getDivs68kCycles (uae_s32 dividend, uae_s16 divisor)
{
	int mcycles;
	uae_u32 aquot;
	int i;

	if(divisor == 0)
		return 0;

	mcycles = 6;

	if( dividend < 0)
		mcycles++;

	// Check for absolute overflow
	if(((uae_u32)abs(dividend) >> 16) >= (uae_u16)abs(divisor))
		return (mcycles + 2) * 2;

	// Absolute quotient
	aquot = (uae_u32) abs(dividend) / (uae_u16)abs(divisor);

	mcycles += 55;

	if(divisor >= 0) {
		if(dividend >= 0)
			mcycles--;
		else
			mcycles++;
	}

	// Count 15 msbits in absolute of quotient

	for( i = 0; i < 15; i++) {
		if((uae_s16)aquot >= 0)
			mcycles++;
		aquot <<= 1;
	}

	return mcycles * 2;
}

/* DIV divide by zero
 *
 * 68000 Signed: NVC=0 Z=1. Unsigned: VC=0 N=(dst>>16)<0 Z=(dst>>16)==0
 * 68020 and 68030: Signed: Z=1 NC=0. V=? (sometimes random!) Unsigned: V=1, N=(dst>>16)<0 Z=(dst>>16)==0, C=0.
 * 68040/68060 C=0.
 *
 */
void divbyzero_special (bool issigned, uae_s32 dst)
{
	if (currprefs.cpu_model == 68020 || currprefs.cpu_model == 68030) {
		CLEAR_CZNV ();
		if (issigned) {
			SET_ZFLG(1);
		} else {
			uae_s16 d = dst >> 16;
			if (d < 0)
				SET_NFLG (1);
			else if (d == 0)
				SET_ZFLG(1);
			SET_VFLG (1);
		}
	} else if (currprefs.cpu_model >= 68040) {
		SET_CFLG (0);
	} else {
		// 68000/010
		CLEAR_CZNV ();
		if (issigned) {
			SET_ZFLG(1);
		} else {
			uae_s16 d = dst >> 16;
			if (d < 0)
				SET_NFLG(1);
			else if (d == 0)
				SET_ZFLG(1);
		}
	}
}

/* DIVU overflow
 *
 * 68000: V=1 N=1
 * 68020: V=1 N=X
 * 68040: V=1
 * 68060: V=1
 *
 * X) N is set if original 32-bit destination value is negative.
 *
 */

void setdivuoverflowflags(uae_u32 dividend, uae_u16 divisor)
{
	if (currprefs.cpu_model >= 68040) {
		SET_VFLG(1);
	} else if (currprefs.cpu_model >= 68020) {
		SET_VFLG(1);
		if ((uae_s32)dividend < 0)
			SET_NFLG(1);
	} else {
		SET_VFLG(1);
		SET_NFLG(1);
	}
}

/*
 * DIVS overflow
 *
 * 68000: V = 1 N = 1
 * 68020: V = 1 ZN = X
 * 68040: V = 1
 * 68060: V = 1
 *
 * X) if absolute overflow(Check getDivs68kCycles for details) : Z = 0, N = 0
 * if not absolute overflow : N is set if internal result BYTE is negative, Z is set if it is zero!
 *
 */

void setdivsoverflowflags(uae_s32 dividend, uae_s16 divisor)
{
	if (currprefs.cpu_model >= 68040) {
		SET_VFLG(1);
	} else if (currprefs.cpu_model >= 68020) {
		SET_VFLG(1);
		// absolute overflow?
		if (((uae_u32)abs(dividend) >> 16) >= (uae_u16)abs(divisor))
			return;
		uae_u32 aquot = (uae_u32)abs(dividend) / (uae_u16)abs(divisor);
		if ((uae_s8)aquot == 0)
			SET_ZFLG(1);
		if ((uae_s8)aquot < 0)
			SET_NFLG(1);
	} else {
		SET_VFLG(1);
		SET_NFLG(1);
	}
}

/*
 * CHK.W undefined flags
 *
 * 68000: CV=0. Z: dst==0. N: dst < 0. !N: dst > src.
 * 68020: Z: dst==0. N: dst < 0. V: src-dst overflow. C: if dst < 0: (dst > src || src >= 0), if dst > src: (src >= 0).
 *
 */
void setchkundefinedflags(uae_s32 src, uae_s32 dst, int size)
{
	CLEAR_CZNV();
	if (currprefs.cpu_model < 68020) {
		if (dst == 0)
			SET_ZFLG(1);
		if (dst < 0)
			SET_NFLG(1);
		else if (dst > src)
			SET_NFLG(0);
	} else if (currprefs.cpu_model == 68020 || currprefs.cpu_model == 68030) {
		if (dst == 0)
			SET_ZFLG(1);
		SET_NFLG(dst < 0);
		if (dst < 0 || dst > src) {
			if (size == sz_word) {
				int flgs = ((uae_s16)(dst)) < 0;
					int flgo = ((uae_s16)(src)) < 0;
					uae_s16 val = (uae_s16)src - (uae_s16)dst;
					int flgn = val < 0;
					SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
			} else {
				int flgs = dst < 0;
					int flgo = src < 0;
					uae_s32 val = src - dst;
				int flgn = val < 0;
				SET_VFLG((flgs ^ flgo) & (flgn ^ flgo));
			}
			if (dst < 0) {
				SET_CFLG(dst > src || src >= 0);
			} else {
				SET_CFLG(src >= 0);
			}
		}
	}
}

void setchk2undefinedflags(uae_u32 lower, uae_u32 upper, uae_u32 val, int size)
{
	uae_u32 nmask;
	if (size == sz_byte)
		nmask = 0x80;
	else if (size == sz_word)
		nmask = 0x8000;
	else
		nmask = 0x80000000;

	SET_NFLG(0);
	if ((upper & nmask) && val > upper) {
		SET_NFLG(1);
	} else if (!(upper & nmask) && val > upper && val < nmask) {
		SET_NFLG(1);
	} else if (val < lower) {
		SET_NFLG(1);
	}
	SET_VFLG(0);
}

#if !defined (uae_s64)
STATIC_INLINE int div_unsigned(uae_u32 src_hi, uae_u32 src_lo, uae_u32 div, uae_u32 *quot, uae_u32 *rem)
{
	uae_u32 q = 0, cbit = 0;
	int i;

	if (div <= src_hi) {
	    return 1;
	}
	for (i = 0 ; i < 32 ; i++) {
		cbit = src_hi & 0x80000000ul;
		src_hi <<= 1;
		if (src_lo & 0x80000000ul) src_hi++;
		src_lo <<= 1;
		q = q << 1;
		if (cbit || div <= src_hi) {
			q |= 1;
			src_hi -= div;
		}
	}
	*quot = q;
	*rem = src_hi;
	return 0;
}
#endif

static void divl_overflow(void)
{
	SET_VFLG(1);
	SET_NFLG(1);
	SET_CFLG(0);
	SET_ZFLG(0);
}

void m68k_divl (uae_u32 opcode, uae_u32 src, uae_u16 extra)
{
  // Done in caller
  //if (src == 0) {
  //  Exception_cpu (5);
  //  return;
  //}
#if defined(uae_s64)
  if (extra & 0x800) {
  	/* signed variant */
  	uae_s64 a = (uae_s64)(uae_s32)m68k_dreg(regs, (extra >> 12) & 7);
  	uae_s64 quot, rem;

  	if (extra & 0x400) {
	    a &= 0xffffffffu;
	    a |= (uae_s64)m68k_dreg(regs, extra & 7) << 32;
  	}

		if ((uae_u64)a == 0x8000000000000000UL && src == ~0u) {
			divl_overflow();
		} else {
    	rem = a % (uae_s64)(uae_s32)src;
    	quot = a / (uae_s64)(uae_s32)src;
    	if ((quot & UVAL64(0xffffffff80000000)) != 0
	      && (quot & UVAL64(0xffffffff80000000)) != UVAL64(0xffffffff80000000))
    	{
  			divl_overflow();
    	} else {
	      if (((uae_s32)rem < 0) != ((uae_s64)a < 0)) rem = -rem;
	      SET_VFLG (0);
	      SET_CFLG (0);
	      SET_ZFLG (((uae_s32)quot) == 0);
	      SET_NFLG (((uae_s32)quot) < 0);
	      m68k_dreg(regs, extra & 7) = (uae_u32)rem;
	      m68k_dreg(regs, (extra >> 12) & 7) = (uae_u32)quot;
			}
  	}
  } else {
  	/* unsigned */
  	uae_u64 a = (uae_u64)(uae_u32)m68k_dreg(regs, (extra >> 12) & 7);
  	uae_u64 quot, rem;

  	if (extra & 0x400) {
	    a &= 0xffffffffu;
	    a |= (uae_u64)m68k_dreg(regs, extra & 7) << 32;
  	}
  	rem = a % (uae_u64)src;
  	quot = a / (uae_u64)src;
  	if (quot > 0xffffffffu) {
			divl_overflow();
  	} else {
	    SET_VFLG (0);
	    SET_CFLG (0);
	    SET_ZFLG (((uae_s32)quot) == 0);
	    SET_NFLG (((uae_s32)quot) < 0);
	    m68k_dreg(regs, extra & 7) = (uae_u32)rem;
	    m68k_dreg(regs, (extra >> 12) & 7) = (uae_u32)quot;
  	}
  }
#else
  if (extra & 0x800) {
  	/* signed variant */
  	uae_s32 lo = (uae_s32)m68k_dreg(regs, (extra >> 12) & 7);
  	uae_s32 hi = lo < 0 ? -1 : 0;
  	uae_s32 save_high;
  	uae_u32 quot, rem;
  	uae_u32 sign;

  	if (extra & 0x400) {
	    hi = (uae_s32)m68k_dreg(regs, extra & 7);
  	}
  	save_high = hi;
  	sign = (hi ^ src);
  	if (hi < 0) {
	    hi = ~hi;
	    lo = -lo;
	    if (lo == 0) hi++;
  	}
  	if ((uae_s32)src < 0) src = -src;
  	if (div_unsigned(hi, lo, src, &quot, &rem) ||
	    (sign & 0x80000000) ? quot > 0x80000000 : quot > 0x7fffffff) {
			divl_overflow();
  	} else {
	    if (sign & 0x80000000) quot = -quot;
	    if (((uae_s32)rem < 0) != (save_high < 0)) rem = -rem;
	    SET_VFLG (0);
	    SET_CFLG (0);
	    SET_ZFLG (((uae_s32)quot) == 0);
	    SET_NFLG (((uae_s32)quot) < 0);
	    m68k_dreg(regs, extra & 7) = rem;
	    m68k_dreg(regs, (extra >> 12) & 7) = quot;
  	}
  } else {
  	/* unsigned */
  	uae_u32 lo = (uae_u32)m68k_dreg(regs, (extra >> 12) & 7);
  	uae_u32 hi = 0;
  	uae_u32 quot, rem;

  	if (extra & 0x400) {
	    hi = (uae_u32)m68k_dreg(regs, extra & 7);
  	}
  	if (div_unsigned(hi, lo, src, &quot, &rem)) {
			divl_overflow();
  	} else {
	    SET_VFLG (0);
	    SET_CFLG (0);
	    SET_ZFLG (((uae_s32)quot) == 0);
	    SET_NFLG (((uae_s32)quot) < 0);
	    m68k_dreg(regs, extra & 7) = rem;
	    m68k_dreg(regs, (extra >> 12) & 7) = quot;
  	}
  }
#endif
}

#if !defined (uae_s64)
STATIC_INLINE void mul_unsigned(uae_u32 src1, uae_u32 src2, uae_u32 *dst_hi, uae_u32 *dst_lo)
{
	uae_u32 r0 = (src1 & 0xffff) * (src2 & 0xffff);
	uae_u32 r1 = ((src1 >> 16) & 0xffff) * (src2 & 0xffff);
	uae_u32 r2 = (src1 & 0xffff) * ((src2 >> 16) & 0xffff);
	uae_u32 r3 = ((src1 >> 16) & 0xffff) * ((src2 >> 16) & 0xffff);
	uae_u32 lo;

	lo = r0 + ((r1 << 16) & 0xffff0000ul);
	if (lo < r0) r3++;
	r0 = lo;
	lo = r0 + ((r2 << 16) & 0xffff0000ul);
	if (lo < r0) r3++;
	r3 += ((r1 >> 16) & 0xffff) + ((r2 >> 16) & 0xffff);
	*dst_lo = lo;
	*dst_hi = r3;
}
#endif

void m68k_mull (uae_u32 opcode, uae_u32 src, uae_u16 extra)
{
#if defined(uae_s64)
  if (extra & 0x800) {
  	/* signed */
  	uae_s64 a = (uae_s64)(uae_s32)m68k_dreg(regs, (extra >> 12) & 7);

  	a *= (uae_s64)(uae_s32)src;
  	SET_VFLG (0);
  	SET_CFLG (0);
		if (extra & 0x400) {
			// 32 * 32 = 64
			m68k_dreg(regs, (extra >> 12) & 7) = (uae_u32)a;
	    m68k_dreg(regs, extra & 7) = (uae_u32)(a >> 32);
    	SET_ZFLG (a == 0);
    	SET_NFLG (a < 0);
		} else {
			// 32 * 32 = 32
			uae_s32 b = (uae_s32)a;
			m68k_dreg(regs, (extra >> 12) & 7) = (uae_u32)a;
      if ((a & UVAL64 (0xffffffff80000000)) != 0 && (a & UVAL64(0xffffffff80000000)) != UVAL64(0xffffffff80000000)) {
  	    SET_VFLG (1);
			}
			SET_ZFLG(b == 0);
			SET_NFLG(b < 0);
	  }
  } else {
	  /* unsigned */
	  uae_u64 a = (uae_u64)(uae_u32)m68k_dreg(regs, (extra >> 12) & 7);

  	a *= (uae_u64)src;
	  SET_VFLG (0);
	  SET_CFLG (0);
		if (extra & 0x400) {
			// 32 * 32 = 64
			m68k_dreg(regs, (extra >> 12) & 7) = (uae_u32)a;
	    m68k_dreg(regs, extra & 7) = (uae_u32)(a >> 32);
	    SET_ZFLG (a == 0);
	    SET_NFLG (((uae_s64)a) < 0);
		} else {
			// 32 * 32 = 32
			uae_s32 b = (uae_s32)a;
			m68k_dreg(regs, (extra >> 12) & 7) = (uae_u32)a;
			if ((a & UVAL64(0xffffffff00000000)) != 0) {
	      SET_VFLG (1);
			}
			SET_ZFLG(b == 0);
			SET_NFLG(b < 0);
	  }
  }
#else
  if (extra & 0x800) {
	  /* signed */
	  uae_s32 src1,src2;
	  uae_u32 dst_lo,dst_hi;
	  uae_u32 sign;

  	src1 = (uae_s32)src;
	  src2 = (uae_s32)m68k_dreg(regs, (extra >> 12) & 7);
	  sign = (src1 ^ src2);
	  if (src1 < 0) src1 = -src1;
	  if (src2 < 0) src2 = -src2;
	  mul_unsigned((uae_u32)src1,(uae_u32)src2,&dst_hi,&dst_lo);
	  if (sign & 0x80000000) {
	  	dst_hi = ~dst_hi;
	  	dst_lo = -dst_lo;
	  	if (dst_lo == 0) dst_hi++;
	  }
  	SET_VFLG (0);
	  SET_CFLG (0);
		if (extra & 0x400) {
			m68k_dreg(regs, extra & 7) = dst_hi;
	  SET_ZFLG (dst_hi == 0 && dst_lo == 0);
	  SET_NFLG (((uae_s32)dst_hi) < 0);
		} else {
			if ((dst_hi != 0 || (dst_lo & 0x80000000) != 0) && ((dst_hi & 0xffffffff) != 0xffffffff || (dst_lo & 0x80000000) != 0x80000000)) {
	      SET_VFLG (1);
			}
			SET_ZFLG(dst_lo == 0);
			SET_NFLG(((uae_s32)dst_lo) < 0);
  	}
  	m68k_dreg(regs, (extra >> 12) & 7) = dst_lo;
  } else {
  	/* unsigned */
  	uae_u32 dst_lo,dst_hi;

  	mul_unsigned(src,(uae_u32)m68k_dreg(regs, (extra >> 12) & 7),&dst_hi,&dst_lo);

  	SET_VFLG (0);
  	SET_CFLG (0);
		if (extra & 0x400) {
			m68k_dreg(regs, extra & 7) = dst_hi;
    	SET_ZFLG (dst_hi == 0 && dst_lo == 0);
    	SET_NFLG (((uae_s32)dst_hi) < 0);
		} else {
			if (dst_hi != 0) {
  	    SET_VFLG (1);
			}
			SET_ZFLG(dst_lo == 0);
			SET_NFLG(((uae_s32)dst_lo) < 0);
  	}
  	m68k_dreg(regs, (extra >> 12) & 7) = dst_lo;
  }
#endif
}

uae_u32 exception_pc(int nr)
{
	// bus error, address error, illegal instruction, privilege violation, a-line, f-line
	if (nr == 2 || nr == 3 || nr == 4 || nr == 8 || nr == 10 || nr == 11)
		return regs.instruction_pc;
	return m68k_getpc ();
}

void Exception_build_stack_frame(uae_u32 oldpc, uae_u32 currpc, uae_u32 ssw, int nr, int format)
{
  switch (format) {
    case 0x0: // four word stack frame
	  case 0x1: // throwaway four word stack frame
      break;
    case 0x2: // six word stack frame
      m68k_areg (regs, 7) -= 4;
      x_put_long (m68k_areg (regs, 7), oldpc);
      break;
		case 0x3: // floating point post-instruction stack frame (68040)
			m68k_areg (regs, 7) -= 4;
			x_put_long (m68k_areg (regs, 7), regs.fp_ea);
			break;
    case 0x4: // floating point unimplemented stack frame (68LC040, 68EC040)
			m68k_areg (regs, 7) -= 4;
			x_put_long (m68k_areg (regs, 7), ssw);
			m68k_areg (regs, 7) -= 4;
			x_put_long (m68k_areg (regs, 7), oldpc);
			break;
  	case 0x8: // address error (68010)
  		for (int i = 0; i < 15; i++) {
  			m68k_areg(regs, 7) -= 2;
  			x_put_word(m68k_areg(regs, 7), 0);
  		}
  		m68k_areg(regs, 7) -= 2;
  		x_put_word(m68k_areg(regs, 7), 0); // version
  		m68k_areg(regs, 7) -= 2;
  		x_put_word(m68k_areg(regs, 7), regs.opcode); // instruction input buffer
  		m68k_areg(regs, 7) -= 2;
  		x_put_word(m68k_areg(regs, 7), 0); // unused
  		m68k_areg(regs, 7) -= 2;
  		x_put_word(m68k_areg(regs, 7), 0); // data input buffer
  		m68k_areg(regs, 7) -= 2;
  		x_put_word(m68k_areg(regs, 7), 0); // unused
  		m68k_areg(regs, 7) -= 2;
  		x_put_word(m68k_areg(regs, 7), 0); // data output buffer
  		m68k_areg(regs, 7) -= 2;
  		x_put_word(m68k_areg(regs, 7), 0); // unused
  		m68k_areg(regs, 7) -= 4;
  		x_put_long(m68k_areg(regs, 7), 0); // fault addr
  		m68k_areg(regs, 7) -= 2;
  		x_put_word(m68k_areg(regs, 7), ssw); // ssw
  		break;
  	case 0xA:
  		// short bus cycle fault stack frame (68020, 68030)
  		// used when instruction's last write causes bus fault
  		m68k_areg(regs, 7) -= 4;
			x_put_long(m68k_areg(regs, 7), 0);
  		m68k_areg(regs, 7) -= 4;
  		// Data output buffer = value that was going to be written
  		x_put_long(m68k_areg(regs, 7), 0);
  		m68k_areg(regs, 7) -= 4;
			x_put_long(m68k_areg(regs, 7), regs.irc);  // Internal register (opcode storage)
  		m68k_areg(regs, 7) -= 4;
  		x_put_long(m68k_areg(regs, 7), 0); // data cycle fault address
  		m68k_areg(regs, 7) -= 2;
  		x_put_word(m68k_areg(regs, 7), 0);  // Instr. pipe stage B
  		m68k_areg(regs, 7) -= 2;
  		x_put_word(m68k_areg(regs, 7), 0);  // Instr. pipe stage C
  		m68k_areg(regs, 7) -= 2;
  		x_put_word(m68k_areg(regs, 7), ssw);
  		m68k_areg(regs, 7) -= 2;
  		x_put_word(m68k_areg(regs, 7), 0); // = mmu030_state[1]);
  		break;
		default:
      write_log(_T("Unknown exception stack frame format: %X\n"), format);
      return;
    }
    m68k_areg (regs, 7) -= 2;
    x_put_word (m68k_areg (regs, 7), (format << 12) | (nr * 4));
    m68k_areg (regs, 7) -= 4;
    x_put_long (m68k_areg (regs, 7), currpc);
    m68k_areg (regs, 7) -= 2;
    x_put_word (m68k_areg (regs, 7), regs.sr);
}

void Exception_build_stack_frame_common(uae_u32 oldpc, uae_u32 currpc, int nr)
{
	if (nr == 5 || nr == 6 || nr == 7 || nr == 9) {
		if (currprefs.cpu_model <= 68010)
			Exception_build_stack_frame(oldpc, currpc, 0, nr, 0x0);
		else
  		Exception_build_stack_frame(oldpc, currpc, 0, nr, 0x2);
	} else if (nr == 60 || nr == 61) {
		Exception_build_stack_frame(oldpc, regs.instruction_pc, 0, nr, 0x0);
	} else if (nr >= 48 && nr <= 55) {
		if (regs.fpu_exp_pre) {
			Exception_build_stack_frame(oldpc, regs.instruction_pc, 0, nr, 0x0);
		} else { /* post-instruction */
			Exception_build_stack_frame(oldpc, currpc, 0, nr, 0x3);
		}
	} else if (nr == 11 && regs.fp_unimp_ins) {
		regs.fp_unimp_ins = false;
		if (currprefs.cpu_model == 68040 && currprefs.fpu_model == 0) {
			Exception_build_stack_frame(regs.fp_ea, currpc, regs.instruction_pc, nr, 0x4);
		} else {
			Exception_build_stack_frame(regs.fp_ea, currpc, 0, nr, 0x2);
		}
	} else {
		Exception_build_stack_frame(oldpc, currpc, 0, nr, 0x0);
	}
}

void Exception_build_68000_address_error_stack_frame(uae_u16 mode, uae_u16 opcode, uaecptr fault_addr, uaecptr pc)
{
	// undocumented bits seem to contain opcode
	mode |= opcode & ~31;
	m68k_areg(regs, 7) -= 14;
	x_put_word(m68k_areg(regs, 7) + 0, mode);
	x_put_long(m68k_areg(regs, 7) + 2, fault_addr);
	x_put_word(m68k_areg(regs, 7) + 6, opcode);
	x_put_word(m68k_areg(regs, 7) + 8, regs.sr);
	x_put_long(m68k_areg(regs, 7) + 10, pc);
}

// Low word: Z and N
void ccr_68000_long_move_ae_LZN(uae_s32 src)
{
	CLEAR_CZNV();
	uae_s16 vsrc = (uae_s16)(src & 0xffff);
	SET_ZFLG(vsrc == 0);
	SET_NFLG(vsrc < 0);
}

// Low word: N only
void ccr_68000_long_move_ae_LN(uae_s32 src)
{
	CLEAR_CZNV();
	uae_s16 vsrc = (uae_s16)(src & 0xffff);
	SET_NFLG(vsrc < 0);
}

// High word: N and Z clear.
void ccr_68000_long_move_ae_HNZ(uae_s32 src)
{
	uae_s16 vsrc = (uae_s16)(src >> 16);
	if(vsrc < 0) {
		SET_NFLG(1);
		SET_ZFLG(0);
	} else if (vsrc) {
		SET_NFLG(0);
		SET_ZFLG(0);
	} else {
		SET_NFLG(0);
	}
}

// Set normally.
void ccr_68000_long_move_ae_normal(uae_s32 src)
{
	CLEAR_CZNV();
	SET_ZFLG(src == 0);
	SET_NFLG(src < 0);
}
void ccr_68000_word_move_ae_normal(uae_s16 src)
{
	CLEAR_CZNV();
	SET_ZFLG(src == 0);
	SET_NFLG(src < 0);
}
